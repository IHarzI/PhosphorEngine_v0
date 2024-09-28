#include "PH_Mesh.h"
#include "Containers/PH_Map.h"
#include "Utils/PH_Utils.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "TinyObjLoader/tiny_obj_loader.h"

#include "Core/PH_REngineContext.h"
#include "Core/PH_Device.h"
#include "Core/PH_Buffer.h"
#include "Renderer/PH_MaterialSystem.h"
#include <cstring>

#define __DEFAULT_VERTEX_COLOR__ 0.7f,0.5f,0.3f

namespace std {
	template<>
	struct hash<PhosphorEngine::PH_Mesh::_Vertex> {
		size_t operator()(PhosphorEngine::PH_Mesh::_Vertex const& vertex)const {
			size_t seed{ 0 };
			PhosphorEngine::Utils::Hash::hashCombine(seed, vertex.position, vertex.color, vertex.normal, vertex.UV_X, vertex.UV_Y);
			return seed;
		}
	};
}

namespace PhosphorEngine {
	
	PH_Mesh::PH_Mesh(const PH_Mesh::_MeshBuilder& builder)
		: MeshName(builder.MeshName)
	{

		createVertexBuffers(builder.vertices);
		createIndexBuffer(builder.indices);
		
	}

	PH_Mesh::~PH_Mesh() {	}

	PH_UniqueMemoryHandle<PH_Mesh> PH_Mesh::createMeshFromFile(const PH_String& filepath, int EntityID, PH_String MeshName)
	{
		_MeshBuilder builder{};
		builder.loadmesh(filepath,EntityID);
		builder.MeshName = MeshName;

		return PH_UniqueMemoryHandle<PH_Mesh>::Create(builder);
	}

	void PH_Mesh::bind(VkCommandBuffer commandBuffer)
	{
		VkBuffer buffers[] = { VertexBuffer->getBuffer()};
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);

		if (bHasIndexBuffer){
			vkCmdBindIndexBuffer(commandBuffer, IndexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
		}
	}

	void PH_Mesh::draw(VkCommandBuffer commandBuffer, uint32 DrawObjectIndex)
	{
		if (bHasIndexBuffer)
		{
			vkCmdDrawIndexed(commandBuffer, IndexCount,1,0,0,DrawObjectIndex);
		}
		else
		{
			vkCmdDraw(commandBuffer, VertexCount, 1, 0, DrawObjectIndex);
		}
	}
	 
	void PH_Mesh::createVertexBuffers(const PH_DynamicArray<_Vertex>& vertices)
	{
		VertexCount = static_cast<uint32_t>(vertices.size());
		PH_ASSERT_MSG(VertexCount >= 3, "Vertex count must be at least 3!");
		VkDeviceSize bufferSize = sizeof(vertices[0]) * VertexCount;
		uint32_t vertexSize = sizeof(vertices[0]);

		PH_Buffer stagingBuffer{
			vertexSize,
			VertexCount,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		};

		stagingBuffer.map();
		stagingBuffer.writeToBuffer((void*)vertices.data());

		VertexBuffer = PH_UniqueMemoryHandle<PH_Buffer>::Create(
			vertexSize,
			VertexCount,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		PH_REngineContext::GetDevice().copyBuffer(stagingBuffer.getBuffer(), VertexBuffer->getBuffer(), bufferSize);
	}

	void PH_Mesh::createIndexBuffer(const PH_DynamicArray<uint32_t>& indices)
	{
		IndexCount = static_cast<uint32_t>(indices.size());
		bHasIndexBuffer = IndexCount > 0;
		if (!bHasIndexBuffer)
			return;
		VkDeviceSize bufferSize = sizeof(indices[0]) * IndexCount;
		uint32_t indexSize = sizeof(indices[0]);

		PH_Buffer stagingBuffer{
			indexSize,
			IndexCount,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		};

		stagingBuffer.map();
		stagingBuffer.writeToBuffer((void*)indices.data());

		IndexBuffer = PH_UniqueMemoryHandle<PH_Buffer>::Create(
			indexSize,
			IndexCount,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		PH_REngineContext::GetDevice().copyBuffer(stagingBuffer.getBuffer(), IndexBuffer->getBuffer(), bufferSize);
	}

	PH_DynamicArray<VkVertexInputBindingDescription> PH_Mesh::_Vertex::getBindingDescription()
	{
		PH_DynamicArray<VkVertexInputBindingDescription> bindingDescription(1);
		bindingDescription[0].binding = 0;
		bindingDescription[0].stride = sizeof(_Vertex);
		bindingDescription[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bindingDescription;
	}

	PH_DynamicArray<VkVertexInputAttributeDescription> PH_Mesh::_Vertex::getAttributeDescriptions()
	{
		PH_DynamicArray<VkVertexInputAttributeDescription> attributeDescriptions{};
		attributeDescriptions.push_back({ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(_Vertex, position) });
		attributeDescriptions.push_back({ 1, 0, VK_FORMAT_R32_SFLOAT, offsetof(_Vertex, UV_X) });
		attributeDescriptions.push_back({ 2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(_Vertex, color) });
		attributeDescriptions.push_back({ 3, 0, VK_FORMAT_R32_SFLOAT, offsetof(_Vertex, UV_Y) });
	
		attributeDescriptions.push_back({ 4, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(_Vertex, normal) });
		attributeDescriptions.push_back({ 5, 0, VK_FORMAT_R32_SFLOAT, offsetof(_Vertex, TextureIndex) });
		attributeDescriptions.push_back({ 6, 0, VK_FORMAT_R32_SINT, offsetof(_Vertex, EntityID) });

		return attributeDescriptions;
	}

	void PH_Mesh::_MeshBuilder::loadmesh(const PH_String & filepath, int EntityID)
	{
		tinyobj::attrib_t attrib;	
		PH_DynamicArray<tinyobj::shape_t> shapes;
		PH_DynamicArray<tinyobj::material_t> materials;
		PH_String warn, err;
		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.c_str()))
		{
			PH_ERROR("%s %s",warn.c_str(), err.c_str());
		}

		vertices.clear();
		indices.clear();

		PH_Map<_Vertex, uint32_t> uniqueVertices{};

		for (const auto& shape : shapes)
		{
			for (const auto& index : shape.mesh.indices)
			{
				_Vertex vertex{};
				vertex.EntityID = EntityID;
				if (index.vertex_index >= 0)
				{
					vertex.position = { 
						attrib.vertices[3 * index.vertex_index + 0],
						attrib.vertices[3 * index.vertex_index + 1],
						attrib.vertices[3 * index.vertex_index + 2]
					};

					vertex.color = {
						1.f,1.f,1.f
					};
					
					//vertex.color = {
					//	attrib.colors[3*index.vertex_index + 0],
					//	attrib.colors[3 * index.vertex_index + 1],
					//	attrib.colors[3 * index.vertex_index + 2]
					//};
				}

				if (index.normal_index >= 0)
				{
					vertex.normal = { 
						attrib.normals[3 * index.normal_index + 0],
						attrib.normals[3 * index.normal_index + 1],
						attrib.normals[3 * index.normal_index + 2]
					};
				}

				if (index.texcoord_index >= 0)
				{
					if (attrib.texcoords.size() >= 3 * index.texcoord_index + 1)
					{
						vertex.UV_X = attrib.texcoords[2 * index.texcoord_index + 0];
						vertex.UV_Y = 1.0f - attrib.texcoords[2 * index.texcoord_index + 1];
					};
				}
				if (uniqueVertices.count(vertex) == 0)
				{
					uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
					vertices.push_back(vertex);
				}
				indices.push_back(uniqueVertices[vertex]);
			}
		}
	}
}