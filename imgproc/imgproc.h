/** @file imgproc.h
 *
 *  @brief image processing operations
 *
 *  @author Victor Pogrebnyak <vp@aifil.ru>
 *
 *  $Id$
 *
 *
 */

#ifndef AIFIL_IMAGE_PROCESSING_H
#define AIFIL_IMAGE_PROCESSING_H

//#define DEBUG_PRINT
//#ifdef DEBUG_PRINT
//#endif

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/types_c.h>

#include <stdint.h>
#include <list>
#include <vector>
#include <string>

#include "filter.h"

namespace aifil {
namespace imgproc {

enum IMAGE_TRANSFORM
{
	IMG_TRANSFORM_NONE = 0,
	IMG_TRANSFORM_FLIP_X = 0x1,
	IMG_TRANSFORM_FLIP_Y = 0x2,
	IMG_TRANSFORM_FLIP_XY = IMG_TRANSFORM_FLIP_X | IMG_TRANSFORM_FLIP_Y
};

// basic
void resize(const uint8_t *src, uint32_t src_w, uint32_t src_h, uint32_t src_rs,
	uint8_t *dst, int dst_w, int dst_h, int dst_rs);
int difference(const cv::Mat &src_0, const cv::Mat &src_1,
	int thresh, cv::Mat &dst, cv::Mat &tmp);
int difference(const cv::Mat &src_0, const cv::Mat &src_1,
	const cv::Mat &thresh, cv::Mat &dst, cv::Mat &tmp);

// colorspaces
void rgb_from_yuv(const cv::Mat &Y, const cv::Mat &U, const cv::Mat &V,
				  cv::Mat &rgb);
void rgb_to_luv_pix(const float r, const float g, const float b,
	float* pl, float* pu, float* pv);


// feature points
typedef double (*corners_metric_t)(double, double, double, double);
void corners(const cv::Mat &dx, const cv::Mat &dy,
	int block_size, int max_corners, int th, corners_metric_t metric,
	double coeff = 0);
void corners_harris(const cv::Mat &dx, const cv::Mat &dy, double K = 0.04,
	int block_size = 3, int max_corners = 1000, int threshold = 1e9);
void corners_shi_tomasi(const cv::Mat &dx, const cv::Mat &dy,
	int block_size = 3, int max_corners = 1000, int eigenval_th = 32000);

// quantizations
// simple histogram with weight 1 of each value
template<typename T>
void histogram(const cv::Mat &src, std::vector<int> &dst,
	int nbins, int channel = 0, int min = 0, int max = 255);
// normalized histogram (values from 0 to 1)
template<typename T>
void histogram_normalized(const cv::Mat &src, std::vector<double> &dst,
	int nbins, int channel, int min, int max);
template<typename T>
void histogram_all_channels(const cv::Mat &src, std::vector<std::vector<int> > &hists);

//dst is multichannel matrix with the same size as src
//channel value: weight in point(x, y) if channel index == normalized_src / nbins
//or zero otherwise
//actually there is (nbins+1) values, in first we store max_weight in input channel
void quantize(const cv::Mat &src, const cv::Mat &weights, cv::Mat &dst, int nbins,
	int start_ch = 0, float min = 0, float max = 1.0);
void quantize_soft_32f(const cv::Mat &src, const cv::Mat &weights, cv::Mat &dst,
	int nbins, int start_ch = 0, float min = 0, float max = 1.0);
void quantize_soft_8u(const cv::Mat &src, const cv::Mat &weights, cv::Mat &dst,
	int nbins, int start_ch = 0, float min = 0, float max = 1.0);

void flatten(const cv::Mat &src, cv::Mat &dst); //merge all channels
cv::Point2f perspective_transform_apply(cv::Point2f point, cv::Size size,
	float m00, float m01, float m02, float m10, float m11, float m12,
	float m20, float m21, float m22);
void perspective_transform(const cv::Mat & a, cv::Mat &b,
	float m00, float m01, float m02, float m10, float m11, float m12,
	float m20, float m21, float m22);

void integrate(const cv::Mat &src, cv::Mat &dst);


//classic historgam of oriented gradient
void hog_classic(const cv::Mat &grangle, const cv::Mat &grmag, cv::Mat &dst,
	int nbins, int cell_w, int cell_h, cv::Mat &cn, cv::Mat &ca);
void hog_kernel(const cv::Mat &grangle, const cv::Mat &grmag, cv::Mat &dst, int start_ch);

} //namespace imgproc

template<typename T> T* mat_ptr(const cv::Mat &mat, int row, int col, int ch)
{
	return (T*)(mat.data) + row * mat.step1() + col * mat.channels() + ch;
}

template<typename T> T trunc_val(T x, T min_val, T max_val)
{
	if (x < min_val)
		return min_val;
	else if (x > max_val)
		return max_val;
	else
		return x;
}

// safe types conversion (saturate cast)
template<typename T>
inline uint8_t to_uint8_t(T val)
{
	if (val > static_cast<T>(255))
		return 0xFF;
	else if (val < 0)
		return 0;
	else
		return static_cast<uint8_t>(val);
}

template<typename T>
inline uint16_t to_uint16_t(T val)
{
	if (val > static_cast<T>(65535))
		return 0xFFFF;
	else if (val < 0)
		return 0;
	else
		return static_cast<uint16_t>(val);
}

// for using in templated functions: create matrix of required type
template<typename T>
cv::Mat cv_mat_create(int rows, int cols, int channels);

template<> inline
cv::Mat cv_mat_create<uint8_t>(int rows, int cols, int channels)
{
	return cv::Mat(rows, cols, CV_MAKETYPE(CV_8U, channels));
}

template<> inline
cv::Mat cv_mat_create<int>(int rows, int cols, int channels)
{
	return cv::Mat(rows, cols, CV_MAKETYPE(CV_32S, channels));
}

template<> inline
cv::Mat cv_mat_create<float>(int rows, int cols, int channels)
{
	return cv::Mat(rows, cols, CV_MAKETYPE(CV_32F, channels));
}

template<> inline
cv::Mat cv_mat_create<double>(int rows, int cols, int channels)
{
	return cv::Mat(rows, cols, CV_MAKETYPE(CV_64F, channels));
}

template<typename T>
cv::Mat cv_mat_convert(const cv::Mat &src);

template<> inline
cv::Mat cv_mat_convert<double>(const cv::Mat &src)
{
	cv::Mat dst;
	if (src.depth() == CV_64F)
		src.copyTo(dst);
	else
		src.convertTo(dst, CV_64F);
	return dst;
}

template<> inline
cv::Mat cv_mat_convert<float>(const cv::Mat &src)
{
	cv::Mat dst;
	if (src.depth() == CV_32F)
		src.copyTo(dst);
	else
		src.convertTo(dst, CV_32F);
	return dst;
}

}  // namespace aifil

#endif  // AIFIL_IMAGE_PROCESSING_H
