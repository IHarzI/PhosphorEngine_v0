#include "PH_DynamicAllocator.h"
#include "PH_MemoryStatistic.h"

#include <sstream>

namespace PhosphorEngine
{
	namespace Memory {
		namespace detail {
			PH_DynamicAllocator* INTERNAL_PH_DynamicAllocator_Handle::PH_DynamicAllocator_Stat_Inst{};
		};

		PH_DynamicAllocator::PH_DynamicAllocator()
		{
			UseFreeBinNodesID = 0;
			total_size = 0;
			free_space_size = 0;
			Nodes.reserve(PH_DYNAMIC_ALLOCATOR_MinAllocSizeRequirement);
			NodesIdsFreeBin.reserve(PH_DYNAMIC_ALLOCATOR_MinAllocSizeRequirement);
		};
		
		PH_DynamicAllocator::PH_DynamicAllocator(Size_T BaseAllocationSize, Size_T MaxAllocations)
		{
			UseFreeBinNodesID = 0;
			total_size = 0;
			free_space_size = 0;
			Nodes.reserve(MaxAllocations);
			NodesIdsFreeBin.reserve(MaxAllocations);
			Resize(BaseAllocationSize);
		}

		PH_DynamicAllocator::~PH_DynamicAllocator()
		{
			Clear();
		};

		bool PH_DynamicAllocator::Resize(Size_T SizeToChange)
		{
			bool result = true;
			// if new size is smaller than node for allocated memory block
			if (SizeToChange <= PH_DYNAMIC_ALLOCATOR_MinAllocSizeRequirement)
			{
				PH_WARN("Dynamic Allocator resize with small amount of memory %llu Bytes.\
							 Consider using another allocator/data structure for allocations.", SizeToChange);
			};

			if (Nodes.empty() && total_size == 0)
			{
				// Head node must be invalid
				PH_ASSERT(HeadNodeIndex == PH_DYNAMIC_ALLOCATOR_InvalidNodeID);

				void* allocatedMemoryBlockForResize = InternalAllocator::Allocate(SizeToChange,detail::DYNAMIC_ALLOCATOR);
				PH_ASSERT_MSG(allocatedMemoryBlockForResize, "Failed to allocated memory for Dynamic Allocator resize,\
							requsted size for resize %llu, allocator total size: %llu", SizeToChange,this->total_size);

				total_size = SizeToChange;
				free_space_size = SizeToChange;
				HeadNodeIndex = 0; 
				LastNodeIndex = 0;
				MemoryHeaderBlockNode NewReservedNode{};
				NewReservedNode.IsNextNodeAdjacent = 0;
				NewReservedNode.IsPrimaryAllocated = 1;
				NewReservedNode.IsBlockFree = 1;
				NewReservedNode.NodeMemory = allocatedMemoryBlockForResize;
				NewReservedNode.Size = SizeToChange;
				NewReservedNode.NextNodeIndex = PH_DYNAMIC_ALLOCATOR_InvalidNodeID;
				Nodes.push_back(std::move(NewReservedNode));
			}
			else
			{
				PH_ASSERT(SizeToChange != 0);

				// if Size is less than current size, than this call SHOULD decrease size of allocator
				//Try to fully deallocate memory from Allocator preallocated space
				if (SizeToChange < total_size && free_space_size >= SizeToChange)
				{
					NodeID_T previousNodeID = PH_DYNAMIC_ALLOCATOR_InvalidNodeID;
					for (NodeID_T nodeIndex = HeadNodeIndex; nodeIndex != PH_DYNAMIC_ALLOCATOR_InvalidNodeID; nodeIndex = Nodes.at(nodeIndex).NextNodeIndex)
					{
						auto& NodeToDealloc = Nodes.at(nodeIndex);
						if (NodeToDealloc.IsPrimaryAllocated == 1 &&
							NodeToDealloc.IsBlockFree == 1 && 
							NodeToDealloc.IsNextNodeAdjacent == 0)
						{
							InternalAllocator::Deallocate(NodeToDealloc.NodeMemory);
							NodesIdsFreeBin.push_back(nodeIndex);
							free_space_size -= NodeToDealloc.Size;
							total_size -= NodeToDealloc.Size;

							if (nodeIndex == HeadNodeIndex)
							{
								HeadNodeIndex = NodeToDealloc.NextNodeIndex;
							}

							if (previousNodeID != PH_DYNAMIC_ALLOCATOR_InvalidNodeID)
							{
								if (nodeIndex == LastNodeIndex)
								{
									LastNodeIndex = previousNodeID;
								}
								else
								{
									Nodes.at(previousNodeID).NextNodeIndex = NodeToDealloc.NextNodeIndex;
								}
							};

							// Invalidate node
							NodeToDealloc = MemoryHeaderBlockNode{};

							if (free_space_size <= SizeToChange || total_size <= SizeToChange)
							{
								break;
							}
						}

						previousNodeID = nodeIndex;
					};

					CheckAndSetFreeIdsUse();

					if (total_size >= SizeToChange || free_space_size >= SizeToChange)
					{
						return false;
					}
				}
				// If size is more than current size of allocated memory, allocate new block and add it to the total space
				else
				{
					uint64 SizeToAllocate = SizeToChange - total_size;
					void* allocatedMemoryBlockForResize = InternalAllocator::Allocate(SizeToAllocate, detail::DYNAMIC_ALLOCATOR);
					PH_ASSERT_MSG(allocatedMemoryBlockForResize, "Failed to allocated memory for Dynamic Allocator resize,\
							requsted size for resize %llu, allocator total size: %llu", SizeToChange, total_size);
					MemoryHeaderBlockNode NewReservedNode{};
					NewReservedNode.IsNextNodeAdjacent = 0;
					NewReservedNode.IsPrimaryAllocated = 1;
					NewReservedNode.IsBlockFree = 1;
					NewReservedNode.NodeMemory = allocatedMemoryBlockForResize;
					NewReservedNode.Size = SizeToAllocate;
					NewReservedNode.NextNodeIndex = PH_DYNAMIC_ALLOCATOR_InvalidNodeID;
					Nodes.push_back(std::move(NewReservedNode));

					free_space_size += SizeToAllocate;
					total_size = SizeToChange;

					// Update data about last node
					if (LastNodeIndex == PH_DYNAMIC_ALLOCATOR_InvalidNodeID)
					{
						PH_ASSERT("LastNodeIndex should be valid at this point (bug?)");
						LastNodeIndex = Nodes.size() - 1;
					}
					else
					{
						Nodes.at(LastNodeIndex).NextNodeIndex = Nodes.size() - 1;
						LastNodeIndex = Nodes.size() - 1;
					};
				};
			}
			return result;
		};

		void* PH_DynamicAllocator::Allocate(Size_T size)
		{
			void* resultPointer = nullptr;

			// TODO: Small allocation optimization? Different pool with memory, optimized for small allocation inside dynamic allocator, so 
			// small allocations will be packed all together and aside from other "big/medium" allocations
			//if (size <= PH_DYNAMIC_ALLOCATOR_MinAllocSizeRequirement)
			//	PH_WARN("Allocation of small amount of memory from Dynamic Allocator, consider using another allocator. \
			//				Requested size to allocate: %llu", size);

			if (size > free_space_size)
				Resize(total_size + size);
				//return resultPointer;
			// If we have already memory blocks in freelist(always should)
			if (Nodes.size() > 0)
			{
				// Loop through nodes by using indexes for vector 
				// (order could be "random", because of previous allocations, for example: 0 5 9 2 3 1 ....)
				NodeID_T BestNodeIDForAllocation = PH_DYNAMIC_ALLOCATOR_InvalidNodeID;
				for (NodeID_T nodeIndex = HeadNodeIndex; nodeIndex != PH_DYNAMIC_ALLOCATOR_InvalidNodeID; nodeIndex = Nodes.at(nodeIndex).NextNodeIndex)
				{
					MemoryHeaderBlockNode& NodeCandidateHeader = Nodes.at(nodeIndex);
					// Check if Node is suitable for allocation
					if (NodeCandidateHeader.Size >= size && NodeCandidateHeader.IsBlockFree == 1)
					{
						// 1 time stop...
						if (BestNodeIDForAllocation == PH_DYNAMIC_ALLOCATOR_InvalidNodeID)
							BestNodeIDForAllocation = nodeIndex;

						// If candidate is better that current BestNode, make candidate The Best
						if (BestNodeIDForAllocation != PH_DYNAMIC_ALLOCATOR_InvalidNodeID
							&& Nodes.at(BestNodeIDForAllocation).Size > NodeCandidateHeader.Size)
						{
							BestNodeIDForAllocation = nodeIndex;
						};
					};
				}

				if (BestNodeIDForAllocation == PH_DYNAMIC_ALLOCATOR_InvalidNodeID)
					// Do New Allocation for block of memory
				{
					PH_WARN("No more space in Dynamic Allocator for allocation(Out of space/Fragmentation of memory blocks), | free space: %lu | asked size to allocate: %lu | Dynamic Allocator must do resizing", free_space_size, size);
					Resize(total_size + size);
					BestNodeIDForAllocation = LastNodeIndex;
				}

				// IF we found the best fitted node or make resizing before this ^^^
				if(BestNodeIDForAllocation!= PH_DYNAMIC_ALLOCATOR_InvalidNodeID)
				{
					MemoryHeaderBlockNode& BestNode = Nodes.at(BestNodeIDForAllocation);
					// Check if we can make new memory node block from lefted memory in this memory node block
					if (BestNode.Size > size && BestNode.Size - size >= PH_DYNAMIC_ALLOCATOR_MinAllocSizeRequirement)
					{
						// Create new node from remainded memory
						MemoryHeaderBlockNode NewNodeFromRemaindedMemoryInBestNode{};
						NewNodeFromRemaindedMemoryInBestNode.IsNextNodeAdjacent = BestNode.IsNextNodeAdjacent;
						NewNodeFromRemaindedMemoryInBestNode.NextNodeIndex = BestNode.NextNodeIndex;
						NewNodeFromRemaindedMemoryInBestNode.IsBlockFree = 1;
						NewNodeFromRemaindedMemoryInBestNode.NodeMemory = (void*)((uint8*)BestNode.NodeMemory + size);
						NewNodeFromRemaindedMemoryInBestNode.Size = BestNode.Size - size;
						NewNodeFromRemaindedMemoryInBestNode.IsPrimaryAllocated = 0;

						NodeID_T NewNodeID = PH_DYNAMIC_ALLOCATOR_InvalidNodeID;

						if (UseFreeBinNodesID == 0)
						{
							Nodes.push_back(std::move(NewNodeFromRemaindedMemoryInBestNode));
							NewNodeID = Nodes.size() - 1;
						}
						else
						{
							PH_ASSERT(NodesIdsFreeBin.size() > 0);
							uint64 FreeIndex = NodesIdsFreeBin.at(NodesIdsFreeBin.size() - 1);
							std::swap(Nodes.at(FreeIndex),NewNodeFromRemaindedMemoryInBestNode);
							NodesIdsFreeBin.pop_back();
							NewNodeID = FreeIndex;
							// If no more free indexes, uncheck this flag
							if (NodesIdsFreeBin.size() == 0)
								UseFreeBinNodesID = 0;
						}

						PH_ASSERT(NewNodeID != PH_DYNAMIC_ALLOCATOR_InvalidNodeID);

						// if it's created at the end of "list", update last node index
						if (LastNodeIndex == BestNodeIDForAllocation || LastNodeIndex == PH_DYNAMIC_ALLOCATOR_InvalidNodeID)
							LastNodeIndex = NewNodeID;

						// Edit best node with taking in mind of loosed memory block in the end
						BestNode.IsNextNodeAdjacent = 1;
						BestNode.IsBlockFree = 0;
						BestNode.Size = size;
						BestNode.NextNodeIndex = NewNodeID;
					}
					else
					{
						BestNode.IsBlockFree = 0;
					}
					resultPointer = BestNode.NodeMemory;
					free_space_size -= BestNode.Size;
				}
			}

			return resultPointer;
		}

		bool PH_DynamicAllocator::Deallocate(pointer address)
		{
			NodeID_T previousNodeIndex = PH_DYNAMIC_ALLOCATOR_InvalidNodeID;
			for (NodeID_T currentNodeIndex = HeadNodeIndex;
				currentNodeIndex != PH_DYNAMIC_ALLOCATOR_InvalidNodeID; 
				currentNodeIndex = Nodes.at(currentNodeIndex).NextNodeIndex)
			{
				if (Nodes.at(currentNodeIndex).NodeMemory == address)
				{
					MemoryHeaderBlockNode& DealocatedNode = Nodes.at(currentNodeIndex);
					DealocatedNode.IsBlockFree = 1;
					free_space_size += DealocatedNode.Size;

					// Check if next to deacloted node is free and adjacent(next in memory),
					if (DealocatedNode.NextNodeIndex != PH_DYNAMIC_ALLOCATOR_InvalidNodeID &&
						DealocatedNode.IsNextNodeAdjacent == 1					  &&
						Nodes.at(DealocatedNode.NextNodeIndex).IsBlockFree == 1)
					{
						// If it is, than add it's size(update other stuff) and make this(next) node as "empty"
						
						// Update info about this node, add size of next node, next node index, etc...
						uint64 NextBlockIndex = DealocatedNode.NextNodeIndex;
						MemoryHeaderBlockNode& NextToDealocatedBlock = Nodes.at(NextBlockIndex);
						DealocatedNode.IsNextNodeAdjacent = NextToDealocatedBlock.IsNextNodeAdjacent;
						DealocatedNode.Size += NextToDealocatedBlock.Size;
						DealocatedNode.NextNodeIndex = NextToDealocatedBlock.NextNodeIndex;

						// Make this adjacent node invalid
						NextToDealocatedBlock = MemoryHeaderBlockNode{};

						// If next block is last, make this as last block
						if (LastNodeIndex == NextBlockIndex)
							LastNodeIndex = currentNodeIndex;

						// Push this adjacent node index into free node's indexes bin
						NodesIdsFreeBin.push_back(NextBlockIndex);
					};

					// Check if previous node is adjacent to this and is empty
					if (previousNodeIndex!= PH_DYNAMIC_ALLOCATOR_InvalidNodeID &&
						Nodes.at(previousNodeIndex).IsNextNodeAdjacent == 1 && 
						Nodes.at(previousNodeIndex).IsBlockFree == 1)
					{
						// If it is, than add to previous node size of current node, update other information
						// and make current node as empty
						MemoryHeaderBlockNode& PreviousNodeBlock = Nodes.at(previousNodeIndex);
						PreviousNodeBlock.Size += DealocatedNode.Size;
						PreviousNodeBlock.IsNextNodeAdjacent = DealocatedNode.IsNextNodeAdjacent;
						PreviousNodeBlock.NextNodeIndex = DealocatedNode.NextNodeIndex;
						
						// If current node is Last, make previous node as last
						if (LastNodeIndex == currentNodeIndex)
						{
							LastNodeIndex = previousNodeIndex;
						}

						// Make this dealocated node invalid
						DealocatedNode = MemoryHeaderBlockNode{};

						// Push this dealocated node index into free node's indexes bin
						NodesIdsFreeBin.push_back(currentNodeIndex);
					}

					// If we have enouth free indexes in free bin to use, set dynamic allocator to use them
					CheckAndSetFreeIdsUse();

					// MAYBE delete node, if it's primarly allocated and is free and 
					// no more chunks of memory from this node memory are in use

					return true;
				}
				previousNodeIndex = currentNodeIndex;
			};
			return false;
		}

		PH_String PH_DynamicAllocator::GetAllocatorStats() const
		{
			std::stringstream result{};
			result << "\nDynamic Allocator stats: _----------_\nDynamicAllocator address: ";
			result << this;
			result << "\nNodes: ";
			for(NodeID_T nodeIndex = HeadNodeIndex; nodeIndex != PH_DYNAMIC_ALLOCATOR_InvalidNodeID; nodeIndex = Nodes.at(nodeIndex).NextNodeIndex)
			{
				auto& NodeRef = Nodes.at(nodeIndex);
				result << " ID[" << nodeIndex << "] size[" << NodeRef.Size << ']' << std::boolalpha << " isFree[" << (bool)NodeRef.IsBlockFree << ']';
				result << " isPrimarlyAllocated[" << (bool)NodeRef.IsPrimaryAllocated << ']' << " NextNodeID[" << NodeRef.NextNodeIndex << ']';
				result << " isNextNodeAdjacent[" <<(bool)NodeRef.IsNextNodeAdjacent << ']' << " NodeAddress[" << NodeRef.NodeMemory << ']';
			}
			if (NodesIdsFreeBin.size() > 0)
			{
				result << '\n' << " FREE IDS: |";
				for (uint64 index = 0; index < NodesIdsFreeBin.size(); index++)
				{
					result << NodesIdsFreeBin[index] << '|';
				}
			}
			else
			{
				result << "\n NO FREE IDS IN THIS ALLOCATOR";
			};
			result << "\nEnd of stats : .........-__________-.........\n";
			return result.str();
		}

		void PH_DynamicAllocator::Clear()
		{
			for (NodeID_T nodeIndex = HeadNodeIndex; nodeIndex != PH_DYNAMIC_ALLOCATOR_InvalidNodeID; nodeIndex = Nodes.at(nodeIndex).NextNodeIndex)
			{
				auto& Node = Nodes.at(nodeIndex);
				if (Node.IsPrimaryAllocated == 1)
				{
					InternalAllocator::Deallocate(Node.NodeMemory);
				}
			};

			Nodes.clear();
			NodesIdsFreeBin.clear();
			HeadNodeIndex = PH_DYNAMIC_ALLOCATOR_InvalidNodeID;
			LastNodeIndex = PH_DYNAMIC_ALLOCATOR_InvalidNodeID;
			free_space_size = 0;
			total_size = 0;
			UseFreeBinNodesID = 0;
		}

		PH_DynamicAllocator::MemoryHeaderBlockNode PH_DynamicAllocator::GetNodeMetadata(pointer nodeMemory)
		{
			for (NodeID_T nodeIndex = HeadNodeIndex; nodeIndex != PH_DYNAMIC_ALLOCATOR_InvalidNodeID; nodeIndex = Nodes.at(nodeIndex).NextNodeIndex)
			{
				if (Nodes.at(nodeIndex).NodeMemory == nodeMemory)
					return Nodes.at(nodeIndex);
			};
			return {};
		}

		uint64 PH_DynamicAllocator::GetFreeNodeIndex()
		{
			for (NodeID_T nodeIndex = HeadNodeIndex; nodeIndex != PH_DYNAMIC_ALLOCATOR_InvalidNodeID; nodeIndex = Nodes.at(nodeIndex).NextNodeIndex)
			{
				if (Nodes.at(nodeIndex).IsBlockFree == 1)
					return nodeIndex;
			};
			return 0;
		}

		uint64 PH_DynamicAllocator::GetNodeSize(pointer nodeMemory)
		{
			for (NodeID_T nodeIndex = HeadNodeIndex; nodeIndex != PH_DYNAMIC_ALLOCATOR_InvalidNodeID; nodeIndex = Nodes.at(nodeIndex).NextNodeIndex)
			{
				if (Nodes.at(nodeIndex).NodeMemory == nodeMemory)
				{
					return Nodes.at(nodeIndex).Size;
				}
			};
			return 0;
		}

	};
};
