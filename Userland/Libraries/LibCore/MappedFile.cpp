/*
 * Copyright (c) 2018-2021, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/DeprecatedString.h>
#include <AK/ScopeGuard.h>
#include <LibCore/File.h>
#include <LibCore/MappedFile.h>
#include <LibCore/System.h>
#include <fcntl.h>
#if !defined(AK_OS_WINDOWS)
#    include <sys/mman.h>
#endif
#include <unistd.h>

namespace Core {

ErrorOr<NonnullRefPtr<MappedFile>> MappedFile::map(StringView path)
{
#if !defined(AK_OS_WINDOWS)
    auto fd = TRY(Core::System::open(path, O_RDONLY | O_CLOEXEC, 0));
    return map_from_fd_and_close(fd, path);
#else
    auto handle = CreateFile(path.to_deprecated_string().characters(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (handle == INVALID_HANDLE_VALUE) {
        dbgln("CreateFile failed: {}", GetLastError());
        return Error::from_string_literal("OpenFile failed");
    }

    return map_from_handle_and_close(handle, path);
#endif
}

ErrorOr<NonnullRefPtr<MappedFile>> MappedFile::map_from_file(NonnullOwnPtr<Core::File> stream, StringView path)
{
#if !defined(AK_OS_WINDOWS)
    return map_from_fd_and_close(stream->leak_fd(Badge<MappedFile> {}), path);
#else
    return map_from_handle_and_close((HANDLE)_get_osfhandle(stream->leak_fd(Badge<MappedFile> {})), path);
#endif
}

#if defined(AK_OS_WINDOWS)
ErrorOr<NonnullRefPtr<MappedFile>> MappedFile::map_from_handle_and_close(HANDLE handle, [[maybe_unused]] StringView path)
{
    auto size = GetFileSize(handle, nullptr);
    if (size == INVALID_FILE_SIZE) {
        dbgln("GetFileSize failed {}", GetLastError());
        return Error::from_string_literal("GetFileSize failed");
    }

    auto* ptr = CreateFileMapping(handle, nullptr, PAGE_READONLY, 0, 0, nullptr);
    if (!ptr) {
        dbgln("CreateFileMapping failed {}", GetLastError());
        return Error::from_string_literal("CreateFileMapping failed");
    }

    auto* data = MapViewOfFile(ptr, FILE_MAP_READ, 0, 0, 0);
    if (!data) {
        dbgln("MapViewOfFile failed {}", GetLastError());
        return Error::from_string_literal("MapViewOfFile failed");
    }

    CloseHandle(handle);

    return adopt_ref(*new MappedFile(data, size));
}
#else
ErrorOr<NonnullRefPtr<MappedFile>> MappedFile::map_from_fd_and_close(int fd, [[maybe_unused]] StringView path)
{

    TRY(Core::System::fcntl(fd, F_SETFD, FD_CLOEXEC));

    ScopeGuard fd_close_guard = [fd] {
        close(fd);
    };

    auto stat = TRY(Core::System::fstat(fd));
    auto size = stat.st_size;

    auto* ptr = TRY(Core::System::mmap(nullptr, size, PROT_READ, MAP_SHARED, fd, 0, 0, path));

    return adopt_ref(*new MappedFile(ptr, size));
}
#endif

MappedFile::MappedFile(void* ptr, size_t size)
    : m_data(ptr)
    , m_size(size)
{
}

MappedFile::~MappedFile()
{
#if !defined(AK_OS_WINDOWS)
    auto res = Core::System::munmap(m_data, m_size);
    if (res.is_error())
        dbgln("Failed to unmap MappedFile (@ {:p}): {}", m_data, res.error());
#else
    UnmapViewOfFile(m_data);
#endif
}

}
