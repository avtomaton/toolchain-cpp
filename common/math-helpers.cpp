#include "math-helpers.h"

namespace aifil {

// Checks if p1p2 intersects p4p3
bool intersect(const cv::Point2f &pd1, const cv::Point2f &pd2,
	const cv::Point2f &p3, const cv::Point2f &p4)
{
	//check projections
	//max(x1, x2) >= min(x3, x4), max(x3, x4) >= min(x1, x2)
	//max(y1, y2) >= min(y3, y4), max(y3, y4) >= min(y1, y2)

	//denominator
	float d = (pd2.x - pd1.x)*(p3.y - p4.y) - (p3.x - p4.x)*(pd2.y - pd1.y);

	// If d == 0, lines are parallel or are partically/wholy the same
	if (d == 0)
		return false;

	//numerators
	float d1 = (p3.x - pd1.x)*(p3.y - p4.y) - (p3.x - p4.x)*(p3.y - pd1.y);
	float d2 = (pd2.x - pd1.x)*(p3.y - pd1.y) - (p3.x - pd1.x)*(pd2.y - pd1.y);

	float t1 = d1 / d;
	float t2 = d2 / d;

	if (t1 >= 0 && t1 <= 1 && t2 >= 0 && t2 <= 1)
		return true;

	return false;
}

bool intersect(float x1, float y1, float x2, float y2,
			   float x3, float y3, float x4, float y4)
{
	bool vec_sides =  ((((x3 - x1) * (y2 - y1) - (y3 - y1) * (x2 - x1)) *
						((x4 - x1) * (y2 - y1) - (y4 - y1) * (x2 - x1)) <= 0) &&
					   (((x1 - x3) * (y4 - y3) - (y1 - y3) * (x4 - x3)) *
						((x2 - x3) * (y4 - y3) - (y2 - y3) * (x4 - x3)) <= 0));

	//maybe check projections
	return vec_sides;
}

}  // namespace aifil
