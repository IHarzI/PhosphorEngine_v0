#pragma once

#include "Core/PH_CORE.h"
#include "Core/PH_Statistics.h"
#include "Application.h"

namespace PhosphorEngine {

	class PH_Main
	{
	private:
		static PH_UniqueMemoryHandle<Application> App;
		static PH_UniqueMemoryHandle<PH_Main> MainInst;

		static void init();
	public:
		static void run();
		const char* Args = nullptr;
	};

}
