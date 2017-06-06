#include "rngutils.hpp"

#include <random>

namespace aifil {

static std::random_device current_random_device;
static std::mt19937 rng(current_random_device());

template<typename T>
T get_rand_int(T min, T max)
{
	if (min == max)
		return min;
	if (min > max)
		std::swap(min, max);

	std::uniform_int_distribution<T> dist(min, max);

	return dist(rng);
}

template<typename T>
T get_rand_real(T min, T max)
{
	if (min == max)
		return min;
	if (min > max)
		std::swap(min, max);

	std::uniform_real_distribution<T> dist(min, max);

	return dist(rng);
}

template<>
int get_rand_value(int min, int max)
{
	return get_rand_int<int>(min, max);
}

template<>
int64_t get_rand_value(int64_t min, int64_t max)
{
	return get_rand_int<int64_t>(min, max);
}

template<>
size_t get_rand_value(size_t min, size_t max)
{
	return get_rand_int<size_t>(min, max);
}

template<>
char get_rand_value(char min, char max)
{
	return get_rand_int<char>(min, max);
}

template<>
uint8_t get_rand_value(uint8_t min, uint8_t max)
{
	return get_rand_int<uint8_t>(min, max);
}

template<>
bool get_rand_value(bool, bool)
{
	return static_cast<bool>(get_rand_int<int>(0, 1));
}

template<>
float get_rand_value(float min, float max)
{
	return get_rand_real<float>(min, max);
}

template<>
double get_rand_value(double min, double max)
{
	return get_rand_real<double>(min, max);
}

// explicit instantiations
//template char get_rand_value<char>(char min, char max);
//template unsigned char get_rand_value<unsigned char>(unsigned char min, unsigned char max);
//template int get_rand_value<int>(int min, int max);
//template int64_t get_rand_value<int64_t>(int64_t min, int64_t max);
//template size_t get_rand_value<size_t>(size_t min, size_t max);
//template float get_rand_value<float>(float min, float max);
//template double get_rand_value<double>(double min, double max);

} //namespace aifil
