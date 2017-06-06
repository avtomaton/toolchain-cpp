/**
 * @file basic-connection.cpp.
 * Реализация функций класса NetworkConnection.
 */

#include "network-connection.hpp"

#include <boost/bind.hpp>
#include <common/errutils.hpp>
#include <common/timeutils.hpp>


std::map<NetworkConnection::ErrorStatus, std::string> NetworkConnection::error_desc =
{
	{NetworkConnection::ErrorStatus::OK, "ok"},
	{NetworkConnection::ErrorStatus::IDLE, "idle"},
	{NetworkConnection::ErrorStatus::IO_ERROR, "io_error"},
	{NetworkConnection::ErrorStatus::CONNECT_ERROR, "connect_error"},
	{NetworkConnection::ErrorStatus::CONNECT_ABORT, "connect_abort"},
	{NetworkConnection::ErrorStatus::CONNECT_TIMEOUT, "connect_timeout"},
	{NetworkConnection::ErrorStatus::SIGNAL_ERROR, "signal_error"},
	{NetworkConnection::ErrorStatus::APP_ERROR, "app_error"}
};


NetworkConnection::NetworkConnection(uint16_t _listen_port) :
  port(_listen_port),
  ip(""),
  raw_log_file_name(""),
  erase_raw_log_file(false),
  first_connection(true),
  iddle_timeout_ms(0),
  read_buffer(nullptr),
  read_buffer_size(0),
  full_stop_flag(false),
  is_run_flag(false),
  is_connected_flag(false),
  session_stop_flag(false),
  reconnection_flag(false),
  handle_signals(false),
  is_clean_to_run(true),
  shutdown_flag(false),
  timers(0),
  start_timestamp(0),
  last_live_timestamp(0),
  live_flag(false),
  start_flag(false),
  error_message(""),
  error_flag(false),
  error_status(ErrorStatus::IDLE)
{
}


NetworkConnection::NetworkConnection(const std::string & _remote_ip,
                                     uint16_t _remote_port) :
  port(_remote_port),
  ip(_remote_ip),
  raw_log_file_name(""),
  erase_raw_log_file(false),
  first_connection(true),
  iddle_timeout_ms(0),
  read_buffer(nullptr),
  read_buffer_size(0),
  full_stop_flag(false),
  is_run_flag(false),
  is_connected_flag(false),
  session_stop_flag(false),
  reconnection_flag(false),
  handle_signals(false),
  is_clean_to_run(true),
  shutdown_flag(false),
  timers(0),
  start_timestamp(0),
  last_live_timestamp(0),
  live_flag(false),
  start_flag(false),
  new_accept_flag(false),
  error_message(""),
  error_flag(false)
{
}


NetworkConnection::~NetworkConnection()
{
	shutdown();
}


// Виртуальные методы для переопределения классами-потомками.
// ================================================================================================

void NetworkConnection::on_before_new_connection() {}
void NetworkConnection::on_new_connection() {}
void NetworkConnection::on_close_connection() {}
void NetworkConnection::on_session_end() {}
void NetworkConnection::on_read_socket(const void * buffer, size_t octets) {}
void NetworkConnection::on_async_data_sended(size_t octets_transferred, size_t octets_required) {}
void NetworkConnection::on_timer(uint64_t handle) {}


// Открытый интерфейс.
// ================================================================================================


void NetworkConnection::set_raw_log_file(const std::string & _file_name, bool _erase)
{
	if ((raw_log_file) && (raw_log_file->is_open()))
		raw_log_file->close();

	raw_log_file.reset();
	raw_log_file_name = _file_name;
	erase_raw_log_file = _erase;
}


void NetworkConnection::set_reconnection_mode(bool _auto_reconnect)
{
	reconnection_flag = _auto_reconnect;
}


void NetworkConnection::set_handle_signals_mode(bool _handle_signals)
{
	handle_signals = _handle_signals;
}


void NetworkConnection::set_timeout(uint64_t _timeout_ms)
{
	iddle_timeout_ms = _timeout_ms;
}


bool NetworkConnection::is_run() const
{
	return is_run_flag;
}


bool NetworkConnection::is_connected() const
{
	return is_connected_flag;
}


bool NetworkConnection::possible_need_restart() const
{
	if (!start_flag)
		return false;

	if (!live_flag)
		return (aifil::get_current_utc_unix_time_ms() > start_timestamp + STARTUP_TIMEOUT);

	return (aifil::get_current_utc_unix_time_ms() > last_live_timestamp + LIVE_TIMEOUT);
}


bool NetworkConnection::is_live_stream() const
{
	if (!live_flag)
		return false;

	return (aifil::get_current_utc_unix_time_ms() <=
	        last_live_timestamp + LIVE_TIMEOUT);
}


bool NetworkConnection::is_idle() const
{
	return idle_flag;
}


void NetworkConnection::run()
{
	if (!is_clean_to_run)
		af_exception("The connection is not ready to start.");

	idle_flag = false;
	is_clean_to_run = false;
	is_run_flag = true;
	start_timestamp = aifil::get_current_utc_unix_time_ms();
	start_flag = true;

	try
	{
		main_loop();
		is_run_flag = false;
		is_connected_flag = false;
		live_flag = false;
		idle_flag = true;
	}

	catch (const std::exception & e)
	{
		is_run_flag = false;
		is_connected_flag = false;
		live_flag = false;
		idle_flag = true;

		std::string msg = "NetworkConnection::main_loop(): Exception with message: ";
		msg += e.what();
		set_error_status(ErrorStatus::APP_ERROR, msg);
	}

	catch (...)
	{
		is_run_flag = false;
		is_connected_flag = false;
		live_flag = false;
		idle_flag = true;

		std::string msg = "NetworkConnection::main_loop(): Exception without message.";
		set_error_status(ErrorStatus::APP_ERROR, msg);
	}
}


void NetworkConnection::run_async()
{
	if (!is_clean_to_run)
		af_exception("The connection is not ready to start.");

	is_clean_to_run = false;
	is_run_flag = true;
	start_timestamp = aifil::get_current_utc_unix_time_ms();
	start_flag = true;
	idle_flag = false;
	async_thread.reset(new std::thread(&NetworkConnection::async_loop, this));
}


void NetworkConnection::stop()
{
	is_clean_to_run = false;
	start_flag = false;
	reset_error_status();
	stop_work();
}


void NetworkConnection::join()
{
	if ((async_thread) && (async_thread->joinable()))
	{
		async_thread->join();
		async_thread.reset();
	}
}


void NetworkConnection::reset()
{
	if (async_thread)
		af_exception("Asynchronous thread still exists.");

	reset_error_status();
	cleanup_session_resources();

	read_buffer = nullptr;
	read_buffer_size = 0;
	is_run_flag = false;
	session_stop_flag = false;
	full_stop_flag = false;
	first_connection = true;
	is_connected_flag = false;
	is_clean_to_run = true;
	start_flag = false;
	live_flag = false;
	idle_flag = true;
}


// Защищенные методы для использования классами-потомками.
// ================================================================================================


void NetworkConnection::update_live_timestamp()
{
	if (!live_flag)
		live_flag = true;
	last_live_timestamp = aifil::get_current_utc_unix_time_ms();
}


void NetworkConnection::set_error_status(const ErrorStatus error_status_, const std::string &err_message)
{
	std::lock_guard<std::mutex> lock(error_message_mutex);

	if (error_flag) return;

	error_message = err_message;
	error_flag = true;

	// APP_ERROR имеет больший приоритет, чем OK и IDLE, но меньший чем остальные ошибки
	if (error_status_ != ErrorStatus::APP_ERROR)
		error_status = error_status_;
	else if (error_status == ErrorStatus::OK || error_status == ErrorStatus::IDLE)
		error_status = ErrorStatus::APP_ERROR;
}


void NetworkConnection::reset_error_status()
{
	std::lock_guard<std::mutex> lock(error_message_mutex);
	error_message.clear();
	error_flag = false;
	error_status = ErrorStatus::IDLE;
}


void NetworkConnection::set_ok_status()
{
	error_status = ErrorStatus::OK;
}


void NetworkConnection::close_current_session()
{
	std::lock_guard<std::mutex> lock(session_resources_mutex);

	session_stop_flag = true;
	cancel_all_timers();

	if (socket && (socket->is_open()))
	{
		boost::system::error_code ec;
		socket->close(ec);
	}

	if (io_service && (!io_service->stopped()))
		io_service->stop();
}


void NetworkConnection::stop_work()
{
	full_stop_flag = true;
	close_current_session();
}


uint64_t NetworkConnection::create_timer()
{
	uint64_t handle = timers.size() + 1;
	using std::placeholders::_1;
	timers.emplace_back(handle, std::bind(&NetworkConnection::timers_handler, this, _1));
	return handle;
}


void NetworkConnection::remove_timer(uint64_t handle)
{
	bool erased = false;

	for (auto it = timers.begin(); it != timers.end(); ++it)
		if (it->get_handle() == handle)
		{
			timers.erase(it);
			erased = true;
			break;
		}

	if (!erased)
		af_exception("This timer is not exist.");
}


void NetworkConnection::remove_all_timers()
{
	timers.clear();
}


void NetworkConnection::cancel_all_timers()
{
	for (auto & timer : timers)
		timer.cancel();
}


void NetworkConnection::restart_timer(uint64_t handle, int64_t timeout_ms)
{
	af_assert(timeout_ms >= 0);
	bool resetted = false;

	for (auto it = timers.begin(); it != timers.end(); ++it)
		if (it->get_handle() == handle)
		{
			it->restart(timeout_ms);
			resetted = true;
			break;
		}

	if (!resetted)
		af_exception("This timer is not exist.");
}


void NetworkConnection::async_read_some_from_socket()
{
	af_assert(socket);

	if (on_request_buffer(read_buffer, read_buffer_size))
		socket->async_read_some(boost::asio::buffer(read_buffer, read_buffer_size),
		                        boost::bind(&NetworkConnection::asio_read_handler, this,
		                                    boost::asio::placeholders::error,
		                                    boost::asio::placeholders::bytes_transferred));
}


void NetworkConnection::async_send_data(const void * buffer, size_t octets)
{
	af_assert(socket);

	boost::asio::async_write(*socket, boost::asio::buffer(buffer, octets),
	                         boost::bind(&NetworkConnection::asio_write_handler, this,
	                                     boost::asio::placeholders::error,
	                                     boost::asio::placeholders::bytes_transferred,
	                                     octets));
}


void NetworkConnection::send_data(const void * buffer, size_t octets)
{
	af_assert(socket);

	try
	{
		size_t sended = boost::asio::write(*socket, boost::asio::buffer(buffer, octets));

		if (sended != octets)
			af_exception("The amount of data sent is not equal to the desired.");

		restart_iddle_timer();
		set_ok_status();
	}

	catch (...)
	{
		set_error_status(ErrorStatus::IO_ERROR, "Failed to write data to the socket.");
		close_current_session();
	}
}


// Закрытые методы для использования внутри класса.
// ================================================================================================


void NetworkConnection::open_log_file()
{
	if (raw_log_file_name.empty())
		return;

	std::ios_base::openmode open_mode = std::ios_base::binary | std::ios_base::out;

	if (erase_raw_log_file && first_connection)
		open_mode |= std::ios_base::trunc;
	else
		open_mode |= std::ios_base::app;

	raw_log_file.reset(new std::ofstream);
	raw_log_file->open(raw_log_file_name, open_mode);

	if (!raw_log_file->is_open())
		af_exception("File for data recording not opened.");
}


void NetworkConnection::write_buffer_to_log_file(const void * buffer, size_t octets)
{
	if (!raw_log_file)
		return;

	raw_log_file->write((const char *) buffer, octets);
	raw_log_file->flush();
}


void NetworkConnection::attach_signal_handlers_to_io_service()
{
	system_signals.reset(new boost::asio::signal_set(*io_service));
	system_signals->add(SIGINT);
	system_signals->add(SIGTERM);
	system_signals->async_wait(boost::bind(&NetworkConnection::asio_signal_handler, this,
	                                       boost::asio::placeholders::error,
	                                       boost::asio::placeholders::signal_number()));
}


void NetworkConnection::post_connection_task_to_io_service()
{
	if (ip.empty())
	{
		endpoint.reset(new boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port));
		acceptor.reset(new boost::asio::ip::tcp::acceptor(*io_service, *endpoint, port));
		acceptor->set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
		acceptor->async_accept(*socket,
		                       boost::bind(&NetworkConnection::asio_accept_handler, this,
		                                   boost::asio::placeholders::error));
	}
	else
	{
		endpoint.reset(new boost::asio::ip::tcp::endpoint(
		                 boost::asio::ip::address::from_string(ip), port));
		socket->async_connect(*endpoint,
		                      boost::bind(&NetworkConnection::asio_connect_handler, this,
		                                  boost::asio::placeholders::error));
	}
}


void NetworkConnection::setup_session_resources()
{
	session_stop_flag = false;
	is_connected_flag = false;

	io_service.reset(new boost::asio::io_service());

	if (handle_signals)
		attach_signal_handlers_to_io_service();

	if (iddle_timeout_ms > 0)
		iddle_timer.reset(new boost::asio::deadline_timer(*io_service));

	attach_timers_to_io_service();
	socket.reset(new boost::asio::ip::tcp::socket(*io_service));
}


void NetworkConnection::cleanup_session_resources()
{
	session_stop_flag = false;
	is_connected_flag = false;
	live_flag = false;
	detach_timers_from_io_service();
	iddle_timer.reset();
	system_signals.reset();
	socket.reset();
	acceptor.reset();
	endpoint.reset();
	raw_log_file.reset();
	io_service.reset();
}


void NetworkConnection::restart_iddle_timer()
{
	if (!iddle_timer)
		return;

	iddle_timer->expires_from_now(boost::posix_time::milliseconds(iddle_timeout_ms));
	iddle_timer->async_wait(boost::bind(&NetworkConnection::asio_iddle_timer_handler, this,
	                                    boost::asio::placeholders::error));
}


void NetworkConnection::attach_timers_to_io_service()
{
	for (AsioTimer & timer : timers)
		timer.assign_io_service(*io_service);
}


void NetworkConnection::detach_timers_from_io_service()
{
	for (AsioTimer & timer : timers)
		timer.detach_from_io_service();
}


void NetworkConnection::shutdown()
{
	if (shutdown_flag)
		return;

	start_flag = false;
	live_flag = false;
	stop_work();

	if (async_thread && (async_thread->joinable()))
	{
		async_thread->join();
		async_thread.reset();
	}

	timers.clear();
	iddle_timer.reset();
	system_signals.reset();
	socket.reset();
	acceptor.reset();
	endpoint.reset();
	raw_log_file.reset();
	io_service.reset();

	shutdown_flag = true;
}


void NetworkConnection::main_loop()
{
	while (!full_stop_flag)
	{
		reset_error_status();
		open_log_file();

		{
			std::lock_guard<std::mutex> lock(session_resources_mutex);

			if (full_stop_flag)
				break;

			setup_session_resources();
		}

		post_connection_task_to_io_service();
		on_before_new_connection();
		first_connection = false;

		io_service->run();

		if (is_connected_flag)
			on_close_connection();

		is_connected_flag = false;
		on_session_end();

		{
			std::lock_guard<std::mutex> lock(session_resources_mutex);
			cleanup_session_resources();
		}

		if ((!reconnection_flag) || full_stop_flag)
			break;

		std::this_thread::sleep_for(std::chrono::milliseconds(CPU_PREVENT_OVERLOAD_TIMEOUT_MS));
	}
}


void NetworkConnection::async_loop()
{
	try
	{
		main_loop();
		is_run_flag = false;
		is_connected_flag = false;
		live_flag = false;
		idle_flag = true;
	}

	catch (const std::exception & e)
	{
		is_run_flag = false;
		is_connected_flag = false;
		live_flag = false;
		idle_flag = true;

		std::string msg = "NetworkConnection::async_loop(): Exception with message: ";
		msg += e.what();
		set_error_status(ErrorStatus::APP_ERROR, msg);
	}

	catch (...)
	{
		is_run_flag = false;
		is_connected_flag = false;
		live_flag = false;
		idle_flag = true;

		std::string msg = "NetworkConnection::async_loop(): Exception without message.";
		set_error_status(ErrorStatus::APP_ERROR, msg);
	}
}


// Закрытые обработчики событий.
// ================================================================================================


void NetworkConnection::asio_accept_handler(const boost::system::error_code & ec)
{
	if (ec != 0)
	{
		close_current_session();

		if (ec != boost::asio::error::operation_aborted)
			set_error_status(ErrorStatus::CONNECT_ERROR, "Error while waiting for client connection.");
	}

	if (session_stop_flag)
		return;

	is_connected_flag = true;
	on_new_connection();
	async_read_some_from_socket();
	restart_iddle_timer();
	set_ok_status();
}


void NetworkConnection::asio_connect_handler(const boost::system::error_code & ec)
{
	if (ec != 0)
	{
		close_current_session();

		if (ec != boost::asio::error::operation_aborted)
			set_error_status(ErrorStatus::CONNECT_ERROR, "Unable connect to the host.");
	}

	if (session_stop_flag)
		return;

	is_connected_flag = true;
	on_new_connection();
	async_read_some_from_socket();
	restart_iddle_timer();
	set_ok_status();
}


void NetworkConnection::asio_read_handler(const boost::system::error_code & ec,
                                          size_t octets_received)
{
	if (ec != 0)
	{
		close_current_session();

		if (ec == boost::asio::error::connection_reset)
		{
			set_error_status(ErrorStatus::CONNECT_ABORT, "Connection reset by peer.");
			return;
		}

		if (ec == boost::asio::error::connection_aborted)
		{
			set_error_status(ErrorStatus::CONNECT_ABORT, "Connection aborted by peer.");
			return;
		}

		if (ec == boost::asio::error::operation_aborted)
			return;

		set_error_status(ErrorStatus::IO_ERROR, "An error occurred while reading data from the socket.");
	}

	if (session_stop_flag)
		return;

	write_buffer_to_log_file(read_buffer, octets_received);
	on_read_socket(read_buffer, octets_received);
	if (!new_accept_flag)
		async_read_some_from_socket();
	else
		new_accept();
	restart_iddle_timer();
	set_ok_status();
}


void NetworkConnection::asio_write_handler(const boost::system::error_code & ec,
                                           size_t octets_transferred, size_t octets_required)
{
	if ((ec != 0) || (octets_transferred != octets_required))
	{
		close_current_session();

		if (ec == boost::asio::error::connection_reset)
		{
			set_error_status(ErrorStatus::CONNECT_ABORT, "Connection reset by peer.");
			return;
		}

		if (ec == boost::asio::error::connection_aborted)
		{
			set_error_status(ErrorStatus::CONNECT_ABORT, "Connection aborted by peer.");
			return;
		}

		if (ec == boost::asio::error::operation_aborted)
			return;

		set_error_status(ErrorStatus::IO_ERROR, "Failed to write data to the socket.");
	}

	if (session_stop_flag)
		return;

	restart_iddle_timer();
	on_async_data_sended(octets_transferred, octets_required);
	set_ok_status();
}


void NetworkConnection::asio_signal_handler(const boost::system::error_code & ec,
                                            int signal_number)
{
	if (ec.value() != 0)
		return;

	set_error_status(ErrorStatus::SIGNAL_ERROR, "Got the system signal.");
	stop_work();
}


void NetworkConnection::asio_iddle_timer_handler(const boost::system::error_code & ec)
{
	if (ec.value() != 0)
		return;

	set_error_status(ErrorStatus::CONNECT_TIMEOUT, "Timer expires due to inactivity of the network connection.");
	close_current_session();
}


void NetworkConnection::timers_handler(uint64_t handle)
{
	on_timer(handle);
}


std::string NetworkConnection::get_error_status() const
{
	if (is_live_stream())
		return "ok";
	else if (error_status != ErrorStatus::OK)
		return error_desc[error_status];

	return error_desc[ErrorStatus::APP_ERROR];
}


std::string NetworkConnection::get_error_message()
{
	std::lock_guard<std::mutex> lock(error_message_mutex);
	return error_message;
}

void NetworkConnection::new_accept()
{
	//debug_msg("NetworkConnection::new_accept()");
	if (socket)
	{
		boost::system::error_code err;
		socket->shutdown(boost::asio::ip::tcp::socket::shutdown_both, err);
		if (socket->is_open())
			socket->close();
	}

	//socket.reset(new boost::asio::ip::tcp::socket(*io_service));
	af_assert(acceptor);
	acceptor->async_accept(*socket,
						   boost::bind(&NetworkConnection::asio_accept_handler, this,
									   boost::asio::placeholders::error));
	new_accept_flag = false;
}
