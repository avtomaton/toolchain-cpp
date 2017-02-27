#ifndef AIFIL_IMGPROC_FILTER_H
#define AIFIL_IMGPROC_FILTER_H

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/video/video.hpp>
#include <opencv2/imgproc/types_c.h>

#include <functional>
#include <vector>

namespace aifil {
namespace imgproc {

enum HIST_TRUNC_METHOD {
	HIST_TRUNC_PEAK_REL,
	HIST_TRUNC_PEAK_ABS,
	HIST_TRUNC_SQUARE
};
enum HIST_PEAK_METHOD {
	HIST_PEAK_MEAN,  // mean between red, green and blue peaks
	HIST_PEAK_CENTER,  // center of intensity histogram
	HIST_PEAK_TERRAIN  // center of green+blue histogram
};

// helpers
int log_function(int color, float A, float B);
int exp_function(int color, float A, float B);
int power_law_function(int color, float A, float B);
int lin_function(int color, float A, float B);
cv::Vec2f lin_transform(const cv::Vec2i &minmax, int min_val, int max_val);

/**
 * @brief Convolutional with custom filter.
 * @param src [in] input matrix.
 * @param window [in] Convolution window size.
 * @param func [in] Convolution kernel function.
 * @param border_type [in] Border type.
 * @return Output matrix.
 */

template<typename outtype>
cv::Mat_<outtype> convolution(
	const cv::Mat &src,
	const cv::Size &window, const std::function<outtype(const cv::Mat &)> &func,
	cv::BorderTypes border_type = cv::BORDER_REFLECT_101);

void blur(const cv::Mat& src, cv::Mat& dst, int type, int size, cv::Mat &tmp);
void erode_dilate(cv::Mat &img);
void sobel(const cv::Mat &src, cv::Mat &dst, int dx, int dy, int ksize = 3);
void sobel_v2(const cv::Mat &src, cv::Mat &dst, cv::Mat &dx, cv::Mat &dy);
void gradient(const cv::Mat &src, cv::Mat &angle, cv::Mat &magnitude,
	int ksize, cv::Mat &dx, cv::Mat &dy);
//quantized version, all output is uint8_t
void gradient2(const cv::Mat &src, cv::Mat &angle, cv::Mat &magnitude,
	int threshold = 0, int ksize = 3);
float f32_bilinear_1u8(IplImage *img, float x, float y);

/**
 * Median filter.
 * @param src [in] Input image.
 * @param size [in] Filter window size.
 * @return filtered image.
 */
cv::Mat filter_median(
		const cv::Mat &src, const cv::Size &size,
		cv::BorderTypes borderType = cv::BORDER_WRAP);

/**
 * Adaptive median filter.
 * @param src [in] input image.
 * @param size [in] Filter window size.
 * @return filtered image.
 */
cv::Mat_<float> filter_adaptive_median(
		const cv::Mat &src, const cv::Size &window,
		cv::BorderTypes border_type = cv::BORDER_WRAP);

/**
 * @brief Homomorphic filtering (mapping image to domain where linear filters works).
 * tmp = DCT(img)
 * for each element: tmp(y, x) *= lower +
 * (upper - lower) * (1 - 1 / (1 + exp(sqrt(x^2 + y^2) - threshold)))
 * output = iDCT(tmp)
 * @param img[in] Input image.
 * @param output[out] Filtered image.
 * @param lower[in] See above description.
 * @param upper[in] See above description.
 * @param threshold[in] See above description.
 */
void filter_homomorphic(
		const cv::Mat& img, cv::Mat& output,
		float lower, float upper, float threshold);

// channel adjustment
void channel_linear_stretch(const cv::Mat &src, cv::Mat &dst,
		int min_val, int max_val);
void channel_linear_stretch(const cv::Mat &src, cv::Mat &dst);

// histogram manipulations
void histogram_local(const cv::Mat &src, std::vector<std::vector<int> > &hists,
		const cv::Rect &rect);
void histogram_save_debug(const std::vector<int> &hists, int scale);
cv::Vec2i histogram_bounds(const std::vector<int> &hist, int threshold = 1,
		int start_left = 0, int start_right = 255);
int historgam_peak(const std::vector<int> &hist, int threshold = 1);
cv::Vec2i histogram_bounds_from_peak_height_rel(
		const std::vector<int> &hist,
		int peak, float min_height,
		int min_left = 0, int max_right = 255);
cv::Vec2i histogram_bounds_from_peak_height_abs(
		const std::vector<int> &hist,
		int peak, int min_height);
cv::Vec2i histogram_bounds_from_peak_square(
		const std::vector<int> &hist,
		int peak, int min_square,
		int min_left = 0, int max_right = 255);
void histogram_truncate(std::vector<float> &fhist, int maxval);

// contrast adjustment
// dst = (src - mean) / (sqrt(bias + var))
template<typename T>
cv::Mat contrast_normalize_mean_var(const cv::Mat &src, double sqrt_bias);
// shift around mean
void contrast_stretch_around_mean(const cv::Mat &src, cv::Mat &dst, float factor);
// shift around 128
void contrast_shift_128(const cv::Mat &src, cv::Mat &dst, float contrast);
void contrast_histogram_shift_stretch(
		const cv::Mat &src, cv::Mat &dst,
		cv::Scalar new_center = cv::Scalar(128, 128, 128, 128),
		cv::Scalar ch_weights = cv::Scalar(0, 0.33, 0.33, 0.33),
		float min_peak_part = 0.1,
		int trunc_method = HIST_TRUNC_PEAK_REL,
		int peak_method = HIST_PEAK_MEAN,
		bool verbose = false);
void contrast_nonlinear_equalize(const cv::Mat &src, cv::Mat &dst,
		float P, float G, int (*function)(int, float, float));
void contrast_histogram_equalize(const cv::Mat &src, cv::Mat &dst);
void contrast_adaptive_equalization(const cv::Mat &src, cv::Mat &dst, int window);
void contrast_limit_adaptive_equalization(
		const cv::Mat &src, cv::Mat &dst, int window, int maxval);
void contrast_mask_equalize(
		const cv::Mat &src, cv::Mat &dst, float blur_value, float reduce_value);

}  // namespace imgproc
}  // namespace aifil

#endif  // AIFIL_IMGPROC_FILTER_H
