/*
 * Copyright (c) 2022, kleines Filmröllchen <filmroellchen@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Directory.h"
#include "DirIterator.h"
#if !defined(AK_OS_WINDOWS)
#    include "System.h"
#else
#    include <winbase.h>
#    include <windows.h>
#    include <winternl.h>
#endif
#include <dirent.h>

namespace Core {

// We assume that the fd is a valid directory.
#if !defined(AK_OS_WINDOWS)
Directory::Directory(int fd, Optional<LexicalPath> path)
    : m_path(move(path))
    , m_directory_fd(fd)
{
}

Directory::Directory(Directory&& other)
    : m_path(move(other.m_path))
    , m_directory_fd(other.m_directory_fd)
{
    other.m_directory_fd = -1;
}
#else
Directory::Directory(HANDLE handle, Optional<LexicalPath> path)
    : m_path(move(path))
    , m_directory_handle(handle)
{
}

Directory::Directory(Directory&& other)
    : m_path(move(other.m_path))
    , m_directory_handle(other.m_directory_handle)
{
    other.m_directory_handle = INVALID_HANDLE_VALUE;
}
#endif

Directory::~Directory()
{
#if !defined(AK_OS_WINDOWS)
    if (m_directory_fd != -1)
        MUST(System::close(m_directory_fd));
#else
    if (m_directory_handle != INVALID_HANDLE_VALUE)
        CloseHandle(m_directory_handle);
#endif
}

ErrorOr<void> Directory::chown(uid_t uid, gid_t gid)
{
#if !defined(AK_OS_WINDOWS)
    if (m_directory_fd == -1)
        return Error::from_syscall("fchown"sv, -EBADF);
    TRY(Core::System::fchown(m_directory_fd, uid, gid));
    return {};
#else
    (void)uid;
    (void)gid;
    dbgln("Directory::chown() not implemented on Windows");
    VERIFY_NOT_REACHED();
#endif
}

#if !defined(AK_OS_WINDOWS)
ErrorOr<bool> Directory::is_valid_directory(int fd)
{
    auto stat = TRY(System::fstat(fd));
    return stat.st_mode & S_IFDIR;
}

ErrorOr<Directory> Directory::adopt_fd(int fd, Optional<LexicalPath> path)
{
    // This will also fail if the fd is invalid in the first place.
    if (!TRY(Directory::is_valid_directory(fd)))
        return Error::from_errno(ENOTDIR);
    return Directory { fd, move(path) };
}
#else
ErrorOr<bool> Directory::is_valid_directory(HANDLE handle)
{
    // Stat the file to see if it's a directory.
    FILE_BASIC_INFO basic_info;
    if (GetFileInformationByHandleEx(handle, FileBasicInfo, &basic_info, sizeof(basic_info)) == 0)
        return Error::from_errno(ENOTDIR);
    return basic_info.FileAttributes & FILE_ATTRIBUTE_DIRECTORY;
}

ErrorOr<Directory> Directory::adopt_handle(HANDLE handle, Optional<LexicalPath> path)
{
    // This will also fail if the fd is invalid in the first place.
    if (!TRY(Directory::is_valid_directory(handle)))
        return Error::from_errno(ENOTDIR);
    return Directory { handle, move(path) };
}
#endif

ErrorOr<Directory> Directory::create(DeprecatedString path, CreateDirectories create_directories, mode_t creation_mode)
{
    return create(LexicalPath { move(path) }, create_directories, creation_mode);
}

ErrorOr<Directory> Directory::create(LexicalPath path, CreateDirectories create_directories, mode_t creation_mode)
{
    if (create_directories == CreateDirectories::Yes)
        TRY(ensure_directory(path, creation_mode));
        // FIXME: doesn't work on Linux probably
#if !defined(AK_OS_WINDOWS)
    auto fd = TRY(System::open(path.string(), O_CLOEXEC));
    return adopt_fd(fd, move(path));
#else
    auto* handle = CreateFile(path.string().characters(), FILE_LIST_DIRECTORY | SYNCHRONIZE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, nullptr);
    return adopt_handle(handle, move(path));
#endif
}

ErrorOr<void> Directory::ensure_directory(LexicalPath const& path, mode_t creation_mode)
{
    if (path.basename() == "/" || path.basename() == ".")
        return {};

    TRY(ensure_directory(path.parent(), creation_mode));

#if !defined(AK_OS_WINDOWS)
    auto return_value = System::mkdir(path.string(), creation_mode);
    // We don't care if the directory already exists.
    if (return_value.is_error() && return_value.error().code() != EEXIST)
        return return_value;
#else
    // auto dir_exists = [](LexicalPath const& path) {
    //     DWORD file_attributes = GetFileAttributesA(path.string().characters());
    //     if (file_attributes == INVALID_FILE_ATTRIBUTES)
    //         return false;

    //     if ((file_attributes & FILE_ATTRIBUTE_DIRECTORY) != 0u)
    //         return true; 

    //     return false;
    // };

    auto return_value = mkdir(path.string().characters());
    if (return_value != 0 && errno != EEXIST) {
        dbgln("Directory::ensure_directory() mkdir() failed");
        return Error::from_errno(EROFS);
    }
#endif

    return {};
}

ErrorOr<LexicalPath> Directory::path() const
{
    if (!m_path.has_value())
        return Error::from_string_literal("Directory wasn't created with a path");
    return m_path.value();
}

ErrorOr<NonnullOwnPtr<Stream::File>> Directory::open(StringView filename, Stream::OpenMode mode) const
{
#if !defined(AK_OS_WINDOWS)
    auto fd = TRY(System::openat(m_directory_fd, filename, Stream::File::open_mode_to_options(mode)));
    return Stream::File::adopt_fd(fd, mode);
#else
    (void)mode;
    dbgln("Directory::open({}) not implemented on Windows", filename);
    VERIFY_NOT_REACHED();
#endif
}

ErrorOr<struct stat> Directory::stat() const
{
#if !defined(AK_OS_WINDOWS)
    return System::fstat(m_directory_fd);
#else
    dbgln("Directory::stat() not implemented on Windows");
    VERIFY_NOT_REACHED();
#endif
}

ErrorOr<DirIterator> Directory::create_iterator() const
{
    return DirIterator { TRY(path()).string() };
}
}
