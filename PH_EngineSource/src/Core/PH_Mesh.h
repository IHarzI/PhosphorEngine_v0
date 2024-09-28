#pragma once

#include "PH_CORE.h"
#include "PH_MATH.h"
#include "Memory/PH_Memory.h"
#include "Containers/PH_DynamicArray.h"
#include "Containers/PH_String.h"
#include "Renderer/RendererTypes.h"
#include "Utils/PH_Utils.h"

namespace PhosphorEngine {

	using namespace PhosphorEngine::Containers;
	class PH_Material;
	class PH_Buffer;

	class PH_Mesh
	{
	public:

		struct _Vertex
		{
			Math::vec3 position{};
			float UV_X;
			Math::vec3 color{};
			float UV_Y;
			Math::vec3 normal{};
			float TextureIndex;
			int EntityID;

			static PH_DynamicArray<VkVertexInputBindingDescription> getBindingDescription();
			static PH_DynamicArray<VkVertexInputAttributeDescription> getAttributeDescriptions();

			bool operator==(const _Vertex& other) const {
				return position == other.position && 
					color == other.color && 
					normal == other.normal && 
					UV_X == other.UV_X && UV_Y == other.UV_Y;
			}

		};

		struct _MeshBuilder {
			PH_DynamicArray<_Vertex> vertices{};
			PH_DynamicArray<uint32> indices;

			PH_String MeshName{ "Mesh" };

			void loadmesh(const PH_String& filepath, int EntityID);
		};

		PH_Mesh(const PH_Mesh::_MeshBuilder& builder);
		~PH_Mesh();

		PH_Mesh(const PH_Mesh&) = delete;
		PH_Mesh& operator=(const PH_Mesh&) = delete;

		static PH_UniqueMemoryHandle<PH_Mesh> createMeshFromFile(const PH_String& filepath, int EntityID, PH_String MeshName = {"Mesh"});

		void bind(VkCommandBuffer commandBuffer);
		void draw(VkCommandBuffer commandBuffer, uint32 DrawObjectIndex);

		PH_String GetMeshName() const { return MeshName; }

		PH_UniqueMemoryHandle<PH_Buffer> VertexBuffer;
		uint32_t VertexCount;

		PH_UniqueMemoryHandle<PH_Buffer> IndexBuffer;

		PH_SharedMemoryHandle<PH_Material> Material;

		RenderBounds renderBounds;
		uint32_t IndexCount;
		bool bHasIndexBuffer = false;
		PH_String MeshName{};

		uint32_t bDrawForwardPass : 1;
		uint32_t bDrawShadowPass : 1;
	private:
		void createVertexBuffers(const PH_DynamicArray<_Vertex>& vertices);
		void createIndexBuffer(const PH_DynamicArray<uint32_t>& indices);

	};

}
