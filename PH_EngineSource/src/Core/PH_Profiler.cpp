#include "PH_Profiler.h"

#include "imgui.h"

namespace PhosphorEngine
{
	
	void PH_Profiler::DrawImguiStats()
	{
		auto& Instance = GetProfiler();

		ImGui::Text("Profiler stats: ");

		auto& ProfileScopes = Instance.GetProfileScopes();

		PH_String ScopesStatsText{};
		for (auto& ProfileScopeInfo : ProfileScopes)
		{
			ScopesStatsText += ProfileScopeInfo.ScopeName;
			ScopesStatsText += " : ";
			ScopesStatsText += StringUtilities::FloatingPointNumberToString(ProfileScopeInfo.AverageTime);
			ScopesStatsText += " ms\n";
		}
		ImGui::Text(ScopesStatsText.c_str());
	}

}

