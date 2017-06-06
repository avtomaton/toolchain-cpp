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

/**
 * @brief Парсит строковое представление даты-времени в utc метку времени.
 *
 * Примеры использования:
 * @code{.cpp}
 * parse_datetime("2017-04-09 12:00:00", ts);// true, ts = 1491739200000, 09 Apr 2017 12:00:00 GMT
 * parse_datetime("2017-04-09 12:00:00+05", ts); // ts = 1491721200000, 09 Apr 2017 07:00:00 GMT
 * parse_datetime("2017-04-09T12:00:00+05", ts); // ture, ts = 1491721200000, 09 Apr 2017 07:00:00 GMT
 * parse_datetime("2017-04-09 12:00:00-03:30", ts); // true, ts = 1491751800000, 09 Apr 2017 15:30:00 GMT
 * parse_datetime("2017-04-09 12:00:00+130", ts); // false, ts = 0
 * parse_datetime("2017-04-09 12:00:00+0130", ts); // false, ts = 0
 * @endcode
 *
 * @param datetime [in] строковое представление даты-времени
 * @param utc [out] метка времени utc в миллисекундах
 * @param format [in] пользовательский формат даты-времени (опционально)
 * по умолчанию -- 2000-01-01 12:00:00+00
 * см. подробнее в boost::local_time::local_time_input_facet
 *
 * @return true -- если парсинг прошёл успешно, false -- если нет
 */
bool parse_datetime(
		const std::string &datetime,
		int64_t &utc,
		const std::string &format = "%Y-%m-%d %H:%M:%S%F %ZP");
#endif
	

	
} //namespace aifil

#endif //AIFIL_TIMEUTILS_H
