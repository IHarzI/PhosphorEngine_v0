#pragma once

#include "Core/PH_CORE.h"
#include "Memory/PH_Memory.h"
#include "Containers/PH_String.h"
#include "Containers/PH_RingBuffer.h"
#include "Core/PH_Statistics.h"
#include "Utils/PH_Utils.h"
#include "Core/PH_Assert.h"

#define PH_PROFILE_SCOPE(Name) ProfileScopeTimer PHOPSHOR_PROFILER_ProfileScopeTimer_##Name{#Name};

namespace PhosphorEngine {
	using namespace Utils;
	class PH_Profiler
	{
		friend Statistics::PH_Statistics;
	public:
		PH_STATIC_LOCAL constexpr uint32 MaxProfileScopeName = 75;

		PH_STATIC_LOCAL PH_Profiler& GetProfiler() 
		{
			return StaticInstance.GetReference(); 
		}

		PH_STATIC_LOCAL void CalculateAverageStats(uint64 FPS)
		{
			for (auto& Info : GetProfileScopes())
			{
				Info.CalculateAverage(FPS);
			}
		}

		PH_STATIC_LOCAL void RegisterProfileScope(const char* Name, Time::TimeUnit<float64, Time::miliscnds> ScopeTimeMeasure)
		{
			if (auto* ProfileScopeInfo = FindProfileScope(Name))
			{
				ProfileScopeInfo->UpdateMeasure(ScopeTimeMeasure);
			}
			else
			{
				// Should be only push front, as we iterate by basic start + end, so no tail on the "others side" of the ring
				GetProfileScopes().PushFront({ Name, ScopeTimeMeasure });
			}
		}

		PH_STATIC_LOCAL void DrawImguiStats();
	private:

		PH_STATIC_LOCAL void Init() {
			PH_ASSERT(!StaticInstance); 
			StaticInstance = PH_UniqueMemoryHandle<PH_Profiler>::Create();
		}

		PH_STATIC_LOCAL void Destroy()
		{
			PH_ASSERT(StaticInstance.IsValid());
			StaticInstance.Release();
		}

		const uint64 MaxProfileScopes = 1024;

		struct ProfileScopeInfo
		{
			ProfileScopeInfo(const char* Name, Time::TimeUnit<float64, Time::miliscnds> ScopeTimeMeasure)
			{
				AverageTime = 0.f;
				LastTimeFrame = ScopeTimeMeasure.AsMiliseconds();
				AccumulatedTime = ScopeTimeMeasure.AsMiliseconds();
				PH_ASSERT(strlen(Name) < MaxProfileScopeName);
				strcpy(ScopeName, Name);
			}

			void UpdateMeasure(Time::TimeUnit<float64, Time::miliscnds> ScopeTimeMeasure)
			{
				LastTimeFrame = ScopeTimeMeasure.AsMiliseconds();
				AccumulatedTime += ScopeTimeMeasure.AsMiliseconds();
			}

			void CalculateAverage(uint64 FPS)
			{
				AverageTime = AccumulatedTime / (float64)FPS;
				AccumulatedTime = 0.f;
			}

			// All Time measures are in miliseconds
			// average from last frame
			float64 AverageTime;
			// last register call info
			float64 LastTimeFrame;
			// Accumulate in this frame
			float64 AccumulatedTime;
			char ScopeName[MaxProfileScopeName];
		};

		PH_RingBuffer<ProfileScopeInfo> ProfileScopes{ MaxProfileScopes };
		PH_STATIC_LOCAL PH_RingBuffer<ProfileScopeInfo>& GetProfileScopes() { PH_ASSERT(StaticInstance); return StaticInstance->ProfileScopes; };

		PH_STATIC_LOCAL ProfileScopeInfo* FindProfileScope(const char* Name)
		{ 
			ProfileScopeInfo* ResultScopeInfo = nullptr;
			for (auto& Info : GetProfileScopes())
			{
				if (strcmp(Info.ScopeName, Name) == 0)
				{
					ResultScopeInfo = &Info;
					break;
				}
			}
			return ResultScopeInfo;
		}

		PH_STATIC_LOCAL PH_INLINE PH_UniqueMemoryHandle<PH_Profiler> StaticInstance;
	};

	class ProfileScopeTimer
	{
	public:
		ProfileScopeTimer(const char* Name)
		{
			Timer.Count();
			PH_ASSERT(strlen(Name) < PH_Profiler::MaxProfileScopeName);
			strcpy(ScopeName, Name);
		};

		~ProfileScopeTimer()
		{
			Timer.Stop();
			PH_Profiler::RegisterProfileScope(ScopeName, Timer.ElapsedTime().AsMiliseconds());
		}
		Time::HResStopWatch<> Timer{};
		char ScopeName[PH_Profiler::MaxProfileScopeName];
	};
};