#include "PH_SceneRenderer.h"

#include "Core/PH_Profiler.h"
#include "Systems/PH_CVarSystem.h"
#include "Core/PH_REngineContext.h"
#include "Core/PH_Buffer.h"
#include "Containers/PH_DynamicArray.h"

#include "Core/PH_Camera.h"

#include <future>

AutoCVar_Int CVAR_FreezeCull("culling.freeze", "Locks culling", 0, CVarFlags::EditCheckbox);

AutoCVar_Int CVAR_Shadowcast("gpu.shadowcast", "Use shadowcasting", 1, CVarFlags::EditCheckbox);

AutoCVar_Float CVAR_ShadowBias("gpu.shadowBias", "Distance cull", 5.25f);
AutoCVar_Float CVAR_SlopeBias("gpu.shadowBiasSlope", "Distance cull", 4.75f);

namespace PhosphorEngine {

	Math::vec4 normalizePlane(Math::vec4 p)
	{
		return p / Math::Length(p.xyz);
	}

	void PH_SceneRenderer::executeComputeCull(PH_RenderScene& RenderScene, VkCommandBuffer cmd, PH_RenderScene::MeshPass& pass, CullParams& params)
	{
		if (CVAR_FreezeCull.Get()) return;

		if (pass.batches.size() == 0) return;
		PH_PROFILE_SCOPE(CullDispatch);

		auto& REngineContext = PH_REngineContext::GetRenderEngineContext();

		VkDescriptorBufferInfo objectBufferInfo = RenderScene.objectDataBuffer.GetReference().descriptorInfo();

		VkDescriptorBufferInfo dynamicInfo = REngineContext.GetCurrentFrameInfo().dynamicData->GetBuffer().descriptorInfo();
		dynamicInfo.range = sizeof(GlobalUbo);

		VkDescriptorBufferInfo instanceInfo = pass.passObjectsBuffer->descriptorInfo();

		VkDescriptorBufferInfo finalInfo = pass.compactedInstanceBuffer->descriptorInfo();

		VkDescriptorBufferInfo indirectInfo = pass.drawIndirectBuffer->descriptorInfo();
		const int32 frameIndex = REngineContext.GetRenderer().getFrameIndex();
		VkDescriptorImageInfo depthPyramid;
		depthPyramid.sampler = REngineContext.GetRenderer().GetSwapchain().GetDepthSampler(frameIndex);
		depthPyramid.imageView = REngineContext.GetRenderer().GetSwapchain().GetDepthView(frameIndex);
		depthPyramid.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		VkDescriptorSet COMPObjectDataSet;
		PH_DescriptorSetLayout::Builder Builder{};
		auto DescriptorSetLayout = Builder.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
			.addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
			.addBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
			.addBinding(3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
			.addBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)
			.addBinding(5, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
			.buildFromCache();
		VkDescriptorSetLayout SetLayout = REngineContext.GetDescriptorLayoutCache().CreateSetLayout(Builder);

		PH_DescriptorWriter DescriptorWriter(DescriptorSetLayout.GetReference());
		DescriptorWriter.writeBuffer(0, &objectBufferInfo)
			.writeBuffer(1, &indirectInfo)
			.writeBuffer(2, &instanceInfo)
			.writeBuffer(3, &finalInfo)
			.writeImage(4, &depthPyramid)
			.writeBuffer(5, &dynamicInfo)
			.build(COMPObjectDataSet);


		Math::mat4x4 projection = params.projmat;
		Math::mat4x4 projectionT = Math::Transpose(projection);

		Math::vec4 frustumX = normalizePlane(projectionT[3] + projectionT[0]); // x + w < 0
		Math::vec4 frustumY = normalizePlane(projectionT[3] + projectionT[1]); // y + w < 0

		DrawCullData cullData = {};
		cullData.P00 = projection[0][0];
		cullData.P11 = projection[1][1];
		cullData.znear = 0.1f;
		cullData.zfar = params.drawDist;
		cullData.frustum[0] = frustumX.x;
		cullData.frustum[1] = frustumX.z;
		cullData.frustum[2] = frustumY.y;
		cullData.frustum[3] = frustumY.z;
		cullData.drawCount = static_cast<uint32_t>(pass.flat_batches.size());
		cullData.cullingEnabled = params.frustrumCull;
		cullData.lodEnabled = false;
		cullData.occlusionEnabled = params.occlusionCull;
		cullData.lodBase = 10.f;
		cullData.lodStep = 1.5f;
		cullData.pyramidWidth = static_cast<float>(1);
		cullData.pyramidHeight = static_cast<float>(1);
		cullData.viewMat = params.viewmat;//get_view_matrix();

		cullData.AABBcheck = params.aabb;
		cullData.aabbmin_x = params.aabbmin.x;
		cullData.aabbmin_y = params.aabbmin.y;
		cullData.aabbmin_z = params.aabbmin.z;

		cullData.aabbmax_x = params.aabbmax.x;
		cullData.aabbmax_y = params.aabbmax.y;
		cullData.aabbmax_z = params.aabbmax.z;

		if (params.drawDist > 10000)
		{
			cullData.distanceCheck = false;
		}
		else
		{
			cullData.distanceCheck = true;
		}

		REngineContext.GetCullPipeline().bind(cmd,VK_PIPELINE_BIND_POINT_COMPUTE);

		vkCmdPushConstants(cmd, REngineContext.GetCullLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(DrawCullData), &cullData);

		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, REngineContext.GetCullLayout(), 0, 1, &COMPObjectDataSet, 0, nullptr);

		vkCmdDispatch(cmd, static_cast<uint32>((pass.flat_batches.size() / 256) + 1), 1, 1);


		//barrier the 2 buffers we just wrote for culling, the indirect draw one, and the instances one, so that they can be read well when rendering the pass
		{
			VkBufferMemoryBarrier barrier = PH_BufferMemoryBuilder::build(pass.compactedInstanceBuffer.GetReference().getBuffer(), 
				PH_REngineContext::GetDevice().GetGraphicsQueueIndex());
			barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;

			VkBufferMemoryBarrier barrier2 = PH_BufferMemoryBuilder::build(pass.drawIndirectBuffer.GetReference().getBuffer(), 
				PH_REngineContext::GetDevice().GetGraphicsQueueIndex());
			barrier2.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
			barrier2.dstAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;

			VkBufferMemoryBarrier barriers[] = { barrier,barrier2 };

			REngineContext.GetPostCullBarriers().push_back(barrier);
			REngineContext.GetPostCullBarriers().push_back(barrier2);

		}
		/*
		if (*PH_CVarSystem::Get()->GetIntCVar("culling.outputIndirectBufferToFile"))
		{
			uint32_t offset = get_current_frame().debugDataOffsets.back();
			VkBufferCopy debugCopy;
			debugCopy.dstOffset = offset;
			debugCopy.size = pass.batches.size() * sizeof(GPUIndirectObject);
			debugCopy.srcOffset = 0;
			vkCmdCopyBuffer(cmd, pass.drawIndirectBuffer._buffer, get_current_frame().debugOutputBuffer._buffer, 1, &debugCopy);
			get_current_frame().debugDataOffsets.push_back(offset + static_cast<uint32_t>(debugCopy.size));
			get_current_frame().debugDataNames.push_back("Cull Indirect Output");
		}
		*/
	}

	void PH_SceneRenderer::readyMeshDraw(PH_RenderScene& RenderScene, VkCommandBuffer cmd)
	{
		PH_PROFILE_SCOPE(SceneRenderer_DrawUpload);

		auto& REngineContext = PH_REngineContext::GetRenderEngineContext();

		if (RenderScene.dirtyObjects.size() > 0)
		{
			PH_PROFILE_SCOPE(SceneRenderer_RefreshObjectBuffer)

				uint64 copySize = RenderScene.renderables.size() * sizeof(RenderObject);
			if (RenderScene.objectDataBuffer.GetReference().getBufferSize() < copySize)
			{
				// Reallocate buffer, if size is less then needed for copy
				RenderScene.objectDataBuffer.Reset(RenderScene.objectDataBuffer.Create(sizeof(RenderObject), RenderScene.renderables.size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
					VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT).RetrieveResourse());
			}

			if (RenderScene.dirtyObjects.size() >= RenderScene.renderables.size() * 0.7f)
			{
				RenderScene.objectDataBuffer.GetReference().writeByStagingBuffer(RenderScene.renderables.data());
			}
			else
			{
				PH_DynamicArray<VkBufferCopy> copies;
				copies.reserve(RenderScene.dirtyObjects.size());

				uint64 buffersize = sizeof(GPUObjectData) * RenderScene.dirtyObjects.size();
				uint64 vec4size = sizeof(Math::vec4);
				uint64 intsize = sizeof(uint32);
				uint64 wordsize = sizeof(GPUObjectData) / sizeof(uint32);
				uint64 uploadSize = RenderScene.dirtyObjects.size() * wordsize * intsize;

				PH_Buffer newBuffer{ sizeof(GPUObjectData),RenderScene.dirtyObjects.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
					VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT };
				PH_Buffer targetBuffer{ wordsize * intsize ,RenderScene.dirtyObjects.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
					VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT };

				targetBuffer.map();
				newBuffer.map();
				uint32* TargetData = (uint32*)targetBuffer.getMappedMemory();
				GPUObjectData* objectSSBO = (GPUObjectData*)newBuffer.getMappedMemory();

				uint32 launchcount = 0;
				{
					PH_PROFILE_SCOPE(WriteDirtyObjects);
					uint32 sidx = 0;
					for (int32 i = 0; i < RenderScene.dirtyObjects.size(); i++)
					{
						RenderScene.writeObject(objectSSBO + i, RenderScene.dirtyObjects[i]);

						uint32 dstOffset = static_cast<uint32>(wordsize * RenderScene.dirtyObjects[i].handle);

						for (int b = 0; b < wordsize; b++)
						{
							uint32_t tidx = dstOffset + b;
							TargetData[sidx] = tidx;
							sidx++;
						}
					}
					launchcount = sidx;
				}
				newBuffer.unmap();
				targetBuffer.unmap();

				VkDescriptorBufferInfo indexData = targetBuffer.descriptorInfo();

				VkDescriptorBufferInfo sourceData = newBuffer.descriptorInfo();

				VkDescriptorBufferInfo targetInfo = RenderScene.objectDataBuffer.GetReference().descriptorInfo();

				VkDescriptorSet COMPObjectDataSet;

				PH_DescriptorSetLayout::Builder Builder{};
				auto DescriptorSetLayout = Builder.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
					.addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
					.addBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
					.addBinding(3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
					.buildFromCache();
				VkDescriptorSetLayout SetLayout = REngineContext.GetDescriptorLayoutCache().CreateSetLayout(Builder);

				PH_DescriptorWriter DescriptorWriter(DescriptorSetLayout.GetReference());
				DescriptorWriter.writeBuffer(0, &indexData)
					.writeBuffer(1, &sourceData)
					.writeBuffer(2, &targetInfo)
					.build(COMPObjectDataSet);

				REngineContext.GetSparseUploadPipeline().bind(cmd, VK_PIPELINE_BIND_POINT_COMPUTE);

				vkCmdPushConstants(cmd, REngineContext.GetSparseUploadLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(uint32), &launchcount);

				vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, REngineContext.GetSparseUploadLayout(), 0, 1, &COMPObjectDataSet, 0, nullptr);

				vkCmdDispatch(cmd, ((launchcount) / 256) + 1, 1, 1);
			}

			VkBufferMemoryBarrier barrier = PH_BufferMemoryBuilder::build(RenderScene.objectDataBuffer.GetReference().getBuffer(), REngineContext.GetDevice().GetGraphicsQueueIndex());
			barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			REngineContext.GetUploadBarriers().push_back(barrier);
			RenderScene.clearDirtyObjects();
		}

		PH_RenderScene::MeshPass* passes[3] = { &RenderScene._forwardPass,&RenderScene._transparentForwardPass,&RenderScene._shadowPass };
		for (uint32 p = 0; p < 3; p++)
		{
			auto& pass = *passes[p];

			//reallocate the gpu side buffers if needed

			if (pass.drawIndirectBuffer.GetReference().getBufferSize() < pass.batches.size() * sizeof(GPUIndirectObject))
			{
				RenderScene.objectDataBuffer.Reset(RenderScene.objectDataBuffer.Create(sizeof(GPUIndirectObject), pass.batches.size(),
					VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT).RetrieveResourse());
			}

			if (pass.compactedInstanceBuffer.GetReference().getBufferSize() < pass.flat_batches.size() * sizeof(uint32))
			{
				pass.compactedInstanceBuffer.Reset(pass.compactedInstanceBuffer.Create(sizeof(uint32), pass.batches.size(),
					VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT).RetrieveResourse());
			}

			if (pass.passObjectsBuffer.GetReference().getBufferSize() < pass.flat_batches.size() * sizeof(GPUInstance))
			{
				pass.passObjectsBuffer.Reset(pass.passObjectsBuffer.Create(sizeof(GPUInstance), pass.flat_batches.size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT).RetrieveResourse());
			}
		}

		PH_DynamicArray<std::future<void>> async_calls;
		async_calls.reserve(9);

		PH_DynamicArray<PH_Buffer*> unmaps;

		for (uint32 p = 0; p < 3; p++)
		{
			PH_RenderScene::MeshPass& pass = *passes[p];
			PH_RenderScene::MeshPass* ppass = passes[p];

			PH_RenderScene* pScene = &RenderScene;
			//if the pass has changed the batches, need to reupload them
			if (pass.needsIndirectRefresh && pass.batches.size() > 0)
			{
				PH_PROFILE_SCOPE(RefreshIndirectBuffer);

				PH_Buffer newBuffer{ sizeof(GPUIndirectObject), pass.batches.size(),
					VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
					VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT };
				newBuffer.map();
				GPUIndirectObject* indirect = (GPUIndirectObject*)newBuffer.getMappedMemory();

				async_calls.push_back(std::async(std::launch::async, [=] {
					pScene->fillIndirectArray(indirect, *ppass);

					}));
				newBuffer.unmap();
				pass.clearIndirectBuffer.Reset(PH_UniqueMemoryHandle<PH_Buffer>::Create( newBuffer ));
				pass.needsIndirectRefresh = false;
			}

			if (pass.needsInstanceRefresh && pass.flat_batches.size() > 0)
			{
				PH_PROFILE_SCOPE(RefreshInstancingBuffer);

				PH_Buffer newBuffer{ sizeof(GPUInstance), pass.flat_batches.size(),
					VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
					VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT };
				newBuffer.map();
				GPUInstance* instanceData = (GPUInstance*)newBuffer.getMappedMemory();

				async_calls.push_back(std::async(std::launch::async, [=] {
					pScene->fillInstancesArray(instanceData, *ppass);

					}));

				REngineContext.GetDevice().copyBuffer(newBuffer.getBuffer(), pass.passObjectsBuffer->getBuffer(), pass.flat_batches.size() * sizeof(GPUInstance));

				VkBufferMemoryBarrier barrier = PH_BufferMemoryBuilder::build(pass.passObjectsBuffer->getBuffer(), REngineContext.GetDevice().GetGraphicsQueueIndex());
				barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

				REngineContext.GetUploadBarriers().push_back(barrier);

				pass.needsInstanceRefresh = false;
			}
		}

		for (auto& s : async_calls)
		{
			s.get();
		}

		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, 
			nullptr, static_cast<uint32>(REngineContext.GetUploadBarriers().size()), REngineContext.GetUploadBarriers().data(), 0, nullptr);//1, &readBarrier);
		REngineContext.GetUploadBarriers().clear();
	}

	void PH_SceneRenderer::drawObjectsForward(VkCommandBuffer cmd, PH_RenderScene::MeshPass& pass)
	{
		PH_PROFILE_SCOPE(DrawObjects);
		auto& REngineCOntext = PH_REngineContext::GetRenderEngineContext();
		//make a model view matrix for rendering the object
		//camera view
		Math::mat4x4 view = REngineCOntext.GetCurrentFrameInfo().camera->getView();
		//camera projection
		Math::mat4x4 projection = REngineCOntext.GetCurrentFrameInfo().camera->getProjectionMatrix();

	}

	void PH_SceneRenderer::executeDrawCommands(VkCommandBuffer cmd, PH_RenderScene::MeshPass& pass, VkDescriptorSet ObjectDataSet, PH_DynamicArray<uint32> dynamic_offsets, VkDescriptorSet GlobalSet)
	{
	}

	void PH_SceneRenderer::drawObjectsShadow(VkCommandBuffer cmd, PH_RenderScene::MeshPass& pass)
	{
	}

	void PH_SceneRenderer::reduceDepth(VkCommandBuffer cmd)
	{
	}
}