#include "cached-vector.h"

#include <cstdio>

namespace aifil {

const int CACHED_VEC_MAX_CACHE = 512 * 1024 * 1024; //512 Mb
const int64_t CACHED_VEC_MAX_SIZE = 30LL * 1024 * 1024 * 1024; //30 Gb

int CacheManager::dump_counter = 0;
std::string CacheManager::cache_path = "./";

int CacheManager::cache_size()
{
	return cache_data.size() / element_size;
}

void CacheManager::cache_resize(int new_size)
{
	if (new_size * element_size != (int)cache_data.size())
		cache_data.resize(new_size * element_size);
}

CacheManager::CacheManager()
	: samples_num(0),
	  file(0), file_size(0), file_pos(0),
	  writing_pos(0), can_increase_file(true),
	  cache_pos_in_dump(0)
{
	cache_max_size = 200;
}

CacheManager::~CacheManager()
{
	clear(false);
}

void CacheManager::init(const std::string &name, int el_size_in_bytes, int existing_elements)
{
	clear(false);

	element_size = el_size_in_bytes;

	//no more than 512 Mb and or at least 1024 samples
	cache_max_size = std::min(512, CACHED_VEC_MAX_CACHE / element_size);
	++dump_counter;
	my_name = cache_path + name + stdprintf("-%d.tmpdump", dump_counter);

	if (existing_elements > 0)
	{
		file = fopen(my_name.c_str(), "ab+");
		af_assert(file && "cannot create cache"); //we MUST open file to work!
		if (fseek64(file, int64_t(existing_elements) * element_size, SEEK_CUR) == -1)
		{
			clear(false);
			except(0, "incorrect existing dump");
		}

		samples_num = existing_elements;
		file_size = int64_t(existing_elements) * element_size;
		file_pos = int64_t(existing_elements) * element_size;
		writing_pos = int64_t(existing_elements) * element_size;
	}
	else
	{
		file = fopen(my_name.c_str(), "wb+");
		af_assert(file && "cannot create cache"); //we MUST open file to work!
	}
}

void CacheManager::clear(bool reopen)
{
	cache_data.clear();
	cache_pos_in_dump = 0;

	samples_num = 0;
	file_size = 0;
	file_pos = 0;
	writing_pos = 0;
	can_increase_file = true;
	if (file)
	{
		fclose(file);
		file = 0;
		remove(my_name.c_str());
	}

	//so strange behaviour need to provide standard vector.clear() functionality
	if (!my_name.empty() && reopen)
	{
		file = fopen(my_name.c_str(), "wb+");
		af_assert(file && "cannot create cache"); //we MUST open file to work!
	}
}

void CacheManager::add(uint8_t *data)
{
	af_assert(file && "cache: add() without init()");

	if (writing_pos + element_size > CACHED_VEC_MAX_SIZE)
	{
		writing_pos = 0;
		can_increase_file = false;
	}

	if (file_pos != writing_pos)
	{
		fseek64(file, writing_pos, SEEK_SET);
		file_pos = writing_pos;
	}

	fwrite(data, element_size, 1, file);
	file_pos += element_size;
	writing_pos += element_size;
	if (can_increase_file)
	{
		samples_num += 1;
		file_size += element_size;
	}
}

const uint8_t* CacheManager::get_sample(int index)
{
	af_assert(file && "cache: get() without init()");

	if (index >= cache_pos_in_dump && index < cache_pos_in_dump + cache_size())
	{
		int stride = (index - cache_pos_in_dump) * element_size;
		return &cache_data[stride];
	}

	int64_t stride = int64_t(index) * element_size;
	af_assert(stride < file_size);
	if (stride != file_pos)
	{
		fseek64(file, stride, SEEK_SET);
		file_pos = stride;
	}

	int count = std::min(samples_num - index, cache_max_size);
	cache_resize(count);
	af_assert((int)fread(&(cache_data[0]), element_size, count, file) == count);
	cache_pos_in_dump = index;
	return &cache_data[0];
}

#ifdef HAVE_OPENCV
CachedVectorMat::CachedVectorMat()
	: patch_w(0), patch_h(0), channels(0), depth(0), cache(0)
{
}

CachedVectorMat::~CachedVectorMat()
{
	delete cache;
}

void CachedVectorMat::init(const std::string &name, int patch_w_, int patch_h_,
	int ch, int depth_, int existing_samples)
{
	if (cache)
		delete cache;

	patch_w = patch_w_;
	patch_h = patch_h_;
	channels = ch;
	depth = depth_;

	cache = new CacheManager;
	if (depth == CV_64F)
		cache->init(name, patch_w * patch_h * channels * sizeof(double), existing_samples);
	else if (depth == CV_32F)
		cache->init(name, patch_w * patch_h * channels * sizeof(float), existing_samples);
	else if (depth == CV_32S)
		cache->init(name, patch_w * patch_h * channels * sizeof(int), existing_samples);
	else if (depth == CV_16S || depth == CV_16U)
		cache->init(name, patch_w * patch_h * channels * sizeof(short), existing_samples);
	else if (depth == CV_8S || depth == CV_8U)
		cache->init(name, patch_w * patch_h * channels * sizeof(uint8_t), existing_samples);
	else
		af_assert(!"incorrect type cor cached vector of matrixes");
}

void CachedVectorMat::add(const cv::Mat &sample)
{
	af_assert(sample.cols == patch_w && sample.rows == patch_h &&
		sample.channels() == channels && sample.isContinuous());

	cache->add(sample.data);
}

const cv::Mat& CachedVectorMat::operator[](int index)
{
	cached_sample = cv::Mat(patch_h, patch_w, CV_MAKETYPE(depth, channels),
		(void*)cache->get_sample(index));
	return cached_sample;
}
#endif  // HAVE_OPENCV

}  // namespace aifil
