#pragma once

#include "Core/PH_CORE.h"
#include "Containers/PH_String.h"
#include "vulkan/vulkan.h"
#include "Memory/PH_Memory.h"

namespace PhosphorEngine {

    // Forward declaractions
    struct PH_ImageTexture;
    class PH_Device;
    class PH_Buffer;

    // Typedefs and enums
    using ph_texture_flags = uint8;

    enum PH_Texture_Types : uint8 
    {
        /** @brief A standard two-dimensional texture. */
        TEXTURE_TYPE_2D,
        /** @brief A 2d array texture. */
        TEXTURE_TYPE_2D_ARRAY,
        /** @brief A cube texture, used for cubemaps. */
        TEXTURE_TYPE_CUBE,
        /** @brief A cube array texture, used for arrays of cubemaps. */
        TEXTURE_TYPE_CUBE_ARRAY,
        TEXTURE_TYPE_COUNT
    };

    // Structs and classes
    struct PH_ImageBuilder
    {
        using ImageHandle = PH_UniqueMemoryHandle<PH_ImageTexture>;
        using Map = std::unordered_map<uint32, ImageHandle>;

        static ImageHandle BuildImage(PH_String TextureName,
            uint32 Width, uint32 Height, uint32 ChannelCount);

        static void CreateImageTexture(PH_ImageTexture& ImageTextureHandle, VkImageType type,
            VkFormat format,
            VkImageTiling tiling,
            VkImageUsageFlags usage,
            VkMemoryPropertyFlags memoryFlags,
            bool shouldCreateView,
            VkImageAspectFlags viewAspectFlags);


        static void CreateImageSampler(PH_ImageTexture& ImageTextureHandle,
            VkFilter Filter,
            VkSamplerMipmapMode MipMap,
            VkSamplerAddressMode addressMode, float minLod = 0.f, float MaxLod = 1.f, VkBorderColor BorderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK, bool compareEnable = false, VkCompareOp CompareOp = VK_COMPARE_OP_ALWAYS);

        static void CreateImageView(PH_ImageTexture& ImageTextureHandle,
            VkFormat format, VkImageAspectFlags aspect_flags);

        static void TransitionLayout(PH_ImageTexture& ImageTextureHandle, VkCommandBuffer& CommandBuffer,
            VkFormat Format, VkImageLayout OldLayout, VkImageLayout NewLayout);

        static void CopyFromBuffer(PH_ImageTexture& ImageTextureHandle,
            PH_Buffer& Buffer, VkCommandBuffer& CommandBuffer);

        static void DestroyImage(PH_ImageTexture& ImageTextureHandle);

        static void DestroyImageSampler(PH_ImageTexture& ImageTextureHandle);

        static void DestroyImageView(PH_ImageTexture& ImageTextureHandle);

    private:
        static PH_INLINE uint32 CurrentIDForNextImage = 0;
    };

	struct PH_ImageTexture
	{
        PH_ImageTexture(PH_String TextureName, uint32 Width, uint32 Height, uint32 ChannelCount) :
            TextureName(TextureName), Width(Width), Height(Height), ChannelCount(ChannelCount) {};

        PH_ImageTexture(PH_ImageTexture&& Other) noexcept :
            TextureName(Other.TextureName), Width(Other.Width), Height(Other.Height), ChannelCount(Other.ChannelCount) {};

        PH_ImageTexture(const PH_ImageTexture&) = delete;
        PH_ImageTexture& operator=(const PH_ImageTexture&) = delete;

		uint32 ID;
        PH_Texture_Types TextureType;

        uint32 Width;
        uint32 Height;
        uint8 ChannelCount;
        uint16 ArrayLayersCount = 0;
        ph_texture_flags Flags;
        
        PH_String TextureName;
        uint8 MipLevels;

		VkImage ImageHandle;
		VkImageView ImageView;
        VkDeviceMemory ImageMemory;
        VkSampler Sampler;
	};

}