#include "PH_CORE.h"
#include "PH_Pipeline.h"

#include "Core/PH_REngineContext.h"
#include "Systems/PH_FileSystem.h"
#include "Renderer/PH_Shader.h"
#include "PH_Mesh.h"

namespace PhosphorEngine {

	void PH_Pipeline::destroy()
	{
		if(Pipeline)
			vkDestroyPipeline(PH_REngineContext::GetDevice().GetDevice(), Pipeline, PH_REngineContext::GetDevice().GetAllocator());
		Pipeline = 0;
	}

	void PH_Pipeline::bind(VkCommandBuffer commandBuffer, VkPipelineBindPoint BindPoint)
	{
		vkCmdBindPipeline(commandBuffer, BindPoint, Pipeline);
	}

	void PH_PipelineBuilder::defaultPipelineConfigInfo(PipelineConfigInfo& configInfo)
	{
		configInfo.inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		configInfo.inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		configInfo.inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

		configInfo.viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		configInfo.viewportInfo.viewportCount = 1;
		configInfo.viewportInfo.pViewports = nullptr;
		configInfo.viewportInfo.scissorCount = 1;
		configInfo.viewportInfo.pScissors = nullptr;

		configInfo.rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		configInfo.rasterizationInfo.depthClampEnable = VK_FALSE;
		configInfo.rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
		configInfo.rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
		configInfo.rasterizationInfo.lineWidth = 1.f;
		configInfo.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
		configInfo.rasterizationInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
		configInfo.rasterizationInfo.depthBiasEnable = VK_FALSE;
		configInfo.rasterizationInfo.depthBiasConstantFactor = 0.f;
		configInfo.rasterizationInfo.depthBiasClamp = 0.f;
		configInfo.rasterizationInfo.depthBiasSlopeFactor = 0.f;

		configInfo.multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		configInfo.multisampleInfo.sampleShadingEnable = VK_FALSE;
		configInfo.multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		configInfo.multisampleInfo.minSampleShading = 1.f;
		configInfo.multisampleInfo.pSampleMask = nullptr;
		configInfo.multisampleInfo.alphaToCoverageEnable = VK_FALSE;
		configInfo.multisampleInfo.alphaToOneEnable = VK_FALSE;

		configInfo.colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
			| VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		configInfo.colorBlendAttachment.blendEnable = VK_FALSE;
		configInfo.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		configInfo.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		configInfo.colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		configInfo.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		configInfo.colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		configInfo.colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

		configInfo.colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		configInfo.colorBlendInfo.logicOpEnable = VK_FALSE;
		configInfo.colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;
		configInfo.colorBlendInfo.attachmentCount = 1;
		configInfo.colorBlendInfo.pAttachments = &configInfo.colorBlendAttachment;
		configInfo.colorBlendInfo.blendConstants[0] = 0.f;
		configInfo.colorBlendInfo.blendConstants[1] = 0.f;
		configInfo.colorBlendInfo.blendConstants[2] = 0.f;
		configInfo.colorBlendInfo.blendConstants[3] = 0.f;

		configInfo.depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		configInfo.depthStencilInfo.depthTestEnable = VK_TRUE;
		configInfo.depthStencilInfo.depthWriteEnable = VK_TRUE;
		configInfo.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
		configInfo.depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
		configInfo.depthStencilInfo.minDepthBounds = 0.f;
		configInfo.depthStencilInfo.maxDepthBounds = 1.f;
		configInfo.depthStencilInfo.stencilTestEnable = VK_FALSE;
		configInfo.depthStencilInfo.front = {};
		configInfo.depthStencilInfo.back = { };

		configInfo.dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_DEPTH_BIAS };
		configInfo.dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		configInfo.dynamicStateInfo.pDynamicStates = configInfo.dynamicStateEnables.data();
		configInfo.dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(configInfo.dynamicStateEnables.size());
		configInfo.dynamicStateInfo.flags = 0;

		configInfo.bindingDescriptions = PH_Mesh::_Vertex::getBindingDescription();
		configInfo.attributeDescriptions = PH_Mesh::_Vertex::getAttributeDescriptions();
	}

	PH_Pipeline PH_PipelineBuilder::BuildGraphicsPipeline(PipelineConfigInfo& configInfo, bool ShouldAddToPool)
	{
		auto& bindingDescriptions = configInfo.bindingDescriptions;
		auto& attributeDescriptions = configInfo.attributeDescriptions;

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size());
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
		vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();

		VkGraphicsPipelineCreateInfo pipelineInfo{};

		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = configInfo.shaderStages.size();
		pipelineInfo.pStages = configInfo.shaderStages.data();
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &configInfo.inputAssemblyInfo;
		pipelineInfo.pViewportState = &configInfo.viewportInfo;
		pipelineInfo.pRasterizationState = &configInfo.rasterizationInfo;
		pipelineInfo.pColorBlendState = &configInfo.colorBlendInfo;
		pipelineInfo.pDepthStencilState = &configInfo.depthStencilInfo;
		pipelineInfo.pMultisampleState = &configInfo.multisampleInfo;
		pipelineInfo.pDynamicState = &configInfo.dynamicStateInfo;
		
		pipelineInfo.layout = configInfo.pipelineLayout;
		pipelineInfo.renderPass = configInfo.renderPass;
		pipelineInfo.subpass = configInfo.subpass;

		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
		pipelineInfo.basePipelineIndex = -1;

		PH_Pipeline Pipeline{};

		PH_VULKAN_CHECK_MSG(vkCreateGraphicsPipelines(PH_REngineContext::GetDevice().GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, 
			PH_REngineContext::GetDevice().GetAllocator(), &Pipeline.Pipeline),
			"Failed to create graphics pipeline!");
		if(ShouldAddToPool)
			PH_REngineContext::GetPipelinesCache().AddPipeline(Pipeline.GetVulkanPipeline());
		
		return Pipeline;
	}

	PH_Pipeline PH_PipelineBuilder::BuildComputePipeline(ComputePipelineConfigInfo& configInfo, bool ShouldAddToPool)
	{
		VkComputePipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipelineInfo.pNext = nullptr;

		pipelineInfo.stage = configInfo.shaderStage;
		pipelineInfo.layout = configInfo.pipelineLayout;

		PH_Pipeline newPipeline{};
		PH_VULKAN_CHECK_MSG(vkCreateComputePipelines(
			PH_REngineContext::GetDevice().GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, PH_REngineContext::GetDevice().GetAllocator(), &newPipeline.Pipeline),
			"Failed to build compute pipeline");
		if(ShouldAddToPool)
			PH_REngineContext::GetPipelinesCache().AddPipeline(newPipeline.GetVulkanPipeline());
		return newPipeline;
	}

	void PH_PipelineBuilder::enableAplhaBlending(PipelineConfigInfo& configInfo)
	{
		configInfo.colorBlendAttachment.blendEnable = VK_TRUE;
		configInfo.colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
			| VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		configInfo.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		configInfo.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		configInfo.colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		configInfo.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		configInfo.colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		configInfo.colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	}

	void PH_PipelineBuilder::FillShaders(PipelineConfigInfo& configInfo, PH_ShaderEffect& Shader)
	{
		Shader.FillStages(configInfo.shaderStages);
		configInfo.pipelineLayout = Shader.builtLayout;
	}

	void PH_PipelineBuilder::EnablePipelineRenderFeatureSet(PipelineConfigInfo& configInfo)
	{
		auto RenderFeatures = PH_REngineContext::GetDevice().GetRenderFeaturesConfig();
		if (RenderFeatures.drawWireframe == 1)
		{
			configInfo.rasterizationInfo.polygonMode = VK_POLYGON_MODE_LINE;
		}
	}

	VkPipelineLayoutCreateInfo PH_PipelineLayoutBuilder::defaultPipelineLayoutInfo()
	{
		VkPipelineLayoutCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		info.pNext = nullptr;

		info.flags = 0;
		info.setLayoutCount = 0;
		info.pSetLayouts = nullptr;
		info.pushConstantRangeCount = 0;
		info.pPushConstantRanges = nullptr;
		return info;
	}

	void PH_PipelineLayoutBuilder::BuildPipelineLayout(VkPipelineLayout& PipelineLayout, VkPipelineLayoutCreateInfo LayoutConfig)
	{
		vkCreatePipelineLayout(PH_REngineContext::GetDevice().GetDevice(), &LayoutConfig, PH_REngineContext::GetDevice().GetAllocator(), &PipelineLayout);

		PH_REngineContext::GetPipelinesLayoutCache().AddPipeline(PipelineLayout);
	}

	void PH_PipelinesLayoutCache::cleanup()
	{
		for (auto& Pipeline : pipelinesLayoutCache)
			vkDestroyPipelineLayout(PH_REngineContext::GetDevice().GetDevice(), Pipeline, PH_REngineContext::GetDevice().GetAllocator());

		pipelinesLayoutCache.clear();
	}

	void PH_PipelinesCache::cleanup()
	{
		for (auto& Pipeline : pipelinesCache)
			vkDestroyPipeline(PH_REngineContext::GetDevice().GetDevice(), Pipeline, PH_REngineContext::GetDevice().GetAllocator());

		pipelinesCache.clear();
	}
}