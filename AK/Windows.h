#pragma once

#if defined(AK_OS_WINDOWS)
#    include <winsock2.h>
#    include <windows.h>
#    include <ws2tcpip.h>
#    define sighandler_t int
#    define gid_t int
#    define uid_t int
#    define posix_spawn_file_actions_t int
#	 define sockaddr_un int
#    define posix_spawnattr_t int
#if defined(sched_yield)
#    undef sched_yield
#endif
#    define sched_yield(a)
#endif
