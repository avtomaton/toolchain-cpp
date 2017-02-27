#include "media-reader.hpp"

#include <common/errutils.hpp>
#include <common/fileutils.hpp>
#include <common/logging.hpp>
#include <common/stringutils.hpp>

#include <opencv2/videoio/videoio.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include <errno.h>
#include <stdexcept>
#include <set>

using aifil::except;

namespace aifil {


static void copy_yuv420p_frame_to_yuv420p(AVFrame *frame, int w, int h,
	uint8_t *Y, uint8_t *U, uint8_t *V, int rs_Y, int rs_U, int rs_V)
{
	for (int y = 0; y < h; y++)
		memcpy(Y + rs_Y * y, frame->data[0] + frame->linesize[0] * y, w);
	for (int y = 0; y < h / 2; y++)
	{
		memcpy(U + rs_U * y, frame->data[1] + frame->linesize[1] * y, w / 2);
		memcpy(V + rs_V * y, frame->data[2] + frame->linesize[2] * y, w / 2);
	}
}

static void convert_rgb888_frame_to_yuv420p_Y_only(AVFrame *frame, int w, int h,
		uint8_t *Y, int rs_Y)
{
	for (int y = 0; y < h; y++)
	{
		for (int x = 0; x < w; x++)
		{
			uint8_t *s = frame->data[0] + frame->linesize[0]*y + 3 * x;
			*(Y + rs_Y * y + x) = (*s + (*(s+1) << 1) + *(s+2)) >> 2;
		}
	}
}

static void convert_gray8_frame_to_yuv420p(AVFrame *frame, int w, int h,
		uint8_t *Y, uint8_t *U, uint8_t *V, int rs_Y, int rs_U, int rs_V)
{
	for (int y = 0; y < h; y++)
		memcpy(Y + rs_Y * y, frame->data[0] + frame->linesize[0] * y, w);
	for (int y = 0; y < h / 2; y++)
	{
		memset(U + rs_U * y, 0, w / 2);
		memset(V + rs_V * y, 0, w / 2);
	}
}

static void init_ffmpeg()
{
	static bool ffmpeg_initialized = false;
	if (ffmpeg_initialized)
		return;

	avcodec_register_all();
	av_register_all();
	ffmpeg_initialized = true;
}


/**
* @brief Хранит информацию о медиафайле, позволяет получить кадр и информацию о нём.
*/
struct MovieReaderInternals {
	MovieReaderInternals();
	~MovieReaderInternals();

	AVFormatContext *format_video;          ///< инфа о форматах и потоках загружаемого файла
	AVCodecContext *context_codec_video;    ///< тут можно найти тип пакета
	AVCodec *codec_video;                   ///< инфа о кодеке
	AVPacket pkt;							///< пакет в котором содержится кадр (для видео в 1-ом пакете - 1 кадр)
	AVFrame *picture_video;					///< кадр, который выковыривается из пакета
	int video_stream_index;                 ///< номер видеопотока загружаемого файла
	int number_of_frames;
	bool eof;

	void init(const std::string &filename);
	void get_frame(int frame);
};


MovieReaderInternals::MovieReaderInternals():
	format_video(0),
	context_codec_video(0),
	codec_video(0),
	video_stream_index(-1),
	eof(false), number_of_frames(0)
{
	init_ffmpeg();
	memset(&pkt, 0, sizeof(pkt));
	picture_video = av_frame_alloc();
	//picture_video = avcodec_alloc_frame();
}

MovieReaderInternals::~MovieReaderInternals()
{
	if (picture_video)
		av_free(picture_video);

	if (pkt.size)
		av_packet_unref(&pkt);

	if (format_video)
	{
		avformat_close_input(&format_video);
		format_video = 0;
	}

	//if (codec_video) {
	//	avcodec_close(cx_video);
	//}
}


void MovieReaderInternals::init(const std::string &filename)
{
	// Читаем заголовок видео и сохраняем информацию о формате.
	int error_code = avformat_open_input(
		&format_video, filename.c_str(),
		0, /*AVInputFormat *fmt,*/
		0  /*AVDictionary **options */);

	except(error_code != AVERROR(EIO), "i/o error occurred in '" + filename + "'");
	except(error_code != AVERROR(EILSEQ),
		   "unknown movie format in '" + filename + "'");

	except(error_code >= 0, "error while opening movie");
	af_assert(format_video);

	// Сохраняем информацию о потоках файла (будет находится в format->streams).
	error_code = avformat_find_stream_info(format_video, 0);
	except(error_code >= 0, "cannot find stream info in '" + filename + "'");

	// Ищем среди всех потоков видеопоток и сохраняем его кодек.
	for (size_t stream_index = 0; stream_index < (int)format_video->nb_streams; stream_index++)
	{

#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(57, 48, 0)
		if (format_video->streams[stream_index]->codec->codec_type != AVMEDIA_TYPE_VIDEO)
#else
		if (format_video->streams[stream_index]->codecpar->codec_type != AVMEDIA_TYPE_VIDEO)
#endif
			continue;

		video_stream_index = stream_index;

#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(57, 48, 0)
		context_codec_video = format_video->streams[stream_index]->codec;
#else
		context_codec_video = avcodec_alloc_context3(nullptr);

		// Бобавляем все параметры "ручками".
		if(avcodec_parameters_to_context(context_codec_video, format_video->streams[stream_index]->codecpar)) {
			// Невозможно добавить параметры кодека.
			continue;
		}

		// Корректируем timebase и id.
		av_codec_set_pkt_timebase(context_codec_video, format_video->streams[stream_index]->time_base);
		context_codec_video->codec_id = codec_video->id;
#endif

		break;
	}

	except(video_stream_index != -1,
		   "haven't video stream in '" + filename + "'");

	af_assert(context_codec_video);
	codec_video = avcodec_find_decoder(context_codec_video->codec_id);
	except(codec_video,
		   "cannot find video decoder for '" + filename + "'");
	error_code = avcodec_open2(context_codec_video, codec_video, 0);
	except(error_code >= 0, "cannot open video decoder for '" + filename + "'");
}

void MovieReaderInternals::get_frame(int frame)
{
	if (pkt.size)
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(57, 48, 0)
		av_free_packet(&pkt);
#else
		av_packet_unref(&pkt);
#endif

	if (frame != -1)
	{
		int flags = AVSEEK_FLAG_FRAME;
		if (frame < number_of_frames)
			flags |= AVSEEK_FLAG_BACKWARD;

		if (av_seek_frame(format_video, video_stream_index, frame, flags) >= 0)
			number_of_frames = frame;
	}

	while (true)
	{
		// Считываем пакет из видео (для видео в 1-ом пакете - 1 кадр).
		if (av_read_frame(format_video, &pkt) < 0)
		{
			eof = true;
			break;
		}

		// Если пакет не принадлежит видеопотоку - обрабатывать не будем.
		if (pkt.stream_index != video_stream_index)
			continue;

		// Декодируем пакет в фрейм
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(57, 48, 0)
		int got_picture = 0;
		avcodec_decode_video2(context_codec_video, picture_video, &got_picture, &pkt);
		if (got_picture)
		{
			++number_of_frames;
			break;
		}
#else
		int ret;
		if ((ret = avcodec_send_packet(context_codec_video, &pkt)) < 0) {
			except(ret<0, "avcodec_send_packet error. ret=%08x\n" + AVERROR(ret));
		}
		if ((ret = avcodec_receive_frame(context_codec_video, picture_video)) < 0) {
			if (ret != AVERROR(EAGAIN)) {
				printf("avcodec_receive_frame error. ret=%08x\n", AVERROR(ret));
				break;
			}
		}
		if (ret >=0)
		{
			++number_of_frames;
			break;
		}
#endif

	}
}


MovieReader::MovieReader(const std::string &filename):
	Y(0), U(0), V(0), reader_internals(0), data(0)
{
	reader_internals = new MovieReaderInternals;
	try
	{
		reader_internals->init(filename);

		info = filename + aifil::stdprintf(
				" %i:%i %s",
				reader_internals->context_codec_video->width,
			    reader_internals->context_codec_video->height,
				reader_internals->codec_video->name);
		w = reader_internals->context_codec_video->width;
		h = reader_internals->context_codec_video->height;

	}
	catch (const std::runtime_error &)
	{
		delete reader_internals;
		reader_internals = 0;
		throw;
	}

}

MovieReader::~MovieReader()
{
	delete reader_internals;
	delete[] data;
}

int MovieReader::get_frame(int frame)
{
	if (!reader_internals)
		return 0;

	reader_internals->get_frame(frame);
	if (reader_internals->eof)
		return 0;

	Y = U = V = 0;
	rs_Y = rs_U = rs_V = 0;

	int h = reader_internals->context_codec_video->height;
	int w = reader_internals->context_codec_video->width;

	if (!data) // alloc
		data = new uint8_t[w * h * 3 / 2 + 16 * 3];

	rs_Y = w;
	rs_U = w / 2;
	rs_V = w / 2;
	af_assert((rs_Y & 15) == 0);
	af_assert((rs_U & 7) == 0);
	af_assert((rs_V & 7) == 0);
	Y = data + (16 - ((size_t)data & 15));
	U = Y + w * h;
	V = Y + w * h + w * h / 4;
	//printf("format: %i\n", p->cx_video->pix_fmt);

	switch (reader_internals->context_codec_video->pix_fmt)
	{
	case AV_PIX_FMT_YUV420P:
		copy_yuv420p_frame_to_yuv420p(reader_internals->picture_video, w, h, Y, U, V, rs_Y, rs_U, rs_V);
		break;
	case AV_PIX_FMT_RGB24:
	case AV_PIX_FMT_BGR24:
		convert_rgb888_frame_to_yuv420p_Y_only(reader_internals->picture_video, w, h, Y, rs_Y);
		break;
	case AV_PIX_FMT_GRAY8:
	case AV_PIX_FMT_PAL8:
		convert_gray8_frame_to_yuv420p(reader_internals->picture_video, w, h, Y, U, V, rs_Y, rs_U, rs_V);
		break;
	default:
		break;
	}

	//printf("%i\n", (int) p->picture_video->pts);

	return reader_internals->number_of_frames;
}

int MovieReader::cur_frame_num()
{
	return reader_internals->number_of_frames;
}



SequentialReader::~SequentialReader()
{
	delete movie;
	delete cv_movie;
}

void SequentialReader::setup(const std::string &input_path, int fps)
{
	using namespace boost::filesystem;
	wanted_fps = fps;

	delete movie;
	movie = 0;
	delete cv_movie;
	cv_movie = 0;

	cur_frame_num = 0;
	photos.clear();
	cur_frame_name.clear();
	cur_frame.reset();

	media_path = input_path;
	path my_dir(input_path);
	except(exists(my_dir), "'" + media_path + "' path for media does not exist");

	if (!is_directory(my_dir) && !aifil::endswith(input_path, ".lst"))
	{
		// try to load movie
		bool ffmpeg_explicit_backend = false;
		if (ffmpeg_explicit_backend)
		{
			movie = new MovieReader(input_path);

			except(!!movie, "movie is specified but is not found");
			aifil::log_state("movie mode (%s), frame size: %ix%i",
					  input_path.c_str(), movie->w, movie->h);
			cur_frame.reset(new MatCache(movie->w, movie->h));
		}
		else
		{
			cv_movie = new cv::VideoCapture(input_path);
			except(!!cv_movie, "movie is specified but is not found");

			int movie_w = int(cv_movie->get(cv::CAP_PROP_FRAME_WIDTH));
			int movie_h = int(cv_movie->get(cv::CAP_PROP_FRAME_HEIGHT));
			aifil::log_state("movie mode (%s), frame size: %ix%i",
					  input_path.c_str(), movie_w, movie_h);
			cur_frame.reset(new MatCache(movie_w, movie_h));
		}
	}
	else
	{
		FILE *file_index = 0;
		if (aifil::endswith(input_path, ".lst"))
			file_index = fopen(input_path.c_str(), "rb");
		if (file_index)
		{
			media_path = path(media_path).parent_path().string();
			char buf[2048];
			while (true)
			{
				int res = fscanf(file_index, "%s ", buf);
				if (res != 1)
					break;
				std::string full_name = buf;
				if (exists(full_name) && is_regular_file(full_name))
					photos.push_back(full_name);
				else
					aifil::log_warning("can't find file '%s'", full_name.c_str());
			}
			fclose(file_index);
		}
		else
		{
			photos.clear();
			std::set<std::string> extensions = {".png", ".pgm", ".jpg", ".jpeg" };
			photos = aifil::ls_directory(input_path, extensions);
		}
		// equalize photos order
		photos.sort();
		except(!photos.empty(), "can't find any image");
		next_photo = photos.begin();
		aifil::log_state("photos mode (%s)", input_path.c_str());
	} // if photo dir
}

bool SequentialReader::feed_frame()
{
	if (movie)
	{
		int frame_num = movie->get_frame();
		if (!frame_num)
			return false;

		cur_frame_num = frame_num;
		cur_frame->set_yuv422(movie->Y, movie->rs_Y,
							  movie->U, movie->rs_U,
							  movie->V, movie->rs_V);
	}
	else if (cv_movie)
	{
		cv::Mat frame;
		if (!cv_movie->read(frame))
			return false;

		cur_frame_num = int(cv_movie->get(cv::CAP_PROP_POS_FRAMES));
		cur_frame->set_cv_mat(frame);
	}
	else if (!photos.empty())
	{
		if (next_photo == photos.end())
			return false;

		cur_frame_name = aifil::get_relative_path(media_path, *next_photo);
		cv::Mat mat = cv::imread(*next_photo);
		if (mat.empty())
		{
			aifil::log_warning("wrong image file '%s'", next_photo->c_str());
			
			// don't break GUI functionality, just create empty black image
			mat = cv::Mat(640, 480, CV_8UC3, cv::Scalar(0, 0, 0));
			return false;
		}
		if (!mat.empty()
			&& (!cur_frame
				|| cur_frame->width != mat.cols
				|| cur_frame->height != mat.rows))
			cur_frame.reset(new MatCache(mat.cols, mat.rows));

		cur_frame->set_cv_mat(mat);
		++next_photo;
		++cur_frame_num;
	}
	else
		return false;

	return true;
}

bool SequentialReader::fast_feed_frame()//cur_frame is invalid
{
	if (movie)
	{
		int frame_num = movie->get_frame();
		if (!frame_num)
			return false;
		cur_frame_num = frame_num;
	}
	else if (cv_movie)
	{
		// TODO: make in faster
		cv::Mat frame;
		if (!cv_movie->read(frame))
			return false;

		cur_frame_num = int(cv_movie->get(cv::CAP_PROP_POS_FRAMES));
	}
	else if (!photos.empty())
	{
		if (next_photo == photos.end())
			return false;

		auto next = next_photo;
		++next;
		if (next == photos.end())
			feed_frame();
		else
		{
			cur_frame_name = aifil::get_relative_path(media_path, *next_photo);
			++next_photo;
			++cur_frame_num;
		}
	}
	else
		return false;

	return true;
}

cv::Mat SequentialReader::get_image_gray()
{
	if (cur_frame)
		return cur_frame->get_gray_8u();
	else
		return cv::Mat();
}

cv::Mat SequentialReader::get_image_rgb()
{
	if (cur_frame)
		return cur_frame->get_rgb_8u();
	else
		return cv::Mat();
}

}  // namespace aifil
