#include "PH_Logger.h"
#include "PH_Assert.h"

namespace PhosphorEngine {

	Logger* Logger::inst;

	int32 _ph_log_levels_enum_to_index(_ph_log_levels level)
	{
		int32 index = 0;
		if((int32)level== 0)
			return 0;
		else
		{
			while (((int32)level >> index) != 1)
			{
				index++;
				if (index > sizeof(int32) * 8)
					return -1;
			}

			return index;
		}
		return -1;
	}

	void _ph_report_assertion_failure(const char* expression, const char* msg, const char* file, int32 line)
	{
		Logger::_ph_log_output(_ph_log_levels::LOG_LEVEL_FATAL, "Assertion failure: %s, message: %s, in file: %s, line: %d\n", expression, msg, file, line);
	}

	bool Logger::_ph_initialize_logging() {

		return false;
	}

	void Logger::_ph_shutdown_logging()
	{

	}

	Logger::Logger()
	{
		inst = this;
	}

	Logger::~Logger()
	{
		_ph_shutdown_logging();
	}

	int32 Logger::_ph_log_output(_ph_log_levels level, const char* msg, ...) {
		static const char* levels[6] = { "[FATAL]: ", "[ERROR]: ", "[WARN]:  ", "[INFO]:  ", "[DEBUG]: ", "[TRACE]: " };
		// check if it's FATAL or ERROR
		bool is_error = (level & _ph_log_levels::LOG_LEVEL_FATAL | level & _ph_log_levels::LOG_LEVEL_ERROR);

		const int32 msg_size = 12000;
		char raw_out_msg[msg_size];
		memset(raw_out_msg, 0, sizeof(raw_out_msg));

		char* arg_ptr;
		va_start(arg_ptr, msg);
		vsnprintf(raw_out_msg, msg_size, msg, arg_ptr);
		va_end(arg_ptr);

		char out_msg[msg_size];
		int32 LogIndex = _ph_log_levels_enum_to_index(level);
		sprintf(out_msg, "%s%s\n", levels[LogIndex], raw_out_msg);

		printf("%s", out_msg);

		if (is_error)
			return PH_LOG_ERROR_OR_FATAL_CODE;

		return 1;
	};

}