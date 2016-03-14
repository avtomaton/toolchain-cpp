/** @file scales-handler.h
 *
 *  @brief image scales handler
 *
 *  @author Victor Pogrebnyak <vp@aifil.ru>
 *
 *  $Id$
 *
 */

#ifndef AIFIL_SCALES_HANDLER_H
#define AIFIL_SCALES_HANDLER_H

#include "mat-cache.h"

#include <opencv2/core/core.hpp>

#include <list>
#include <memory>
#include <cstdint>

namespace aifil {

// image scales handler
class ScalesHandler
{
public:
	ScalesHandler();

	std::list<std::weak_ptr<MatCache> > scales;

	std::map<std::string, std::string> figures; // "type" -> "LINE ... RECT ... TEXT ..."

	std::shared_ptr<MatCache> scale_find(int w, int h, bool create_if_not_found);

	void process(uint64_t timestamp,
		const uint8_t *ch_0, int w_ch_0, int h_ch_0, int rs_ch_0,
		const uint8_t *ch_1 = 0, int w_ch_1 = 0, int h_ch_1 = 0, int rs_ch_1 = 0,
		const uint8_t *ch_2 = 0, int w_ch_2 = 0, int h_ch_2 = 0, int rs_ch_2 = 0
		);
	void set_rgb(const cv::Mat &mat);
	void invalidate(); // kill original image pointers

	const cv::Mat* get_img_gray(int w = 0, int h = 0, bool warn = true);
	const uint8_t* get_bytes_gray(int w = 0, int h = 0, bool warn = true);
	const cv::Mat* get_img_rgb(int w = 0, int h = 0, bool warn = true);

	//"good", "canonical", "dssl"
	const cv::Mat* get_image_cf(int w, int h, const std::string &method = "good");
	const cv::Mat* get_image_icf(int w, int h, const std::string &method = "good");

	void calc_avg_rgb(uint64_t ts, int w, int h, uint64_t time_for_average);

	// pointers valid from process() to process_over()
	int orig_w;
	int orig_h;
	cv::Mat orig_Y;
	cv::Mat orig_U;
	cv::Mat orig_V;
	cv::Mat orig_rgb;

	// TODO: replace with standard fps meter
	uint64_t start_timestamp;
	uint64_t timestamp;
	uint64_t real_timestamp;
	double current_fps;
	double uptime; //in seconds
	int frames_counter;

	// average image
	cv::Mat img_avg;
	uint64_t img_avg_ts;

private:
	void fps_update(uint64_t timestamp);
};

}  // namespace aifil

#endif // AIFIL_SCALES_HANDLER_H
