/*
 * Copyright (c) 2018-2021, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/DeprecatedString.h>
#include <AK/ScopeGuard.h>
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
    (void)path;
    dbgln("MappedFile::map not implemented");
    VERIFY_NOT_REACHED();
#endif
}

ErrorOr<NonnullRefPtr<MappedFile>> MappedFile::map_from_fd_and_close(int fd, [[maybe_unused]] StringView path)
{
#if !defined(AK_OS_WINDOWS)

    TRY(Core::System::fcntl(fd, F_SETFD, FD_CLOEXEC));

    ScopeGuard fd_close_guard = [fd] {
        close(fd);
    };

    auto stat = TRY(Core::System::fstat(fd));
    auto size = stat.st_size;

    auto* ptr = TRY(Core::System::mmap(nullptr, size, PROT_READ, MAP_SHARED, fd, 0, 0, path));

    return adopt_ref(*new MappedFile(ptr, size));
#else
    (void)fd;
    (void)path;
    dbgln("MappedFile::map_from_fd_and_close not implemented");
    VERIFY_NOT_REACHED();
#endif
}

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
    dbgln("MappedFile::~MappedFile not implemented");
    VERIFY_NOT_REACHED();
#endif
}

}
