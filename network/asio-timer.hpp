#ifndef ASIOTIMER_HPP
#define ASIOTIMER_HPP

#include <memory>
#include <functional>
#include <boost/asio.hpp>
#include <boost/bind.hpp>


class AsioTimer
{
	public:
		AsioTimer();
		AsioTimer(uint64_t _handle, std::function<void(uint64_t)> _timer_handler);
		~AsioTimer();

		void set_handler(uint64_t _handle, std::function<void(uint64_t)> _timer_handler);
		void assign_io_service(boost::asio::io_service & _io_service);
		void detach_from_io_service();
		void restart(int64_t _timeout_ms);
		uint64_t get_handle() const;
		void cancel();

	private:
		uint64_t handle;
		std::unique_ptr<boost::asio::deadline_timer> timer;
		std::function<void(uint64_t)> timer_handler;

		void asio_timer_handler(const boost::system::error_code & ec);
};

#endif // ASIOTIMER_HPP
