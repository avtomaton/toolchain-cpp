/** @file mat-cache.cpp
 *
 *  @brief cached image processing
 *
 *  @author Victor Pogrebnyak <vp@aifil.ru>
 *
 *  $Id$
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "mat-cache.hpp"

#include "imgproc.hpp"

#include <common/errutils.hpp>
#include <common/logging.hpp>
#include <common/profiler.hpp>
#include <common/stringutils.hpp>

#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/imgproc/types_c.h>
//extern "C" {
//#include <libswscale/swscale.h>
//}

#include <stdint.h>

namespace aifil {

MatCache::MatCache(int w, int h)
{
	icf_hog_bins = 6;
	icf_color_bands = 3;

	width = w;
	height = h;
	scale_x = 1.0;
	scale_y = 1.0;
}

void MatCache::invalidate()
{
	for (auto it = images.begin(); it != images.end(); ++it)
		it->second.ready = false;
}

void MatCache::clear()
{
	images.clear();
}

void MatCache::set_cv_mat(const cv::Mat &image)
{
	invalidate();

	af_assert(image.depth() == CV_8U);
	if (image.channels() == 1)
		update_image("gray_8u", image);
	else if (image.channels() == 3)
	{
		cv::Mat mat;
		cv::cvtColor(image, mat, CV_BGR2RGB);
		update_image("rgb_8u", mat);
	}
	else if (image.channels() == 4)
	{
		cv::Mat mat;
		cv::cvtColor(image, mat, CV_BGRA2RGB);
		update_image("rgb_8u", mat);
	}
	else
		aifil::except(false, "MatCache: incorrect input channels num: " +
			   std::to_string(image.channels()));
}

void MatCache::set_yuv422(uint8_t *y, int rs_y,
						  uint8_t *u, int rs_u, uint8_t *v, int rs_v)
{
	invalidate();

	cv::Mat y_mat(height, width, CV_8UC1, (void*)y, rs_y);
	update_image("gray_8u", y_mat);
	cv::Mat u_mat(height / 2, width / 2, CV_8UC1, (void*)u, rs_u);
	update_image("yuv422_u_8u", u_mat);
	cv::Mat v_mat(height / 2, width / 2, CV_8UC1, (void*)v, rs_v);
	update_image("yuv422_v_8u", v_mat);
}

const cv::Mat &MatCache::get_gray_8u()
{
	auto it = images.find("gray_8u");
	if (it == images.end() || it->second.ready == false)
	{
		// check other source image
		auto it2 = images.find("rgb_8u");
		if (it2 == images.end() || it2->second.ready == false)
			return empty_mat;

		// actual conversion
		if (it == images.end())
			it = images.insert(std::make_pair(std::string("gray_8u"), CachedMat())).first;

		cv::cvtColor(it2->second.mat, it->second.mat, CV_RGB2GRAY);
		it->second.ready = true;
	}

	return it->second.mat;
}

const cv::Mat &MatCache::get_rgb_8u()
{
	auto it = images.find("rgb_8u");
	if (it == images.end() || it->second.ready == false)
	{
		// check other source image
		rgb_from_yuv();
		it = images.find("rgb_8u");
		if (it == images.end() || it->second.ready == false)
			return empty_mat;
	}

	return it->second.mat;
}

int MatCache::type_from_name(const std::string &name)
{
	int depth = CV_8U;
	int channels = 1;

	if (aifil::endswith(name, "_8u"))
		depth = CV_8U;
	else if (aifil::endswith(name, "_16u"))
		depth = CV_16U;
	else if (aifil::endswith(name, "_16s"))
		depth = CV_16S;
	else if (aifil::endswith(name, "_32s"))
		depth = CV_32S;
	else if (aifil::endswith(name, "_32f"))
		depth = CV_32F;
	else if (aifil::endswith(name, "_64f"))
		depth = CV_64F;
	else
		af_assert(0);  // incorrect name

	if (name.find("gray") == 0)
		channels = 1;
	else if (name.find("rgb") == 0)
		channels = 3;
	else if (name.find("rgba") == 0)
		channels = 4;
	else if (name.find("yuv422") == 0)
		channels = 1;
	else if (name.find("yuv422") == 0)
		channels = 1;
	else
		af_assert(0);  // incorrect name

	return CV_MAKETYPE(depth, channels);
}

CachedMat& MatCache::prepare_image(const std::string &name)
{
	CachedMat &item = images[name];
	if (item.mat.cols != width || item.mat.rows != height)
		item.mat = cv::Mat(height, width, type_from_name(name));
	return item;
}

void MatCache::update_image(const std::string &name, const cv::Mat &mat)
{
	CachedMat &item = images[name];
	mat.copyTo(item.mat);
	item.ready = true;
}

void MatCache::rgb_from_yuv()
{
	auto it_y = images.find("gray_8u");
	auto it_u = images.find("yuv422_u_8u");
	auto it_v = images.find("yuv422_v_8u");
	if (it_y == images.end() || it_y->second.ready == false
		|| it_y == images.end() || it_y->second.ready == false
		|| it_y == images.end() || it_y->second.ready == false)
		return;

	CachedMat& rgb = prepare_image("rgb_8u");
	imgproc::rgb_from_yuv(it_y->second.mat, it_u->second.mat, it_v->second.mat, rgb.mat);
	rgb.ready = true;

//	cv::Mat bgr;
//	cv::cvtColor(rgb.mat, bgr, CV_RGB2BGR);
//	cv::imshow("test-ffmpeg", bgr);
//	cv::waitKey();
}

/*
void MatCache::icf_create(const std::string &method)
{
	if (icf_integral_ready)
		return;

	PROFILE("ICF compute");
	if (!icf_channels_mat_ready)
	{
		if (method == "good")
			cf_create();
		else if (method == "canonical")
			cf_create_canonical();
		else if (method == "dssl")
			cf_create_dssl();
		else
		{
			aifil::log_warning("unknown ICF type requested");
			return;
		}
	}

	if (icf_channels_mat_ready)
	{
		PROFILE("ICF integrate");
		imgproc::integrate(icf_channels_mat, icf_integral);
		icf_integral_ready = true;
	}
}

void MatCache::cf_create()
{
	if (icf_channels_mat_ready)
		return;

	if (!img_rgb_ready)
	{
		aifil::log_warning("CACHE: CF requested but don't have input RGB");
		return;
	}

	int ich = 3;
	int channels = 1 + icf_hog_bins + icf_color_bands;
	af_assert(icf_color_bands <= ich && "need more input channels");

	if (img_rgb_32f.empty())
		img_rgb_32f = cv::Mat(height, width, CV_32FC(ich));
	if (icf_sobel_dx.empty())
		icf_sobel_dx = cv::Mat(height, width, CV_32FC(ich));
	if (icf_sobel_dy.empty())
		icf_sobel_dy = cv::Mat(height, width, CV_32FC(ich));
	if (icf_grad_angle.empty())
		icf_grad_angle = cv::Mat(height, width, CV_32FC1);
	if (icf_grad_mag.empty())
		icf_grad_mag = cv::Mat(height, width, CV_32FC1);
	if (icf_channels_mat.empty())
		icf_channels_mat = cv::Mat(height, width, CV_32FC(channels));

	if (!img_rgb_32f_ready)
		img_rgb.convertTo(img_rgb_32f, CV_32F);
	af_assert(img_rgb_32f.channels() == 3);
	img_rgb_32f_ready = true;

	{
		PROFILE("CF gradient");
		imgproc::gradient(img_rgb_32f, icf_grad_angle, icf_grad_mag,
			3, icf_sobel_dx, icf_sobel_dy);
	}

	int ch_start = 0;
	//0: gradient magnitude, 1-6: HOG bins
	{
		PROFILE("CF quantize");
		imgproc::quantize_soft_32f(icf_grad_angle, icf_grad_mag, icf_channels_mat,
			icf_hog_bins, ch_start, 0, 179.99f);
		ch_start = icf_hog_bins + 1;
	}

	if (icf_color_bands == 3)
	{
		PROFILE("CF colors");
		//7-8-9: LUV/RGB/HSV channels
		if (true)
		{
			float *psrc = (float*)img_rgb_32f.data;
			float *res = (float*)icf_channels_mat.data;
			int res_step = icf_channels_mat.channels();
			res += ch_start;
			for (int i = 0; i < img_rgb_32f.rows * img_rgb_32f.cols;
				 ++i, psrc += 3, res += res_step)
			{
				//res[0] = psrc[0];
				//res[1] = psrc[1];
				//res[2] = psrc[2];
				float mult = 1.0f / 255.0f;
				imgproc::rgb_to_luv_pix(psrc[0] * mult, psrc[1] * mult, psrc[2] * mult,
					res, res + 1, res + 2);
			}
		}
		else
		{
			int from_to[] = { 0, ch_start, 1, ch_start + 1, 2, ch_start + 2 };
			cv::mixChannels(&img_rgb_32f, 1, &icf_channels_mat, 1, from_to, 3);
		}
		ch_start += 3;
	}
	else if (icf_color_bands == 1)
	{
		if (!img_gray_32f_ready)
		{
			if (!img_gray_8u_ready)
				cv::cvtColor(img_rgb, img_gray_8u, CV_RGB2GRAY);
			img_gray_8u_ready = true;
			img_gray_8u.convertTo(img_gray_32f, CV_32F);
			af_assert(img_gray_32f.channels() == 1);
			img_gray_32f_ready = true;
		}
		int from_to[] = { 0, ch_start };
		cv::mixChannels(&img_gray_32f, 1, &icf_channels_mat, 1, from_to, 1);
		ch_start += 1;
	}
	else if (icf_color_bands != 0)
		aifil::log_warning("ICF: can't create colors");

	icf_channels_mat_ready = true;
}

void MatCache::cf_create_canonical()
{
	if (icf_channels_mat_ready)
		return;

	if (!img_rgb_ready)
	{
		aifil::log_warning("CACHE: CF requested but don't have input RGB");
		return;
	}

	int ich = 3;
	int channels = 1 + icf_hog_bins + icf_color_bands;
	af_assert(icf_color_bands <= ich && "need more input channels");

	if (img_rgb_32f.empty())
		img_rgb_32f = cv::Mat(height, width, CV_32FC(ich));
	if (icf_sobel_dx.empty())
		icf_sobel_dx = cv::Mat(height, width, CV_32FC(ich));
	if (icf_sobel_dy.empty())
		icf_sobel_dy = cv::Mat(height, width, CV_32FC(ich));
	if (icf_grad_angle.empty())
		icf_grad_angle = cv::Mat(height, width, CV_32FC1);
	if (icf_grad_mag.empty())
		icf_grad_mag = cv::Mat(height, width, CV_32FC1);
	if (icf_channels_mat.empty())
		icf_channels_mat = cv::Mat(height, width, CV_32FC(channels));

	if (!img_rgb_32f_ready)
		img_rgb.convertTo(img_rgb_32f, CV_32F);
	af_assert(img_rgb_32f.channels() == 3);
	img_rgb_32f_ready = true;

	if (icf_color_bands == 3)
	{
		float *psrc = (float*)img_rgb_32f.data;
		float *res = (float*)icf_channels_mat.data;
		int res_step = icf_channels_mat.channels();
		for (int i = 0; i < img_rgb_32f.rows * img_rgb_32f.cols; ++i, psrc += ich, res += res_step)
			imgproc::rgb_to_luv_pix(psrc[0] / 255.0f, psrc[1] / 255.0f, psrc[2] / 255.0f,
				res, res + 1, res + 2);
	}
	else if (icf_color_bands == 1)
	{
		//0: brightness
		if (!img_gray_32f_ready)
		{
			if (!img_gray_8u_ready)
				cv::cvtColor(img_rgb, img_gray_8u, CV_RGB2GRAY);
			img_gray_8u_ready = true;
			img_gray_8u.convertTo(img_gray_32f, CV_32F);
			af_assert(img_gray_32f.channels() == 1);
			img_gray_32f_ready = true;
		}
		int from_to[] = { 0, 0 };
		cv::mixChannels(&img_gray_32f, 1, &icf_channels_mat, 1, from_to, 1);
	}
	else
		af_assert(!"CF create: incorrect parameters/input");

	{
		PROFILE("CF gradient");
		imgproc::gradient(img_rgb_32f, icf_grad_angle, icf_grad_mag,
			3, icf_sobel_dx, icf_sobel_dy);
		float magnitude_scaling = 1.0f / sqrtf(2.0f); // regularize it to 0~1
		icf_grad_mag *= magnitude_scaling;
	}

	//ich: gradient magnitude, ich+1...ich+6: HOG bins
	{
		PROFILE("CF quantize");
		imgproc::quantize_soft_32f(icf_grad_angle, icf_grad_mag, icf_channels_mat,
			icf_hog_bins, icf_color_bands, 0, 179.99f);
	}

	icf_channels_mat_ready = true;
}

void MatCache::cf_create_dssl()
{
	if (icf_channels_mat_ready)
		return;

	if (!img_rgb_ready)
	{
		aifil::log_warning("CACHE: CF requested but don't have input RGB");
		return;
	}

	int ich = 3;
	int channels = 1 + icf_hog_bins + icf_color_bands;
	af_assert(icf_color_bands <= ich && "need more input channels");

	if (img_rgb_32f.empty())
		img_rgb_32f = cv::Mat(height, width, CV_32FC(ich));
	if (icf_sobel_dx.empty())
		icf_sobel_dx = cv::Mat(height, width, CV_32FC(ich));
	if (icf_sobel_dy.empty())
		icf_sobel_dy = cv::Mat(height, width, CV_32FC(ich));
	if (icf_grad_angle.empty())
		icf_grad_angle = cv::Mat(height, width, CV_32FC1);
	if (icf_grad_mag.empty())
		icf_grad_mag = cv::Mat(height, width, CV_32FC1);
	if (icf_channels_mat.empty())
		icf_channels_mat = cv::Mat(height, width, CV_8UC(channels));

	if (!img_rgb_32f_ready)
		img_rgb.convertTo(img_rgb_32f, CV_32F);
	af_assert(img_rgb_32f.channels() == 3);
	img_rgb_32f_ready = true;

	{
		PROFILE("CF gradient");
		imgproc::gradient(img_rgb_32f, icf_grad_angle, icf_grad_mag,
			3, icf_sobel_dx, icf_sobel_dy);
	}

	int ch_start = 0;
	//0: gradient magnitude, 1-6: HOG bins
	{
		PROFILE("CF quantize");
		imgproc::quantize_soft_8u(icf_grad_angle, icf_grad_mag, icf_channels_mat,
			icf_hog_bins, ch_start, 0, 179.99f);
		ch_start = icf_hog_bins + 1;
	}

	if (icf_color_bands == 3)
	{
		PROFILE("CF colors");
		//7-8-9: LUV/RGB/HSV channels
		float tmp[3];
		float *psrc = (float*)img_rgb_32f.data;
		uint8_t *res = icf_channels_mat.data;
		int res_step = icf_channels_mat.channels();
		res += ch_start;
		const float recip_255 = 1.0f / 255.0f;
		for (int i = 0; i < img_rgb_32f.rows * img_rgb_32f.cols; ++i, psrc += 3, res += res_step)
		{
			//res[0] = psrc[0];
			//res[1] = psrc[1];
			//res[2] = psrc[2];

			imgproc::rgb_to_luv_pix(psrc[0] * recip_255, psrc[1] * recip_255, psrc[2] * recip_255,
				tmp, tmp + 1, tmp + 2);
			res[0] = uint8_t(tmp[0]);
			res[1] = uint8_t(tmp[1]);
			res[2] = uint8_t(tmp[2]);
		}
		ch_start += 3;
	}
	else if (icf_color_bands == 1)
	{
		if (!img_gray_8u_ready)
			cv::cvtColor(img_rgb, img_gray_8u, CV_RGB2GRAY);
		img_gray_8u_ready = true;
		int from_to[] = { 0, ch_start };
		cv::mixChannels(&img_gray_8u, 1, &icf_channels_mat, 1, from_to, 1);
		ch_start += 1;
	}
	else if (icf_color_bands != 0)
		aifil::log_warning("ICF create: can't create colors");

	icf_channels_mat_ready = true;
}
*/


}  // namespace aifil
