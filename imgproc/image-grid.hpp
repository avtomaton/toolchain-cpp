#ifndef AIFIL_IMAGE_GRID_H
#define AIFIL_IMAGE_GRID_H

#include <opencv2/core/core.hpp>

#include <vector>

struct ImageCell
{
	ImageCell();
	ImageCell(int x, int y, int w, int h);
	cv::Rect rect();

	int x;
	int y;
	int size_w;
	int size_h;
};

struct ImageGrid
{
	// grid of w * h non-overlapped cells
	ImageGrid(const cv::Mat &img, int w, int h);

	// grid of cell_w * cell_h cells
	ImageGrid(
			const cv::Mat &img,
			int cell_w, int cell_h,
			int cell_stride_x, int cell_stride_y);

	void generate_cells();

	int image_w;
	int image_h;
	int cell_w;
	int cell_h;
	int cells_x;
	int cells_y;

	// cell can be overlapped
	int cell_stride_x;
	int cell_stride_y;

	int border_left;
	int border_right;
	int border_up;
	int border_down;

	cv::Mat image;
	std::vector<ImageCell> cells;
};

#endif  // AIFIL_IMAGE_GRID_H
