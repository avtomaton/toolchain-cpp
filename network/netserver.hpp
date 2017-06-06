#ifndef HTTP_SERVER_HPP
#define HTTP_SERVER_HPP

// Для работы с http сервером необходимо определить два производных класса:
//   class MyCustomHttpSession : public HttpSesson;
// и
//   class MyCustomHttpServer : TcpServer;
//
// Вся логика по обработке запросов пишется в MyCustomHttpSession.
// Для этого нужно переопределить методы on_get(), on_post(), on_put() (не обязательно переопределять все).
// Один экземпляр сессии обрабатывает одного клиента.
//
// В классе MyCustomHttpServer необходимо переопределить деструктор и метод create_session().
// В деструкторе обязательно надо вызвать shutdown().
// Метод create_session() создаёт экземпляр MyCustomHttpSession
//   std::shared_ptr<TcpSession> MyCustomHttpServer::create_session()
//   {
//       return std::make_shared<MyCustomHttpSession>(io_service);
//   }
// Возвращаемое значение должно иметь тип смарт-указателя на TcpSession.
//
// Таймаут соединения устанавливается в HttpSesson::set_timeout().
// По умолчанию сессия работает без таймаута, т.е. постоянно держит соединение.
// Параметр keep-alive устанавливается в методе HttpSession::set_keep_alive().
// По умолчанию параметра равен false, т.е. сессия закрывается после ответа клиенту.
//
// Создание и запуск сервера:
//   MyCustomHttpServer http_server(1234); // 1234 -- порт
//   http_server.run_async();
// Остановка сервера:
//   http_server.stop();
//   http_server.join();


#include <boost/asio.hpp>
#include <curl/curl.h>

#include <string>
#include <atomic>
#include <thread>
#include <functional>

#include <network/http-common.hpp>
#include <network/net-connection.hpp>
#include <network/tcp-server.hpp>

#include <boost/property_tree/ptree.hpp>


class HttpSession: public TcpSession
{
public:
	explicit HttpSession(std::shared_ptr<boost::asio::io_service> io_service_);
	virtual ~HttpSession();

	virtual void on_get(
		const std::string &relative_path,
		const std::map<std::string, std::string> &params,
		const std::string &content,
		const std::string &debug_url);

	virtual void on_post(
		const std::string &relative_path,
		const std::map<std::string, std::string> &params,
		const std::string &content,
		const std::string &debug_url);

	virtual void on_put(
		const std::string &relative_path,
		const std::map<std::string, std::string> &params,
		const std::string &content,
		const std::string &debug_url);

	void set_keep_alive(bool keep_alive_);

protected:
	void on_read_socket(const void * buffer, size_t octets);
	void on_after_send();
	void on_after_read();
	void response_200();
	void response_400();
	void response_500();

	std::atomic<bool> keep_alive_flag;
	HttpStreamSplitter http_splitter;
	bool request_ready;

	std::string unescape(const std::string &src);
	CURL* curl_handle;
};


class WebApiHandler
{
public:
	virtual ~WebApiHandler() {}

	virtual std::string accept_query(
			const std::map<std::string, std::string> &params,
			const std::string &content,
			const std::string &debug_url) = 0;

protected:
	static std::string response_200();
	static std::string response_400();
	static std::string response_500();
	virtual std::string json_as_response(boost::property_tree::ptree &json_tree);
};

class Http400Handler : public WebApiHandler
{
	virtual std::string accept_query(
			const std::map<std::string, std::string> &params,
			const std::string &content,
			const std::string &debug_url) override;
};

class WebApiSession : public HttpSession
{
public:
	explicit WebApiSession(std::shared_ptr<boost::asio::io_service> io_service_);
	virtual ~WebApiSession();

	virtual void on_get(
		const std::string &short_url,
		const std::map<std::string, std::string> &params,
		const std::string &content,
		const std::string &debug_url) override;

	virtual void on_post(
		const std::string &short_url,
		const std::map<std::string, std::string> &params,
		const std::string &content,
		const std::string &debug_url) override;

	virtual void on_put(
		const std::string &short_url,
		const std::map<std::string, std::string> &params,
		const std::string &content,
		const std::string &debug_url) override;

	void bind(const std::string &short_url, const std::shared_ptr<WebApiHandler> &web_handler);
	void bind_unexpected_url(const std::shared_ptr<WebApiHandler> &web_handler);
protected:
	void accept_query(
			const std::string &short_url,
			const std::map<std::string, std::string> &params,
			const std::string &content,
			const std::string &debug_url);

	std::map<std::string, std::shared_ptr<WebApiHandler>> web_handler_map;
	std::shared_ptr<WebApiHandler> unexpected_url_handler;
};

#endif // HTTP_SERVER_HPP
