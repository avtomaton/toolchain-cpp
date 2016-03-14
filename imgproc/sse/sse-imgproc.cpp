//SIMD-optimized functions
//http://software.intel.com/en-us/node/460796

//for AVX compile with -march=corei7 or -arch=AVX

//#include <pmmintrin.h> //SSE3
//#include <tmmintrin.h> //SSSE3
//#include <smmintrin.h> //SSE4.1
//#include <nmmintrin.h> //SSE4.2
//#include <ammintrin.h> //SSE4A
//#include <immintrin.h> //AVX and post-32nm
//#include <zmmintrin.h> //AVX-512

#include <malloc.h>
#include <string.h> //memset

#if defined(_MSC_VER) && _MSC_VER <= 1310
#define UGLY_VC71
#include <mmintrin.h> //MMX
#include <xmmintrin.h> //SSE
#include <emmintrin.h> //SSE2
typedef unsigned uint32_t;
typedef unsigned char uint8_t;
#define _mm_castps_si128(x) (__m128i)(x)
#define _mm_castsi128_ps(x) (__m128)(x)
static void inline cpuid(int cpuinfo[4], int info_type)
{
    __asm
    {
        push   ebx
        push   esi

        mov    esi, cpuinfo
        mov    eax, info_type
        cpuid
        mov    dword ptr [esi], eax
        mov    dword ptr [esi+4], ebx
        mov    dword ptr [esi+8], ecx
        mov    dword ptr [esi+0Ch], edx

        pop    esi
        pop    ebx
    }
}
#else //not VC 7.1
#include <stdint.h>
#endif

#if defined(_MSC_VER) && _MSC_VER > 1310
#define cpuid __cpuid
#include <intrin.h> //get CPUID capability and all other headers
#endif

#ifdef _MSC_VER //VVRR
//MSVC
#define ALIGN16(a) _MM_ALIGN16 a
#define aligned_malloc _aligned_malloc
#define aligned_free _aligned_free
#else //not MSVC
#include <x86intrin.h> //all intrinsic functions
#define aligned_malloc _mm_malloc
#define aligned_free _mm_free
void cpuid(int cpu_info[4], int info_type)
{
    __asm__ __volatile__ (
        "cpuid"
		: "=a" (cpu_info[0]), "=b" (cpu_info[1]), "=c" (cpu_info[2]), "=d" (cpu_info[3])
		: "a" (info_type)
    );
}
#define ALIGN16(a)	a __attribute__ ((aligned (16)))
#endif

#ifdef __GNUC__ //version
//gcc-specific code
#endif

#ifdef __INTEL_COMPILER //VRP - version, revision, patch
//ICC-specific code
#endif

#define M_PIf 3.141592653589793238462643f

struct SSECapabilities
{
	bool x64;
	bool mmx;
	bool sse;
	bool sse2;
	bool sse3;
	bool ssse3;
	bool sse41;
	bool sse42;
	bool sse4a;
	bool avx;
	bool xop;
	bool fma3;
	bool fma4;

	bool checked;

	SSECapabilities()
	{
		checked = false;

		x64 = false;
		mmx = false;
		sse = false;
		sse2 = false;
		sse3 = false;
		ssse3 = false;
		sse41 = false;
		sse42 = false;
		sse4a = false;
		avx = false;
		xop = false;
		fma3 = false;
		fma4 = false;
	}

	void check()
	{
		if (checked)
			return;

		checked = true;
		int info[4];
		cpuid(info, 0);
		int n_ids = info[0];
		cpuid(info, 0x80000000);
		int n_ext_ids = info[0];

		if (n_ids >= 1)
		{
			cpuid(info, 0x00000001);
			mmx   = !!(info[3] & (1 << 23));
			sse   = !!(info[3] & (1 << 25));
			sse2  = !!(info[3] & (1 << 26));
			sse3  = !!(info[2] & 1);
			ssse3 = !!(info[2] & (1 <<  9));
			sse41 = !!(info[2] & (1 << 19));
			sse42 = !!(info[2] & (1 << 20));

			avx   = !!(info[2] & (1 << 28));
			fma3  = !!(info[2] & (1 << 12));
		}

		if (n_ext_ids >= 0x80000001)
		{
			cpuid(info, 0x80000001);
			x64   = !!(info[3] & (1 << 29));
			sse4a = !!(info[2] & (1 <<  6));
			fma4  = !!(info[2] & (1 << 16));
			xop   = !!(info[2] & (1 << 11));
		}

	}
};

static SSECapabilities sse_capabilities;

void sse_sqrt_sum_of_squares(float* src_x, float* src_y, float* dst, uint32_t count)
{
	//dst aligned?

	for (uint32_t i = 0; i < count; i += 4)
	{
		__m128 x = _mm_loadu_ps(src_x + i);
		__m128 y = _mm_loadu_ps(src_y + i);

		x = _mm_add_ps(_mm_mul_ps(x, x), _mm_mul_ps(y, y));
        x = _mm_sqrt_ps(x);
		_mm_storeu_ps(dst + i, x);
		//_mm_store_ps(dst + i, x);
	}
}

void sse_subs_180(float* pangle, uint32_t count)
{
	__m128 pi = _mm_set1_ps(180.0f);
	for (uint32_t i = 0; i < count; i += 4)
	{
		__m128 a = _mm_loadu_ps(pangle + i);
		__m128 greater = _mm_cmpge_ps(a, pi);
		__m128 pi_masked = _mm_and_ps(pi, greater);
		__m128 res = _mm_sub_ps(a, pi_masked);
		_mm_storeu_ps(pangle + i, res);
	}
}

void sse_atan2_0_180(float *src_x, float *src_y, float *angle, int count)
{
	//TODO: rs_assert(aligned)

	//from openCV atan2
	static const float atan2_p1 = 0.9997878412794807f * (float)(180.0f / M_PIf);
	static const float atan2_p3 = -0.3258083974640975f * (float)(180.0f / M_PIf);
	static const float atan2_p5 = 0.1555786518463281f * (float)(180.0f / M_PIf);
	static const float atan2_p7 = -0.04432655554792128f * (float)(180.0f / M_PIf);

	union { int i; float fl; } iabsmask;
	iabsmask.i = 0x7fffffff;
	__m128 eps = _mm_set1_ps((float)1e-6);
	__m128 absmask = _mm_set1_ps(iabsmask.fl);
	__m128 _90 = _mm_set1_ps(90.0f);
	__m128 _180 = _mm_set1_ps(180.0f);
	__m128 _360 = _mm_set1_ps(360.0f);
	__m128 zero = _mm_setzero_ps();
	__m128 p1 = _mm_set1_ps(atan2_p1);
	__m128 p3 = _mm_set1_ps(atan2_p3);
    __m128 p5 = _mm_set1_ps(atan2_p5);
	__m128 p7 = _mm_set1_ps(atan2_p7);

	for(int i = 0; i < count; i += 4)
	{
		__m128 x = _mm_loadu_ps(src_x + i);
		__m128 y = _mm_loadu_ps(src_y + i);
		//__m128 x = _mm_load_ps(src_x + i);
		//__m128 y = _mm_load_ps(src_y + i);
		__m128 ax = _mm_and_ps(x, absmask);
		__m128 ay = _mm_and_ps(y, absmask);
		__m128 mask = _mm_cmplt_ps(ax, ay);
		__m128 tmin = _mm_min_ps(ax, ay);
		__m128 tmax = _mm_max_ps(ax, ay);
		__m128 c = _mm_div_ps(tmin, _mm_add_ps(tmax, eps));
		__m128 c2 = _mm_mul_ps(c, c);
		__m128 a = _mm_mul_ps(c2, p7);
		a = _mm_mul_ps(_mm_add_ps(a, p5), c2);
		a = _mm_mul_ps(_mm_add_ps(a, p3), c2);
		a = _mm_mul_ps(_mm_add_ps(a, p1), c);

		__m128 b = _mm_sub_ps(_90, a);
		a = _mm_xor_ps(a, _mm_and_ps(_mm_xor_ps(a, b), mask));

		b = _mm_sub_ps(_180, a);
		mask = _mm_cmplt_ps(x, zero);
		a = _mm_xor_ps(a, _mm_and_ps(_mm_xor_ps(a, b), mask));

		b = _mm_sub_ps(_360, a);
		mask = _mm_cmplt_ps(y, zero);
		a = _mm_xor_ps(a, _mm_and_ps(_mm_xor_ps(a, b), mask));

		__m128 pi_masked = _mm_and_ps(_180, _mm_cmpge_ps(a, _180));
		a = _mm_sub_ps(a, pi_masked);

		_mm_storeu_ps(angle + i, a);
	}
}

void sse_max_diff_mag_c3(const float *src_x, const float *src_y, const float *src_mag,
	float *dst_x, float *dst_y, float *dst_mag, int count)
{
	for (int i = 0; i < count - 12; i += 12)
	{
		//set uses reverse order!
		//_mm_prefetch((const char*)(src_mag + i), _MM_HINT_NTA);
		__m128 ch0 = _mm_set_ps(src_mag[i + 9], src_mag[i + 6], src_mag[i + 3], src_mag[i + 0]);
		__m128 ch1 = _mm_set_ps(src_mag[i + 10], src_mag[i + 7], src_mag[i + 4], src_mag[i + 1]);
		__m128 ch2 = _mm_set_ps(src_mag[i + 11], src_mag[i + 8], src_mag[i + 5], src_mag[i + 2]);

		__m128 max_mag = _mm_max_ps(ch0, ch1);
		max_mag = _mm_max_ps(max_mag, ch2);
		_mm_storeu_ps(&dst_mag[i / 3], max_mag);

		__m128 ch0_mask = _mm_cmpeq_ps(max_mag, ch0);
		__m128 ch1_mask = _mm_cmpeq_ps(max_mag, ch1);
		__m128 ch2_mask = _mm_cmpeq_ps(max_mag, ch2);

		//_mm_prefetch((const char*)(src_x + i), _MM_HINT_NTA);
		__m128 ch_div_tmp = _mm_set_ps(src_x[i + 9], src_x[i + 6], src_x[i + 3], src_x[i + 0]);
		ch_div_tmp = _mm_and_ps(ch_div_tmp, ch0_mask);
		__m128 ch_div_max = _mm_set_ps(src_x[i + 10], src_x[i + 7], src_x[i + 4], src_x[i + 1]);
		ch_div_max = _mm_and_ps(ch_div_max, ch1_mask);
		ch_div_max = _mm_or_ps(ch_div_tmp, ch_div_max);
		ch_div_tmp = _mm_set_ps(src_x[i + 11], src_x[i + 8], src_x[i + 5], src_x[i + 2]);
		ch_div_tmp = _mm_and_ps(ch_div_tmp, ch2_mask);
		ch_div_max = _mm_or_ps(ch_div_tmp, ch_div_max);
		_mm_storeu_ps(&dst_x[i / 3], ch_div_max);

		//_mm_prefetch((const char*)(src_y + i), _MM_HINT_NTA);
		ch_div_tmp = _mm_set_ps(src_y[i + 9], src_y[i + 6], src_y[i + 3], src_y[i + 0]);
		ch_div_tmp = _mm_and_ps(ch_div_tmp, ch0_mask);
		ch_div_max = _mm_set_ps(src_y[i + 10], src_y[i + 7], src_y[i + 4], src_y[i + 1]);
		ch_div_max = _mm_and_ps(ch_div_max, ch1_mask);
		ch_div_max = _mm_or_ps(ch_div_tmp, ch_div_max);
		ch_div_tmp = _mm_set_ps(src_y[i + 11], src_y[i + 8], src_y[i + 5], src_y[i + 2]);
		ch_div_tmp = _mm_and_ps(ch_div_tmp, ch2_mask);
		ch_div_max = _mm_or_ps(ch_div_tmp, ch_div_max);
		_mm_storeu_ps(&dst_y[i / 3], ch_div_max);
	}
}

extern "C"
void sse_gradient_c1(int height, int width, float *pdx, float *pdy,
	float *pangle, float *pmag)
{
	int len = width * height;
	sse_sqrt_sum_of_squares(pdx, pdy, pmag, len);

	//float *tpangle = pangle;
	//float *tpdx = pdx;
	//float *tpdy = pdy;
	//for (int i = 0; i < len; ++i, ++tpdx, ++tpdy)
	//	*tpangle = cv::fastAtan2(pdy, pdx);
	//sse_subs_180(pangle, len);

	sse_atan2_0_180(pdx, pdy, pangle, len);
}

extern "C"
void sse_gradient_c3(int height, int width, float *pdx, float *pdy,
	float *pangle, float *pmag)
{
	int len = width * height;

	float *squares = (float*)aligned_malloc((len * 3) * sizeof(float), 16);
	float *pdx1 = (float*)aligned_malloc(len * sizeof(float), 16);
	float *pdy1 = (float*)aligned_malloc(len * sizeof(float), 16);
	float *tpdx1 = pdx1;
	float *tpdy1 = pdy1;
	//printf("squares\n");
	sse_sqrt_sum_of_squares(pdx, pdy, squares, len * 3);
	//printf("max\n");
	//sse_max_diff_mag_c3(pdx, pdy, squares, pdx1, pdy1, pmag, len * 3);
	for (int i = 0; i < len; ++i, pdx += 3, pdy += 3, ++pmag, ++tpdx1, ++tpdy1)
	{
		float max_mv = -1.0f;
		int max_ind = 0;
		for (int c = 0; c < 3; ++c)
		{
			float mv = squares[i * 3 + c];
			if (mv > max_mv) {
				max_ind = c;
				max_mv = mv;
			}
		}
		*pmag = max_mv;
		*tpdx1 = pdx[max_ind];
		*tpdy1 = pdy[max_ind];
	}

	//float *tpangle = pangle;
	//for (int i = 0; i < len; ++i)
	//	*tpangle = cv::fastAtan2(pdy1, pdx1);
	//sse_subs_180(pangle, len);

	sse_atan2_0_180(pdx1, pdy1, pangle, len);
	aligned_free(squares);
	aligned_free(pdx1);
	aligned_free(pdy1);
}

extern "C"
void sse_integral_c10_32f(int height, int width, float *src, float *dst)
{
	static const int channels = 10;
	const int src_step = width * channels;
	const int sum_step = (width + 1) * channels;
	memset(dst, 0, sum_step * sizeof(float));
	dst += sum_step;

	for (int y = 0; y < height - 1; ++y)
	{
		__m128 sum_x0 = _mm_setzero_ps();
		__m128 sum_x1 = _mm_setzero_ps();
		__m128 sum_x2 = _mm_setzero_ps();
		memset(dst, 0, channels * sizeof(float));
		dst += channels;

		//will write small piece to next row
		for (int x = 0; x < width * channels; x += channels)
		{
			__m128 x0 = _mm_loadu_ps(src + x);
			__m128 x1 = _mm_loadu_ps(src + x + 4);
			__m128 x2 = _mm_loadu_ps(src + x + 8);
			sum_x0 = _mm_add_ps(sum_x0, x0);
			sum_x1 = _mm_add_ps(sum_x1, x1);
			sum_x2 = _mm_add_ps(sum_x2, x2);

			__m128 dst0 = _mm_loadu_ps(dst - sum_step + x);
			__m128 dst1 = _mm_loadu_ps(dst - sum_step + x + 4);
			__m128 dst2 = _mm_loadu_ps(dst - sum_step + x + 8);
			dst0 = _mm_add_ps(dst0, sum_x0);
			dst1 = _mm_add_ps(dst1, sum_x1);
			dst2 = _mm_add_ps(dst2, sum_x2);
			_mm_storeu_ps(dst + x, dst0);
			_mm_storeu_ps(dst + x + 4, dst0);
			_mm_storeu_ps(dst + x + 8, dst0);
		}

		src += src_step;
		dst += sum_step;
	}

	//last row must be processed separately
	__m128 sum_x0 = _mm_setzero_ps();
	__m128 sum_x1 = _mm_setzero_ps();
	__m128 sum_x2 = _mm_setzero_ps();
	memset(dst, 0, channels * sizeof(float));
	dst += channels;
	int x = 0;
	for ( ; x < width * channels - channels; x += channels)
	{
		__m128 x0 = _mm_loadu_ps(src + x);
		__m128 x1 = _mm_loadu_ps(src + x + 4);
		__m128 x2 = _mm_loadu_ps(src + x + 8);
		sum_x0 = _mm_add_ps(sum_x0, x0);
		sum_x1 = _mm_add_ps(sum_x1, x1);
		sum_x2 = _mm_add_ps(sum_x2, x2);

		__m128 dst0 = _mm_loadu_ps(dst - sum_step + x);
		__m128 dst1 = _mm_loadu_ps(dst - sum_step + x + 4);
		__m128 dst2 = _mm_loadu_ps(dst - sum_step + x + 8);
		dst0 = _mm_add_ps(dst0, sum_x0);
		dst1 = _mm_add_ps(dst1, sum_x1);
		dst2 = _mm_add_ps(dst2, sum_x2);
		_mm_storeu_ps(dst + x, dst0);
		_mm_storeu_ps(dst + x + 4, dst0);
		_mm_storeu_ps(dst + x + 8, dst0);
	}

	//last row, last column
	float last_src_row_col[4] = { src[x + 8], src[x + 9], 0.0f, 0.0f };
	float last_dst_row_col[4];
	__m128 x0 = _mm_loadu_ps(src + x);
	__m128 x1 = _mm_loadu_ps(src + x + 4);
	__m128 x2 = _mm_loadu_ps(last_src_row_col);
	sum_x0 = _mm_add_ps(sum_x0, x0);
	sum_x1 = _mm_add_ps(sum_x1, x1);
	sum_x2 = _mm_add_ps(sum_x2, x2);
	__m128 dst0 = _mm_loadu_ps(dst - sum_step + x);
	__m128 dst1 = _mm_loadu_ps(dst - sum_step + x + 4);
	__m128 dst2 = _mm_loadu_ps(dst - sum_step + x + 8);
	dst0 = _mm_add_ps(dst0, sum_x0);
	dst1 = _mm_add_ps(dst1, sum_x1);
	dst2 = _mm_add_ps(dst2, sum_x2);
	_mm_storeu_ps(dst + x, dst0);
	_mm_storeu_ps(dst + x + 4, dst1);
	_mm_storeu_ps(last_dst_row_col, dst2);

	dst[x + 8] = last_dst_row_col[0];
	dst[x + 9] = last_dst_row_col[1];
}

extern "C"
void sse_integral_c10_64f(int height, int width, float *src, double *dst)
{
	static const int channels = 10;
	const int src_step = width * channels;
	const int sum_step = (width + 1) * channels;
	memset(dst, 0, sum_step * sizeof(double));
	dst += sum_step;

	for (int y = 0; y < height - 1; ++y)
	{
		__m128 sum_x0 = _mm_setzero_ps();
		__m128 sum_x1 = _mm_setzero_ps();
		__m128 sum_x2 = _mm_setzero_ps();
		memset(dst, 0, channels * sizeof(double));
		dst += channels;

		//will write small piece to next row
		for (int x = 0; x < width * channels; x += channels)
		{
			__m128 x0 = _mm_loadu_ps(src + x);
			__m128 x1 = _mm_loadu_ps(src + x + 4);
			__m128 x2 = _mm_loadu_ps(src + x + 8);
			sum_x0 = _mm_add_ps(sum_x0, x0);
			sum_x1 = _mm_add_ps(sum_x1, x1);
			sum_x2 = _mm_add_ps(sum_x2, x2);

			__m128d dst0 = _mm_loadu_pd(dst - sum_step + x);
			__m128d dst1 = _mm_loadu_pd(dst - sum_step + x + 2);
			__m128d dst2 = _mm_loadu_pd(dst - sum_step + x + 4);
			__m128d dst3 = _mm_loadu_pd(dst - sum_step + x + 6);
			__m128d dst4 = _mm_loadu_pd(dst - sum_step + x + 8);

			__m128d dst_s = _mm_cvtps_pd(sum_x0);
			dst0 = _mm_add_pd(dst0, dst_s);
#ifndef UGLY_VC71
			__m128 dst_d = _mm_castsi128_ps(_mm_srli_si128(_mm_castps_si128(sum_x0), 8));
#else
			__m128 dst_d = _mm_set_ps(0.0f, 0.0f, sum_x0.m128_f32[3], sum_x0.m128_f32[2]);
#endif
			dst_s = _mm_cvtps_pd(dst_d);
			dst1 = _mm_add_pd(dst1, dst_s);

			dst_s = _mm_cvtps_pd(sum_x1);
			dst2 = _mm_add_pd(dst2, dst_s);
#ifndef UGLY_VC71
			dst_d = _mm_castsi128_ps(_mm_srli_si128(_mm_castps_si128(sum_x1), 8));
#else
			dst_d = _mm_set_ps(0.0f, 0.0f, sum_x1.m128_f32[3], sum_x1.m128_f32[2]);
#endif
			dst_s = _mm_cvtps_pd(dst_d);
			dst3 = _mm_add_pd(dst3, dst_s);

			dst_s = _mm_cvtps_pd(sum_x2);
			dst4 = _mm_add_pd(dst4, dst_s);

			_mm_storeu_pd(dst + x, dst0);
			_mm_storeu_pd(dst + x + 2, dst1);
			_mm_storeu_pd(dst + x + 4, dst2);
			_mm_storeu_pd(dst + x + 6, dst3);
			_mm_storeu_pd(dst + x + 8, dst4);
		}

		src += src_step;
		dst += sum_step - channels;
	}

	//last row must be processed separately
	__m128 sum_x0 = _mm_setzero_ps();
	__m128 sum_x1 = _mm_setzero_ps();
	__m128 sum_x2 = _mm_setzero_ps();
	memset(dst, 0, channels * sizeof(double));
	dst += channels;
	int x = 0;
	for ( ; x < width * channels - channels; x += channels)
	{
		__m128 x0 = _mm_loadu_ps(src + x);
		__m128 x1 = _mm_loadu_ps(src + x + 4);
		__m128 x2 = _mm_loadu_ps(src + x + 8);
		sum_x0 = _mm_add_ps(sum_x0, x0);
		sum_x1 = _mm_add_ps(sum_x1, x1);
		sum_x2 = _mm_add_ps(sum_x2, x2);

		__m128d dst0 = _mm_loadu_pd(dst - sum_step + x);
		__m128d dst1 = _mm_loadu_pd(dst - sum_step + x + 2);
		__m128d dst2 = _mm_loadu_pd(dst - sum_step + x + 4);
		__m128d dst3 = _mm_loadu_pd(dst - sum_step + x + 6);
		__m128d dst4 = _mm_loadu_pd(dst - sum_step + x + 8);

		__m128d dst_s = _mm_cvtps_pd(sum_x0);
		dst0 = _mm_add_pd(dst0, dst_s);
#ifndef UGLY_VC71
		__m128 dst_d = _mm_castsi128_ps(_mm_srli_si128(_mm_castps_si128(sum_x0), 8));
#else
		__m128 dst_d = _mm_set_ps(0.0f, 0.0f, sum_x0.m128_f32[3], sum_x0.m128_f32[2]);
#endif
		dst_s = _mm_cvtps_pd(dst_d);
		dst1 = _mm_add_pd(dst1, dst_s);

		dst_s = _mm_cvtps_pd(sum_x1);
		dst2 = _mm_add_pd(dst2, dst_s);
#ifndef UGLY_VC71
		dst_d = _mm_castsi128_ps(_mm_srli_si128(_mm_castps_si128(sum_x1), 8));
#else
		dst_d = _mm_set_ps(0.0f, 0.0f, sum_x1.m128_f32[3], sum_x1.m128_f32[2]);
#endif
		dst_s = _mm_cvtps_pd(dst_d);
		dst3 = _mm_add_pd(dst3, dst_s);

		dst_s = _mm_cvtps_pd(sum_x2);
		dst4 = _mm_add_pd(dst4, dst_s);

		_mm_storeu_pd(dst + x, dst0);
		_mm_storeu_pd(dst + x + 2, dst1);
		_mm_storeu_pd(dst + x + 4, dst2);
		_mm_storeu_pd(dst + x + 6, dst3);
		_mm_storeu_pd(dst + x + 8, dst4);
	}

	//last row, last column
	float last_src_row_col[4] = { src[x + 8], src[x + 9], 0.0f, 0.0f };
	__m128 x0 = _mm_loadu_ps(src + x);
	__m128 x1 = _mm_loadu_ps(src + x + 4);
	__m128 x2 = _mm_loadu_ps(last_src_row_col);
	sum_x0 = _mm_add_ps(sum_x0, x0);
	sum_x1 = _mm_add_ps(sum_x1, x1);
	sum_x2 = _mm_add_ps(sum_x2, x2);

	__m128d dst0 = _mm_loadu_pd(dst - sum_step + x);
	__m128d dst1 = _mm_loadu_pd(dst - sum_step + x + 2);
	__m128d dst2 = _mm_loadu_pd(dst - sum_step + x + 4);
	__m128d dst3 = _mm_loadu_pd(dst - sum_step + x + 6);
	__m128d dst4 = _mm_loadu_pd(dst - sum_step + x + 8);

	__m128d dst_s = _mm_cvtps_pd(sum_x0);
	dst0 = _mm_add_pd(dst0, dst_s);
#ifndef UGLY_VC71
	__m128 dst_d = _mm_castsi128_ps(_mm_srli_si128(_mm_castps_si128(sum_x0), 8));
#else
	__m128 dst_d = _mm_set_ps(0.0f, 0.0f, sum_x0.m128_f32[3], sum_x0.m128_f32[2]);
#endif
	dst_s = _mm_cvtps_pd(dst_d);
	dst1 = _mm_add_pd(dst1, dst_s);

	dst_s = _mm_cvtps_pd(sum_x1);
	dst2 = _mm_add_pd(dst2, dst_s);

#ifndef UGLY_VC71
	dst_d = _mm_castsi128_ps(_mm_srli_si128(_mm_castps_si128(sum_x1), 8));
#else
	dst_d = _mm_set_ps(0.0f, 0.0f, sum_x1.m128_f32[3], sum_x1.m128_f32[2]);
#endif
	dst_s = _mm_cvtps_pd(dst_d);
	dst3 = _mm_add_pd(dst3, dst_s);

	dst_s = _mm_cvtps_pd(sum_x2);
	dst4 = _mm_add_pd(dst4, dst_s);

	_mm_storeu_pd(dst + x, dst0);
	_mm_storeu_pd(dst + x + 2, dst1);
	_mm_storeu_pd(dst + x + 4, dst2);
	_mm_storeu_pd(dst + x + 6, dst3);
	_mm_storeu_pd(dst + x + 8, dst4);
}

extern "C"
void sse_integral_c10_32s(int height, int width, uint8_t *src, int *dst)
{
	static const int channels = 10;
	const int src_step = width * channels;
	const int sum_step = (width + 1) * channels;
	memset(dst, 0, sum_step * sizeof(int));
	dst += sum_step;

	for (int y = 0; y < height - 1; ++y)
	{
		__m128i sum_x0 = _mm_setzero_si128();
		__m128i sum_x1 = _mm_setzero_si128();
		__m128i sum_x2 = _mm_setzero_si128();
		memset(dst, 0, channels * sizeof(int));
		dst += channels;

		//will write small piece to next row
		for (int x = 0; x < width * channels; x += channels)
		{
			__m128i x_bytes = _mm_loadu_si128((const __m128i*)(src + x));

			//SSE 4.1
			//__m128i x0 = _mm_cvtepu8_epi32(x_bytes);
			//x_bytes = _mm_srli_si128(x_bytes, 4);
			//__m128i x1 = _mm_cvtepu8_epi32(x_bytes);
			//x_bytes = _mm_srli_si128(x_bytes, 4);
			//__m128i x2 = _mm_cvtepu8_epi32(x_bytes);

			//SSE 2
			__m128i x0 = _mm_unpacklo_epi8(x_bytes, _mm_setzero_si128());
			x0 = _mm_unpacklo_epi16(x0, _mm_setzero_si128());
			x_bytes = _mm_srli_si128(x_bytes, 4);
			__m128i x1 = _mm_unpacklo_epi8(x_bytes, _mm_setzero_si128());
			x1 = _mm_unpacklo_epi16(x1, _mm_setzero_si128());
			x_bytes = _mm_srli_si128(x_bytes, 4);
			__m128i x2 = _mm_unpacklo_epi8(x_bytes, _mm_setzero_si128());
			x2 = _mm_unpacklo_epi16(x2, _mm_setzero_si128());

			sum_x0 = _mm_add_epi32(sum_x0, x0);
			sum_x1 = _mm_add_epi32(sum_x1, x1);
			sum_x2 = _mm_add_epi32(sum_x2, x2);

			__m128i dst0 = _mm_loadu_si128((const __m128i*)(dst - sum_step + x));
			__m128i dst1 = _mm_loadu_si128((const __m128i*)(dst - sum_step + x + 4));
			__m128i dst2 = _mm_loadu_si128((const __m128i*)(dst - sum_step + x + 8));

			dst0 = _mm_add_epi32(dst0, sum_x0);
			dst1 = _mm_add_epi32(dst1, sum_x1);
			dst2 = _mm_add_epi32(dst2, sum_x2);

			_mm_storeu_si128((__m128i*)(dst + x), dst0);
			_mm_storeu_si128((__m128i*)(dst + x + 4), dst1);
			_mm_storeu_si128((__m128i*)(dst + x + 8), dst2);
		}

		src += src_step;
		dst += sum_step - channels;
	}

	//last row must be processed separately
	__m128i sum_x0 = _mm_setzero_si128();
	__m128i sum_x1 = _mm_setzero_si128();
	__m128i sum_x2 = _mm_setzero_si128();
	memset(dst, 0, channels * sizeof(int));
	dst += channels;
	int x = 0;
	for ( ; x < width * channels - channels; x += channels)
	{
		//__m128i xl = _mm_loadu_si128((const __m128i*)(src + x));
		//__m128i x0 = _mm_cvt...

		__m128i x0 = _mm_set_epi32(src[x + 3], src[x + 2], src[x + 1], src[x]);
		__m128i x1 = _mm_set_epi32(src[x + 7], src[x + 6], src[x + 5], src[x + 4]);
		__m128i x2 = _mm_set_epi32(0, 0, src[x + 9], src[x + 8]);

		sum_x0 = _mm_add_epi32(sum_x0, x0);
		sum_x1 = _mm_add_epi32(sum_x1, x1);
		sum_x2 = _mm_add_epi32(sum_x2, x2);

		__m128i dst0 = _mm_loadu_si128((const __m128i*)(dst - sum_step + x));
		__m128i dst1 = _mm_loadu_si128((const __m128i*)(dst - sum_step + x + 4));
		__m128i dst2 = _mm_loadu_si128((const __m128i*)(dst - sum_step + x + 8));

		dst0 = _mm_add_epi32(dst0, sum_x0);
		dst1 = _mm_add_epi32(dst1, sum_x1);
		dst2 = _mm_add_epi32(dst2, sum_x2);

		_mm_storeu_si128((__m128i*)(dst + x), dst0);
		_mm_storeu_si128((__m128i*)(dst + x + 4), dst1);
		_mm_storeu_si128((__m128i*)(dst + x + 8), dst2);
	}

	//last row, last column
	__m128i x0 = _mm_set_epi32(src[x + 3], src[x + 2], src[x + 1], src[x]);
	__m128i x1 = _mm_set_epi32(src[x + 7], src[x + 6], src[x + 5], src[x + 4]);
	__m128i x2 = _mm_set_epi32(0, 0, src[x + 9], src[x + 8]);

	sum_x0 = _mm_add_epi32(sum_x0, x0);
	sum_x1 = _mm_add_epi32(sum_x1, x1);
	sum_x2 = _mm_add_epi32(sum_x2, x2);

	__m128i dst0 = _mm_loadu_si128((const __m128i*)(dst - sum_step + x));
	__m128i dst1 = _mm_loadu_si128((const __m128i*)(dst - sum_step + x + 4));
	__m128i dst2 = _mm_loadu_si128((const __m128i*)(dst - sum_step + x + 8));

	dst0 = _mm_add_epi32(dst0, sum_x0);
	dst1 = _mm_add_epi32(dst1, sum_x1);
	dst2 = _mm_add_epi32(dst2, sum_x2);

	_mm_storeu_si128((__m128i*)(dst + x), dst0);
	_mm_storeu_si128((__m128i*)(dst + x + 4), dst1);
	int tmp_dst[4];
	_mm_storeu_si128((__m128i*)tmp_dst, dst2);
	dst[x + 8] = tmp_dst[0];
	dst[x + 9] = tmp_dst[1];
}

//float* rgb2luv_setup(float z, float *mr, float *mg, float *mb,
//					 float &minu, float &minv, float &un, float &vn)
//{
//	// set constants for conversion
//	const float y0=(float) ((6.0/29)*(6.0/29)*(6.0/29));
//	const float a= (float) ((29.0/3)*(29.0/3)*(29.0/3));
//	un=(float) 0.197833; vn=(float) 0.468331;
//	mr[0]=(float) 0.430574*z; mr[1]=(float) 0.222015*z; mr[2]=(float) 0.020183*z;
//	mg[0]=(float) 0.341550*z; mg[1]=(float) 0.706655*z; mg[2]=(float) 0.129553*z;
//	mb[0]=(float) 0.178325*z; mb[1]=(float) 0.071330*z; mb[2]=(float) 0.939180*z;
//	float maxi=(float) 1.0/270; minu=-88*maxi; minv=-134*maxi;
//	// build (padded) lookup table for y->l conversion assuming y in [0,1]
//	static float lTable[1064];
//	static bool lInit = false;
//	if (lInit)
//		return lTable; float y, l;
//	for(int i=0; i<1025; i++) {
//		y = (float) (i/1024.0);
//		l = y>y0 ? 116*(float)pow((double)y,1.0/3.0)-16 : y*a;
//		lTable[i] = l*maxi;
//	}
//	for(int i=1025; i<1064; i++) lTable[i]=lTable[i-1];
//	lInit = true;
//	return lTable;
//}

//// Convert from rgb to luv
//template<class iT, class oT> void rgb2luv( iT *I, oT *J, int n, oT nrm ) {
//  oT minu, minv, un, vn, mr[3], mg[3], mb[3];
//  oT *lTable = rgb2luv_setup(nrm,mr,mg,mb,minu,minv,un,vn);
//  oT *L=J, *U=L+n, *V=U+n; iT *R=I, *G=R+n, *B=G+n;
//  for( int i=0; i<n; i++ ) {
//    oT r, g, b, x, y, z, l;
//    r=(oT)*R++; g=(oT)*G++; b=(oT)*B++;
//    x = mr[0]*r + mg[0]*g + mb[0]*b;
//    y = mr[1]*r + mg[1]*g + mb[1]*b;
//    z = mr[2]*r + mg[2]*g + mb[2]*b;
//    l = lTable[(int)(y*1024)];
//    *(L++) = l; z = 1/(x + 15*y + 3*z + (oT)1e-35);
//    *(U++) = l * (13*4*x*z - 13*un) - minu;
//    *(V++) = l * (13*9*y*z - 13*vn) - minv;
//  }
//}

//void rgb2luv_sse(float *I, float *J, int n, float nrm)
//{
//	const int k=256; float R[k], G[k], B[k];
//	if( (size_t(R)&15||size_t(G)&15||size_t(B)&15||size_t(I)&15||size_t(J)&15)
//		|| n%4>0 ) { rgb2luv(I,J,n,nrm); return; }
//	int i = 0, i1, n1;
//	float minu, minv, un, vn, mr[3], mg[3], mb[3];
//	float *lTable = rgb2luv_setup(nrm,mr,mg,mb,minu,minv,un,vn);
//	while (i < n)
//	{
//		n1 = i + k;
//		if (n1 > n)
//			n1 = n;
//		float *J1=J+i;
//		// convert to floats (and load input into cache)
//		float *R1= I + i;
//		float *G1= R1 + n;
//		float *B1= G1 + n;
//		// compute RGB -> XYZ
//		for( int j=0; j<3; j++ ) {
//			__m128 _mr, _mg, _mb, *_J=(__m128*) (J1+j*n);
//			__m128 *_R=(__m128*) R1, *_G=(__m128*) G1, *_B=(__m128*) B1;
//			_mr=SET(mr[j]); _mg=SET(mg[j]); _mb=SET(mb[j]);
//			for( i1=i; i1<n1; i1+=4 ) *(_J++) = ADD( ADD(MUL(*(_R++),_mr),
//														 MUL(*(_G++),_mg)),MUL(*(_B++),_mb));
//		}
//		{ // compute XZY -> LUV (without doing L lookup/normalization)
//			__m128 _c15, _c3, _cEps, _c52, _c117, _c1024, _cun, _cvn;
//			_c15=SET(15.0f); _c3=SET(3.0f); _cEps=SET(1e-35f);
//			_c52=SET(52.0f); _c117=SET(117.0f), _c1024=SET(1024.0f);
//			_cun=SET(13*un); _cvn=SET(13*vn);
//			__m128 *_X, *_Y, *_Z, _x, _y, _z;
//			_X=(__m128*) J1; _Y=(__m128*) (J1+n); _Z=(__m128*) (J1+2*n);
//			for( i1=i; i1<n1; i1+=4 ) {
//				_x = *_X; _y=*_Y; _z=*_Z;
//				_z = RCP(ADD(_x,ADD(_cEps,ADD(MUL(_c15,_y),MUL(_c3,_z)))));
//				*(_X++) = MUL(_c1024,_y);
//				*(_Y++) = SUB(MUL(MUL(_c52,_x),_z),_cun);
//				*(_Z++) = SUB(MUL(MUL(_c117,_y),_z),_cvn);
//			}
//		}
//		{ // perform lookup for L and finalize computation of U and V
//			for( i1=i; i1<n1; i1++ ) J[i1] = lTable[(int)J[i1]];
//			__m128 *_L, *_U, *_V, _l, _cminu, _cminv;
//			_L=(__m128*) J1; _U=(__m128*) (J1+n); _V=(__m128*) (J1+2*n);
//			_cminu=SET(minu); _cminv=SET(minv);
//			for( i1=i; i1<n1; i1+=4 ) {
//				_l = *(_L++);
//				*(_U++) = SUB(MUL(_l,*_U),_cminu);
//				*(_V++) = SUB(MUL(_l,*_V),_cminv);
//			}
//		}
//		i = n1;
//	}
//}

extern "C"
void sse_rgb_to_luv(int height, int width, float *psrc, float *pdst)
{
	static const int channels = 10;
	int len = width * height;

//	__m128 norm0 = _mm_set1_ps((float)1.0f / 255.0f);
//	for (int i = 0; i < len; ++i)
//	{
//		__m128 x0 = _mm_loadu_ps(psrc);
//		float max_mv = -1.0f;
//		int max_ind = 0;
//		for (int c = 0; c < 3; ++c)
//		{
//			float mv = squares[i * 3 + c];
//			if (mv > max_mv) {
//				max_ind = c;
//				max_mv = mv;
//			}
//		}
//		*pmag = max_mv;
//		*tpdx1 = pdx[max_ind];
//		*tpdy1 = pdy[max_ind];
//		_mm_storeu_ps(pdst, res);
//		psrc += 3;
//		pdst += channels;
//	}
}

extern "C"
void sse_quantize_soft_6_10(int height, int width, float *psrc, float *pweight, float *pdst)
{
	static const int channels = 10;
	const int len = width * height;

	int *indexes0 = (int*)aligned_malloc(len * sizeof(float), 16);
	int *indexes1 = (int*)aligned_malloc(len * sizeof(float), 16);
	float *vals0 = (float*)aligned_malloc(len * sizeof(float), 16);
	float *vals1 = (float*)aligned_malloc(len * sizeof(float), 16);

//	__m128 mult = _mm_set1_ps(6.0f / 179.99f);
//	__m128i one = _mm_set1_pi32(1);
//	__m128i seven = _mm_set1_pi32(7);
//	__m128 onef = _mm_set1_ps(1.0f);
//	for (int idx = 0; idx < len; ++idx)
//	{
//		__m128 binf = _mm_loadu_ps(psrc + idx);
//		__m128 my_val = _mm_loadu_ps(pweight + idx);

//		binf = _mm_mul_ps(my_dir, mult);

//		__m128i bin0 = _mm_cvttps_epi32(binf);
//		__m128i bin1 = _mm_add_epi32(bin0, one);
//		__m128i bin1_mask = _mm_cmpeq_epi32(bin1, seven);
//		bin1 = _mm_andnot_si128(bin1_mask, bin1);

//		__m128 bin1f = _mm_cvtepi32_ps(bin0);
//		bin1f = _mm_sub_ps(binf, bin1f);
//		__m128 bin0f = _mm_sub_ps(onef, fin1f);


//		__m128 sum_x0 = _mm_setzero_ps();
//		__m128 sum_x1 = _mm_setzero_ps();
//		__m128 sum_x2 = _mm_setzero_ps();
//	}

	aligned_free(indexes0);
	aligned_free(indexes1);
	aligned_free(vals0);
	aligned_free(vals1);
}
