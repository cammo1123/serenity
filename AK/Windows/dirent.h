#pragma once

#include <Windows.h>
#include <dirent.h>

struct __dir
{
	struct dirent* entries;
	intptr_t fd;
	long int count;
	long int index;
};

static intptr_t dirfd(DIR * dirp)
{
	if (!dirp) {
		errno = EINVAL;
		return -1;
	}
	return ((struct __dir*)dirp)->fd;
}
