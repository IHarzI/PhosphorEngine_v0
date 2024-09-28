#pragma once

#include "Memory/PH_Memory.h"
#include "Containers/PH_DynamicArray.h"
#include "Containers/PH_String.h"
// Free list must not have such big number of nodes

namespace PhosphorEngine {
	namespace Memory {
		using namespace Containers;
		class PH_DynamicAllocator;

		namespace detail {
			struct INTERNAL_PH_DynamicAllocator_Handle
			{
				static PH_DynamicAllocator* PH_DynamicAllocator_Stat_Inst;
			};
		}

		constexpr static uint32 PH_DYNAMIC_ALLOCATOR_FreeIdsUseThreshold = 64;
		constexpr static uint32 PH_DYNAMIC_ALLOCATOR_MaxAllocationsDefault = 50 * 1024;
		constexpr static uint32 PH_DYNAMIC_ALLOCATOR_MinAllocSizeRequirement = 256;
		constexpr static uint32 PH_DYNAMIC_ALLOCATOR_InvalidNodeID = 0xFFFFFFFF;

		class PH_DynamicAllocator
		{
		public:
			using pointer = void*;
			using NodeID_T = uint64;
			using Size_T = uint64;
			PH_DynamicAllocator();
			PH_DynamicAllocator(Size_T BaseAllocationSize, Size_T MaxAllocations = PH_DYNAMIC_ALLOCATOR_MaxAllocationsDefault);
			~PH_DynamicAllocator();
			PH_DynamicAllocator(PH_DynamicAllocator& other_alloc) = delete;
			PH_DynamicAllocator(PH_DynamicAllocator&& other_alloc) = delete;

			// Resize Dynamic Allocator
			// Return True if operation succesfull
			// False if can't allocate new chunk of memory 
			// OR (no space/not enought space) to deallocate(if initial size was bigger than input to function)
			bool Resize(Size_T SizeToChange);

			// Allocate block of memory from FreeList free space
			pointer Allocate(Size_T size);

			// Deallocate block of memory from FreeList free space
			bool Deallocate(pointer address);

			PH_INLINE Size_T GetTotalSize() const { return total_size; };
			PH_INLINE Size_T GetFreeSpaceSize() const { return free_space_size; };
			PH_INLINE Size_T GetOccupiedSpace() const { PH_ASSERT(free_space_size <= total_size); return total_size - free_space_size; };

			PH_String GetAllocatorStats() const;

			// Clear all allocated memory
			void Clear();

		private:

			struct MemoryHeaderBlockNode
			{
				MemoryHeaderBlockNode()
				{
					Size = 0;
					NodeMemory = nullptr;
					NextNodeIndex = PH_DYNAMIC_ALLOCATOR_InvalidNodeID;
					IsNextNodeAdjacent = 0;
					IsBlockFree = 1;
					IsPrimaryAllocated = 0;
				};

				Size_T Size = 0;
				void* NodeMemory = nullptr;
				NodeID_T NextNodeIndex = 0;

				// ==================== FLAGS

				// Adjacent memory flag(to know if next block of memory in List is "neighbor" to this node's block of memory)
				uint8 IsNextNodeAdjacent : 1;
				uint8 IsBlockFree : 1;
				uint8 IsPrimaryAllocated : 1;
			};

			PH_MEMORY_SYSTEM_FRIEND;
			using InternalAllocator = Memory::PH_PlatformAllocator;
			friend class PH_PlatformAllocator;
			friend class PH_SimpleAllocator;
			MemoryHeaderBlockNode GetNodeMetadata(pointer nodeMemory);
			Size_T GetFreeNodeIndex();
			Size_T GetNodeSize(pointer nodeMemory);

			// Check if we have enough free indexes in bin to use them and set true, if it is
			PH_INLINE bool CheckAndSetFreeIdsUse()
			{
				// Calculate min trigger value
				const bool UseFreeNodeIdsTriggerValue = NodesIdsFreeBin.size() > PH_DYNAMIC_ALLOCATOR_FreeIdsUseThreshold;

				if (UseFreeNodeIdsTriggerValue)
				{
					UseFreeBinNodesID = 1;
				}

				return UseFreeNodeIdsTriggerValue;
			};

			Size_T total_size = 0;
			Size_T free_space_size = 0;

			NodeID_T HeadNodeIndex = PH_DYNAMIC_ALLOCATOR_InvalidNodeID;
			NodeID_T LastNodeIndex = PH_DYNAMIC_ALLOCATOR_InvalidNodeID;

			uint8 UseFreeBinNodesID : 1;

			PH_DynamicArray<MemoryHeaderBlockNode> Nodes{};
			PH_DynamicArray<NodeID_T> NodesIdsFreeBin{};
		};
	};
};


