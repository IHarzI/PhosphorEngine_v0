#include "LightSystem.h"
#include "Core/PH_Assert.h"
#include "Containers/PH_Map.h"
#include "Containers/PH_DynamicArray.h"
#include "Core/PH_Camera.h"
#include "Core/PH_GameWorld.h"

#include "Core/PH_FrameInfo.h"
#include "Core/PH_REngineContext.h"
#include "PH_Shader.h"

namespace PhosphorEngine {
	
	using namespace Containers;

	struct PointLightPushConstant
	{
		Math::vec4 position{};
		Math::vec4 color{};
		float radius;
	};

	void LightSystem::update(PH_FrameInfo* frameInfo, GlobalUbo* ubo) {
		auto rotateLight = Math::Rotate(Math::mat4x4(1.f), 
			0.5f * frameInfo->frameTime, { 0.f, -1.f, 0.f });

		uint32 PointLightIndex = 0;
		uint32 DirectionalLightIndex = 0;
		for (auto& kv : frameInfo->GameWorld->GetObjectMap())
		{
			auto& obj = kv.second;
			if (obj->pointLight == nullptr && obj->directionalLightComponent)
				continue;

			if (obj->pointLight)
			{
				if (obj->HasTag("RotatedLightPoint"))
				{
					auto objTranslationRotationComputed = rotateLight * Math::vec4(obj->transform.translation, 1.f);
					obj->transform.translation = Math::vec3(objTranslationRotationComputed.xyz);
				}

				ubo->pointLights[PointLightIndex].position = Math::vec4(obj->transform.translation, 1.0f);

				// if emit light
				if (obj->pointLight->Emit)
				{
					ubo->pointLights[PointLightIndex].color = Math::vec4(obj->color, obj->pointLight->lightIntensity);
				};

				PointLightIndex += 1;
			}
			else if (obj->directionalLightComponent)
			{
				ubo->DirectionalLights[DirectionalLightIndex].position = Math::vec4(obj->transform.translation, 1.f);

				if (obj->directionalLightComponent->Emit)
				{
					auto& UboDirLight = ubo->DirectionalLights[DirectionalLightIndex];
					UboDirLight.color = Math::vec4(obj->color, obj->directionalLightComponent->lightIntensity);
					UboDirLight.direction = obj->directionalLightComponent->lightDirection;
					UboDirLight.shadowExtent = obj->directionalLightComponent->shadowExtent;
					if (DirectionalLightIndex == 0)
					{
						ubo->DirectionalShadowMatrix = UboDirLight.getProjection() * UboDirLight.getView();
					}
				}
			}
		}
		ubo->numPointLight = PointLightIndex;
		ubo->numDirectionalLight = DirectionalLightIndex;
	}

	LightSystem::LightSystem(VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout)
	{
		createPipelineLayout(globalSetLayout);
		createPipeline(renderPass);
	}

	LightSystem::~LightSystem()
	{
		CleanupPipelines();
	}

	void LightSystem::render(PH_FrameInfo* frameInfo)
	{
		PH_Map<float, uint32> sorted;
		for (auto& kv : frameInfo->GameWorld->GetObjectMap())
		{
			auto& obj = kv.second;
			if (obj->pointLight == nullptr) continue;
			
			auto offset = frameInfo->camera->getPosition() - obj->transform.translation;
			float disSquared = offset.DotProductFromSelf();
			sorted[disSquared] = obj->getId();

		}

		ph_pipeline.bind(frameInfo->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS);

		vkCmdBindDescriptorSets(frameInfo->commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipelineLayout,
			0,
			1,
			&frameInfo->globalDescriptorSet,
			0,
			nullptr);

		for (auto id : sorted)
		{
			auto& obj = frameInfo->GameWorld->GetObjectMap().at(id.second);

			// if emit light
			if (obj->pointLight->Emit)
			{
				PointLightPushConstant push{};
				push.position = Math::vec4(obj->transform.translation, 1.0f);
				push.color = Math::vec4(obj->color, obj->pointLight->lightIntensity);
				push.radius = obj->transform.scale.x;

				vkCmdPushConstants(frameInfo->commandBuffer,
					pipelineLayout,
					VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
					0,
					sizeof(PointLightPushConstant),
					&push
				);
				vkCmdDraw(frameInfo->commandBuffer, 6, 1, 0, 0);
			};
		}
	}

	void LightSystem::RecreatePipelines(VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout)
	{
		CleanupPipelines();

		createPipelineLayout(globalSetLayout);
		createPipeline(renderPass);
	}

	void LightSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout)
	{
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(PointLightPushConstant);

		PH_DynamicArray<VkDescriptorSetLayout> desriptorsSetLayouts{ globalSetLayout };

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(desriptorsSetLayouts.size());
		pipelineLayoutInfo.pSetLayouts = desriptorsSetLayouts.data();
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
		PH_VULKAN_CHECK_MSG(vkCreatePipelineLayout(PH_REngineContext::GetLogicalDevice(), &pipelineLayoutInfo, PH_REngineContext::GetDevice().GetAllocator(), &pipelineLayout),
			"Failed to create pipeline layout!");
	}

	void LightSystem::createPipeline(VkRenderPass renderPass)
	{
		PH_ASSERT_MSG(pipelineLayout != nullptr, "Cannot create pipeline before pipeline layout");

		PipelineConfigInfo PipelineConfig{};
		PH_PipelineBuilder::defaultPipelineConfigInfo(PipelineConfig);
		PH_PipelineBuilder::EnablePipelineRenderFeatureSet(PipelineConfig);
		PH_PipelineBuilder::enableAplhaBlending(PipelineConfig);
		PipelineConfig.attributeDescriptions.clear();
		PipelineConfig.bindingDescriptions.clear();
		PipelineConfig.renderPass = renderPass;
		PipelineConfig.pipelineLayout = pipelineLayout;

		ShaderModule vertModule{};
		ShaderModule::loadAndCreateShaderModule(_PLM_vertPath, vertModule);

		VkPipelineShaderStageCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		info.pNext = nullptr;
		info.pName = "main";
		info.module = vertModule.module;
		info.stage = VK_SHADER_STAGE_VERTEX_BIT;

		PipelineConfig.shaderStages.push_back(info);

		ShaderModule fragModule{};
		ShaderModule::loadAndCreateShaderModule(_PLM_fragPath, fragModule);

		info.module = fragModule.module;
		info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		PipelineConfig.shaderStages.push_back(info);

		ph_pipeline = PH_PipelineBuilder::BuildGraphicsPipeline(PipelineConfig,false);


		ShaderModule::destroyShaderModule(vertModule);
		ShaderModule::destroyShaderModule(fragModule);
	}

	void LightSystem::CleanupPipelines()
	{
		vkDestroyPipelineLayout(PH_REngineContext::GetDevice().GetDevice(), pipelineLayout, PH_REngineContext::GetDevice().GetAllocator());
		ph_pipeline.destroy();
	}
}