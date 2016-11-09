#ifndef AIFIL_TIMEUTILS_H
#define AIFIL_TIMEUTILS_H

#include <string>
#include <ctime>


namespace aifil
{

std::string sec2time(int full_sec, char stop_at = 'n');

std::time_t get_system_time_ms();

#ifdef HAVE_BOOST
int64_t iso_to_unix_time(const std::string & iso_time);
int64_t get_current_utc_unix_time_ms();
int64_t get_current_local_unix_time_ms();
#endif

} //namespace aifil

#endif //AIFIL_TIMEUTILS_H
