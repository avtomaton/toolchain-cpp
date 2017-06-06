#include "tcp-server.hpp"

#include <boost/bind.hpp>
#include <common/errutils.hpp>
#include <common/timeutils.hpp>


TcpSession::TcpSession(std::shared_ptr<boost::asio::io_service> io_service_) :
	iddle_timeout_ms(0),
	shutdown_flag(false),
	is_close_flag(false),
	async_send_flag(false),
	read_buffer(READ_BUFFER_SIZE),
	io_service(io_service_)
{
	socket.reset(new boost::asio::ip::tcp::socket(*io_service));
}

TcpSession::~TcpSession()
{
	shutdown();
}

void TcpSession::on_debug_msg(const std::string &msg)
{
	//aifil::log_state(msg);
}

void TcpSession::set_timeout(uint64_t _timeout_ms)
{
	iddle_timeout_ms = _timeout_ms;
}

void TcpSession::on_read_socket(const void * buffer, size_t octets)
{

}

void TcpSession::on_after_send()
{

}

void TcpSession::on_after_read()
{
	open_async_read();
}

void TcpSession::shutdown()
{
	if (shutdown_flag)
		return;

	iddle_timer.reset();
	socket.reset();

	shutdown_flag = true;
}

bool TcpSession::async_send_data()
{
	on_debug_msg("TcpSession::async_send_data");
	if (async_send_flag)
		return false;

	af_assert(socket);

	async_send_flag = true;
	send_buffer_cursor = 0;
	size_t octets_required = std::min(send_buffer.size(), SEND_PACKAGE_SIZE);

	boost::asio::async_write(*socket, boost::asio::buffer(send_buffer.data(), octets_required),
							 boost::bind(&TcpSession::asio_write_handler, this,
										 boost::asio::placeholders::error,
										 boost::asio::placeholders::bytes_transferred,
										 octets_required));
	return true;
}

bool TcpSession::async_send_data(const std::string &data)
{
	std::copy(data.begin(), data.end(), std::back_inserter(send_buffer));
	return async_send_data();
}

void TcpSession::send_data(const void * buffer, size_t octets)
{
	af_assert(socket);

	try
	{
		size_t sended = boost::asio::write(*socket, boost::asio::buffer(buffer, octets));

		if (sended != octets)
			af_exception("The amount of data sent is not equal to the desired.");

		reset_connection_timeout();
	}

	catch (const std::exception &e)
	{
		on_debug_msg(e.what());
		close_session();
	}
}

void TcpSession::send_data(const std::string &data)
{
	send_data(data.c_str(), data.size());
}

void TcpSession::close_session()
{
	on_debug_msg("TcpSession::close_current_session()");

	if (socket)
	{
		boost::system::error_code ec;
		socket->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
		if (ec != 0)
			on_debug_msg("TcpSession::close_current_session: asio error " + ec.message());
		if (socket->is_open())
			socket->close(ec);
		if (ec != 0)
			on_debug_msg("TcpSession::close_current_session: asio error " + ec.message());
	}
	socket.reset();
	iddle_timer.reset();
	is_close_flag = true;
}

void TcpSession::asio_read_handler(const boost::system::error_code & ec,
										  size_t octets_received)
{
	std::string msg = "TcpSession::asio_read_handler - ";
	msg += ec.message();
	msg += ", octets_received " + std::to_string(octets_received);
	on_debug_msg(msg);

	if (is_close_flag)
		return;

	if (ec != 0)
	{
		on_debug_msg("TcpSession::asio_read_handler: asio error " + ec.message());
		close_session();
		return;
	}

	on_read_socket(read_buffer.data(), octets_received);
	on_after_read();
	reset_connection_timeout();
}

void TcpSession::asio_write_handler(
		const boost::system::error_code & ec,
		size_t octets_transferred,
		size_t octets_required)
{
	if (is_close_flag)
		return;

	if ((ec != 0) || (octets_transferred != octets_required))
	{
		on_debug_msg("TcpSession::asio_write_handler: asio error " + ec.message());
		close_session();
		return;
	}

	send_buffer_cursor += octets_transferred;
	if (send_buffer_cursor < send_buffer.size())
	{
		// не все данные отправлены. отправляем следующую порцию.
		size_t next_octets_required =
				std::min(send_buffer.size() - send_buffer_cursor, SEND_PACKAGE_SIZE);

		boost::asio::async_write(
				*socket,
				boost::asio::buffer(
						send_buffer.data() + send_buffer_cursor,
						next_octets_required),
				boost::bind(&TcpSession::asio_write_handler, this,
						boost::asio::placeholders::error,
						boost::asio::placeholders::bytes_transferred,
						next_octets_required));
	}
	else
	{
		// все данные отправлены
		on_after_send();
	}

	reset_connection_timeout();
}

void TcpSession::asio_iddle_timer_handler(const boost::system::error_code & ec)
{
	close_session();
}

void TcpSession::reset_connection_timeout()
{
	if (!iddle_timer)
		return;

	if (iddle_timeout_ms > 0)
	{
		iddle_timer->expires_from_now(boost::posix_time::milliseconds(iddle_timeout_ms));
		iddle_timer->async_wait(boost::bind(&TcpSession::asio_iddle_timer_handler, this,
											boost::asio::placeholders::error));
	}
}

void TcpSession::open_async_read()
{
	af_assert(socket);

	socket->async_read_some(boost::asio::buffer(read_buffer),
							boost::bind(&TcpSession::asio_read_handler, this,
										boost::asio::placeholders::error,
										boost::asio::placeholders::bytes_transferred));
}

void TcpSession::open_async_accept(boost::asio::ip::tcp::acceptor &acceptor,
		std::function<void(const boost::system::error_code&)> on_accept)
{
	acceptor.async_accept(*socket, on_accept);
}

void TcpSession::setup_session_resources()
{
	if (iddle_timeout_ms > 0)
		iddle_timer.reset(new boost::asio::deadline_timer(*io_service));

	socket.reset(new boost::asio::ip::tcp::socket(*io_service));
}

void TcpSession::cleanup_session_resources()
{
	iddle_timer.reset();
	socket.reset();
}

bool TcpSession::is_close() const
{
	return is_close_flag;
}


//-----------------------------
//---------TcpServer-----------
//-----------------------------


TcpServer::TcpServer(uint16_t _listen_port):
	port(_listen_port),
	thread_is_run(false),
	server_is_run(false),
	is_clean_to_run(true),
	shutdown_flag(false),
	full_stop_flag(false)
{

}

TcpServer::~TcpServer()
{
	shutdown();
}

void TcpServer::run_async()
{
	if (!is_clean_to_run)
		af_exception("The connection is not ready to start.");

	is_clean_to_run = false;
	thread_is_run = true;
	async_thread.reset(new std::thread(&TcpServer::async_loop, this));
}

void TcpServer::run()
{
	if (!is_clean_to_run)
		af_exception("The connection is not ready to start.");

	is_clean_to_run = false;
	thread_is_run = true;

	try
	{
		main_loop();
	}

	catch (const std::exception &e)
	{
		on_debug_msg(e.what());
	}

	thread_is_run = false;
}

void TcpServer::stop()
{
	on_debug_msg("TcpServer::stop");
	is_clean_to_run = false;
	if (io_service && !io_service->stopped())
		io_service->stop();
	full_stop_flag = true;
}

void TcpServer::join()
{
	if ((async_thread) && (async_thread->joinable()))
	{
		async_thread->join();
		async_thread.reset();
	}
}

void TcpServer::reset()
{
	if (async_thread)
		af_exception("Asynchronous thread still exists.");

	new_session = nullptr;
	sessions.clear();

	thread_is_run = false;
	server_is_run = false;
	full_stop_flag = false;
	is_clean_to_run = true;
}

void TcpServer::main_loop()
{
	while (!full_stop_flag)
	{
		try
		{
			init();
			io_service->run();
		}
		catch(const std::exception &err)
		{
			aifil::log_error("Web api error: " + std::string(err.what()));
		}

		close_server();

		if (full_stop_flag)
			break;

		std::this_thread::sleep_for(std::chrono::milliseconds(CPU_PREVENT_OVERLOAD_TIMEOUT_MS));
	}
}

void TcpServer::async_loop()
{
	try
	{
		main_loop();
	}
	catch (const std::exception &e)
	{
		on_debug_msg(e.what());
	}

	thread_is_run = false;
}

bool TcpServer::is_run() const
{
	return thread_is_run;
}

void TcpServer::on_debug_msg(const std::string &msg)
{
	//aifil::log_state(msg);
}

void TcpServer::on_read_socket(const void * buffer, size_t octets)
{

}

void TcpServer::shutdown()
{
	on_debug_msg("TcpServer::shutdown");
	if (shutdown_flag)
		return;

	stop();

	if (async_thread && (async_thread->joinable()))
	{
		async_thread->join();
		async_thread.reset();
	}

	acceptor.reset();
	endpoint.reset();
	io_service.reset();

	shutdown_flag = true;
}

void TcpServer::init()
{
	io_service.reset(new boost::asio::io_service());
	endpoint.reset(new boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port));
	acceptor.reset(new boost::asio::ip::tcp::acceptor(*io_service));
	acceptor->open(endpoint->protocol());
	acceptor->set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
	acceptor->bind(*endpoint);
	acceptor->listen();

	new_session = create_session();
	new_session->open_async_accept(
			*acceptor,
			std::bind(&TcpServer::asio_accept_handler, this, std::placeholders::_1));

	server_is_run = true;
}

void TcpServer::close_server()
{
	on_debug_msg("TcpServer::close_server");

	std::lock_guard<std::mutex> lock(close_server_mutex);

	if (!server_is_run)
	{
		on_debug_msg("TcpServer::close_server alredy closed");
		return;
	}

	if (io_service && (!io_service->stopped()))
		io_service->stop();

	for (std::shared_ptr<TcpSession> session : sessions)
	{
		if (session)
			session->close_session();
		else
			aifil::log_warning("TcpServer: nullptr session");
	}
	sessions.clear();

	acceptor.reset();
	endpoint.reset();
	io_service.reset();

	server_is_run = false;
}

void TcpServer::asio_accept_handler(const boost::system::error_code & ec)
{
	on_debug_msg("TcpServer::asio_accept_handler");

	if (ec != 0)
	{
		new_session->close_session();
	}
	else
	{
		new_session->open_async_read();
		new_session->reset_connection_timeout();
		sessions.push_back(new_session);
	}

	remove_close_sessions();

	new_session = create_session();
	new_session->open_async_accept(
			*acceptor,
			std::bind(&TcpServer::asio_accept_handler, this, std::placeholders::_1));
}

void TcpServer::remove_close_sessions()
{
	std::vector<std::shared_ptr<TcpSession>> tmp_sessions;
	for (const std::shared_ptr<TcpSession> session : sessions)
	{
		if (!session->is_close())
			tmp_sessions.push_back(session);
	}
	std::swap(sessions, tmp_sessions);
}
