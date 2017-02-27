#include "netserver.hpp"

#include <iostream>
#include <functional>
#include <common/errutils.hpp>
#include <common/stringutils.hpp>


NetServer::NetServer(int port_) :
		port(port_),
		in_buffer(BUFFER_SIZE)
{
	
}

NetServer::~NetServer()
{
	stop();
	join();
}

void NetServer::async_run()
{
	thread = std::make_shared<std::thread>(std::bind(&NetServer::main, this));
}

void NetServer::main()
{
	aios.reset(new boost::asio::io_service);
	
	try
	{
		acceptor.reset(new boost::asio::ip::tcp::acceptor(
				*aios, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)));
	}
	catch (const std::exception & e)
	{
		aifil::log_error("%s; port = %i\n", e.what(), port);
		return;
	}
	
	socket.reset(new boost::asio::ip::tcp::socket(*aios));
	
	acceptor->async_accept(*socket, std::bind(&NetServer::on_accept, this, std::placeholders::_1));
	aios->run();
	
	//закрываем соединение
	if (socket)
	{
		boost::system::error_code err;
		socket->shutdown(boost::asio::ip::tcp::socket::shutdown_both, err);
		if (socket->is_open())
			socket->close();
	}
	if (acceptor && acceptor->is_open())
		acceptor->close();
}

void NetServer::stop()
{
	if (aios)
		aios->stop();
}

void NetServer::join()
{
	if (thread && thread->joinable())
		thread->join();
	thread.reset();
}

void NetServer::on_accept(const boost::system::error_code &error)
{
	af_assert(socket);
	socket->async_read_some(boost::asio::buffer(in_buffer), std::bind(
			&NetServer::on_read, this,
			std::placeholders::_1, std::placeholders::_2));
}

void NetServer::new_accept()
{
	if (socket)
	{
		boost::system::error_code err;
		socket->shutdown(boost::asio::ip::tcp::socket::shutdown_both, err);
		if (socket->is_open())
			socket->close();
	}
	
	socket.reset(new boost::asio::ip::tcp::socket(*aios));
	af_assert(acceptor);
	acceptor->async_accept(*socket,
	                       std::bind(&HttpServer::on_accept, this, std::placeholders::_1));
}

void NetServer::on_error(const std::string &err_msg)
{
	// здесь пустой. переопределяется в потомках.
}

void NetServer::on_read(const boost::system::error_code &error, std::size_t len)
{
	bool final_read = false;
	try
	{
		if (error == boost::asio::error::eof)
		{
			new_accept();
			return;
		}
		
		if(error.value() != 0)
			af_exception("Socket error: " << error.message());
		
		final_read = process_read(error, len);

		if (final_read)
			new_accept();
		else
			socket->async_read_some(boost::asio::buffer(in_buffer), std::bind(
					&NetServer::on_read, this,
					std::placeholders::_1, std::placeholders::_2));
	}
	catch (const std::exception & e)
	{
		std::string err_msg(e.what());
		on_error(err_msg);
		new_accept();
	}
}

HttpServer::HttpServer(int port_) :
		NetServer(port_)
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

HttpServer::~HttpServer()
{
	stop();
	join();

	curl_easy_cleanup(curl_handle);
	curl_global_cleanup();
}

bool HttpServer::process_read(const boost::system::error_code &error, std::size_t len)
{
	bool final_read = false;
	size_t char_counter = 0;
	for (const char c: in_buffer)
	{
		final_read = http_splitter.push_char(c);
		char_counter++;
		if (final_read || char_counter >= len)
			break;
	}

	if (!final_read)
		return false;

	//std::string header_str(in_buffer.begin(), in_buffer.begin() + len);
	SimpleHttpRequest request;
	HttpRequestParser parser;
	//parser.parse(header_str, request);
	
	try
	{
		parser.parse_url(http_splitter.url(), request);
	}
	catch (const std::exception & e)
	{
		aifil::log_error(e.what());
	}
	
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
		on_get(unescape_path, unescape_params, unescaped_url);
	else if (http_splitter.method() == "POST")
		on_post(unescape_path, unescape_params, http_splitter.content(), unescaped_url);
	else if (http_splitter.method() == "PUT")
		on_put(http_splitter.download_filename(), http_splitter.content());
	
	http_splitter.reset();
	return true;
}

std::string HttpServer::unescape(const std::string &src)
{
	int ret_len;
	char *ret_ptr = curl_easy_unescape(curl_handle,	src.c_str(), src.size(), &ret_len);
	std::string ret_str(ret_ptr);
	curl_free(ret_ptr);
	return ret_str;
}

void HttpServer::on_send(const boost::system::error_code& error, std::size_t bytes_transferred)
{
	if (error.value() != 0)
	{
		std::stringstream ss;
		ss << "Send data error: " << error.message() << ", bytes transfered: " << bytes_transferred;
		on_error(ss.str());
	}
}

void HttpServer::send(const std::string &send_data)
{
	std::vector<char> out_buffer;
	out_buffer.clear();
	out_buffer.insert(out_buffer.end(), send_data.begin(), send_data.end());
	if (out_buffer.size() < SYNC_DATA_THRESHOLD)
		socket->async_send(
				boost::asio::buffer(out_buffer, out_buffer.size()),
					std::bind(&HttpServer::on_send, this,
							std::placeholders::_1, std::placeholders::_2));
	else
	{
		boost::system::error_code err;
		boost::asio::write(
					*socket,
					boost::asio::buffer(out_buffer, out_buffer.size()),
					err);
		if(err.value() != 0)
			af_exception("Socket error: " << err.message());
	}
}
