#pragma once

#include "Core/PH_CORE.h"
#include "Containers/PH_DynamicArray.h"
#include "Renderer/RendererTypes.h"
#include "Containers/PH_Map.h"
#include "Memory/PH_Memory.h"

namespace PhosphorEngine {
	class PH_GameObject;
	class PH_ShaderPass;
	class PH_Material;
	class PH_Buffer;

	template<typename T>
	struct Handle {
		uint32 handle;
	};

	struct PH_Mesh;
	struct GPUObjectData;

	struct GPUIndirectObject {
		VkDrawIndexedIndirectCommand command;
		uint32 objectID;
		uint32 batchID;
	};

	struct DrawMesh {
		uint32 firstVertex;
		uint32 firstIndex;
		uint32 indexCount;
		uint32 vertexCount;
		bool isMerged;

		PH_Mesh* Mesh;
	};

	struct RenderObject {

		Handle<DrawMesh> meshID;
		Handle<PH_Material> material;

		uint32 updateIndex;
		uint32 customSortKey{ 0 };

		PerPassData<int32> passIndices;

		Math::mat4x4 transformMatrix;

		RenderBounds bounds;
	};

	struct GPUInstance {
		uint32 objectID;
		uint32 batchID;
	};

	class PH_RenderScene
	{
	public:
		struct PassMaterial {
			VkDescriptorSet materialSet;
			PH_ShaderPass* shaderPass;

			bool operator==(const PassMaterial& other) const
			{
				return materialSet == other.materialSet && shaderPass == other.shaderPass;
			}
		};
		struct PassObject {
			PassMaterial material;
			Handle<DrawMesh> meshID;
			Handle<RenderObject> original;
			int32 builtbatch;
			uint32 customKey;
		};
		struct RenderBatch {
			Handle<PassObject> object;
			uint64 sortKey;

			bool operator==(const RenderBatch& other) const
			{
				return object.handle == other.object.handle && sortKey == other.sortKey;
			}
		};
		struct IndirectBatch {
			Handle<DrawMesh> meshID;
			PassMaterial material;
			uint32 first;
			uint32 count;
		};

		struct Multibatch {
			uint32 first;
			uint32 count;
		};
		struct MeshPass {

			PH_DynamicArray<PH_RenderScene::Multibatch> multibatches;

			PH_DynamicArray<PH_RenderScene::IndirectBatch> batches;

			PH_DynamicArray<Handle<RenderObject>> unbatchedObjects;

			PH_DynamicArray<PH_RenderScene::RenderBatch> flat_batches;

			PH_DynamicArray<PassObject> objects;

			PH_DynamicArray<Handle<PassObject>> reusableObjects;

			PH_DynamicArray<Handle<PassObject>> objectsToDelete;


			PH_UniqueMemoryHandle<PH_Buffer> compactedInstanceBuffer;
			PH_UniqueMemoryHandle<PH_Buffer> passObjectsBuffer;

			PH_UniqueMemoryHandle<PH_Buffer> drawIndirectBuffer;
			PH_UniqueMemoryHandle<PH_Buffer> clearIndirectBuffer;

			PassObject* get(Handle<PassObject> handle);

			MeshpassType type;

			bool needsIndirectRefresh = true;
			bool needsInstanceRefresh = true;
		};

		void init();

		Handle<RenderObject> registerObject(PH_GameObject* object);

		void registerObjectBatch(PH_GameObject* first, uint32 count);

		void updateTransform(Handle<RenderObject> objectID, const Math::mat4x4& localToWorld);
		void updateObject(Handle<RenderObject> objectID);

		void fillObjectData(GPUObjectData* data);
		void fillIndirectArray(GPUIndirectObject* data, MeshPass& pass);
		void fillInstancesArray(GPUInstance* data, MeshPass& pass);

		void writeObject(GPUObjectData* target, Handle<RenderObject> objectID);

		void clearDirtyObjects();

		void buildBatches();

		void mergeMeshes();

		void refreshPass(MeshPass* pass);

		void buildIndirectBatches(MeshPass* pass, PH_DynamicArray<IndirectBatch>& outbatches, PH_DynamicArray<PH_RenderScene::RenderBatch>& inobjects);
		RenderObject* getObject(Handle<RenderObject> objectID);
		DrawMesh* getMesh(Handle<DrawMesh> objectID);

		PH_Material* getMaterial(Handle<PH_Material> objectID);
		MeshPass* getMeshPass(MeshpassType name);
		Handle<PH_Material> getMaterialHandle(PH_Material* m);
		Handle<DrawMesh> getMeshHandle(PH_Mesh* m);
	private:
		PH_DynamicArray<RenderObject> renderables;
		PH_DynamicArray<DrawMesh> meshes;
		PH_DynamicArray<PH_Material*> materials;

		PH_DynamicArray<Handle<RenderObject>> dirtyObjects;


		MeshPass _forwardPass;
		MeshPass _transparentForwardPass;
		MeshPass _shadowPass;

		PH_Map<PH_Material*, Handle<PH_Material>> materialConvert;
		PH_Map<PH_Mesh*, Handle<DrawMesh>> meshConvert;


		PH_UniqueMemoryHandle<PH_Buffer> mergedVertexBuffer;
		PH_UniqueMemoryHandle<PH_Buffer> mergedIndexBuffer;

		PH_UniqueMemoryHandle<PH_Buffer> objectDataBuffer;

		friend class PH_SceneRenderer;
	};

	class PH_RenderNode
	{
		void UppdateTransform(const Math::mat4x4 parentMatrix);

		PH_WeakMemoryHandle<PH_RenderNode> Parent;
		PH_DynamicArray<PH_SharedMemoryHandle<PH_RenderNode>> Children;

		Math::mat4x4 worldTransform;
		Math::mat4x4 localTransform;
	};

};