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

ErrorOr<NonnullOwnPtr<File>> File::open(StringView, OpenMode, mode_t)
{
	dbgln("File: open not implemented");
	VERIFY_NOT_REACHED();
}
ErrorOr<NonnullOwnPtr<File>> File::adopt_fd(int, OpenMode, ShouldCloseFileDescriptor)
{
	dbgln("File: adopt_fd not implemented");
	VERIFY_NOT_REACHED();
}

ErrorOr<NonnullOwnPtr<File>> File::standard_input()
{
	dbgln("File: standard_input not implemented");
	VERIFY_NOT_REACHED();
}
ErrorOr<NonnullOwnPtr<File>> File::standard_output()
{
	dbgln("File: standard_output not implemented");
	VERIFY_NOT_REACHED();
}
ErrorOr<NonnullOwnPtr<File>> File::standard_error()
{
	dbgln("File: standard_error not implemented");
	VERIFY_NOT_REACHED();
}

ErrorOr<NonnullOwnPtr<File>> File::open_file_or_standard_stream(StringView, OpenMode)
{
	dbgln("File: open_file_or_standard_stream not implemented");
	VERIFY_NOT_REACHED();
}

int File::open_mode_to_options(OpenMode)
{
	dbgln("File: open_mode_to_options not implemented");
	VERIFY_NOT_REACHED();
}

ErrorOr<void> File::open_path(StringView, mode_t)
{
	dbgln("File: open_path not implemented");
	VERIFY_NOT_REACHED();
}

ErrorOr<Bytes> File::read(Bytes)
{
	dbgln("File: open_path not implemented");
	VERIFY_NOT_REACHED();
}

ErrorOr<ByteBuffer> File::read_until_eof(size_t)
{
	dbgln("File: read_until_eof not implemented");
	VERIFY_NOT_REACHED();
}

ErrorOr<size_t> File::write(ReadonlyBytes)
{
	dbgln("File: write not implemented");
	VERIFY_NOT_REACHED();
}

bool File::is_eof() const
{
	dbgln("File: is_eof not implemented");
	VERIFY_NOT_REACHED();
}
bool File::is_open() const
{
	dbgln("File: is_open not implemented");
	VERIFY_NOT_REACHED();
}

void File::close()
{
	dbgln("File: close not implemented");
	VERIFY_NOT_REACHED();
}

ErrorOr<size_t> File::seek(i64, SeekMode)
{
	dbgln("File: seek not implemented");
	VERIFY_NOT_REACHED();
}

ErrorOr<void> File::truncate(size_t)
{
	dbgln("File: truncate not implemented");
	VERIFY_NOT_REACHED();
}

ErrorOr<int> Socket::create_fd(SocketDomain, SocketType)
{
	dbgln("Socket: create_fd not implemented");
	VERIFY_NOT_REACHED();
}

ErrorOr<IPv4Address> Socket::resolve_host(DeprecatedString const&, SocketType)
{
	dbgln("Socket: resolve_host not implemented");
	VERIFY_NOT_REACHED();
}

ErrorOr<void> Socket::connect_local(int, DeprecatedString const&)
{
	dbgln("Socket: connect_local not implemented");
	VERIFY_NOT_REACHED();
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
	dbgln("PosixSocketHelper: close not implemented");
	VERIFY_NOT_REACHED();
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

ErrorOr<NonnullOwnPtr<LocalSocket>> LocalSocket::connect(DeprecatedString const&, PreventSIGPIPE)
{
	dbgln("LocalSocket: connect not implemented");
	VERIFY_NOT_REACHED();
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
