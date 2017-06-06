#include "netserver.hpp"

#include <iostream>
#include <functional>
#include <common/errutils.hpp>
#include <common/stringutils.hpp>

#include <boost/property_tree/json_parser.hpp>


HttpSession::HttpSession(std::shared_ptr<boost::asio::io_service> io_service_) :
	TcpSession(io_service_),
	keep_alive_flag(false),
	request_ready(false)
{
	try
	{
		// curl init
		if(curl_global_init(CURL_GLOBAL_ALL) != 0)
			throw std::runtime_error("Function curl_global_init() failed!");
		curl_handle = curl_easy_init();
		if(curl_handle == NULL)
			throw std::runtime_error("Function curl_easy_init() failed!");
		// Отключаем вывод страницы в консоль.
		curl_easy_setopt(curl_handle, CURLOPT_NOBODY,1);
	}
	catch(...)
	{
		curl_easy_cleanup(curl_handle);
		curl_global_cleanup();
	}
}

HttpSession::~HttpSession()
{
	shutdown();
	if (curl_handle)
		curl_easy_cleanup(curl_handle);
	else
		aifil::log_warning("HttpSession: nullptr curl handler");

	curl_global_cleanup();
}

void HttpSession::set_keep_alive(bool keep_alive_)
{
	keep_alive_flag = keep_alive_;
}

void HttpSession::on_read_socket(const void * buffer, size_t count_in_bytes)
{
	std::string msg = "HttpSession::on_read_socket - ";
	msg += "count_in_bytes " + std::to_string(count_in_bytes);
	on_debug_msg(msg);
	try
	{
		const uint8_t * data = (uint8_t *) buffer;

		for (size_t i = 0; i < count_in_bytes; i++)
		{
			request_ready = http_splitter.push_char(data[i]);

			if (request_ready)
				break;
		}

		if (!request_ready)
			return;

		on_debug_msg("HttpSession: read complete");
		SimpleHttpRequest request;
		HttpRequestParser parser;

		parser.parse_url(http_splitter.url(), request);

		std::string unescape_path = unescape(request.relative_path);
		std::map<std::string, std::string> unescape_params;

		for (auto key_val: request.params)
		{
			std::string key = unescape(key_val.first);
			std::string val = unescape(key_val.second);
			unescape_params[key] = val;
		}

		// экранируем % -> %% для вывода в консоль
		std::string unescaped_url = request.url;
		size_t index = 0;

		while (true)
		{
			index = unescaped_url.find("%", index);
			if (index == std::string::npos)
				break;
			size_t len = 2; // len("%%")
			unescaped_url.replace(index, len, "%%");
			index += len;
		}

		if (http_splitter.method() == "GET")
			on_get(unescape_path, unescape_params, http_splitter.content(), unescaped_url);
		else if (http_splitter.method() == "POST")
			on_post(unescape_path, unescape_params, http_splitter.content(), unescaped_url);
		else if (http_splitter.method() == "PUT")
			on_put(unescape_path, unescape_params, http_splitter.content(), unescaped_url);

		http_splitter.reset();
	}
	catch(const std::exception & e)
	{
		response_500();
		close_session();
	}
}

void HttpSession::on_after_send()
{
	on_debug_msg("TcpSession::on_after_send()");
	if (!keep_alive_flag)
	{
		close_session();
	}
}

void HttpSession::on_after_read()
{
	if (!request_ready)
		open_async_read();
	request_ready = false;
}

void HttpSession::on_get(
	const std::string &relative_path,
	const std::map<std::string, std::string> &params,
	const std::string &content,
	const std::string &debug_url)
{

}

void HttpSession::on_post(
	const std::string &relative_path,
	const std::map<std::string, std::string> &params,
	const std::string &content,
	const std::string &debug_url)
{

}

void HttpSession::on_put(
		const std::string &relative_path,
		const std::map<std::string, std::string> &params,
		const std::string &content,
		const std::string &debug_url)
{

}

std::string HttpSession::unescape(const std::string &src)
{
	int ret_len;
	char *ret_ptr = curl_easy_unescape(curl_handle,	src.c_str(), src.size(), &ret_len);
	std::string ret_str(ret_ptr);
	curl_free(ret_ptr);
	return ret_str;
}

void HttpSession::response_200()
{
	std::stringstream header;
	header << "HTTP/1.1 200 OK\r\n";
	header << "\r\n";
	async_send_data(header.str());
}

void HttpSession::response_400()
{
	std::stringstream header_teml;
	header_teml << "HTTP/1.1 400 Bad Request\r\n";
	header_teml << "\r\n";
	async_send_data(header_teml.str());
}

void HttpSession::response_500()
{
	std::stringstream header_teml;
	header_teml << "HTTP/1.1 500 Internal Server Error\r\n";
	header_teml << "\r\n";
	async_send_data(header_teml.str());
}


//-----------------------------
//          Web Api
//-----------------------------

std::string WebApiHandler::response_200()
{
	std::stringstream header;
	header << "HTTP/1.1 200 OK\r\n";
	header << "\r\n";
	return header.str();
}

std::string WebApiHandler::response_400()
{
	std::stringstream header_teml;
	header_teml << "HTTP/1.1 400 Bad Request\r\n";
	header_teml << "\r\n";
	return header_teml.str();
}

std::string WebApiHandler::response_500()
{
	std::stringstream header_teml;
	header_teml << "HTTP/1.1 500 Internal Server Error\r\n";
	header_teml << "\r\n";
	return header_teml.str();
}

std::string WebApiHandler::json_as_response(boost::property_tree::ptree &json_tree)
{
	std::stringstream body;
	boost::property_tree::write_json(body, json_tree);

	std::stringstream header;
	header << "HTTP/1.1 200 OK\r\n";
	header << "Content-Length: " << body.str().size() << "\r\n";
	header << "content-type: application/json; charset=utf-8\r\n";
	header << "\r\n";

	return header.str() + body.str();
}

std::string Http400Handler::accept_query(
		const std::map<std::string, std::string> &params,
		const std::string &content,
		const std::string &debug_url)
{
	return response_400();
}

WebApiSession::WebApiSession(std::shared_ptr<boost::asio::io_service> io_service_)
	: HttpSession(io_service_)
{
	bind_unexpected_url(std::make_shared<Http400Handler>());
}

WebApiSession::~WebApiSession()
{
	shutdown();
}

void WebApiSession::on_get(
	const std::string &short_url,
	const std::map<std::string, std::string> &params,
	const std::string &content,
	const std::string &debug_url)
{
	accept_query(short_url, params, content, debug_url);
}

void WebApiSession::on_post(
	const std::string &short_url,
	const std::map<std::string, std::string> &params,
	const std::string &content,
	const std::string &debug_url)
{
	accept_query(short_url, params, content, debug_url);
}

void WebApiSession::on_put(
	const std::string &short_url,
	const std::map<std::string, std::string> &params,
	const std::string &content,
	const std::string &debug_url)
{
	accept_query(short_url, params, content, debug_url);
}

void WebApiSession::bind(
		const std::string &short_url, const std::shared_ptr<WebApiHandler> &web_handler)
{
	web_handler_map[short_url] = web_handler;
}

void WebApiSession::bind_unexpected_url(const std::shared_ptr<WebApiHandler> &web_handler)
{
	unexpected_url_handler = web_handler;
}

void WebApiSession::accept_query(
		const std::string &short_url,
		const std::map<std::string, std::string> &params,
		const std::string &content,
		const std::string &debug_url)
{
	std::shared_ptr<WebApiHandler> cur_handler;
	if (web_handler_map.find(short_url) == web_handler_map.end())
		cur_handler = unexpected_url_handler;
	else
		cur_handler = web_handler_map.at(short_url);

	af_assert(cur_handler);
	std::string response_data = cur_handler->accept_query(params, content, debug_url);
	bool ret = async_send_data(response_data);
	if (!ret)
		af_exception("Web api: error with send data");
}
