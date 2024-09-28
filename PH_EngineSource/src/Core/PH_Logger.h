#pragma once

#include "PH_CORE.h"

#include <cstdarg>

#include "PH_Assert.h"

#define PH_LOG_WARN_ENABLED   00000001
#define PH_LOG_INFO_ENABLED   00000010
#define PH_LOG_DEBUG_ENABLED  00000100
#define PH_LOG_TRACE_ENABLED  00001000

#define PH_LOG_MASK (PH_LOG_WARN_ENABLED | PH_LOG_INFO_ENABLED | PH_LOG_DEBUG_ENABLED | PH_LOG_TRACE_ENABLED)

#define PH_LOG_ERROR_OR_FATAL_CODE -999

#if PH_RELEASE == 1

#define PH_LOG_MASK (PH_LOG_WARN_ENABLED & PH_LOG_INFO_ENABLED)

#endif

namespace PhosphorEngine {

	enum  _ph_log_levels {
		LOG_LEVEL_FATAL = 1 << 0,
		LOG_LEVEL_ERROR = 1 << 1,
		LOG_LEVEL_WARN = 1 << 2,
		LOG_LEVEL_INFO = 1 << 3,
		LOG_LEVEL_DEBUG = 1 << 4,
		LOG_LEVEL_TRACE = 1 << 5
	};

	int _ph_log_levels_enum_to_index(_ph_log_levels level);

	class  Logger {
	private:
		static Logger* inst;
	public:
		static Logger* instance() { return inst; }
	public:
		Logger();
		~Logger();
		static int _ph_log_output(_ph_log_levels level, const char* msg, ...);
		static bool _ph_initialize_logging();
		static void _ph_shutdown_logging();
	};

	 void _ph_report_assertion_failure(const char* expression, const char* msg, const char* file, int line);

}
// logs fatal message
#define PH_FATAL(msg,...) {PhosphorEngine::Logger::_ph_log_output(LOG_LEVEL_FATAL,msg,##__VA_ARGS__); PH_DEBUG_BREAK();};

// ERROR msg
#ifndef PH_ERROR
// logs ERROR message
#define PH_ERROR(msg,...) {PhosphorEngine::Logger::_ph_log_output(LOG_LEVEL_ERROR,msg,##__VA_ARGS__); PH_DEBUG_BREAK();};
#endif // !PH_ERROR

// WARN msg
#if (PH_LOG_MASK & PH_LOG_WARN_ENABLED)
// logs WARN message
#define PH_WARN(msg,...)  PhosphorEngine::Logger::_ph_log_output(LOG_LEVEL_WARN,msg,##__VA_ARGS__);
#else
// nothing if WARN isn't enables
#define PH_WARN(msg,...)
#endif

// INFO msg
#if (PH_LOG_MASK & PH_LOG_INFO_ENABLED)
// logs INFO message
#define PH_INFO(msg,...)  PhosphorEngine::Logger::_ph_log_output(LOG_LEVEL_INFO,msg,##__VA_ARGS__);
#else
// nothing if INFO isn't enables
#define PH_INFO(msg,...)
#endif

// DEBUG msg
#if (PH_LOG_MASK & PH_LOG_DEBUG_ENABLED)
// log DEBUG message
#define PH_DEBUG(msg,...)  PhosphorEngine::Logger::_ph_log_output(LOG_LEVEL_DEBUG,msg,##__VA_ARGS__);
#else
// nothing if DEBUG isn't enables
#define PH_DEBUG(msg,...)
#endif

// TRACE msg
#if (PH_LOG_MASK & PH_LOG_TRACE_ENABLED)
// log TRACE message
#define PH_TRACE(msg,...)  PhosphorEngine::Logger::_ph_log_output(LOG_LEVEL_TRACE,msg,##__VA_ARGS__);
#else
// nothing if TRACE isn't enables
#define PH_TRACE(msg,...)
#endif