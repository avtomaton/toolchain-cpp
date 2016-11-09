#include "logging.hpp"

#include "console.hpp"
#include "errutils.hpp"

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
static FILE *printf_stream = stderr;
static int required_log_level = LOG_NO_SPAM;

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
		if (!file_all && printf_stream)
			fprintf(printf_stream, "\ncannot open '%s'\n", fn.c_str());
		else
		{
#ifdef WIN32
			int fdesc = _fileno(file_all);
			HANDLE fhan = (HANDLE) _get_osfhandle(fdesc);
			if (!SetHandleInformation(fhan, HANDLE_FLAG_INHERIT, 0) && printf_stream)
				fprintf(printf_stream, "Failed to set handle information\n");
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
//			if (printf_stream)
//				fprintf(printf_stream, "Failed to rotate log: %s\n", e.what());
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

void log_debug(const char* fmt, ...)
{
	if (!(required_log_level & LOG_DEBUG))
		return;
	static char prefix[] = "\n";
	char buf[65536];
	strcpy(buf, prefix);
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf + sizeof(prefix) - 1, sizeof(buf) - sizeof(prefix), fmt, ap);
	va_end(ap);
	buf[65535] = 0;
	pretty_printf(LOG_COLOR_GRAY, printf_stream, buf);
	if (logger)
		logger->message(buf);
}

void log_raw(const char *fmt, ...)
{
	if (!(required_log_level & LOG_RAW))
		return;
	char buf[65536];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	buf[65535] = 0;
	pretty_printf(LOG_COLOR_GRAY, printf_stream, buf);
	if (logger)
		logger->message(buf);
}

void log_state(const char* fmt, ...)
{
	if (!(required_log_level & LOG_STATE))
		return;
	static char prefix[] = "\n";
	char buf[65536];
	strcpy(buf, prefix);
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf + sizeof(prefix) - 1, sizeof(buf) - sizeof(prefix), fmt, ap);
	va_end(ap);
	buf[65535] = 0;
	pretty_printf(LOG_COLOR_NORMAL, printf_stream, buf);
	if (logger)
		logger->message(buf);
}

void log_important(const char* fmt, ...)
{
	if (!(required_log_level & LOG_IMPORTANT))
		return;
	static char prefix[] = "\n";
	char buf[65536];
	strcpy(buf, prefix);
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf + sizeof(prefix) - 1, sizeof(buf) - sizeof(prefix), fmt, ap);
	va_end(ap);
	buf[65535] = 0;
	pretty_printf(LOG_COLOR_HL, printf_stream, buf);
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
	if (!(required_log_level & LOG_WARNING))
		return;

	static char prefix[] = "\nWARNING: ";
	char buf[65536];
	strcpy(buf, prefix);

	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf + sizeof(prefix) - 1, sizeof(buf) - sizeof(prefix), fmt, ap);
	va_end(ap);
	buf[65535] = 0;
	pretty_printf(LOG_COLOR_YELLOW, printf_stream, buf);
	if (logger)
		logger->message(buf);
}

void log_error(const char* fmt, ...)
{
	if (!(required_log_level & LOG_ERROR))
		return;

	static char prefix[] = "\nERROR: ";
	char buf[65536];
	strcpy(buf, prefix);

	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf + sizeof(prefix) - 1, sizeof(buf) - sizeof(prefix), fmt, ap);
	va_end(ap);
	buf[65535] = 0;
	pretty_printf(LOG_COLOR_RED, printf_stream, buf);
	if (printf_stream)
		fflush(printf_stream);
	if (logger)
		logger->message(buf);
}

void log_ok(const char* fmt, ...)
{
	if (!(required_log_level & LOG_OK))
		return;

	static char prefix[] = "\n";
	char buf[65536];
	strcpy(buf, prefix);

	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf + sizeof(prefix) - 1, sizeof(buf) - sizeof(prefix), fmt, ap);
	va_end(ap);
	buf[65535] = 0;
	pretty_printf(LOG_COLOR_GREEN, printf_stream, buf);
	if (logger)
		logger->message(buf);
}

void log_debug(const std::string & msg)
{
	log_debug(msg.c_str());
}

void log_raw(const std::string & msg)
{
	log_raw(msg.c_str());
}

void log_state(const std::string & msg)
{
	log_state(msg.c_str());
}

void log_important(const std::string &msg)
{
	log_important(msg.c_str());
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

// TODO Сделать разбор путей с учетом наличия/отсутствия слэшей.

void logger_startup(
		FILE *stream_for_printf,
		int log_level,
		const std::string & directory_to_write_logs,
		const std::string & directory_to_save_crashlogs,
		const std::string & logfile_name)
{
	printf_stream = stream_for_printf;
	required_log_level = log_level;
	if (!printf_stream)
	{
		printf("SILENT MODE! It is the last message, now your app "
			"will not redirect any console output anywhere!\n");
	}

	if (directory_to_write_logs.empty())
		return;

	if (logger)
	{
		printf("\nLogger is already set\n");
		return;
	}

	af_assert(!directory_to_write_logs.empty() && !directory_to_save_crashlogs.empty());

	try
	{
		boost::filesystem::path t(logfile_name);
		const std::string log_stem = t.stem().string();
		const std::string log_extension = t.extension().string();

//		const std::string log = directory_to_write_logs + "/" + logfile_name;
//		const std::string log_bkp = directory_to_save_crashlogs + "/" + log_stem
//			+ ".bkp" + log_extension;

		boost::filesystem::remove(directory_to_write_logs + "/" + log_stem + ".older" + log_extension);
		logger.reset(new SimpleLogger(directory_to_write_logs, logfile_name));

	}

	catch (const std::exception& e)
	{
		printf("\nLOGGER ERROR: %s\n", e.what());
	}
}


void logger_shutdown()
{
	// Removed
	// std::unique_lock<std::mutex> lock(logging_mutex);

	if (printf_stream)
		fprintf(printf_stream, "\n");
	logger.reset();
}

void logger_set_mode(int log_level)
{
	required_log_level = log_level;
}

#else

void logger_startup(const std::string& directory_to_write_logs,
	const std::string& directory_to_save_crashlogs,
	const std::string& logfile_name)
{
}

#endif

} //namespace aifil
