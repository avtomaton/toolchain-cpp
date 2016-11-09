#ifndef __AIFIL_C_ATTRIBS__H__
#define __AIFIL_C_ATTRIBS__H__

#if __GNUC__ >= 3
#define AF_FORMAT(n, f, e)		__attribute__ ((format (n, f, e)))
#else
#define AF_FORMAT(n, f, e)
#endif

// TODO: msvs format check
// see: https://msdn.microsoft.com/en-us/library/ms235402(v=VS.100).aspx


#endif //__AIFIL_C_ATTRIBS__H__
