#include "errutils.h"

#include "logging.h"

#include <csignal>

namespace aifil {

void assertion_failed(const char* cond, const char* file, unsigned lineno)
{
	log_error("ASSERTION FAILED at %s(%i): %s", file, lineno, cond);
#ifdef _WIN32
	exit(3);
#else
	raise(SIGABRT);
#endif
}

} //namespace aifil
