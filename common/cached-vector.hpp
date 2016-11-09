#ifndef AIFIL_CACHED_VECTOR_H
#define AIFIL_CACHED_VECTOR_H

/* vector with HDD cache
 * implements size(), add(val), operator[] and clear()
 */

#include "compat-vc71.hpp"
#include "stringutils.hpp"
#include "errutils.hpp"

#include <stdint.h>
#include <vector>

#ifdef HAVE_OPENCV
#include <opencv2/core/core.hpp>
#endif

#if defined(_WIN32)
#define MARKUP_HUGEFILE
#ifdef _MSC_VER
#define fseek64 _fseeki64
#define ftell64 _ftelli64
#endif //MSC_VER
#elif defined(__APPLE__)
#define fseek64 fseeko
#define ftell64 ftello
#else
#define fseek64 fseeko64
#define ftell64 ftello64
#endif

namespace aifil {

struct CacheManager
{
	std::string my_name;
	int element_size;
	int samples_num;

	FILE *file;
	int64_t file_size; //in bytes
	int64_t file_pos; //in bytes
	int64_t writing_pos; //in bytes
	bool can_increase_file;

	static int dump_counter;
	static std::string cache_path;
	int cache_max_size; //in samples
	int cache_pos_in_dump; //in samples
	std::vector<uint8_t> cache_data;
	int cache_size();
	void cache_resize(int new_size);
	CacheManager();
	~CacheManager();

	void init(const std::string &name, int el_size_in_bytes, int existing_elements = 0);
	void clear(bool reopen = true);
	void add(uint8_t *data);
	const uint8_t* get_sample(int index);
};

//vector for simple types
template<class ValType>
struct CachedVector
{
	ValType cached_sample;
	CacheManager *cache;

	CachedVector() : cache(0) {}
	~CachedVector() { delete cache; }
	void init(const std::string &name)
	{
		af_assert(!cache && "cache file was already init");
		cache = new CacheManager;
		cache->init(name, sizeof(ValType));
	}
	void clear() { cache->clear(); }
	int size() { return cache ? cache->samples_num : 0; }
	void add(const ValType &sample)
	{
		af_assert(sizeof(ValType) == cache->element_size);
		cache->add((uint8_t*)(&sample));
	}

	const ValType& operator[](int index)
	{
		cached_sample = *(reinterpret_cast<const ValType*>(cache->get_sample(index)));
		return cached_sample;
	}
};

#ifdef HAVE_OPENCV
// vector for cv::Mat
struct CachedVectorMat
{
	int patch_w;
	int patch_h;
	int channels;
	int depth;

	cv::Mat cached_sample;
	CacheManager *cache;

	CachedVectorMat();
	~CachedVectorMat();
	void init(const std::string &name, int patch_w_, int patch_h_, int ch, int depth = CV_64F,
		int existing_samples = 0);
	void clear() { cache->clear(); }
	int size() { return cache ? cache->samples_num : 0; }
	void add(const cv::Mat &sample);
	const cv::Mat& operator[](int index);
};
#endif

}  //  namespace aifil

#endif // AIFIL_CACHED_VECTOR_H
