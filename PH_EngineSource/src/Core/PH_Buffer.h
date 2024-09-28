#pragma once

#include "PH_CORE.h"
#include "PH_Device.h"

namespace PhosphorEngine {

	class PH_Buffer
	{
    public:
        PH_Buffer(
            VkDeviceSize instanceSize,
            uint64 instanceCount,
            VkBufferUsageFlags usageFlags,
            VkMemoryPropertyFlags memoryPropertyFlags,
            bool ShouldBindMemory = true,
            VkDeviceSize minOffsetAlignment = 1);
        ~PH_Buffer();

        PH_Buffer(PH_Buffer& Other);
        PH_Buffer& operator=(PH_Buffer& Other);

        VkResult map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
        void unmap();

        void BindMemory(uint64 offset = 0);
        void writeToBuffer(void* data, VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
        void writeByStagingBuffer(void* data, VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize srcFffset = 0, VkDeviceSize dstFffset = 0);
        VkResult flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
        VkDescriptorBufferInfo descriptorInfo(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
        VkResult invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);

        void writeToIndex(void* data, int index);
        VkResult flushIndex(int index);
        VkDescriptorBufferInfo descriptorInfoForIndex(int index);
        VkResult invalidateIndex(int index);

        VkBuffer getBuffer() const { return buffer; }
        void* getMappedMemory() const { return mapped; }
        uint64 getInstanceCount() const { return instanceCount; }
        VkDeviceSize getInstanceSize() const { return instanceSize; }
        VkDeviceSize getAlignmentSize() const { return alignmentSize; }
        VkBufferUsageFlags getUsageFlags() const { return usageFlags; }
        VkMemoryPropertyFlags getMemoryPropertyFlags() const { return memoryPropertyFlags; }
        VkDeviceSize getBufferSize() const { return bufferSize; }

    private:
        static VkDeviceSize getAlignment(VkDeviceSize instanceSize, VkDeviceSize minOffsetAlignment);

        void* mapped = nullptr;
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;

        VkDeviceSize bufferSize;
        uint64 instanceCount;
        VkDeviceSize instanceSize;
        VkDeviceSize alignmentSize;
        VkBufferUsageFlags usageFlags;
        VkMemoryPropertyFlags memoryPropertyFlags;
    };

    class PH_BufferMemoryBuilder
    {
    public:
       PH_STATIC_LOCAL  VkBufferMemoryBarrier build(VkBuffer buffer, uint32 queue);
    };

    class PH_PushBuffer {
    public:
        PH_PushBuffer(
            VkDeviceSize instanceSize,
            uint32 instanceCount,
            VkBufferUsageFlags usageFlags,
            VkMemoryPropertyFlags memoryPropertyFlags,
            bool ShouldBindMemory = true,
            VkDeviceSize minOffsetAlignment = 1) : Buffer(instanceSize, instanceCount, usageFlags, memoryPropertyFlags, ShouldBindMemory, minOffsetAlignment) {};
        template<typename T>
        uint32 push(T& data);

        uint32 push(void* data, uint64 size);
        PH_Buffer& GetBuffer() { return Buffer; }
        void reset();
    private:
        uint32_t currentOffset;
        PH_Buffer Buffer;
    };

    template<typename T>
    uint32 PH_PushBuffer::push(T& data)
    {
        return push(&data, sizeof(T));
    }

}