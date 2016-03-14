
#include "../filter.h"
#include "../imgproc.h"

#include <algorithm>
#include <cstdio>
#include <string>

#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <boost/filesystem.hpp>

using namespace anfisa;
using namespace imgproc;


cv::Mat merge_image(std::vector<cv::Mat> &mats,
		const cv::Mat& dst = cv::Mat(), int transform_type = CV_HSV2BGR)
{
	cv::Mat out;
	if (!dst.empty())
		dst.copyTo(mats[2]);
	cv::merge(mats, out);
	cv::cvtColor(out, out, transform_type);
	cv::Mat small(600, 600 * out.cols / out.rows, CV_8UC3);
	cv::resize(out, small, small.size());
	return small;
}

void save_image(const std::string &name, const cv::Mat& image, bool resize = false)
{
	cv::Mat small(600, 600 * image.cols / image.rows, CV_8UC3);
	cv::resize(image, small, small.size());
	cv::imwrite(name, small);
}

int main(int argc, char *argv[])
{
	std::string filename = "field.tif";
	std::string method = "all";
	if (argc > 2)
		method = argv[2];
	if (argc > 1)
		filename = argv[1];

	boost::filesystem::path file_path(filename);
	std::string out_prefix = file_path.stem().string() + "-";

	cv::Mat input_gbr = cv::imread(filename);
	cv::Mat dst;
	std::vector<int> hist;

	printf("histogram shift+stretch\n");
	contrast_histogram_shift_stretch(
			input_gbr, dst,
			cv::Scalar(128, 128, 128, 128),
			cv::Scalar(1.0, 0, 0, 0),
//			cv::Scalar(0, 0.33, 0.33, 0.33),
			0.1, HIST_TRUNC_SQUARE, HIST_PEAK_TERRAIN, true);
	histogram<uint8_t>(dst, hist, 256, 0);
	histogram_save_debug(hist, 2);
	//save_image(out_prefix + "rgb-mean-advanced.tif", dst, false);
	save_image("small2-" + out_prefix + ".png", dst, true);

	return 0;

	printf("contrast_shift\n");
	contrast_stretch_around_mean(input_gbr, dst, 4.0);
	//dst *= 0.5;
	histogram<uint8_t>(dst, hist, 256, 0);
	histogram_save_debug(hist, 2);
	save_image(out_prefix + "rgb-mean-simple.tif", dst);

	int ahe_window = 101;
	printf("contrast limit adaptive equalization\n");
	int ahe_maxval = 200;
	contrast_limit_adaptive_equalization(input_gbr, dst, ahe_window, ahe_maxval);
	histogram<uint8_t>(dst, hist, 256, 0);
	histogram_save_debug(hist, 2);
	save_image(out_prefix + "rgb-lahe.tif", dst);

	float exp_a = 0.007;
	float exp_b = 128;
	printf("exp contrasting\n");
	contrast_nonlinear_equalize(input_gbr, dst, 1.0 / exp_b, 1.0 / exp_a, exp_function);
	histogram<uint8_t>(dst, hist, 256, 0);
	histogram_save_debug(hist, 2);
	save_image(out_prefix + "rgb-exp.tif", dst);

	{
		cv::Mat input_lab;
		cv::cvtColor(input_gbr, input_lab, CV_BGR2Lab);
		std::vector<cv::Mat> mats_lab;
		cv::split(input_lab, mats_lab);

		histogram<uint8_t>(mats_lab[0], hist, 256, 0);
		histogram_save_debug(hist, 2);

		channel_linear_stretch(mats_lab[0], mats_lab[0]);
		histogram<uint8_t>(mats_lab[0], hist, 256, 0);

		histogram_save_debug(hist, 2);
		cv::imwrite(out_prefix + "lab.tif", merge_image(mats_lab, cv::Mat(), CV_Lab2BGR));
	}

	std::vector<cv::Mat> mats_hsv;
	{
		cv::Mat input_hsv;
		cv::cvtColor(input_gbr, input_hsv, CV_BGR2HSV);
		cv::split(input_hsv, mats_hsv);
	}

	cv::Mat image = mats_hsv[2].clone();
	//image = input;

	histogram<uint8_t>(mats_hsv[1], hist, 256, 0);
	histogram_save_debug(hist, 2);
	histogram<uint8_t>(mats_hsv[2], hist, 256, 0);
	histogram_save_debug(hist, 2);

	//channel_linear_stretch(mats[1], mats[1]);  // stretch saturation
	//channel_linear_stretch(mats[2], mats[2]);  // stretch value

	histogram<uint8_t>(mats_hsv[1], hist, 256, 0);
	histogram_save_debug(hist, 2);

	// increase saturation
	histogram<uint8_t>(mats_hsv[1], hist, 256, 0);
	int mean_saturation = cv::mean(mats_hsv[1])[0];
	cv::Vec2i sat_min_max = histogram_bounds(hist, 5, 5, 250);
	// cv::Mat sat = mats_hsv[1] * int(255.0 / sat_min_max[1]);
	printf("SAT: %d\n", mean_saturation);
	if (mean_saturation < 30)
	{
		cv::Mat sat = mats_hsv[1] * 3.0;
		sat.copyTo(mats_hsv[1]);
	}

	printf("linear_contrasting\n");
//	channel_linear_equalize(input_gbr, dst);
//	cv::imwrite("linear.tif", dst);
	channel_linear_stretch(image, dst);
	histogram<uint8_t>(dst, hist, 256, 0);
	histogram_save_debug(hist, 2);
	cv::imwrite(out_prefix + "linear.tif", merge_image(mats_hsv, dst));

	printf("contrast_adjusting\n");
	float contrast = 50;
	contrast_shift_128(image, dst, contrast);
	histogram<uint8_t>(dst, hist, 256, 0);
	histogram_save_debug(hist, 2);
	cv::imwrite(out_prefix + "hsv-linear-shift.tif", merge_image(mats_hsv, dst));

	printf("exp_contrasting\n");
	contrast_nonlinear_equalize(image, dst, 1.0 / exp_b, 1.0 / exp_a, exp_function);
	histogram<uint8_t>(dst, hist, 256, 0);
	histogram_save_debug(hist, 2);
	cv::imwrite(out_prefix + "hsv-exp.tif", merge_image(mats_hsv, dst));

	/*
	float a = 0.01;
	float b = 128;
	printf("log_contrasting\n");
	contrast_nonlinear_equalize(image, dst, a, b, log_function);
	histogram<uint8_t>(dst, hist, 256, 0);
	histogram_save_debug(hist, 2);
	cv::imwrite(out_prefix + "log.tif", merge_resize(mats_hsv, dst));

	printf("equalization\n");
	contrast_histogram_equalize(image, dst);
	histogram<uint8_t>(dst, hist, 256, 0);
	histogram_save_debug(hist, 2);
	cv::imwrite(out_prefix + "histogram.tif", merge_resize(mats_hsv, dst));
	*/

	printf("adaptive_equalization\n");
	contrast_adaptive_equalization(image, dst, ahe_window);
	histogram<uint8_t>(dst, hist, 256, 0);
	histogram_save_debug(hist, 2);
	cv::imwrite(out_prefix + "hsv-ahe.tif", merge_image(mats_hsv, dst));

	printf("limited adaptive equalization\n");
	contrast_limit_adaptive_equalization(image, dst, ahe_window, ahe_maxval);
	histogram<uint8_t>(dst, hist, 256, 0);
	histogram_save_debug(hist, 2);
	cv::imwrite(out_prefix + "hsv-lahe.tif", merge_image(mats_hsv, dst));

	printf("local_contrasting\n");
	float blur_value = 61;
	float reduce_value = 0.5;
	contrast_mask_equalize(image, dst, blur_value, reduce_value);
	histogram<uint8_t>(dst, hist, 256, 0);
	histogram_save_debug(hist, 2);
	cv::imwrite(out_prefix + "local.tif", merge_image(mats_hsv, dst));
}
