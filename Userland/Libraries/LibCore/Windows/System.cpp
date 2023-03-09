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
#include <AK/Vector.h>
#include <LibCore/File.h>
#include <LibCore/System.h>
#include <unistd.h>

namespace Core::System {

ErrorOr<struct stat> fstat(int fd)
{
    struct stat st = {};
    if (::fstat(fd, &st) < 0)
        return Error::from_syscall("fstat"sv, -errno);
    return st;
}
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
    return {};
}
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
ErrorOr<off_t> lseek(int fd, off_t offset, int whence)
{
    off_t rc = ::lseek(fd, offset, whence);
    if (rc < 0)
        return Error::from_syscall("lseek"sv, -errno);
    return rc;
}
ErrorOr<void> mkdir(StringView path, mode_t)
{
    if (path.is_null())
        return Error::from_errno(EFAULT);
    DeprecatedString path_string = path;
    if (::mkdir(path_string.characters()) < 0)
        return Error::from_syscall("mkdir"sv, -errno);
    return {};
}
ErrorOr<void> ftruncate(int fd, off_t length)
{
    if (::ftruncate(fd, length) < 0)
        return Error::from_syscall("ftruncate"sv, -errno);
    return {};
}
ErrorOr<void> fchown(int, uid_t, gid_t)
{
    dbgln("FIXME: Implement fchown");
    return {};
}
ErrorOr<void> unlink(StringView path)
{
    if (path.is_null())
        return Error::from_errno(EFAULT);

    DeprecatedString path_string = path;
    if (::unlink(path_string.characters()) < 0)
        return Error::from_syscall("unlink"sv, -errno);
    return {};
}
ErrorOr<void> adjtime(const struct timeval* delta, struct timeval* old_delta)
{
    (void)delta;
    (void)old_delta;
    dbgln("FIXME: Implement adjtime");
    VERIFY_NOT_REACHED();
}
ErrorOr<void> exec(StringView filename, ReadonlySpan<StringView> arguments, SearchInPath search_in_path, Optional<ReadonlySpan<StringView>> environment)
{
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
            rc = ::_execvpe(filename_string.characters(), argv.data(), envp.data());
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
ErrorOr<void> connect(int sockfd, struct sockaddr const* address, socklen_t address_length)
{
    if (::connect(sockfd, address, address_length) < 0)
        return Error::from_syscall("connect"sv, -errno);
    return {};
}
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
ErrorOr<ssize_t> recv(int sockfd, void* buffer, size_t length, int flags)
{
    auto received = ::recv(sockfd, (char*)buffer, (int)length, flags);
    if (received < 0)
        return Error::from_syscall("recv"sv, -errno);
    return received;
}
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

}
