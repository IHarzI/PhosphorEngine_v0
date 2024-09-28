#pragma once

#include "Core/PH_CORE.h"
#include "Containers/PH_String.h"
#include "PH_Memory.h"

namespace PhosphorEngine
{
	namespace Memory
	{
		struct PH_GlobalMemoryStats
		{
		private:
			PH_STATIC_LOCAL PH_GlobalMemoryStats* stat_inst;
		public:
			uint64 allocatedMemorySize;
			uint64 allocatedMemorySizePerMemTag[255];
			uint64 allocationsCount;
			uint64 deallocationsCount;

			PH_STATIC_LOCAL const PH_GlobalMemoryStats& GetInstance() {
				PH_ASSERT(stat_inst);
				return *stat_inst;
			}

			PH_STATIC_LOCAL void RegisterAllocation(uint64 size, uint8 Tag)
			{
				stat_inst->allocatedMemorySize += size;
				stat_inst->allocatedMemorySizePerMemTag[Tag] += size;
				stat_inst->allocationsCount++;
			}

			PH_STATIC_LOCAL void RegisterDeallocation(uint64 size, uint8 Tag)
			{
				stat_inst->allocatedMemorySize -= size;
				stat_inst->allocatedMemorySizePerMemTag[Tag] -= size;
				stat_inst->deallocationsCount++;
			}

			PH_STATIC_LOCAL void DrawImguiStats();

			friend class PH_MemorySystem;
		};
	};
};
