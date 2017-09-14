#ifndef AIFIL_MEDIA_READER_H
#define AIFIL_MEDIA_READER_H

#include "imgproc/mat-cache.hpp"

#include <string>
#include <list>
#include <memory>


namespace cv {
class VideoCapture;
}

namespace aifil {

class MovieReader
{
public:
	MovieReader(const std::string &filename);
	~MovieReader();

	int w, h;	///< width, height
	// YUV colorspace:
	uint8_t* Y;	///< luma
	uint8_t* U;	///< chroma
	uint8_t* V;	///< chroma
	int rs_Y;
	int rs_U;
	int rs_V;

	std::string info;
private:
	struct MovieReaderInternals* reader_internals;
	uint8_t *data;

public:
	// return current frame num
	int get_frame(int frame = -1);
	int cur_frame_num();
};

struct SequentialReader
{
	SequentialReader(bool use_ffmpeg = false) :
		movie(nullptr),
		cv_movie(nullptr),
		wanted_fps(25),
		cur_frame_num(0),
		ffmpeg_explicit_backend(use_ffmpeg)
	{}
	~SequentialReader();

	MovieReader *movie;
	cv::VideoCapture *cv_movie;
	bool ffmpeg_explicit_backend;
	std::list<std::string> photos;
	std::list<std::string>::iterator next_photo;
	int wanted_fps;

	std::string media_path;
	int cur_frame_num;
	std::string cur_frame_name;
	std::shared_ptr<MatCache> cur_frame;

	void setup(const std::string &path, int fps = 25);
	bool feed_frame();

	// cur_frame will be empty
	bool fast_feed_frame();

	cv::Mat get_image_gray();
	cv::Mat get_image_rgb();
};

struct FFHandle;

struct VideoStreamReader
{
	enum class ErrorStatus {OK, IDLE, FORMAT_ERROR, CODEC_ERROR, APP_ERROR};

	void open_input(const std::string &stream_addr);
	void close_input();
	cv::Mat get_frame();

	bool stop_flag = false;
	bool input_is_open = false;
	ErrorStatus current_error_status = ErrorStatus::OK;

	int video_stream_index = -1;
	int frame_w = 0;
	int frame_h = 0;

	std::shared_ptr<FFHandle> ff_handle;
};

}  // namespace aifil

#endif  // AIFIL_MEDIA_READER_H
