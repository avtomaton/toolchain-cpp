#ifndef AIFIL_UTILS_ERRUTILS_H
#define AIFIL_UTILS_ERRUTILS_H

#include "logging.h"

#include <string>
#include <stdexcept>

#define af_assert(exp) (void)( (exp) || (aifil::assertion_failed(#exp, __FILE__, __LINE__), 0) )

namespace aifil {

void assertion_failed(const char* cond, const char* file, unsigned lineno);

inline void except(bool condition, const std::string &error_message = "")
{
	if (!condition)
		throw std::runtime_error(error_message);
}

// check condition
inline void check(
		bool condition, const std::string &error_message = "", bool exception = false)
{
	if (condition)
		return;
	if (exception)
		throw std::runtime_error(error_message);
	else
		log_error(error_message.c_str());
}

} //namespace aifil

#endif //AIFIL_UTILS_ERRUTILS_H
