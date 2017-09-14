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
#include <libswscale/swscale.h>
}

#include <errno.h>
#include <stdexcept>
#include <set>
#include <memory>

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
struct MovieReaderInternals
{
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
	bool want_new_packet;

	void init(const std::string &filename);
	void get_frame(int frame);
};


MovieReaderInternals::MovieReaderInternals():
	format_video(0),
	context_codec_video(0),
	codec_video(0),
	video_stream_index(-1),
	number_of_frames(0),
	eof(false),
	want_new_packet(true)
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
		if (avcodec_parameters_to_context(context_codec_video, format_video->streams[stream_index]->codecpar)) {
			// Невозможно добавить параметры кодека.
			continue;
		}

		// Корректируем timebase и id.
		av_codec_set_pkt_timebase(context_codec_video, format_video->streams[stream_index]->time_base);
//		context_codec_video->codec_id = codec_video->id;
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
		// Да ладно? Вот прям всегда?
		// TODO: want_new_packet
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
		int ret = avcodec_send_packet(context_codec_video, &pkt);
		except(ret >= 0, "avcodec_send_packet error. ret=%08x\n" + AVERROR(ret));

		if ((ret = avcodec_receive_frame(context_codec_video, picture_video)) < 0)
		{
			if (ret != AVERROR(EAGAIN))
			{
				printf("avcodec_receive_frame error. ret=%08x\n", AVERROR(ret));
				break;
			}
		}
		if (ret >= 0)
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
			//std::set<std::string> extensions = {".png", ".pgm", ".jpg", ".jpeg" };
			photos = aifil::ls_directory(input_path, PATH_FILE);
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
		cur_frame->set_yuv422(
				movie->Y, movie->rs_Y,
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
		cv::Mat mat;
		while(true)
		{
			if (next_photo == photos.end())
				return false;
			cur_frame_name = aifil::get_relative_path(media_path, *next_photo);
			mat = cv::imread(*next_photo);
			if (mat.empty())
			{
				aifil::log_warning("wrong image file '%s'", next_photo->c_str());
				++next_photo;
				++cur_frame_num;
				continue;
			}
			else
				break;
		}

		af_assert(!mat.empty());
		if (!cur_frame
				|| cur_frame->width != mat.cols
				|| cur_frame->height != mat.rows)
			cur_frame.reset(new MatCache(mat.cols, mat.rows));

		cur_frame->set_cv_mat(mat);
		++next_photo;
		++cur_frame_num;
	}
	else
		return false;

	return true;
}

bool SequentialReader::fast_feed_frame()
{
	// cur_frame will be empty
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

struct FFHandle
{
	AVStream * in_stream = nullptr;
	AVCodecContext  * codec_context = nullptr;
	AVFormatContext *format_context = nullptr;

	AVFrame * av_frame = nullptr;
	SwsContext * img_convert_ctx = nullptr;

	std::list<AVPacket> packets;
};

void VideoStreamReader::open_input(const std::string &stream_addr)
{
	input_is_open = false;
	ff_handle = std::make_shared<FFHandle>();

	if (ff_handle->av_frame)
		av_frame_free(&ff_handle->av_frame);
	ff_handle->av_frame = av_frame_alloc();

	if (ff_handle->format_context)
		avformat_free_context(ff_handle->format_context);
	ff_handle->format_context = avformat_alloc_context();

//	in_format_context->interrupt_callback.callback = avi_interrupt_callback;
//	in_format_context->interrupt_callback.opaque = &avi_interrupt_context;

	aifil::log_state("Opening stream '%s'", stream_addr.c_str());

	int avi_res = avformat_open_input(
			&ff_handle->format_context, stream_addr.c_str(), NULL, NULL);

	if (avi_res != 0)
	{
		current_error_status = ErrorStatus::FORMAT_ERROR;
		af_exception("Error in avformat_open_input");
	}

	avi_res = avformat_find_stream_info(ff_handle->format_context, NULL);

	if (avi_res < 0)
	{
		current_error_status = ErrorStatus::FORMAT_ERROR;
		af_exception("Error in avformat_find_stream_info");
	}

	video_stream_index = -1;

	for (int i = 0; i < int(ff_handle->format_context->nb_streams); i++)
	{
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(57, 48, 0)
		if (ff_handle->format_context->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
#else
		if (ff_handle->format_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
#endif
		{
			ff_handle->in_stream = ff_handle->format_context->streams[i];
			video_stream_index = i;
			break;
		}
	}

	if (video_stream_index < 0)
	{
		current_error_status = ErrorStatus::FORMAT_ERROR;
		af_exception("Video stream not found");
	}

#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(57, 48, 0)
	ff_handle->codec_context = ff_handle->in_stream->codec;
#else
	ff_handle->codec_context = avcodec_alloc_context3(nullptr);

	// We should set them manually
	if (avcodec_parameters_to_context(
			ff_handle->codec_context, ff_handle->in_stream->codecpar))
	{
		af_exception("Cannot setup codec context");
	}

//	// Корректируем timebase и id.
//	av_codec_set_pkt_timebase(ff_handle->codec_context, ff_handle->in_stream->time_base);
//	ff_handle->codec_context->codec_id = ff_handle->in_stream->codec->id;
#endif

	AVCodec *codec = avcodec_find_decoder(ff_handle->codec_context->codec_id);
	if (codec == NULL)
	{
		current_error_status = ErrorStatus::CODEC_ERROR;
		af_exception("Codec not found");
	}
	log_debug("Codec: %s", codec->name);

	if (avcodec_open2(ff_handle->codec_context, codec, NULL) < 0)
	{
		current_error_status = ErrorStatus::CODEC_ERROR;
		af_exception("Codec error");
	}

	av_dump_format(ff_handle->format_context, 0, stream_addr.c_str(), 0);
	frame_w = ff_handle->codec_context->width;
	frame_h = ff_handle->codec_context->height;

	input_is_open = true;
}

void VideoStreamReader::close_input()
{
	af_assert(ff_handle && "OPEN STREAM before closing stream");

	if (ff_handle->av_frame)
		av_frame_free(&ff_handle->av_frame);
	if (ff_handle->img_convert_ctx)
		sws_freeContext(ff_handle->img_convert_ctx);

	if (ff_handle->codec_context)
		avcodec_close(ff_handle->codec_context);

	if (!input_is_open)
		return;

	if (ff_handle->format_context)
		avformat_close_input(&ff_handle->format_context);

	ff_handle->codec_context = nullptr;
	ff_handle->format_context = nullptr;
	input_is_open = false;
}

cv::Mat VideoStreamReader::get_frame()
{
	af_assert(ff_handle && "OPEN STREAM before getting frames");

	if (ff_handle->packets.empty())
	{
		int ret = 0;
		while (true)
		{
			AVPacket packet;
			ret = av_read_frame(ff_handle->format_context, &packet);
			if (ret < 0)
				break;
			if (packet.stream_index != video_stream_index)
				continue;

			ff_handle->packets.emplace_back(packet);
			break;
		}

		if (ff_handle->packets.empty())
		{
			char buf[256];
			av_strerror(ret, buf, sizeof(buf));
			aifil::log_debug("reading fail: %d (%s)", ret, buf);
		}
	}

	if (ff_handle->packets.empty())
		return cv::Mat();

	int frame_finished = 0;
	int decoded_size = avcodec_decode_video2(
			ff_handle->codec_context,
			ff_handle->av_frame,
			&frame_finished,
			&(ff_handle->packets.front()));
	av_free_packet(&(ff_handle->packets.front()));
	ff_handle->packets.pop_front();

	if (decoded_size < 0)
	{
		char buf[256];
		av_strerror(decoded_size, buf, sizeof(buf));
		aifil::log_debug("decoding fail: %d (%s)", decoded_size, buf);
		return cv::Mat();
	}

	frame_w = ff_handle->codec_context->width;
	frame_h = ff_handle->codec_context->height;
	ff_handle->img_convert_ctx = sws_getCachedContext(
			ff_handle->img_convert_ctx,
			frame_w, frame_h, ff_handle->codec_context->pix_fmt,
			frame_w, frame_h, AV_PIX_FMT_BGR24,
			SWS_BICUBIC, NULL, NULL, NULL);
	af_assert(ff_handle->img_convert_ctx);

	size_t required_octets = avpicture_get_size(
			AV_PIX_FMT_BGR24,
			frame_w,
			frame_h);
	cv::Mat cv_frame(frame_h, frame_w, CV_8UC3);
	af_assert(cv_frame.step == cv_frame.cols * cv_frame.elemSize());
	af_assert(cv_frame.total() * cv_frame.elemSize() == required_octets);

	af_assert(AV_NUM_DATA_POINTERS >= 4);

	uint8_t * dst_data[4] = {nullptr, nullptr, nullptr, nullptr};
	int dst_linesize[4] = {0, 0, 0, 0};

	dst_data[0] = cv_frame.data;
	dst_linesize[0] = cv_frame.cols * cv_frame.elemSize();

	sws_scale(
			ff_handle->img_convert_ctx,
			ff_handle->av_frame->data, ff_handle->av_frame->linesize, 0, frame_h,
			dst_data, dst_linesize);
	return cv_frame;
}

}  // namespace aifil
