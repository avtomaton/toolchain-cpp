#include "profiler.h"

#include "stringutils.h"

#include <stdint.h>
#include <cstdio>

#ifdef _WIN32
#include <windows.h>
#else
#include <ctime>
#include <sys/time.h>
#endif

namespace aifil {

static uint64_t timer()
{
	uint64_t r;
#ifdef _WIN32
	QueryPerformanceCounter((LARGE_INTEGER*) &r);
#else
	struct timeval tv;
	gettimeofday(&tv, 0);
	r = uint64_t( tv.tv_sec )*1000000 + tv.tv_usec;
#endif
	return r;
}

MeasureElapsedTime::MeasureElapsedTime()
{
	restart();
}

void MeasureElapsedTime::restart()
{
	time_before = timer();
}

double MeasureElapsedTime::elapsed()
{
	uint64_t persec;
#ifdef _WIN32
	QueryPerformanceFrequency((LARGE_INTEGER*) &persec);
#else
	persec = 1000000;
#endif
	return double(timer() - time_before) / persec * 1000;
}

ProfilerAccumulator::ProfilerAccumulator(const std::string &key):
	start_(timer()),
	key(key)
{
}

ProfilerAccumulator::~ProfilerAccumulator()
{
	if (start_)
		Profiler::instance().update(key, start_, timer() - start_);
	start_ = 0;
}

void ProfilerStat::update(uint64_t t)
{
	if (min_time > t) min_time = t;
	if (max_time < t) max_time = t;
	sum_time += t;
	++count;
}

void Profiler::update(const std::string &key, uint64_t start, uint64_t t)
{
	std::unique_lock<std::mutex> lock(storage_mutex);
	std::map<std::string, ProfilerStat>::iterator it = storage.find(key);
	if (it == storage.end())
		storage_order[start] = key;
	storage[key].update(t);
}

void Profiler::clear()
{
	std::unique_lock<std::mutex> lock(storage_mutex);
	storage.clear();
}

std::string Profiler::print_statistics()
{
	uint64_t persec;
#ifdef _WIN32
	QueryPerformanceFrequency((LARGE_INTEGER*) &persec);
#else
	persec = 1000000;
#endif
	std::string r;
	int count = 1;
	for (std::map<uint64_t, std::string>::iterator it = storage_order.begin();
		it != storage_order.end(); ++it)
	{
		const ProfilerStat &my_stat = storage[it->second];
		r += stdprintf("tavg %7.3lf, tmin %7.3lf, tmax %7.3lf, tsum %15.3lf -- %4i calls, %d. %s\n",
				(double(my_stat.sum_time / my_stat.count) / persec) * 1000.0,
				(double(my_stat.min_time) / persec) * 1000.0,
				(double(my_stat.max_time) / persec) * 1000.0,
				(double(my_stat.sum_time) / persec) * 1000.0,
				my_stat.count,
				count++,
				it->second.c_str()
			);
	}
	return r;
}

} //namespace aifil

