#include "compat-vc71.h"
#include "errutils.h"

namespace aifil {

#if defined(_MSC_VER) && (_MSC_VER <= 1310)

extern "C" __int64 __cdecl _lseeki64(int fh, __int64 pos, int mthd);
int __cdecl _fseeki64(FILE *str,  __int64 offset, int whence)
{
	af_assert(whence != SEEK_CUR && "I don't know SEEK_CUR's behaviour in VC7.1");
//	register FILE *stream;
//	rs_assert(str != NULL);
//	stream = str;

	/* Flush buffer as necessary */
	fflush(str);
	if (str->_flag & _IORW)
		str->_flag &= ~(_IOWRT|_IOREAD);
//	_flush(stream);
//	/* If file opened for read/write, clear flags since we don't know
//		   what the user is going to do next. If the file was opened for
//		   read access only, decrease _bufsiz so that the next _filbuf
//		   won't cost quite so much */
//	if (stream->_flag & _IORW)
//		stream->_flag &= ~(_IOWRT|_IOREAD);
//	else if ( (stream->_flag & _IOREAD) && (stream->_flag & _IOMYBUF) &&
//			  !(stream->_flag & _IOSETVBUF) )
//		stream->_bufsiz = _SMALL_BUFSIZ;

	/* Seek to the desired locale and return. */
//	return(_lseeki64(_fileno(stream), offset, whence) == -1i64 ? -1 : 0);
	return(_lseeki64(_fileno(str), offset, whence) == -1i64 ? -1 : 0);
}

#endif

} //namespace aifil
