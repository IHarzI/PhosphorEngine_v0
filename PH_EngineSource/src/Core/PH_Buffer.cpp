#include "PH_Buffer.h"
#include "PH_Logger.h"

#include "Core/PH_REngineContext.h"
#include <cstring>

namespace PhosphorEngine {

    /**
     * Returns the minimum instance size required to be compatible with devices minOffsetAlignment
     *
     * @param instanceSize The size of an instance
     * @param minOffsetAlignment The minimum required alignment, in bytes, for the offset member (eg
     * minUniformBufferOffsetAlignment)
     *
     * @return VkResult of the buffer mapping call
     */
    VkDeviceSize PH_Buffer::getAlignment(VkDeviceSize instanceSize, VkDeviceSize minOffsetAlignment) {
        if (minOffsetAlignment > 0) {
            return (instanceSize + minOffsetAlignment - 1) & ~(minOffsetAlignment - 1);
        }
        return instanceSize;
    }

    PH_Buffer::PH_Buffer(VkDeviceSize instanceSize, 
        uint64 instanceCount, VkBufferUsageFlags usageFlags, 
        VkMemoryPropertyFlags memoryPropertyFlags, bool ShouldBindMemory, VkDeviceSize minOffsetAlignment)
        : 
        instanceSize{ instanceSize },
        instanceCount{ instanceCount },
        usageFlags{ usageFlags },
        memoryPropertyFlags{ memoryPropertyFlags } {
        alignmentSize = getAlignment(instanceSize, minOffsetAlignment);
        bufferSize = alignmentSize * instanceCount;

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = bufferSize;
        bufferInfo.usage = usageFlags;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        PH_VULKAN_CHECK_MSG(vkCreateBuffer(PH_REngineContext::GetLogicalDevice(), &bufferInfo, PH_REngineContext::GetDevice().GetAllocator(), &buffer),
            "Failed to create vertex buffer!");

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(PH_REngineContext::GetLogicalDevice(), buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = PH_REngineContext::GetDevice().findMemoryType(memRequirements.memoryTypeBits, memoryPropertyFlags);

        PH_VULKAN_CHECK_MSG(vkAllocateMemory(PH_REngineContext::GetLogicalDevice(), &allocInfo, PH_REngineContext::GetDevice().GetAllocator(), &memory),
            "Failed to allocate vertex buffer memory!");

        if (ShouldBindMemory)
        {
            BindMemory(0);
        }
    }

    PH_Buffer::~PH_Buffer() {
        unmap();
        if(buffer)
            vkDestroyBuffer(PH_REngineContext::GetLogicalDevice(), buffer, PH_REngineContext::GetDevice().GetAllocator());
        if(memory)
            vkFreeMemory(PH_REngineContext::GetLogicalDevice(), memory, PH_REngineContext::GetDevice().GetAllocator());
    }

    PH_Buffer::PH_Buffer(PH_Buffer& Other)
    {
        mapped = Other.mapped;
        buffer = Other.buffer;
        memory = Other.memory;
        
        Other.mapped = nullptr;
        Other.buffer = VK_NULL_HANDLE;
        Other.memory = VK_NULL_HANDLE;

        bufferSize = Other.bufferSize;
        instanceCount = Other.instanceCount;
        instanceSize = Other.instanceSize;
        alignmentSize = Other.alignmentSize;
        usageFlags = Other.usageFlags;
        memoryPropertyFlags = Other.memoryPropertyFlags;

        Other.bufferSize = 0;
        Other.instanceCount = 0;
        Other.instanceSize = 0;
        Other.alignmentSize = 0;
        Other.usageFlags = 0;
        Other.memoryPropertyFlags = 0;
    }

    PH_Buffer& PH_Buffer::operator=(PH_Buffer& Other)
    {
        mapped = Other.mapped;
        buffer = Other.buffer;
        memory = Other.memory;

        Other.mapped = nullptr;
        Other.buffer = VK_NULL_HANDLE;
        Other.memory = VK_NULL_HANDLE;

        bufferSize = Other.bufferSize;
        instanceCount = Other.instanceCount;
        instanceSize = Other.instanceSize;
        alignmentSize = Other.alignmentSize;
        usageFlags = Other.usageFlags;
        memoryPropertyFlags = Other.memoryPropertyFlags;
        
        Other.bufferSize = 0;
        Other.instanceCount = 0;
        Other.instanceSize = 0;
        Other.alignmentSize = 0;
        Other.usageFlags = 0;
        Other.memoryPropertyFlags = 0;
        return *this;
    }

    /**
     * Map a memory range of this buffer. If successful, mapped points to the specified buffer range.
     *
     * @param size (Optional) Size of the memory range to map. Pass VK_WHOLE_SIZE to map the complete
     * buffer range.
     * @param offset (Optional) Byte offset from beginning
     *
     * @return VkResult of the buffer mapping call
     */
    VkResult PH_Buffer::map(VkDeviceSize size, VkDeviceSize offset) {
        PH_ASSERT_MSG(buffer && memory, "Called map on buffer before create");
        return vkMapMemory(PH_REngineContext::GetLogicalDevice(), memory, offset, size, 0, &mapped);
    }

    /**
     * Unmap a mapped memory range
     *
     * @note Does not return a result as vkUnmapMemory can't fail
     */
    void PH_Buffer::unmap() {
        if (mapped) {
            vkUnmapMemory(PH_REngineContext::GetLogicalDevice(), memory);
            mapped = nullptr;
        }
    }

    void PH_Buffer::BindMemory(uint64 offset)
    {
        PH_VULKAN_CHECK_MSG(vkBindBufferMemory(PH_REngineContext::GetLogicalDevice(), buffer, memory, offset), "Failed to bind Buffer!");
    }

    /**
     * Copies the specified data to the mapped buffer. Default value writes whole buffer range
     *
     * @param data Pointer to the data to copy
     * @param size (Optional) Size of the data to copy. Pass VK_WHOLE_SIZE to flush the complete buffer
     * range.
     * @param offset (Optional) Byte offset from beginning of mapped region
     *
     */
    void PH_Buffer::writeToBuffer(void* data, VkDeviceSize size, VkDeviceSize offset) {
        PH_ASSERT_MSG(mapped, "Cannot copy to unmapped buffer");

        if (size == VK_WHOLE_SIZE) {
            ph_copy_memory(data, mapped, bufferSize);
        }
        else {
            char* memOffset = (char*)mapped;
            memOffset += offset;
            ph_copy_memory(data, memOffset, size);
        }
    }

    void PH_Buffer::writeByStagingBuffer(void* data, VkDeviceSize size, VkDeviceSize srcFffset, VkDeviceSize dstFffset)
    {
        PH_Buffer StagingBuffer{
            instanceSize,
            instanceCount,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT };
        StagingBuffer.map();
        StagingBuffer.writeToBuffer(data, size);
        StagingBuffer.unmap();

        PH_REngineContext::GetDevice().copyBuffer(StagingBuffer.buffer, buffer, size, srcFffset, dstFffset);
    }

    /**
     * Flush a memory range of the buffer to make it visible to the device
     *
     * @note Only required for non-coherent memory
     *
     * @param size (Optional) Size of the memory range to flush. Pass VK_WHOLE_SIZE to flush the
     * complete buffer range.
     * @param offset (Optional) Byte offset from beginning
     *
     * @return VkResult of the flush call
     */
    VkResult PH_Buffer::flush(VkDeviceSize size, VkDeviceSize offset) {
        VkMappedMemoryRange mappedRange = {};
        mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        mappedRange.memory = memory;
        mappedRange.offset = offset;
        mappedRange.size = size;
        return vkFlushMappedMemoryRanges(PH_REngineContext::GetLogicalDevice(), 1, &mappedRange);
    }

    /**
     * Invalidate a memory range of the buffer to make it visible to the host
     *
     * @note Only required for non-coherent memory
     *
     * @param size (Optional) Size of the memory range to invalidate. Pass VK_WHOLE_SIZE to invalidate
     * the complete buffer range.
     * @param offset (Optional) Byte offset from beginning
     *
     * @return VkResult of the invalidate call
     */
    VkResult PH_Buffer::invalidate(VkDeviceSize size, VkDeviceSize offset) {
        VkMappedMemoryRange mappedRange = {};
        mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        mappedRange.memory = memory;
        mappedRange.offset = offset;
        mappedRange.size = size;
        return vkInvalidateMappedMemoryRanges(PH_REngineContext::GetLogicalDevice(), 1, &mappedRange);
    }

    /**
     * Create a buffer info descriptor
     *
     * @param size (Optional) Size of the memory range of the descriptor
     * @param offset (Optional) Byte offset from beginning
     *
     * @return VkDescriptorBufferInfo of specified offset and range
     */
    VkDescriptorBufferInfo PH_Buffer::descriptorInfo(VkDeviceSize size, VkDeviceSize offset) {
        return VkDescriptorBufferInfo{
            buffer,
            offset,
            size,
        };
    }

    /**
     * Copies "instanceSize" bytes of data to the mapped buffer at an offset of index * alignmentSize
     *
     * @param data Pointer to the data to copy
     * @param index Used in offset calculation
     *
     */
    void PH_Buffer::writeToIndex(void* data, int index) {
        writeToBuffer(data, instanceSize, index * alignmentSize);
    }

    /**
     *  Flush the memory range at index * alignmentSize of the buffer to make it visible to the device
     *
     * @param index Used in offset calculation
     *
     */
    VkResult PH_Buffer::flushIndex(int index) { return flush(alignmentSize, index * alignmentSize); }

    /**
     * Create a buffer info descriptor
     *
     * @param index Specifies the region given by index * alignmentSize
     *
     * @return VkDescriptorBufferInfo for instance at index
     */
    VkDescriptorBufferInfo PH_Buffer::descriptorInfoForIndex(int index) {
        return descriptorInfo(alignmentSize, index * alignmentSize);
    }

    /**
     * Invalidate a memory range of the buffer to make it visible to the host
     *
     * @note Only required for non-coherent memory
     *
     * @param index Specifies the region to invalidate: index * alignmentSize
     *
     * @return VkResult of the invalidate call
     */
    VkResult PH_Buffer::invalidateIndex(int index) {
        return invalidate(alignmentSize, index * alignmentSize);
    }

    uint32 PH_PushBuffer::push(void* data, uint64 size)
    {
        uint32 offset = currentOffset;
        Buffer.writeToBuffer(data, size, offset);
        currentOffset += static_cast<uint32>(size);

        uint32 alignmentSize = Buffer.getAlignmentSize();
        uint32 alignedOffset = currentOffset;
        if (alignmentSize > 0)
        {
            alignedOffset = (alignedOffset + alignmentSize - 1) & ~(alignmentSize - 1);
        }
        currentOffset = alignedOffset;
        return offset;
    }

    void PH_PushBuffer::reset()
    {
        currentOffset = 0;
    }

    VkBufferMemoryBarrier PH_BufferMemoryBuilder::build(VkBuffer buffer, uint32 queue)
    {
        VkBufferMemoryBarrier Barrier{};
        Barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        Barrier.pNext = nullptr;
        Barrier.buffer = buffer;
        Barrier.size = VK_WHOLE_SIZE;
        Barrier.srcQueueFamilyIndex = queue;
        Barrier.dstQueueFamilyIndex = queue;
        return Barrier;
    }

}