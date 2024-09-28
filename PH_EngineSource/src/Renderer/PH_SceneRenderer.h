#pragma once

#include "PH_RenderScene.h"

namespace PhosphorEngine {

	struct CullParams {
		Math::mat4x4 viewmat;
		Math::mat4x4 projmat;
		bool occlusionCull;
		bool frustrumCull;
		float drawDist;
		bool aabb;
		Math::vec3 aabbmin;
		Math::vec3 aabbmax;
	};

	class PH_SceneRenderer
	{
	public:
		void executeComputeCull(PH_RenderScene& RenderScene, VkCommandBuffer cmd, PH_RenderScene::MeshPass& pass, CullParams& params);
		void readyMeshDraw(PH_RenderScene& RenderScene, VkCommandBuffer cmd);
		void drawObjectsForward(VkCommandBuffer cmd, PH_RenderScene::MeshPass& pass);
		void executeDrawCommands(VkCommandBuffer cmd, PH_RenderScene::MeshPass& pass,
			VkDescriptorSet ObjectDataSet, PH_DynamicArray<uint32> dynamic_offsets, VkDescriptorSet GlobalSet);
		void drawObjectsShadow(VkCommandBuffer cmd, PH_RenderScene::MeshPass& pass);
		void reduceDepth(VkCommandBuffer cmd);
	};

};

