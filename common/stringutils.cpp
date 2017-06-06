#include "stringutils.hpp"
#include "errutils.hpp"

#include <algorithm>

#include <functional>
#include <locale>
#include <sstream>

#include <cstdarg>
#include <cstring>
#include <cstdio>
#include <ctime>
#include <cctype>

#define GNUC_4_8 __GNUC__ == 4 && __GNUC_MINOR__ == 8
#if GNUC_4_8
#else
#include <codecvt>
#endif

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

Base64Encoding::Base64Encoding() :
	encoding_table({
		'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
		'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
		'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
		'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'}),
	mod_table({0, 2, 1})
{
	decoding_table = create_decoding_table();
}

void Base64Encoding::encode(const std::vector<char> &src, std::string &dst)
{
	// читаем в буффер
//	std::vector<char> data((
//			std::istreambuf_iterator<char>(in_stream)),
//			std::istreambuf_iterator<char>());

	dst.clear();

	// кодируем

	int input_length = src.size();
	int output_length = 4 * ((input_length + 2) / 3);

	for (int i = 0; i < input_length;)
	{
		uint32_t octet_a = i < input_length ? (unsigned char)src[i++] : 0;
		uint32_t octet_b = i < input_length ? (unsigned char)src[i++] : 0;
		uint32_t octet_c = i < input_length ? (unsigned char)src[i++] : 0;
		uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

		dst.push_back(encoding_table[(triple >> 3 * 6) & 0x3F]);
		dst.push_back(encoding_table[(triple >> 2 * 6) & 0x3F]);
		dst.push_back(encoding_table[(triple >> 1 * 6) & 0x3F]);
		dst.push_back(encoding_table[(triple >> 0 * 6) & 0x3F]);
	}

	for (int i = 0; i < mod_table[input_length % 3]; i++)
		dst[output_length - 1 - i] = '=';
}

void Base64Encoding::decode(const std::string & src, std::vector<char> &dst)
{
	dst.clear();
	size_t input_length = src.size();

	if (input_length % 4 != 0)
		af_exception("corrupt base 64 encoding");

	size_t output_length = input_length / 4 * 3;
	if (src[input_length - 1] == '=') output_length--;
	if (src[input_length - 2] == '=') output_length--;

	dst.resize(output_length);

	for (size_t i = 0, j = 0; i < input_length;)
	{
		uint32_t sextet_a = src[i] == '=' ? 0U & i++ : decoding_table[src[i++]];
		uint32_t sextet_b = src[i] == '=' ? 0U & i++ : decoding_table[src[i++]];
		uint32_t sextet_c = src[i] == '=' ? 0U & i++ : decoding_table[src[i++]];
		uint32_t sextet_d = src[i] == '=' ? 0U & i++ : decoding_table[src[i++]];

		uint32_t triple =
				(sextet_a << 3 * 6)
				+ (sextet_b << 2 * 6)
				+ (sextet_c << 1 * 6)
				+ (sextet_d << 0 * 6);

		if (j < output_length)
			dst[j++] = static_cast<char>((triple >> 2 * 8) & 0xFF);
		if (j < output_length)
			dst[j++] = static_cast<char>((triple >> 1 * 8) & 0xFF);
		if (j < output_length)
			dst[j++] = static_cast<char>((triple >> 0 * 8) & 0xFF);
	}
}

std::vector<char> Base64Encoding::create_decoding_table()
{
	std::vector<char> table(256);
	for (char i = 0; i < 64; i++)
		table[(unsigned char) encoding_table[i]] = i;
	return table;
}

void base64_encode(const std::vector<char> & src, std::string &dst)
{
	Base64Encoding().encode(src, dst);
}

void base64_decode(const std::string & src, std::vector<char> &dst)
{
	Base64Encoding().decode(src, dst);
}

namespace utf8 {
#if GNUC_4_8
#else
		static std::wstring_convert<std::codecvt_utf8<char_t>, char_t> convert;
#endif

std::string encode(const string &str)
{
#if GNUC_4_8
#else
	return convert.to_bytes(str);
#endif
	af_assert(true); //not implemented
}

utf8::string decode(const std::string &str)
{
#if GNUC_4_8
#else
	return convert.from_bytes(str);
#endif
	af_assert(true); //not implemented
}
		

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
