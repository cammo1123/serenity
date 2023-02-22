#pragma once

#if defined(AK_OS_WINDOWS)
#    define gid_t int
#    define uid_t int
#    define AF_LOCAL AF_UNIX
#endif
