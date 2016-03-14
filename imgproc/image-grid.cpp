#include "image-grid.h"

#include <common/errutils.h>

ImageCell::ImageCell()
	: x(0), y(0), size_w(0), size_h(0)
{
}

ImageCell::ImageCell(int x_, int y_, int w, int h)
	: x(x_), y(y_), size_w(w), size_h(h)
{
}

cv::Rect ImageCell::rect()
{
	return cv::Rect(x, y, size_w, size_h);
}

ImageGrid::ImageGrid(const cv::Mat &img, int w, int h)
	: cells_x(w), cells_y(h)
{
	img.copyTo(image);

	image_w = img.cols;
	image_h = img.rows;

	// grid dimensions should be no larger than image dimensions
	af_assert(cells_x <= image_w || cells_y <= image_h);
	af_assert(cells_x > 0 && cells_y > 0);

	cell_w = image_w / cells_x;
	cell_stride_x = cell_w;
	cell_h = image_h / cells_y;
	cell_stride_y = cell_h;

	int covered_x = cell_w * cells_x;
	int covered_y = cell_h * cells_y;
	border_left = (image_w - covered_x) / 2;
	border_right = image_w - covered_x - border_left;
	border_up = (image_h - covered_y) / 2;
	border_down = image_h - covered_y - border_up;

	generate_cells();
}

ImageGrid::ImageGrid(
		const cv::Mat &img,
		int cell_w_, int cell_h_, int cell_stride_x_, int cell_stride_y_)
	: cell_w(cell_w_), cell_h(cell_h_),
	  cell_stride_x(cell_stride_x_), cell_stride_y(cell_stride_y_)
{
	img.copyTo(image);

	af_assert(cell_w > 0 && cell_h > 0 && cell_stride_x > 0 && cell_stride_y > 0);

	image_w = img.cols;
	image_h = img.rows;

	cells_x = (image_w - cell_w) / cell_stride_x + 1;
	cells_y = (image_h - cell_h) / cell_stride_y + 1;

	af_assert(cells_x > 0 && cells_y > 0);

	int covered_x = cell_w + cell_stride_x * (cells_x - 1);
	int covered_y = cell_h + cell_stride_y * (cells_y - 1);
	border_left = (image_w - covered_x) / 2;
	border_right = image_w - covered_x - border_left;
	border_up = (image_h - covered_y) / 2;
	border_down = image_h - covered_y - border_up;

	generate_cells();
}

void ImageGrid::generate_cells()
{
	cells.clear();
	for (int y = 0; y < cells_x; ++y)
	{
		for (int x = 0; x < cells_y; ++x)
		{
			ImageCell cell(
					border_left + x * cell_stride_x,
					border_up + y * cell_stride_y,
					cell_w, cell_h);
			cells.push_back(cell);
		}
	}
}

