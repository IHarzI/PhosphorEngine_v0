#include "PH_MaterialSystem.h"

#include "Core/PH_REngineContext.h"
#include "PH_Shader.h"
#include "Memory/PH_Memory.h"
#include "Core/PH_Descriptors.h"

#include "Core/PH_Buffer.h"

namespace PhosphorEngine {

	PH_UniqueMemoryHandle<PH_ShaderEffect> buildEffect(PH_String vertexShader, PH_String fragmentShader) {
		PH_ShaderEffect::ReflectionOverrides overrides[] = {
			{"sceneData", VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC},
			{"cameraData", VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC}
		};

		//textured defaultlit shader
		auto effect = PH_UniqueMemoryHandle<PH_ShaderEffect>::CreateWithTag(PH_MEM_TAG_SHADER);

		effect->AddStage(ShaderCache::GetCache().GetShader(vertexShader), VK_SHADER_STAGE_VERTEX_BIT);
		if (fragmentShader.size() > 2)
		{
			effect->AddStage(ShaderCache::GetCache().GetShader(fragmentShader), VK_SHADER_STAGE_FRAGMENT_BIT);
		}

		effect->ReflectLayout(overrides, 2);

		return effect;
	}

	PH_MaterialSystem::PH_MaterialSystem()
	{
		ShaderCache::Initialize();
		buildDefaultTemplates();
	};

	void PH_MaterialSystem::cleanup()
	{
		ShaderCache::Destroy();
	}

	void PH_MaterialSystem::buildDefaultTemplates()
	{
		fillPipelineConfigs();

		//default effects	
		auto* texturedLit = buildEffect("shaders/tri_mesh_ssbo_instanced.vert.spv", "shaders/textured_lit.frag.spv").RetrieveResourse();
		auto* defaultLit = buildEffect("shaders/tri_mesh_ssbo_instanced.vert.spv", "shaders/default_lit.frag.spv").RetrieveResourse();
		auto* opaqueShadowcast = buildEffect("shaders/tri_mesh_ssbo_instanced_shadowcast.vert.spv", "").RetrieveResourse();

		//passes
		auto* texturedLitPass = buildShader(forwardPipelineConfig, texturedLit).RetrieveResourse();
		auto* defaultLitPass = buildShader(forwardPipelineConfig, defaultLit).RetrieveResourse();
		auto* opaqueShadowcastPass = buildShader(shadowPipelineConfig, opaqueShadowcast).RetrieveResourse();

		{
			EffectTemplate defaultTextured;
			defaultTextured.passShaders[MeshpassType::Transparency] = nullptr;
			defaultTextured.passShaders[MeshpassType::DirectionalShadow] = opaqueShadowcastPass;
			defaultTextured.passShaders[MeshpassType::Forward] = texturedLitPass;

			defaultTextured.defaultParameters = nullptr;
			defaultTextured.transparency = MaterialAsset::TransparencyMode::Opaque;

			templateCache["texturedPBR_opaque"] = defaultTextured;
		}
		{
			auto transparentForward = forwardPipelineConfig;
			transparentForward.colorBlendAttachment.blendEnable = VK_TRUE;
			transparentForward.colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
			transparentForward.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			transparentForward.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;

			//transparentForward._colorBlendAttachment.colorBlendOp = VK_BLEND_OP_OVERLAY_EXT;
			transparentForward.colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT;

			transparentForward.depthStencilInfo.depthWriteEnable = false;

			transparentForward.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
			//passes
			auto* transparentLitPass = buildShader(transparentForward, texturedLit).RetrieveResourse();

			EffectTemplate defaultTextured;
			defaultTextured.passShaders[MeshpassType::Transparency] = transparentLitPass;
			defaultTextured.passShaders[MeshpassType::DirectionalShadow] = nullptr;
			defaultTextured.passShaders[MeshpassType::Forward] = nullptr;

			defaultTextured.defaultParameters = nullptr;
			defaultTextured.transparency = MaterialAsset::TransparencyMode::Transparent;

			templateCache["texturedPBR_transparent"] = defaultTextured;
		}

		{
			EffectTemplate defaultColored;

			defaultColored.passShaders[MeshpassType::Transparency] = nullptr;
			defaultColored.passShaders[MeshpassType::DirectionalShadow] = opaqueShadowcastPass;
			defaultColored.passShaders[MeshpassType::Forward] = defaultLitPass;
			defaultColored.defaultParameters = nullptr;
			defaultColored.transparency = MaterialAsset::TransparencyMode::Opaque;
			templateCache["colored_opaque"] = defaultColored;
		}
	}

	PH_UniqueMemoryHandle<PH_ShaderPass> PH_MaterialSystem::buildShader(PipelineConfigInfo& PipelineConfig, PH_ShaderEffect* effect)
	{
		auto pass = PH_UniqueMemoryHandle<PH_ShaderPass>::CreateWithTag(PH_MEM_TAG_SHADER);
		pass->effect = effect;
		pass->layout = effect->builtLayout;

		PH_PipelineBuilder::FillShaders(PipelineConfig, *effect);
		pass->pipeline = PH_PipelineBuilder::BuildGraphicsPipeline(PipelineConfig,true).GetVulkanPipeline();
		return pass;
	}

	PH_Material* PH_MaterialSystem::buildMaterial(const PH_String& materialName, const MaterialData& info)
	{
		PH_Material* mat;
		//search material in the cache first in case its already built
		auto it = materialCache.find(info);
		if (it != materialCache.end())
		{
			mat = (*it).second;
			materials[materialName] = mat;
		}
		else {

			//need to build the material
			auto* newMat = PH_UniqueMemoryHandle<PH_Material>::CreateWithTag(PH_MEM_TAG_MATERIAL_INSTANCE).RetrieveResourse();
			newMat->original = &templateCache[info.baseTemplate];
			newMat->parameters = info.parameters;
			//not handled yet
			newMat->passSets[MeshpassType::DirectionalShadow] = VK_NULL_HANDLE;
			newMat->textures = info.textures;

			PH_DescriptorSetLayout::Builder LayoutBuilder{};
			for (int i = 0; i < info.textures.size(); i++)
			{
				LayoutBuilder.addBinding(i, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
			}

			auto Layout = LayoutBuilder.build();
			PH_DescriptorWriter Writer{Layout.GetReference()};
			for (int i = 0; i < info.textures.size(); i++)
			{
				VkDescriptorImageInfo imageBufferInfo;
				imageBufferInfo.sampler = info.textures[i]->Sampler;
				imageBufferInfo.imageView = info.textures[i]->ImageView;
				imageBufferInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				Writer.writeImage(i, &imageBufferInfo);
			};

			Writer.build(newMat->passSets[MeshpassType::Forward]);
			Writer.build(newMat->passSets[MeshpassType::Transparency]);
			PH_INFO("Built New Material %s", materialName);
			//add material to cache
			materialCache[info] = (newMat);
			mat = newMat;
			materials[materialName] = mat;
		}

		return mat;
	}

	PH_Material* PH_MaterialSystem::getMaterial(const PH_String& materialName)
	{
		auto it = materials.find(materialName);
		if (it != materials.end())
		{
			return(*it).second;
		}
		else {
			return nullptr;
		}
	}

	void PH_MaterialSystem::fillPipelineConfigs()
	{
		PH_PipelineBuilder::defaultPipelineConfigInfo(forwardPipelineConfig);

		forwardPipelineConfig.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL;

		forwardPipelineConfig.renderPass = PH_REngineContext::GetRenderer().getSwapChainRenderPass();
		PH_PipelineBuilder::defaultPipelineConfigInfo(shadowPipelineConfig);

		shadowPipelineConfig.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
		shadowPipelineConfig.rasterizationInfo.cullMode = VK_CULL_MODE_FRONT_BIT;

		shadowPipelineConfig.renderPass = PH_REngineContext::GetRenderer().getSwapChainRenderPass();
	}

	bool MaterialData::operator==(const MaterialData& other) const
	{
		if (other.baseTemplate.compare(baseTemplate) != 0 || other.parameters != parameters || other.textures.size() != textures.size())
		{
			return false;
		}
		else {
			//binary compare textures
			bool comp = memcmp(other.textures.data(), textures.data(), textures.size() * sizeof(textures[0])) == 0;
			return comp;
		}
	}

	uint64 MaterialData::hash() const
	{
		uint64 result = std::hash<PH_String>()(baseTemplate);

		for (const auto& b : textures)
		{
			//pack the binding data into a single int64. Not fully correct but its ok
			uint64 texture_hash = (std::hash<uint64>()((uint64)b->Sampler) << 3) && (std::hash<uint64>()((uint64)b->ImageView) >> 7);

			//shuffle the packed binding data and xor it with the main hash
			result ^= std::hash<uint64>()(texture_hash);
		}
		return size_t();
	}
};