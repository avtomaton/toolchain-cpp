#ifndef AIFIL_UTILS_CONSOLE_H
#define AIFIL_UTILS_CONSOLE_H

#include <map>
#include <string>

namespace aifil {

enum CONSOLE_STATE {
	CON_STATE_BEGIN,
	CON_STATE_END
};

enum LOG_COLOR {
	LOG_COLOR_GRAY,
	LOG_COLOR_NORMAL,
	LOG_COLOR_HL,
	LOG_COLOR_RED,
	LOG_COLOR_GREEN,
	LOG_COLOR_BLUE,
	LOG_COLOR_CYAN,
	LOG_COLOR_MAGENTA,
	LOG_COLOR_YELLOW
};

int get_char();
void print_progress(const char* fmt, ...);
void print_progress(int part, int total, int skip = 20);
void pretty_printf(int level, FILE* fd, const char* buf);

/**
 * @brief The simplest argument parser.
 *
 * Works only with "-param value" or "--param value" forms
 */
struct ArgParser
{
	void parse_args(int argc, char *argv[]);
	bool has_param(const std::string &param) const;

	bool has_leading_hyphen(const std::string &param);
	std::string trim_hyphens(const char *param);
	std::map<std::string, std::string> params;
//	std::vector<std::string> positional;
};

} //namespace aifil

#endif // AIFIL_UTILS_CONSOLE_H

