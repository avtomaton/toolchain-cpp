/** @file scales-handler.cpp
 *
 *  @brief image scales handler
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

#include "scales-handler.hpp"

#include "imgproc.hpp"

#include <common/errutils.hpp>
#include <common/logging.hpp>
#include <common/profiler.hpp>

#include <opencv2/opencv.hpp>

extern "C" {
#include <libswscale/swscale.h>
}

#include <stdint.h>

namespace aifil {

ScalesHandler::ScalesHandler()
{
	start_timestamp = 0;
	timestamp = 0;
	real_timestamp = 0;
	current_fps = 25.0;
	uptime = 0;
	frames_counter = 0;

	img_avg_ts = 0;
}

std::shared_ptr<MatCache> ScalesHandler::scale_find(int w, int h, bool create_if_not_found)
{
	for (auto x = scales.begin(); x != scales.end(); )
	{
		std::shared_ptr<MatCache> r = x->lock();
		if (!r)
		{
			x = scales.erase(x);
			continue;
		}
		if (r->width == w && r->height == h)
			return r;
		++x;
	}
	std::shared_ptr<MatCache> r;
	if (create_if_not_found)
	{
		r.reset(new MatCache(w, h));
		scales.push_back(r);
	}
	return r;
}

void ScalesHandler::fps_update(uint64_t ts)
{
	if (!frames_counter)
		start_timestamp = ts;

	double smoothness = 0.01;
	double diff = static_cast<double>(ts - start_timestamp - timestamp);
	if (diff > 0)
		current_fps = current_fps * (1.0 - smoothness) + (1000000.0 / diff) * smoothness;

	//prevent too small fps
	if (current_fps < 0.001)
		current_fps = 0.001;

	real_timestamp = ts;
	timestamp = ts - start_timestamp;
	uptime = static_cast<double>(timestamp) / 1000000.0;
}

void ScalesHandler::process(uint64_t ts,
	const uint8_t *ch_0, int w_ch_0, int h_ch_0, int rs_ch_0,
	const uint8_t *ch_1, int w_ch_1, int h_ch_1, int rs_ch_1,
	const uint8_t *ch_2, int w_ch_2, int h_ch_2, int rs_ch_2)
{
	af_assert(ch_0 && w_ch_0 && h_ch_0);
	fps_update(ts);
	++frames_counter;
	invalidate();

	orig_w = w_ch_0;
	orig_h = h_ch_0;
	orig_Y = cv::Mat(h_ch_0, w_ch_0, CV_8UC1, (void*)ch_0, rs_ch_0);
	if (ch_1 && ch_2)
	{
		orig_U = cv::Mat(h_ch_1, w_ch_1, CV_8UC1, (void*)ch_1, rs_ch_1);
		orig_V = cv::Mat(h_ch_2, w_ch_2, CV_8UC1, (void*)ch_2, rs_ch_2);
	}
}

void ScalesHandler::set_rgb(const cv::Mat &mat)
{
	invalidate();
	orig_rgb = mat;
	orig_w = orig_rgb.cols;
	orig_h = orig_rgb.rows;
}

void ScalesHandler::invalidate()
{
	orig_Y = cv::Mat();
	orig_U = cv::Mat();
	orig_V = cv::Mat();
	orig_rgb = cv::Mat();
	for (auto x = scales.begin(); x != scales.end(); ++x)
	{
		std::shared_ptr<MatCache> r = x->lock();
		if (!r)
			x = scales.erase(x);
		else
			r->invalidate();
	}
}

const cv::Mat* ScalesHandler::get_img_gray(int w, int h, bool warn)
{
	if (w == 0 || h == 0)
	{
		w = orig_w;
		h = orig_h;
	}
	std::shared_ptr<MatCache> s = scale_find(w, h, false);
	if (s)
		return &s->get_gray_8u();
	else
	{
		if (warn)
			log_warning("IMGPROC: get_img_gray(%i,%i) requested, "
						"but no scale of this size found", w, h);
		return 0;
	}
}

const uint8_t* ScalesHandler::get_bytes_gray(int w, int h, bool warn)
{
	const cv::Mat* mat = get_img_gray(w, h, warn);
	return mat ? mat->data : 0;
}

const cv::Mat* ScalesHandler::get_img_rgb(int w, int h, bool warn)
{
	if (w == 0 || h == 0)
	{
		w = orig_w;
		h = orig_h;
	}
	std::shared_ptr<MatCache> s = scale_find(w, h, false);
	if (s)
		return &s->get_rgb_8u();
	else
	{
		if (warn)
			log_warning("IMGPROC: get_img_rgb(%i,%i) requested, "
						"but no scale of this size found", w, h);
		return 0;
	}
}

const cv::Mat* ScalesHandler::get_image_cf(int w, int h, const std::string &type)
{
	std::shared_ptr<MatCache> s = scale_find(w, h, false);
	if (!s)
	{
		log_warning("IMGPROC: get_img_cf(%i,%i) requested, "
					"but no scale of this size found", w, h);
		return 0;
	}

	if (!s->icf_channels_mat_ready)
	{
		get_img_rgb(w, h); //force RGB
		if (s->icf_color_bands == 1)
			get_img_gray(w, h); //force grayscale
		if (type == "good")
			s->cf_create();
		else if (type == "canonical")
			s->cf_create_canonical();
		else if (type == "dssl")
			s->cf_create_dssl();
		else
		{
			log_warning("ICF: unknown channels type requested");
			return 0;
		}
	}
	else
		af_assert(!s->icf_channels_mat.empty());

	if (s->icf_channels_mat_ready)
		return &s->icf_channels_mat;
	else
		return 0;
}

const cv::Mat* ScalesHandler::get_image_icf(int w, int h, const std::string &type)
{
	std::shared_ptr<MatCache> s = scale_find(w, h, false);
	if (!s)
	{
		log_warning("IMGPROC: get_img_icf(%i,%i) requested, "
					"but no scale of this size found", w, h);
		return 0;
	}

	if (!s->icf_integral_ready)
	{
		get_img_rgb(w, h); //force RGB
		if (s->icf_color_bands == 1)
			get_img_gray(w, h); //force grayscale
		s->icf_create(type);
	}
	else
		af_assert(!s->icf_integral.empty());

	if (s->icf_integral_ready)
		return &s->icf_integral;
	else
		return 0;
}

void ScalesHandler::calc_avg_rgb(uint64_t ts, int w, int h, uint64_t time_for_average)
{
	const int min_ts_dist = 1;
	uint64_t ts_dist = ts - img_avg_ts;
	if (img_avg_ts != 0 && ts_dist < min_ts_dist * 1000000UL)
		return;

	//ProfilerAccumulator a("ANAL2 WEIGHT");
	bool initial_state = (img_avg.cols != w || img_avg.rows != h || img_avg_ts == 0);

	const cv::Mat* rgb = get_img_rgb(w, h);
	if (!rgb)
		return;
	img_avg_ts = ts;
	if (img_avg.cols != w || img_avg.rows != h)
		img_avg = cv::Mat(h, w, rgb->type());

	// small fraud but it works when (time_for_average >>> ts_dist)
	// try to compute average image for last time_for_average
	double alpha = double(ts_dist) / double(time_for_average);
	if (initial_state)
		rgb->copyTo(img_avg);
	else
		cv::addWeighted(*rgb, alpha, img_avg, 1.0 - alpha, 0, img_avg);
}

}  // namespace aifil
