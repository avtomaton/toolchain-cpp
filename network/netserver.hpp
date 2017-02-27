#ifndef HTTP_SERVER_HPP
#define HTTP_SERVER_HPP

#include <boost/asio.hpp>
#include <curl/curl.h>

#include <string>
#include <atomic>
#include <thread>

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
private:
	void parse_url(const std::string &url, SimpleHttpRequest &out_request) const;
	void parse_params(
			const std::string &params_str, std::map<std::string, std::string> &out_params) const;
};

class NetServer
{
public:
	NetServer(int port_);
	NetServer(const NetServer &other) = delete;
	NetServer &operator=(const NetServer &other) = delete;
	
	virtual ~NetServer();
	
	void async_run();
	void main();
	void stop();
	void join();
	
protected:
	void on_accept(const boost::system::error_code &error);
	void on_read(const boost::system::error_code &error, std::size_t len);
	virtual void process_read(const boost::system::error_code &error, std::size_t len) = 0;
	virtual void on_error(const std::string &err_msg);
	void new_accept();
	
	const size_t BUFFER_SIZE = 2048;
	
	// свыше этого количества используется синхронный ответ сервера
	const size_t SYNC_DATA_TRESHOLD = 4096;
	
	const int port;
	std::vector<char> in_buffer;
	std::shared_ptr<std::thread> thread;
	std::unique_ptr<boost::asio::io_service> aios;
	std::unique_ptr<boost::asio::ip::tcp::acceptor> acceptor;
	std::unique_ptr<boost::asio::ip::tcp::socket> socket;
};

class HttpServer : public NetServer
{
public:
	HttpServer(int port_);
	HttpServer(const HttpServer &other) = delete;
	HttpServer &operator=(const HttpServer &other) = delete;
	virtual ~HttpServer();
	
	virtual void on_get(
			const std::string &relative_path,
			const std::map<std::string, std::string> &params,
			const std::string &debug_url) = 0;
	virtual void on_post(
			const std::string &relative_path,
			const std::map<std::string, std::string> &params,
			const std::string &debug_url) = 0;
	void send(const std::string &send_data);

private:
	
	virtual void process_read(const boost::system::error_code &error, std::size_t len);
	
	void on_send(const boost::system::error_code& error, std::size_t bytes_transferred);
	std::string unescape(const std::string &src);
	
	CURL* curl_handle;
};


#endif // HTTP_SERVER_HPP
