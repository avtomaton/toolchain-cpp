#include "console.h"

#include "errutils.h"

#include <stdint.h>
#include <cstdio>
#include <cstring>
#include <cstdarg>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

#define LOG_ROTATE_SIZE 10 * 1024 * 1024 // 10M

namespace aifil {

CONSOLE_STATE console_state = CON_STATE_BEGIN;

void get_char()
{
#ifdef _WIN32
	_getch();
#else
	struct termios oldt, newt;
	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);
	int ch = getchar();
	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
#endif
}

void print_progress(const char* fmt, ...)
{
	char buf[65536];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	buf[65535] = 0;
	if (console_state == CON_STATE_END)
	{
		fprintf(stdout, "\n");
		console_state = CON_STATE_BEGIN;
	}
	fprintf(stdout, "%s\r", buf);
	fflush(stdout);
}

void print_progress(int part, int total, int skip)
{
	if (console_state == CON_STATE_END)
	{
		fprintf(stdout, "\n");
		console_state = CON_STATE_BEGIN;
	}

	part += 1;
	if (part % skip != 0)
		return;

	fprintf(stdout, "%.2lf%% (%d/%d)\r",
			total ? part * 100.0 / total : 0, part, total);
	fflush(stdout);
}

void pretty_printf(int level, FILE* fd, const char* buf)
{
	//std::unique_lock lock(logging_mutex);
#if defined(_WIN32) && !defined(CYGWIN)
	HANDLE con = 0;
	if (fd == stderr)
		con = GetStdHandle(STD_ERROR_HANDLE);
	else
		con = GetStdHandle(STD_OUTPUT_HANDLE);
	if (con)
	{
		switch (level) {
		case LOG_COLOR_RED:
			SetConsoleTextAttribute(con, FOREGROUND_RED | FOREGROUND_INTENSITY);
			break;
		case LOG_COLOR_GREEN:
			SetConsoleTextAttribute(con, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
			break;
		case LOG_COLOR_BLUE:
			SetConsoleTextAttribute(con, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
			break;
		case LOG_COLOR_CYAN:
			SetConsoleTextAttribute(con, FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
			break;
		case LOG_COLOR_MAGENTA:
			SetConsoleTextAttribute(con, FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
			break;
		case LOG_COLOR_YELLOW:
			SetConsoleTextAttribute(con, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
			break;
		case LOG_COLOR_GRAY:
			SetConsoleTextAttribute(con, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
			break;
		case LOG_COLOR_HL:
		default:
			SetConsoleTextAttribute(con,
				FOREGROUND_RED | FOREGROUND_GREEN |
				FOREGROUND_BLUE | FOREGROUND_INTENSITY);
		}
	}
#else
	if (isatty(fileno(fd)))
	{
		switch (level) {
		case LOG_COLOR_RED:
			fprintf(fd, "\033[31m");
			break;
		case LOG_COLOR_GREEN:
			fprintf(fd, "\033[32m");
			break;
		case LOG_COLOR_BLUE:
			fprintf(fd, "\033[34m");
			break;
		case LOG_COLOR_CYAN:
			fprintf(fd, "\033[36m");
			break;
		case LOG_COLOR_MAGENTA:
			fprintf(fd, "\033[35m");
			break;
		case LOG_COLOR_YELLOW:
			fprintf(fd, "\033[33m");
			break;
		case LOG_COLOR_GRAY:
			break;
		case LOG_COLOR_HL:
		default:
			fprintf(fd, "\033[36m");
			break;
		}
	}
#endif

	fputs(buf, fd);
	if (strlen(buf) > 0 && buf[strlen(buf) - 1] != '\n')
		console_state = CON_STATE_END;

#ifdef WIN32
	if (con)
		SetConsoleTextAttribute(con, 263);
#else
	if (isatty(fileno(fd)))
		fprintf(fd, "\033[0m");
#endif
}

void ArgParser::parse_args(int argc, char *argv[])
{
	int opt_count = 1;
	while (opt_count < argc)
	{
		// skip positioning parameters
		if (!has_hyphens(argv[opt_count]))
		{
			++opt_count;
			continue;
		}

		std::string param_name = trim_hyphens(argv[opt_count]);
		if (param_name.empty())
		{
			log_error("Incorrect parameter '%s'", argv[opt_count]);
			++opt_count;
			continue;
		}

		++opt_count;
		std::string param_value;
		if (opt_count < argc)
			param_value = argv[opt_count];

		params[param_name] = param_value;
	}
}

bool ArgParser::has_param(const std::string &param)
{
	return params.find(param) != params.end();
}

bool ArgParser::has_hyphens(const char *param)
{
	return strlen(param) > 0 && *param == '-';
}

std::string ArgParser::trim_hyphens(const char *param)
{
	std::string trimmed;
	if (strlen(param) > 0 && *param == '-')
		++param;
	if (strlen(param) > 0 && *param == '-')
		++param;
	trimmed = param;
	return trimmed;
}

} //namespace aifil
