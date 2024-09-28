#pragma once

#include "vulkan/vulkan.h"
#include "RendererTypes.h"
#include "Core/PH_Device.h"
#include "Core/PH_Asset.h"
#include "PH_ImageTexture.h"
#include "Core/PH_Pipeline.h"
#include "PH_Renderer.h"

namespace PhosphorEngine {
	struct PH_ShaderEffect;
	class PH_DescriptorPoolAllocator;
	class PH_DescriptorLayoutCache;

	enum class VertexAttributeTemplate {
		DefaultVertex,
		DefaultVertexPosOnly
	};

	class EffectBuilder {

		VertexAttributeTemplate vertexAttrib;
		PH_ShaderEffect* effect{ nullptr };

		VkPrimitiveTopology topology;
		VkPipelineRasterizationStateCreateInfo rasterizerInfo;
		VkPipelineColorBlendAttachmentState colorBlendAttachmentInfo;
		VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
	};

	struct PH_ShaderPass {
		PH_ShaderEffect* effect{ nullptr };
		VkPipeline pipeline{ VK_NULL_HANDLE };
		VkPipelineLayout layout{ VK_NULL_HANDLE };
	};

	struct ShaderParameters
	{
	};

	struct EffectTemplate {
		PerPassData<PH_SharedMemoryHandle<PH_ShaderPass>> passShaders;

		ShaderParameters* defaultParameters;
		MaterialAsset::TransparencyMode transparency;
	};

	struct MaterialData {
		PH_DynamicArray<PH_SharedMemoryHandle<PH_ImageTexture>> textures;
		ShaderParameters* parameters;
		PH_String baseTemplate;

		bool operator==(const MaterialData& other) const;

		uint64 hash() const;
	};

	struct PH_Material {
		EffectTemplate* original;
		PerPassData<VkDescriptorSet> passSets;

		PH_DynamicArray<PH_SharedMemoryHandle<PH_ImageTexture>> textures;

		ShaderParameters* parameters;

		PH_Material& operator=(const PH_Material& other) = default;
	};

	class PH_MaterialSystem {
	public:
		PH_MaterialSystem();
		void cleanup();

		void buildDefaultTemplates();

		PH_UniqueMemoryHandle<PH_ShaderPass> buildShader(PipelineConfigInfo& PipelineConfig, PH_ShaderEffect* effect);

		PH_Material* buildMaterial(const PH_String& materialName, const MaterialData& info);
		PH_Material* getMaterial(const PH_String& materialName);

		void fillPipelineConfigs();
	private:

		struct MaterialInfoHash
		{
			std::size_t operator()(const MaterialData& k) const
			{
				return k.hash();
			}
		};

		PipelineConfigInfo forwardPipelineConfig;
		PipelineConfigInfo shadowPipelineConfig;

		PH_Map<PH_String, EffectTemplate> templateCache;
		PH_Map<PH_String, PH_Material*> materials;
		PH_Map<MaterialData, PH_Material*, MaterialInfoHash> materialCache;
	};
};