#include "stringutils.h"

#include <algorithm>
#include <functional>
#include <sstream>
#include <cstdarg>
#include <cstring>
#include <cstdio>
#include <ctime>
#include <cctype>

#if defined(_MSC_VER) && _MSC_VER <= 1310
#define vsnprintf _vsnprintf
#endif

namespace aifil {

std::string stdprintf(const char* fmt, ...)
{
	char buf[32768];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	buf[32768-1] = 0;
	return buf;
}

bool endswith(const std::string &str, const std::string &with)
{
	size_t str_len  = strlen(str.c_str());
	size_t with_len = strlen(with.c_str());
	if (str_len < with_len)
		return false;
	const char* s = str.c_str()  + (str_len - with_len);
	return strcmp(s, with.c_str()) == 0;
}

std::string stdftime(const char* fmt, uint64_t t64)
{
	time_t tt = time_t(t64 / 1000000);
	std::tm* t = localtime(&tt);
	if (!t)
		return "invalid time";
	char buf[1024];
	strftime(buf, sizeof(buf), fmt, t);
	return buf;
}

std::string uint64_t_to_str(uint64_t v)
{
	return stdprintf(STDIO_U64F, v);
}

std::list<std::string> split(const std::string &s, char delim)
{
	std::list<std::string> elems;
	std::stringstream ss(s);
	std::string item;
	while (std::getline(ss, item, delim))
		elems.push_back(item);
	return elems;
}

std::list<std::string> split(const std::string &input, const std::string &delimiters)
{
	std::list<std::string> elems;
	size_t cur = 0;
	size_t next = cur;
	while (next != std::string::npos)
	{
		while (true)
		{
			next = input.find_first_of(delimiters, cur);
			if (next != cur)
			{
				break;
			}
			else
				cur += 1;
		}
		if (next != cur && cur != input.size())
			elems.push_back(input.substr(cur, next - cur));
		cur = next;
	}
	return elems;
}

std::string& ltrim(std::string &s)
{
	s.erase(s.begin(), std::find_if(
				s.begin(), s.end(),
				std::not1(std::ptr_fun<int, int>(std::isspace))));
	return s;
}

std::string& rtrim(std::string &s)
{
	s.erase(std::find_if(s.rbegin(), s.rend(),
				std::not1(std::ptr_fun<int, int>(std::isspace))).base(),
			s.end());
	return s;
}

std::string& strip(std::string &s)
{
	return ltrim(rtrim(s));
}

bool is_in(const std::string &str, const std::vector<std::string> &lst)
{
	for (const auto &el: lst)
	{
		if (el == str)
			return true;
	}
	return false;
}

/*
static void tests()
{
	std::cout << "SSS\n";
	auto l = split("a\t  \tee   s  \tt\n\n", "\t \n");
	for (auto el: l)
		std::cout << el << "\n";
	std::cout << "SSS\n";
	l = split("\t\t   \t", "\t \n");
	for (auto el: l)
		std::cout << el << "\n";
	std::cout << "SSS\n";
	l = split("", "\t \n");
	for (auto el: l)
		std::cout << el << "\n";
	std::cout << "SSS\n";
	l = split("abcdef", "\t \n");
	for (auto el: l)
		std::cout << el << "\n";
}
*/

} //namespace aifil
