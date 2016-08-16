#ifndef AIFIL_TIMEUTILS_H
#define AIFIL_TIMEUTILS_H

#include <string>
#include <ctime>

namespace aifil
{

std::string sec2time(int full_sec, char stop_at = 'n');

// Возвращает текущее время в миллисекундах.
std::time_t get_current_time_ms();

} //namespace aifil

#endif //AIFIL_TIMEUTILS_H
