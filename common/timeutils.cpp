#include "timeutils.hpp"

#include "stringutils.hpp"

#include <chrono>

#ifdef HAVE_BOOST
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>
#include <boost/date_time.hpp>
#endif

namespace aifil
{

std::string sec2time(int full_sec, char stop_at)
{
	std::string res;
	int sec = full_sec;
	int days = sec / 86400;
	sec -= days * 86400;
	if (days || stop_at == 'd')
		res += stdprintf("%dd", days);
	if (stop_at == 'd')
		return res;

	int hours = sec / 3600;
	sec -= hours * 3600;
	if (hours || (res.empty() && stop_at == 'h'))
		res += stdprintf("%dh", hours);
	if (stop_at == 'h')
		return res;

	int min = sec / 60;
	sec -= min * 60;
	if (min || (res.empty() && stop_at == 'm'))
		res += stdprintf("%dm", min);
	if (stop_at == 'm')
		return res;

	if (sec || res.empty())
		res += stdprintf("%ds", sec);

	return res;
}

std::time_t get_system_time_ms()
{
	return std::chrono::duration_cast<std::chrono::milliseconds>
		(std::chrono::system_clock::now().time_since_epoch()).count();
}

#ifdef HAVE_BOOST
int64_t iso_to_unix_time(const std::string & iso_time)
{
	std::string time_string = iso_time;
	size_t pos = time_string.find(' ');

	if (pos > 0)
		time_string[pos] = 'T';

	boost::posix_time::ptime time_stamp_iso =
	    boost::date_time::parse_delimited_time<boost::posix_time::ptime>(time_string, 'T');
	boost::posix_time::ptime unix_epoch(boost::gregorian::date(1970, 1, 1));
	boost::posix_time::time_duration delta = time_stamp_iso - unix_epoch;

	return delta.total_milliseconds();
}


int64_t get_current_utc_unix_time_ms()
{
	using boost::gregorian::date;
	using boost::posix_time::ptime;
	using boost::posix_time::microsec_clock;

	ptime const epoch(date(1970, 1, 1));

	return (microsec_clock::universal_time() - epoch).total_milliseconds();
}


int64_t get_current_local_unix_time_ms()
{
	using boost::gregorian::date;
	using boost::posix_time::ptime;
	using boost::posix_time::microsec_clock;

	ptime const epoch(date(1970, 1, 1));

	return (microsec_clock::local_time() - epoch).total_milliseconds();
}
#endif  // HAVE_BOOST

} //namespace aifil
