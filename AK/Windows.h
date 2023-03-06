#pragma once

#if defined(AK_OS_WINDOWS)
#    if defined(_WIN32_WINNT)
#        undef _WIN32_WINNT
#    endif
#    if defined(WINVER)
#        undef WINVER
#    endif
#    define _WIN32_WINNT 0x0603
#    define WINVER 0x0603
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
#    if defined(BI_BITFIELDS)
#        undef BI_BITFIELDS
#    endif
#    if defined(BI_RGB)
#        undef BI_RGB
#    endif
#    if defined(NOERROR)
#        undef NOERROR
#    endif
#    if defined(IN)
#        undef IN
#    endif
#    define sched_yield(a)
#    if defined(WAVE_FORMAT_PCM)
#        undef WAVE_FORMAT_PCM
#    endif
#    define sched_yield(a)
#endif
