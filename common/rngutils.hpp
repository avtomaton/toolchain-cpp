#ifndef AIFIL_UTILS_RNG_H
#define AIFIL_UTILS_RNG_H

#include <limits>
#include <string>
#include <type_traits>

namespace aifil {

/**
 * Produce random value in specified range.
 * @param min [in] Lower boundary.
 * @param max [in] Upper boundary.
 * @return Random value between min and max inclusive.
*/
template<typename T>
typename std::enable_if<std::is_arithmetic<T>::value, T>::type
get_rand_value(T min = std::numeric_limits<T>::min(), T max = std::numeric_limits<T>::max());

template<typename T>
typename std::enable_if<std::is_same<
	std::string, typename std::remove_cv<T>::type>::value, std::string>::type
get_rand_value(std::size_t min_size = 1, std::size_t max_size = 30)
{
	std::string str;
	str.resize(get_rand_value<std::size_t>(min_size, max_size));

	for (char & ch : str)
		ch = get_rand_value<char>(char(33), char(123));

	return str;
};

} //namespace aifil

#endif // AIFIL_UTILS_RNG_H

