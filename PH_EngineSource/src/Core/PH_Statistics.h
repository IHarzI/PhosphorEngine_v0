#pragma once

#include "PH_CORE.h"
#include "Utils/PH_Utils.h"

#include "Memory/PH_Memory.h"

namespace PhosphorEngine {
	class PH_Main;
	namespace Statistics {
		using namespace PhosphorEngine::Utils::Time;
		
		class PH_Statistics
		{
		private:
			friend class PhosphorEngine::PH_Main;

			PH_STATIC_LOCAL HResTimer<> Timer;
			PH_STATIC_LOCAL PH_UniqueMemoryHandle<PH_Statistics> StatInst;

			PH_STATIC_LOCAL void init();
			PH_STATIC_LOCAL void collapse();
		public:
			template <typename t_unit = long double, typename Utils::Time::timeFormat t_format = Utils::Time::timeFormat::scnds>
			PH_STATIC_LOCAL Utils::Time::TimeUnit<t_unit, t_format> GetElapsedEngineTime();
		};
	}
}

using namespace PhosphorEngine;
using namespace PhosphorEngine::Statistics;

template <typename t_unit, typename Utils::Time::timeFormat t_format>
Utils::Time::TimeUnit<t_unit, t_format> PH_Statistics::GetElapsedEngineTime()
{
	return Timer.ElapsedTime<t_unit, t_format>();
}
