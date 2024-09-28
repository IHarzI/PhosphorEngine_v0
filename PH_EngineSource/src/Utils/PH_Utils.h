#pragma once

#include <functional>
#include <chrono>

#include "Core/PH_CORE.h"
#include "Core/PH_Assert.h"
#include <stdint.h>

#define _PH_T_UNIT_DEFAULT_TYPE float64
#define _PH_T_FORMAT_DEFAULT_FORMAT timeFormat::scnds

namespace PhosphorEngine {
	namespace Utils
	{
		namespace Hash
		{
			// from: https://stackoverflow.com/a/57595105
			template <typename T, typename... Rest>
			void hashCombine(std::size_t& seed, const T& v, const Rest&... rest)
			{
				seed ^= std::hash<T>()(v)+0x9e3779b9 + (seed << 6) + (seed >> 2);
				(hashCombine(seed, rest), ...);
			}
		}

		namespace Time
		{
			enum timeFormat
			{
				nanoscnds = 1000000000,
				microscnds = 1000000,
				miliscnds = 1000,
				scnds = 1,
				mints = -60,
				hrs = -3600
			};

			template <typename t_unit = _PH_T_UNIT_DEFAULT_TYPE, typename timeFormat t_format = _PH_T_FORMAT_DEFAULT_FORMAT>
			class TimeUnit
			{
			private:
				timeFormat format{ t_format };
				t_unit time = 0;
			public:
				TimeUnit(int64 time) : time(time) {};
				TimeUnit(t_unit time) : time(time) {};

				PH_INLINE operator t_unit() const { return time; }

				PH_INLINE t_unit GetRawTime() const { return time; }

				PH_INLINE t_unit AsMinutes() const
				{
					return ConvertTime(time, format, timeFormat::mints);
				};

				PH_INLINE t_unit AsSeconds() const
				{
					return ConvertTime(time, format, timeFormat::scnds);
				};

				PH_INLINE t_unit AsMiliseconds() const
				{
					return ConvertTime(time, format, timeFormat::miliscnds);
				}

				// Get time in converted format without changing time unit.
				// @return Time value in desired format
				PH_INLINE t_unit GetConvertedTime(timeFormat cnvrt_format) const
				{
					return ConvertTime(time, format, cnvrt_format);
				}
				
				// Convert this time unit to another format.
				// @return Time value in desired format
				PH_INLINE t_unit Convert(timeFormat cnvrt_format)
				{
					if (cnvrt_format != format)
					{
						time = ConvertTime(time, format, cnvrt_format);
						format = cnvrt_format;
					};
					return time;
				}
			};

			template <typename t_unit = _PH_T_UNIT_DEFAULT_TYPE, typename timeFormat t_format = _PH_T_FORMAT_DEFAULT_FORMAT>
			PH_INLINE TimeUnit<t_unit, t_format> CalculateTimeDuration(std::chrono::high_resolution_clock::time_point startTime,
				std::chrono::high_resolution_clock::time_point endTime)
			{
				TimeUnit<t_unit, timeFormat::nanoscnds> cnvrt_time((endTime - startTime).count());
				return cnvrt_time.Convert(t_format);
			}

			template <typename t_unit = _PH_T_UNIT_DEFAULT_TYPE>
			PH_INLINE t_unit ConvertTime(t_unit timeToConvert,
				timeFormat format_before, timeFormat format_to_convert)
			{
				if (format_before == format_to_convert)
					return timeToConvert;

				if (format_before < timeFormat::scnds)
					timeToConvert *= (t_unit)format_before * (t_unit)(-1);
				else if (format_before > timeFormat::scnds)
						timeToConvert /= (t_unit)format_before;

				if (format_to_convert >= timeFormat::scnds)
					timeToConvert *= (t_unit)format_to_convert;
				else
					timeToConvert /= ((t_unit)format_to_convert * (t_unit)(-1));

				return timeToConvert;
			}

			template <typename t_unit = _PH_T_UNIT_DEFAULT_TYPE, typename timeFormat t_format = _PH_T_FORMAT_DEFAULT_FORMAT>
			class HResStopWatch
			{
			private:
				std::chrono::high_resolution_clock::time_point StartTime;
				std::chrono::high_resolution_clock::time_point EndTime;
			public:

				// Start count.
				// @return Time since epoch
				template <typename custom_t_unit = t_unit, typename timeFormat custom_t_format = t_format>
				PH_INLINE TimeUnit<custom_t_unit, custom_t_format> Count()
				{
					StartTime = std::chrono::high_resolution_clock::now();
					return ConvertTime<custom_t_unit>(StartTime.time_since_epoch().count(),
						timeFormat::nanoscnds, custom_t_format);
				}

				// Stop time count.
				// Call after Count.
				// @return Time sample
				template <typename custom_t_unit = t_unit, typename timeFormat custom_t_format = t_format>
				PH_INLINE TimeUnit<custom_t_unit, custom_t_format> Stop()
				{
					EndTime = std::chrono::high_resolution_clock::now();

					return TimeUnit<custom_t_unit, custom_t_format>(CalculateTimeDuration<custom_t_unit, custom_t_format>(StartTime, EndTime));
				}

				// Get elapsed time.
				// UNDEFINED if count isn't called at least once!
				// @return Elapsed time
				template <typename custom_t_unit = t_unit, typename timeFormat custom_t_format = t_format>
				PH_INLINE TimeUnit<custom_t_unit, custom_t_format> ElapsedTime() const
				{
					return CalculateTimeDuration<custom_t_unit, custom_t_format>(StartTime,
						std::chrono::high_resolution_clock::now());
				}

				// Get last time duration count.
				// UNDEFINED if no time sample was made before on this stopwatch!
				// @return Last time sample
				template <typename custom_t_unit = t_unit, typename timeFormat custom_t_format = t_format>
				PH_INLINE TimeUnit<custom_t_unit, custom_t_format> LastDurationSample() const
				{
					TimeUnit<custom_t_unit, custom_t_format> DurationFromLastSample = CalculateTimeDuration<custom_t_unit, custom_t_format>(StartTime,
						EndTime);
					PH_ASSERT(DurationFromLastSample >= 0);
					return DurationFromLastSample;
				}
			};

			template <typename t_unit = _PH_T_UNIT_DEFAULT_TYPE, typename timeFormat t_format = _PH_T_FORMAT_DEFAULT_FORMAT>
			class HResTimer
			{
			private:			
				std::chrono::steady_clock::time_point StartTime;
			public:
				HResTimer() {};

				// Start count.
				// @return Time since epoch
				template <typename custom_t_unit = t_unit, typename timeFormat custom_t_format = t_format>
				PH_INLINE TimeUnit<custom_t_unit, custom_t_format> Count()
				{
					StartTime = std::chrono::high_resolution_clock::now();
					return ConvertTime<custom_t_unit>(StartTime.time_since_epoch().count(),
						timeFormat::nanoscnds, custom_t_format);
				}

				// Get elapsed time.
				// UNDEFINED if count isn't called at least once!
				// @return Elapsed time
				template <typename custom_t_unit = t_unit, typename timeFormat custom_t_format = t_format>
				PH_INLINE TimeUnit<custom_t_unit, custom_t_format> ElapsedTime() const
				{
					return ConvertTime<custom_t_unit>(
						(std::chrono::duration<custom_t_unit, std::nano>(std::chrono::high_resolution_clock::now() - StartTime)).count(),
						timeFormat::nanoscnds, custom_t_format);
				}

				// Get current time since epoch.
				// @return Time since epoch
				template <typename custom_t_unit = t_unit, typename timeFormat custom_t_format = t_format>
				PH_INLINE TimeUnit<custom_t_unit, custom_t_format> CurrentTime() const
				{
					return ConvertTime<custom_t_unit>(std::chrono::high_resolution_clock::now().time_since_epoch().count(),
						timeFormat::nanoscnds, custom_t_format);
				};
			};
		}
	}
}