#pragma once

#include "Core/PH_CORE.h"
#include "Core/PH_Assert.h"
#include "Containers/PH_Map.h"
#include "Containers/PH_DynamicArray.h"
#include <cstdint>

#define PH_METADATA_SIZE 64ul

#define PH_MAX_UINT40 (uint64)(1099511627775ul)

// Just 40 bytes mask from 64 bit uint, so metadata property of size of allocated block in header is "uint40"
// Be aware of that when allocate, max allocation size must be now more than PH_MAX_UINT40
#define PH_METADATA_HEADER_PROPERTY_SIZE_MASK 0b1111111111111111111111111111111111111111000000000000000000000000ul

#define PH_METADATA_FREE_FLAG_MASK_IN_SECOND_BYTE 0b10000000

#define PH_SHIFT_VALUE_FOR_SIZE_FROM_UINT64 24

#define KIBIBYTES(number) (uint64)(number*1024ul)
#define MIBIBYTES(number) (uint64)(number*1024ul*1024ul)
#define GIBIBYTES(number) (uint64)(number*1024ul*1024u*1024ul)
// 8 GiB should be enough for one allocation size...
#define PH_MAX_ALLOCATION_SIZE GIBIBYTES(8)

#define PH_MEM_BASIC_BASE_ALLOCATER_PREALLOCATION_MEMORY_SIZE MIBIBYTES(1300)
#define PH_MEM_BASIC_DYNAMIC_ALLOCATOR_PREALLOCATION_MEMORY_SIZE MIBIBYTES(256)

#define PH_MEM_SIMPLE_ALLOCATOR_ALLOCATIONS_MIN_RESERVE 1024*1024
#define PH_MEM_PLATFORM_ALLOCATOR_ALLOCATIONS_MIN_RESERVE 2*1024

#define PH_MEMORY_SYSTEM_FRIEND friend struct PH_GlobalMemoryStats; friend class PH_MemorySystem;

// NOTE, Currently no allocators/memory tools are designed to be "thread-safe"

namespace PhosphorEngine {

	using namespace Containers;

	namespace Memory
	{
		PH_INLINE uint64 get_aligned(uint64 operand, uint64 granularity) {
			return ((operand + (granularity - 1)) & ~(granularity - 1));
		}

		class PH_MemorySystem final
		{
		public:
			// Memory System Intializaion, must be called first in Engine init proccess
			PH_STATIC_LOCAL void init();
			PH_STATIC_LOCAL void destroy();
			PH_INLINE PH_STATIC_LOCAL bool IsInitialized() {
				if (stat_inst) { return stat_inst->IsSystemInitialized; }; return false;
			}
		private:
			bool IsSystemInitialized = false;
			PH_STATIC_LOCAL PH_MemorySystem* stat_inst;
			PH_MemorySystem();
			~PH_MemorySystem();
			PH_INLINE PH_MemorySystem* GetInstance() { return stat_inst; };
		};

		// METADATA LAYOUT -- FULL SIZE IS 8 BYTES
		// 5 byte(uint40?) - size(in bytes)
		// 1 byte - tag(ph_mem_tag)
		// 1 bit - is free(start of 1 last byte)
		// 2(-1 bit) bytes - empty space(could be used for some information in future)
		//  ''''''''''''''''''''''''''''''''''''''''	''''''''	 '		        ''''''''''''''
		//                  size			    	       tag    free flag          empty space

		namespace detail {
			enum PH_Allocators_Tags_Enum : uint8
			{
				NONE_ALLOCATOR,
				BASE_ALLOCATOR,
				DYNAMIC_ALLOCATOR
				// SIMPLY ALLOCATOR is simply wrapper over allocations from all this allocators, so dosn't need tag
				// because this tag used (currently, at least) only for Simply allocator to know which address is from which allocator
			};
		}

		enum ph_mem_tag : uint8 {
			PH_MEM_TAG_UNKNOWN = 0,
			PH_MEM_TAG_TREE,
			PH_MEM_TAG_APPLICATION,
			PH_MEM_TAG_JOB,
			PH_MEM_TAG_ARRAY,
			PH_MEM_TAG_TEXTURE,
			PH_MEM_TAG_MATERIAL_INSTANCE,
			PH_MEM_TAG_SHADER,
			PH_MEM_TAG_RENDERER,
			PH_MEM_TAG_GAME,
			PH_MEM_TAG_GAME_OBJECT,
			PH_MEM_TAG_COMPONENT,
			PH_MEM_TAG_SCENE,
			PH_MEM_TAG_RINGBUFFER,
			PH_MEM_TAG_NODE,
			PH_MEM_TAG_ALLOCATOR,
			PH_MEM_TAG_MEMORY_HANDLE,

			PH_MEM_TAG_MAX
		};

		PH_STATIC_GLOBAL const char* array_to_char_from_ph_mem_tag[] =
		{
			"MEMORY_TAG_UNKNOWN",
			"MEMORY_TAG_TREE",
			"MEMORY_TAG_APPLICATION",
			"MEMORY_TAG_JOB",
			"MEMORY_TAG_ARRAY",
			"MEMORY_TAG_TEXTURE",
			"MEMORY_TAG_MATERIAL_INSTANCE",
			"MEMORY_TAG_SHADER",
			"MEMORY_TAG_RENDERER",
			"MEMORY_TAG_GAME",
			"MEMORY_TAG_GAME_OBJECT",
			"MEMORY_TAG_COMPONENT",
			"MEMORY_TAG_SCENE",
			"MEMORY_TAG_RING_BUFFER",
			"MEMORY_TAG_NODE",
			"MEMORY_TAG_ALLOCATOR",
			"MEMORY_TAG_MEMORY_HANDLE",
			"MEMORY_TAG_MAX"
		};

		PH_INLINE const char* String_From_Ph_Mem_Tag(ph_mem_tag tag)
		{
			const char* value = array_to_char_from_ph_mem_tag[tag];
			return value;
		}

		namespace detail {

			// Get allocated block tag (be sure that this block is allocated from right allocator)
			PH_INLINE ph_mem_tag getTagFromAllocatedBlock(void* block)
			{
				// move to flag pos(-3 bytes from header start)
				return (ph_mem_tag)((*((uint8*)block - 24)));
			}

			// Check if allocated block is free (be sure that this block is allocated from right allocator)
			PH_INLINE bool isFree(void* block)
			{
				// step back by 2 byte and get flag from last bit of byte 
				return ((*((uint8*)block - 16)) >> 7);
			}

			// Get allocated block size (be sure that this block is allocated from right allocator)
			PH_INLINE uint64 getSizeOfAllocatedBlock(void* block)
			{
				// step back by 8 bytes and get rid of last 3 bytes of other info from header
				// and return "uint40" number
				return *((uint64*)(((uint8*)block - PH_METADATA_SIZE))) >> (PH_SHIFT_VALUE_FOR_SIZE_FROM_UINT64);
			}

			// Set allocated block size (be sure that this block is allocated from right allocator)
			PH_INLINE void setAllocatedBlockSize(void* block, uint64 size)
			{
				PH_ASSERT_MSG(size < PH_MAX_UINT40, "Allocation of very large memory region");
				// step back by 8 bytes and filter other stuff from size(make all size bits in metadata 0 ZERO)
				uint64 formated_header_with_new_size = (*((uint64*)((uint8*)block - PH_METADATA_SIZE))) & ~PH_METADATA_HEADER_PROPERTY_SIZE_MASK;
				// now set size by increasing it's 64 uint representation by 23 (like shift uint40 into 64) and mask it for safety with
				// size mask(all first 40 bits will be like size other will remain as it is)
				formated_header_with_new_size |= (size << PH_SHIFT_VALUE_FOR_SIZE_FROM_UINT64) & PH_METADATA_HEADER_PROPERTY_SIZE_MASK;
				*(uint64*)((uint8*)block - PH_METADATA_SIZE) = formated_header_with_new_size;

			}

			void InitHeader(void* block);

			void InitHeader(void* block, ph_mem_tag tag, uint64 size, bool freeFlag);

			// Set allocated block tag (be sure that this block is allocated from right allocator)
			PH_INLINE void setTag(void* block, ph_mem_tag tag)
			{
				// set tag value by steppint into tag pos in header(-3 bytes)
				*((uint8*)(block)-24) = (uint8)(tag);
			}

			// Set allocated block free flag (be sure that this block is allocated from right allocator)
			PH_INLINE void setFreeFlag(void* block, bool flag)
			{
				// reset free flag and set new value
				*((uint8*)(block)-16) = (((uint8)(flag) << 7) & PH_METADATA_FREE_FLAG_MASK_IN_SECOND_BYTE) |
					*((uint8*)(block)-16) & ~(PH_METADATA_FREE_FLAG_MASK_IN_SECOND_BYTE);
			}
		}

		class PH_PlatformAllocator final
		{
		public:
			using pointer = void*;
			[[nodiscard]] PH_STATIC_LOCAL pointer Allocate(uint64 size, detail::PH_Allocators_Tags_Enum tag);
			PH_STATIC_LOCAL bool Deallocate(pointer memoryBlock);
			struct PlatformAllocationInfo {
				detail::PH_Allocators_Tags_Enum UsedAllocatorTag;
				pointer MemoryBlock;
				uint64 AllocSize;
				PH_INLINE bool operator==(PlatformAllocationInfo& other) { return MemoryBlock == other.MemoryBlock; };
			};
			using AllocationsVector = PH_DynamicArray<PH_PlatformAllocator::PlatformAllocationInfo>;
			PH_PlatformAllocator();
			~PH_PlatformAllocator();
		private:
			PH_MEMORY_SYSTEM_FRIEND
				// Platform Allocations should be minimal amount, so vector could handle it
				PH_STATIC_LOCAL AllocationsVector AllocatedMemoryBlocks;
		};

		// Wrapper over engine allocators/ system malloc(if memory sys isn't initialized/hasn't enough memory to give, SHOULDN'T never happen)
		// For everyday usage 
		class PH_SimpleAllocator final
		{
		public:
			using pointer = void*;
			PH_SimpleAllocator();
			~PH_SimpleAllocator();

			[[nodiscard]] PH_STATIC_LOCAL pointer Allocate(uint64 size, ph_mem_tag tag);
			PH_STATIC_LOCAL bool Deallocate(pointer memoryBlock);
			struct AllocationInfo {
				detail::PH_Allocators_Tags_Enum UsedAllocatorTag;
				ph_mem_tag AllocationTag;
			};
			using AllocationsMap = PH_Map<pointer, AllocationInfo>;
		private:
			PH_MEMORY_SYSTEM_FRIEND
				PH_STATIC_LOCAL AllocationsMap AllocatedMemoryBlocks;
		};

		// Simple base stack-like linear allocator 
		class PH_BaseAllocator final {
		private:
			PH_MEMORY_SYSTEM_FRIEND
				// Occupied by allocated space(by user)
				PH_STATIC_LOCAL uint64 _occupied_size;
			PH_STATIC_LOCAL uint64 _size_of_alloc_per_tag[ph_mem_tag::PH_MEM_TAG_MAX];
			PH_STATIC_LOCAL uint8** _allocated_memory;
			// Allocated by Allocator for use
			PH_STATIC_LOCAL uint64 _size_of_alloc_mem;

			// Get first block in allocator space
			PH_INLINE PH_STATIC_LOCAL void* GetFirstBlock() { return (uint8*)_allocated_memory + PH_METADATA_SIZE; };

			// Get base(first) header in allocator memory space
			PH_STATIC_LOCAL void* GetAllocatedMemoryHead() { return _allocated_memory; }

			// For DEBUG/INTERNAL purposes
			PH_STATIC_LOCAL void ClearMemory();
		public:
			PH_BaseAllocator();
			~PH_BaseAllocator();
			// Base Allocate for allocating memory space, from which allocator will give memory for usage
			PH_STATIC_LOCAL void BaseAllocate(uint64 size);

			// Allocate memory from allocator memory space
			PH_STATIC_LOCAL void* Allocate(uint64 size, ph_mem_tag tag);

			// Free allocated memory from this allocator
			PH_STATIC_LOCAL bool Deallocate(void* block);

			// Get tag from allocated block
			PH_STATIC_LOCAL ph_mem_tag GetTag(void* address);
			// Get occupied memory size
			PH_INLINE PH_STATIC_LOCAL uint64 OccupiedMemorySize() { return _occupied_size; };
			// Get allocated(free and occupied) memory size
			PH_INLINE PH_STATIC_LOCAL uint64 AllocatedMemorySize() { return _size_of_alloc_mem; };
			// Get allocated()
			PH_INLINE PH_STATIC_LOCAL uint64 AllocatedMemorySizeForTag(ph_mem_tag tag) { return _size_of_alloc_per_tag[tag]; };
		};

		PH_INLINE PH_STATIC_GLOBAL void* ph_allocate(uint64 size, ph_mem_tag tag) { return PH_SimpleAllocator::Allocate(size, tag); };

		PH_INLINE PH_STATIC_GLOBAL void ph_free(void* block) { PH_SimpleAllocator::Deallocate(block); };

		PH_INLINE PH_STATIC_GLOBAL void ph_zero_memory(void* block, uint64 size) { memset(block, 0, size); };

		PH_INLINE PH_STATIC_GLOBAL void* ph_copy_memory(void* src, void* dst, uint64 size) { memcpy_s(dst, size, src, size); return dst; };

		PH_INLINE PH_STATIC_GLOBAL void ph_set_memory(void* dst, int value, uint64 size) { memset(dst, value, size); };
	};
}

using namespace PhosphorEngine;
using namespace PhosphorEngine::Memory;

// Call deconstructor for object and release occupied memory block
template <typename T>
PH_STATIC_GLOBAL void ph_delete(T* address)
{
	address->~T();
	ph_free(address);
}


// Allocate memory block of <T> object size and call <T> constructor with arguments in-place 
template <typename T, typename ...Args>
[[nodiscard]] PH_STATIC_GLOBAL T* ph_new(ph_mem_tag tag, Args&&... args)
{
	T* ptr = (T*)ph_allocate(sizeof(T), tag);
	if (ptr)
		ptr = (new(ptr) T(std::forward<Args>(args)...)); // create object in-place
	return ptr;
}
// Allocate memory block of <T> object size and call <T> constructor in-place 
template <typename T>
[[nodiscard]] PH_STATIC_GLOBAL T* ph_new(ph_mem_tag tag)
{
	T* ptr = (T*)ph_allocate(sizeof(T), tag);
	if (ptr)
		ptr = (new(ptr) T()); // create object in-place
	return ptr;
}

template <typename T>
struct PH_WeakMemoryHandle;

// Unique memory handle that holds heap allocated data and own's it,
// so there should be no 2 unique memory handles pointing on same object(UB, if so)
// Will release data, when descucting
template <typename T>
struct PH_UniqueMemoryHandle
{
public:
	template <typename ...Args>
	PH_STATIC_LOCAL PH_UniqueMemoryHandle Create(Args&&... args)
	{
		return PH_UniqueMemoryHandle((T*)(ph_new<T>(PH_MEM_TAG_MEMORY_HANDLE, args...)));
	}

	template <typename ...Args>
	PH_STATIC_LOCAL PH_UniqueMemoryHandle CreateWithTag(ph_mem_tag MemTag, Args&&... args)
	{
		return PH_UniqueMemoryHandle((T*)(ph_new<T>(MemTag, args...)));
	}

	PH_UniqueMemoryHandle() : Data(nullptr) {};

	PH_UniqueMemoryHandle(const PH_UniqueMemoryHandle&) = delete;
	PH_UniqueMemoryHandle& operator=(const PH_UniqueMemoryHandle&) = delete;

	PH_UniqueMemoryHandle(PH_UniqueMemoryHandle& OtherHandle)
	{
		Data = OtherHandle.RetrieveResourse();
	}

	PH_UniqueMemoryHandle(PH_UniqueMemoryHandle&& OtherHandle) noexcept
	{
		Data = OtherHandle.RetrieveResourse();
	}

	PH_UniqueMemoryHandle& operator=(PH_UniqueMemoryHandle&& OtherHandle) {
		if (this != &OtherHandle) {
			if (IsValid())
			{
				Release();
			}
			Data = OtherHandle.RetrieveResourse();
		}
		return *this;
	}

	PH_UniqueMemoryHandle(T* DataToHandle)
	{
		Data = DataToHandle;
	}

	~PH_UniqueMemoryHandle()
	{
		if (IsValid())
		{
			ph_delete(Data);
		};
	}

	bool IsValid() const { return Data != nullptr; }

	T* Get() const { return Data; };
	T* Get() { return Data; };

	const T& GetReference() const { PH_ASSERT(IsValid()); return *Data; };
	T& GetReference() { PH_ASSERT(IsValid()); return *Data; };

	void Release() {
		ph_delete(Data);
		Data = nullptr;
	};

	void Reset(T* DataToHandle)
	{
		if (IsValid())
		{
			Release();
		}
		Data = DataToHandle;
	}

	void Reset(PH_UniqueMemoryHandle<T> OtherHandle)
	{
		if (IsValid())
		{
			Release();
		}
		Data = OtherHandle.RetrieveResourse();
	}

	T* RetrieveResourse() { T* ResourseToReturn = Data; Data = nullptr; return ResourseToReturn; }

	explicit operator bool() const {
		return IsValid();
	}

	bool operator==(T* DataPtr)
	{
		return Data == DataPtr;
	}

	T& operator*() const {
		return *Data;
	}

	T* operator->() const {
		return Data;
	}

	T& operator*() {
		return *Data;
	}

	T* operator->() {
		return Data;
	}

private:
	T* Data = nullptr;
};

// Shared memory handle that keeps track of ref count and deletes object, if ref count < 0 (replacement of std::shared_ptr)
// NOTE* It's not designed for multithreading!
template <typename T>
struct PH_SharedMemoryHandle
{
public:
	template <typename ...Args>
	PH_STATIC_LOCAL PH_SharedMemoryHandle Create(Args&&... args)
	{
		return PH_SharedMemoryHandle((T*)(ph_new<T>(PH_MEM_TAG_MEMORY_HANDLE, args...)));
	}

	template <typename ...Args>
	PH_STATIC_LOCAL PH_SharedMemoryHandle CreateWithTag(ph_mem_tag MemTag, Args&&... args)
	{
		return PH_SharedMemoryHandle((T*)(ph_new<T>(MemTag, args...)));
	}

	PH_SharedMemoryHandle() : Data(nullptr) {};

	PH_SharedMemoryHandle(const PH_SharedMemoryHandle& OtherHandle)
	{
		AcquireDataToHandle(OtherHandle.Get());
	}

	PH_SharedMemoryHandle& operator=(const PH_SharedMemoryHandle& OtherHandle)
	{
		if (this != &OtherHandle) {
			if (IsValid())
			{
				Release();
			}
			AcquireDataToHandle(OtherHandle.Get());
		}
		return *this;
	}

	PH_SharedMemoryHandle(PH_SharedMemoryHandle& OtherHandle)
	{
		if (IsValid())
		{
			Release();
		}
		AcquireDataToHandle(OtherHandle.Data);
	}

	PH_SharedMemoryHandle(PH_UniqueMemoryHandle<T>&& OtherUniqueHandle) noexcept
	{
		AcquireDataToHandle(OtherUniqueHandle.RetrieveResourse());
	}

	PH_SharedMemoryHandle(PH_SharedMemoryHandle&& OtherHandle) noexcept
	{
		AcquireDataToHandle(OtherHandle.Get());
		OtherHandle.Release();
	}

	PH_SharedMemoryHandle& operator=(PH_SharedMemoryHandle&& OtherHandle)
	{
		if (this != &OtherHandle) {
			if (IsValid())
			{
				Release();
			}
			AcquireDataToHandle(OtherHandle.Get());
			OtherHandle.Release();
		}
		return *this;
	}

	PH_SharedMemoryHandle(T* DataToHandle)
	{
		AcquireDataToHandle(DataToHandle);
	}

	~PH_SharedMemoryHandle()
	{
		if (IsValid())
		{
			Release();
		};
	}

	bool IsValid() const { return Data != nullptr; }

	T* Get() const { return Data; };
	T* Get() { return Data; };

	const T& GetReference() const { PH_ASSERT(IsValid()); return *Data; };
	T& GetReference() { PH_ASSERT(IsValid()); return *Data; };

	void Release() { ReleaseResourseChecked(); };

	void Reset(T* DataToHandle)
	{
		if (IsValid())
		{
			Release();
		}
		AcquireDataToHandle(DataToHandle);
	}

	void Reset(PH_SharedMemoryHandle<T> OtherHandle)
	{
		if (IsValid())
		{
			Release();
		}
		AcquireDataToHandle(OtherHandle.Get());
	}

	explicit operator bool() const {
		return IsValid();
	}

	bool operator==(T* DataPtr)
	{
		return Data == DataPtr;
	}

	bool operator==(PH_SharedMemoryHandle<T>& OtherHandle)
	{
		return Data == OtherHandle.Data;
	}
	
	bool operator!=(T* DataPtr)
	{
		return !(Data == DataPtr);
	}

	bool operator!=(PH_SharedMemoryHandle<T>& OtherHandle)
	{
		return !(Data == OtherHandle.Data_);
	}

	T& operator*() const {
		return *Data;
	}

	T* operator->() const {
		return Data;
	}

	T& operator*() {
		return *Data;
	}

	T* operator->() {
		return Data;
	}

	PH_WeakMemoryHandle<T> GetWeak() { return PH_WeakMemoryHandle<T>{ *this }; }

private:

	void AcquireDataToHandle(T* DataToHandle)
	{
		if (!DataToHandle)
		{
			return;
		}

		InitCheckedRefMap();

		if (auto RefCounterToData = RefMap->find(DataToHandle); RefCounterToData != RefMap->end())
		{
			RefCounterToData->second++;
		}
		else
		{
			RefMap->insert({ DataToHandle, 1 });
		}

		Data = DataToHandle;
	}

	void InitCheckedRefMap()
	{
		if (!RefMap)
		{
			RefMap = ph_new<RefMapType>(PH_MEM_TAG_TREE);
			PH_ASSERT(RefMap);
		}
	}

	void ReleaseRefMapIfEmpty()
	{
		if (RefMap && RefMap->size() < 1)
		{
			ph_delete(RefMap);
			RefMap = nullptr;
		}
	}

	void ReleaseResourseChecked()
	{
		if (RefMap && Data)
		{
			auto RefCounterToData = RefMap->find(Data);
			if (RefCounterToData != RefMap->end())
			{
				RefCounterToData->second--;
				if (RefCounterToData->second < 1)
				{
					RefMap->erase(Data);
					ph_delete(Data);
				}
			};
			ReleaseRefMapIfEmpty();
		};
		Data = nullptr;
	}

	T* Data = nullptr;
	using RefCount = uint64;
	using ValuePtr = T*;
	using RefMapType = PH_Map<ValuePtr, RefCount>;
	static inline RefMapType* RefMap = nullptr;
	friend PH_WeakMemoryHandle<T>;
};

template <typename T>
struct PH_WeakMemoryHandle
{
	PH_WeakMemoryHandle() : Data(nullptr) {};

	PH_WeakMemoryHandle(const PH_WeakMemoryHandle& OtherHandle)
	{
		Data = OtherHandle.Data;
	}

	PH_WeakMemoryHandle(const PH_SharedMemoryHandle<T>& SharedHandle)
	{
		Data = SharedHandle.Get();
	}

	PH_WeakMemoryHandle& operator=(const PH_WeakMemoryHandle& OtherHandle)
	{
		Data = OtherHandle.Data;
		return *this;
	}

	PH_WeakMemoryHandle(PH_WeakMemoryHandle& OtherHandle)
	{
		Data = OtherHandle.Data;
	}

	PH_WeakMemoryHandle& operator=(PH_WeakMemoryHandle&& OtherHandle)
	{
		Data = OtherHandle.Data;
		return *this;
	}

	~PH_WeakMemoryHandle()
	{}

	bool IsValid() const { return Data != nullptr && CheckRefCount(Data); }

	T* Get() const { return Data; };

	T& GetReference() { PH_ASSERT(IsValid()); return *Data; };

	void Reset(PH_SharedMemoryHandle<T>& SharedHandle)
	{
		Data = SharedHandle.Get();
	}

	explicit operator bool() const {
		return IsValid();
	}

	bool operator==(T* DataPtr)
	{
		return Data == DataPtr;
	}

	bool operator==(PH_WeakMemoryHandle<T>& OtherHandle)
	{
		return Data == OtherHandle.Data;
	}

	bool operator==(PH_SharedMemoryHandle<T>& OtherHandle)
	{
		return Data == OtherHandle.Get();
	}

	bool operator!=(T* DataPtr)
	{
		return !(Data == DataPtr);
	}

	bool operator!=(PH_WeakMemoryHandle<T>& OtherHandle)
	{
		return !(Data == OtherHandle.Data);
	}

	bool operator!=(PH_SharedMemoryHandle<T>& OtherHandle)
	{
		return !(Data == OtherHandle.Data);
	}

	T& operator*() const {
		PH_ASSERT(IsValid()); return *Data;
	}

	T* operator->() const {
		PH_ASSERT(IsValid()); return Data;
	}

private:

	bool CheckRefCount(T* Ptr) const
	{
		if (PH_SharedMemoryHandle<T>::RefMap &&
			PH_SharedMemoryHandle<T>::RefMap->find(Ptr) != PH_SharedMemoryHandle<T>::RefMap->end()
			&& PH_SharedMemoryHandle<T>::RefMap->find(Ptr)->second > 0)
		{
			return true;
		}
		return false;
	}

	T* Data = nullptr;
	using ValuePtr = T*;
};