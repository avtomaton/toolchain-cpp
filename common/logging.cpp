#include "logging.h"

#include "console.h"
#include "errutils.h"

#ifdef HAVE_BOOST
#include <boost/filesystem.hpp>
#endif

#include <mutex>

#include <stdint.h>
#include <cstdio>
#include <cstdarg>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#endif

#define LOG_ROTATE_SIZE 10 * 1024 * 1024 // 10M

namespace aifil {

extern CONSOLE_STATE console_state;

#ifdef HAVE_BOOST
class SimpleLogger
{
public:
	std::string version;
	FILE* file_all;
	FILE* file_quickstate;

	std::string log_name;
	std::string dir_log;

	SimpleLogger(const std::string& directory_to_keep_logs, const std::string& logfile_name);
	virtual ~SimpleLogger();

	virtual void message(const char* msg);
};

static std::shared_ptr<SimpleLogger> logger;
static std::mutex logging_mutex;

SimpleLogger::SimpleLogger(const std::string& directory_to_keep_logs,
		const std::string& logfile_name)
 : file_all(0), file_quickstate(0),
   log_name(logfile_name), dir_log(directory_to_keep_logs)
{
	if (log_name.empty())
		return;
}

SimpleLogger::~SimpleLogger()
{
	if (log_name.empty())
		return;

	std::unique_lock<std::mutex> lock(logging_mutex);

	if (file_all)
	{
		fflush(file_all);
		fclose(file_all);
	}
	if (file_quickstate)
	{
		fflush(file_quickstate);
		fclose(file_quickstate);
	}
	file_all = 0;
	file_quickstate = 0;
}

void SimpleLogger::message(const char* msg)
{
	if (log_name.empty())
		return;
	using namespace boost::filesystem;
	std::unique_lock<std::mutex> lock(logging_mutex);
	if (!file_all)
	{
		std::string fn = (dir_log + "/" + log_name);
		file_all = fopen(fn.c_str(), "wb");
		if (!file_all)
			fprintf(stderr, "cannot open '%s'\n", fn.c_str());
		else
		{
#ifdef WIN32
			int fdesc = _fileno(file_all);
			HANDLE fhan = (HANDLE) _get_osfhandle(fdesc);
			if (!SetHandleInformation(fhan, HANDLE_FLAG_INHERIT, 0))
				fprintf(stderr, "Failed to set handle information\n");
#endif
			setbuf(file_all, 0);
			fwrite(version.c_str(), version.size(), 1, file_all);
		}
	}
	if (!file_all)
		return;

	size_t len = strlen(msg);
	fwrite(msg, len, 1, file_all);
	fflush(file_all);

	int file_size_so_far = ftell(file_all);
	static bool rotation_broken = false;
	if (file_size_so_far > LOG_ROTATE_SIZE && !rotation_broken) {
		fclose(file_all);
		file_all = 0;

		path t(log_name);
		const std::string log_stem = t.stem().string();
		const std::string log_extension = t.extension().string();

		try {
			remove(dir_log + "/" + log_stem + ".older" + log_extension);
		} catch (const std::exception&) {
			// no such file
		}
//		try {
//			rename_or_move_file(dir_log + "/" + log_name,
//				dir_log + "/" + log_stem + ".older" + log_extension,
//				true);
//		} catch (const std::exception& e) {
//			rotation_broken = true;
//			fprintf(stderr, "Failed to rotate log: %s\n", e.what());
//		}
	}
}

#else
// dummy
struct SimpleLogger
{
	void message(const char*) { af_assert(!"need boost library for this functionality"); }
};
static SimpleLogger *logger = 0;
#endif

void log_state(const char* fmt, ...)
{
	static char prefix[] = "\n";
	char buf[65536];
	strcpy(buf, prefix);
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf + sizeof(prefix) - 1, sizeof(buf) - sizeof(prefix), fmt, ap);
	va_end(ap);
	buf[65535] = 0;
	pretty_printf(LOG_COLOR_GRAY, stderr, buf);
	if (logger)
		logger->message(buf);
}

void log_raw(const char *fmt, ...)
{
	char buf[65536];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	buf[65535] = 0;
	pretty_printf(LOG_COLOR_GRAY, stderr, buf);
	if (logger)
		logger->message(buf);
}

void log_quiet(const char* fmt, ...)
{
	if (!logger)
		return;

	static char prefix[] = "\n";
	char buf[65536];
	strcpy(buf, prefix);
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf + sizeof(prefix) - 1, sizeof(buf) - sizeof(prefix), fmt, ap);
	va_end(ap);
	buf[65535] = 0;
	logger->message(buf);
}

void log_warning(const char* fmt, ...)
{
	static char prefix[] = "\nWARNING: ";
	char buf[65536];
	strcpy(buf, prefix);

	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf + sizeof(prefix) - 1, sizeof(buf) - sizeof(prefix), fmt, ap);
	va_end(ap);
	buf[65535] = 0;
	pretty_printf(LOG_COLOR_YELLOW, stderr, buf);
	if (logger)
		logger->message(buf);
}

void log_error(const char* fmt, ...)
{
	static char prefix[] = "\nERROR: ";
	char buf[65536];
	strcpy(buf, prefix);

	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf + sizeof(prefix) - 1, sizeof(buf) - sizeof(prefix), fmt, ap);
	va_end(ap);
	buf[65535] = 0;
	pretty_printf(LOG_COLOR_RED, stderr, buf);
	if (logger)
		logger->message(buf);
}

void log_ok(const char* fmt, ...)
{
	static char prefix[] = "\n";
	char buf[65536];
	strcpy(buf, prefix);

	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf + sizeof(prefix) - 1, sizeof(buf) - sizeof(prefix), fmt, ap);
	va_end(ap);
	buf[65535] = 0;
	pretty_printf(LOG_COLOR_GREEN, stderr, buf);
	if (logger)
		logger->message(buf);
}


void log_state(const std::string & msg)
{
	log_state(msg.c_str());
}


void log_raw(const std::string & msg)
{
	log_raw(msg.c_str());
}


void log_warning(const std::string & msg)
{
	log_warning(msg.c_str());
}


void log_error(const std::string & msg)
{
	log_error(msg.c_str());
}


void log_ok(const std::string & msg)
{
	log_ok(msg.c_str());
}


#ifdef HAVE_BOOST
void logger_startup(const std::string& directory_to_write_logs,
	const std::string& directory_to_save_crashlogs,
	const std::string& logfile_name)
{
	if (logger)
	{
		printf("Logger is already set\n");
		return;
	}
	af_assert(!directory_to_write_logs.empty() && !directory_to_save_crashlogs.empty());
	try {
		boost::filesystem::path t(logfile_name);
		const std::string log_stem = t.stem().string();
		const std::string log_extension = t.extension().string();

		const std::string log = directory_to_write_logs + "/" + logfile_name;
		const std::string log_bkp = directory_to_save_crashlogs + "/" + log_stem
			+ ".bkp" + log_extension;

		boost::filesystem::remove(directory_to_write_logs + "/" + log_stem + ".older" + log_extension);
		logger.reset(new SimpleLogger(directory_to_write_logs, log));

	} catch (const std::exception& e) {
		printf("LOGGER ERROR: %s\n", e.what());
	}
}

void logger_shutdown()
{
	// Removed
	// std::unique_lock<std::mutex> lock(logging_mutex);

	logger.reset();
}
#else

void logger_startup(const std::string& directory_to_write_logs,
	const std::string& directory_to_save_crashlogs,
	const std::string& logfile_name)
{
}

#endif

} //namespace aifil
