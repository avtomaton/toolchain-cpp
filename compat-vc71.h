#ifndef AIFIL_UTILS_COMPATVC71_H
#define AIFIL_UTILS_COMPATVC71_H

//fseek/ftell WITHOUT LOCKING!!! Don't use simaltenously!
#if defined(_MSC_VER) && (_MSC_VER <= 1310)

#include <cstdio>

namespace aifil {

int __cdecl _fseeki64(FILE *, __int64, int);

} //namespace aifil

#endif //_MSC_VER <= 1310
#endif //AIFIL_UTILS_COMPATVC71_H
