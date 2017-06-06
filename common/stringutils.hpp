#ifndef AIFIL_STRINGUTILS_H
#define AIFIL_STRINGUTILS_H

#include "c_attribs.hpp"

#include <list>
#include <stdint.h>
#include <string>
#include <vector>

#ifdef _WIN32
#define STDIO_U64F "%I64x"
#elif defined (__x86_64__) || defined (__x86_64)
#define STDIO_U64F "%llx"
#else
#define STDIO_U64F "%lx"
#endif

namespace aifil {

std::string stdprintf(const char* fmt, ...) AF_FORMAT(printf, 1, 2)
#ifndef _MSC_VER
__attribute__ ((format (printf, 1, 2)))
#endif
;
bool endswith(const std::string &str, const std::string &with);
std::string stdftime(const char* fmt, uint64_t t64);
std::string uint64_t_to_str(uint64_t v);

// print something like 'prefix: 30% (6/20)'
std::string percent_str(int part, int total, const std::string &prefix = "");

// split by selected character
std::vector<std::string> split(const std::string &s, char delim);
// split by ANY symbol combinations in delimiters
std::vector<std::string> split(const std::string &s, const std::string &delimiters);
// split by selected substring
std::vector<std::string> split_by_substr(
		const std::string &s, const std::string &delim_substr, const bool only_first = false);

std::string& ltrim(std::string &s);  // trim from begin
std::string& rtrim(std::string &s);  // trim from end
std::string& strip(std::string &s);

// for using like this:
// if (is_in("test", {"train", "test", "install"})) do_something();
bool is_in(const std::string &str, const std::vector<std::string> &lst);

int hamming(const std::string &str1, const std::string &str2);

class Base64Encoding
{
public:
	Base64Encoding();

	void encode(const std::vector<char> &src, std::string &dst);
	void decode(const std::string &src, std::vector<char> &dst);

private :
	std::vector<char> create_decoding_table();

	const std::vector<char> encoding_table;
	const std::vector<int> mod_table;
	std::vector<char> decoding_table;
};

void base64_encode(const std::vector<char> &src, std::string &dst);
void base64_decode(const std::string &src, std::vector<char> &dst);

namespace utf8
{
typedef char16_t char_t;
typedef std::u16string string;

std::string encode(const string &str);
string decode(const std::string &str);
}

//typedef std::list<std::string> csv_list;

//int short_notation_compare_predicate(const std::string& left, const std::string& right);
//std::string standard_model_name(
//	const std::string& vendor, const std::string& model,
//	const csv_list& available_models);

//bool endswith(const char* str, const char* with);
//std::string print_uint64(uint64_t);
//uint64_t scan_uint64(const std::string&);
//std::string print_uint64hex(uint64_t);
//uint64_t scan_uint64hex(const std::string&);

//std::list<std::string> csv_explode(const std::string& s);
//std::string csv_implode(const std::list<std::string>& l);
//int32_t our_hash_impl(const char *s);

//std::string ansi_to_unicode(const char* str);
//std::string unicode_to_ansi(const char* str);

//std::string escape_string(const std::string& uri);
//std::string unescape_string(const std::string& uri);

} //namespace aifil

#endif // AIFIL_STRINGUTILS_H
