#include "PH_Statistics.h"
#include "PH_CORE.h"
#include "Memory/PH_Memory.h"
#include "PH_Profiler.h"

namespace PhosphorEngine {
	namespace Statistics {
		PH_UniqueMemoryHandle<PH_Statistics> PH_Statistics::StatInst = nullptr;
		HResTimer<> PH_Statistics::Timer{};

		void PH_Statistics::init() {
			PH_ASSERT_MSG(!(StatInst.IsValid()), "Double call to init Statistics, don't do this!");
			if (!StatInst.IsValid())
				StatInst = ph_new<PH_Statistics>(PH_MEM_TAG_UNKNOWN);
			Timer.Count();

			if (!PH_Profiler::StaticInstance.IsValid())
				PH_Profiler::Init();
			PH_INFO("Statistics initialized..");
		}
		void PH_Statistics::collapse()
		{
			StatInst.Release();
			PH_Profiler::Destroy();
			PH_INFO("Statistics collapesd");
		}
	};
}