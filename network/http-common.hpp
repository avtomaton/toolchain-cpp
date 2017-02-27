#ifndef HTTP_COMMON_HPP
#define HTTP_COMMON_HPP

#include <string>
#include <map>


namespace http
{
	// RFC 2616 - "Hypertext Transfer Protocol -- HTTP/1.1", Section 4.2, "Message Headers"
	// Each header field consists of a name followed by a colon (":") and the field value.
	// Field names are case-insensitive.

	// "Красивые" константы для формирования заголовков.
	const std::string CONTENT_TYPE = "Content-Type";
	const std::string CONTENT_DISPOSITION = "Content-Disposition";
	const std::string CONTENT_LENGTH = "Content-Length";

	// Константы для сравнения прочитанных из заголовка имен полей, приведенных к
	// верхнему регистру, для нечувствительности к регистру, согласно RFC.

	const std::string UPPER_CONTENT_TYPE = "CONTENT-TYPE";
	const std::string UPPER_CONTENT_DISPOSITION = "CONTENT-DISPOSITION";
	const std::string UPPER_CONTENT_LENGTH = "CONTENT-LENGTH";

	// TODO: Проверить по стандарту, чувствительны ли к регистру следующие объявления.
	
	const std::string CONTENT_TYPE_FILE = "application/octet-stream";
	const std::string CONTENT_TYPE_JSON = "application/json";
}


class HttpStreamSplitter
{
public:
	HttpStreamSplitter();

	void reset();

	bool push_char(char ch);
	bool push_line(const std::string & line);

	std::string method() const;
	std::string url() const;
	std::string content() const;
	std::string download_filename() const;

private:
	/**
		 * Перечисление возможных состояний.
		 */
	enum class State { WAIT_HEADER, PARSE_HEADER, READ_CONTENT };

	void process_line_buffer();

	State state;
	std::string line_buffer;
	std::string content_buffer;
	size_t content_oktets_counter;
	size_t content_length;
	std::string request_method;
	std::string request_url;
	std::string download_name;
};

struct SimpleHttpRequest
{
	enum Method {UNKNOWN, GET, POST};

	Method method = UNKNOWN;
	std::string protocol;
	std::string host;
	std::string port;
	std::string url;
	std::string relative_path; // with one leading slash
	std::map<std::string, std::string> params;
};

class HttpRequestParser
{
public:
	void parse(const std::string &request_str, SimpleHttpRequest &out_request) const;
	void parse_url(const std::string &url, SimpleHttpRequest &out_request) const;
	void parse_params(
	    const std::string &params_str, std::map<std::string, std::string> &out_params) const;
};

#endif // HTTP_COMMON_HPP
