#pragma once

#include "Core/PH_CORE.h"
#include "vulkan/vulkan.h"
#include "Core/PH_Descriptors.h"
#include "Core/PH_Pipeline.h"

#include "Containers/PH_DynamicArray.h"
#include "Containers/PH_StaticArray.h"
#include "Containers/PH_Map.h"
#include "Containers/PH_String.h"

namespace PhosphorEngine {

	using namespace Containers;

	class PH_Device;

	uint32 ShaderDescriptorLayoutInfoHash(const VkDescriptorSetLayoutCreateInfo& info);

	bool loadComputeShader(const char* filePath, PH_Pipeline& Pipeline, VkPipelineLayout& layout);

	struct ShaderModule {
		PH_DynamicArray<char> code;
		VkShaderModule module;

		static void createShaderModule(ShaderModule& shaderModule);
		static bool loadShaderModule(const char* filePath, ShaderModule& shaderModule);
		static bool loadAndCreateShaderModule(const char* filePath, ShaderModule& shaderModule);
		static void destroyShaderModule(ShaderModule& shaderModule);
	};
	
	// Shader related info
	struct PH_ShaderEffect {

		struct ReflectionOverrides {
			const char* name;
			VkDescriptorType overridenType;
		};

		void AddStage(ShaderModule* shaderModule, VkShaderStageFlagBits stage);

		void ReflectLayout(ReflectionOverrides* overrides, uint32 overrideCount);

		void FillStages(PH_DynamicArray<VkPipelineShaderStageCreateInfo>& pipelineStages);
		VkPipelineLayout builtLayout;

		struct ReflectedBinding {
			uint32_t set;
			uint32_t binding;
			VkDescriptorType type;
		};
		PH_Map<PH_String, ReflectedBinding> bindings;
		PH_StaticArray<VkDescriptorSetLayout, 4> setLayouts;
		PH_StaticArray<uint32_t, 4> setHashes;

		PH_STATIC_LOCAL VkPipelineShaderStageCreateInfo getShaderStageConfigInfo(VkShaderStageFlagBits stage, VkShaderModule shaderModule);
	private:
		struct ShaderStage {
			ShaderModule* shaderModule;
			VkShaderStageFlagBits stage;
		};

		PH_DynamicArray<ShaderStage> stages;
	};

	struct ShaderDescriptorBinder {

		struct BufferWriteDescriptor {
			int dstSet;
			int dstBinding;
			VkDescriptorType descriptorType;
			VkDescriptorBufferInfo bufferInfo;

			uint32_t dynamicOffset;

			VkWriteDescriptorSet GetWriteDescriptorSet();
		};

		void BindBuffer(const char* name, const VkDescriptorBufferInfo& bufferInfo);

		void BindDynamicBuffer(const char* name, uint32_t offset, const VkDescriptorBufferInfo& bufferInfo);

		void ApplyBinds(VkCommandBuffer cmd);

		void BuildSets(VkDevice device, PH_DescriptorPoolAllocator& allocator);

		void SetShader(PH_ShaderEffect* newShader);

		PH_StaticArray<VkDescriptorSet, 4> cachedDescriptorSets;
	private:
		struct DynOffsets {
			PH_StaticArray<uint32_t, 16> offsets;
			uint32_t count{ 0 };
		};
		PH_StaticArray<DynOffsets, 4> setOffsets;

		PH_ShaderEffect* shaders{ nullptr };
		PH_DynamicArray<BufferWriteDescriptor> bufferWrites;
	};

	class ShaderCache {

	public:
		ShaderModule* GetShader(const PH_String& path);

		PH_STATIC_LOCAL ShaderCache& GetCache() { return shaderCache.GetReference(); };
		PH_STATIC_LOCAL void Initialize();
		PH_STATIC_LOCAL void Destroy();
	private:
		PH_Map<PH_String, ShaderModule> ModulesCache{};

		PH_STATIC_LOCAL PH_INLINE PH_UniqueMemoryHandle<ShaderCache> shaderCache{};
	};
};