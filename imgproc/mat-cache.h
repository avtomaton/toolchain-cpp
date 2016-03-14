/** @file mat-cache.h
 *
 *  @brief cached image processing interface
 *
 *  @author Viktor Pogrebniak <vp@aifil.ru>
 *
 *  $Id$
 *
 */

#ifndef AIFIL_MAT_CACHE_H
#define AIFIL_MAT_CACHE_H

#include <opencv2/core/core.hpp>
#include <stdint.h>
#include <map>

namespace aifil {

struct CachedMat
{
	CachedMat() : ready(false) {}

	bool ready;
	cv::Mat mat;
};

// cached image processing
// matrix name notation:
// colorspace_{8|16|32|64}{u|s|f}

/* source images:
 * yuv422_*
 * rgb_*
 */
struct MatCache
{
	MatCache(int w, int h);
	void invalidate();
	void clear();

	void set_cv_mat(const cv::Mat &image);
	void set_yuv422(uint8_t *y, int rs_y,
					uint8_t *u, int rs_u, uint8_t *v, int rs_v);

	const cv::Mat& get_gray_8u();
	const cv::Mat& get_rgb_8u();

	int type_from_name(const std::string &name);
	CachedMat &prepare_image(const std::string &name);
	void update_image(const std::string &name, const cv::Mat &mat);

	// -------conversions---------
	void rgb_from_yuv();

	// TODO: integral channel features
//	void cf_create();
//	void icf_create(const std::string &method = "good"); // "canonical", "dssl"
//	void cf_create_canonical();
//	void cf_create_dssl();

	// properties
	int width;
	int height;
	double scale_x;
	double scale_y;

	// parameters
	int icf_hog_bins;
	int icf_color_bands;

	// gray, rgb, YUV, Luv, Lab, integral, icf_channels
	// sobel_dx, sobel_dy, grad_angle, grad_mag
	std::map<std::string, CachedMat> images;
	// just for avoid temporary objects in const references
	cv::Mat empty_mat;

	// temporaries (avoid multiple allocations)
	cv::Mat img_tmp_8u;
	cv::Mat img_tmp_32f;
};

}  // namespace aifil

#endif  // AIFIL_MAT_CACHE_H
