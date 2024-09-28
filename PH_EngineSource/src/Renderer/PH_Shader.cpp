#include "PH_Shader.h"
#include "Core/PH_Device.h"

#include "Core/PH_Pipeline.h"
#include "Systems/PH_FileSystem.h"
#include "Core/PH_REngineContext.h"

#include <sstream>

#include "SpirVReflectLibStandalone/spirv_reflect.h"

namespace PhosphorEngine {

	// FNV-1a 32bit hashing algorithm.
	constexpr uint32_t fnv1a_32(char const* s, uint64 count)
	{
		return ((count ? fnv1a_32(s, count - 1) : 2166136261u) ^ s[count]) * 16777619u;
	}

	uint32 ShaderDescriptorLayoutInfoHash(VkDescriptorSetLayoutCreateInfo& info)
	{
		std::stringstream ss{};

		ss << info.flags;
		ss << info.bindingCount;
		for (uint32 index = 0; index < info.bindingCount; index++)
		{
			const VkDescriptorSetLayoutBinding& binding = info.pBindings[index];

			ss << binding.binding;
			ss << binding.descriptorCount;
			ss << binding.descriptorType;
			ss << binding.stageFlags;
		}

		auto str = ss.str();
		return fnv1a_32(str.c_str(), str.length());
	}

	bool loadComputeShader(const char* filePath, PH_Pipeline& Pipeline, VkPipelineLayout& layout)
	{
		ShaderModule computeModule;
		if (!ShaderModule::loadAndCreateShaderModule(filePath, computeModule))
		{
			PH_ERROR("Failed to load compute shader!");
			return false;
		}

		PH_ShaderEffect computeEffect{};
		computeEffect.AddStage(&computeModule, VK_SHADER_STAGE_COMPUTE_BIT);
		computeEffect.ReflectLayout(nullptr, 0);

		ComputePipelineConfigInfo CompouteCreateInfo{ PH_ShaderEffect::getShaderStageConfigInfo(VK_SHADER_STAGE_COMPUTE_BIT, computeModule.module), computeEffect.builtLayout };

		Pipeline = PH_PipelineBuilder::BuildComputePipeline(CompouteCreateInfo, true);
		layout = computeEffect.builtLayout;
		ShaderModule::destroyShaderModule(computeModule);

		return true;
	}

	void ShaderModule::createShaderModule(ShaderModule& shaderModule)
	{
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = shaderModule.code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderModule.code.data());

		PH_VULKAN_CHECK_MSG(vkCreateShaderModule(PH_REngineContext::GetLogicalDevice(), &createInfo, nullptr, &shaderModule.module),
			"Failed to create shader module!");
	}

	bool ShaderModule::loadShaderModule(const char* filePath, ShaderModule& shaderModule)
	{
		PH_String FilePath{ filePath };
		auto ShaderFile = PH_FileSystem::ReadFile(FilePath);
		if (ShaderFile.size() < 1)
		{
			PH_ERROR("Failed to load shader module on path: %s", filePath);
			return false;
		}
		shaderModule.code = std::move(ShaderFile);

		return true;
	}

	bool ShaderModule::loadAndCreateShaderModule(const char* filePath, ShaderModule& shaderModule)
	{
		bool loaded = loadShaderModule(filePath, shaderModule);
		if (!loaded)
		{
			return loaded;
		}
		createShaderModule(shaderModule);
		return true;
	}

	void ShaderModule::destroyShaderModule(ShaderModule& shaderModule)
	{
		vkDestroyShaderModule(PH_REngineContext::GetDevice().GetDevice(), shaderModule.module, PH_REngineContext::GetDevice().GetAllocator());
	}

	void PH_ShaderEffect::AddStage(ShaderModule* shaderModule, VkShaderStageFlagBits stage)
	{
		ShaderStage shaderStage{ shaderModule, stage };
		stages.push_back(shaderStage);
	}

	void PH_ShaderEffect::ReflectLayout(ReflectionOverrides* overrides, uint32 overrideCount)
	{
		PH_DynamicArray<PH_DescriptorSetLayout::Builder> setLayoutsBuilders{};

		PH_DynamicArray<VkPushConstantRange> constantRanges;

		for (auto& s : stages) {

			SpvReflectShaderModule spvmodule;
			SpvReflectResult result = spvReflectCreateShaderModule(s.shaderModule->code.size() * sizeof(uint8), s.shaderModule->code.data(), &spvmodule);

			uint32 count = 0;
			result = spvReflectEnumerateDescriptorSets(&spvmodule, &count, NULL);
			PH_ASSERT(result == SPV_REFLECT_RESULT_SUCCESS);

			PH_DynamicArray<SpvReflectDescriptorSet*> sets{ count };
			result = spvReflectEnumerateDescriptorSets(&spvmodule, &count, sets.data());
			PH_ASSERT(result == SPV_REFLECT_RESULT_SUCCESS);

			for (uint64 i_set = 0; i_set < sets.size(); ++i_set) {
				
				const SpvReflectDescriptorSet& refl_set = *(sets[i_set]);

				PH_DescriptorSetLayout::Builder layoutBuilder{};

				layoutBuilder.GetBindings().resize(refl_set.binding_count);
				for (uint32 i_binding = 0; i_binding < refl_set.binding_count; ++i_binding) {
					const SpvReflectDescriptorBinding& refl_binding = *(refl_set.bindings[i_binding]);
					VkDescriptorSetLayoutBinding& layoutBinding = layoutBuilder.GetBinding(i_binding);

					layoutBinding.binding = refl_binding.binding;
					layoutBinding.descriptorType = static_cast<VkDescriptorType>(refl_binding.descriptor_type);

					for (int32 ov = 0; ov < overrideCount; ov++)
					{
						if (strcmp(refl_binding.name, overrides[ov].name) == 0) {
							layoutBinding.descriptorType = overrides[ov].overridenType;
						}
					}

					layoutBinding.descriptorCount = 1;
					for (uint32 i_dim = 0; i_dim < refl_binding.array.dims_count; ++i_dim) {
						layoutBinding.descriptorCount *= refl_binding.array.dims[i_dim];
					}
					layoutBinding.stageFlags = static_cast<VkShaderStageFlagBits>(spvmodule.shader_stage);

					ReflectedBinding reflected;
					reflected.binding = layoutBinding.binding;
					reflected.set = refl_set.set;
					reflected.type = layoutBinding.descriptorType;

					bindings[refl_binding.name] = reflected;
				}
				layoutBuilder.SetLayoutSetNumber(refl_set.set);

				setLayoutsBuilders.push_back(layoutBuilder);
			}

			//pushconstants	

			result = spvReflectEnumeratePushConstantBlocks(&spvmodule, &count, NULL);
			PH_ASSERT(result == SPV_REFLECT_RESULT_SUCCESS);

			PH_DynamicArray<SpvReflectBlockVariable*> pconstants{count};
			result = spvReflectEnumeratePushConstantBlocks(&spvmodule, &count, pconstants.data());
			PH_ASSERT(result == SPV_REFLECT_RESULT_SUCCESS);

			if (count > 0) {
				VkPushConstantRange pcs{};
				pcs.offset = pconstants[0]->offset;
				pcs.size = pconstants[0]->size;
				pcs.stageFlags = s.stage;

				constantRanges.push_back(pcs);
			}
		}

		PH_StaticArray<PH_DescriptorSetLayout::Builder, 4> mergedLayoutsBuilders;

		for (int32 i = 0; i < 4; i++) {

			auto& ly = mergedLayoutsBuilders[i];

			ly.SetLayoutSetNumber(i);

			PH_Map<int32, VkDescriptorSetLayoutBinding> binds;
			for (auto& s : setLayoutsBuilders) {
				if (s.GetLayoutSetNumber() == i) {
					for (auto& b : s.GetBindings())
					{
						auto it = binds.find(b.binding);
						if (it == binds.end())
						{
							binds[b.binding] = b;
						}
						else {
							//merge flags
							binds[b.binding].stageFlags |= b.stageFlags;
						}
					}
				}
			}
			for (auto [k, v] : binds)
			{
				ly.addBinding(v);
			}

			//sort the bindings, for hash purposes
			std::sort(ly.GetBindings().begin(), ly.GetBindings().end(), [](VkDescriptorSetLayoutBinding& a, VkDescriptorSetLayoutBinding& b) {
				return a.binding < b.binding;
				});

			if (ly.GetBindings().size() > 0)
			{
				//ly.buildRawVulkanLayout(Device, setLayouts[i]);
				setLayouts[i] = PH_REngineContext::GetDescriptorLayoutCache().CreateSetLayout(ly);
			}
			else {
				setHashes[i] = 0;
				setLayouts[i] = VK_NULL_HANDLE;
			}
		}

		//we start from just the default empty pipeline layout info
		VkPipelineLayoutCreateInfo MeshPipelineLayoutInfo = PH_PipelineLayoutBuilder::defaultPipelineLayoutInfo();

		MeshPipelineLayoutInfo.pPushConstantRanges = constantRanges.data();
		MeshPipelineLayoutInfo.pushConstantRangeCount = (uint32)constantRanges.size();

		PH_StaticArray<VkDescriptorSetLayout, 4> compactedLayouts;
		int32 s = 0;
		for (int32 i = 0; i < 4; i++) {
			if (setLayouts[i] != VK_NULL_HANDLE) {
				compactedLayouts[s] = setLayouts[i];
				s++;
			}
		}

		MeshPipelineLayoutInfo.setLayoutCount = s;
		MeshPipelineLayoutInfo.pSetLayouts = compactedLayouts.data();

		PH_PipelineLayoutBuilder::BuildPipelineLayout(builtLayout, MeshPipelineLayoutInfo);
	}

	void PH_ShaderEffect::FillStages(PH_DynamicArray<VkPipelineShaderStageCreateInfo>& pipelineStages)
	{
		pipelineStages.clear();
		for (auto& s : stages)
		{
			pipelineStages.push_back(getShaderStageConfigInfo(s.stage, s.shaderModule->module));
		}
	}

	VkPipelineShaderStageCreateInfo PH_ShaderEffect::getShaderStageConfigInfo(VkShaderStageFlagBits stage, VkShaderModule shaderModule)
	{
		VkPipelineShaderStageCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		info.pNext = nullptr;

		//shader stage
		info.stage = stage;
		//module containing the code for this shader stage
		info.module = shaderModule;
		//the entry point of the shader
		info.pName = "main";
		return info;
	}

	ShaderModule* ShaderCache::GetShader(const PH_String& path)
	{
		auto it = ModulesCache.find(path);
		if (it == ModulesCache.end())
		{
			ShaderModule newShader;

			bool result = ShaderModule::loadAndCreateShaderModule(path.c_str(), newShader);
			if (!result)
			{
				PH_TRACE("Error when compilling shader: %s", path.c_str());
				return nullptr;
			}

			ModulesCache[path] = newShader;
		}
		return &ModulesCache[path];
	}

	void ShaderCache::Initialize()
	{
		PH_ASSERT(!shaderCache.IsValid());

		shaderCache = PH_UniqueMemoryHandle<ShaderCache>::Create();
	}

	void ShaderCache::Destroy()
	{
		PH_ASSERT(shaderCache.IsValid());
		auto& REngineContext = PH_REngineContext::GetRenderEngineContext();
		for (auto& shader : shaderCache->ModulesCache) 
		{ 
			ShaderModule::destroyShaderModule(shader.second);
		}
		shaderCache.Release();
	}

	void ShaderDescriptorBinder::BindBuffer(const char* name, const VkDescriptorBufferInfo& bufferInfo)
	{
		BindDynamicBuffer(name, -1, bufferInfo);
	}

	void ShaderDescriptorBinder::BindDynamicBuffer(const char* name, uint32_t offset, const VkDescriptorBufferInfo& bufferInfo)
	{
		auto binding = shaders->bindings.find(name);
		if (binding != shaders->bindings.end())
		{
			const PH_ShaderEffect::ReflectedBinding& bind = (*binding).second;

			for (auto& write : bufferWrites)
			{
				if (write.dstBinding == bind.binding &&
					write.dstSet == bind.set)
				{
					if (write.bufferInfo.buffer != bufferInfo.buffer ||
						write.bufferInfo.range != bufferInfo.range ||
						write.bufferInfo.offset != bufferInfo.offset)
					{
						write.bufferInfo = bufferInfo;
						write.dynamicOffset = offset;

						cachedDescriptorSets[write.dstSet] = VK_NULL_HANDLE;
					}
					else
					{
						write.dynamicOffset = offset;
					}
				
					return;
				}
			}

			BufferWriteDescriptor newWrite;
			newWrite.dstSet = bind.set;
			newWrite.dstBinding = bind.binding;
			newWrite.descriptorType = bind.type;
			newWrite.bufferInfo = bufferInfo;
			newWrite.dynamicOffset = offset;

			cachedDescriptorSets[bind.set] = VK_NULL_HANDLE;

			bufferWrites.push_back(newWrite);
		}
	}

	void ShaderDescriptorBinder::ApplyBinds(VkCommandBuffer cmd)
	{
		for (int32 index = 0; index < 2; index++)
		{
			if (cachedDescriptorSets[index] != VK_NULL_HANDLE)
			{
				vkCmdBindDescriptorSets(cmd, 
					VK_PIPELINE_BIND_POINT_GRAPHICS,
					shaders->builtLayout, 
					index, 1, 
					&cachedDescriptorSets[index],
					setOffsets[index].count, 
					setOffsets[index].offsets.data());
			}
		}
	}

	void ShaderDescriptorBinder::BuildSets(VkDevice device, PH_DescriptorPoolAllocator& allocator)
	{
		PH_StaticArray<PH_DynamicArray<VkWriteDescriptorSet>, 4> writes{};

		std::sort(bufferWrites.begin(), bufferWrites.end(), [](BufferWriteDescriptor& a, BufferWriteDescriptor& b)
			{
				if (b.dstSet != a.dstSet)
				{
					return a.dstBinding < b.dstBinding;
				}
				return false;
			});

		for (auto& s : setOffsets)
		{
			s.count = 0;
		}

		for (BufferWriteDescriptor& w : bufferWrites)
		{
			VkWriteDescriptorSet write = w.GetWriteDescriptorSet();

			writes[w.dstSet].push_back(write);

			if (w.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC || w.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC)
			{
				DynOffsets& offsetSet = setOffsets[w.dstSet];
				offsetSet.offsets[offsetSet.count] = w.dynamicOffset;
				offsetSet.count++;
			}
		}

		for (int32 index = 0; index < 4; index++)
		{
			if (writes[index].size() > 0)
			{
				if (cachedDescriptorSets[index] == VK_NULL_HANDLE)
				{
					auto layout = shaders->setLayouts[index];

					VkDescriptorSet newDescriptor{};
					allocator.allocateDescriptor(layout, newDescriptor);

					for (auto& w : writes[index])
					{
						w.dstSet = newDescriptor;
					}
					vkUpdateDescriptorSets(device, (uint32)writes[index].size(), writes[index].data(), 0, nullptr);

					cachedDescriptorSets[index] = newDescriptor;
				}
			}
		}

	}

	void ShaderDescriptorBinder::SetShader(PH_ShaderEffect* newShader)
	{
		if (shaders && shaders != newShader)
		{
			for (int32 index = 0; index < 4; index++)
			{
				if (newShader->setHashes[index] != shaders->setHashes[index])
				{
					cachedDescriptorSets[index] = VK_NULL_HANDLE;
				}
				else if (newShader->setHashes[index] == 0)
				{
					cachedDescriptorSets[index] = VK_NULL_HANDLE;
				}
			}
		}
		else
		{
			for (int32 index = 0; index < 4; index++)
			{
				cachedDescriptorSets[index] = VK_NULL_HANDLE;
			}
		}

		shaders = newShader;
	}

	VkWriteDescriptorSet ShaderDescriptorBinder::BufferWriteDescriptor::GetWriteDescriptorSet()
	{
		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.pNext = nullptr;

		write.dstBinding = dstBinding;
		write.dstSet = VK_NULL_HANDLE;
		write.descriptorCount = 1;
		write.descriptorType = descriptorType;
		write.pBufferInfo = &bufferInfo;
		return write;
	}
};