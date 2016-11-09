#ifndef AIFIL_UTILS_ERRUTILS_H
#define AIFIL_UTILS_ERRUTILS_H

#include <string>
#include <sstream>
#include <stdexcept>

#include <csignal>

#include "c_attribs.hpp"
#include "logging.hpp"
#include "errstream.hpp"

#ifdef __GNUC__
#define FUNC_SIGNATURE __PRETTY_FUNCTION__
#elif defined(_MSC_VER)
#define FUNC_SIGNATURE __FUNCSIG__
#else
#define FUNC_SIGNATURE __func__
#endif

#define af_assert(exp) (void)( (exp) || (aifil::assertion_failed(#exp, __FILE__, __LINE__), 0) )

#ifdef _MSC_VER
// use inline lambda instead of compound statement, seems working
#define af_exception(msg) \
[](){\
	std::stringstream message; \
	message << msg; \
	std::stringstream ss; \
	ss << "Exception in file: " << __FILE__ \
	<< ", function: " << FUNC_SIGNATURE \
	 << ", line: " << __LINE__; \
	throw aifil::Exception(message.str(), ss.str());\
}()
#else
// seems working in clang too, but not sure
#define af_exception(msg) \
({\
	std::stringstream message; \
	message << msg; \
	std::stringstream ss; \
	ss << "Exception in file: " << __FILE__ \
	<< ", function: " << FUNC_SIGNATURE \
	 << ", line: " << __LINE__; \
	throw aifil::Exception(message.str(), ss.str());\
})
#endif

namespace aifil
{

void assertion_failed(const char* cond, const char* file, unsigned lineno);

// enable core dumps for debug builds
void core_dump_enable();

#ifdef __GNUC__
/**
 * @brief Signal handler for printing backtrace into logs.
 * Prints backtrace and terminates application.
 * Should be set with the following:
 * @code
 * struct sigaction sa;
 * sa.sa_sigaction = aifil::signal_set_exit_with_backtrace;
 * // possibly set some flags
 * sigaction(SIGSEGV, &sa, NULL);
 * // sigaction(<some other signal>, %sa, NULL);
 * @endcode
 * @param sig [in] signal code (SIGSEGV, SIGILL etc.)
 * @param info [in] Some additional info added by sigaction.
 * @param ucontext [in] Signal context added by sigaction.
 */
void signal_set_exit_with_backtrace(int sig, siginfo_t *info, void *ucontext);
#endif

/**
 * @brief Return current backtrace with demangled calls.
 * @param max_frames [in] maximal frames in trace.
 * @param caller_addr [in] optional caller address for replacing signal handler.
 * @return Backtrace content.
 */
std::string pretty_backtrace(int max_frames = 50, void *caller_addr = 0);

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

class Exception : public std::exception
{
public:
	Exception(int code = -1, const std::string &message_ = "Description inaccessible",
			const std::string &debug_message = "") noexcept :
		err(code), message(message_), debug(debug_message)
	{}

	Exception(const std::string &message_, const std::string &debug_message = "") noexcept :
		Exception(-1, message_, debug_message)
	{}

	const char *what() const noexcept
	{
		return message.c_str();
	}

	const char *debug_info() const noexcept
	{
		return debug.c_str();
	}

	int errcode() const noexcept
	{
		return err;
	}

private:

protected:

	int err;
	std::string message;
	std::string debug;
};


} //namespace aifil

#endif //AIFIL_UTILS_ERRUTILS_H
