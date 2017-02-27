#include "stringutils.hpp"

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

bool endswith(const std::string& str, const std::string& with)
{
	if (with.empty())
		return true;
	return (str.size() >= with.size()) && std::equal(with.rbegin(), with.rend(), str.rbegin());
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

std::string percent_str(int part, int total, const std::string &prefix)
{
	std::string result;
	if (!prefix.empty())
		result += prefix + ": ";
	result += stdprintf("%.2f%% (%d/%d)",
			total ? part * 100.0 / total : 0, part, total);
	return result;
}

std::vector<std::string> split(const std::string &s, char delim)
{
	std::vector<std::string> elems;
	std::stringstream ss(s);
	std::string item;
	while (std::getline(ss, item, delim))
		elems.push_back(item);
	return elems;
}

std::vector<std::string> split(const std::string &input, const std::string &delimiters)
{
	std::vector<std::string> elems;
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

std::vector<std::string> split_by_substr(
		const std::string &input, const std::string &delim_substr, const bool only_first)
{
	std::vector<std::string> elems;
	size_t cur = 0;
	size_t next = cur;
	while (next != std::string::npos)
	{
		while (true)
		{
			next = input.find(delim_substr, cur);
			if (next != cur)
			{
				break;
			}
			else
				cur += delim_substr.size();
		}
		if (next != cur && cur != input.size())
		{
			elems.push_back(input.substr(cur, next - cur));
			if (only_first && elems.size() == 2)
				break;
		}
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

int hamming(const std::string &str1, const std::string &str2)
{
	int ret = 0;
	size_t size_min = std::min(str1.size(), str2.size());
	size_t size_max = std::max(str1.size(), str2.size());
	for (size_t i = 0; i < size_min; i++)
	{
		if (str1[i] != str2[i])
			ret++;
	}
	ret += size_max - size_min;
	return ret;
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
