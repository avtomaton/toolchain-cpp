#include "imgproc.h"

#include <common/stringutils.h>
#include <common/errutils.h>
#include <common/logging.h>
#include <common/profiler.h>

namespace aifil {
namespace imgproc {

// helpers
int log_function(int color, float A, float B)
{
	return B * log(A * color + 1);
}

int exp_function(int color, float A, float B)
{
	return B * (exp(A * color) - 1);
}

int power_law_function(int color, float A, float B)
{
	return A * pow(color, B);
}

int lin_function(int color, float A, float B)
{
	return A * color + B;
}

cv::Vec2f lin_transform(const cv::Vec2i &minmax, int min_val, int max_val)
{
	float A = float(max_val - min_val) / (minmax[1] - minmax[0]);
	float B = min_val - A * minmax[0];
	return cv::Vec2f(A, B);
}

void blur(const cv::Mat& src, cv::Mat& dst, int type, int size, cv::Mat &tmp)
{
	af_assert(src.type() == CV_8UC1);

	if (dst.rows != src.rows || dst.cols != src.cols || dst.type() != src.type())
		dst = cv::Mat(src.rows, src.cols, src.type());
	if (tmp.rows != src.rows || tmp.cols != src.cols || tmp.type() != src.type())
		tmp = cv::Mat(src.rows, src.cols, src.type());

	bool processed = false;
#ifdef HAVE_SSE
	int w = src.cols;
	int h = src.rows;

	if (type == 1 && (size == 3 || size == 5) && !(w % 16) && !(h % 8))
	{
		src.copyTo(tmp);
		if (size == 3)
		{
			::blur_1u8_3x3_sse((uint8_t *)dst.data, (uint8_t *)tmp.data, w, h, w);
			processed = true;
		}
		if (size == 5)
		{
			::blur_1u8_5x5_sse((uint8_t *)dst.data, (uint8_t *)tmp.data, w, h, w);
			processed = true;
		}
	}
#endif

	if (processed)
		return;

	cv::Size blur_size_cv(size, size);
	switch (type)
	{
	case 1:
		cv::blur(src, dst, blur_size_cv);
		break;
	case 2:
		cv::GaussianBlur(src, dst, blur_size_cv, 0);
		break;
	case 3:
		cv::medianBlur(src, dst, size);
		break;
	default:
		cv::blur(src, dst, blur_size_cv);
	}
}

void erode_dilate(cv::Mat &img)
{
	cv::erode(img, img, cv::Mat(), cv::Point(-1, -1), 1);
	cv::dilate(img, img, cv::Mat(), cv::Point(-1, -1), 1);
}

void sobel(const cv::Mat &src, cv::Mat &dst,
	int dx, int dy, int ksize)
{
	if (dst.rows != src.rows || dst.cols != src.cols || dst.type() != src.type())
		dst = cv::Mat(src.rows, src.cols, src.type());

	PROFILE("sobel");
	cv::Sobel(src, dst, -1, dx, dy, ksize, 1, 0, cv::BORDER_DEFAULT);
}

void sobel_v2(const cv::Mat &src, cv::Mat &dx, cv::Mat &dy)
{
	if (dx.rows != src.rows || dx.cols != src.cols || dx.type() != src.type())
		dx = cv::Mat(src.rows, src.cols, src.type());
	if (dy.rows != src.rows || dy.cols != src.cols || dy.type() != src.type())
		dy = cv::Mat(src.rows, src.cols, src.type());

	af_assert(src.depth() == CV_32F);

	PROFILE("sobel v2");
	int addr[4] = { -3, 3, -int(src.step1()), int(src.step1()) };
	float *psrc = (float*)src.data;
	float *pdx = (float*)dx.data;
	float *pdy = (float*)dy.data;

	for (int row = 1; row < src.rows - 1; ++row)
	{
		psrc = ((float*)src.data) + row * src.step1() + 3;
		pdx = ((float*)dx.data) + row * dx.step1() + 3;
		pdy = ((float*)dy.data) + row * dx.step1() + 3;
		for (int col = 1; col < src.cols - 1; ++col)
		{
			for (int ch = 0; ch < 3; ++ch)
			{
				*pdx = psrc[addr[1]] - psrc[addr[0]];
				*pdy = psrc[addr[3]] - psrc[addr[2]];
				++pdx;
				++pdy;
				++psrc;
			}
		}
	}
}

void gradient(const cv::Mat &src, cv::Mat &angle, cv::Mat &magnitude,
		int ksize, cv::Mat &dx, cv::Mat &dy)
{
	if (angle.rows != src.rows || angle.cols != src.cols ||
		(angle.channels() != 1 && angle.channels() != src.channels()))
		angle = cv::Mat(src.rows, src.cols,  CV_32FC1);
	if (magnitude.rows != src.rows || magnitude.cols != src.cols ||
		(magnitude.channels() != 1 && magnitude.channels() != src.channels()))
		magnitude = cv::Mat(src.rows, src.cols,  CV_32FC1);
	if (dx.rows != src.rows || dx.cols != src.cols || dx.type() != src.type())
		dx = cv::Mat(src.rows, src.cols, src.type());
	if (dy.rows != src.rows || dy.cols != src.cols || dy.type() != src.type())
		dy = cv::Mat(src.rows, src.cols, src.type());

	af_assert(src.depth() == CV_32F && angle.depth() == CV_32F &&
		magnitude.depth() == CV_32F && dx.depth() == CV_32F && dy.depth() == CV_32F);

	int sch = src.channels();

	sobel(src, dx, 1, 0, ksize);
	sobel(src, dy, 0, 1, ksize);
	//sobel_v2(src, dx, dy);

#ifdef HAVE_CUDA
	if (sch == 3 && src.isContinuous())
	{
		cuda_gradient_c3(src.rows, src.cols, (float*)dx.data, (float*)dy.data,
			(float*)angle.data, (float*)magnitude.data);
		return;
	}
	else if (sch == 1 && src.isContinuous())
	{
		cuda_gradient_c1(src.rows, src.cols, (float*)dx.data, (float*)dy.data,
			(float*)angle.data, (float*)magnitude.data);
		return;
	}
#endif

#ifdef HAVE_SSE
	if (sch == 3 && src.isContinuous())
	{
		sse_gradient_c3(src.rows, src.cols, (float*)dx.data, (float*)dy.data,
			(float*)angle.data, (float*)magnitude.data);
		return;
	}
	else if (sch == 1 && src.isContinuous())
	{
		sse_gradient_c1(src.rows, src.cols, (float*)dx.data, (float*)dy.data,
			(float*)angle.data, (float*)magnitude.data);
		return;
	}
#endif

	//modified atan2
	int len = src.rows * src.cols;
	float *pdx = (float*)dx.data;
	float *pdy = (float*)dy.data;
	float *pangle = (float*)angle.data;
	float *pmag = (float*)magnitude.data;
	if (angle.channels() == src.channels())
	{
		for (int i = 0; i < len * sch; ++i, ++pdx, ++pdy, ++pangle, ++pmag)
		{
			*pmag = sqrtf((*pdx) * (*pdx) + (*pdy) * (*pdy));
			//*pmag = (*pdx) * (*pdx) + (*pdy) * (*pdy);
			*pangle = cv::fastAtan2(*pdy, *pdx);
			//*pangle = atan2f(*pdy, *pdx);
			if (*pangle >= 180.0f)
				*pangle = *pangle - 180.0f;
		}
	}
	else if (angle.channels() == 1)
	{
		for (int i = 0; i < len; ++i, pdx += sch, pdy += sch, ++pangle, ++pmag)
		{
			float max_mv = -1.0f;
			int max_ind = 0;
			for (int c = 0; c < sch; ++c)
			{
				float mv = pdx[c] * pdx[c] + pdy[c] * pdy[c];
				if (mv > max_mv)
				{
					max_ind = c;
					max_mv = mv;
				}
			}
			*pmag = sqrtf(max_mv);
			//*pmag = max_mv;
			*pangle = cv::fastAtan2(pdy[max_ind], pdx[max_ind]);
//			float mpi_mult = 180.0f / 3.14159265358979323846f;
//			*pangle = 180.0f + mpi_mult * atan2(pdy[max_ind], pdx[max_ind]);
			if (*pangle >= 180.0f)
				*pangle = *pangle - 180.0f;
		}
	}
	else
		af_assert(!"incorrect dst matrixes");
}

void gradient2(const cv::Mat &src, cv::Mat &angle, cv::Mat &magnitude,
	int threshold, int /* ksize */)
{
	if (angle.rows != src.rows || angle.cols != src.cols ||
		angle.type() != src.type())
		angle = cv::Mat(src.rows, src.cols,  CV_8UC1);
	if (magnitude.rows != src.rows || magnitude.cols != src.cols ||
		magnitude.type() != src.type())
		magnitude = cv::Mat(src.rows, src.cols,  CV_8UC1);

	int ch = 1;
	int shift = 1; //ksize / 2
	af_assert(src.type() == CV_8UC1 && "other is not implemented now");
	af_assert(magnitude.type() == CV_8UC1 && "other is not implemented now");
	af_assert(angle.type() == CV_8UC1 && "gradient2 is quantized gradient version");

	angle.setTo(0);
	magnitude.setTo(0);

	int stride = static_cast<int>(src.step1());
	int addr[8] = { shift, stride * shift + shift, stride * shift,
					stride * shift - shift, -shift, -stride * shift - shift,
					-stride * shift, -stride * shift + shift };

	uint8_t *pang = angle.data + stride * shift + shift;
	uint8_t *pmag = magnitude.data + stride * shift + shift;
	for (int col = shift; col < src.cols - shift; ++col)
	{
		pang = angle.data;
		pmag = magnitude.data;
		for (int row = shift; row < src.rows - shift; ++row)
		{
			int pixel_index = src.cols * row + col;
			int diff = 0;
			int max_diff = threshold;
			for (int i = 0; i < 8; ++i)
			{
				diff = abs(src.data[pixel_index + addr[i]] - src.data[pixel_index - addr[i]]);
				if (diff > max_diff)
				{
					max_diff = diff;
					*pang = i + 1;
					*pmag = diff;
				}
			}
			++pang;
			++pmag;
		}
		pang += shift * 2 * ch;
		pmag += shift * 2 * ch;
	}
}

float f32_bilinear_1u8(IplImage *img, float x, float y)
{
	int x2 = (int)ceil(x);
	int x1 = x2-1;
	int y2 = (int)ceil(y);
	int y1 = y2-1;

	if (x1<0 || y1<0 || x2>img->width-1 || y2>img->height-1)
		return 0;

	float kx = x - x1;
	float ky = y - y1;

	float col1 = (1.0f-kx) * (*cvPtr2D(img, y1, x1)) + kx * (*cvPtr2D(img, y1, x2));
	float col2 = (1.0f-kx) * (*cvPtr2D(img, y2, x1)) + kx * (*cvPtr2D(img, y2, x2));
	float col = (1.0f-ky) * col1 + ky * col2;

	return col;
}

void histogram_local(const cv::Mat &src, std::vector<std::vector<int> > &hists,
		const cv::Rect &rect)
{
	int channels = src.channels();

	hists.clear();
	hists.resize(channels);

	int cols = src.cols;
	int rows = src.rows;

	int xl = std::max<int>(rect.x, 0);
	int yl = std::max<int>(rect.y, 0);
	int xr = std::min<int>(rect.x + rect.width - 1, cols - 1);
	int yr = std::min<int>(rect.y + rect.height - 1, rows - 1);

	for (int c = 0; c < channels; ++c)
	{
		hists[c].resize(256, 0);
		for (int y = yl; y <= yr; ++y)
		{
			const uint8_t* Mi = src.ptr(y);
			for (int x = xl; x <= xr; ++x)
				++hists[c][ Mi[x*channels + c] ];
			//if rect.x < 0
			for (int x = xl; x < -rect.x; ++x)
				++hists[c][ Mi[x*channels + c] ];
			//if (rect.x + rect.width) > cols
			for (int x = cols - 1; x >= (cols - 1) + cols - (rect.x + rect.width); --x)
				++hists[c][ Mi[x*channels + c] ];
		}
		//if rect.y < 0
		for (int y = 0; y < -rect.y; ++y)
		{
			const uint8_t* Mi = src.ptr(y);
			for (int x = xl; x <= xr; ++x)
				++hists[c][ Mi[x*channels + c] ];
			//if rect.x < 0
			for (int x = xl; x < -rect.x; ++x)
				++hists[c][ Mi[x*channels + c] ];
			//if (rect.x + rect.width) > cols
			for (int x = cols - 1; x >= (cols - 1) + cols - (rect.x + rect.width); --x)
				++hists[c][ Mi[x*channels + c] ];
		}
		//if (rect.y + rect.height) > rows
		for (int y = rows - 1; y >= (rows - 1) + rows - (rect.y + rect.height); --y)
		{
			const uint8_t* Mi = src.ptr(y);
			for (int x = xl; x <= xr; ++x)
				++hists[c][ Mi[x*channels + c] ];
			//if rect.x < 0
			for (int x = xl; x < -rect.x; ++x)
				++hists[c][ Mi[x*channels + c] ];
			//if (rect.x + rect.width) > cols
			for (int x = cols - 1; x >= (cols - 1) + cols - (rect.x + rect.width); --x)
				++hists[c][ Mi[x*channels + c] ];
		}
	}
}

void histogram_save_debug(const std::vector<int> &hists, int scale)
{
	static int counter = 0;
	int ymax = 800;
	cv::Mat histImg = cv::Mat::zeros(ymax, scale * 256, CV_8U);
	for (int s = 0; s < int(hists.size()); ++s)
	{
		cv::rectangle(histImg,
					  cv::Point(s * scale, 0),
					  cv::Point(s * scale + 1, ymax - hists[s] / 500),
					  cv::Scalar::all(255), CV_FILLED);
	}
	cv::imwrite("histogram_" + std::to_string(counter++) + ".png", histImg);
}

cv::Vec2i histogram_bounds(const std::vector<int> &hist, int threshold,
		int start_left, int start_right)
{
	int N = int(hist.size());
	int min = start_left;
	int accum = 0;
	for ( ; min < N; ++min)
	{
		if (hist[min] >= threshold)
			accum += 1;
		else
			accum = 0;

		if (accum > int(hist.size() * 0.02))
			break;
	}
	if (min == N)
		return cv::Vec2i(0, 255);
	int max = start_right;
	for ( ; max >= 0; --max)
	{
		if (hist[max] >= threshold)
			accum += 1;
		else
			accum = 0;

		if (accum > int(hist.size() * 0.02))
			break;
	}
	if (max < min)
		max = min;
	return cv::Vec2i(min, max);
}

int historgam_peak(const std::vector<int> &hist, int threshold)
{
	int N = int(hist.size());
	int accum = 0;
	int max_val = 0;
	int max_ind = 0;
	for (int idx = 0; idx < N; ++idx)
	{
		if (hist[idx] >= threshold)
			accum += 1;
		else
			accum = 0;

		if (accum > int(hist.size() * 0.02) && hist[idx] > max_val)
		{
			max_val = hist[idx];
			max_ind = idx;
		}
	}

	int start = 0;
	int finish = 255;

	accum = 0;
	for (int idx = max_ind; idx >= 0; --idx)
	{
		if (hist[idx] < max_val / 2)
			accum += 1;
		else
			accum = 0;
		if (accum > 3)
		{
			start = idx;
			break;
		}
	}
	accum = 0;
	for (int idx = max_ind; idx < N; ++idx)
	{
		if (hist[idx] < max_val / 2)
			accum += 1;
		else
			accum = 0;
		if (accum > 3)
		{
			finish = idx;
			break;
		}
	}

	return (start + finish) / 2;
}

cv::Vec2i histogram_bounds_from_peak_height_rel(
		const std::vector<int> &hist, int peak, float min_height,
		int min_left, int max_right)
{
	int accum = 0;
	int start = -1;
	int finish = -1;
	for (int idx = peak; idx >= min_left; --idx)
	{
		if (hist[idx] > int(hist[peak] * min_height))
			accum += 1;
		else
			accum = 0;
		if (accum > 2)
			start = idx;
	}
	accum = 0;
	for (int idx = peak; idx < max_right; ++idx)
	{
		if (hist[idx] > int(hist[peak] * min_height))
			accum += 1;
		else
			accum = 0;
		if (accum > 2)
			finish = idx;
	}
	return cv::Vec2i(start, finish);
}

cv::Vec2i histogram_bounds_from_peak_height_abs(
		const std::vector<int> &hist, int peak, int min_height)
{
	int N = int(hist.size());
	int accum = 0;
	int start = -1;
	int finish = -1;
	for (int idx = peak; idx >= 0; --idx)
	{
		if (hist[idx] < min_height)
			accum += 1;
		else
			accum = 0;
		if (accum > 3)
		{
			start = idx;
			break;
		}
	}
	accum = 0;
	for (int idx = peak; idx < N; ++idx)
	{
		if (hist[idx] < min_height)
			accum += 1;
		else
			accum = 0;
		if (accum > 3)
		{
			finish = idx;
			break;
		}
	}
	return cv::Vec2i(start, finish);
}


cv::Vec2i histogram_bounds_from_peak_square(
		const std::vector<int> &hist, int peak, int min_square,
		int min_left, int max_right)
{
	int idx = peak;
	int left = idx -1;
	int right = idx + 1;
	int pixel_sum = hist[idx];
	while (pixel_sum < min_square)
	{
		bool smth_done = false;
		if (left > min_left && hist[left] > hist[right])
		{
			pixel_sum += hist[left];
			--left;
			smth_done = true;
		}
		else if (right < max_right && hist[right] > hist[left])
		{
			pixel_sum += hist[right];
			++right;
			smth_done = true;
		}
		if (!smth_done)
			break;
	}
	return cv::Vec2i(left, right);
}

void histogram_truncate(std::vector<float> &fhist, int maxval)
{
	float sum = 0;
	int N = int(fhist.size());
	for (int k = 0; k < N; ++k)
	{
		if (fhist[k] > maxval)
		{
			sum += fhist[k] - maxval;
			fhist[k] = maxval;
		}
	}
	sum /= N;
	for (int k = 0; k < N; ++k)
		fhist[k] += sum;
}

template<typename T>
cv::Mat contrast_normalize_mean_var(const cv::Mat &src, double sqrt_bias)
{
	cv::Mat flt = cv_mat_convert<T>(src);

	cv::Scalar mean;

	// exactly as in python
	mean = cv::mean(flt);
	cv::Mat tmp = flt - mean;
	cv::Mat tmp1;
	cv::multiply(tmp, tmp, tmp1);
	double s = cv::sum(tmp1)[0];
	double var = s / (tmp1.cols * tmp1.rows - 1);

	// less code, less precision
//	cv::Scalar std_dev;
//	cv::meanStdDev(flt, mean, std_dev);
//	double var = std_dev[0] * std_dev[0];

	double divisor = std::max(sqrt(sqrt_bias + var), 1e-6);

	if (src.depth() == CV_32F || src.depth() == CV_64F)
		return (src - mean[0]) / divisor;
	else
		return (flt - mean[0]) / divisor;
}
template cv::Mat contrast_normalize_mean_var<float>(const cv::Mat &src, double sqrt_bias);
template cv::Mat contrast_normalize_mean_var<double>(const cv::Mat &src, double sqrt_bias);

void contrast_stretch_around_mean(const cv::Mat &src, cv::Mat &dst, float factor)
{
	// simplest contrast enhacement
	int channels_num = src.channels();
	dst = src.clone();
	cv::Mat gray;
	if (channels_num == 3)
		cv::cvtColor(src, gray, CV_BGR2GRAY);
	else
		src.copyTo(gray);
	int mean_brightness = cv::mean(gray)[0];
	uint8_t buf[256];
	for (int i = 0; i < 256; ++i)
	{
		int new_val = (i - mean_brightness) * factor + mean_brightness;
		buf[i] = to_uint8_t(new_val);
	}

	for (int i = 0; i < src.cols * src.rows * channels_num; i += channels_num)
	{
		dst.data[i] = buf[src.data[i]];
		dst.data[i + 1] = buf[src.data[i + 1]];
		dst.data[i + 2] = buf[src.data[i + 2]];
	}
}

void contrast_shift_128(const cv::Mat &src, cv::Mat &dst, float contrast)
{
	int channels = src.channels();
	dst = src.clone();

	channel_linear_stretch(dst, dst);

	float factor = (259 * (contrast + 255)) / (255 * (259 - contrast));
	int cols = dst.cols;
	int rows = dst.rows;
	if (dst.isContinuous())
	{
		cols *= rows;
		rows = 1;
	}
	for (int i = 0; i < rows; ++i)
	{
		uint8_t* Mi = dst.ptr(i);
		for (int j = 0; j < cols; ++j)
		{
			for (int c = 0; c < channels; ++c)
				Mi[j*channels + c] = to_uint8_t(factor * (Mi[j * channels + c] - 128) + 128);
		}
	}
}

void contrast_histogram_shift_stretch(
		const cv::Mat &src, cv::Mat &dst,
		cv::Scalar new_center, cv::Scalar ch_weights,
		float min_peak_part, int trunc_method, int peak_method, bool verbose)
{
	int channels_num = src.channels();
	dst = src.clone();
	cv::Vec2i bounds;
	double mult[256];
	int peak = 0;
	if (src.channels() == 1)
	{
		std::vector<int> hist;
		histogram<uint8_t>(src, hist, 256);
		peak = historgam_peak(hist);
		bounds = histogram_bounds_from_peak_height_rel(hist, peak, min_peak_part);
		for (int i = 0; i < 256; ++i)
		{
			int new_val = (i - peak) * 255 / (bounds[1] - bounds[0]) + new_center[0];
			mult[i] = double(new_val) / i;
		}
	//	printf("HIST %d %d %d\n", bounds[0], bounds[1], peak);
	}
	else
	{
		std::vector<int> hist_b;
		histogram<uint8_t>(src, hist_b, 256, 0);
		int peak_b = historgam_peak(hist_b);
		std::vector<int> hist_g;
		histogram<uint8_t>(src, hist_g, 256, 1);
		int peak_g = historgam_peak(hist_g);
		std::vector<int> hist_r;
		histogram<uint8_t>(src, hist_r, 256, 2);
		int peak_r = historgam_peak(hist_r);
		cv::Vec2i bounds_b(0, 255);
		cv::Vec2i bounds_g(0, 255);
		cv::Vec2i bounds_r(0, 255);
		if (trunc_method == HIST_TRUNC_PEAK_REL)
		{
			bounds_b = histogram_bounds_from_peak_height_rel(
					hist_b, peak_b, min_peak_part, 15, 240);
			bounds_g = histogram_bounds_from_peak_height_rel(
					hist_g, peak_g, min_peak_part, 15, 240);
			bounds_r = histogram_bounds_from_peak_height_rel(
					hist_r, peak_r, min_peak_part, 15, 240);
		}
		else if (trunc_method == HIST_TRUNC_PEAK_ABS)
		{
			int fg_pixels = 0;
			for (int i = 0; i < src.cols * src.rows * channels_num; i += channels_num)
			{
				if (src.data[i] > 0 && src.data[i + 1] > 0 && src.data[i + 2] > 0)
					++fg_pixels;
			}
			int min_abs_height = int(0.001 * fg_pixels);
			bounds_b = histogram_bounds_from_peak_height_abs(
					hist_b, peak_b, min_abs_height);
			bounds_g = histogram_bounds_from_peak_height_abs(
					hist_g, peak_g, min_abs_height);
			bounds_r = histogram_bounds_from_peak_height_abs(
					hist_r, peak_r, min_abs_height);
		}
		else if (trunc_method == HIST_TRUNC_SQUARE)
		{
			int fg_pixels = 0;
			for (int i = 0; i < src.cols * src.rows * channels_num; i += channels_num)
			{
				if (src.data[i] > 0 && src.data[i + 1] > 0 && src.data[i + 2] > 0)
					++fg_pixels;
			}
			int min_peak_square = int((1.0 - min_peak_part) * fg_pixels);
			bounds_b = histogram_bounds_from_peak_square(
					hist_b, peak_b, min_peak_square);
			bounds_g = histogram_bounds_from_peak_square(
					hist_g, peak_g, min_peak_square);
			bounds_r = histogram_bounds_from_peak_square(
					hist_r, peak_r, min_peak_square);
		}

		if (verbose)
		{
			printf("peak: %d %d %d, bounds: (%d %d) (%d %d) (%d %d)\n",
					peak_b, peak_g, peak_r,
					bounds_b[0], bounds_b[1],
					bounds_g[0], bounds_g[1],
					bounds_r[0], bounds_r[1]);
		}

		// shift peaks to histogram meaningful area center
		peak_b = (bounds_b[0] + bounds_b[1]) / 2;
		peak_g = (bounds_g[0] + bounds_g[1]) / 2;
		peak_r = (bounds_r[0] + bounds_r[1]) / 2;

		if (peak_method == HIST_PEAK_MEAN)
		{
			bounds[0] = std::min(std::min(bounds_b[0], bounds_g[0]), bounds_r[0]);
			bounds[1] = std::max(std::max(bounds_b[1], bounds_g[1]), bounds_r[1]);
			peak = (peak_b + peak_g + peak_r) / 3;
		}
		else if (peak_method == HIST_PEAK_CENTER)
		{
			bounds[0] = std::min(std::min(bounds_b[0], bounds_g[0]), bounds_r[0]);
			bounds[1] = std::max(std::max(bounds_b[1], bounds_g[1]), bounds_r[1]);
			peak = (bounds[0] + bounds[1]) / 2;
		}
		else if (peak_method == HIST_PEAK_TERRAIN)
		{
			bounds[0] = std::min(bounds_b[0], bounds_g[0]);
			bounds[1] = std::max(bounds_b[1], bounds_g[1]);
//			int w = bounds[1] - bounds[0];
//			if (w < 80)
//			{
//				bounds[1] += (80 - w) / 2;
//				if (bounds[1] > 255)
//					bounds[1] = 255;
//				bounds[0] -= (80 - w) / 2;
//				if (bounds[0] < 0)
//					bounds[0] = 0;
//			}
			peak = peak_g;
		}
		if (verbose)
			printf("peak: %d, bounds: %d %d\n", peak, bounds[0], bounds[1]);
		for (int i = 0; i < 256; ++i)
		{
			int new_val = (i - peak) * 255 / (bounds[1] - bounds[0]) + new_center[0];
			int new_b = (i - peak_b) * 255 / (bounds_b[1] - bounds_b[0]) + new_center[1];
			int new_g = (i - peak_g) * 255 / (bounds_g[1] - bounds_g[0]) + new_center[2];
			int new_r = (i - peak_r) * 255 / (bounds_r[1] - bounds_r[0]) + new_center[3];
			mult[i] = (ch_weights[0] * new_val +
					ch_weights[1] * new_b +
					ch_weights[2] * new_g +
					ch_weights[3] * new_r) / i;
		}
	}

//	uint8_t buf[256];
//	for (int i = 0; i < 256; ++i)
//	{
//		int new_val = (i - peak) * 255 / (bounds[1] - bounds[0]) + new_center;
//		mult[i] = double(new_val) / i;
//		buf[i] = to_uint8_t(new_val);
//	}

	for (int i = 0; i < src.cols * src.rows * channels_num; i += channels_num)
	{
		for (int c = 0; c < channels_num; ++c)
		{
			int color = dst.data[i + c];
			dst.data[i + c] = to_uint8_t(color * mult[color]);
		}
	}
}

void channel_linear_stretch(const cv::Mat &src, cv::Mat &dst, int min_val, int max_val)
{
	int channels = src.channels();
	cv::Vec2i minmax_vec;
	if (channels == 3)
	{
		// use only first channel
		// it's necessary for preserve aspect ratio
		std::vector<cv::Mat> mats;
		cv::split(src, mats);
		std::vector<int> hist;
		histogram<uint8_t>(mats[0], hist, 256, 0);
		// don't use too dark and too lighly areas
		minmax_vec = histogram_bounds(hist, int(src.total() * 1e-4), 10, 250);
	}
	else
	{
		std::vector<int> hist;
		histogram<uint8_t>(src, hist, 256, 0);
		minmax_vec = histogram_bounds(hist, int(src.total() * 1e-4), 5, 250);
	}

	cv::Vec2f lin_params = lin_transform(minmax_vec, min_val, max_val);

	int cols = src.cols;
	int rows = src.rows;
	dst = src.clone();

	if (src.isContinuous())
	{
		cols *= rows;
		rows = 1;
	}
	for (int c = 0; c < channels; ++c)
	{
		for (int i = 0; i < rows; ++i)
		{
			const uint8_t* ps = src.ptr(i);
			uint8_t* pd = dst.ptr(i);
			for (int j = 0; j < cols; ++j)
			{
				pd[j * channels + c] = to_uint8_t(trunc_val<float>(
						lin_params[0] * ps[j*channels + c] + lin_params[1],
						min_val, max_val));
			}
		}
	}
}

void channel_linear_stretch(const cv::Mat &src, cv::Mat &dst)
{
	channel_linear_stretch(src, dst, 0, 255);
}

void contrast_nonlinear_equalize(const cv::Mat &src, cv::Mat &dst,
		float P, float G, int (*function)(int, float, float))
{
	int channels = src.channels();
	int cols = src.cols;
	int rows = src.rows;
	dst = src.clone();
	if (src.isContinuous())
	{
		cols *= rows;
		rows = 1;
	}
	for (int c = 0; c < channels; ++c)
	{
		for (int i = 0; i < rows; ++i)
		{
			const uint8_t* Si = src.ptr(i);
			uint8_t* Mi = dst.ptr(i);
			for (int j = 0; j < cols; ++j)
				Mi[j * channels + c] = to_uint8_t(function(Si[j*channels + c], P, G));
		}
	}
}

void contrast_histogram_equalize(const cv::Mat &src, cv::Mat &dst)
{
	int channels = src.channels();
	int count_bins = 256;
	uint64_t count_pixels = src.total();

	std::vector<int> hist;
	histogram<uint8_t>(src, hist, 256);

	std::vector<float> nrm(count_bins);
	nrm[0] = (float)hist[0] / count_pixels;
	for (int k = 1; k < count_bins; ++k)
		nrm[k] = (float)hist[k] / count_pixels + nrm[k - 1];

	int cols = src.cols;
	int rows = src.rows;
	dst = src.clone();

	if (src.isContinuous())
	{
		cols *= rows;
		rows = 1;
	}
	for (int c = 0; c < channels; ++c)
	{
		for (int i = 0; i < rows; ++i)
		{
			const uint8_t* Si = src.ptr(i);
			uint8_t* Mi = dst.ptr(i);
			for (int j = 0; j < cols; ++j)
				Mi[j*channels + c] = to_uint8_t(255.0f * nrm[ Si[j*channels + c] ]);
		}
	}
}

void contrast_adaptive_equalization(const cv::Mat &src, cv::Mat &dst, int window)
{
	contrast_limit_adaptive_equalization(src, dst, window, -1);
}

void contrast_limit_adaptive_equalization(const cv::Mat &src, cv::Mat &dst,
		int window, int maxval)
{
	int channels = src.channels();
	int count_bins = 256;
	size_t count_pixels = window * window;
	int resize_factor = 16;
	/*
	resize_factor = std::min(src.cols, src.rows) / (2 * window);
	resize_factor = std::min(16, resize_factor);
	resize_factor = std::max(1, resize_factor);
	*/

	std::vector<std::vector<int> > hists;
	std::vector<float> fhist(count_bins);

	cv::Mat tmp;
	cv::resize(src, tmp, cv::Size(src.cols / resize_factor + 1, src.rows / resize_factor + 1));
	int cols = tmp.cols;
	int rows = tmp.rows;
	cv::Mat gray;
	if (src.channels() == 3)
		cv::cvtColor(tmp, gray, CV_BGR2GRAY);
	else
		tmp.copyTo(gray);
	std::vector<cv::Mat> nrm;
	nrm.resize(count_bins);
	for (int k = 0; k < count_bins; ++k)
		nrm[k] = cv::Mat::zeros(cv::Size(cols, rows), CV_32F);

	for (int y = 0; y < rows; ++y)
	{
		for (int x = 0; x < cols; ++x)
		{
			histogram_local(gray, hists, cv::Rect(x - window / 2, y - window / 2, window, window));
			if (maxval != -1) // CLAHE
			{
				fhist.assign(hists[0].begin(), hists[0].end());
				histogram_truncate(fhist, maxval);
				nrm[0].at<float>(y, x) = fhist[0] / count_pixels;
				for (int k = 1; k < count_bins; ++k)
					nrm[k].at<float>(y, x) = fhist[k] /
							count_pixels + nrm[k - 1].at<float>(y, x);
			}
			else // AHE
			{
				nrm[0].at<float>(y, x) = (float)hists[0][0] / count_pixels;
				for (int k = 1; k < count_bins; ++k)
					nrm[k].at<float>(y, x) = (float)hists[0][k] / count_pixels +
							nrm[k - 1].at<float>(y, x);
			}
		}
	}

	cols = src.cols;
	rows = src.rows;
	dst = cv::Mat::zeros(rows, cols, src.type());
	for (int c = 0; c < channels; ++c)
	{
		for (int i = 0; i < rows; ++i)
		{
			const uint8_t* ps = src.ptr(i);
			uint8_t* pd = dst.ptr(i);
			for (int j = 0; j < cols; ++j)
				pd[j * channels + c] = to_uint8_t(
						255.0f * nrm[ ps[j*channels + c] ].at<float>(
							(int)(i * tmp.rows / (float)src.rows),
							(int)(j * tmp.cols / (float)src.cols))
						);
		}
	}
}

void contrast_mask_equalize(const cv::Mat &src, cv::Mat &dst, float blur_value, float reduce_value)
{
	cv::Mat gray;
	if (src.channels() == 3)
		cv::cvtColor(src, gray, CV_BGR2GRAY);
	else
		src.copyTo(gray);
	cv::Mat tmp;
	channel_linear_stretch(gray, tmp);

	dst = 255 * cv::Mat::ones(src.rows, src.cols, CV_8UC1) - tmp; // negative
	cv::blur(dst, dst, cv::Size(blur_value, blur_value)); // blur
	dst = dst * reduce_value; // reduce
	dst += tmp; // add with original;
	channel_linear_stretch(dst, dst);
}

}  // namespace imgproc
}  // namespace aifil
