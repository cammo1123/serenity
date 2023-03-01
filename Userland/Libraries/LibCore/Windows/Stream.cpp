/*
 * Copyright (c) 2018-2021, Andreas Kling <kling@serenityos.org>
 * Copyright (c) 2021, sin-ack <sin-ack@protonmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibCore/Stream.h>
#include <LibCore/System.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#ifdef AK_OS_SERENITY
#    include <serenity.h>
#endif
#ifdef AK_OS_FREEBSD
#    include <sys/ucred.h>
#endif

namespace Core::Stream {

ErrorOr<NonnullOwnPtr<File>> File::open(StringView filename, OpenMode mode, mode_t permissions)
{
    auto file = TRY(adopt_nonnull_own_or_enomem(new (nothrow) File(mode)));
    TRY(file->open_path(filename, permissions));
    return file;
}

ErrorOr<NonnullOwnPtr<File>> File::adopt_fd(int, OpenMode, ShouldCloseFileDescriptor)
{
    dbgln("File: adopt_fd not implemented");
    VERIFY_NOT_REACHED();
}

ErrorOr<NonnullOwnPtr<File>> File::standard_input()
{
    return File::adopt_fd(STDIN_FILENO, OpenMode::Read, ShouldCloseFileDescriptor::No);
}
ErrorOr<NonnullOwnPtr<File>> File::standard_output()
{
    return File::adopt_fd(STDOUT_FILENO, OpenMode::Write, ShouldCloseFileDescriptor::No);
}
ErrorOr<NonnullOwnPtr<File>> File::standard_error()
{
    return File::adopt_fd(STDERR_FILENO, OpenMode::Write, ShouldCloseFileDescriptor::No);
}

ErrorOr<NonnullOwnPtr<File>> File::open_file_or_standard_stream(StringView filename, OpenMode mode)
{
    if (!filename.is_empty() && filename != "-"sv)
        return File::open(filename, mode);

    switch (mode) {
    case OpenMode::Read:
        return standard_input();
    case OpenMode::Write:
        return standard_output();
    default:
        VERIFY_NOT_REACHED();
    }
}

int File::open_mode_to_options(OpenMode mode)
{
    int flags = 0;
    if (has_flag(mode, OpenMode::ReadWrite)) {
        flags |= O_RDWR | O_CREAT;
    } else if (has_flag(mode, OpenMode::Read)) {
        flags |= O_RDONLY;
    } else if (has_flag(mode, OpenMode::Write)) {
        flags |= O_WRONLY | O_CREAT;
        bool should_truncate = !has_any_flag(mode, OpenMode::Append | OpenMode::MustBeNew);
        if (should_truncate)
            flags |= O_TRUNC;
    }

    if (has_flag(mode, OpenMode::Append))
        flags |= O_APPEND;
    if (has_flag(mode, OpenMode::Truncate))
        flags |= O_TRUNC;
    if (has_flag(mode, OpenMode::MustBeNew))
        flags |= O_EXCL;
    return flags;
}

ErrorOr<void> File::open_path(StringView filename, mode_t permissions)
{
    VERIFY(m_fd == -1);
    auto flags = open_mode_to_options(m_mode);

    m_fd = TRY(System::open(filename, flags, permissions));
    return {};
}

ErrorOr<Bytes> File::read(Bytes buffer)
{
    if (!has_flag(m_mode, OpenMode::Read)) {
        // NOTE: POSIX says that if the fd is not open for reading, the call
        //       will return EBADF. Since we already know whether we can or
        //       can't read the file, let's avoid a syscall.
        return Error::from_errno(EBADF);
    }

    ssize_t nread = TRY(System::read(m_fd, buffer));
    m_last_read_was_eof = nread == 0;
    return buffer.trim(nread);
}

ErrorOr<ByteBuffer> File::read_until_eof(size_t block_size)
{
    // Note: This is used as a heuristic, it's not valid for devices or virtual files.
    auto const potential_file_size = TRY(System::fstat(m_fd)).st_size;

    return read_until_eof_impl(block_size, potential_file_size);
}

ErrorOr<size_t> File::write(ReadonlyBytes buffer)
{

    if (!has_flag(m_mode, OpenMode::Write)) {
        // NOTE: Same deal as Read.
        return Error::from_errno(EBADF);
    }

    return TRY(System::write(m_fd, buffer));
}

bool File::is_eof() const { return m_last_read_was_eof; }
bool File::is_open() const { return m_fd >= 0; }

void File::close()
{
    if (!is_open()) {
        return;
    }

    // NOTE: The closing of the file can be interrupted by a signal, in which
    // case EINTR will be returned by the close syscall. So let's try closing
    // the file until we aren't interrupted by rude signals. :^)
    ErrorOr<void> result;
    do {
        result = System::close(m_fd);
    } while (result.is_error() && result.error().code() == EINTR);

    VERIFY(!result.is_error());
    m_fd = -1;
}

ErrorOr<size_t> File::seek(i64 offset, SeekMode mode)
{
    int syscall_mode;
    switch (mode) {
    case SeekMode::SetPosition:
        syscall_mode = SEEK_SET;
        break;
    case SeekMode::FromCurrentPosition:
        syscall_mode = SEEK_CUR;
        break;
    case SeekMode::FromEndPosition:
        syscall_mode = SEEK_END;
        break;
    default:
        VERIFY_NOT_REACHED();
    }

    size_t seek_result = TRY(System::lseek(m_fd, offset, syscall_mode));
    m_last_read_was_eof = false;
    return seek_result;
}

ErrorOr<void> File::truncate(size_t)
{
    dbgln("File: truncate not implemented");
    VERIFY_NOT_REACHED();
}

ErrorOr<int> Socket::create_fd(SocketDomain domain, SocketType type)
{
    int socket_domain;
    switch (domain) {
    case SocketDomain::Local:
        socket_domain = AF_UNIX;
        break;
    case SocketDomain::Inet:
        socket_domain = AF_INET;
        break;
    default:
        VERIFY_NOT_REACHED();
    }

    int socket_type;
    switch (type) {
    case SocketType::Stream:
        socket_type = SOCK_STREAM;
        break;
    case SocketType::Datagram:
        socket_type = SOCK_DGRAM;
        break;
    default:
        VERIFY_NOT_REACHED();
    }

    // Let's have a safe default of CLOEXEC. :^)
#ifdef SOCK_CLOEXEC
    return System::socket(socket_domain, socket_type | SOCK_CLOEXEC, 0);
#else
    auto fd = TRY(System::socket(socket_domain, socket_type, 0));
    return fd;
#endif
}

ErrorOr<IPv4Address> Socket::resolve_host(DeprecatedString const&, SocketType)
{
    dbgln("Socket: resolve_host not implemented");
    VERIFY_NOT_REACHED();
}

ErrorOr<void> Socket::connect_local(int fd, DeprecatedString const& path)
{
    auto address = SocketAddress::local(path);
    auto maybe_sockaddr = address.to_sockaddr_un();
    if (!maybe_sockaddr.has_value()) {
        dbgln("Core::Stream::Socket::connect_local: Could not obtain a sockaddr_un");
        return Error::from_errno(EINVAL);
    }

    auto addr = maybe_sockaddr.release_value();
    return System::connect(fd, bit_cast<struct sockaddr*>(&addr), sizeof(addr));
}

ErrorOr<void> Socket::connect_inet(int, SocketAddress const&)
{
    dbgln("Socket: connect_inet not implemented");
    VERIFY_NOT_REACHED();
}

ErrorOr<Bytes> PosixSocketHelper::read(Bytes, int)
{
    dbgln("PosixSocketHelper: read not implemented");
    VERIFY_NOT_REACHED();
}

ErrorOr<size_t> PosixSocketHelper::write(ReadonlyBytes, int)
{
    dbgln("PosixSocketHelper: write not implemented");
    VERIFY_NOT_REACHED();
}

void PosixSocketHelper::close()
{
    if (!is_open()) {
        return;
    }

    if (m_notifier)
        m_notifier->set_enabled(false);

    ErrorOr<void> result;
    do {
        result = System::close(m_fd);
    } while (result.is_error() && result.error().code() == EINTR);

    VERIFY(!result.is_error());
    m_fd = -1;
}

ErrorOr<bool> PosixSocketHelper::can_read_without_blocking(int) const
{
    dbgln("PosixSocketHelper: can_read_without_blocking not implemented");
    VERIFY_NOT_REACHED();
}

ErrorOr<void> PosixSocketHelper::set_blocking(bool)
{
    dbgln("PosixSocketHelper: set_blocking not implemented");
    VERIFY_NOT_REACHED();
}

ErrorOr<void> PosixSocketHelper::set_close_on_exec(bool)
{
    dbgln("PosixSocketHelper: set_close_on_exec not implemented");
    VERIFY_NOT_REACHED();
}

ErrorOr<void> PosixSocketHelper::set_receive_timeout(Time)
{
    dbgln("PosixSocketHelper: set_receive_timeout not implemented");
    VERIFY_NOT_REACHED();
}

void PosixSocketHelper::setup_notifier()
{
    dbgln("PosixSocketHelper: setup_notifier not implemented");
    VERIFY_NOT_REACHED();
}

ErrorOr<NonnullOwnPtr<TCPSocket>> TCPSocket::connect(DeprecatedString const&, u16)
{
    dbgln("TCPSocket: connect not implemented");
    VERIFY_NOT_REACHED();
}

ErrorOr<NonnullOwnPtr<TCPSocket>> TCPSocket::connect(SocketAddress const&)
{
    dbgln("TCPSocket: connect not implemented");
    VERIFY_NOT_REACHED();
}

ErrorOr<NonnullOwnPtr<TCPSocket>> TCPSocket::adopt_fd(int)
{
    dbgln("TCPSocket: adopt_fd not implemented");
    VERIFY_NOT_REACHED();
}

ErrorOr<size_t> PosixSocketHelper::pending_bytes() const
{
    dbgln("PosixSocketHelper: pending_bytes not implemented");
    VERIFY_NOT_REACHED();
}

ErrorOr<NonnullOwnPtr<UDPSocket>> UDPSocket::connect(DeprecatedString const&, u16, Optional<Time>)
{
    dbgln("UDPSocket: connect not implemented");
    VERIFY_NOT_REACHED();
}

ErrorOr<NonnullOwnPtr<UDPSocket>> UDPSocket::connect(SocketAddress const&, Optional<Time>)
{
    dbgln("UDPSocket: connect not implemented");
    VERIFY_NOT_REACHED();
}

ErrorOr<NonnullOwnPtr<LocalSocket>> LocalSocket::connect(DeprecatedString const& path, PreventSIGPIPE prevent_sigpipe)
{
    auto socket = TRY(adopt_nonnull_own_or_enomem(new (nothrow) LocalSocket(prevent_sigpipe)));

    auto fd = TRY(create_fd(SocketDomain::Local, SocketType::Stream));
    socket->m_helper.set_fd(fd);

    TRY(connect_local(fd, path));

    socket->setup_notifier();
    return socket;
}

ErrorOr<NonnullOwnPtr<LocalSocket>> LocalSocket::adopt_fd(int, PreventSIGPIPE)
{
    dbgln("LocalSocket: adopt_fd not implemented");
    VERIFY_NOT_REACHED();
}

ErrorOr<int> LocalSocket::receive_fd(int)
{
    dbgln("LocalSocket: receive_fd not implemented");
    VERIFY_NOT_REACHED();
}

ErrorOr<void> LocalSocket::send_fd(int)
{
    dbgln("LocalSocket: send_fd not implemented");
    VERIFY_NOT_REACHED();
}

ErrorOr<pid_t> LocalSocket::peer_pid() const
{
    dbgln("LocalSocket: peer_pid not implemented");
    VERIFY_NOT_REACHED();
}

ErrorOr<Bytes> LocalSocket::read_without_waiting(Bytes)
{
    dbgln("LocalSocket: read_without_waiting not implemented");
    VERIFY_NOT_REACHED();
}

Optional<int> LocalSocket::fd() const
{
    dbgln("LocalSocket: fd not implemented");
    VERIFY_NOT_REACHED();
}

ErrorOr<int> LocalSocket::release_fd()
{
    dbgln("LocalSocket: release_fd not implemented");
    VERIFY_NOT_REACHED();
}
}
