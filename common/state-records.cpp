#include "state-records.hpp"


MonParamType AbstractMonParam::get_type() const
{
	return MonParamType::UNDEFINED;
}


MonParamLevel AbstractMonParam::get_level() const
{
	return level;
}


void AbstractMonParam::increment(const int64_t)
{
	af_exception("Not implemented for this parameter type.");
}


void AbstractMonParam::set_int(const int64_t)
{
	af_exception("Not implemented for this parameter type.");
}


void AbstractMonParam::set_double(const double)
{
	af_exception("Not implemented for this parameter type.");
}


void AbstractMonParam::update()
{
	af_exception("Not implemented for this parameter type.");
}


MonParamType IntValueParam::get_type() const
{
	return MonParamType::INT_VALUE;
}


bool IntValueParam::any_threshold_reached() const
{
	if (is_null)
		return false;

	return ((value > high_threshold) || (value < low_threshold));
}


void IntValueParam::set_int(const int64_t val)
{
	value = val;
	is_null = false;
}


MonParamType DoubleValueParam::get_type() const
{
	return MonParamType::DOUBLE_VALUE;
}


bool DoubleValueParam::any_threshold_reached() const
{
	if (is_null)
		return false;

	return ((value > high_threshold) || (value < low_threshold));
}


void DoubleValueParam::set_double(const double val)
{
	value = val;
	is_null = false;
}


MonParamType CounterParam::get_type() const
{
	return MonParamType::COUNTER;
}


bool CounterParam::any_threshold_reached() const
{
	if (is_null)
		return false;

	return (value > high_threshold);
}


void CounterParam::increment(const int64_t step)
{
	value += step;
	is_null = false;
}


MonParamType UtcTimestampParam::get_type() const
{
	return MonParamType::UTC_TIMESTAMP;
}


bool UtcTimestampParam::any_threshold_reached() const
{
	if (is_null)
		return false;

	return (value > aifil::get_current_utc_unix_time_ms() + timeout);
}


void UtcTimestampParam::set_int(const int64_t val)
{
	value = val;
	is_null = false;
}


void UtcTimestampParam::update()
{
	value = aifil::get_current_utc_unix_time_ms();
	is_null = false;
}


std::shared_ptr<IntValueParam> MonParamBuilder::make_int_value_param(
    const MonParamLevel level,
    const MonParamDescriptor & descriptor)
{
	std::shared_ptr<IntValueParam> param = std::make_shared<IntValueParam>();
	param->code = descriptor.code;
	param->description = descriptor.description;
	param->human_readable_category = descriptor.human_readable_category;
	param->level = level;
	return param;
}


std::shared_ptr<IntValueParam> MonParamBuilder::make_int_value_param(
    const MonParamLevel level,
    const MonParamDescriptor & descriptor, const int64_t value)
{
	std::shared_ptr<IntValueParam> param = make_int_value_param(level, descriptor);
	param->is_null = false;
	param->value = value;
	return param;
}


std::shared_ptr<IntValueParam> MonParamBuilder::make_int_value_param(
    const MonParamLevel level,
    const MonParamDescriptor & descriptor, const int64_t value,
    const int64_t low_threshold, const int64_t high_threshold)
{
	std::shared_ptr<IntValueParam> param = make_int_value_param(level, descriptor, value);
	param->low_threshold = low_threshold;
	param->high_threshold = high_threshold;
	return param;
}


std::shared_ptr<DoubleValueParam> MonParamBuilder::make_double_value_param(
    const MonParamLevel level,
    const MonParamDescriptor & descriptor)
{
	std::shared_ptr<DoubleValueParam> param = std::make_shared<DoubleValueParam>();
	param->code = descriptor.code;
	param->description = descriptor.description;
	param->human_readable_category = descriptor.human_readable_category;
	param->level = level;
	return param;
}


std::shared_ptr<DoubleValueParam> MonParamBuilder::make_double_value_param(
    const MonParamLevel level,
    const MonParamDescriptor & descriptor, const double value)
{
	std::shared_ptr<DoubleValueParam> param = make_double_value_param(level, descriptor);
	param->is_null = false;
	param->value = value;
	return param;
}


std::shared_ptr<DoubleValueParam> MonParamBuilder::make_double_value_param(
    const MonParamLevel level,
    const MonParamDescriptor & descriptor, const double value,
    const double low_threshold, const double high_threshold)
{
	std::shared_ptr<DoubleValueParam> param = make_double_value_param(level, descriptor, value);
	param->low_threshold = low_threshold;
	param->high_threshold = high_threshold;
	return param;
}


std::shared_ptr<CounterParam> MonParamBuilder::make_counter_param(
    const MonParamLevel level,
    const MonParamDescriptor & descriptor)
{
	std::shared_ptr<CounterParam> param = std::make_shared<CounterParam>();
	param->code = descriptor.code;
	param->description = descriptor.description;
	param->human_readable_category = descriptor.human_readable_category;
	param->level = level;
	return param;
}


std::shared_ptr<CounterParam> MonParamBuilder::make_counter_param(
    const MonParamLevel level,
    const MonParamDescriptor & descriptor, const size_t value)
{
	std::shared_ptr<CounterParam> param = make_counter_param(level, descriptor);
	param->is_null = false;
	param->value = value;
	return param;
}


std::shared_ptr<CounterParam> MonParamBuilder::make_counter_param(
    const MonParamLevel level,
    const MonParamDescriptor & descriptor,
    const size_t value, const size_t high_threshold)
{
	std::shared_ptr<CounterParam> param = make_counter_param(level, descriptor, value);
	param->high_threshold = high_threshold;
	return param;
}


std::shared_ptr<UtcTimestampParam> MonParamBuilder::make_utc_timestamp_param(
    const MonParamLevel level,
    const MonParamDescriptor & descriptor)
{
	std::shared_ptr<UtcTimestampParam> param = std::make_shared<UtcTimestampParam>();
	param->code = descriptor.code;
	param->description = descriptor.description;
	param->human_readable_category = descriptor.human_readable_category;
	param->level = level;
	return param;
}


std::shared_ptr<UtcTimestampParam> MonParamBuilder::make_utc_timestamp_param(
    const MonParamLevel level,
    const MonParamDescriptor & descriptor, const int64_t value)
{
	std::shared_ptr<UtcTimestampParam> param = make_utc_timestamp_param(level, descriptor);
	param->is_null = false;
	param->value = value;
	return param;
}


std::shared_ptr<UtcTimestampParam> MonParamBuilder::make_utc_timestamp_param(
    const MonParamLevel level,
    const MonParamDescriptor & descriptor,
    const int64_t value, const int64_t timeout)
{
	std::shared_ptr<UtcTimestampParam> param = make_utc_timestamp_param(level, descriptor, value);
	param->timeout = timeout;
	return param;
}
