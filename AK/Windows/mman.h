#pragma once

#if defined(AK_OS_WINDOWS)
#	define PROT_READ 0
#	define PROT_WRITE 0
#	define MAP_FILE 0
#	define MAP_SHARED 0
#	define MAP_FAILED 0
#	define MAP_PRIVATE 0
#	define MAP_ANONYMOUS 0
#endif
