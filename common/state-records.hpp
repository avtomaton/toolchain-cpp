#ifndef STATE_RECORDS_HPP
#define STATE_RECORDS_HPP

#include <climits>
#include <cstdint>
#include <cmath>
#include <string>
#include <memory>
#include <unordered_map>

#include <common/errutils.hpp>
#include <common/timeutils.hpp>

#include "state-events.hpp"


enum class MonParamType { UNDEFINED, COUNTER, INT_VALUE, DOUBLE_VALUE, UTC_TIMESTAMP };
enum class MonParamLevel { COMMON, WARNING, FAIL };


struct MonParamDescriptor
{
	MonParamDescriptor(const uint64_t _code,
	                   const MonParamType _type,
	                   const std::string & _human_readable_category,
	                   const std::string & _description) :
	  code(_code),
	  type(_type),
	  human_readable_category(_human_readable_category),
	  description(_description) {}

	uint64_t code = 0;
	MonParamType type;
	std::string human_readable_category;
	std::string description;
};


namespace Diagnostic
{
// _code = The Current Unix Timestamp from https://www.unixtimestamp.com
const MonParamDescriptor EXAMPLE_MON_PARAM (1504705707, MonParamType::COUNTER,
                                            "example", "Example counter");
}


class AbstractMonParam
{
public:
	virtual ~AbstractMonParam() {}

public:
	virtual MonParamType get_type() const;

	virtual MonParamLevel get_level() const;

	virtual bool any_threshold_reached() const = 0;

	virtual void increment(const int64_t);

	virtual void set_int(const int64_t);

	virtual void set_double(const double);

	virtual void update();

public:
	bool is_null = true;
	uint64_t code = 0;
	std::string description;
	std::string human_readable_category;
	MonParamLevel level = MonParamLevel::COMMON;
};


class IntValueParam : public AbstractMonParam
{
public:
	~IntValueParam() override {}

public:
	virtual MonParamType get_type() const override;

	virtual bool any_threshold_reached() const override;

	virtual void set_int(const int64_t val) override;

public:
	int64_t value = 0;
	int64_t low_threshold = std::numeric_limits<int64_t>::min();
	int64_t high_threshold = std::numeric_limits<int64_t>::max();
};


class DoubleValueParam : public AbstractMonParam
{
public:
	~DoubleValueParam() override {}

public:
	virtual MonParamType get_type() const override;

	virtual bool any_threshold_reached() const override;

	virtual void set_double(const double val) override;

public:
	double value = 0;
	double low_threshold = std::numeric_limits<double>::min();
	double high_threshold = std::numeric_limits<double>::max();
};


class CounterParam : public AbstractMonParam
{
public:
	~CounterParam() override {}

public:
	virtual MonParamType get_type() const override;

	virtual bool any_threshold_reached() const override;

	virtual void increment(const int64_t step) override;

public:
	size_t value = 0;
	size_t high_threshold = std::numeric_limits<size_t>::max();
};


class UtcTimestampParam : public AbstractMonParam
{
public:
	~UtcTimestampParam() override {}

public:
	virtual MonParamType get_type() const override;

	virtual bool any_threshold_reached() const override;

	virtual void set_int(const int64_t val) override;

	virtual void update() override;

public:
	int64_t value = 0;
	int64_t timeout = std::numeric_limits<int64_t>::max();
};


class MonParamBuilder
{
public:
	static std::shared_ptr<IntValueParam> make_int_value_param(
	    const MonParamLevel level,
	    const MonParamDescriptor & descriptor);

	static std::shared_ptr<IntValueParam> make_int_value_param(
	    const MonParamLevel level,
	    const MonParamDescriptor & descriptor,
	    const int64_t value);

	static  std::shared_ptr<IntValueParam> make_int_value_param(
	    const MonParamLevel level,
	    const MonParamDescriptor & descriptor,
	    const int64_t value,
	    const int64_t low_threshold,
	    const int64_t high_threshold);

	static  std::shared_ptr<DoubleValueParam> make_double_value_param(
	    const MonParamLevel level,
	    const MonParamDescriptor & descriptor);

	static std::shared_ptr<DoubleValueParam> make_double_value_param(
	    const MonParamLevel level,
	    const MonParamDescriptor & descriptor,
	    const double value);

	static std::shared_ptr<DoubleValueParam> make_double_value_param(
	    const MonParamLevel level,
	    const MonParamDescriptor & descriptor,
	    const double value,
	    const double low_threshold,
	    const double high_threshold);

	static std::shared_ptr<CounterParam> make_counter_param(
	    const MonParamLevel level,
	    const MonParamDescriptor & descriptor);

	static std::shared_ptr<CounterParam> make_counter_param(
	    const MonParamLevel level,
	    const MonParamDescriptor & descriptor,
	    const size_t value);

	static std::shared_ptr<CounterParam> make_counter_param(
	    const MonParamLevel level,
	    const MonParamDescriptor & descriptor,
	    const size_t value,
	    const size_t high_threshold);

	static std::shared_ptr<UtcTimestampParam> make_utc_timestamp_param(
	    const MonParamLevel level,
	    const MonParamDescriptor & descriptor);

	static std::shared_ptr<UtcTimestampParam> make_utc_timestamp_param(
	    const MonParamLevel level,
	    const MonParamDescriptor & descriptor,
	    const int64_t value);

	static std::shared_ptr<UtcTimestampParam> make_utc_timestamp_param(
	    const MonParamLevel level,
	    const MonParamDescriptor & descriptor,
	    const int64_t value,
	    const int64_t timeout);
};


class MonitoredRecords
{
public:
	typedef std::unordered_map<uint64_t, std::shared_ptr<AbstractMonParam>> RecordsType;

public:
	virtual ~MonitoredRecords() {}

public:
	virtual void add_parameter(const std::shared_ptr<AbstractMonParam> param)
	{
		records.insert(std::make_pair(param->code, param));
	}

	virtual void clear()
	{
		records.clear();
	}

	virtual void increment(const MonParamDescriptor & descriptor, const int64_t step)
	{
		std::shared_ptr<AbstractMonParam> param = find_param(descriptor.code);
		af_assert(param);
		param->increment(step);
	}

	virtual void set_int(const MonParamDescriptor & descriptor, const int64_t value)
	{
		std::shared_ptr<AbstractMonParam> param = find_param(descriptor.code);
		af_assert(param);
		param->set_int(value);
	}

	virtual void set_double(const MonParamDescriptor & descriptor, const double value)
	{
		std::shared_ptr<AbstractMonParam> param = find_param(descriptor.code);
		af_assert(param);
		param->set_double(value);
	}

	virtual void update(const MonParamDescriptor & descriptor)
	{
		std::shared_ptr<AbstractMonParam> param = find_param(descriptor.code);
		af_assert(param);
		param->update();
	}

	virtual std::shared_ptr<AbstractMonParam> find_param(const uint64_t code)
	{
		std::shared_ptr<AbstractMonParam> result;
		const auto p = records.find(code);

		if (p != records.end())
			result = p->second;

		return result;
	}

public:
	RecordsType records;
};


class Monitorable
{
public:
	virtual ~Monitorable() {}

	//public:

	//	virtual void get_state_records(MonitoredRecords & records) const
	//	{
	//		std::lock_guard<std::mutex> lock(state_mutex);
	//		state_records.clone_to(records);
	//	}


	//	virtual bool check_states_thresholds_ok() const
	//	{
	//		std::lock_guard<std::mutex> lock(state_mutex);
	//		return state_records.check_thresholds_ok();
	//	}

	//protected:

	//	virtual void add_monitored_state(const IntegerParameter & record)
	//	{
	//		std::lock_guard<std::mutex> lock(state_mutex);
	//		state_records.add_record(record);
	//	}


	//	virtual void add_monitored_state(const DoubleParameter & record)
	//	{
	//		std::lock_guard<std::mutex> lock(state_mutex);
	//		state_records.add_record(record);
	//	}


	//	virtual bool accept_monitored_state(const uint64_t code)
	//	{
	//		std::lock_guard<std::mutex> lock(state_mutex);
	//		return state_records.accept_state(code);
	//	}


	//	virtual bool accept_monitored_state(const uint64_t code, const int64_t value)
	//	{
	//		std::lock_guard<std::mutex> lock(state_mutex);
	//		return state_records.accept_state(code, value);
	//	}


	//	virtual bool accept_monitored_state(const uint64_t code, const double value)
	//	{
	//		std::lock_guard<std::mutex> lock(state_mutex);
	//		return state_records.accept_state(code, value);
	//	}


	//	virtual void clear_monitor_table()
	//	{
	//		std::lock_guard<std::mutex> lock(state_mutex);
	//		state_records.clear();
	//	}

	virtual void increment(const MonParamDescriptor & descriptor, const int64_t step = 1)
	{
		std::lock_guard<std::mutex> lock(records_mutex);
		records.increment(descriptor, step);
	}

	virtual void set_int(const MonParamDescriptor & descriptor, const int64_t value)
	{
		std::lock_guard<std::mutex> lock(records_mutex);
		records.set_int(descriptor, value);
	}

	virtual void set_double(const MonParamDescriptor & descriptor, const double value)
	{
		std::lock_guard<std::mutex> lock(records_mutex);
		records.set_double(descriptor, value);
	}

	virtual void update(const MonParamDescriptor & descriptor)
	{
		std::lock_guard<std::mutex> lock(records_mutex);
		records.update(descriptor);
	}

	virtual void clear()
	{
		std::lock_guard<std::mutex> lock(records_mutex);
		records.clear();
	}

	virtual void add_parameter(const std::shared_ptr<AbstractMonParam> param)
	{
		records.add_parameter(param);
	}

private:
	mutable std::mutex records_mutex;
	MonitoredRecords records;
};


#endif // STATE_RECORDS_HPP
