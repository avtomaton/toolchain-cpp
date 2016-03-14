#ifndef AIFIL_MEDIA_READER_H
#define AIFIL_MEDIA_READER_H

#include "imgproc/mat-cache.h"

#include <string>
#include <list>
#include <memory>

namespace aifil {

class MovieReader
{
public:
	MovieReader(const std::string &filename);
	~MovieReader();

	// return current frame num
	int get_frame(int frame = -1);
	int cur_frame_num();

	int w, h;
	uint8_t* Y;
	int rs_Y;
	uint8_t* U;
	int rs_U;
	uint8_t* V;
	int rs_V;

	std::string info;
private:
	struct MovieReaderInternals* p;
	uint8_t *data;
};

struct SequentalReader
{
	MovieReader *movie;
	std::list<std::string> photos;
	std::list<std::string>::iterator next_photo;
	int wanted_fps;

	std::string media_path;
	int cur_frame_num;
	std::string cur_frame_name;
	std::shared_ptr<MatCache> cur_frame;

	SequentalReader() : movie(0), wanted_fps(25), cur_frame_num(0) {}
	~SequentalReader();

	void setup(const std::string &path, int fps = 25);
	bool feed_frame();
	bool fast_feed_frame();//cur_frame is invalid

	cv::Mat get_image_gray();
	cv::Mat get_image_rgb();
};

}  // namespace aifil

#endif  // AIFIL_MEDIA_READER_H
