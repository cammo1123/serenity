/*
 * Copyright (c) 2018-2021, Andreas Kling <kling@serenityos.org>
 * Copyright (c) 2021, sin-ack <sin-ack@protonmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "AK/Format.h"
#include <LibCore/Socket.h>
#include <LibCore/System.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <winsock2.h>
#ifdef AK_OS_SERENITY
#    include <serenity.h>
#endif
#ifdef AK_OS_FREEBSD
#    include <sys/ucred.h>
#endif
#ifdef AK_OS_WINDOWS
#    include <AK/Windows.h>
#endif

namespace Core {

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

ErrorOr<Bytes> PosixSocketHelper::read(Bytes buffer, int flags)
{
    if (!is_open()) {
        return Error::from_errno(ENOTCONN);
    }

    ssize_t nread = TRY(System::recv(m_fd, buffer.data(), buffer.size(), flags));
    m_last_read_was_eof = nread == 0;

    // If a socket read is EOF, then no more data can be read from it because
    // the protocol has disconnected. In this case, we can just disable the
    // notifier if we have one.
    if (m_last_read_was_eof && m_notifier)
        m_notifier->set_enabled(false);

    return buffer.trim(nread);
}

ErrorOr<size_t> PosixSocketHelper::write(ReadonlyBytes buffer, int flags)
{
    if (!is_open()) {
        return Error::from_errno(ENOTCONN);
    }

    return TRY(System::send(m_fd, buffer.data(), buffer.size(), flags));
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
    if (!m_notifier)
        m_notifier = Core::Notifier::construct((HANDLE)_get_osfhandle(m_fd), Core::Notifier::Read);
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

ErrorOr<NonnullOwnPtr<LocalSocket>> LocalSocket::adopt_fd(int fd, PreventSIGPIPE prevent_sigpipe)
{
    if (fd < 0) {
        return Error::from_errno(EBADF);
    }

    auto socket = TRY(adopt_nonnull_own_or_enomem(new (nothrow) LocalSocket(prevent_sigpipe)));
    socket->m_helper.set_fd(fd);
    socket->setup_notifier();
    return socket;
}

ErrorOr<int> LocalSocket::receive_fd(int flags)
{
    dbgln("LocalSocket::receive_fd(flags: {}) not implemented", flags);
    VERIFY_NOT_REACHED();
}
ErrorOr<void> LocalSocket::send_fd(int fd)
{
    dbgln("LocalSocket::send_fd(fd: {}) not implemented", fd);
    return{};
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
    if (!is_open())
        return {};
    return m_helper.fd();
}

ErrorOr<int> LocalSocket::release_fd()
{
    if (!is_open()) {
        return Error::from_errno(ENOTCONN);
    }

    auto fd = m_helper.fd();
    m_helper.set_fd(-1);
    return fd;
}
}
