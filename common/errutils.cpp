#include "errutils.hpp"

#include "logging.hpp"
#include "stringutils.hpp"

#include <string>

#include <cstdarg>
#include <cstring>
#include <cstdlib>

#ifdef _WIN32
#else

#ifdef __APPLE__
// The deprecated ucontext routines require _XOPEN_SOURCE to be defined
#define _XOPEN_SOURCE
#else
#include <malloc.h>
#endif

#include <cxxabi.h>

#include <sys/resource.h>
#include <execinfo.h>
#include <ucontext.h>

#endif

namespace aifil
{

void assertion_failed(const char* cond, const char* file, unsigned lineno)
{
	log_error("ASSERTION FAILED at %s(%i): %s", file, lineno, cond);
	logger_set_mode(LOG_ALL);
	log_state(pretty_backtrace(100));
	fflush(stdout);
#ifdef _WIN32
	exit(3);
#else
	raise(SIGABRT);
#endif
}

void core_dump_enable()
{
#ifdef __GNUC__
	struct rlimit core_limit = { RLIM_INFINITY, RLIM_INFINITY };
	if (setrlimit(RLIMIT_CORE, &core_limit ) != 0)
		log_warning("Cannot enable core dumps!");
#else
	log_warning("Core dumps are not implemented for current platform");
#endif
}

#ifdef __GNUC__
void signal_set_exit_with_backtrace(int sig_num, siginfo_t *info, void *ucontext)
{
	ucontext_t *uc = (ucontext_t *)ucontext;

	 /* Get the address at the time the signal was raised */
	void *caller_address = 0;
#if defined(__i386__) // gcc specific
	 caller_address = (void *)uc->uc_mcontext.gregs[REG_EIP] // EIP: x86 specific
#elif defined(__x86_64__) // gcc specific
#ifndef __APPLE__
	 caller_address = (void *)uc->uc_mcontext.gregs[REG_RIP]; // RIP: x86_64 specific
#endif
#else
	#error Unsupported architecture.
#endif

	log_error("signal %d (%s), address is %p from %p\n",
		sig_num, strsignal(sig_num), info->si_addr,
				(void *)caller_address);

	log_state(pretty_backtrace(100, caller_address));
	fflush(stdout);
	exit(EXIT_FAILURE);
}
#endif

std::string pretty_backtrace(int max_frames, void *caller_addr)
{
#ifdef _WIN32
	return "backtrace is not implemented";
#else

	std::string result = stdprintf("stack trace:\n");

	// storage array for stack trace addresses data
	void **trace = (void**)malloc((max_frames + 1) * sizeof(void*));

	// retrieve current stack addresses
	int trace_size = backtrace(trace, max_frames);
	if (trace_size == 0)
	{
		result += stdprintf("  <empty, possibly corrupt>\n");
		free(trace);
		return result;
	}

	// overwrite signal handler info with caller's address
	if (caller_addr)
		trace[1] = caller_addr;

	// resolve addresses into strings containing "filename(function+address)",
	// this array must be free()-ed
	char** symbol_list = backtrace_symbols(trace, trace_size);

#ifdef __GNUC__

	// allocate string which will be filled with the demangled function name
	size_t funcnamesize = 512;
	char *funcname = (char*)malloc(funcnamesize);

	// iterate over the returned symbol lines. skip the first, it is the
	// address of this function.
	for (int i = 1; i < trace_size; ++i)
	{
		char *begin_name = 0, *begin_offset = 0, *end_offset = 0;

		// find parentheses and +address offset surrounding the mangled name:
		// ./module(function+0x...)[0x.....]
		for (char *p = symbol_list[i]; *p; ++p)
		{
			if (*p == '(')
				begin_name = p;
			else if (*p == '+')
				begin_offset = p;
			else if (*p == ')' && begin_offset) {
				end_offset = p;
				break;
			}
		}

		// try to demangle function name
		if (begin_name && begin_offset && end_offset && begin_name < begin_offset)
		{
			*begin_name++ = '\0';
			*begin_offset++ = '\0';
			*end_offset = '\0';

			// mangled name is now in [begin_name, begin_offset)
			// caller offset in [begin_offset, end_offset)
			int status;
			char* ret = abi::__cxa_demangle(
					begin_name, funcname, &funcnamesize, &status);
			if (status == 0)
			{
				// replace initial pointer, string can be reallocated
				funcname = ret;
				result += aifil::stdprintf("  %s[%p]: %s+%s\n",
						symbol_list[i], trace[i], ret, begin_offset);
			}
			else
			{
				// demangling failed, print it like C function without arguments:
				// module: function()offset
				result += aifil::stdprintf("  %s[%p]: %s()+%s\n",
						symbol_list[i], trace[i], begin_name, begin_offset);
			}
		}
		else
		{
			// couldn't parse the line: print initial one
			result += aifil::stdprintf("  %s\n", symbol_list[i]);
		}
	}

	free(funcname);

#else  // apple, need check
	for (i = 1; i < size && messages != NULL; ++i)
		result += aifil::stdprintf("  %s\n", symbol_list[i]);
#endif

	free(symbol_list);
	free(trace);

	return result;
#endif  // non-win32
}


void ErrContext::set_error_message(const std::string & message)
{
	std::lock_guard<std::mutex> lock(error_message_mutex);

	if ((!message.empty()) && (error_message_present))
		return;

	if (message.empty())
		error_message_present = false;
	else
		error_message_present = true;

	error_message = message;
}


std::string ErrContext::get_error_message(const std::string & sub_message)
{
	std::lock_guard<std::mutex> lock(error_message_mutex);

	if (!sub_message.empty())
	{
		error_message = sub_message;
		error_message_present = true;
	}

	return error_message;
}

} //namespace aifil
