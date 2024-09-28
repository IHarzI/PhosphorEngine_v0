#pragma once

#include "Containers/PH_STring.h"
#include "Containers/PH_DynamicArray.h"
#include "Containers/PH_Map.h"
#include "Containers/PH_StaticArray.h"

namespace PhosphorEngine {

	struct AssetFile {
		uint8 type[4];
		int32 version;
		PH_String json;
		PH_DynamicArray<uint8> binaryBlob;
	};

	enum class CompressionMode : uint32 {
		None,
		LZ4
	};

	namespace MaterialAsset 
	{
		bool SaveBinaryFile(const char* path, const AssetFile& file);

		bool LoadBinaryFile(const char* path, AssetFile& outputFile);

		CompressionMode ParseCompression(const char* f);

		enum class TransparencyMode : uint8 {
			Opaque,
			Transparent,
			Masked
		};

		struct MaterialInfo {
			PH_String baseEffect;
			PH_Map<PH_String, PH_String> textures; //name -> path
			PH_Map<PH_String, PH_String> customProperties;
			TransparencyMode transparency;
		};

		MaterialInfo ReadMaterialInfo(AssetFile* file);

		AssetFile PackMaterial(MaterialInfo* info);
	}

	namespace MeshAsset {
		struct Vertex_f32_PNCV {

			float position[3];
			float normal[3];
			float color[3];
			float uv[2];
		};
		struct Vertex_P32N8C8V16 {

			float position[3];
			uint8 normal[3];
			uint8 color[3];
			float uv[2];
		};

		enum class VertexFormat : uint32
		{
			Unknown = 0,
			PNCV_F32, //everything at 32 bits
			P32N8C8V16 //position at 32 bits, normal at 8 bits, color at 8 bits, uvs at 16 bits float
		};

		struct MeshBounds {

			float origin[3];
			float radius;
			float extents[3];
		};

		struct MeshInfo {
			uint64 vertexBuferSize;
			uint64 indexBuferSize;
			MeshBounds bounds;
			VertexFormat vertexFormat;
			uint8 indexSize;
			CompressionMode compressionMode;
			PH_String originalFile;
		};

		MeshInfo ReadMeshInfo(AssetFile* file);

		void UnpackMesh(MeshInfo* info, const char* sourcebuffer, size_t sourceSize, char* vertexBufer, char* indexBuffer);

		AssetFile PackMesh(MeshInfo* info, char* vertexData, char* indexData);

		MeshBounds CalculateBounds(Vertex_f32_PNCV* vertices, size_t count);
	}

	namespace PrefabAsset {
		struct PrefabInfo {
			//points to matrix array in the blob
			PH_Map<uint64, int> node_matrices;
			PH_Map<uint64, PH_String> node_names;

			PH_Map<uint64, uint64> node_parents;

			struct NodeMesh {
				std::string material_path;
				std::string mesh_path;
			};

			PH_Map<uint64, NodeMesh> node_meshes;

			PH_DynamicArray<PH_StaticArray<float, 16>> matrices;
		};


		PrefabInfo ReadPrefabInfo(AssetFile* file);
		AssetFile PackPrefab(const PrefabInfo& info);
	}

	namespace TextureAsset {
		enum class TextureFormat : uint32
		{
			Unknown = 0,
			RGBA8
		};

		struct PageInfo {
			uint32 width;
			uint32 height;
			uint32 compressedSize;
			uint32 originalSize;
		};

		struct TextureInfo {
			uint64 textureSize;
			TextureFormat textureFormat;
			CompressionMode compressionMode;

			PH_String originalFile;
			PH_DynamicArray<PageInfo> pages;
		};

		TextureInfo ReadTextureInfo(AssetFile* file);

		void UnpackTexture(TextureInfo* info, const char* sourcebuffer, size_t sourceSize, char* destination);

		void UnpackTexturePage(TextureInfo* info, int pageIndex, char* sourcebuffer, char* destination);

		AssetFile PackTexture(TextureInfo* info, void* pixelData);

		TextureFormat ParseTextureFormat(const char* format);

	}

	class PH_Asset
	{

	};

};