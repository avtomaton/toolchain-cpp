/** @file imgproc.cpp
 *
 *  @brief image processing operations
 *
 *  @author Viktor Pogrebniak <vp@aifil.ru>
 *
 *  $Id$
 *
 *
 */
#include "imgproc.hpp"

//#include "sse/sse.h"

#include <common/errutils.hpp>
#include <common/logging.hpp>
#include <common/profiler.hpp>
#include <common/rngutils.hpp>
#include <common/stringutils.hpp>

#ifdef HAVE_SWSCALE
extern "C" {
#include <libswscale/swscale.h>
}
#endif

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/video/video.hpp>
#include <opencv2/imgproc/types_c.h>

namespace aifil {

#ifdef HAVE_SSE
// extern SSE code
extern "C" void sse_gradient_c1(int height, int width, float *pdx, float *pdy,
	float *pangle, float *pmag);
extern "C" void sse_gradient_c3(int height, int width, float *pdx, float *pdy,
	float *pangle, float *pmag);

extern "C" void sse_integral_c10_32f(int height, int width, float *src, float *dst);
extern "C" void sse_integral_c10_64f(int height, int width, float *src, double *dst);
extern "C" void sse_integral_c10_32s(int height, int width, uint8_t *src, int *dst);

extern "C" void sse_quantize_soft_6_10(int height, int width,
	float *psrc, float *pweight, float *pdst);

extern "C" void cuda_gradient_c1(int height, int width, float *dx, float *dy,
	 float *angle, float *magnitude);
extern "C" void cuda_gradient_c3(int height, int width, float *dx, float *dy,
	float *angle, float *magnitude);
#endif

namespace imgproc {

void rgb_from_yuv(const cv::Mat &Y, const cv::Mat &U, const cv::Mat &V,
	cv::Mat &rgb)
{
	af_assert(Y.type() == CV_8UC1);
	af_assert(U.cols == Y.cols / 2 && U.rows == Y.rows / 2 && U.type() == CV_8UC1);
	af_assert(V.cols == Y.cols / 2 && V.rows == Y.rows / 2 && V.type() == CV_8UC1);

	int width = Y.cols;
	int height = Y.rows;
	if (rgb.cols != Y.cols || rgb.rows != Y.rows || rgb.channels() != 3)
		rgb = cv::Mat(height, width, CV_8UC3);

#ifdef HAVE_SWSCALE
	struct SwsContext* img_convert_ctx = sws_getContext(
		Y.cols, Y.rows, AV_PIX_FMT_YUV420P,
		width, height, AV_PIX_FMT_RGB24,
		SWS_AREA, NULL, NULL, NULL);
	aifil::except(img_convert_ctx, "IMGPROC: sws_getContext failed");
	{
		const uint8_t* pSrc[3] = { Y.data, U.data, V.data };
		int srcStep[3] = { (int)Y.step1(), (int)U.step1(), (int)V.step1() };
		uint8_t* pDst[1] = { rgb.data };
		int dstStep[1] = { 3 * width };
		sws_scale(img_convert_ctx, pSrc, srcStep, 0, Y.rows, pDst, dstStep);
	}
	sws_freeContext(img_convert_ctx);
#else
	cv::Mat yuv(rgb.rows, rgb.cols, CV_8UC3);
	int y_size = rgb.cols * rgb.rows;
	memcpy(yuv.data, Y.data, y_size);
	memcpy(yuv.data + y_size, U.data, y_size / 4);
	memcpy(yuv.data + 5 * y_size / 4, V.data, y_size / 4);
	cv::cvtColor(Y, yuv, CV_YUV2RGB_I420);
#endif

//	cv::Mat rgb;
//	cv::cvtColor(s->img_rgb, rgb, CV_RGB2BGR);
//	cv::imshow("test-ffmpeg", rgb);
//	cv::waitKey();
}

void resize(const uint8_t *src, uint32_t src_w, uint32_t src_h, uint32_t src_rs,
		uint8_t *dst, int dst_w, int dst_h, int dst_rs)
{
	cv::Mat src_mat(src_h, src_w, CV_8UC1, (void*)src, src_rs);
	cv::Mat dst_mat(dst_h, dst_w, CV_8UC1, (void*)dst, dst_rs);
	if (src_w == static_cast<uint32_t>(dst_w) && src_h == static_cast<uint32_t>(dst_h))
		src_mat.copyTo(dst_mat);
	else
		cv::resize(src_mat, dst_mat, dst_mat.size());
}

int difference(const cv::Mat &src_0, const cv::Mat &src_1,
	int thresh, cv::Mat &dst, cv::Mat &tmp)
{
	af_assert(src_0.rows == src_1.rows && src_0.cols == src_1.cols &&
		src_0.type() == src_1.type());
	af_assert(src_0.type() == CV_8UC1);

	if (dst.rows != src_0.rows || dst.cols != src_0.cols || dst.type() != src_0.type())
		dst = cv::Mat(src_0.rows, src_0.cols, src_0.type());
	if (tmp.rows != src_0.rows || tmp.cols != src_0.cols || tmp.type() != src_0.type())
		tmp = cv::Mat(src_0.rows, src_0.cols, src_0.type());

#ifdef HAVE_SSE
	int w = src_0.cols;
	int h = src_0.rows;
	::subtract_abs_1u8_sse((uint8_t *)tmp.data, (uint8_t *)src_0.data,
				(uint8_t *)src_1.data, h, w);
#else
	cv::absdiff(src_0, src_1, tmp);
#endif

	if (thresh == -1)
		return 0;

	int mask_square = 0;
#ifdef HAVE_SSE
	mask_square = ::binarize_1u8_sse((uint8_t *)dst.data, (uint8_t *)tmp.data,
				h, w, thresh);
#else
	cv::threshold(tmp, dst, thresh, 255, CV_THRESH_BINARY);
	mask_square = cv::countNonZero(dst);
#endif

	return mask_square;
}

int difference(const cv::Mat &src_0, const cv::Mat &src_1,
	const cv::Mat &thresh, cv::Mat &dst, cv::Mat &tmp)
{
	af_assert(src_0.rows == src_1.rows && src_0.cols == src_1.cols &&
		   src_0.rows == thresh.rows && src_0.cols == thresh.cols &&
		   src_0.type() == src_1.type() && src_0.type() == thresh.type());
	af_assert(src_0.type() == CV_8UC1);

	if (dst.rows != src_0.rows || dst.cols != src_0.cols || dst.type() != src_0.type())
		dst = cv::Mat(src_0.rows, src_0.cols, src_0.type());
	if (tmp.rows != src_0.rows || tmp.cols != src_0.cols || tmp.type() != src_0.type())
		tmp = cv::Mat(src_0.rows, src_0.cols, src_0.type());

#ifdef HAVE_SSE
	int w = src_0.cols;
	int h = src_0.rows;
	::subtract_abs_1u8_sse((uint8_t *)tmp.data, (uint8_t *)src_0.data,
			(uint8_t *)src_1.data, h, w);
#else
	cv::absdiff(src_0, src_1, tmp);
#endif

	dst = tmp - thresh;
	return cv::countNonZero(dst);
}

cv::Mat rotate(const cv::Mat &src, double angle, double scale, cv::Rect roi, cv::Point2d center)
{
	if (roi == cv::Rect())
		roi = cv::Rect(0, 0, src.cols, src.rows);

	if (angle == 0.0 && scale == 1.0)
		return src(roi).clone();

	if (center == cv::Point2d(-1,-1))
		center = cv::Point2d(roi.x + roi.width/2.0, roi.y + roi.height/2.0);

	int diag = 2 + (int)ceil(sqrt(double(roi.width * roi.width + roi.height * roi.height)));

	cv::Rect crop(center.x - diag / 2.0, center.y - diag / 2.0, diag, diag);
	if (crop.x < 0)
	{
		crop.width += crop.x;
		crop.x = 0;
	}
	if (crop.y < 0)
	{
		crop.height += crop.y;
		crop.y = 0;
	}
	if (crop.x + crop.width > src.cols)
		crop.width = src.cols - crop.x;
	if (crop.y + crop.height > src.rows)
		crop.height = src.rows - crop.y;

	cv::Rect roi_crop(roi.x - crop.x, roi.y - crop.y, roi.width, roi.height);
	cv::Point2f crop_center(center.x - crop.x, center.y - crop.y);
	cv::Mat crop_image = src(crop);

	cv::Mat dst;
	cv::Mat rot_mat = cv::getRotationMatrix2D(crop_center, -angle, 1.0 / scale);
	cv::warpAffine(crop_image, dst, rot_mat, crop_image.size(), cv::INTER_LINEAR | cv::WARP_INVERSE_MAP, cv::BORDER_REPLICATE);
	dst = dst(roi_crop);

	return dst;
}

void flatten(const cv::Mat &src, cv::Mat &dst)
{
	af_assert(src.type() == CV_32FC1);

	if (dst.rows != src.rows || dst.cols != src.cols || dst.type() != src.type())
		dst = cv::Mat(src.rows, src.cols, src.type());

	int ch = src.channels();
	float* sp = (float*)src.data;
	float* dp = (float*)dst.data;
	for (int i = 0; i < src.rows; ++i)
	{
		for (int j = 0; j < dst.cols; ++j)
		{
			float sum = 0;
			for (int k = 0; k < ch; ++k)
				sum += sp[j * ch + k];
			dp[j] = sum;
		}
		sp += src.cols * ch;
		dp += dst.cols;
	}
}

cv::Point2f perspective_transform_apply(cv::Point2f point, cv::Size size,
	float m00, float m01, float m02, float m10, float m11, float m12,
	float m20, float m21, float m22)
{
	float div = 1.0f / std::max(size.width, size.height);
	m00 *= div;
	m01 *= div;
	m02 *= div;
	m10 *= div;
	m11 *= div;
	m12 *= div;
	m22 *= div;
	div = 1.0f / (std::max(size.width, size.height) * std::max(size.width, size.height));
	m20 *= div;
	m21 *= div;

	point.x -= size.width * 0.5f;
	point.y -= size.height * 0.5f;
	float wz = 1.0f / (point.x * m20 + point.y * m21 + m22);
	float wx = size.width * 0.5f + (point.x * m00 + point.y * m01 + m02) * wz;
	float wy = size.height * 0.5f + (point.x * m10 + point.y * m11 + m12) * wz;
	return cv::Point2f(wx, wy);
}

void perspective_transform(const cv::Mat &src, cv::Mat &dst,
	float m00, float m01, float m02, float m10, float m11, float m12,
	float m20, float m21, float m22)
{
	//af_assert(src.depth() == CV_32F);
	af_assert(src.depth() == CV_8U);

	if (dst.rows != src.rows || dst.cols != src.cols || dst.type() != src.type())
		dst = cv::Mat(src.rows, src.cols, src.type());

	//with default of bilinear interpolation
	//assume field of view is 60, modify the matrix value to reflect that
	//(basically, apply x / std::max(a.rows, a.cols), y / std::max(a.rows, a.cols)
	//before hand
	float div = 1.0f / std::max(src.rows, src.cols);
	m00 *= div;
	m01 *= div;
	m02 *= div;
	m10 *= div;
	m11 *= div;
	m12 *= div;
	m22 *= div;
	div = 1.0f / (std::max(src.rows, src.cols) * std::max(src.rows, src.cols));
	m20 *= div;
	m21 *= div;

	int ch = src.channels();
//	float* a_ptr = (float*)src.data;
//	float *b_ptr = (float*)dst.data;
	uint8_t* a_ptr = (uint8_t*)src.data;
	uint8_t *b_ptr = (uint8_t*)dst.data;
	for (int i = 0; i < dst.rows; ++i)
	{
		float cy = i - dst.rows * 0.5f;
		float crx = cy * m01 + m02; \
		float cry = cy * m11 + m12; \
		float crz = cy * m21 + m22; \
		for (int j = 0; j < dst.cols; ++j)
		{
			float cx = j - dst.cols * 0.5f;
			float wz = 1.0f / (cx * m20 + crz);
			float wx = src.cols * 0.5f + (cx * m00 + crx) * wz;
			float wy = src.rows * 0.5f + (cx * m10 + cry) * wz;
			int iwx = (int)wx;
			int iwy = (int)wy;
			wx = wx - iwx;
			wy = wy - iwy;
			int iwx1 = std::min(iwx + 1, src.cols - 1);
			int iwy1 = std::min(iwy + 1, src.rows - 1);
			if (iwx >= 0 && iwx <= src.cols && iwy >= 0 && iwy < src.rows)
				for (int k = 0; k < ch; k++)
					b_ptr[j * ch + k] = to_uint8_t(
						a_ptr[iwy * src.step1() + iwx * ch + k] * (1 - wx) * (1 - wy) +
						a_ptr[iwy * src.step1() + iwx1 * ch + k] * wx * (1 - wy) +
						a_ptr[iwy1 * src.step1() + iwx * ch + k] * (1 - wx) * wy +
						a_ptr[iwy1 * src.step1() + iwx1 * ch + k] * wx * wy);
			else
				for (int k = 0; k < ch; ++k)
					b_ptr[j * ch + k] = 0;
		}
		b_ptr += dst.step1();
	}
}

inline double corner_harris_metric(
	double dx2, double dy2, double dxdy, double K)
{
	// original Harris and Stephens operator: det(A) - K * trace(A)
	return (dx2 * dy2 - dxdy * dxdy) - K * (dx2 + dy2) * (dx2 + dy2);
}

inline double corner_shi_tomasi_metric(
	double dx2, double dy2, double dxdy, double)
{
	// Shi-Tomasi (or Kanade-Tomasi) corner detector
	// (use direct minimal eigenvalue)
	double sqrt_part = sqrt(dxdy * dxdy + (dx2 - dy2) * (dx2 - dy2) * 0.25);
	//double lambda0 = (dx2 + dy2) * 0.5 + sqrt_part;
	return (dx2 + dy2) * 0.5 - sqrt_part;
}

void corners(const cv::Mat &dx, const cv::Mat &dy,
	int block_size, int max_corners, int threshold, corners_metric_t metric,
	double coeff)
{
	af_assert(dx.rows == dy.rows && dx.cols == dy.cols);
	af_assert(dx.type() == CV_8UC1 && dy.type() == CV_8UC1);

	bool stop = false;
	int count = 0;
	std::vector<cv::Point> corners;
	for (int y = 0; y < dx.rows && !stop; ++y)
	{
		for (int x = 0; x < dx.cols; ++x)
		{
			// compute covariance matrix
			double dx2 = 0;
			double dy2 = 0;
			double dxdy = 0;

			// apply block filter
			int half = block_size / 2;
			int cnt = 0;
			for (int i = -half; i <= half; ++i)
			{
				for (int j = -half; j <= half; ++j)
				{
					if (y + i < 0 || y + i >= dx.rows ||
						x + j < 0 || x + j >= dx.cols)
						continue;

					double dx_cur = dx.at<uint8_t>(y + i, x + j);
					double dy_cur = dy.at<uint8_t>(y + i, x + j);
					dx2 += dx_cur * dx_cur;
					dy2 += dy_cur * dy_cur;
					dxdy += dx_cur * dy_cur;
					++cnt;
				}
			}
			if (cnt)
			{
				dx2 /= cnt;
				dy2 /= cnt;
				dxdy /= cnt;
			}

			double metric_val = metric(dx2, dy2, dxdy, coeff);
			if (metric_val > threshold)
			{
				corners[count].x = x;
				corners[count].y = y;
				++count;
			}

			if (count >= max_corners)
			{
				stop = true;
				break;
			}
		}  // for each column
	}  // for each row
}

void corners_harris(const cv::Mat &dx, const cv::Mat &dy, double K,
	int block_size, int max_corners, int threshold)
{
	corners(dx, dy, block_size, max_corners, threshold, &corner_harris_metric, K);
}

void corners_shi_tomasi(const cv::Mat &dx, const cv::Mat &dy,
	int block_size, int max_corners, int eigenval_th)
{
	corners(dx, dy, block_size, max_corners, eigenval_th, &corner_shi_tomasi_metric);
}

template<typename T>
void histogram(const cv::Mat &src, std::vector<int> &dst,
			   int nbins, int channel, int min_val, int max_val)
{
	int ch_num = src.channels();
	int bin_size = (max_val - min_val + 1) / nbins;
	af_assert(nbins && !((max_val - min_val) % bin_size) && "want uniform binning");
	af_assert(channel >= 0 && channel < ch_num && "illegal channel requested");

	if (nbins != (int)dst.size())
		dst.resize(nbins);
	dst.assign(dst.size(), 0);
	for (int row = 0; row < src.rows; ++row)
	{
		T *psrc = (T*)src.ptr(row);
		for (int col = 0; col < src.cols; ++col)
		{
			int bin = psrc[channel] / bin_size;
			dst[bin] += 1;
			psrc += ch_num;
		}
	}
}

template<typename T>
void histogram_normalized(const cv::Mat &src, std::vector<double> &dst,
	int nbins, int channel, int min_val, int max_val)
{
	std::vector<int> hist;
	histogram<T>(src, hist, nbins, channel, min_val, max_val);
	dst.resize(hist.size());
	double square = double(src.cols * src.rows);
	for (size_t i = 0; i < hist.size(); ++i)
		dst[i] = hist[i] / square;
}

template<typename T>
void histogram_all_channels(const cv::Mat &src, std::vector<std::vector<int> > &hists)
{
	int channels = src.channels();
	hists.clear();
	hists.resize(channels);

	for (int ch = 0; ch < channels; ++ch)
		histogram<T>(src, hists[ch], 256, ch);
}

// explicit instantiation
template
void histogram<uint8_t>(const cv::Mat &src, std::vector<int> &dst,
	int nbins, int channel, int min_val, int max_val);
template
void histogram<uint16_t>(const cv::Mat &src, std::vector<int> &dst,
	int nbins, int channel, int min_val, int max_val);
template
void histogram_normalized<uint8_t>(const cv::Mat &src, std::vector<double> &dst,
	int nbins, int channel, int min_val, int max_val);
template
void histogram_normalized<uint16_t>(const cv::Mat &src, std::vector<double> &dst,
	int nbins, int channel, int min_val, int max_val);
template
void histogram_all_channels<uint8_t>(const cv::Mat &src,
	std::vector<std::vector<int> > &hists);
template
void histogram_all_channels<uint16_t>(const cv::Mat &src,
	std::vector<std::vector<int> > &dst);

void quantize(const cv::Mat &src, const cv::Mat &weights, cv::Mat &dst,
		int nbins, int start_ch, float min, float max)
{
	af_assert(src.cols == weights.cols && src.rows == weights.rows &&
		src.channels() == weights.channels());
	af_assert(src.cols == dst.cols && src.rows == dst.rows && "dst must be allocated for quantize");
	af_assert(nbins + 1 + start_ch <= dst.channels());
	af_assert(src.channels() == 1);

	float *psrc = (float*)src.data;
	float *pweight = (float*)weights.data;
	float *res = (float*)dst.data + start_ch;
	int res_step = dst.channels();
	float mult = nbins / (max - min);
	for (int i = 0; i < src.rows * src.cols; ++i, ++psrc, ++pweight, res += res_step)
	{
		memset(res, 0, (nbins + 1) * sizeof(float));
		res[0] = *pweight;
		int bin = int((*psrc) * mult);
		res[bin + 1] = *pweight;
	}
}

void quantize_soft_32f(const cv::Mat &src, const cv::Mat &weights, cv::Mat &dst,
		int nbins, int start_ch, float min, float max)
{
	af_assert(src.cols == weights.cols && src.rows == weights.rows &&
		src.channels() == weights.channels());
	af_assert(src.cols == dst.cols && src.rows == dst.rows && "dst must be allocated for quantize");
	af_assert(nbins + start_ch <= dst.channels());
	af_assert(src.depth() == CV_32F && weights.depth() == CV_32F);
	af_assert(dst.depth() == CV_32F);
	af_assert(src.channels() == 1);

	float *psrc = (float*)src.data;
	float *pweight = (float*)weights.data;
	float *res = (float*)dst.data + start_ch;
	int res_step = dst.channels();
	float mult = nbins / (max - min);
	for (int i = 0; i < src.rows * src.cols; ++i, ++psrc, ++pweight, res += res_step)
	{
		memset(res, 0, (nbins + 1) * sizeof(float));
		res[0] = *pweight;
		float binf = (*psrc) * mult;
		int bin0 = int(binf);
		int bin1 = bin0 < (nbins - 1) ? bin0 + 1 : 0;
		binf = binf - bin0;
		res[bin0 + 1] = (*pweight) * (1.0f - binf);
		res[bin1 + 1] = (*pweight) * binf;
	}
}

void quantize_soft_8u(const cv::Mat &src, const cv::Mat &weights, cv::Mat &dst,
		int nbins, int start_ch, float min, float max)
{
	af_assert(src.cols == weights.cols && src.rows == weights.rows &&
		src.channels() == weights.channels());
	af_assert(src.cols == dst.cols && src.rows == dst.rows && "dst must be allocated for quantize");
	af_assert(nbins + start_ch <= dst.channels());
	af_assert(src.depth() == CV_32F && weights.depth() == CV_32F);
	af_assert(dst.depth() == CV_8U);
	af_assert(src.channels() == 1 && "must be only 1ch after gradient");

	float *psrc = (float*)src.data;
	float *pweight = (float*)weights.data;
	uint8_t *res = dst.data + start_ch;
	int res_step = dst.channels();
	const float mult = nbins / (max - min);
	const float mag_mult = 0.2f;
	for (int i = 0; i < src.rows * src.cols; ++i, ++psrc, ++pweight, res += res_step)
	{
		memset(res, 0, nbins + 1);
		float mag = (*pweight) * mag_mult;
		res[0] = std::min(int(mag), 255);
		float binf = (*psrc) * mult;
		int bin0 = int(binf);
		int bin1 = bin0 < (nbins - 1) ? bin0 + 1 : 0;
		binf = binf - bin0;
		res[bin0 + 1] = std::min(int(mag * (1.0f - binf)), 255);
		res[bin1 + 1] = std::min(int(mag * binf), 255);
	}
}

void integrate(const cv::Mat &src, cv::Mat &dst)
{
	if (src.depth() == CV_8U)
	{
		if (dst.rows != src.rows + 1 || dst.cols != src.cols + 1 ||
				dst.type() != CV_32SC(src.channels()))
			dst = cv::Mat(src.rows + 1, src.cols + 1, CV_32SC(src.channels()));
#ifdef HAVE_SSE
		if (src.isContinuous() && src.channels() == 10)
		{
			sse_integral_c10_32s(src.rows, src.cols, src.data, (int*)dst.data);
			return;
		}
#endif
		cv::integral(src, dst, CV_32S);
	}
	else if (src.depth() == CV_32F)
	{
		if (dst.rows != src.rows + 1 || dst.cols != src.cols + 1 ||
			dst.type() != CV_64FC(src.channels()))
			dst = cv::Mat(src.rows + 1, src.cols + 1, CV_64FC(src.channels()));
#ifdef HAVE_SSE
		if (src.isContinuous() && src.channels() == 10)
		{
			sse_integral_c10_64f(src.rows, src.cols, (float*)src.data, (double*)dst.data);
			return;
		}
#endif
		cv::integral(src, dst, CV_64F);
	}
	else
		af_assert(!"integrate(): incorrect input image");
}

void hog_classic(const cv::Mat &grangle, const cv::Mat &grmag, cv::Mat &dst,
	int nbins, int cell_w, int cell_h, cv::Mat &cn, cv::Mat &ca)
{
	af_assert(cell_w == cell_h && "zzzzz X(");
	af_assert(grangle.channels() == 1 && "1-ch gradient must be computed");
	af_assert(grangle.depth() == CV_32F && grmag.depth() == CV_32F);

	af_assert(nbins * 3 + 4 <= dst.channels());

	int drows = grangle.rows / (cell_h);
	int dcols = grangle.cols / (cell_w);
	af_assert(dst.rows == drows && dst.cols == dcols);

	if (cn.rows != drows || cn.cols != dcols || cn.type() != CV_32FC(nbins * 2))
		cn = cv::Mat(drows, dcols, CV_32FC(nbins * 2));
	if (ca.rows != drows || ca.cols != dcols || ca.type() != CV_32FC(nbins * 2))
		ca = cv::Mat(drows, dcols, CV_32FC1);

	//calculate histograms for cell_w * cell_h cells
	float *cnp = (float*)cn.data;
	float *agp = (float*)grangle.data;
	float *mgp = (float*)grmag.data;
	int cnbins = nbins * 2;
	float agmult = cnbins / 360.0f;
	float mgmult = 1.0f / 255.0f;
	for (int row = 0; row < drows * cell_h; ++row)
	{
		for (int col = 0; col < dcols * cell_w; ++col)
		{
			float agv = agp[col] * agmult;
			float mgv = mgp[col] * mgmult;

			int ag0 = (int)agv;
			int ag1 = (ag0 + 1 < cnbins) ? ag0 + 1 : 0;
			float agr0 = agv - ag0;
			float agr1 = 1.0f - agr0;

			float yp = (row + 0.5f) / cell_h - 0.5f;
			float xp = (col + 0.5f) / cell_w - 0.5f;
			int iyp = (int)yp;
			int ixp = (int)xp;
			float vy0 = yp - iyp;
			float vx0 = xp - ixp;
			float vy1 = 1.0f - vy0;
			float vx1 = 1.0f - vx0;
			if (ixp >= 0 && iyp >= 0)
			{
				int ind = iyp * dcols * cnbins + ixp * cnbins;
				cnp[ind + ag0] += agr1 * vx1 * vy1 * mgv;
				cnp[ind + ag1] += agr0 * vx1 * vy1 * mgv;
			}
			if (ixp + 1 < dcols && iyp >= 0)
			{
				int ind = iyp * dcols * cnbins + (ixp + 1) * cnbins;
				cnp[ind + ag0] += agr1 * vx0 * vy1 * mgv;
				cnp[ind + ag1] += agr0 * vx0 * vy1 * mgv;
			}
			if (ixp >= 0 && iyp + 1 < drows)
			{
				int ind = (iyp + 1) * dcols * cnbins + ixp * cnbins;
				cnp[ind + ag0] += agr1 * vx1 * vy0 * mgv;
				cnp[ind + ag1] += agr0 * vx1 * vy0 * mgv;
			}
			if (ixp + 1 < dcols && iyp + 1 < drows)
			{
				int ind = (iyp + 1) * dcols * cnbins + (ixp + 1) * cnbins;
				cnp[ind + ag0] += agr1 * vx0 * vy0 * mgv;
				cnp[ind + ag1] += agr0 * vx0 * vy0 * mgv;
			}
		}
		agp += grangle.cols;
		mgp += grmag.cols;
	}

	//calculate local normalizer
	cnp = (float*)cn.data;
	float *cap = (float*)ca.data;
	for (int row = 0; row < drows; ++row)
	{
		for (int col = 0; col < dcols; ++col, ++cap, cnp += cnbins)
		{
			*cap = 0;
			for (int k = 0; k < nbins; k++)
				*cap += (cnp[k] + cnp[k + nbins]) * (cnp[k] + cnp[k + nbins]);
		}
	}

	//normalize and put in destination
	cnp = (float*)cn.data;
	cap = (float*)ca.data;
	float *dbp = (float*)dst.data;
	int dnbins = 3 * nbins + 4;

	int indexes[4][4] = {
		{ 0, 1, dcols, dcols + 1 },
		{ 0, 1, -dcols, -dcols + 1 },
		{ 0, -1, dcols, dcols - 1 },
		{ 0, -1, -dcols, -dcols - 1 }
	};

	//compute up border
	cnp = (float*)cn.data + dcols * cnbins;
	cap = (float*)cn.data + dcols;
	dbp = (float*)dst.data + dcols * dnbins;
	for (int row = 1; row < drows - 1; ++row)
	{
		//compule left border
		cnp += cnbins;
		dbp += dnbins;
		++cap;
		for (int col = 1; col < dcols - 1; ++col)
		{
			for (int idx = 0; idx < 4; ++idx)
			{
				float norm = 1.0f / sqrtf(0.0001f +
					cap[indexes[idx][0]] + cap[indexes[idx][1]] +
					cap[indexes[idx][2]] + cap[indexes[idx][3]]);
				for (int k = 0; k < cnbins; k++)
				{
					float v = 0.5f * std::min(cnp[k] * norm, 0.2f); \
					dbp[4 + nbins + k] += v;
					dbp[idx] += v;
				}
				dbp[idx] *= 0.2357f;
				for (int k = 0; k < nbins; k++)
				{
					float v = 0.5f * std::min((cnp[k] + cnp[k + nbins]) * norm, 0.2f);
					dbp[4 + k] += v;
				}
			}
			cnp += cnbins;
			dbp += dnbins;
			++cap;
		}
		//compute right border
		cnp += cnbins;
		dbp += dnbins;
		++cap;
	}
	//TODO: compute down border

}

void hog_kernel(const cv::Mat &grangle, const cv::Mat &grmag, cv::Mat &dst, int start_ch)
{
	const int nbins = 9;
	const int ksize = 5;

	af_assert(grangle.channels() == 1 && grmag.channels() == 1);
	af_assert(grangle.depth() == CV_32F && grmag.depth() == CV_32F);
	af_assert(grangle.cols == grmag.cols && grangle.rows == grmag.rows);

	static const float kshift = -(ksize - 1) / 2.0f;
	static const float sigma = (0.5f * ksize);
	//static const float sigma = 0.3f * ((ksize - 1) * 0.5f - 1.0f) + 0.8f;
	static const float sigma2 = 2.0f * sigma * sigma;
	static float kernel[ksize * ksize];

	double ksum = 0;
	for (int y = 0; y < ksize; ++y)
	{
		for (int x = 0; x < ksize; ++x)
		{
			float val =  expf(
				-((x + kshift) * (x + kshift) + (y + kshift) * (y + kshift)) / sigma2);
			kernel[y * ksize + x] = val;
			ksum += val;
		}
	}
	for (int i = 0; i < ksize * ksize; ++i)
		kernel[i] = float(kernel[i] / ksum);

	float *pangle = (float*)grangle.data;
	float *pmag = (float*)grmag.data;
	float *res = (float*)dst.data + start_ch;
	int step = static_cast<int>(grangle.step1());
	int res_step = dst.channels();
	float mult = nbins / 180.0f;
	for (int row = 0; row < grangle.rows - ksize; ++row)
	{
		for (int col = 0; col < grangle.cols - ksize; ++col, ++pangle, ++pmag, res += res_step)
		{
			memset(res, 0, nbins * sizeof(float));

			float *ppmag = pmag;
			float *ppangle = pangle;
			int kind = 0;
			for (int y = 0; y < ksize; ++y, ppangle += step, ppmag += step)
			{
				for (int x = 0; x < ksize; ++x, ++kind)
				{
					float binf = ppangle[x] * mult;
					int bin0 = int(binf);
					int bin1 = bin0 < (nbins - 1) ? bin0 + 1 : 0;
					binf = binf - bin0;
					res[bin0] += ppmag[x] * kernel[kind] * (1.0f - binf);
					res[bin1] += ppmag[x] * kernel[kind] * binf;
				}
			} //for kernel
		} //for each col
	} //for each row
}

void floodfill(cv::Mat &mat, uint8_t background,
		uint8_t fill_with, int min_cont_size)
{
	af_assert(background != fill_with && "trying to floodfill with the same color");

	uint8_t *data = mat.data;
	int w = mat.size().width;
	int h = mat.size().height;

	for (int y = 0; y < h; y++)
	{
		for (int x = 0; x < w; x++)
		{
			uint8_t b = data[y * w + x];
			if (b == 0 || b == fill_with)
				continue;
			int size = floodfill_pixel(
					data, w, h,
					background, fill_with,
					x, y);
			if (size < min_cont_size) // erase
				floodfill_pixel(data, w, h, fill_with, 0, x, y);
//			else
//				floodfill_pixel(data, w, h, fill_with, 0, x, y);
		}
	}
}

int floodfill_pixel(uint8_t* data, int w, int h,
					uint8_t background, uint8_t fill_with,
					int start_x, int start_y)
{
	static int dx[] = { -1, -1, 0, 1, 1, 1, 0, -1 };
	static int dy[] = { 0, -1, -1, -1, 0, 1, 1, 1 };

	std::vector<int> stack_x(w * h);
	std::vector<int> stack_y(w * h);
	stack_x[0] = start_x;
	stack_y[0] = start_y;
	data[start_y * w + start_x] = fill_with;
	int pixel_counter = 1;
	int sp = 1;
	while (sp)
	{
		sp--;
		int x = stack_x[sp];
		int y = stack_y[sp];
		for (int c = 0; c < 8; c++)
		{
			int rx = x + dx[c];
			int ry = y + dy[c];
			if (rx < 0 || rx > w - 1 || ry < 0 || ry > h - 1)
				continue;
			if (data[ry * w + rx] == background)
			{
				stack_x[sp] = rx;
				stack_y[sp] = ry;
				data[ry * w + rx] = fill_with;
				pixel_counter++;
				sp++;
			}
		}
	}

	return pixel_counter;
}

int find_rects(cv::Mat &img, uint8_t start_color,
		uint8_t min_color, uint8_t result_color,
		std::vector<cv::Rect> &rects,
		int min_obj_size, int allowed_gap)
{
	af_assert(start_color != result_color && "trying to floodfill with the same color");
	af_assert(!rects.size() && "rectangles vector is not empty");
	af_assert(start_color >= min_color && min_color > result_color);

	uint8_t *data = img.data;
	int w = img.size().width;
	int h = img.size().height;

	int max_rect_square = 0;
	for (int y = 0; y < h; y++)
	{
		for (int x = 0; x < w; x++)
		{
			uint8_t b = data[y * w + x];
			if (b < start_color)
				continue;
			cv::Rect rect = find_bounding_rect(
					data, w, h,
					min_color, result_color,
					x, y, allowed_gap);

			int area = rect.area();
			if (double(rect.width) > min_obj_size * 0.01 * w ||
					double(rect.height) > min_obj_size * 0.01 * h)
				rects.push_back(rect);

			if (area > max_rect_square)
				max_rect_square = area;
		}
	}

	return max_rect_square;
}

cv::Rect find_bounding_rect(uint8_t* data, int w, int h,
		uint8_t start_color, uint8_t result_color,
		int start_x, int start_y, int allowed_gap)
{
	std::vector<int> stack_x(w * h);
	std::vector<int> stack_y(w * h);

	stack_x[0] = start_x;
	stack_y[0] = start_y;
	data[start_y * w + start_x] = result_color;
	int up_left_x = start_x;
	int up_left_y = start_y;
	int down_right_x = start_x;
	int down_right_y = start_y;
	int pixel_counter = 1;
	int sp = 1;
	while (sp)
	{
		sp--;
		int x = stack_x[sp];
		int y = stack_y[sp];
		for (int dx = -allowed_gap; dx <= allowed_gap; dx++)
		{
			for (int dy = -allowed_gap; dy <= allowed_gap; dy++)
			{
				int rx = x + dx;
				int ry = y + dy;
				if (rx < 0 || rx > w - 1 || ry < 0 || ry > h - 1)
					continue;
				if (data[ry * w + rx] >= start_color)
				{
					if (rx < up_left_x)
						up_left_x = rx;
					if (ry < up_left_y)
						up_left_y = ry;
					if (rx > down_right_x)
						down_right_x = rx;
					if (ry > down_right_y)
						down_right_y = ry;
					stack_x[sp] = rx;
					stack_y[sp] = ry;
					data[ry * w + rx] = result_color;
					pixel_counter++;
					sp++;
				}
			}
		}
	}

	cv::Rect rect;

	rect.x = up_left_x;
	rect.y = up_left_y;
	rect.width = down_right_x - up_left_x;
	rect.height = down_right_y - up_left_y;
//	rect.density = ((float)pixel_counter) / rect.rect.area();

	return rect;
}

class LuvHelperTable
{
public:

	LuvHelperTable(const int num_bins)
		: lookup_table(num_bins)
	{
		max_ind = float(num_bins - 1);
		for(int i = 0; i < num_bins; ++i)
		{
			const float x = float(i) / max_ind;
			lookup_table[i] = std::max(0.f, 116.f * powf(x, 1.0f / 3.0f) - 16.f);
		}
	}
	float operator()(const float x) const
	{
		const size_t i = static_cast<size_t>(x * max_ind);
		af_assert(i < lookup_table.size());

		return lookup_table[i];
	}

private:
	std::vector<float> lookup_table;
	float max_ind;
};

void rgb_to_luv_pix(const float r, const float g, const float b,
	float* pl, float* pu, float* pv)
{
	static const LuvHelperTable helper_table(2048);

	const float x = 0.412453f * r + 0.35758f * g + 0.180423f * b;
	const float y = 0.212671f * r + 0.71516f * g + 0.072169f * b;
	const float z = 0.019334f * r + 0.119193f * g + 0.950227f * b;

	const float x_n = 0.312713f, y_n = 0.329016f;
	const float uv_n_divisor = -2.f * x_n + 12.f * y_n + 3.f;
	const float u_n = 4.f * x_n / uv_n_divisor;
	const float v_n = 9.f * y_n / uv_n_divisor;

	const float uv_divisor = std::max((x + 15.f * y + 3.f * z), FLT_EPSILON);
	const float u = 4.f * x / uv_divisor;
	const float v = 9.f * y / uv_divisor;

	//const float l_value = std::max(0.f, ((116.f * cv::cubeRoot(y)) - 16.f));
	const float l_value = helper_table(y);
	const float u_value = 13.f * l_value * (u - u_n);
	const float v_value = 13.f * l_value * (v - v_n);

	// L in [0, 100], U in [-134, 220], V in [-140, 122]
	*pl = l_value * (255.f / 100.f);
	*pu = (u_value + 134.f) * (255.f / (220.f + 134.f));
	*pv = (v_value + 140.f) * (255.f / (122.f + 140.f));
}

cv::Mat to8b(const cv::Mat &src)
{
	cv::Mat dst;
	if (src.type() == CV_8U)
		return src.clone();
	src.convertTo(dst, CV_8U);
	return dst;
}

cv::Mat to_gray(const cv::Mat &src)
{
	if (src.channels() == 3)
	{
		cv::Mat gray;
		cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
		return gray;
	}
	else if (src.channels() == 1)
		return src.clone();
	else
		aifil::log_error("Conversion to grayscale: unknown image type");
	return cv::Mat();
}

cv::Mat to32f(const cv::Mat &src)
{
	if (src.type() == CV_32F)
		return src.clone();
	cv::Mat dst;
	src.convertTo(dst, CV_32F);
	return dst;
}

bool out_of_border(const cv::Rect &rect, const cv::Mat &mat)
{
	return rect.x < 0 || rect.x + rect.width > mat.cols ||
		rect.y < 0 || rect.y + rect.height > mat.rows;
}

// function generator for wrappers with cv::Mat input:
// determine type and call templated version
#define MAKE_FUNC_FOR_OCV_TYPES(RETVAL_TYPE, FUNC) \
RETVAL_TYPE FUNC(const cv::Mat& src) \
{ \
	if (src.depth() == CV_8U) \
		return FUNC<uint8_t>(src); \
	else if (src.depth() == CV_32S) \
		return FUNC<int>(src); \
	else if (src.depth() == CV_32F) \
		return FUNC<float>(src); \
	else if (src.depth() == CV_64F) \
		return FUNC<double>(src); \
	else \
		af_assert(!bool("unsupported matrix depth")); \
	return RETVAL_TYPE(); \
}

template<typename T>
cv::Point2d expected_value(const cv::Mat_<T> &src)
{
	af_assert(src.channels() == 1);

	cv::Point2d m(src.cols / 2.0, src.rows / 2.0);

	if (src.empty())
		return m;

	double sum = cv::sum(src)[0];
	if (sum == 0)
		return m;

	for (int y = 0; y < src.rows; ++y)
	{
		for (int x = 0; x < src.cols; ++x)
		{
			m.x += double(x) * src(y, x); // x
			m.y += double(y) * src(y, x); // y
		}
	}
	m.x /= sum;
	m.y /= sum;

	return m;
}

template<typename type>
cv::Point2d variance(const cv::Mat_<type> &src)
{
	af_assert(src.channels() == 1);

	cv::Point2d m2(0, 0);

	if (src.empty())
		return m2;

	double sum = cv::sum(src)[0];
	if (sum == 0)
		return m2;

	// M[X^2]
	for (int y = 0; y < src.rows; ++y)
	{
		for (int x = 0; x < src.cols; ++x)
		{
			m2.x += double(x) * x * src(y, x);
			m2.y += double(y) * y * src(y, x);
		}
	}
	m2.x /= sum;
	m2.y /= sum;

	// M[X]^2
	cv::Point2d m = expected_value(src);
	m.x *= m.x;
	m.y *= m.y;

	return m2 - m;
}

template<typename type_t>
double median(const cv::Mat_<type_t> &src)
{
	if (src.empty())
		return 0;

	af_assert(src.channels() == 1);

	std::vector<type_t> vec(src.total());
	if (src.cols == 1)
	{
		cv::Mat_<type_t> tmp;
		cv::transpose(src, tmp);
		auto data = tmp[0];
		std::copy(data, data + tmp.total(), &(vec[0]));
	}
	else
	{
		for (int i = 0; i < src.rows; ++i)
		{
			auto data = src[i];
			std::copy(data, data + src.cols, &(vec[i * src.cols]));
		}
	}
	std::nth_element(vec.begin(), vec.begin() + vec.size()/2, vec.end());
	return double(vec[vec.size() / 2]);
}

MAKE_FUNC_FOR_OCV_TYPES(cv::Point2d, expected_value)
MAKE_FUNC_FOR_OCV_TYPES(cv::Point2d, variance)
MAKE_FUNC_FOR_OCV_TYPES(double, median)

cv::Scalar pick_random_color()
{
	unsigned char r = get_rand_value<uint8_t>();
	unsigned char g = get_rand_value<uint8_t>();
	unsigned char b = get_rand_value<uint8_t>();
	return cv::Scalar(b, g, r);
}

}  // namespace imgproc
}  // namespace aifil
