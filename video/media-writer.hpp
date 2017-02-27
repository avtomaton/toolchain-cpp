#ifndef AIFIL_MEDIA_WRITER_H
#define AIFIL_MEDIA_WRITER_H

#include <string>
#include <opencv2/core/mat.hpp>


namespace aifil {

class MovieWriter {
public:
	MovieWriter(const std::string &filename, int frame_width, int frame_height);
	~MovieWriter();

private:
	class MovieWriterInternals *writer_internals;
	std::string info;

public:
	void add_frame(const cv::Mat &input_image, int pts);
	void add_empty_frame(int pts, int frame_size);
};

}

#endif //AIFIL_MEDIA_WRITER_H
