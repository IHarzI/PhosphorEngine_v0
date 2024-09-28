#pragma once

#include <vector>
#include "Memory/PH_Memory.h"
#include "Memory/PH_DynamicAllocator.h"
#include "Containers/PH_Iterator.h"

namespace PhosphorEngine {
	namespace Containers {

		// Ring buffer container with dynamic size. Could be used as static, if allocator is static, but resize operation will be limited 
		// by allocation memory size. Allocator Type must have following methods:
		// Allocate(size_t bytes_to_allocate), Deallocate(void* MemoryToDeallocate)
		// and be **Copy/Default Constructable**(to be able to construct/copy construct RingBuffer)
		// Value Type must be Default constructable and movable

		template<typename ValueT, typename AllocatorT = Memory::PH_SimpleAllocator>
		class PH_RingBuffer
		{
		public:
			using Iterator = PH_Iterator<ValueT, PH_RingBuffer>;
			friend Iterator;

			using SizeType = uint64;
			using IndexT = SizeType;
			using InternalAllocator = AllocatorT;

			PH_RingBuffer();
			PH_RingBuffer(PH_RingBuffer& Other);
			PH_RingBuffer(PH_RingBuffer&& Other);
			PH_RingBuffer(SizeType capacity);
			PH_RingBuffer(AllocatorT& allocator);
			PH_RingBuffer(std::initializer_list<ValueT> list);
			~PH_RingBuffer();

			IndexT PushBack(ValueT value);
			IndexT EmplaceBack(ValueT&& value);
			IndexT PushFront(ValueT value);
			IndexT EmplaceFront(ValueT&& value);

			PH_INLINE void Clear() {
				head = InvalidIndex(); 
				elementsInside = 0;
			}

			// Look at the first front element, don't use a pointer after pushes/emplacements elements inside the ring
			ValueT* PeekFront();

			// Look at the first back element, don't use a pointer after pushes/emplacements elements inside the ring
			ValueT* PeekBack();

			// Look at the first front element, don't use a pointer after pushes/emplacements elements inside the ring
			const	ValueT* PeekFront()	const;

			// Look at the first back element, don't use a pointer after pushes/emplacements elements inside the ring
			const	ValueT* PeekBack()	const;

			// Pop element from front
			ValueT&& PopFront();
			// Pop element from back
			ValueT&& PopBack();

			// Look at this index in the container, don't use a pointer after pushes/emplacements elements inside the ring
			// NOTE: if index will be out of bounds(more that head index and less that tail index) or incorrect, return will be nullptr
			ValueT* LookAtIndex(IndexT index);

			// Look at this index in the container, don't use a pointer after pushes/emplacements elements inside the ring
			// NOTE: if index will be out of bounds(more that head index and less that tail index) or incorrect, return will be nullptr
			const ValueT* LookAtIndex(IndexT index) const;

			// Resize container
			bool Resize(SizeType capacity);

			// Get capacity;
			PH_INLINE SizeType GetCapacity() const { return capacity; };

			// Get number of elements inside
			PH_INLINE SizeType GetSize() const { return elementsInside; };

			// Get head index;
			PH_INLINE IndexT GetHeadIndex() const { return head; };

			// Get tail index
			PH_INLINE IndexT GetTailIndex() const;

			// Get tail index
			PH_INLINE IndexT Empty() const {
				return elementsInside == 0;
			};

			PH_INLINE constexpr IndexT InvalidIndex() const { return IndexT(-1); };
			
			// Ranges and useful operators
			PH_INLINE constexpr SizeType size() const { return elementsInside; };

			PH_INLINE ValueT& operator[](SizeType index) { PH_ASSERT(index < capacity); return PointToValueAtIndex(index); }
			PH_INLINE const ValueT& operator[](SizeType index) const { PH_ASSERT(index < capacity); return PointToValueAtIndex(index); }

			PH_INLINE ValueT& at(SizeType index) { PH_ASSERT(index < capacity); return PointToValueAtIndex(index); }
			PH_INLINE const ValueT& at(SizeType index) const { PH_ASSERT(index < capacity); return PointToValueAtIndex(index); }

			PH_INLINE constexpr ValueT* data() noexcept { return (ValueT*)MemoryBlock; };
			PH_INLINE constexpr const ValueT* data() const noexcept { return (ValueT*)MemoryBlock; };

			PH_INLINE constexpr ValueT* begin()	const { return (ValueT*)MemoryBlock; };
			PH_INLINE constexpr ValueT* end()	const { return (ValueT*)(MemoryBlock) + size(); };

		private:
			PH_INLINE ValueT* PointToValueAtIndex(size_t index);
			PH_INLINE ValueT** GetData() { return MemoryBlock; }
			PH_INLINE IndexT GetNextHeadIndex() const;
			PH_INLINE IndexT GetNextTailIndex() const;
			InternalAllocator m_InternalAllocator;
			ValueT** MemoryBlock = nullptr;;
			SizeType capacity = 0;
			IndexT head = InvalidIndex();
			SizeType elementsInside = 0;
		};

		template<typename ValueT, typename AllocatorT>
		class PH_Iterator <ValueT, PH_RingBuffer<ValueT, AllocatorT>>
		{

		};

		template<typename ValueT, typename AllocatorT>
		PH_RingBuffer<ValueT, AllocatorT>::PH_RingBuffer()
		{
			head = InvalidIndex();
		};

		template<typename ValueT, typename AllocatorT>
		PH_RingBuffer<ValueT, AllocatorT>::PH_RingBuffer(PH_RingBuffer& Other)
		{
			Resize(Other.capacity);
			if (Other.elementsInside > 0)
			{
				detail::CopyMemory(Other.MemoryBlock, MemoryBlock, capacity);
				head = Other.head;
				elementsInside = Other.elementsInside;
			};
		};

		template<typename ValueT, typename AllocatorT>
		PH_RingBuffer<ValueT, AllocatorT>::PH_RingBuffer(PH_RingBuffer&& Other)
		{
			MemoryBlock = Other.MemoryBlock;
			head = Other.head;
			elementsInside = Other.elementsInside;
			capacity = Other.capacity;
			m_InternalAllocator = Other.m_InternalAllocator;
		};

		template<typename ValueT, typename AllocatorT>
		PH_RingBuffer<ValueT, AllocatorT>::PH_RingBuffer(SizeType capacity)
		{
			if (capacity > 0 && capacity != InvalidIndex())
			{
				MemoryBlock = (ValueT**)m_InternalAllocator.Allocate(capacity * (sizeof(ValueT)), PH_MEM_TAG_RINGBUFFER);
				if (MemoryBlock)
				{
					this->capacity = capacity;
					head = InvalidIndex();
				}
			};
		};

		template<typename ValueT, typename AllocatorT>
		PH_RingBuffer<ValueT, AllocatorT>::PH_RingBuffer(AllocatorT& allocator)
		{
			head = InvalidIndex();
			m_InternalAllocator = allocator;
		}

		template<typename ValueT, typename AllocatorT>
		PH_RingBuffer<ValueT, AllocatorT>::PH_RingBuffer(std::initializer_list<ValueT> list)
		{
			head = InvalidIndex();
			Resize(list.size() * sizeof(ValueT) * 8);
			for (auto& Value : list)
			{
				PushBack(Value);
			}
		};

		template<typename ValueT, typename AllocatorT>
		PH_RingBuffer<ValueT, AllocatorT>::~PH_RingBuffer()
		{
			if (MemoryBlock)
			{
				m_InternalAllocator.Deallocate(MemoryBlock);
			}
		};

		template<typename ValueT, typename AllocatorT>
		uint64 PH_RingBuffer<ValueT, AllocatorT>::PushBack(ValueT value)
		{
			if (MemoryBlock && capacity > elementsInside)
			{
				IndexT IndexForPushedElement = 0;

				if (!(GetTailIndex() == InvalidIndex()))
				{
					IndexForPushedElement = GetNextTailIndex();
				};

				ValueT* ItemAtIndex = PointToValueAtIndex(IndexForPushedElement);

				*ItemAtIndex = value;
				elementsInside++;
				if (head == InvalidIndex())
				{
					head = IndexForPushedElement;
				};
				return IndexForPushedElement;
			}
			return InvalidIndex();
		};

		template<typename ValueT, typename AllocatorT>
		uint64 PH_RingBuffer<ValueT, AllocatorT>::EmplaceBack(ValueT&& value)
		{
			if (MemoryBlock && capacity > elementsInside)
			{
				IndexT IndexForEmplacedElement = 0;

				if (!(GetTailIndex() == InvalidIndex()))
				{
					IndexForEmplacedElement = GetNextTailIndex();
				};

				std::swap(*PointToValueAtIndex(IndexForEmplacedElement), value);
				elementsInside++;

				// If it's first push/emplace of element, set head by this index
				if (head == InvalidIndex())
				{
					head = IndexForEmplacedElement;
				}

				return IndexForEmplacedElement;
			};
			return InvalidIndex();
		};

		template<typename ValueT, typename AllocatorT>
		uint64 PH_RingBuffer<ValueT, AllocatorT>::PushFront(ValueT value)
		{
			if (MemoryBlock && capacity > elementsInside)
			{
				IndexT IndexForPushedElement = 0;

				if (!(head == InvalidIndex()))
				{
					IndexForPushedElement = GetNextHeadIndex();
				};

				*PointToValueAtIndex(IndexForPushedElement) = value;
				head = IndexForPushedElement;
				elementsInside++;
				return IndexForPushedElement;
			};
			return InvalidIndex();
		};

		template<typename ValueT, typename AllocatorT>
		uint64 PH_RingBuffer<ValueT, AllocatorT>::EmplaceFront(ValueT&& value)
		{
			if (MemoryBlock && capacity > elementsInside)
			{
				IndexT IndexForEmplacedElement = 0;

				if (!(head == InvalidIndex()))
				{
					IndexForEmplacedElement = GetNextHeadIndex();
				};

				std::swap(*PointToValueAtIndex(IndexForEmplacedElement), value);
				head = IndexForEmplacedElement;
				elementsInside++;
				return IndexForEmplacedElement;
			};
			return InvalidIndex();
		};

		template<typename ValueT, typename AllocatorT>
		ValueT* PH_RingBuffer<ValueT, AllocatorT>::PeekFront()
		{
			ValueT* result = nullptr;

			if (head != InvalidIndex())
				result = PointToValueAtIndex(head);
			return result;
		};

		template<typename ValueT, typename AllocatorT>
		ValueT* PH_RingBuffer<ValueT, AllocatorT>::PeekBack()
		{
			ValueT* result = nullptr;

			if (GetTailIndex() != InvalidIndex())
				result = PointToValueAtIndex(GetTailIndex());
			return result;
		};

		template<typename ValueT, typename AllocatorT>
		const ValueT* PH_RingBuffer<ValueT, AllocatorT>::PeekFront() const
		{
			ValueT* result = nullptr;

			if (head != InvalidIndex())
				result = PointToValueAtIndex(head);
			return result;
		};

		template<typename ValueT, typename AllocatorT>
		const ValueT* PH_RingBuffer<ValueT, AllocatorT>::PeekBack() const
		{
			ValueT* result = nullptr;

			if (GetTailIndex() != InvalidIndex())
				result = PointToValueAtIndex(GetTailIndex());
			return result;
		};

		template<typename ValueT, typename AllocatorT>
		ValueT&& PH_RingBuffer<ValueT, AllocatorT>::PopFront()
		{
			ValueT* Result = nullptr;
			if (head != InvalidIndex())
			{
				Result = PointToValueAtIndex(head);
				if (elementsInside > 1)
				{
					elementsInside--;
					// Avoid "underflow?" if head == 0
					if (head == 0)
						head = capacity - 1;
					else
						head = head - 1 % capacity;
				}
				else
				{
					head = InvalidIndex();
					elementsInside = 0;
				};
			}
			if (Result)
			{
				return std::move(*Result);
			};

			return ValueT{};
		};

		template<typename ValueT, typename AllocatorT>
		ValueT&& PH_RingBuffer<ValueT, AllocatorT>::PopBack()
		{
			ValueT* Result = nullptr;
			if (GetTailIndex() != InvalidIndex())
			{
				Result = PointToValueAtIndex(GetTailIndex());
				if (elementsInside > 1)
				{
					elementsInside--;
				}
				else
				{
					head = InvalidIndex();
					elementsInside = 0;
				}
			};

			if (Result)
			{
				return std::move(*Result);
			};

			return ValueT{};
		};

		template<typename ValueT, typename AllocatorT>
		ValueT* PH_RingBuffer<ValueT, AllocatorT>::LookAtIndex(IndexT index)
		{
			if (index >= capacity ||
				elementsInside == 0 ||
				index == InvalidIndex() ||
				index < GetTailIndex() && index > GetHeadIndex() ||
				index > GetTailIndex() && index > GetHeadIndex() && GetTailIndex() <= GetHeadIndex())
				return nullptr;
			return (ValueT*)GetData() + index;
		};

		template<typename ValueT, typename AllocatorT>
		const ValueT* PH_RingBuffer<ValueT, AllocatorT>::LookAtIndex(IndexT index) const
		{
			if (index >= capacity ||
				elementsInside == 0 ||
				index == InvalidIndex() ||
				index < GetTailIndex() && index > GetHeadIndex() ||
				index > GetTailIndex() && index > GetHeadIndex() && GetTailIndex() <= GetHeadIndex())
				return nullptr;
			return (ValueT*)GetData() + index;
		};

		template<typename ValueT, typename AllocatorT>
		bool PH_RingBuffer<ValueT, AllocatorT>::Resize(SizeType NewCapacity)
		{
			if (NewCapacity > 0 && NewCapacity != size_t(-1) && NewCapacity >= elementsInside)
			{
				ValueT** NewAllocatedMemory = (ValueT**)m_InternalAllocator.Allocate(NewCapacity * sizeof(ValueT), PH_MEM_TAG_RINGBUFFER);
				if (NewAllocatedMemory)
				{
					if (MemoryBlock)
					{
						if (elementsInside > 0 && head != InvalidIndex())
						{
							IndexT TailIndex = 0;

							if (GetTailIndex() != InvalidIndex())
								TailIndex = GetTailIndex();

							if (TailIndex > head)
							{
								// Just copy value in loop
								IndexT StartIndexForTailPart = 0;
								for (IndexT copyIndex = TailIndex; copyIndex < capacity; copyIndex++)
								{
									*((ValueT*)NewAllocatedMemory + StartIndexForTailPart++) = *((ValueT*)MemoryBlock + copyIndex);
								};
								for (IndexT copyIndex = 0; copyIndex <= head; copyIndex++)
								{
									*((ValueT*)NewAllocatedMemory + StartIndexForTailPart++) = *((ValueT*)MemoryBlock + copyIndex);
								};
								
								// Update info about container
								head = elementsInside - 1;
							}
							else
							{
								// copy all elements into new container
								Memory::ph_copy_memory(GetData(), NewAllocatedMemory, head+1 * sizeof(ValueT));
							}
						};
						m_InternalAllocator.Deallocate(GetData());
					};
					capacity = NewCapacity;
					MemoryBlock = NewAllocatedMemory;
					return true;
				};
			};
			return false;
		};

		template<typename ValueT, typename AllocatorT>
		PH_INLINE uint64 PH_RingBuffer<ValueT, AllocatorT>::GetTailIndex() const
		{
			if (capacity == 0)
			{
				return InvalidIndex();
			};

			if (head == InvalidIndex())
			{
				return InvalidIndex();
			};

			PH_ASSERT(elementsInside > 0);

			if (elementsInside == 1)
			{
				return head;
			};

			return head < elementsInside - 1 ? capacity - (elementsInside - head - 1) : head - (elementsInside - 1);
		}

		template<typename ValueT, typename AllocatorT>
		PH_INLINE ValueT* PH_RingBuffer<ValueT, AllocatorT>::PointToValueAtIndex(size_t index)
		{
			if (index >= capacity)
				return nullptr;

			return (ValueT*)GetData() + index;
		}

		template<typename ValueT, typename AllocatorT>
		PH_INLINE uint64 PH_RingBuffer<ValueT, AllocatorT>::GetNextHeadIndex() const
		{
			if (capacity == 0 || capacity == elementsInside)
			{
				return InvalidIndex();
			}
			IndexT NextIndex = head == capacity - 1 ? 0 : head + 1;
			return NextIndex;
		};

		template<typename ValueT, typename AllocatorT>
		PH_INLINE uint64 PH_RingBuffer<ValueT, AllocatorT>::GetNextTailIndex() const
		{
			if (capacity == 0 || capacity == elementsInside)
			{
				return InvalidIndex();
			}
			IndexT NextIndex = GetTailIndex() == 0 ? capacity - 1 : GetTailIndex() - 1;
			return NextIndex;
		};
	};
};
