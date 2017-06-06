#include "media-writer.hpp"

#include <common/stringutils.hpp>

#include <opencv2/videoio/videoio.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
}

namespace aifil {

//TODO (sergsenta1995): Такая функция уже есть в media-reader.
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
 * @brief Содержит в себе низкоуровневую работу по записи (склеивании) изображений в видеофайл.
 */
struct MovieWriterInternals {
	/**
 	* @brief В онструкторе инициализируются интерфунсы ffmpeg,
 	* вызываются функции для обнуления памяти используемых объектов.
 	*/
	MovieWriterInternals(const std::string &video_name, int frame_width, int frame_height);

	/**
 	* @brief В деструкторе кодируются и записываются последние кадры и освобождаются ресурсы.
 	*/
	~MovieWriterInternals();

	AVCodecContext *codec_context;	///< содержит тип пакета и его параметры
	AVFrame *frame;					///< кадр, который выковыривается из пакета
	AVPacket packet;				///< пакет в котором содержится кадр (для видео в 1-ом пакете - 1 кадр)
	AVCodec *encoder;				///< кодирует изображение в часть видео
	SwsContext* sws_context;		///< служит для выполнения преобразований над изображением
	FILE *output_video_file;

public:
	/**
	 * @brief Записывает изображение с указанным presentation timestamp (pts).
	 * @param image [in] Изображение, которое нужно записать в видеофайл.
	 * @param pts [in] Значение presentation timestamp.
	 */
	void write_image(const cv::Mat &image, int pts);

	/**
	 * @brief Эмулирует задержку посредством записи в видео пустого кадра.
	 * @param pts [in] Значение presentation timestamp.
	 * @para, frame_size [in] Размер пустого кадра.
	 */
	void emulate_delay(int pts, int frame_size);

private:
	/**
 	* @brief В деструкторе кодируются и записываются последние кадры и освобождаются ресурсы.
 	*/
	void write_last_frames();

	/**
	 * @brief Конвертирует изображение из цветовой модели RGB
	 * в специальную цветовую модель для видео - YUV
	 * @param image_rgb_buffer [in] Данные изображения.
	 */
	void convert_rgb_frame_to_yuv(uint8_t *image_rgb_buffer);
};


/**
 * @brief В онструкторе инициализируются интерфунсы ffmpeg,
 * вызываются функции для обнуления памяти используемых объектов.
 */
MovieWriterInternals::MovieWriterInternals(const std::string &video_name, int frame_width, int frame_height)
{
	init_ffmpeg();

	encoder = avcodec_find_encoder(AV_CODEC_ID_H264);
	if (!encoder) {
		std::cout << "Codec not found: avcodec_find_encoder() is failed" << std::endl;
	}

	codec_context = avcodec_alloc_context3(encoder);
	if (!codec_context) {
		std::cout << "Could not allocate video codec context: avcodec_alloc_context3() is failed" << std::endl;
	}
	codec_context->bit_rate      = 6400000;
	codec_context->width         = frame_width;
	codec_context->height        = frame_height;
	codec_context->time_base.den = 50;					// к-во кадров
	codec_context->time_base.num = 1;					// в секунду
	codec_context->gop_size      = 40;					// к-во неключевых кадров между ключевыми (образуют группу)
	codec_context->max_b_frames  = 0;					// к-во кадров, учитывающих информацию с пред- и -пост кадра
	codec_context->pix_fmt       = AV_PIX_FMT_YUV420P;	// цветовая схема будущего видео




	//av_opt_set(codec_context->priv_data, "preset", "slow", 0);
	av_opt_set(codec_context->priv_data, "tune", "zerolatency", 0);

	if (avcodec_open2(codec_context, encoder, NULL) < 0) {
		std::cout << "Could not open codec: avcodec_open2() is failed" << std::endl;
	}

	output_video_file = fopen(video_name.c_str(), "wb");
	if (!output_video_file) {
		std::cout << "Video file <" << video_name << "> could not open." << std::endl;
	}

	frame = av_frame_alloc();
	if (!frame) {
		std::cout << "Could not allocate video frame: av_frame_alloc() is failed." << std::endl;
	}
	frame->format = codec_context->pix_fmt;
	frame->width  = codec_context->width;
	frame->height = codec_context->height;
	int ret = av_image_alloc(frame->data, frame->linesize, codec_context->width, codec_context->height, codec_context->pix_fmt, 32);
	if (ret < 0) {
		std::cout << "Could not allocate raw picture buffer: av_image_alloc() is failed" << std::endl;
	}
}


/**
 * @brief В деструкторе кодируются и записываются последние кадры и освобождаются ресурсы.
 */
MovieWriterInternals::~MovieWriterInternals()
{
	write_last_frames();

	fclose(output_video_file);
	avcodec_close(codec_context);
	av_free(codec_context);
	av_freep(&frame->data[0]);
	av_frame_free(&frame);
}


/**
 * @brief Конвертирует изображение из цветовой модели BGR
 * в специальную цветовую модель для видео - YUV
 * @param image_rgb_buffer [in] Данные изображения.
 */
void MovieWriterInternals::convert_rgb_frame_to_yuv(uint8_t *image_rgb_buffer) {
	int image_line_size = 3 * codec_context->width;

	sws_context = sws_getCachedContext(
			nullptr,
			codec_context->width, codec_context->height, AV_PIX_FMT_BGR24,     // из чего
			codec_context->width, codec_context->height, AV_PIX_FMT_YUV420P,   // во что
			0, 0, 0, 0);

	sws_scale(sws_context,
			  (const uint8_t * const *)&image_rgb_buffer,
			  &image_line_size,
			  0,
			  codec_context->height,
			  frame->data,
			  frame->linesize);
}


/**
 * @brief Записывает изображение с указанным presentation timestamp (pts).
 * @param image [in] Изображение, которое нужно записать в видеофайл.
 * @param pts [in] Значение presentation timestamp.
 */
void MovieWriterInternals::write_image(const cv::Mat &image, int pts)
{
	frame->pts = pts;
	// В видеовайле другая цветовая схема - конвертим.
	convert_rgb_frame_to_yuv(image.data);

	av_init_packet(&packet);
	packet.data = NULL;
	packet.size = 0;

	int got_output;
	// Фрейм - в пакет.
	int return_code = avcodec_encode_video2(codec_context, &packet, frame, &got_output);
	if (return_code < 0) {
		std::cout << "Error encoding frame: avcodec_encode_video2() is failed." << std::endl;
		return;
	}
	if (got_output) {
		// Пакет - в файл.
		fwrite(packet.data, 1, packet.size, output_video_file);
		av_free_packet(&packet);
	}
}


/**
 * @brief Эмулирует задержку посредством записи в видео пустого кадра.
 * @param pts [in] Значение presentation timestamp.
 * @para, frame_size [in] Размер пустого кадра.
 */
void MovieWriterInternals::emulate_delay(int pts, int frame_size)
{
	frame->pts = pts;
	// Суть пустой кадр.
	memset(frame->data[0], 0, frame_size);

	av_init_packet(&packet);
	packet.data = NULL;
	packet.size = 0;

	int got_output;
	// Фрейм - в пакет.
	int return_code = avcodec_encode_video2(codec_context, &packet, frame, &got_output);
	if (return_code < 0) {
		std::cout << "Error encoding frame: avcodec_encode_video2() is failed." << std::endl;
		return;
	}
	if (got_output) {
		// Пакет - в файл.
		fwrite(packet.data, 1, packet.size, output_video_file);
		av_free_packet(&packet);
	}
}


/**
 * @brief Кодирует и записывает последние кадры + дописывает признак конца файла.
 */
void MovieWriterInternals::write_last_frames()
{
	int got_output;

	do {
		fflush(stdout);
		int return_code = avcodec_encode_video2(codec_context, &packet, NULL, &got_output);
		if (return_code < 0) {
			std::cout << "Error encoding frame: avcodec_encode_video2() is failed" << std::endl;
			return;
		}
		if (got_output) {
			fwrite(packet.data, 1, packet.size, output_video_file);
			av_packet_unref(&packet);
		}
	} while (got_output);

	// Записываем признак конца видеофайла.
	uint8_t end_code[] = { 0, 0, 1, 0xb7 };
	fwrite(end_code, 1, sizeof(end_code), output_video_file);
}


MovieWriter::MovieWriter(const std::string &filename, int frame_width, int frame_height)
{
	writer_internals = new MovieWriterInternals(filename, frame_width, frame_height);
	try
	{
		info = filename + aifil::stdprintf(
				" %i:%i %s",
				writer_internals->codec_context->width,
				writer_internals->codec_context->height,
				writer_internals->encoder->name);

	}
	catch (const std::runtime_error &)
	{
		delete writer_internals;
		writer_internals = 0;
		throw;
	}
}


MovieWriter::~MovieWriter()
{
	delete writer_internals;
}


void MovieWriter::add_frame(const cv::Mat &input_image, int pts)
{
	writer_internals->write_image(input_image, pts);
}

void MovieWriter::add_empty_frame(int pts, int frame_size)
{
	writer_internals->emulate_delay(pts, frame_size);
}

}
