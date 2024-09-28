#include "Core/PH_Pipeline.h"
#include "Core/PH_GameObject.h"
#include "Core/PH_Camera.h"

#define _SRS_vertPath "shaders/simple_shader.vert.spv"
#define _SRS_fragPath "shaders/simple_shader.frag.spv"

namespace PhosphorEngine {
	struct PH_FrameInfo;

	class SimpleRenderSystem
	{
	public:
		SimpleRenderSystem(VkRenderPass renderPass,VkDescriptorSetLayout globalSetLayout);
		~SimpleRenderSystem();

		SimpleRenderSystem(const SimpleRenderSystem&) = delete;
		SimpleRenderSystem& operator=(const SimpleRenderSystem&) = delete;

		void renderGameObjects(PH_FrameInfo* frameInfo);

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