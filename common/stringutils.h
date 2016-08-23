#ifndef AIFIL_STRINGUTILS_H
#define AIFIL_STRINGUTILS_H

#include <list>
#include <stdint.h>
#include <string>
#include <vector>

#ifdef _WIN32
#define STDIO_U64F "%I64x"
#elif __x86_64__
#define STDIO_U64F "%lx"
#else
#define STDIO_U64F "%llx"
#endif

namespace aifil {

std::string stdprintf(const char* fmt, ...)
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

std::string& ltrim(std::string &s);  // trim from begin
std::string& rtrim(std::string &s);  // trim from end
std::string& strip(std::string &s);

// for using like this:
// if (is_in("test", {"train", "test", "install"})) do_something();
bool is_in(const std::string &str, const std::vector<std::string> &lst);

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

//std::vector<uint32_t> utf8_decode(const char* text);

//std::string base64_decode(const std::string&);
//std::string base64_encode(const std::string&);

//std::string miniz_compress(const std::string&);
//std::string miniz_uncompress(const std::string&);

} //namespace aifil

#endif // AIFIL_STRINGUTILS_H
