#include "asio-timer.hpp"
#include <common/errutils.hpp>


AsioTimer::AsioTimer() :
  handle(0),
  timer(nullptr),
  timer_handler(nullptr)
{
}


AsioTimer::AsioTimer(uint64_t _handle, std::function<void (uint64_t)> _timer_handler) :
  handle(_handle),
  timer(nullptr),
  timer_handler(_timer_handler)
{
}


AsioTimer::~AsioTimer()
{
	timer.reset();
}


void AsioTimer::set_handler(uint64_t _handle, std::function<void(uint64_t)> _timer_handler)
{
	handle = _handle;
	timer_handler = _timer_handler;
}


void AsioTimer::assign_io_service(boost::asio::io_service & _io_service)
{
	timer.reset(new boost::asio::deadline_timer(_io_service));
}


void AsioTimer::detach_from_io_service()
{
	timer.reset();
}


uint64_t AsioTimer::get_handle() const
{
	return handle;
}


void AsioTimer::cancel()
{
	if (!timer)
		return;

	boost::system::error_code ec;
	timer->cancel(ec);
}


void AsioTimer::restart(int64_t _timeout_ms)
{
	af_assert(_timeout_ms >= 0);
	af_assert(timer);
	af_assert(timer_handler != nullptr);

	timer->expires_from_now(boost::posix_time::milliseconds(_timeout_ms));
	timer->async_wait(boost::bind(&AsioTimer::asio_timer_handler, this,
	                              boost::asio::placeholders::error));
}


void AsioTimer::asio_timer_handler(const boost::system::error_code & ec)
{
	if (ec.value() != 0)
		return;

	af_assert(timer_handler != nullptr);
	timer_handler(handle);
}
