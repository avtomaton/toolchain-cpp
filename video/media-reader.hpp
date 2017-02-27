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

	int w, h;	///< ширина, высота
	// Переменные одноимённого цветогого пространства YUV:
	uint8_t* Y;	///< яркость
	uint8_t* U;	///< компонент цветности
	uint8_t* V;	///< и ещё один компонент цветности
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
	SequentialReader() : movie(0), cv_movie(0), wanted_fps(25), cur_frame_num(0) {}
	~SequentialReader();

	MovieReader *movie;
	cv::VideoCapture *cv_movie;
	std::list<std::string> photos;
	std::list<std::string>::iterator next_photo;
	int wanted_fps;

	std::string media_path;
	int cur_frame_num;
	std::string cur_frame_name;
	std::shared_ptr<MatCache> cur_frame;

	void setup(const std::string &path, int fps = 25);
	bool feed_frame();
	bool fast_feed_frame();//cur_frame is invalid

	cv::Mat get_image_gray();
	cv::Mat get_image_rgb();
};

}  // namespace aifil

#endif  // AIFIL_MEDIA_READER_H
