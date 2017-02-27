#ifndef AIFIL_TIMEUTILS_H
#define AIFIL_TIMEUTILS_H

#include <string>
#include <ctime>
#include <chrono>


namespace aifil
{

std::string sec2time(int full_sec, char stop_at = 'n');

std::time_t get_system_time_ms();

/**
 * @return Current local date and time as YYYY-MM-DD-hh:mm:ss
 */
std::string get_current_date_and_time();

/**
* Преобразует время в милисекундах в форматированную строку с датой и временем.
* @param ts [in] Время в милисекундах.
* @return Форматированная строка с датой и временем YYYY-MM-DD-hh:mm:ss.
*/
std::string date_time_from_ts(std::chrono::milliseconds ts);
	
#ifdef HAVE_BOOST
int64_t iso_to_unix_time(const std::string & iso_time);
int64_t get_current_utc_unix_time_ms();
int64_t get_current_local_unix_time_ms();
#endif
	

	
} //namespace aifil

#endif //AIFIL_TIMEUTILS_H
