/*
 * Copyright (c) 2018-2020, Andreas Kling <kling@serenityos.org>
 * Copyright (c) 2023, Sam Atkins <atkinssj@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/Vector.h>
#include <LibCore/DirIterator.h>
#include <errno.h>

#if defined(AK_OS_WINDOWS)
#    include <io.h>
#endif

namespace Core {

DirIterator::DirIterator(DeprecatedString path, Flags flags)
    : m_path(move(path))
    , m_flags(flags)
{
#if !defined(AK_OS_WINDOWS)
    m_dir = opendir(m_path.characters());
    if (!m_dir) {
        m_error = Error::from_errno(errno);
    }
#endif
}

DirIterator::~DirIterator()
{
#if !defined(AK_OS_WINDOWS)
    if (m_dir) {
        closedir(m_dir);
        m_dir = nullptr;
    }
#else
    if (m_dir != INVALID_HANDLE_VALUE) {
        FindClose(m_dir);
        m_dir = INVALID_HANDLE_VALUE;
    }
#endif
}

DirIterator::DirIterator(DirIterator&& other)
    : m_dir(other.m_dir)
    , m_error(move(other.m_error))
    , m_next(move(other.m_next))
    , m_path(move(other.m_path))
    , m_flags(other.m_flags)
{
    other.m_dir = nullptr;
}

bool DirIterator::advance_next()
{

    while (true) {
#if defined(AK_OS_WINDOWS)
        if (!initialized) {
            initialized = true;
            auto path = DeprecatedString::formatted("{}/*", m_path);
            m_dir = FindFirstFile(path.characters(), &m_find_data);
            if (m_dir == INVALID_HANDLE_VALUE) {
                m_error = Error::from_windows_error(GetLastError());
                return false;
            }
        } else {
            if (!FindNextFile(m_dir, &m_find_data)) {
                m_error = Error::from_windows_error(GetLastError());
                return false;
            }
        }

        m_next = DirectoryEntry::from_find_data(m_find_data);
#else
        errno = 0;
        auto* de = readdir(m_dir);
        if (!de) {
            m_error = Error::from_errno(errno);
            m_next.clear();
            return false;
        }

        m_next = DirectoryEntry::from_dirent(*de);

#endif
        if (m_next->name.is_empty())
            return false;

        if (m_flags & Flags::SkipDots && m_next->name.starts_with('.'))
            continue;

        if (m_flags & Flags::SkipParentAndBaseDir && (m_next->name == "." || m_next->name == ".."))
            continue;

        return !m_next->name.is_empty();
    }
}

bool DirIterator::has_next()
{
    if (m_next.has_value())
        return true;

    return advance_next();
}

Optional<DirectoryEntry> DirIterator::next()
{
    if (!m_next.has_value())
        advance_next();

    auto result = m_next;
    m_next.clear();
    return result;
}

DeprecatedString DirIterator::next_path()
{
    auto entry = next();
    if (entry.has_value())
        return entry->name;
    return "";
}

DeprecatedString DirIterator::next_full_path()
{
    StringBuilder builder;
    builder.append(m_path);
    if (!m_path.ends_with('/'))
        builder.append('/');
    builder.append(next_path());
    return builder.to_deprecated_string();
}

int DirIterator::fd() const
{
    if (!m_dir)
        return -1;
#if defined(AK_OS_WINDOWS)
    auto handle = CreateFile(m_path.characters(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
    if (handle == INVALID_HANDLE_VALUE)
        return -1;
    return _open_osfhandle((intptr_t)handle, 0);
#else
    return dirfd(m_dir);
#endif
}

}
