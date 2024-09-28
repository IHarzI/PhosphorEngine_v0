#pragma once

#include "Core/PH_CORE.h"
#include "Core/PH_Pipeline.h"=

#define _PLM_vertPath "shaders/point_light_shader.vert.spv"
#define _PLM_fragPath "shaders/point_light_shader.frag.spv"

namespace PhosphorEngine {
	struct PH_FrameInfo;
	struct GlobalUbo;

	class LightSystem
	{
	public:
		LightSystem(VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
		~LightSystem();

		LightSystem(const LightSystem&) = delete;
		LightSystem& operator=(const LightSystem&) = delete;

		void update(PH_FrameInfo* frameInfo, GlobalUbo* ubo);

		void render(PH_FrameInfo* frameInfo);

		void RecreatePipelines(VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);

	private:
		void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
		void createPipeline(VkRenderPass renderPass);

		void CleanupPipelines();

	private:
		PH_Pipeline ph_pipeline;
		VkPipelineLayout pipelineLayout;
	};

}