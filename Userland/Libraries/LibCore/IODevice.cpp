/*
 * Copyright (c) 2018-2020, Andreas Kling <kling@serenityos.org>
 * Copyright (c) 2022, the SerenityOS developers.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/ByteBuffer.h>
#include <AK/Platform.h>
#include <LibCore/IODevice.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#if !defined(AK_OS_WINDOWS)
#    include <sys/select.h>
#else
#    include <AK/Windows.h>
#endif
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

namespace Core {

IODevice::IODevice(Object* parent)
    : Object(parent)
{
}

char const* IODevice::error_string() const
{
    return strerror(m_error);
}

int IODevice::read(u8* buffer, int length)
{
    auto read_buffer = read(length);
    memcpy(buffer, read_buffer.data(), length);
    return read_buffer.size();
}

ByteBuffer IODevice::read(size_t max_size)
{
#if !defined(AK_OS_WINDOWS)
    if (m_fd < 0)
#else
    if (m_handle == INVALID_HANDLE_VALUE)
#endif
        return {};
    if (!max_size)
        return {};

    if (m_buffered_data.size() < max_size)
        populate_read_buffer(max(max_size - m_buffered_data.size(), 1024));

    auto size = min(max_size, m_buffered_data.size());
    auto buffer_result = ByteBuffer::create_uninitialized(size);
    if (buffer_result.is_error()) {
        dbgln("IODevice::read: Not enough memory to allocate a buffer of {} bytes", size);
        return {};
    }
    auto buffer = buffer_result.release_value();
    auto* buffer_ptr = (char*)buffer.data();

    memcpy(buffer_ptr, m_buffered_data.data(), size);
    m_buffered_data.remove(0, size);

    return buffer;
}

#if !defined(AK_OS_WINDOWS)
bool IODevice::can_read_from_fd() const
{
    // FIXME: Can we somehow remove this once Core::Socket is implemented using non-blocking sockets?
    fd_set rfds {};
    FD_ZERO(&rfds);
    FD_SET(m_fd, &rfds);
    struct timeval timeout {
        0, 0
    };

    for (;;) {
        if (::select(m_fd + 1, &rfds, nullptr, nullptr, &timeout) < 0) {
            if (errno == EINTR)
                continue;
            perror("IODevice::can_read_from_fd: select");
            return false;
        }
        break;
    }
    return FD_ISSET(m_fd, &rfds);
}
#else
bool IODevice::can_read_from_handle() const
{
    if (m_handle == INVALID_HANDLE_VALUE)
        return false;
    return WaitForSingleObject(m_handle, 0) == WAIT_OBJECT_0;
}
#endif
bool IODevice::can_read_line() const
{
    if (m_eof && !m_buffered_data.is_empty())
        return true;

    if (m_buffered_data.contains_slow('\n'))
        return true;

#if !defined(AK_OS_WINDOWS)
    if (!can_read_from_fd())
        return false;
#else
    if (!can_read_from_handle())
        return false;
#endif

    while (true) {
        // Populate buffer until a newline is found or we reach EOF.

        auto previous_buffer_size = m_buffered_data.size();
        populate_read_buffer();
        auto new_buffer_size = m_buffered_data.size();

        if (m_error)
            return false;

        if (m_eof)
            return !m_buffered_data.is_empty();

        if (m_buffered_data.contains_in_range('\n', previous_buffer_size, new_buffer_size - 1))
            return true;
    }
}

bool IODevice::can_read() const
{
#if !defined(AK_OS_WINDOWS)
    return !m_buffered_data.is_empty() || can_read_from_fd();
#else
    return !m_buffered_data.is_empty() || can_read_from_handle();
#endif
}

bool IODevice::can_read_only_from_buffer() const
{
#if !defined(AK_OS_WINDOWS)
    return !m_buffered_data.is_empty() && !can_read_from_fd();
#else
    return !m_buffered_data.is_empty() && !can_read_from_handle();
#endif
}

ByteBuffer IODevice::read_all()
{
#if !defined(AK_OS_WINDOWS)
    off_t file_size = 0;
    struct stat st;
    int rc = fstat(fd(), &st);
    if (rc == 0)
        file_size = st.st_size;

    Vector<u8> data;
    data.ensure_capacity(file_size);

    if (!m_buffered_data.is_empty()) {
        data.append(m_buffered_data.data(), m_buffered_data.size());
        m_buffered_data.clear();
    }

    while (true) {
        char read_buffer[4096];
        int nread = ::read(m_fd, read_buffer, sizeof(read_buffer));
        if (nread < 0) {
            set_error(errno);
            break;
        }
        if (nread == 0) {
            set_eof(true);
            break;
        }
        data.append((u8 const*)read_buffer, nread);
    }
#else
    Vector<u8> data;

    if (!m_buffered_data.is_empty()) {
        data.append(m_buffered_data.data(), m_buffered_data.size());
        m_buffered_data.clear();
    }

    while (true) {
        char read_buffer[4096];
        DWORD nread = 0;
        if (!ReadFile(m_handle, read_buffer, sizeof(read_buffer), &nread, nullptr)) {
            set_error(GetLastError());
            break;
        }
        if (nread == 0) {
            set_eof(true);
            break;
        }
        data.append((u8 const*)read_buffer, nread);
    }
#endif

    auto result = ByteBuffer::copy(data);
    if (!result.is_error())
        return result.release_value();

    set_error(ENOMEM);
    return {};
}

DeprecatedString IODevice::read_line(size_t max_size)
{
#if !defined(AK_OS_WINDOWS)
    if (m_fd < 0)
        return {};
#else
    if (m_handle == INVALID_HANDLE_VALUE)
        return {};
#endif
    if (!max_size)
        return {};
    if (!can_read_line())
        return {};
    if (m_eof) {
        if (m_buffered_data.size() > max_size) {
            dbgln("IODevice::read_line: At EOF but there's more than max_size({}) buffered", max_size);
            return {};
        }
        auto line = DeprecatedString((char const*)m_buffered_data.data(), m_buffered_data.size(), Chomp);
        m_buffered_data.clear();
        return line;
    }
    auto line_result = ByteBuffer::create_uninitialized(max_size + 1);
    if (line_result.is_error()) {
        dbgln("IODevice::read_line: Not enough memory to allocate a buffer of {} bytes", max_size + 1);
        return {};
    }
    auto line = line_result.release_value();
    size_t line_index = 0;
    while (line_index < max_size) {
        u8 ch = m_buffered_data[line_index];
        line[line_index++] = ch;
        if (ch == '\n') {
            Vector<u8> new_buffered_data;
            new_buffered_data.append(m_buffered_data.data() + line_index, m_buffered_data.size() - line_index);
            m_buffered_data = move(new_buffered_data);
            line.resize(line_index);
            return DeprecatedString::copy(line, Chomp);
        }
    }
    return {};
}

bool IODevice::populate_read_buffer(size_t size) const
{
#if !defined(AK_OS_WINDOWS)
    if (m_fd < 0)
        return false;
    if (!size)
        return false;

    auto buffer_result = ByteBuffer::create_uninitialized(size);
    if (buffer_result.is_error()) {
        dbgln("IODevice::populate_read_buffer: Not enough memory to allocate a buffer of {} bytes", size);
        return {};
    }
    auto buffer = buffer_result.release_value();
    auto* buffer_ptr = (char*)buffer.data();

    int nread = ::read(m_fd, buffer_ptr, size);
    if (nread < 0) {
        set_error(errno);
        return false;
    }
    if (nread == 0) {
        set_eof(true);
        return false;
    }
    m_buffered_data.append(buffer.data(), nread);
    return true;
#else
    if (m_handle == INVALID_HANDLE_VALUE)
        return false;
    if (!size)
        return false;

    auto buffer_result = ByteBuffer::create_uninitialized(size);
    if (buffer_result.is_error()) {
        dbgln("IODevice::populate_read_buffer: Not enough memory to allocate a buffer of {} bytes", size);
        return {};
    }

    auto buffer = buffer_result.release_value();
    auto* buffer_ptr = (char*)buffer.data();

    DWORD nread = 0;
    if (!ReadFile(m_handle, buffer_ptr, size, &nread, nullptr)) {
        set_error(GetLastError());
        return false;
    }

    if (nread == 0) {
        set_eof(true);
        return false;
    }

    m_buffered_data.append(buffer.data(), nread);
    return true;
#endif
}

bool IODevice::close()
{
#if !defined(AK_OS_WINDOWS)
    if (fd() < 0 || m_mode == OpenMode::NotOpen)
        return false;
    int rc = ::close(fd());
    if (rc < 0) {
        set_error(errno);
        return false;
    }
    set_fd(-1);
    set_mode(OpenMode::NotOpen);
#else
    if (m_handle == INVALID_HANDLE_VALUE || m_mode == OpenMode::NotOpen)
        return false;
    int rc = CloseHandle(m_handle);
    if (!rc) {
        set_error(GetLastError());
        return false;
    }
    m_handle = INVALID_HANDLE_VALUE;
    set_mode(OpenMode::NotOpen);
#endif
    return true;
}

bool IODevice::seek(i64 offset, SeekMode mode, off_t* pos)
{
    int m = SEEK_SET;
    switch (mode) {
    case SeekMode::SetPosition:
        m = SEEK_SET;
        break;
    case SeekMode::FromCurrentPosition:
        m = SEEK_CUR;
        offset -= m_buffered_data.size();
        break;
    case SeekMode::FromEndPosition:
        m = SEEK_END;
        break;
    }
#if !defined(AK_OS_WINDOWS)
    off_t rc = lseek(m_fd, offset, m);
    if (rc < 0) {
        set_error(errno);
        if (pos)
            *pos = -1;
        return false;
    }
#else
    LARGE_INTEGER li;
    li.QuadPart = offset;
    li.LowPart = SetFilePointer(m_handle, li.LowPart, &li.HighPart, m);
    if (li.LowPart == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR) {
        set_error(GetLastError());
        if (pos)
            *pos = -1;
        return false;
    }
    off_t rc = li.QuadPart;
#endif
    m_buffered_data.clear();
    m_eof = false;
    if (pos)
        *pos = rc;
    return true;
}

bool IODevice::truncate(off_t size)
{
#if defined(AK_OS_WINDOWS)
    if (!SetFilePointer(m_handle, size, nullptr, FILE_BEGIN)) {
        set_error(GetLastError());
        return false;
    }
#else
    int rc = ftruncate(m_fd, size);
    if (rc < 0) {
        set_error(errno);
        return false;
    }
#endif
    return true;
}

bool IODevice::write(u8 const* data, int size)
{
#if !defined(AK_OS_WINDOWS)
    dbgln("IODevice::write: fd={}, size={}", m_fd, size);
    int rc = ::write(m_fd, data, size);
    if (rc < 0) {
        set_error(errno);
        perror("IODevice::write: write");
        return false;
    }
#else
    dbgln("IODevice::write: handle={}, size={}", m_handle, size);
    int rc = WriteFile(m_handle, data, size, nullptr, nullptr);
    if (!rc) {
        set_error(GetLastError());
        perror("IODevice::write: WriteFile");
        return false;
    }
#endif
    return rc == size;
}

#if !defined(AK_OS_WINDOWS)
void IODevice::set_fd(SOCKET fd)
{
    if (m_fd == fd)
        return;

    m_fd = fd;
    did_update_fd(fd);
}
#else
void IODevice::set_handle(HANDLE handle)
{
    if (m_handle == handle)
        return;

    m_handle = handle;
    did_update_handle(handle);
}
#endif

bool IODevice::write(StringView v)
{
    return write((u8 const*)v.characters_without_null_termination(), v.length());
}

LineIterator::LineIterator(IODevice& device, bool is_end)
    : m_device(device)
    , m_is_end(is_end)
{
    if (!m_is_end) {
        ++*this;
    }
}

bool LineIterator::at_end() const
{
    return m_device->eof();
}

LineIterator& LineIterator::operator++()
{
    m_buffer = m_device->read_line();
    return *this;
}

LineIterator LineRange::begin() { return m_device.line_begin(); }
LineIterator LineRange::end() { return m_device.line_end(); }
}
