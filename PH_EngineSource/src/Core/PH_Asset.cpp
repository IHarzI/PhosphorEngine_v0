#include "PH_Asset.h"

#include "nlohmann_json/Json.h"

#include <iostream>

namespace PhosphorEngine {

    using namespace PhosphorEngine::MaterialAsset;
    using namespace PhosphorEngine::MeshAsset;
    using namespace PhosphorEngine::PrefabAsset;
    using namespace PhosphorEngine::TextureAsset;

    bool MaterialAsset::SaveBinaryFile(const char* path, const AssetFile& file)
    {
        return false;
    }

    bool MaterialAsset::LoadBinaryFile(const char* path, AssetFile& outputFile)
    {
        return false;
    }

    CompressionMode MaterialAsset::ParseCompression(const char* f)
    {
        return CompressionMode();
    }

    MaterialAsset::MaterialInfo MaterialAsset::ReadMaterialInfo(AssetFile* file)
    {
        return MaterialInfo();
    }

    AssetFile MaterialAsset::PackMaterial(MaterialInfo* info)
    {
        return AssetFile();
    }

    MeshAsset::MeshInfo MeshAsset::ReadMeshInfo(AssetFile* file)
    {
        return MeshInfo();
    }

    void MeshAsset::UnpackMesh(MeshInfo* info, const char* sourcebuffer, size_t sourceSize, char* vertexBufer, char* indexBuffer)
    {
    }

    AssetFile MeshAsset::PackMesh(MeshInfo* info, char* vertexData, char* indexData)
    {
        return AssetFile();
    }

    MeshAsset::MeshBounds MeshAsset::CalculateBounds(Vertex_f32_PNCV* vertices, size_t count)
    {
        return MeshBounds();
    }

    PrefabAsset::PrefabInfo PrefabAsset::ReadPrefabInfo(AssetFile* file)
    {
        return PrefabInfo();
    }

    AssetFile PrefabAsset::PackPrefab(const PrefabInfo& info)
    {
        return AssetFile();
    }

    TextureAsset::TextureInfo TextureAsset::ReadTextureInfo(AssetFile* file)
    {
        TextureInfo info;

        nlohmann::json texture_metadata = nlohmann::json::parse(file->json);

        PH_String formatString = texture_metadata["format"];
        info.textureFormat = ParseTextureFormat(formatString.c_str());

        PH_String compressionString = texture_metadata["compression"];
        info.compressionMode = ParseCompression(compressionString.c_str());

        info.textureSize = texture_metadata["buffer_size"];
        info.originalFile = texture_metadata["original_file"];

        for (auto& [key, value] : texture_metadata["pages"].items())
        {
            PageInfo page;

            page.compressedSize = value["compressed_size"];
            page.originalSize = value["original_size"];
            page.width = value["width"];
            page.height = value["height"];

            info.pages.push_back(page);
        }


        return info;
    }

    void TextureAsset::UnpackTexture(TextureInfo* info, const char* sourcebuffer, size_t sourceSize, char* destination)
    {
    }

    void TextureAsset::UnpackTexturePage(TextureInfo* info, int pageIndex, char* sourcebuffer, char* destination)
    {
    }

    AssetFile TextureAsset::PackTexture(TextureInfo* info, void* pixelData)
    {
        return AssetFile();
    }

    TextureFormat TextureAsset::ParseTextureFormat(const char* format)
    {
        if (strcmp(format, "RGBA8") == 0)
        {
            return TextureFormat::RGBA8;
        }
        else {
            return TextureFormat::Unknown;
        }
    }

};