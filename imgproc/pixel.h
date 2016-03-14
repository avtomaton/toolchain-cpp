/** @file pixel.h
 *
 *  @brief color pixel simple operations
 *
 *  @author Victor Pogrebnyak <vp@aifil.ru>
 *
 *  $Id$
 *
 */

#ifndef AIFIL_PIXEL_H
#define AIFIL_PIXEL_H

#include <stdint.h>
#include <opencv2/core/core.hpp>

namespace aifil {

template<typename T> T* mat_ptr(const cv::Mat &mat, int row, int col)
{
	return (T*)(mat.data) + row * mat.step1() + col * mat.channels();
}

template<int ch, typename T> class pixel_t_;

template<int ch, typename T>
pixel_t_<ch, T> operator+(const pixel_t_<ch, T> &l, const pixel_t_<ch, T> &r);
template<int ch, typename T>
pixel_t_<ch, T> operator-(const pixel_t_<ch, T> &l, const pixel_t_<ch, T> &r);
template<int ch, typename T>
pixel_t_<ch, T> operator*(const pixel_t_<ch, T> &l, const pixel_t_<ch, T> &r);
template<int ch, typename T>
pixel_t_<ch, T> operator/(const pixel_t_<ch, T> &l, const pixel_t_<ch, T> &r);

template<int ch, typename pix_t>
pixel_t_<ch, pix_t> operator+(const pixel_t_<ch, pix_t> &pix, float val);
template<int ch, typename pix_t>
pixel_t_<ch, pix_t> operator-(const pixel_t_<ch, pix_t> &pix, float val);
template<int ch, typename pix_t>
pixel_t_<ch, pix_t> operator*(const pixel_t_<ch, pix_t> &pix, float val);
template<int ch, typename pix_t>
pixel_t_<ch, pix_t> operator/(const pixel_t_<ch, pix_t> &pix, float val);

template<int ch, typename T>
class pixel_t_
{
public:
	pixel_t_() { ptr = new T[ch]; need_free = true; }
	pixel_t_(T *p) : ptr(p) { need_free = false; }
	~pixel_t_() { if (need_free) delete ptr; }

	//for assignment operators with pixels of another value type
	const T* get_data() const { return ptr; }

	//replace default assignment operator
	pixel_t_& operator=(const pixel_t_ &o)
	{
		for (int i = 0; i < ch; ++i)
			ptr[i] = T(o.ptr[i]);
		return *this;
	}

	//copy from pixel of other type
	template<typename oT>
	pixel_t_& operator=(const pixel_t_<ch, oT> &o)
	{
		const oT *p = o.get_data();
		for (int i = 0; i < ch; ++i)
			ptr[i] = static_cast<T>(p[i]);
		return *this;
	}

	//copy from pointer
	template<typename oT>
	pixel_t_& operator=(const oT *o)
	{
		for (int i = 0; i < ch; ++i)
			ptr[i] = static_cast<T>(o[i]);
		return *this;
	}

	//for !obj checking
	operator bool() const
	{
		if (!ptr)
			return false;
		for (int i = 0; i < ch; ++i)
			if (ptr[i])
				return true;
		return false;
	}

	friend bool operator==(const pixel_t_ &l, const pixel_t_ &r);
	friend bool operator==(const pixel_t_ &me, T val);
	friend bool operator!=(const pixel_t_ &l, const pixel_t_ &r) { return !(l == r); }

	friend pixel_t_<ch, T> operator+ <>(const pixel_t_<ch, T> &l, const pixel_t_<ch, T> &r);
	friend pixel_t_<ch, T> operator- <>(const pixel_t_<ch, T> &l, const pixel_t_<ch, T> &r);
	friend pixel_t_<ch, T> operator* <>(const pixel_t_<ch, T> &l, const pixel_t_<ch, T> &r);
	friend pixel_t_<ch, T> operator/ <>(const pixel_t_<ch, T> &l, const pixel_t_<ch, T> &r);

	friend pixel_t_<ch, T> operator+ <>(const pixel_t_<ch, T> &pix, float val);
	friend pixel_t_<ch, T> operator- <>(const pixel_t_<ch, T> &pix, float val);
	friend pixel_t_<ch, T> operator* <>(const pixel_t_<ch, T> &pix, float val);
	friend pixel_t_<ch, T> operator/ <>(const pixel_t_<ch, T> &pix, float val);

private:
	pixel_t_(const pixel_t_ &) {}
	T *ptr;
	bool need_free;
};

template<int ch, typename T>
bool operator==(const pixel_t_<ch, T> &l, const pixel_t_<ch, T> &r)
{
	for (int i = 0; i < ch; ++i)
		if (r.ptr[i] != r.ptr[i])
			return false;
	return true;
}

template<int ch, typename T>
bool operator==(const pixel_t_<ch, T> pix, T val)
{
	for (int i = 0; i < ch; ++i)
		if (pix.ptr[i] != val)
			return false;
	return true;
}

template<int ch, typename T>
pixel_t_<ch, T> operator+(const pixel_t_<ch, T> &l, const pixel_t_<ch, T> &r)
{
	pixel_t_<ch, T> res;
	for (int i = 0; i < ch; ++i)
		res.ptr[i] = l.ptr[i] + r.ptr[i];
	return res;
}

template<int ch, typename T>
pixel_t_<ch, T> operator-(const pixel_t_<ch, T> &l, const pixel_t_<ch, T> &r)
{
	pixel_t_<ch, T> res;
	for (int i = 0; i < ch; ++i)
		res.ptr[i] = l.ptr[i] - r.ptr[i];
	return res;
}

template<int ch, typename T>
pixel_t_<ch, T> operator*(const pixel_t_<ch, T> &l, const pixel_t_<ch, T> &r)
{
	pixel_t_<ch, T> res;
	for (int i = 0; i < ch; ++i)
		res.ptr[i] = l.ptr[i] * r.ptr[i];
	return res;
}

template<int ch, typename T>
pixel_t_<ch, T> operator/(const pixel_t_<ch, T> &l, const pixel_t_<ch, T> &r)
{
	pixel_t_<ch, T> res;
	for (int i = 0; i < ch; ++i)
		res.ptr[i] = l.ptr[i] / r.ptr[i];
	return res;
}

template<int ch, typename pix_t>
pixel_t_<ch, pix_t> operator+(const pixel_t_<ch, pix_t> &pix, float val)
{
	pixel_t_<ch, pix_t> res;
	for (int i = 0; i < ch; ++i)
		res.ptr[i] = pix_t(pix.ptr[i] + val);
	return res;
}

template<int ch, typename pix_t>
pixel_t_<ch, pix_t> operator-(const pixel_t_<ch, pix_t> &pix, float val)
{
	pixel_t_<ch, pix_t> res;
	for (int i = 0; i < ch; ++i)
		res.ptr[i] = pix_t(pix.ptr[i] - val);
	return res;
}

template<int ch, typename pix_t>
pixel_t_<ch, pix_t> operator*(const pixel_t_<ch, pix_t> &pix, float val)
{
	pixel_t_<ch, pix_t> res;
	for (int i = 0; i < ch; ++i)
		res.ptr[i] = pix_t(pix.ptr[i] * val);
	return res;
}

template<int ch, typename pix_t>
pixel_t_<ch, pix_t> operator/(const pixel_t_<ch, pix_t> &pix, float val)
{
	pixel_t_<ch, pix_t> res;
	for (int i = 0; i < ch; ++i)
		res.ptr[i] = pix_t(pix.ptr[i] / val);
	return res;
}

}  // namespace aifil

#endif  // AIFIL_PIXEL_H
