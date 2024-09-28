#include "Core/PH_Descriptors.h"

#include "Core/PH_REngineContext.h"
#include <algorithm>

namespace PhosphorEngine {

    // *************** Descriptor Set Layout Builder *********************

    PH_DescriptorSetLayout::Builder& PH_DescriptorSetLayout::Builder::addBinding(
        uint32_t binding,
        VkDescriptorType descriptorType,
        VkShaderStageFlags stageFlags,
        uint32_t count) {

        if (bindings.size() <= binding)
        {
            bindings.resize(binding+1);
        }

        VkDescriptorSetLayoutBinding layoutBinding{};
        layoutBinding.binding = binding;
        layoutBinding.descriptorType = descriptorType;
        layoutBinding.descriptorCount = count;
        layoutBinding.stageFlags = stageFlags;
        bindings[binding] = layoutBinding;
        return *this;
    }

    PH_UniqueMemoryHandle<PH_DescriptorSetLayout> PH_DescriptorSetLayout::Builder::build() const {
        return PH_UniqueMemoryHandle<PH_DescriptorSetLayout>::Create( bindings, DescriptorSetNumber);
    }

    PH_UniqueMemoryHandle<PH_DescriptorSetLayout> PH_DescriptorSetLayout::Builder::buildFromCache() const
    {
        VkDescriptorSetLayout VulkanSetLayout = PH_REngineContext::GetDescriptorLayoutCache().CreateSetLayout(*this);
        return PH_UniqueMemoryHandle<PH_DescriptorSetLayout>::Create(VulkanSetLayout,bindings, DescriptorSetNumber);
    }

    void PH_DescriptorSetLayout::Builder::buildRawVulkanLayout(VkDescriptorSetLayout& DescriptorSetLayout) const
    {
        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo = GetDescriptorRawVulkanInfo();

        PH_VULKAN_CHECK_MSG(vkCreateDescriptorSetLayout(
            PH_REngineContext::GetLogicalDevice(),
            &descriptorSetLayoutInfo,
            PH_REngineContext::GetDevice().GetAllocator(),
            &DescriptorSetLayout), "Failed to create descriptor set layout!");
    }

    DescriptorLayoutInfo PH_DescriptorSetLayout::Builder::GetDescriptorLayoutInfo() const
    {
        return DescriptorLayoutInfo{ bindings };
    }

    VkDescriptorSetLayoutCreateInfo PH_DescriptorSetLayout::Builder::GetDescriptorRawVulkanInfo() const
    {
        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
        descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorSetLayoutInfo.bindingCount = static_cast<uint32>(bindings.size());
        descriptorSetLayoutInfo.pBindings = bindings.data();
        return descriptorSetLayoutInfo;
    }

    // *************** Descriptor Set Layout *********************

    PH_DescriptorSetLayout::PH_DescriptorSetLayout(
        PH_DynamicArray<VkDescriptorSetLayoutBinding> bindings, uint32 setNumber)
        : bindings{ bindings }, setNumber(setNumber) {
        PH_DynamicArray<VkDescriptorSetLayoutBinding> setLayoutBindings{bindings};
        destroyOnDestruct = 1;
        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
        descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorSetLayoutInfo.bindingCount = static_cast<uint32>(setLayoutBindings.size());
        descriptorSetLayoutInfo.pBindings = setLayoutBindings.data();

        PH_VULKAN_CHECK_MSG(vkCreateDescriptorSetLayout(
            PH_REngineContext::GetLogicalDevice(),
            &descriptorSetLayoutInfo,
            PH_REngineContext::GetDevice().GetAllocator(),
            &descriptorSetLayout), "Failed to create descriptor set layout!");
    }

    PH_DescriptorSetLayout::~PH_DescriptorSetLayout() {
        vkDestroyDescriptorSetLayout(PH_REngineContext::GetLogicalDevice(), descriptorSetLayout, nullptr);
    }

    // *************** Descriptor Pool Builder *********************

    PH_DescriptorPoolAllocator::Builder& PH_DescriptorPoolAllocator::Builder::addPoolSize(
        VkDescriptorType descriptorType, uint32 count) {
        poolSizes.push_back({ descriptorType, count });
        return *this;
    }

    PH_DescriptorPoolAllocator::Builder& PH_DescriptorPoolAllocator::Builder::setPoolFlags(
        VkDescriptorPoolCreateFlags flags) {
        poolFlags = flags;
        return *this;
    }
    PH_DescriptorPoolAllocator::Builder& PH_DescriptorPoolAllocator::Builder::setMaxSets(uint32 count) {
        maxSets = count;
        return *this;
    }

    PH_UniqueMemoryHandle<PH_DescriptorPoolAllocator> PH_DescriptorPoolAllocator::Builder::build() const {
        return PH_UniqueMemoryHandle<PH_DescriptorPoolAllocator>::Create(maxSets, poolFlags, poolSizes);
    }

    // Util function to create descriptor pool
    VkDescriptorPool createPool(PH_DescriptorPoolAllocator& PoolAllocator)
    {
        VkDescriptorPoolCreateInfo descriptorPoolInfo{};
        descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(PoolAllocator.GetPoolSizes().size());
        descriptorPoolInfo.pPoolSizes = PoolAllocator.GetPoolSizes().data();
        descriptorPoolInfo.maxSets = PoolAllocator.GetMaxSets();
        descriptorPoolInfo.flags = PoolAllocator.GetPoolFlags();

        VkDescriptorPool descriptorPool;
        PH_VULKAN_CHECK_MSG(vkCreateDescriptorPool(PH_REngineContext::GetLogicalDevice(), &descriptorPoolInfo, PH_REngineContext::GetDevice().GetAllocator(), &descriptorPool),
            "Failed to create descriptor pool!");
        return descriptorPool;
    }

    // *************** Descriptor Pool *********************

    PH_DescriptorPoolAllocator::PH_DescriptorPoolAllocator(
        uint32 maxSets, VkDescriptorPoolCreateFlags flags, PH_DynamicArray<VkDescriptorPoolSize> poolSizes)
        : maxSets(maxSets), flags(flags), poolSizes(poolSizes)
    {}

    PH_DescriptorPoolAllocator::~PH_DescriptorPoolAllocator() {
        cleanup();
    }

    bool PH_DescriptorPoolAllocator::allocateDescriptor(
        const VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet& descriptorSet) {

        if (currentPool == VK_NULL_HANDLE)
        {
            currentPool = getNextFreePool();
            usedPools.push_back(currentPool);
        }

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = currentPool;
        allocInfo.pSetLayouts = &descriptorSetLayout;
        allocInfo.descriptorSetCount = 1;

        // Might want to create a "DescriptorPoolManager" class that handles this case, and builds
        // a new pool whenever an old pool fills up. But this is beyond our current scope
        VkResult AllocationResult = vkAllocateDescriptorSets(PH_REngineContext::GetLogicalDevice(), &allocInfo, &descriptorSet);
        bool ShouldReallocate = false;

        switch (AllocationResult)
        {
        case VK_SUCCESS:
            return true;
            break;
        case VK_ERROR_FRAGMENTED_POOL:
        case VK_ERROR_OUT_OF_POOL_MEMORY:
            ShouldReallocate = true;
            break;
        default:
            return false;
        }

        if (ShouldReallocate)
        {
            currentPool = getNextFreePool();
            usedPools.push_back(currentPool);
            AllocationResult = vkAllocateDescriptorSets(PH_REngineContext::GetLogicalDevice(), &allocInfo, &descriptorSet);
            
            PH_VULKAN_CHECK_MSG(AllocationResult, "Allocation failed on descriptor after reallocation! Fragmentation/OUT of memory?");

            if(AllocationResult == VK_SUCCESS)
                return true;
        }

        return false;
    }

    void PH_DescriptorPoolAllocator::resetPools() {
        for (auto pool : usedPools)
        {
            vkResetDescriptorPool(PH_REngineContext::GetLogicalDevice(), pool, 0);
        }
        
        freePools = usedPools;
        usedPools.clear();
        currentPool = VK_NULL_HANDLE;
    }

    void PH_DescriptorPoolAllocator::cleanup()
    {
        for (auto p : freePools)
        {
            vkDestroyDescriptorPool(PH_REngineContext::GetDevice().GetDevice(), p, nullptr);
        }
        for (auto p : usedPools)
        {
            vkDestroyDescriptorPool(PH_REngineContext::GetDevice().GetDevice(), p, nullptr);
        }
        freePools.clear();
        usedPools.clear();
        currentPool = VK_NULL_HANDLE;
    }

    // *************** Descriptor Writer *********************

    PH_DescriptorWriter::PH_DescriptorWriter(PH_DescriptorSetLayout& setLayout)
        : setLayout{ setLayout } {}

    PH_DescriptorWriter& PH_DescriptorWriter::writeBuffer(
        uint32_t binding, VkDescriptorBufferInfo* bufferInfo) {

        auto& bindingDescription = setLayout.bindings[binding];

        PH_ASSERT_MSG(
            bindingDescription.descriptorCount == 1,
            "Binding single descriptor info, but binding expects multiple");

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.descriptorCount = 1;
        write.descriptorType = bindingDescription.descriptorType;
        write.pBufferInfo = bufferInfo;
        write.dstBinding = binding;

        writes.push_back(write);
        return *this;
    }

    PH_DescriptorWriter& PH_DescriptorWriter::writeImage(
        uint32_t binding, VkDescriptorImageInfo* imageInfo) {
        auto& bindingDescription = setLayout.bindings[binding];

        PH_ASSERT_MSG(
            bindingDescription.descriptorCount == 1,
            "Binding single descriptor info, but binding expects multiple");

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.descriptorCount = 1;
        write.descriptorType = bindingDescription.descriptorType;
        write.dstBinding = binding;
        write.pImageInfo = imageInfo;

        writes.push_back(write);
        return *this;
    }

    bool PH_DescriptorWriter::build(VkDescriptorSet& set) {

        bool success = PH_REngineContext::GetDescriptorPoolAllocator().allocateDescriptor(setLayout.getDescriptorSetLayout(), set);
        if (!success) {
            return false;
        }
        overwrite(set);
        return true;
    }

    void PH_DescriptorWriter::overwrite(VkDescriptorSet& set) {
        for (auto& write : writes) {
            write.dstSet = set;
        }
        vkUpdateDescriptorSets(PH_REngineContext::GetDevice().GetDevice(), writes.size(), writes.data(), 0, nullptr);
    }

    void PH_DescriptorLayoutCache::cleanup()
    {
        for (auto pair : layoutCache)
        {
            vkDestroyDescriptorSetLayout(PH_REngineContext::GetDevice().GetDevice(), pair.second, PH_REngineContext::GetDevice().GetAllocator());
        }
    }

    bool DescriptorLayoutInfo::operator==(const DescriptorLayoutInfo& other) const
    {
        if (other.bindings.size() != bindings.size())
        {
            return false;
        }
        else {
            //compare each of the bindings is the same. Bindings are sorted so they will match
            for (int i = 0; i < bindings.size(); i++) {
                if (other.bindings[i].binding != bindings[i].binding)
                {
                    return false;
                }
                if (other.bindings[i].descriptorType != bindings[i].descriptorType)
                {
                    return false;
                }
                if (other.bindings[i].descriptorCount != bindings[i].descriptorCount)
                {
                    return false;
                }
                if (other.bindings[i].stageFlags != bindings[i].stageFlags)
                {
                    return false;
                }
            }
            return true;
        }
    }

    size_t DescriptorLayoutInfo::hash() const
    {
        using std::hash;

        uint64 result = hash<uint64>()(bindings.size());

        for (const VkDescriptorSetLayoutBinding& b : bindings)
        {
            //pack the binding data into a single int64. Not fully correct but its ok
            uint64 binding_hash = b.binding | b.descriptorType << 8 | b.descriptorCount << 16 | b.stageFlags << 24;

            //shuffle the packed binding data and xor it with the main hash
            result ^= hash<uint64>()(binding_hash);
        }

        return result;
    }

    VkDescriptorSetLayout PH_DescriptorLayoutCache::CreateSetLayout(const PH_DescriptorSetLayout::Builder& SetBuilder)
    {
        auto it = layoutCache.find(SetBuilder.GetDescriptorLayoutInfo());
        if (it != layoutCache.end())
        {
            return (*it).second;
        }
        else {
            VkDescriptorSetLayout layout;
            SetBuilder.buildRawVulkanLayout(layout);

            layoutCache[SetBuilder.GetDescriptorLayoutInfo()] = layout;
            return layout;
        }
    }

    VkDescriptorPool PH_DescriptorPoolAllocator::getNextFreePool()
    {
        if (freePools.size() > 0)
        {
            VkDescriptorPool pool = freePools.back();
            freePools.pop_back();
            return pool;
        }
        else {
            return createPool( *this);
        }
    }

}