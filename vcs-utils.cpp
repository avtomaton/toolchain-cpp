#include "vcs-utils.h"

#include <cstdio>

#ifdef _WIN32
#define popen _popen
#define pclose _pclose
#endif

namespace aifil {

int hg_revision()
{
	int rev = 0;
	FILE *tmp = popen("hg summary", "r");
	if (!tmp)
		return rev;

	char buf0[1024];
	char buf1[1024];
	fscanf(tmp, "%s %d:%s", buf0, &rev, buf1);
	pclose(tmp);
	return rev;
}

} // namespace aifil
