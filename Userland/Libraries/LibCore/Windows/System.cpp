/*
 * Copyright (c) 2021-2022, Andreas Kling <kling@serenityos.org>
 * Copyright (c) 2021-2022, Kenneth Myhra <kennethmyhra@serenityos.org>
 * Copyright (c) 2021-2022, Sam Atkins <atkinssj@serenityos.org>
 * Copyright (c) 2022, Matthias Zimmerman <matthias291999@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "AK/Format.h"
#include <AK/DeprecatedString.h>
#include <AK/FixedArray.h>
#include <AK/ScopedValueRollback.h>
#include <AK/StdLibExtras.h>
#include <AK/Vector.h>
#include <AK/Windows.h>
#include <LibCore/File.h>
#include <LibCore/SessionManagement.h>
#include <LibCore/System.h>
#include <fileapi.h>
#include <limits.h>
#include <processenv.h>
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
    // CloseHandle((HANDLE)_get_osfhandle(fd));
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
ErrorOr<void> killpg(int, int)
{
    dbgln("FIXME: Implement killpg()");
    return {};
}
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
ErrorOr<void> chmod(StringView pathname, mode_t mode);
ErrorOr<off_t> lseek(int fd, off_t offset, int whence)
{
    off_t rc = ::lseek(fd, offset, whence);
    if (rc < 0)
        return Error::from_syscall("lseek"sv, -errno);
    return rc;
}
ErrorOr<void> endgrent();
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
ErrorOr<void> exec(StringView filename, ReadonlySpan<StringView> arguments, SearchInPath search_in_path, Optional<ReadonlySpan<StringView>> environment)
{
#ifdef AK_OS_SERENITY
    Syscall::SC_execve_params params;

    auto argument_strings = TRY(FixedArray<Syscall::StringArgument>::create(arguments.size()));
    for (size_t i = 0; i < arguments.size(); ++i) {
        argument_strings[i] = { arguments[i].characters_without_null_termination(), arguments[i].length() };
    }
    params.arguments.strings = argument_strings.data();
    params.arguments.length = argument_strings.size();

    size_t env_count = 0;
    if (environment.has_value()) {
        env_count = environment->size();
    } else {
        for (size_t i = 0; environ[i]; ++i)
            ++env_count;
    }

    auto environment_strings = TRY(FixedArray<Syscall::StringArgument>::create(env_count));
    if (environment.has_value()) {
        for (size_t i = 0; i < env_count; ++i) {
            environment_strings[i] = { environment->at(i).characters_without_null_termination(), environment->at(i).length() };
        }
    } else {
        for (size_t i = 0; i < env_count; ++i) {
            environment_strings[i] = { environ[i], strlen(environ[i]) };
        }
    }
    params.environment.strings = environment_strings.data();
    params.environment.length = environment_strings.size();

    auto run_exec = [](Syscall::SC_execve_params& params) -> ErrorOr<void> {
        int rc = syscall(Syscall::SC_execve, &params);
        if (rc < 0)
            return Error::from_syscall("exec"sv, rc);
        return {};
    };

    DeprecatedString exec_filename;

    if (search_in_path == SearchInPath::Yes) {
        auto maybe_executable = Core::File::resolve_executable_from_environment(filename);

        if (!maybe_executable.has_value())
            return ENOENT;

        exec_filename = maybe_executable.release_value();
    } else {
        exec_filename = filename.to_deprecated_string();
    }

    params.path = { exec_filename.characters(), exec_filename.length() };
    TRY(run_exec(params));
    VERIFY_NOT_REACHED();
#else
    DeprecatedString filename_string { filename };

    auto argument_strings = TRY(FixedArray<DeprecatedString>::create(arguments.size()));
    auto argv = TRY(FixedArray<char*>::create(arguments.size() + 1));
    for (size_t i = 0; i < arguments.size(); ++i) {
        argument_strings[i] = arguments[i].to_deprecated_string();
        dbgln("argv[{}]: {}", i, argument_strings[i]);
        argv[i] = const_cast<char*>(argument_strings[i].characters());
    }
    argv[arguments.size()] = nullptr;

    int rc = 0;
    if (environment.has_value()) {
        auto environment_strings = TRY(FixedArray<DeprecatedString>::create(environment->size()));
        auto envp = TRY(FixedArray<char*>::create(environment->size() + 1));
        for (size_t i = 0; i < environment->size(); ++i) {
            environment_strings[i] = environment->at(i).to_deprecated_string();
            envp[i] = const_cast<char*>(environment_strings[i].characters());
        }
        envp[environment->size()] = nullptr;

        if (search_in_path == SearchInPath::Yes && !filename.contains('/')) {
#    if defined(AK_OS_MACOS) || defined(AK_OS_FREEBSD)
            // These BSDs don't support execvpe(), so we'll have to manually search the PATH.
            ScopedValueRollback errno_rollback(errno);

            auto maybe_executable = Core::File::resolve_executable_from_environment(filename_string);

            if (!maybe_executable.has_value()) {
                errno_rollback.set_override_rollback_value(ENOENT);
                return Error::from_errno(ENOENT);
            }

            rc = ::execve(maybe_executable.release_value().characters(), argv.data(), envp.data());
#    else
            rc = ::_execvpe(filename_string.characters(), argv.data(), envp.data());
#    endif
        } else {
            rc = ::execve(filename_string.characters(), argv.data(), envp.data());
        }

    } else {
        if (search_in_path == SearchInPath::Yes) {
            rc = ::execvp(filename_string.characters(), argv.data());
        } else {
            rc = ::execv(filename_string.characters(), argv.data());
        }
    }

    if (rc < 0)
        return Error::from_syscall("exec"sv, rc);
    VERIFY_NOT_REACHED();
#endif
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
ErrorOr<ssize_t> send(int sockfd, void const* buffer, size_t buffer_length, int flags)
{
    char const* buffer_ptr = (char const*)buffer;
    int buffer_length_int = (int)buffer_length;

    dbgln("send: sockfd={}, buffer={}, buffer_length={}, flags={}", sockfd, buffer_ptr, buffer_length_int, flags);

    auto sent = ::send(sockfd, buffer_ptr, buffer_length_int, flags);
    if (sent < 0)
        return Error::from_syscall("send"sv, -errno);

    return sent;
}
ErrorOr<ssize_t> sendmsg(int sockfd, const struct msghdr*, int flags);
ErrorOr<ssize_t> sendto(int sockfd, void const*, size_t, int flags, struct sockaddr const*, socklen_t);
ErrorOr<ssize_t> recv(int sockfd, void* buffer, size_t length, int flags)
{
    auto received = ::recv(sockfd, (char*)buffer, (int)length, flags);
    if (received < 0)
        return Error::from_syscall("recv"sv, -errno);
    return received;
}
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
ErrorOr<void> socketpair(int domain, int type, int protocol, int sv[2]);
ErrorOr<Vector<gid_t>> getgroups();
ErrorOr<void> setgroups(ReadonlySpan<gid_t>);
ErrorOr<void> mknod(StringView pathname, mode_t mode, dev_t dev);
ErrorOr<void> mkfifo(StringView pathname, mode_t mode);
ErrorOr<void> setenv(StringView name, StringView value, bool overwrite)
{
    auto builder = TRY(StringBuilder::create());
    TRY(builder.try_append(name));
    TRY(builder.try_append('\0'));
    TRY(builder.try_append(value));
    TRY(builder.try_append('\0'));
    // Note the explicit null terminators above.
    auto c_name = builder.string_view().characters_without_null_termination();
    auto c_value = c_name + name.length() + 1;

    if (!overwrite && GetEnvironmentVariableA(c_name, nullptr, 0) > 0) {
        printf("setenv: %s already exists\n", c_name);
        return {};
    }
    SetEnvironmentVariable(c_name, c_value);

    return {};
}
ErrorOr<void> putenv(StringView);
ErrorOr<int> posix_openpt(int flags);
ErrorOr<void> grantpt(int fildes);
ErrorOr<void> unlockpt(int fildes);
ErrorOr<void> access(StringView pathname, int mode);
ErrorOr<DeprecatedString> readlink(StringView pathname);
ErrorOr<int> poll(Span<struct pollfd>, int timeout);

}
