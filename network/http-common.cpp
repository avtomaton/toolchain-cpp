#include "http-common.hpp"

#include <cstring>
#include <iostream>
#include <vector>

#include "boost/algorithm/string/split.hpp"
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/find.hpp>
#include <boost/algorithm/string/case_conv.hpp>

#include <common/errutils.hpp>
#include <common/stringutils.hpp>
#include <regex>


HttpStreamSplitter::HttpStreamSplitter() :
  state(State::WAIT_HEADER),
  line_buffer(""),
  content_buffer(""),
  content_oktets_counter(0),
  content_length(0),
  request_method(""),
  download_name("")
{
}


/**
 * Сброс всех параметров и очистка буфера.
 */

void HttpStreamSplitter::reset()
{
	state = State::WAIT_HEADER;
	line_buffer.clear();
	content_buffer.clear();
	content_oktets_counter = 0;
	content_length = 0;
	request_method.clear();
	request_url.clear();
	download_name.clear();
}


/**
 *
 * @return
 */
std::string HttpStreamSplitter::method() const
{
	return request_method;
}

std::string HttpStreamSplitter::url() const
{
	return request_url;
}


/**
 *
 * @return
 */

std::string HttpStreamSplitter::content() const
{
	return content_buffer;
}

std::string HttpStreamSplitter::download_filename() const
{
	return download_name;
}



// https://www.w3.org/Protocols/rfc2616/rfc2616-sec19.html#sec19.3
// The line terminator for message-header fields is the sequence CRLF.
// However, we recommend that applications, when parsing such headers,
// recognize a single LF as a line terminator and ignore the leading CR.

/**
 * Считывает символ.
 * @param ch [in] Символ, поступающий на вход.
 * @return
 */
bool HttpStreamSplitter::push_char(char ch)
{
	const char CR = '\r';
	const char LF = '\n';
	bool ret = false;

	switch (state)
	{
		case State::WAIT_HEADER:
		case State::PARSE_HEADER:
		{
			if (ch == CR)
				break;

			if (ch == LF)
			{
				process_line_buffer();
				line_buffer.clear();
				break;
			}

			line_buffer.push_back(ch);
		}
			break;

		case State::READ_CONTENT:
		{
			content_buffer.push_back(ch);
			content_oktets_counter++;
		}
			break;
	}

	if (state == State::READ_CONTENT && content_oktets_counter >= content_length)
	{
		state = State::WAIT_HEADER;
		ret = true;
	}

	return ret;
}


/**
 * Считывает строку.
 * @param ch [in] Строка, поступающая на вход.
 * @return
 */

bool HttpStreamSplitter::push_line(const std::string & line)
{
	bool ret = false;

	switch (state)
	{
		case State::WAIT_HEADER :
		case State::PARSE_HEADER :
		{
			line_buffer = line;
			process_line_buffer();
			line_buffer.clear();
		}
			break;

		case State::READ_CONTENT :
		{
			content_buffer.append(line);
			content_oktets_counter = content_buffer.size();

			if (content_oktets_counter >= content_length)
			{
				state = State::WAIT_HEADER;
				ret = true;
			}
		}
			break;
	}

	return ret;
}


/**
 * Обработка буфера из строк.
 * @return
 */
void HttpStreamSplitter::process_line_buffer()
{
	switch (state)
	{
		case State::WAIT_HEADER :
		{
			boost::algorithm::trim_left_if(line_buffer, boost::is_any_of(" \t"));
			boost::algorithm::trim_right_if(line_buffer, boost::is_any_of(" \t"));

			std::vector<std::string> tokens;
			boost::algorithm::split(tokens, line_buffer,
			                        boost::is_any_of(" \t"),
			                        boost::algorithm::token_compress_on);

			if (tokens.size() < 3)
				break;

			boost::algorithm::to_upper(tokens[2]);
			boost::algorithm::to_upper(tokens[0]);

			if (tokens[2].substr(0, 4) != "HTTP")
				break;

			state = State::PARSE_HEADER;
			request_method = tokens[0];
			request_url = tokens[1];
			content_length = 0;
			content_oktets_counter = 0;
			content_buffer.clear();
		}
			break;

		case State::PARSE_HEADER :
		{
			if (line_buffer.empty())
			{
				state = State::READ_CONTENT;
				break;
			}

			boost::algorithm::trim_left_if(line_buffer, boost::is_any_of(" \t"));
			boost::algorithm::trim_right_if(line_buffer, boost::is_any_of(" \t"));

			std::vector<std::string> tokens;
			boost::algorithm::split(tokens, line_buffer,
			                        boost::is_any_of(":"),
			                        boost::algorithm::token_compress_on);

			for (auto & token : tokens)
			{
				boost::algorithm::trim_left_if(token, boost::is_any_of(" \t"));
				boost::algorithm::trim_right_if(token, boost::is_any_of(" \t"));
			}

			if (tokens.size() < 2)
				break;

			// RFC 2616 - "Hypertext Transfer Protocol -- HTTP/1.1", Section 4.2, "Message Headers"
			boost::algorithm::to_upper(tokens[0]);
			
			if (tokens[0] == http::UPPER_CONTENT_DISPOSITION)
			{
				// не знаю почему, но регекспы здесь не работают
				// временно используем дедовский метод
				// TODO разобраться в проблеме
				/*
				std::regex rgx(".*filename=\"(.[^\"]*)\".*");
				std::smatch match;
				if (std::regex_search(tokens[1], match, rgx))
					download_name = match[1];
				*/

				size_t start_ind = tokens[1].find('\"');
				if (start_ind != std::string::npos)
				{
					start_ind++;
					size_t end_ind = 0;
					end_ind = tokens[1].find('\"', start_ind);
					if  (end_ind != std::string::npos)
						download_name = tokens[1].substr(start_ind, end_ind - start_ind);
				}
			}

			if (tokens[0] == http::UPPER_CONTENT_LENGTH)
			{
				long long cl = std::atol(tokens[1].c_str());

				if (cl < 0)
					cl = 0;

				content_length = static_cast<size_t>(cl);
			}
		}
			break;

		case State::READ_CONTENT :
			break;
	}
}


void HttpRequestParser::parse(const std::string &request_str, SimpleHttpRequest &out_request) const
{
	std::vector<std::string> split_str = aifil::split_by_substr(request_str, " ");
	if (split_str.size() < 2)
		af_exception("Invalid http header: " << request_str);
	if (split_str[0] == "GET")
	{
		out_request.method = SimpleHttpRequest::GET;
		out_request.url = split_str[1];
		parse_url(out_request.url, out_request);
	}
	else if (split_str[0] == "POST")
	{
		out_request.method = SimpleHttpRequest::POST;
		out_request.url = split_str[1];
		parse_url(out_request.url, out_request);
		std::string last = *split_str.rbegin();
		split_str = aifil::split_by_substr(last, "\r\n");
		if (split_str.size() == 2)
			parse_params(split_str[1], out_request.params);
	}
}

void HttpRequestParser::parse_url(const std::string &url, SimpleHttpRequest &out_request) const
{
	out_request.url = url;
	std::vector<std::string> split_str = aifil::split_by_substr(url, "://", true);
	if (split_str.empty())
		af_exception("Invalid url: " << url);
	std::string path;
	if (split_str.size() == 1)
	{
		out_request.protocol = "";
		path = split_str[0];
	}
	else
	{
		out_request.protocol = split_str[0];
		path = split_str[1];
	}
	// parse path
	split_str = aifil::split_by_substr(path, "/", true);
	if (split_str.empty())
		af_exception("Invalid url: " << url);
	std::string host_port;
	std::string rel_path;
	if (split_str.size() == 1)
	{
		host_port = "";
		rel_path = "/" + split_str[0];
	}
	else
	{
		host_port = split_str[0];
		rel_path = split_str[1];
	}
	// parse host_port
	if (!host_port.empty())
	{
		split_str = aifil::split_by_substr(host_port, ":", true);
		if (split_str.empty())
			af_exception("Invalid url: " << url);
		if (split_str.size() == 1)
		{
			out_request.host = split_str[0];
			out_request.port = "";
		}
		else
		{
			out_request.host = split_str[0];
			out_request.port = split_str[1];
		}
	}
	// parse rel_path
	if (!rel_path.empty())
	{
		split_str = aifil::split_by_substr(rel_path, "?", true);
		if (split_str.empty())
			af_exception("Invalid url: " << url);
		out_request.relative_path = split_str[0];
		if (split_str.size() >= 2)
			parse_params(split_str[1], out_request.params);
	}
}

void HttpRequestParser::parse_params(
    const std::string &params_str, std::map<std::string, std::string> &out_params) const
{
	out_params.clear();
	std::vector<std::string> split_str = aifil::split_by_substr(params_str, "&");
	for (const std::string &key_val_str : split_str)
	{
		std::vector<std::string> key_val = aifil::split_by_substr(key_val_str, "=");
		if (key_val.size() != 2)
			af_exception("Invalid http params string: " << params_str);
		out_params[key_val[0]] = key_val[1];
	}
}
