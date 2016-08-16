#include "timeutils.h"

#include <chrono>
#include "stringutils.h"


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


std::time_t get_current_time_ms()
{
	return (std::chrono::system_clock::now().time_since_epoch() /
	        std::chrono::milliseconds(1));
}

} //namespace aifil
