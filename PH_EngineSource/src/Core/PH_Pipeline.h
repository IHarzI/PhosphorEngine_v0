#pragma once

#include "PH_CORE.h"
#include "PH_Device.h"

#include "Containers/PH_String.h"
#include "Containers/PH_DynamicArray.h"

namespace PhosphorEngine {

	using namespace PhosphorEngine::Containers;
	struct PH_ShaderEffect;
	struct ShaderModule;

	struct  PipelineConfigInfo {
		PH_DynamicArray<VkVertexInputBindingDescription> bindingDescriptions{};
		PH_DynamicArray<VkVertexInputAttributeDescription> attributeDescriptions{};
		PH_DynamicArray<VkPipelineShaderStageCreateInfo> shaderStages;
		VkPipelineViewportStateCreateInfo viewportInfo;
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
		VkPipelineRasterizationStateCreateInfo rasterizationInfo;
		VkPipelineMultisampleStateCreateInfo multisampleInfo;
		VkPipelineColorBlendAttachmentState colorBlendAttachment;
		VkPipelineColorBlendStateCreateInfo colorBlendInfo;
		VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
		PH_DynamicArray<VkDynamicState> dynamicStateEnables;
		VkPipelineDynamicStateCreateInfo dynamicStateInfo;
		VkPipelineLayout pipelineLayout = {};
		VkRenderPass renderPass = {};
		uint32_t subpass = 0;
	};

	struct ComputePipelineConfigInfo {
		VkPipelineShaderStageCreateInfo shaderStage;
		VkPipelineLayout pipelineLayout;
	};

	struct PH_Pipeline {
	public:
		PH_Pipeline() { Pipeline = VK_NULL_HANDLE; };
		void destroy();

		void bind(VkCommandBuffer commandBuffer, VkPipelineBindPoint BindPoint);
		VkPipeline GetVulkanPipeline() { return Pipeline; };
	private:
		friend class PH_PipelineBuilder;
		VkPipeline Pipeline;
	};

	class PH_PipelinesCache {
	public:
		void cleanup();
		void AddPipeline(VkPipeline Pipeline) { pipelinesCache.push_back(Pipeline); }
	private:
		PH_DynamicArray<VkPipeline> pipelinesCache{};
	};

	class PH_PipelineBuilder {
	public:
		PH_STATIC_LOCAL void defaultPipelineConfigInfo(PipelineConfigInfo& configInfo);
		PH_STATIC_LOCAL void EnablePipelineRenderFeatureSet(PipelineConfigInfo& configInfo);
		PH_STATIC_LOCAL void enableAplhaBlending(PipelineConfigInfo& configInfo);
		PH_STATIC_LOCAL void FillShaders(PipelineConfigInfo& configInfo, PH_ShaderEffect& Shader);
		PH_STATIC_LOCAL PH_Pipeline BuildGraphicsPipeline(PipelineConfigInfo& configInfo, bool ShouldAddToPool);
		PH_STATIC_LOCAL PH_Pipeline BuildComputePipeline(ComputePipelineConfigInfo& configInfo, bool ShouldAddToPool);
	};
	
	class PH_PipelinesLayoutCache {
	public:
		void cleanup();
		void AddPipeline(VkPipelineLayout Pipeline) { pipelinesLayoutCache.push_back(Pipeline); }
	private:
		PH_DynamicArray<VkPipelineLayout> pipelinesLayoutCache{};
	};

	class PH_PipelineLayoutBuilder {
	public:
		PH_STATIC_LOCAL VkPipelineLayoutCreateInfo defaultPipelineLayoutInfo();
		PH_STATIC_LOCAL void BuildPipelineLayout(VkPipelineLayout& PipelineLayout, VkPipelineLayoutCreateInfo LayoutConfig);
	};
}
