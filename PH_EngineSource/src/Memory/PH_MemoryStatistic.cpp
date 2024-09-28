#include "PH_MemoryStatistic.h"
#include "PH_Memory.h"
#include "Containers/PH_String.h"
#include "imgui.h"

namespace PhosphorEngine
{
	namespace Memory
	{
		namespace util {
			struct MemoryShowDebugInfo
			{
				PH_String MemoryTier = { " BYTES" };
				float64 Size;
			};

			MemoryShowDebugInfo FormatMemorySize(uint64 Size)
			{
				MemoryShowDebugInfo Result{};
				if (Size > GIBIBYTES(1))
				{
					Result.MemoryTier = " GIB";
					Result.Size = (float64)Size / (float64)(GIBIBYTES(1));
				}
				else if (Size > MIBIBYTES(1))
				{
					Result.MemoryTier = " MiB";
					Result.Size = (float64)Size / (float64)(MIBIBYTES(1));
				}
				else if (Size > KIBIBYTES(1))
				{
					Result.MemoryTier = " KiB";
					Result.Size = (float64)Size / (float64)(KIBIBYTES(1));
				}
				else
				{
					Result.Size = Size;
				}
				return Result;
			}
		}

		void PH_GlobalMemoryStats::DrawImguiStats()
		{
			const auto& Instance = GetInstance();

			ImGui::Text("Memory Stats: Total Allocated Memory %u bytes, Total allocations: %u, Total deallocations: %u", Instance.allocatedMemorySize,
				Instance.allocationsCount, Instance.deallocationsCount);
			PH_String MemPerTagStats{};
			const uint64 MemoryLabelLenght = 45;
			for (uint32 i = 0; i < ph_mem_tag::PH_MEM_TAG_MAX; i++)
			{
				const char* MemLabel = String_From_Ph_Mem_Tag((ph_mem_tag)i);
				const uint64 LabelSize = strlen(MemLabel);
				MemPerTagStats += MemLabel;
				MemPerTagStats += ": ";
				auto MemShowStat = util::FormatMemorySize(Instance.allocatedMemorySizePerMemTag[i]);
				if (LabelSize < MemoryLabelLenght)
				{
					for (uint32 i = 0; i < MemoryLabelLenght - LabelSize; i++)
					{
						MemPerTagStats += ' ';
					}
				}
				MemPerTagStats += StringUtilities::FloatingPointNumberToString(MemShowStat.Size);
				MemPerTagStats += MemShowStat.MemoryTier;
				MemPerTagStats += "\n";
			}
			ImGui::Text(MemPerTagStats.c_str());
		}
	}
};