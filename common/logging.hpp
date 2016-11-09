#ifndef AIFIL_UTILS_LOGGING_H
#define AIFIL_UTILS_LOGGING_H

#include <string>

namespace aifil {

enum LOG_LEVEL {
	LOG_DEBUG = 0x1,
	LOG_RAW = 0x2,
	LOG_STATE = 0x4,
	LOG_OK = 0x8,
	LOG_IMPORTANT = 0x10,
	LOG_WARNING = 0x20,
	LOG_ERROR = 0x40,

	LOG_ALL = LOG_DEBUG | LOG_RAW | LOG_STATE | LOG_OK |
			LOG_IMPORTANT | LOG_WARNING | LOG_ERROR,
	LOG_NO_SPAM = LOG_STATE | LOG_IMPORTANT | LOG_WARNING | LOG_ERROR,
	LOG_NO_SPAM_AT_ALL = LOG_IMPORTANT | LOG_WARNING | LOG_ERROR,
	LOG_DISABLED = 0
};

void log_debug(const char *fmt, ...);
void log_raw(const char *fmt, ...);
void log_state(const char *fmt, ...);
void log_important(const char *fmt, ...);
void log_warning(const char *fmt, ...);
void log_error(const char *fmt, ...);
void log_ok(const char* fmt, ...);

void log_debug(const std::string & msg);
void log_raw(const std::string & msg);
void log_state(const std::string & msg);
void log_important(const std::string & msg);
void log_warning(const std::string & msg);
void log_error(const std::string & msg);
void log_ok(const std::string & msg);


//don't write to console
void log_quiet(const char* fmt, ...);

/**
 * @brief Logger initialization and paths setup
 * @param stream_for_printf [in] stream for redirecting all logger messages.
 * Usually stderr or stdout should be used here,
 * but any alive file descriptor can be used as well.
 * BE CAREFUL, your app can become silent if you pass null pointer here!
 * @param log_level [in] 0 - default, 1 - enable debug logs
 * @param directory_to_write_logs Folder for writing logs.
 * @param directory_to_save_crashlogs Folder for writing crash dumps.
 * @param logfile_name log file name
 */
void logger_startup(
		FILE *stream_for_printf = stderr,
		int log_level = LOG_NO_SPAM,
		const std::string& directory_to_write_logs = std::string(),
		const std::string& directory_to_save_crashlogs = std::string(),
		const std::string& logfile_name = std::string());
void logger_set_mode(int log_level);
void logger_shutdown();

} //namespace aifil

#endif // AIFIL_UTILS_LOGGING_H

