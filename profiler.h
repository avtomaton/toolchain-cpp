#ifndef AIFIL_UTILS_PROFILER_H
#define AIFIL_UTILS_PROFILER_H

#include <limits>
#include <map>
#include <mutex>
#include <stdint.h>
#include <string>

namespace aifil {

template<typename T>
class singleton
{
protected:
	singleton() {}
	~singleton() {}

public:
	static T& instance();
};

template<class T> T& singleton<T>::instance()
{
	static T t;
	return t;
}

struct ProfilerStat
{
	int index;
	uint64_t min_time;
	uint64_t max_time;
	uint64_t sum_time;
	int count;

	ProfilerStat(): index(0),
		min_time(std::numeric_limits<uint64_t>::max()), max_time(0),
		sum_time(0), count(0) {}
	void update(uint64_t t);
};

class Profiler: public singleton<Profiler>
{
public:
	std::string print_statistics();
	void update(const std::string &key, uint64_t start, uint64_t t);
	void clear();

private:
	std::mutex storage_mutex;
	std::map<std::string, ProfilerStat> storage;
	std::map<uint64_t, std::string> storage_order;
	friend class ProfilerAccumulator;
};

class ProfilerAccumulator
{
	uint64_t start_;
	std::string key;

public:
	ProfilerAccumulator(const std::string &key);
	~ProfilerAccumulator();
};

#ifdef USE_PROFILER
#define PROFILE(key) ProfilerAccumulator profiler__(key);
#else
#define PROFILE(key)
#endif

class MeasureElapsedTime {
	uint64_t time_before;
public:
	MeasureElapsedTime();
	void restart();
	double elapsed(); // ms
};

} //namespace aifil

#endif // AIFIL_UTILS_PROFILER_H
