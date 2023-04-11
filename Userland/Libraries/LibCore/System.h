/*
 * Copyright (c) 2021, Andreas Kling <kling@serenityos.org>
 * Copyright (c) 2021-2022, Kenneth Myhra <kennethmyhra@serenityos.org>
 * Copyright (c) 2021, Sam Atkins <atkinssj@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Error.h>
#include <AK/Noncopyable.h>
#include <AK/OwnPtr.h>
#include <AK/StringView.h>
#include <AK/Vector.h>
#include <signal.h>
#include <sys/stat.h>
#include <time.h>
#if !defined(AK_OS_WINDOWS)
#    include <dirent.h>
#    include <fcntl.h>
#    include <grp.h>
#    include <netdb.h>
#    include <poll.h>
#    include <pwd.h>
#    include <spawn.h>
#    include <sys/ioctl.h>
#    include <sys/socket.h>
#    include <sys/time.h>
#    include <sys/utsname.h>
#    include <sys/wait.h>
#    include <termios.h>
#    include <utime.h>
#else
// Windows doesn't define W_OK, but it's used in Core::System::access().
// https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/access-waccess?view=msvc-170#remarks
#    if defined(AK_OS_WINDOWS)
#        define W_OK 2
#    endif
#    if !defined(PATH_MAX)
#        define PATH_MAX MAX_PATH
#    endif
#    include <WS2tcpip.h>
#    include <WinSock2.h>
#    include <io.h>
using sighandler_t = _crt_signal_t;
#endif

#if !defined(AK_OS_BSD_GENERIC) && !defined(AK_OS_ANDROID) && !defined(AK_OS_WINDOWS)
#    include <shadow.h>
#endif

#ifdef AK_OS_SOLARIS
#    include <sys/filio.h>
#    include <ucred.h>
#endif

namespace Core::System {

#ifdef AK_OS_SERENITY
ErrorOr<void> beep(Optional<size_t> tone);
ErrorOr<void> pledge(StringView promises, StringView execpromises = {});
ErrorOr<void> unveil(StringView path, StringView permissions);
ErrorOr<void> unveil_after_exec(StringView path, StringView permissions);
ErrorOr<void> sendfd(int sockfd, int fd);
ErrorOr<int> recvfd(int sockfd, int options);
ErrorOr<void> ptrace_peekbuf(pid_t tid, void const* tracee_addr, Bytes destination_buf);
ErrorOr<void> mount(int source_fd, StringView target, StringView fs_type, int flags);
ErrorOr<void> umount(StringView mount_point);
ErrorOr<long> ptrace(int request, pid_t tid, void* address, void* data);
ErrorOr<void> disown(pid_t pid);
ErrorOr<void> profiling_enable(pid_t, u64 event_mask);
ErrorOr<void> profiling_disable(pid_t);
ErrorOr<void> profiling_free_buffer(pid_t);
#else
inline ErrorOr<void> unveil(StringView, StringView)
{
    return {};
}
inline ErrorOr<void> pledge(StringView, StringView = {}) { return {}; }
#endif

template<size_t N>
ALWAYS_INLINE ErrorOr<void> pledge(char const (&promises)[N])
{
    return pledge(StringView { promises, N - 1 });
}

template<size_t NPromises, size_t NExecPromises>
ALWAYS_INLINE ErrorOr<void> pledge(char const (&promises)[NPromises], char const (&execpromises)[NExecPromises])
{
    return pledge(StringView { promises, NPromises - 1 }, StringView { execpromises, NExecPromises - 1 });
}

template<size_t NPath, size_t NPermissions>
ALWAYS_INLINE ErrorOr<void> unveil(char const (&path)[NPath], char const (&permissions)[NPermissions])
{
    return unveil(StringView { path, NPath - 1 }, StringView { permissions, NPermissions - 1 });
}

ALWAYS_INLINE ErrorOr<void> unveil(nullptr_t, nullptr_t)
{
    return unveil(StringView {}, StringView {});
}

#if !defined(AK_OS_BSD_GENERIC) && !defined(AK_OS_ANDROID)
ErrorOr<Optional<struct spwd>> getspent();
ErrorOr<Optional<struct spwd>> getspnam(StringView name);
#endif

#ifndef AK_OS_MACOS
ErrorOr<int> accept4(int sockfd, struct sockaddr*, socklen_t*, int flags);
#endif

ErrorOr<void> sigaction(int signal, struct sigaction const* action, struct sigaction* old_action);
#if defined(AK_OS_SOLARIS)
ErrorOr<SIG_TYP> signal(int signal, SIG_TYP handler);
#elif defined(AK_OS_BSD_GENERIC)
ErrorOr<sig_t> signal(int signal, sig_t handler);
#else
ErrorOr<sighandler_t> signal(int signal, sighandler_t handler);
#endif
ErrorOr<struct stat> fstat(int fd);
ErrorOr<int> fcntl(int fd, int command, ...);
ErrorOr<void*> mmap(void* address, size_t, int protection, int flags, int fd, off_t, size_t alignment = 0, StringView name = {});
ErrorOr<void> munmap(void* address, size_t);
ErrorOr<int> anon_create(size_t size, int options);
ErrorOr<int> open(StringView path, int options, mode_t mode = 0);
ErrorOr<int> openat(int fd, StringView path, int options, mode_t mode = 0);
ErrorOr<void> close(int fd);
ErrorOr<void> ftruncate(int fd, off_t length);
ErrorOr<struct stat> stat(StringView path);
ErrorOr<struct stat> lstat(StringView path);
ErrorOr<ssize_t> read(int fd, Bytes buffer);
ErrorOr<ssize_t> write(int fd, ReadonlyBytes buffer);
ErrorOr<int> dup(int source_fd);
ErrorOr<int> dup2(int source_fd, int destination_fd);
ErrorOr<DeprecatedString> ptsname(int fd);
ErrorOr<DeprecatedString> gethostname();
ErrorOr<void> sethostname(StringView);
ErrorOr<DeprecatedString> getcwd();
ErrorOr<void> ioctl(int fd, unsigned request, ...);
ErrorOr<struct termios> tcgetattr(int fd);
ErrorOr<void> tcsetattr(int fd, int optional_actions, struct termios const&);
ErrorOr<void> chmod(StringView pathname, mode_t mode);
ErrorOr<off_t> lseek(int fd, off_t, int whence);
ErrorOr<bool> isatty(int fd);
ErrorOr<void> link(StringView old_path, StringView new_path);
ErrorOr<void> symlink(StringView target, StringView link_path);
ErrorOr<void> mkdir(StringView path, mode_t);
ErrorOr<void> chdir(StringView path);
ErrorOr<void> rmdir(StringView path);
ErrorOr<int> mkstemp(Span<char> pattern);
ErrorOr<String> mkdtemp(Span<char> pattern);
ErrorOr<void> fchmod(int fd, mode_t mode);
ErrorOr<void> rename(StringView old_path, StringView new_path);
ErrorOr<void> unlink(StringView path);
ErrorOr<void> utime(StringView path, Optional<struct utimbuf>);
ErrorOr<void> utimensat(int fd, StringView path, struct timespec const times[2], int flag);
ErrorOr<Array<int, 2>> pipe2(int flags);
#ifndef AK_OS_ANDROID
ErrorOr<void> adjtime(const struct timeval* delta, struct timeval* old_delta);
#endif
enum class SearchInPath {
    No,
    Yes,
};

#ifdef AK_OS_SERENITY
ErrorOr<void> exec_command(Vector<StringView>& command, bool preserve_env);
#endif

ErrorOr<void> exec(StringView filename, ReadonlySpan<StringView> arguments, SearchInPath, Optional<ReadonlySpan<StringView>> environment = {});

#ifdef AK_OS_SERENITY
ErrorOr<void> join_jail(u64 jail_index);
ErrorOr<u64> create_jail(StringView jail_name);
#endif

ErrorOr<int> socket(int domain, int type, int protocol);
ErrorOr<void> bind(int sockfd, struct sockaddr const*, socklen_t);
ErrorOr<void> listen(int sockfd, int backlog);
ErrorOr<int> accept(int sockfd, struct sockaddr*, socklen_t*);
ErrorOr<void> connect(int sockfd, struct sockaddr const*, socklen_t);
ErrorOr<void> shutdown(int sockfd, int how);
ErrorOr<ssize_t> send(int sockfd, void const*, size_t, int flags);
ErrorOr<ssize_t> sendmsg(int sockfd, const struct msghdr*, int flags);
ErrorOr<ssize_t> sendto(int sockfd, void const*, size_t, int flags, struct sockaddr const*, socklen_t);
ErrorOr<ssize_t> recv(int sockfd, void*, size_t, int flags);
ErrorOr<ssize_t> recvmsg(int sockfd, struct msghdr*, int flags);
ErrorOr<ssize_t> recvfrom(int sockfd, void*, size_t, int flags, struct sockaddr*, socklen_t*);
ErrorOr<void> getsockopt(int sockfd, int level, int option, void* value, socklen_t* value_size);
ErrorOr<void> setsockopt(int sockfd, int level, int option, void const* value, socklen_t value_size);
ErrorOr<void> getsockname(int sockfd, struct sockaddr*, socklen_t*);
ErrorOr<void> getpeername(int sockfd, struct sockaddr*, socklen_t*);
ErrorOr<void> socketpair(int domain, int type, int protocol, int sv[2]);
ErrorOr<void> setenv(StringView, StringView, bool);
ErrorOr<void> putenv(StringView);
ErrorOr<int> posix_openpt(int flags);
ErrorOr<void> grantpt(int fildes);
ErrorOr<void> unlockpt(int fildes);
ErrorOr<void> access(StringView pathname, int mode);
ErrorOr<DeprecatedString> readlink(StringView pathname);
ErrorOr<int> poll(Span<struct pollfd>, int timeout);

#if !defined(AK_OS_WINDOWS)
ErrorOr<void> kill(pid_t, int signal);
ErrorOr<void> killpg(pid_t, int signal);
ErrorOr<void> clock_settime(clockid_t clock_id, struct timespec* ts);
ErrorOr<Vector<gid_t>> getgroups();
ErrorOr<void> setgroups(Span<gid_t const>);
ErrorOr<void> mknod(StringView pathname, mode_t mode, dev_t dev);
ErrorOr<void> mkfifo(StringView pathname, mode_t mode);
ErrorOr<void> fchown(int fd, uid_t, gid_t);
ErrorOr<struct utsname> uname();
ErrorOr<pid_t> fork();
ErrorOr<pid_t> posix_spawn(StringView path, posix_spawn_file_actions_t const* file_actions, posix_spawnattr_t const* attr, char* const arguments[], char* const envp[]);
ErrorOr<pid_t> posix_spawnp(StringView path, posix_spawn_file_actions_t* const file_actions, posix_spawnattr_t* const attr, char* const arguments[], char* const envp[]);
ErrorOr<int> tcsetpgrp(int fd, pid_t pgrp);
struct WaitPidResult {
    pid_t pid;
    int status;
};
ErrorOr<void> lchown(StringView pathname, uid_t uid, gid_t gid);
ErrorOr<void> chown(StringView pathname, uid_t uid, gid_t gid);
ErrorOr<Optional<struct passwd>> getpwent(Span<char> buffer);
ErrorOr<Optional<struct passwd>> getpwnam(StringView name);
ErrorOr<Optional<struct group>> getgrnam(StringView name);
ErrorOr<Optional<struct passwd>> getpwuid(uid_t);
ErrorOr<Optional<struct group>> getgrent(Span<char> buffer);
ErrorOr<Optional<struct group>> getgrgid(gid_t);
ErrorOr<void> endgrent();

ErrorOr<WaitPidResult> waitpid(pid_t waitee, int options = 0);
ErrorOr<void> setuid(uid_t);
ErrorOr<void> seteuid(uid_t);
ErrorOr<void> setgid(gid_t);
ErrorOr<void> setegid(gid_t);
ErrorOr<void> setpgid(pid_t pid, pid_t pgid);
ErrorOr<pid_t> setsid();
ErrorOr<pid_t> getsid(pid_t pid = 0);
ErrorOr<void> drop_privileges();
#endif

class AddressInfoVector {
    AK_MAKE_NONCOPYABLE(AddressInfoVector);

public:
    AddressInfoVector(AddressInfoVector&&) = default;
    ~AddressInfoVector() = default;

    ReadonlySpan<struct addrinfo> addresses() const { return m_addresses; }

private:
    friend ErrorOr<AddressInfoVector> getaddrinfo(char const* nodename, char const* servname, struct addrinfo const& hints);

    AddressInfoVector(Vector<struct addrinfo>&& addresses, struct addrinfo* ptr)
        : m_addresses(move(addresses))
        , m_ptr(adopt_own_if_nonnull(ptr))
    {
    }

    struct AddrInfoDeleter {
        void operator()(struct addrinfo* ptr) { ::freeaddrinfo(ptr); }
    };

    Vector<struct addrinfo> m_addresses {};
    OwnPtr<struct addrinfo, AddrInfoDeleter> m_ptr {};
};

ErrorOr<AddressInfoVector> getaddrinfo(char const* nodename, char const* servname, struct addrinfo const& hints);

#ifdef AK_OS_SERENITY
ErrorOr<void> posix_fallocate(int fd, off_t offset, off_t length);
#endif

}
