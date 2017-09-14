#ifndef ID_GENERATOR_HPP
#define ID_GENERATOR_HPP

#include <cstddef>
#include <atomic>
#include <string>
#include <sstream>
#include <mutex>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>


class IdGenerator
{
	static constexpr int64_t START_ID = 10000000000;

public:
	static int64_t new_id()
	{
		static volatile std::atomic_int_least64_t counter(START_ID);
		static std::mutex counter_mutex;

		std::lock_guard<std::mutex> lock(counter_mutex);
		return counter++;
	}

	static uint64_t new_id_uint()
	{
		return static_cast<uint64_t>(new_id());
	}

	static std::string new_uuid()
	{
		boost::uuids::uuid uuid = boost::uuids::random_generator()();
		std::stringstream ss;
		ss << uuid;
		return ss.str();
	}
};

#endif // ID_GENERATOR_HPP
