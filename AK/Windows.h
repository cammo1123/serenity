#pragma once

#if defined(AK_OS_WINDOWS)
#    include <io.h>
#    include <winsock2.h>
#    include <windows.h>
#    include <ws2tcpip.h>

#    define sighandler_t int
#    define posix_spawn_file_actions_t int
#    define sockaddr_un int
#    define timegm _mkgmtime
#    define posix_spawnattr_t int
#    if defined(sched_yield)
#        undef sched_yield
#    endif
#    if defined(interface)
#        undef interface
#    endif
#  if defined(BI_BITFIELDS)
#    undef BI_BITFIELDS
#  endif
#if defined(BI_RGB)
#    undef BI_RGB
#  endif
#    define sched_yield(a)
#endif
