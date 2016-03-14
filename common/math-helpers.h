#ifndef AIFIL_MATH_HELPERS_H
#define AIFIL_MATH_HELPERS_H

#include <cmath>
#include <opencv2/core/core.hpp>

namespace aifil {

const double rather_small_val = 1e-15;
const double rather_big_val = 1e+15;
const float M_PIf = 3.14159265358979323846f;

template<typename T>
inline T sqr(T val) { return val * val; }

inline float cross_correlation(float *vec0, float *vec1, int len)
{
	float sum1 = 0, sum2 = 0, sum12 = 0;
	for (int i = 0; i < len; i++)
	{
		sum1 += vec0[i] * vec0[i];
		sum2 += vec1[i] * vec1[i];
		sum12 += vec0[i] * vec1[i];
	}
	float res = 0;
	if (sum1 && sum2)
		res = sum12 / sqrt(sum1 * sum2);
	return res;
}

template<typename T>
T val_similarity(T val0, T val1)
{
	T sim = 0;
	if (val1)
		sim = val0 / val1;
	if (sim > 1.0)
		sim = 1.0 / sim;
	return sim;
}

//[0; 1] (0.5 when different_area == common_area)
inline float rect_similarity(const cv::Rect &r1, const cv::Rect &r2)
{
	float common = (r1 & r2).area();
	float different = (r1.area() + r2.area() - 2.0f * common);
	if (different > FLT_EPSILON)
		return std::min(0.5f * common / different, 1.0f);
	else
		return 1.0f;
}

// returns > 0, if point c is to the right of line ab
// returns < 0, if point c is to the left of line ab
// returns == 0, if point c is on the line ab
template <typename T>
inline T cross_product(cv::Point_<T> a, cv::Point_<T> b, cv::Point_<T> c)
{
	return (a.x - c.x) * (b.y - c.y) - (a.y - c.y) * (b.x - c.x);
}

template <typename T>
inline T euclid_distance2(T x1, T y1, T x2, T y2)
{
	return (x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2);
}

template <typename T>
inline T euclid_distance(T x1, T y1, T x2, T y2)
{
	return sqrt(euclid_distance2(x1, y1, x2, y2));
}

// Checks if p1p2 intersects p4p3
bool intersect(const cv::Point2f &pd1, const cv::Point2f &pd2,
	const cv::Point2f &p3, const cv::Point2f &p4);

// Checks if x1y1-x2y2 intersects x3y3-x4y4
bool intersect(float x1, float y1, float x2, float y2,
	float x3, float y3, float x4, float y4);

inline double smart_inverse(double val)
{
	if (abs(1.0 - val) > 1e-10)
		return 1.0 / val;
	else
		return 1.0;
}

inline float to_float(double val)
{
	if (val > FLT_MAX)
		return FLT_MAX;
	else if (val < FLT_MIN)
		return FLT_MIN;
	else if (abs(val - 1.0) <= FLT_EPSILON)
		return 1.0f;
	else
		return float(val);
}

}  // namespace aifil

#endif  // AIFIL_MATH_HELPERS_H
