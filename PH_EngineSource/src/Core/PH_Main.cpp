#include "PH_Main.h"
#include "PH_CORE.h"

#include "Core/PH_REngineContext.h"
#include "Memory/PH_Memory.h"

namespace PhosphorEngine
{
	PH_UniqueMemoryHandle<Application> PH_Main::App = nullptr;
	PH_UniqueMemoryHandle<PH_Main> PH_Main::MainInst = nullptr;

	void PH_Main::run() {
		if (!MainInst.IsValid())
			init();
		
		App->run();
		std::cout << "\nEngine runtime is: " << PH_Statistics::GetElapsedEngineTime().AsSeconds() << " seconds" << "\n";

		Statistics::PH_Statistics::collapse();
		App.Release();

		PH_REngineContext::Destroy();
		MainInst.Release();
		PH_MemorySystem::destroy();
	}

	void PH_Main::init() {
		PH_MemorySystem::init();
		if (!(App.IsValid() || MainInst.IsValid()));
		{
			MainInst = ph_new<PH_Main>(PH_MEM_TAG_UNKNOWN);
			App = ph_new<Application>(PH_MEM_TAG_APPLICATION);
		}
		Statistics::PH_Statistics::init();
	}
}