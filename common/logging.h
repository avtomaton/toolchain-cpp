#ifndef AIFIL_UTILS_LOGGING_H
#define AIFIL_UTILS_LOGGING_H

#include <string>

#ifdef DEBUG
#define log_debug(a) log_state(a)
#else
#define log_debug(a) ;
#endif

namespace aifil {

void log_state(const char *fmt, ...);
void log_raw(const char *fmt, ...);
void log_warning(const char *fmt, ...);
void log_error(const char *fmt, ...);
void log_ok(const char* fmt, ...);

void log_state(const std::string & msg);
void log_raw(const std::string & msg);
void log_warning(const std::string & msg);
void log_error(const std::string & msg);
void log_ok(const std::string & msg);


//don't write to console
void log_quiet(const char* fmt, ...);

void logger_startup(const std::string& directory_to_write_logs,
	const std::string& directory_to_save_crashlogs, const std::string& logfile_name);
void logger_shutdown();

} //namespace aifil

#endif // AIFIL_UTILS_LOGGING_H

