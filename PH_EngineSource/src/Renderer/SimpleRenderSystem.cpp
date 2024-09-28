#include "SimpleRenderSystem.h"
#include "Core/PH_Assert.h"
#include "Core/PH_MATH.h"
#include "Containers/PH_DynamicArray.h"
#include "Containers/PH_Map.h"

#include "Core/PH_FrameInfo.h"
#include "Core/PH_GameWorld.h"
#include "Core/PH_REngineContext.h"
#include "Core/PH_Profiler.h"

#include "PH_Shader.h"

namespace PhosphorEngine {

	using namespace Containers;

	SimpleRenderSystem::SimpleRenderSystem(VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout)
	{
		createPipelineLayout(globalSetLayout);
		createPipeline(renderPass);
	}

	SimpleRenderSystem::~SimpleRenderSystem()
	{
		CleanupPipelines();
	}

	void SimpleRenderSystem::renderGameObjects(PH_FrameInfo* frameInfo)
	{
		PH_PROFILE_SCOPE(SimpleRenderSystem_renderGameObjects)

		ph_pipeline.bind(frameInfo->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS);

		vkCmdBindDescriptorSets(frameInfo->commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipelineLayout,
			0,
			1,
			&frameInfo->globalDescriptorSet,
			0,
			nullptr);
		for (auto& rawObjectPtr : frameInfo->GameWorld->GetObjectMap())
		{
			if (rawObjectPtr.second->model == nullptr) continue;
			auto& object = rawObjectPtr.second;

			object->model->bind(frameInfo->commandBuffer);
			object->model->draw(frameInfo->commandBuffer, 0);
		}
	}

	void SimpleRenderSystem::RecreatePipelines(VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout)
	{
		CleanupPipelines();

		createPipelineLayout(globalSetLayout);
		createPipeline(renderPass);
	}

	void SimpleRenderSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout)
	{
		PH_DynamicArray<VkDescriptorSetLayout> desriptorsSetLayouts{ globalSetLayout };

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(desriptorsSetLayouts.size());
		pipelineLayoutInfo.pSetLayouts = desriptorsSetLayouts.data();	
		pipelineLayoutInfo.pushConstantRangeCount = 0;
		pipelineLayoutInfo.pPushConstantRanges = nullptr;
		PH_VULKAN_CHECK_MSG(vkCreatePipelineLayout(PH_REngineContext::GetLogicalDevice(), &pipelineLayoutInfo, PH_REngineContext::GetDevice().GetAllocator(), &pipelineLayout),
			"Failed to create pipeline layout!");
	}

	void SimpleRenderSystem::createPipeline(VkRenderPass renderPass)
	{
		PH_ASSERT_MSG(pipelineLayout != nullptr, "Cannot create pipeline before pipeline layout");

		PipelineConfigInfo PipelineConfig{};
		PH_PipelineBuilder::defaultPipelineConfigInfo(PipelineConfig);
		PH_PipelineBuilder::EnablePipelineRenderFeatureSet(PipelineConfig);
		PipelineConfig.renderPass = renderPass;
		PipelineConfig.pipelineLayout = pipelineLayout;

		ShaderModule vertModule{};
		ShaderModule::loadAndCreateShaderModule(_SRS_vertPath, vertModule);

		VkPipelineShaderStageCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		info.pNext = nullptr;
		info.pName = "main";
		info.module = vertModule.module;
		info.stage = VK_SHADER_STAGE_VERTEX_BIT;

		PipelineConfig.shaderStages.push_back(info);

		ShaderModule fragModule{};
		ShaderModule::loadAndCreateShaderModule(_SRS_fragPath, fragModule);

		info.module = fragModule.module;
		info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		info.pName = "main";
		PipelineConfig.shaderStages.push_back(info);

		ph_pipeline = PH_PipelineBuilder::BuildGraphicsPipeline(PipelineConfig,false);

		ShaderModule::destroyShaderModule(vertModule);
		ShaderModule::destroyShaderModule(fragModule);
	}

	void SimpleRenderSystem::CleanupPipelines()
	{
		vkDestroyPipelineLayout(PH_REngineContext::GetLogicalDevice(), pipelineLayout, PH_REngineContext::GetDevice().GetAllocator());
		ph_pipeline.destroy();
	}

}