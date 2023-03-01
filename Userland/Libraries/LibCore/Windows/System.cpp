/*
 * Copyright (c) 2021-2022, Andreas Kling <kling@serenityos.org>
 * Copyright (c) 2021-2022, Kenneth Myhra <kennethmyhra@serenityos.org>
 * Copyright (c) 2021-2022, Sam Atkins <atkinssj@serenityos.org>
 * Copyright (c) 2022, Matthias Zimmerman <matthias291999@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/DeprecatedString.h>
#include <AK/FixedArray.h>
#include <AK/ScopedValueRollback.h>
#include <AK/StdLibExtras.h>
#include <AK/Vector.h>
#include <AK/Windows.h>
#include <LibCore/File.h>
#include <LibCore/SessionManagement.h>
#include <LibCore/System.h>
#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <winternl.h>

namespace Core::System {

ErrorOr<int> accept4(int sockfd, struct sockaddr*, socklen_t*, int flags);
ErrorOr<void> sigaction(int signal, struct sigaction const* action, struct sigaction* old_action);
ErrorOr<sighandler_t> signal(int signal, sighandler_t handler);
ErrorOr<struct stat> fstat(int fd)
{
    struct stat st = {};
    if (::fstat(fd, &st) < 0)
        return Error::from_syscall("fstat"sv, -errno);
    return st;
}
ErrorOr<int> fcntl(int fd, int command, ...);
ErrorOr<void*> mmap([[maybe_unused]] void* address, size_t size, int protection, int flags, int fd, off_t offset, [[maybe_unused]] size_t alignment, [[maybe_unused]] StringView name)
{
    HANDLE file_handle = (HANDLE)_get_osfhandle(fd);
    HANDLE file_mapping_handle = ::CreateFileMapping(file_handle, NULL, protection, 0, size, NULL);
    if (file_mapping_handle == NULL)
        return Error::from_syscall("CreateFileMapping"sv, GetLastError());

    LPVOID ptr = ::MapViewOfFile(file_mapping_handle, flags, 0, offset, size);
    if (ptr == NULL) {
        ::CloseHandle(file_mapping_handle);
        return Error::from_syscall("MapViewOfFile"sv, GetLastError());
    }

    return ptr;
}
ErrorOr<void> munmap(void* address, [[maybe_unused]] size_t size)
{
    if (::UnmapViewOfFile(address) == 0)
        return Error::from_syscall("UnmapViewOfFile"sv, GetLastError());
    return {};
}
ErrorOr<int> anon_create(size_t size, int options);
ErrorOr<int> open(StringView path, int options, mode_t mode)
{
    return openat(-100, path, options, mode);
}

ErrorOr<int> openat(int fd, StringView path, int options, mode_t mode)
{
    // TODO, properly parse the path and open the file
    (void)fd;
    (void)options;
    (void)mode;

    if (!path.characters_without_null_termination())
        return Error::from_syscall("open"sv, -EFAULT);

    DeprecatedString path_string = path;

    HANDLE file_handle = CreateFileA(
        path_string.characters(),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);

    if (file_handle == INVALID_HANDLE_VALUE) {
        if (GetLastError() == ERROR_FILE_NOT_FOUND) {
            file_handle = CreateFileA(
                path_string.characters(),
                GENERIC_READ | GENERIC_WRITE,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                nullptr,
                CREATE_NEW,
                FILE_ATTRIBUTE_NORMAL,
                nullptr);
        }

        if (file_handle == INVALID_HANDLE_VALUE) {
            return Error::from_syscall("open"sv, -errno);
        }
    }

    int rc = _open_osfhandle((intptr_t)file_handle, 0);
    if (rc < 0)
        return Error::from_syscall("open"sv, -errno);
    return rc;
}
ErrorOr<void> close(int fd)
{
    _close(fd);
    CloseHandle((HANDLE)_get_osfhandle(fd));
    return {};
}
ErrorOr<void> ftruncate(int fd, off_t length);
ErrorOr<struct stat> stat(StringView path)
{
    if (!path.characters_without_null_termination())
        return Error::from_syscall("stat"sv, -EFAULT);

    struct stat st = {};
    DeprecatedString path_string = path;
    if (::stat(path_string.characters(), &st) < 0)
        return Error::from_syscall("stat"sv, -errno);
    return st;
}
ErrorOr<struct stat> lstat(StringView path);
ErrorOr<ssize_t> read(int fd, Bytes buffer)
{
    ssize_t rc = ::read(fd, buffer.data(), buffer.size());
    if (rc < 0)
        return Error::from_syscall("read"sv, -errno);
    return rc;
}
ErrorOr<ssize_t> write(int fd, ReadonlyBytes buffer)
{
    ssize_t rc = ::write(fd, buffer.data(), buffer.size());
    if (rc < 0)
        return Error::from_syscall("write"sv, -errno);
    return rc;
}
ErrorOr<void> kill(pid_t, int)
{
    dbgln("FIXME: Implement kill()");
    return {};
}
ErrorOr<void> killpg(int pgrp, int signal);
ErrorOr<int> dup(int source_fd)
{
    int rc = ::dup(source_fd);
    if (rc < 0)
        return Error::from_syscall("dup"sv, -errno);
    return rc;
}
ErrorOr<int> dup2(int source_fd, int destination_fd);
ErrorOr<DeprecatedString> ptsname(int fd);
ErrorOr<DeprecatedString> gethostname();
ErrorOr<void> sethostname(StringView);
ErrorOr<DeprecatedString> getcwd();
ErrorOr<void> ioctl(int fd, unsigned request, ...);
ErrorOr<struct termios> tcgetattr(int fd);
ErrorOr<void> tcsetattr(int fd, int optional_actions, struct termios const&);
ErrorOr<int> tcsetpgrp(int fd, pid_t pgrp);
ErrorOr<void> chmod(StringView pathname, mode_t mode);
ErrorOr<void> lchown(StringView pathname, uid_t uid, gid_t gid);
ErrorOr<void> chown(StringView pathname, uid_t uid, gid_t gid);
ErrorOr<Optional<struct passwd>> getpwent(Span<char> buffer);
ErrorOr<Optional<struct passwd>> getpwnam(StringView name);
ErrorOr<Optional<struct group>> getgrnam(StringView name);
ErrorOr<Optional<struct passwd>> getpwuid(uid_t);
ErrorOr<Optional<struct group>> getgrent(Span<char> buffer);
ErrorOr<Optional<struct group>> getgrgid(gid_t);
ErrorOr<void> clock_settime(clockid_t clock_id, struct timespec* ts);
ErrorOr<pid_t> posix_spawn(StringView path, posix_spawn_file_actions_t const* file_actions, posix_spawnattr_t const* attr, char* const arguments[], char* const envp[])
{
	(void)path;
	(void)file_actions;
	(void)attr;
	(void)arguments;
	(void)envp;
	dbgln("FIXME: Implement posix_spawn()");
	VERIFY_NOT_REACHED();
}
ErrorOr<pid_t> posix_spawnp(StringView path, posix_spawn_file_actions_t* const file_actions, posix_spawnattr_t* const attr, char* const arguments[], char* const envp[]);
ErrorOr<off_t> lseek(int fd, off_t offset, int whence)
{
    off_t rc = ::lseek(fd, offset, whence);
    if (rc < 0)
        return Error::from_syscall("lseek"sv, -errno);
    return rc;
}
ErrorOr<void> endgrent();
ErrorOr<WaitPidResult> waitpid(pid_t waitee, int options);
ErrorOr<void> setuid(uid_t);
ErrorOr<void> seteuid(uid_t);
ErrorOr<void> setgid(gid_t);
ErrorOr<void> setegid(gid_t);
ErrorOr<void> setpgid(pid_t pid, pid_t pgid);
ErrorOr<pid_t> setsid();
ErrorOr<pid_t> getsid(pid_t pid);
ErrorOr<void> drop_privileges();
ErrorOr<bool> isatty(int fd);
ErrorOr<void> link(StringView old_path, StringView new_path);
ErrorOr<void> symlink(StringView target, StringView link_path);
ErrorOr<void> mkdir(StringView path, mode_t)
{
    if (path.is_null())
        return Error::from_errno(EFAULT);
    DeprecatedString path_string = path;
    if (::mkdir(path_string.characters()) < 0)
        return Error::from_syscall("mkdir"sv, -errno);
    return {};
}
ErrorOr<void> chdir(StringView path);
ErrorOr<void> rmdir(StringView path);
ErrorOr<pid_t> fork();
ErrorOr<int> mkstemp(Span<char> pattern);
ErrorOr<void> fchmod(int fd, mode_t mode);
ErrorOr<void> fchown(int, uid_t, gid_t)
{
    dbgln("FIXME: Implement fchown");
    return {};
}
ErrorOr<void> rename(StringView old_path, StringView new_path);
ErrorOr<void> unlink(StringView path)
{
    if (path.is_null())
        return Error::from_errno(EFAULT);

    DeprecatedString path_string = path;
    if (::unlink(path_string.characters()) < 0)
        return Error::from_syscall("unlink"sv, -errno);
    return {};
}
ErrorOr<void> utime(StringView path, Optional<struct utimbuf>);
ErrorOr<struct utsname> uname();
ErrorOr<Array<int, 2>> pipe2(int flags);
ErrorOr<void> adjtime(const struct timeval* delta, struct timeval* old_delta)
{
    (void)delta;
    (void)old_delta;
    dbgln("FIXME: Implement adjtime");
    VERIFY_NOT_REACHED();
}
ErrorOr<void> exec(StringView filename, ReadonlySpan<StringView> arguments, SearchInPath, Optional<ReadonlySpan<StringView>> environment)
{
    (void)filename;
    (void)arguments;
    (void)environment;
    dbgln("FIXME: Implement exec()");
    VERIFY_NOT_REACHED();
}
ErrorOr<int> socket(int domain, int type, int protocol)
{
    int rc = ::socket(domain, type, protocol);
    if (rc < 0)
        return Error::from_syscall("socket"sv, -errno);
    return rc;
}
ErrorOr<void> bind(int sockfd, struct sockaddr const* address, socklen_t address_length)
{
    if (::bind(sockfd, address, address_length) < 0)
        return Error::from_syscall("bind"sv, -errno);
    return {};
}
ErrorOr<void> listen(int sockfd, int backlog)
{
    if (::listen(sockfd, backlog) < 0)
        return Error::from_syscall("listen"sv, -errno);
    return {};
}
ErrorOr<int> accept(int sockfd, struct sockaddr* address, socklen_t* address_length)
{
    auto fd = ::accept(sockfd, address, address_length);
    if (fd < 0)
        return Error::from_syscall("accept"sv, -errno);
    return fd;
}
ErrorOr<void> connect(int sockfd, struct sockaddr const*, socklen_t);
ErrorOr<void> shutdown(int sockfd, int how);
ErrorOr<ssize_t> send(int sockfd, void const*, size_t, int flags);
ErrorOr<ssize_t> sendmsg(int sockfd, const struct msghdr*, int flags);
ErrorOr<ssize_t> sendto(int sockfd, void const*, size_t, int flags, struct sockaddr const*, socklen_t);
ErrorOr<ssize_t> recv(int sockfd, void*, size_t, int flags);
ErrorOr<ssize_t> recvmsg(int sockfd, struct msghdr*, int flags);
ErrorOr<ssize_t> recvfrom(int sockfd, void*, size_t, int flags, struct sockaddr*, socklen_t*);
ErrorOr<void> getsockopt(int sockfd, int level, int option, void* value, socklen_t* value_size)
{
    if (::getsockopt(sockfd, level, option, (char*)value, value_size) < 0)
        return Error::from_syscall("getsockopt"sv, -errno);
    return {};
}
ErrorOr<void> setsockopt(int sockfd, int level, int option, void const* value, socklen_t value_size)
{
    if (::setsockopt(sockfd, level, option, (char*)value, value_size) < 0)
        return Error::from_syscall("setsockopt"sv, -errno);
    return {};
}
ErrorOr<void> getsockname(int sockfd, struct sockaddr*, socklen_t*);
ErrorOr<void> getpeername(int sockfd, struct sockaddr*, socklen_t*);
ErrorOr<void> socketpair(int domain, int type, int protocol, int sv[2])
ErrorOr<Vector<gid_t>> getgroups();
ErrorOr<void> setgroups(ReadonlySpan<gid_t>);
ErrorOr<void> mknod(StringView pathname, mode_t mode, dev_t dev);
ErrorOr<void> mkfifo(StringView pathname, mode_t mode);
ErrorOr<void> setenv(StringView, StringView, bool);
ErrorOr<void> putenv(StringView);
ErrorOr<int> posix_openpt(int flags);
ErrorOr<void> grantpt(int fildes);
ErrorOr<void> unlockpt(int fildes);
ErrorOr<void> access(StringView pathname, int mode);
ErrorOr<DeprecatedString> readlink(StringView pathname);
ErrorOr<int> poll(Span<struct pollfd>, int timeout);

}
