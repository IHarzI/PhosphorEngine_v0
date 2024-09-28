#include "PH_Memory.h"
#include "Core/PH_Assert.h"
#include "Core/PH_Statistics.h"
#include "Containers/PH_String.h"
#include "PH_DynamicAllocator.h"
#include "PH_MemoryStatistic.h"

namespace PhosphorEngine {
    namespace Memory
    {
        PH_DynamicAllocator*& STATIC_PH_DYNAMIC_ALLOCATOR = detail::INTERNAL_PH_DynamicAllocator_Handle::PH_DynamicAllocator_Stat_Inst;
        // Static's inits
        PH_GlobalMemoryStats* PH_GlobalMemoryStats::stat_inst = nullptr;
        PH_SimpleAllocator::AllocationsMap PH_SimpleAllocator::AllocatedMemoryBlocks{ };
        PH_MemorySystem* PH_MemorySystem::stat_inst = nullptr;

        PH_PlatformAllocator::AllocationsVector PH_PlatformAllocator::AllocatedMemoryBlocks{};

        uint64 PH_BaseAllocator::_occupied_size{};
        uint64 PH_BaseAllocator::_size_of_alloc_per_tag[ph_mem_tag::PH_MEM_TAG_MAX]{};
        uint8** PH_BaseAllocator::_allocated_memory;
        uint64 PH_BaseAllocator::_size_of_alloc_mem{};

        // Functions definitions

        void PH_BaseAllocator::BaseAllocate(uint64 size) {
            PH_ASSERT_MSG(!_allocated_memory, "Double allocation of base memory in Allocator!")
                if (_allocated_memory)
                {
                    return;
                };

            _allocated_memory = (uint8**)PH_SimpleAllocator::Allocate(size + PH_METADATA_SIZE,PH_MEM_TAG_ALLOCATOR);
            PH_ASSERT(_allocated_memory);
            _size_of_alloc_mem = size + PH_METADATA_SIZE;
            ph_zero_memory((void*)_allocated_memory, _size_of_alloc_mem);
            void* header = GetFirstBlock(); // alloc_memory address + metadata size
            detail::InitHeader(header, PH_MEM_TAG_UNKNOWN, _size_of_alloc_mem - PH_METADATA_SIZE, true);
            PH_GlobalMemoryStats::RegisterAllocation(size, PH_MEM_TAG_ALLOCATOR);
            return;
        };

        void* PH_BaseAllocator::Allocate(uint64 size, ph_mem_tag tag)
        {
            // Calculate real size with METADATA for possible allocated block
            uint64 realSize = size + PH_METADATA_SIZE;

            // Final result address(null if no space for allocation)
            void* free_address = nullptr;

            // Check if size is not bigger than max allocatin size(including metadata for next free block) or less than 0
            if (size >= PH_MAX_ALLOCATION_SIZE - PH_METADATA_SIZE * 2 || size <= 0)
            {
                PH_ERROR("Allocating memory with incorrect size: %ld bytes!", size);
                return nullptr;
            }

            // Check if real size + next(possible) block header is less than size of free memory in allocator
            if (realSize < _size_of_alloc_mem - _occupied_size)
            {
                uint8* current_pos = (uint8*)GetFirstBlock();
                while (!free_address)
                {
                    uint64 blockSize = detail::getSizeOfAllocatedBlock(current_pos);
                    ph_mem_tag Tag = detail::getTagFromAllocatedBlock(current_pos);
                    if (detail::isFree(current_pos))
                    {
                        uint64 blockSize = detail::getSizeOfAllocatedBlock(current_pos);
                        if (blockSize >= realSize)
                        {
                            // init this block header
                            detail::InitHeader(current_pos, tag, realSize, false);

                            // init next block header
                            detail::InitHeader(current_pos + realSize, PH_MEM_TAG_UNKNOWN, blockSize - realSize, true);

                            free_address = current_pos;
                            break;
                        }
                        else
                        {
                            PH_ASSERT("Fragmentation happen here: %p", current_pos);
                        }
                    }

                    // Check if it's last block (out of free space)
                    if ((current_pos + detail::getSizeOfAllocatedBlock(current_pos)) >= ((uint8*)_allocated_memory + _size_of_alloc_mem)- PH_METADATA_SIZE )
                        return nullptr;

                    current_pos = (current_pos + detail::getSizeOfAllocatedBlock(current_pos));
                }
            }
            else
            {
                return nullptr;
            }

            _occupied_size += realSize;
            _size_of_alloc_per_tag[tag] += realSize;

            PH_GlobalMemoryStats::RegisterAllocation(size, tag);

            return free_address;
        }

        bool PH_BaseAllocator::Deallocate(void* block)
        {
            bool result = false;
            if (!block)
                return result;

            PH_ASSERT_MSG((block > _allocated_memory && block < (_allocated_memory + _size_of_alloc_mem)),
                "Trying to free memory that is not allocated by this allocator!");

            if (block > _allocated_memory && block < (_allocated_memory + _size_of_alloc_mem))
            {
                size_t size = detail::getSizeOfAllocatedBlock(block); // DON'T forget about METADATA size of next block is INCLUDED
                bool occupied = !detail::isFree(block);
                ph_mem_tag tag = detail::getTagFromAllocatedBlock(block);

                PH_ASSERT(occupied);

                if (occupied)
                {
                    result = true;
                    _occupied_size -= size;
                    _size_of_alloc_per_tag[tag] -= size;

                    void* nextBlockPosition = (uint8*)block + size;
                    
                    // Set block size as this block+ next block sizes(if all conditions are satisfied)
                    // check if size of this(free) block and size of next free block(if it's) is in valid range
                    if ((uint8*)nextBlockPosition <= ((uint8*)_allocated_memory + _size_of_alloc_mem))
                    {
                        // if next block is free
                        if (detail::isFree(nextBlockPosition))
                        {
                            // set this free block size as size of this block and size of the next block(which is also free)
                            detail::setAllocatedBlockSize(block,
                                // this block size
                                detail::getSizeOfAllocatedBlock(block) +
                                // next block size
                                detail::getSizeOfAllocatedBlock(nextBlockPosition));
                        }
                    }

                    // nullify this memory block
                    ph_zero_memory(block, detail::getSizeOfAllocatedBlock(block));
                    detail::setFreeFlag(block, true);
                    detail::setTag(block, PH_MEM_TAG_UNKNOWN);

                    PH_GlobalMemoryStats::RegisterDeallocation(size, tag);
                }
            }
            return result;
        }

        ph_mem_tag PH_BaseAllocator::GetTag(void* address)
        {
            if (address > _allocated_memory && address < (_allocated_memory + _size_of_alloc_mem))
                return detail::getTagFromAllocatedBlock(address);

            return ph_mem_tag::PH_MEM_TAG_UNKNOWN;
        }

        void PH_BaseAllocator::ClearMemory()
        {
            if (!(_allocated_memory && _size_of_alloc_mem > 0))
            {
                PH_ASSERT("Double call to clear memory for Base Allocator");
                return;
            }


            PH_GlobalMemoryStats::RegisterDeallocation(_size_of_alloc_mem, PH_MEM_TAG_ALLOCATOR);

            PH_SimpleAllocator::Deallocate((void*)_allocated_memory);
            _allocated_memory = nullptr; 
            _size_of_alloc_mem = 0; 
            _occupied_size = 0;
            ph_set_memory(&_size_of_alloc_per_tag, 0, sizeof(_size_of_alloc_per_tag));
        }

        PH_BaseAllocator::PH_BaseAllocator()
        {
        };

        PH_BaseAllocator::~PH_BaseAllocator()
        {
        };

        PH_SimpleAllocator::PH_SimpleAllocator()
        {
        }

        // Does it even need to have destructor?
        PH_SimpleAllocator::~PH_SimpleAllocator()
        {
            // This allocator has to be deleted when Program is closed, so no deallocation needed
        }

        PH_SimpleAllocator::pointer PH_SimpleAllocator::Allocate(uint64 size, ph_mem_tag tag)
        {
           pointer ResultAllocatedMemory = nullptr;

           const uint64 SmallSize = 128;
           const uint64 MediumSize = MIBIBYTES(256);
           const uint64 BigSize = PH_MAX_ALLOCATION_SIZE;

           if (PH_MemorySystem::IsInitialized() && !(tag == PH_MEM_TAG_ALLOCATOR))
           {
              /* if (size < SmallSize)
               {
                   ResultAllocatedMemory = PH_BaseAllocator::Allocate(size, tag);
                   if (ResultAllocatedMemory)
                   {
                       AllocatedMemoryBlocks.emplace(ResultAllocatedMemory,
                           AllocationInfo{ detail::PH_Allocators_Tags_Enum::BASE_ALLOCATOR, tag });
                   };
               }
               else */if (size < MediumSize)
               {
                   //Curently Medium sizes will use Dynamic Allocator
                  // TODO: DIFFERENT ALLOCATOR FOR MEDIUM OR SMALL SIZE, DEPENDS ON DYNAMIC ALLOCATOR EFFICIENCY/STATS
                   ResultAllocatedMemory = STATIC_PH_DYNAMIC_ALLOCATOR->Allocate(size);

                   if (ResultAllocatedMemory)
                   {
                       // IF DYNAMIC ALLOCATOR WILL REGISTER ALLOCATION WITH TAG, DELETE THIS
                       PH_GlobalMemoryStats::RegisterAllocation(size, tag);
                       AllocationInfo allocInfo{ detail::PH_Allocators_Tags_Enum::DYNAMIC_ALLOCATOR, tag };
                       AllocatedMemoryBlocks.insert({ ResultAllocatedMemory,
                      allocInfo });
                   };
               }
               else if (size < BigSize)
               {
                   ResultAllocatedMemory = PH_BaseAllocator::Allocate(size, tag);
                   if (ResultAllocatedMemory)
                   {
                       AllocationInfo allocInfo{ detail::PH_Allocators_Tags_Enum::BASE_ALLOCATOR, tag };
                       AllocatedMemoryBlocks.insert({ ResultAllocatedMemory,
                           allocInfo });
                       return ResultAllocatedMemory;
                   };
               }
           }
           // if Mem System isn't initialized OR tag is PH_MEM_TAG_ALLOCATOR, make system malloc
           if (!ResultAllocatedMemory)
           {
               if (pointer allocatedMemory = PH_PlatformAllocator::Allocate(size,detail::NONE_ALLOCATOR))
               {
                   ResultAllocatedMemory = allocatedMemory;
                   if (!(tag == PH_MEM_TAG_ALLOCATOR))
                   {
                       PH_WARN("Allocation from OS LEVEL. Consider using another allocation strategy. Address: %p size: %lu tag: %s",
                               allocatedMemory, size, String_From_Ph_Mem_Tag(tag));
                   };

                   ph_zero_memory(allocatedMemory, size);

                   // register allocation im memory stats
                   AllocationInfo allocInfo{ detail::PH_Allocators_Tags_Enum::NONE_ALLOCATOR, tag };
                   AllocatedMemoryBlocks.insert({ allocatedMemory,
                       allocInfo });
                   return allocatedMemory;
               }
           };

           return ResultAllocatedMemory;
        }
        bool PH_SimpleAllocator::Deallocate(pointer memoryBlock)
        {
            bool result = false;
            AllocationsMap::iterator AllocationMemBlockInfoIter = AllocatedMemoryBlocks.find(memoryBlock);
            // If there would be troubles with perfomance, do some advanced search/book keeping of allocation
            if(AllocationMemBlockInfoIter != AllocatedMemoryBlocks.end())
            {
                const auto AllocatorTag = AllocationMemBlockInfoIter->second.UsedAllocatorTag;
                const auto AllocationTypeTag = AllocationMemBlockInfoIter->second.AllocationTag;
                switch (AllocatorTag) {
                case detail::PH_Allocators_Tags_Enum::NONE_ALLOCATOR:
                    result = PH_PlatformAllocator::Deallocate(memoryBlock);
                    break;
                case detail::PH_Allocators_Tags_Enum::BASE_ALLOCATOR:
                    result = PH_BaseAllocator::Deallocate(memoryBlock);
                    break;
                case detail::PH_Allocators_Tags_Enum::DYNAMIC_ALLOCATOR:
                    // IF DYNAMIC ALLOCATOR WILL UNREGISTER ALLOCATION INTO MEMORY SYSTEM, DELETE THIS CODE
                    const uint64 sizeOfBlock = STATIC_PH_DYNAMIC_ALLOCATOR->GetNodeSize(memoryBlock);
                    PH_ASSERT(sizeOfBlock > 0);

                    result = STATIC_PH_DYNAMIC_ALLOCATOR->Deallocate(memoryBlock);

                    PH_GlobalMemoryStats::RegisterDeallocation(sizeOfBlock, AllocationTypeTag);
                    // END OF CODE TO DELETE }
                    break;
                };
                AllocatedMemoryBlocks.erase(AllocationMemBlockInfoIter);
            }
            PH_ASSERT(result);
            // Wrong memory block(dealocated from another allocator/wrong memory address)
            return result;
        }

        PH_MemorySystem::PH_MemorySystem()
        {
        }

        PH_MemorySystem::~PH_MemorySystem()
        {

        }

        // Memory System Intializaion implementation
        void PH_MemorySystem::init()
        {
            PH_ASSERT(!stat_inst);
            PH_ASSERT(!PH_GlobalMemoryStats::stat_inst);

            if (!stat_inst)
            {
                stat_inst = new PH_MemorySystem{};
                if (!PH_GlobalMemoryStats::stat_inst)
                {
                    PH_GlobalMemoryStats::stat_inst = new PH_GlobalMemoryStats{};
                };
            }

            if (!stat_inst->IsSystemInitialized)
            {
                stat_inst->IsSystemInitialized = true;
                PH_PlatformAllocator::AllocatedMemoryBlocks.reserve(PH_MEM_PLATFORM_ALLOCATOR_ALLOCATIONS_MIN_RESERVE);
                PH_SimpleAllocator::AllocatedMemoryBlocks.reserve(PH_MEM_SIMPLE_ALLOCATOR_ALLOCATIONS_MIN_RESERVE);
                //PH_BaseAllocator::BaseAllocate(PH_MEM_BASIC_BASE_ALLOCATER_PREALLOCATION_MEMORY_SIZE);
                if (!STATIC_PH_DYNAMIC_ALLOCATOR)
                {
                    STATIC_PH_DYNAMIC_ALLOCATOR = new PH_DynamicAllocator();
                    STATIC_PH_DYNAMIC_ALLOCATOR->Resize(PH_MEM_BASIC_DYNAMIC_ALLOCATOR_PREALLOCATION_MEMORY_SIZE);
                };
            };
        }

        void PH_MemorySystem::destroy()
        {
            STATIC_PH_DYNAMIC_ALLOCATOR->Clear();
            delete PH_GlobalMemoryStats::stat_inst;
            PH_GlobalMemoryStats::stat_inst = nullptr;
            delete stat_inst;
            stat_inst = nullptr;
            //PH_SimpleAllocator::AllocatedMemoryBlocks.clear();
            // delete PH_SimpleAllocator::instance;
            // delete PH_GlobalMemoryStats::stat_inst;
            //PH_BaseAllocator::ClearMemory();
        }
        void* PH_PlatformAllocator::Allocate(uint64 size, detail::PH_Allocators_Tags_Enum tag)
        {
            void* resultPtr = malloc(size);
            PlatformAllocationInfo AllocInfo{ tag,resultPtr, size };
            AllocatedMemoryBlocks.push_back(AllocInfo);
            PH_GlobalMemoryStats::RegisterAllocation(size, PH_MEM_TAG_ALLOCATOR);
            return resultPtr;
        };

        bool PH_PlatformAllocator::Deallocate(pointer memoryBlock)
        {
            bool result = false;
            for (auto AllocInfoIter = AllocatedMemoryBlocks.begin(); AllocInfoIter != AllocatedMemoryBlocks.end(); ++AllocInfoIter) {

                if (AllocInfoIter->MemoryBlock == memoryBlock)
                {
                    free(memoryBlock);
                    PH_GlobalMemoryStats::RegisterDeallocation(AllocInfoIter->AllocSize, PH_MEM_TAG_ALLOCATOR);
                    AllocatedMemoryBlocks.erase(AllocInfoIter);
                    result = true;
                    break;
                };
            }
            if (!result)
            {
                PH_WARN("Called Platform deallocation for wrong address: %p", memoryBlock);
            };



            return result;
        }
        PH_PlatformAllocator::PH_PlatformAllocator()
        {
        }
        PH_PlatformAllocator::~PH_PlatformAllocator()
        {
        }
};
}

void PhosphorEngine::Memory::detail::InitHeader(void* block)
{
    ph_zero_memory(((uint8*)block - PH_METADATA_SIZE), PH_METADATA_SIZE);
}

void PhosphorEngine::Memory::detail::InitHeader(void* block, ph_mem_tag tag, uint64 size, bool freeFlag)
{
    ph_zero_memory(((uint8*)block - PH_METADATA_SIZE), PH_METADATA_SIZE);
    setTag(block, tag);
    setFreeFlag(block, freeFlag);
    setAllocatedBlockSize(block, size);
}
