#include "PH_RenderScene.h"

#include "Core/PH_Profiler.h"
#include "Core/PH_Mesh.h"
#include "Core/PH_Buffer.h"
#include "Core/PH_REngineContext.h"
#include "Core/PH_GameObject.h"
#include "Renderer/PH_MaterialSystem.h"
#include <future>

namespace PhosphorEngine {

	PH_RenderScene::PassObject* PH_RenderScene::MeshPass::get(Handle<PassObject> handle)
	{
		return &objects[handle.handle];
	}

	Handle<PH_Material> PH_RenderScene::getMaterialHandle(PH_Material* m)
	{
		Handle<PH_Material> handle;
		auto it = materialConvert.find(m);
		if (it == materialConvert.end())
		{
			uint32 index = static_cast<uint32>(materials.size());
			materials.push_back(m);

			handle.handle = index;
			materialConvert[m] = handle;
		}
		else {
			handle = (*it).second;
		}
		return handle;
	}

	Handle<DrawMesh> PH_RenderScene::getMeshHandle(PH_Mesh* m)
	{
		Handle<DrawMesh> handle;
		auto it = meshConvert.find(m);
		if (it == meshConvert.end())
		{
			uint32_t index = static_cast<uint32_t>(meshes.size());

			DrawMesh newMesh;
			newMesh.Mesh = m;
			newMesh.firstIndex = 0;
			newMesh.firstVertex = 0;
			newMesh.vertexCount = static_cast<uint32_t>(m->VertexCount);
			newMesh.indexCount = static_cast<uint32_t>(m->IndexCount);

			meshes.push_back(newMesh);

			handle.handle = index;
			meshConvert[m] = handle;
		}
		else {
			handle = (*it).second;
		}
		return handle;
	}

	void PH_RenderNode::UppdateTransform(const Math::mat4x4 parentMatrix)
	{
	}

	void PH_RenderScene::init()
	{
		_forwardPass.type = MeshpassType::Forward;
		_shadowPass.type = MeshpassType::DirectionalShadow;
		_transparentForwardPass.type = MeshpassType::Transparency;
	}

	Handle<RenderObject> PH_RenderScene::registerObject(PH_GameObject* object)
	{
		RenderObject newObj;
		PH_Mesh* ObjectMesh = object->model.Get();
		if (!ObjectMesh)
		{
			return {};
		}
		newObj.bounds = ObjectMesh->renderBounds;
		newObj.transformMatrix = object->transform.mat4();
		newObj.material = getMaterialHandle(ObjectMesh->Material.Get());
		newObj.meshID = getMeshHandle(object->model.Get());
		newObj.updateIndex = (uint32_t)-1;
		newObj.customSortKey = object->customSortKey;
		newObj.passIndices.clear(-1);
		Handle<RenderObject> handle;
		handle.handle = static_cast<uint32_t>(renderables.size());

		renderables.push_back(newObj);

		if (ObjectMesh->bDrawForwardPass)
		{
			if (ObjectMesh->Material.Get()->original->passShaders[MeshpassType::Transparency])
			{
				_transparentForwardPass.unbatchedObjects.push_back(handle);
			}
			if (ObjectMesh->Material.Get()->original->passShaders[MeshpassType::Forward])
			{
				_forwardPass.unbatchedObjects.push_back(handle);
			}
		}
		if (ObjectMesh->bDrawShadowPass)
		{
			if (ObjectMesh->Material.Get()->original->passShaders[MeshpassType::DirectionalShadow])
			{
				_shadowPass.unbatchedObjects.push_back(handle);
			}
		}

		updateObject(handle);
		return handle;
	}

	void PH_RenderScene::registerObjectBatch(PH_GameObject* first, uint32_t count)
	{
		renderables.reserve(count);

		for (uint32 i = 0; i < count; i++) {
			registerObject(&(first[i]));
		}
	}

	void PH_RenderScene::updateTransform(Handle<RenderObject> objectID, const Math::mat4x4& localToWorld)
	{
		getObject(objectID)->transformMatrix = localToWorld;
		updateObject(objectID);
	}

	void PH_RenderScene::updateObject(Handle<RenderObject> objectID)
	{
		auto& passIndices = getObject(objectID)->passIndices;
		if (passIndices[MeshpassType::Forward] != -1)
		{
			Handle<PassObject> obj;
			obj.handle = passIndices[MeshpassType::Forward];

			_forwardPass.objectsToDelete.push_back(obj);
			_forwardPass.unbatchedObjects.push_back(objectID);

			passIndices[MeshpassType::Forward] = -1;
		}

		if (passIndices[MeshpassType::DirectionalShadow] != -1)
		{
			Handle<PassObject> obj;
			obj.handle = passIndices[MeshpassType::DirectionalShadow];

			_shadowPass.objectsToDelete.push_back(obj);
			_shadowPass.unbatchedObjects.push_back(objectID);

			passIndices[MeshpassType::DirectionalShadow] = -1;
		}

		if (passIndices[MeshpassType::Transparency] != -1)
		{
			Handle<PassObject> obj;
			obj.handle = passIndices[MeshpassType::Transparency];

			_transparentForwardPass.unbatchedObjects.push_back(objectID);
			_transparentForwardPass.objectsToDelete.push_back(obj);

			passIndices[MeshpassType::Transparency] = -1;
		}

		if (getObject(objectID)->updateIndex == (uint32)-1)
		{
			getObject(objectID)->updateIndex = static_cast<uint32>(dirtyObjects.size());

			dirtyObjects.push_back(objectID);
		}
	}

	void PH_RenderScene::fillObjectData(GPUObjectData* data)
	{
		for (int i = 0; i < renderables.size(); i++)
		{
			Handle<RenderObject> h;
			h.handle = i;
			writeObject(data + i, h);
		}
	}

	void PH_RenderScene::fillIndirectArray(GPUIndirectObject* data, MeshPass& pass)
	{
		PH_PROFILE_SCOPE(FillIndirect);

		int32 dataIndex = 0;
		for (int32 i = 0; i < pass.batches.size(); i++) {
			auto batch = pass.batches[i];

			data[dataIndex].command.firstInstance = batch.first;
			data[dataIndex].command.instanceCount = 0;
			data[dataIndex].command.firstIndex = getMesh(batch.meshID)->firstIndex;
			data[dataIndex].command.vertexOffset = getMesh(batch.meshID)->firstVertex;
			data[dataIndex].command.indexCount = getMesh(batch.meshID)->indexCount;
			data[dataIndex].objectID = 0;
			data[dataIndex].batchID = i;

			dataIndex++;
		}
	}

	void PH_RenderScene::fillInstancesArray(GPUInstance* data, MeshPass& pass)
	{
		PH_PROFILE_SCOPE(FillInstances);
		int32 dataIndex = 0;
		for (int32 i = 0; i < pass.batches.size(); i++) {
			auto batch = pass.batches[i];

			for (int32 b = 0; b < batch.count; b++)
			{
				data[dataIndex].objectID = pass.get(pass.flat_batches[b + batch.first].object)->original.handle;
				data[dataIndex].batchID = i;
				dataIndex++;
			}
		}
	}

	void PH_RenderScene::writeObject(GPUObjectData* target, Handle<RenderObject> objectID)
	{
		RenderObject* renderable = getObject(objectID);
		GPUObjectData object;

		object.modelMatrix = renderable->transformMatrix;
		object.origin_rad = Math::vec4(renderable->bounds.origin, renderable->bounds.radius);
		object.extents = Math::vec4(renderable->bounds.extents, renderable->bounds.valid ? 1.f : 0.f);

		ph_copy_memory(&object, target, sizeof(GPUObjectData));
	}

	void PH_RenderScene::clearDirtyObjects()
	{
		for (auto obj : dirtyObjects)
		{
			getObject(obj)->updateIndex = (uint32)-1;
		}
		dirtyObjects.clear();
	}

	void PH_RenderScene::buildBatches()
	{
		auto fwd = std::async(std::launch::async, [&] { refreshPass(&_forwardPass); });
		auto shadow = std::async(std::launch::async, [&] { refreshPass(&_shadowPass); });
		auto transparent = std::async(std::launch::async, [&] { refreshPass(&_transparentForwardPass); });

		transparent.get();
		shadow.get();
		fwd.get();
	}

	void PH_RenderScene::mergeMeshes()
	{
		PH_PROFILE_SCOPE(MeshMerge);

		uint64 total_vertices = 0;
		uint64 total_indices = 0;

		for (auto& m : meshes)
		{
			m.firstIndex = static_cast<uint32>(total_indices);
			m.firstVertex = static_cast<uint32>(total_vertices);

			total_vertices += m.vertexCount;
			total_indices += m.indexCount;

			m.isMerged = true;
		}

		mergedVertexBuffer = mergedVertexBuffer.Create(sizeof(PH_Mesh::_Vertex), total_vertices * sizeof(PH_Mesh::_Vertex), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		mergedIndexBuffer = mergedIndexBuffer.Create(sizeof(uint32), total_indices * sizeof(uint32), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		{
			auto cmd = PH_REngineContext::GetDevice().beginSingleTimeCommands();
			for (auto& m : meshes)
			{
				VkBufferCopy vertexCopy;
				vertexCopy.dstOffset = m.firstVertex * sizeof(PH_Mesh::_Vertex);
				vertexCopy.size = m.vertexCount * sizeof(PH_Mesh::_Vertex);
				vertexCopy.srcOffset = 0;

				vkCmdCopyBuffer(cmd, m.Mesh->VertexBuffer->getBuffer(), mergedVertexBuffer->getBuffer(), 1, &vertexCopy);

				VkBufferCopy indexCopy;
				indexCopy.dstOffset = m.firstIndex * sizeof(uint32);
				indexCopy.size = m.indexCount * sizeof(uint32);
				indexCopy.srcOffset = 0;

				vkCmdCopyBuffer(cmd, m.Mesh->IndexBuffer->getBuffer(), mergedIndexBuffer->getBuffer(), 1, &indexCopy);
			}
			PH_REngineContext::GetDevice().endSingleTimeCommands(cmd);
		};

	}

	void PH_RenderScene::refreshPass(MeshPass* pass)
	{
		pass->needsIndirectRefresh = true;
		pass->needsInstanceRefresh = true;

		PH_DynamicArray<uint32> newObjects{};
		if (pass->objectsToDelete.size() > 0)
		{
			PH_PROFILE_SCOPE(RefreshPass_DeleteObjects);

			//create the render batches so that then we can do the deletion on the flat-array directly

			PH_DynamicArray<PH_RenderScene::RenderBatch> deletionBatches;
			deletionBatches.reserve(pass->batches.size());

			for (auto i : pass->objectsToDelete) {
				pass->reusableObjects.push_back(i);
				PH_RenderScene::RenderBatch newCommand;

				auto obj = pass->objects[i.handle];
				newCommand.object = i;

				uint64 pipelinehash = std::hash<uint64>()(uint64(obj.material.shaderPass->pipeline));
				uint64 sethash = std::hash<uint64>()((uint64)obj.material.materialSet);

				uint32 mathash = static_cast<uint32>(pipelinehash ^ sethash);

				uint32 meshmat = uint64(mathash) ^ uint64(obj.meshID.handle);

				//pack mesh id and material into 64 bits				
				newCommand.sortKey = uint64(meshmat) | (uint64(obj.customKey) << 32);

				pass->objects[i.handle].customKey = 0;
				pass->objects[i.handle].material.shaderPass = nullptr;
				pass->objects[i.handle].meshID.handle = -1;
				pass->objects[i.handle].original.handle = -1;

				deletionBatches.push_back(newCommand);

			}
			pass->objectsToDelete.clear();
			{
				PH_PROFILE_SCOPE(RefreshPass_DeletionSort);
				std::sort(deletionBatches.begin(), deletionBatches.end(), [](const PH_RenderScene::RenderBatch& A, const PH_RenderScene::RenderBatch& B) {
					if (A.sortKey < B.sortKey) { return true; }
					else if (A.sortKey == B.sortKey) { return A.object.handle < B.object.handle; }
					else { return false; }
					});
			}
			{
				PH_PROFILE_SCOPE(RefreshPass_Removal);

				PH_DynamicArray<PH_RenderScene::RenderBatch> newbatches;
				newbatches.reserve(pass->flat_batches.size());

				{
					PH_PROFILE_SCOPE(RefreshPass_SetDifference);
					std::set_difference(pass->flat_batches.begin(), pass->flat_batches.end(), deletionBatches.begin(), deletionBatches.end(), std::back_inserter(newbatches),
						[](const PH_RenderScene::RenderBatch& A, const PH_RenderScene::RenderBatch& B) {
							if (A.sortKey < B.sortKey) { return true; }
							else if (A.sortKey == B.sortKey) { return A.object.handle < B.object.handle; }
							else { return false; }
						});
				}
				pass->flat_batches = std::move(newbatches);
			}
		}
		{
			PH_PROFILE_SCOPE(RefreshPass_FillObjectList);

			newObjects.reserve(pass->unbatchedObjects.size());
			for (auto o : pass->unbatchedObjects)
			{
				PH_RenderScene::PassObject newObject;

				newObject.original = o;
				newObject.meshID = getObject(o)->meshID;

				//pack mesh id and material into 32 bits
				PH_Material* mt = getMaterial(getObject(o)->material);
				newObject.material.materialSet = mt->passSets[pass->type];
				newObject.material.shaderPass = mt->original->passShaders[pass->type].Get();
				newObject.customKey = getObject(o)->customSortKey;

				uint32 handle = -1;

				//reuse handle
				if (pass->reusableObjects.size() > 0)
				{
					handle = pass->reusableObjects.back().handle;
					pass->reusableObjects.pop_back();
					pass->objects[handle] = newObject;
				}
				else
				{
					handle = pass->objects.size();
					pass->objects.push_back(newObject);
				}


				newObjects.push_back(handle);
				getObject(o)->passIndices[pass->type] = static_cast<int32>(handle);
			}

			pass->unbatchedObjects.clear();
		}

		PH_DynamicArray<PH_RenderScene::RenderBatch> newBatches;
		newBatches.reserve(newObjects.size());

		{
			PH_PROFILE_SCOPE(RefreshPass_FillDrawList);

			for (auto i : newObjects) {
				{
					PH_RenderScene::RenderBatch newCommand;

					auto obj = pass->objects[i];
					newCommand.object.handle = i;

					uint64 pipelinehash = std::hash<uint64>()(uint64(obj.material.shaderPass->pipeline));
					uint64 sethash = std::hash<uint64>()((uint64)obj.material.materialSet);

					uint32 mathash = static_cast<uint32>(pipelinehash ^ sethash);

					uint32 meshmat = uint64(mathash) ^ uint64(obj.meshID.handle);

					//pack mesh id and material into 64 bits				
					newCommand.sortKey = uint64(meshmat) | (uint64(obj.customKey) << 32);

					newBatches.push_back(newCommand);
				}
			}
		}

		{
			PH_PROFILE_SCOPE(RefreshPass_DrawSort);
			std::sort(newBatches.begin(), newBatches.end(), [](const PH_RenderScene::RenderBatch& A, const PH_RenderScene::RenderBatch& B) {
				if (A.sortKey < B.sortKey) { return true; }
				else if (A.sortKey == B.sortKey) { return A.object.handle < B.object.handle; }
				else { return false; }
				});
		}
		{
			PH_PROFILE_SCOPE(RefreshPass_DrawMergeBatches);

			//merge the new batches into the main batch array

			if (pass->flat_batches.size() > 0 && newBatches.size() > 0)
			{
				size_t index = pass->flat_batches.size();
				pass->flat_batches.reserve(pass->flat_batches.size() + newBatches.size());

				for (auto b : newBatches)
				{
					pass->flat_batches.push_back(b);
				}

				PH_RenderScene::RenderBatch* begin = pass->flat_batches.data();
				PH_RenderScene::RenderBatch* mid = begin + index;
				PH_RenderScene::RenderBatch* end = begin + pass->flat_batches.size();

				std::inplace_merge(begin, mid, end, [](const PH_RenderScene::RenderBatch& A, const PH_RenderScene::RenderBatch& B) {
					if (A.sortKey < B.sortKey) { return true; }
					else if (A.sortKey == B.sortKey) { return A.object.handle < B.object.handle; }
					else { return false; }
					});
			}
			else if (pass->flat_batches.size() == 0)
			{
				pass->flat_batches = std::move(newBatches);
			}
		}

		{
			PH_PROFILE_SCOPE(RefreshPass_DrawMerge);

			pass->batches.clear();

			buildIndirectBatches(pass, pass->batches, pass->flat_batches);

			//flatten batches into multibatch
			Multibatch newbatch;
			pass->multibatches.clear();

			newbatch.count = 1;
			newbatch.first = 0;

			for (int i = 1; i < pass->batches.size(); i++)
			{
				IndirectBatch* joinbatch = &pass->batches[newbatch.first];
				IndirectBatch* batch = &pass->batches[i];

				bool bCompatibleMesh = getMesh(joinbatch->meshID)->isMerged;

				bool bSameMat = false;

				if (bCompatibleMesh && joinbatch->material.materialSet == batch->material.materialSet &&
					joinbatch->material.shaderPass == batch->material.shaderPass
					)
				{
					bSameMat = true;
				}

				if (!bSameMat || !bCompatibleMesh)
				{
					pass->multibatches.push_back(newbatch);
					newbatch.count = 1;
					newbatch.first = i;
				}
				else {
					newbatch.count++;
				}
			}
			pass->multibatches.push_back(newbatch);
		}
	}

	void PH_RenderScene::buildIndirectBatches(MeshPass* pass, PH_DynamicArray<IndirectBatch>& outbatches, PH_DynamicArray<PH_RenderScene::RenderBatch>& inobjects)
	{
		if (inobjects.size() == 0) return;

		PH_PROFILE_SCOPE(BuildIndirectBatches);

		PH_RenderScene::IndirectBatch newBatch;
		newBatch.first = 0;
		newBatch.count = 0;

		newBatch.material = pass->get(inobjects[0].object)->material;
		newBatch.meshID = pass->get(inobjects[0].object)->meshID;

		outbatches.push_back(newBatch);
		PH_RenderScene::IndirectBatch* back = &pass->batches.back();

		PH_RenderScene::PassMaterial lastMat = pass->get(inobjects[0].object)->material;
		for (int i = 0; i < inobjects.size(); i++) {
			PassObject* obj = pass->get(inobjects[i].object);

			bool bSameMesh = obj->meshID.handle == back->meshID.handle;
			bool bSameMaterial = false;
			if (obj->material == lastMat)
			{
				bSameMaterial = true;
			}

			if (!bSameMaterial || !bSameMesh)
			{
				newBatch.material = obj->material;

				if (newBatch.material == back->material)
				{
					bSameMaterial = true;
				}
			}

			if (bSameMesh && bSameMaterial)
			{
				back->count++;
			}
			else {

				newBatch.first = i;
				newBatch.count = 1;
				newBatch.meshID = obj->meshID;

				outbatches.push_back(newBatch);
				back = &outbatches.back();
			}
		}
	}

	RenderObject* PH_RenderScene::getObject(Handle<RenderObject> objectID)
	{
		return &renderables[objectID.handle];
	}

	DrawMesh* PH_RenderScene::getMesh(Handle<DrawMesh> objectID)
	{
		return &meshes[objectID.handle];
	}

	PH_Material* PH_RenderScene::getMaterial(Handle<PH_Material> objectID)
	{
		return materials[objectID.handle];
	}

	PH_RenderScene::MeshPass* PH_RenderScene::getMeshPass(MeshpassType name)
	{
		switch (name)
		{
		case MeshpassType::Forward:
			return &_forwardPass;
			break;
		case MeshpassType::Transparency:
			return &_transparentForwardPass;
			break;
		case MeshpassType::DirectionalShadow:
			return &_shadowPass;
			break;
		}
		return nullptr;
	}
}