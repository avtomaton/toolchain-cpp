#ifndef AIFIL_UTILS_CONSOLE_H
#define AIFIL_UTILS_CONSOLE_H

#include <string>

#ifdef DEBUG
#define log_debug(a) log_state(a)
#else
#define log_debug(a) ;
#endif

namespace aifil {

enum CONSOLE_STATE {
	CON_STATE_BEGIN,
	CON_STATE_END
};

enum LOG_COLOR {
	LOG_COLOR_GRAY,
	LOG_COLOR_HL,
	LOG_COLOR_RED,
	LOG_COLOR_GREEN,
	LOG_COLOR_BLUE,
	LOG_COLOR_CYAN,
	LOG_COLOR_MAGENTA,
	LOG_COLOR_YELLOW
};

void get_char();
void print_progress(const char* fmt, ...);
void pretty_printf(int level, FILE* fd, const char* buf);

} //namespace aifil

#endif // AIFIL_UTILS_CONSOLE_H

