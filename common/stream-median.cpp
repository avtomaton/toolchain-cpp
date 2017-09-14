#include "stream-median.hpp"
#include "errutils.hpp"


IntStreamMedian::IntStreamMedian(const size_t _set_size) :
  set_size(_set_size)
{
	if (set_size < MIN_AVAILABLE_SET_SIZE)
		af_exception("The size of the set is too small.");
}


IntStreamMedian::~IntStreamMedian()
{
}


void IntStreamMedian::push(const int64_t num)
{
	nums.insert(num);

	size_t n = nums.size() / 2;
	auto begin = nums.begin();
	auto end = nums.end();
	auto mid = begin;
	std::advance(mid, n);

	if (mid == end)
		mid = begin;

	median = *mid;

	if (nums.size() >= set_size)
	{
		end--;
		nums.erase(end);
		nums.erase(nums.begin());
	}
}


int64_t IntStreamMedian::get_median() const
{
	return median;
}


void IntStreamMedian::reset()
{
	nums.clear();
	median = 0;
}


// FIXME: Arrr. Test me help, or optimize. This is madness.
void IntStreamMedian::set_nums_set_size(const size_t _set_size)
{
	size_t prev_set_size = set_size;
	set_size = _set_size;

	if (set_size < MIN_AVAILABLE_SET_SIZE)
		af_exception("The size of the set is too small.");

	if (set_size >= prev_set_size)
		return;

	size_t chomp_size = (prev_set_size - set_size) / 2;

	for (size_t j = 0; j < chomp_size; j++)
		nums.erase(nums.begin());

	chomp_size = prev_set_size - set_size - chomp_size;

	for (size_t j = 0; j < chomp_size; j++)
		nums.erase(--nums.end());
}
