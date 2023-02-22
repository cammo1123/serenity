/*
 * Copyright (c) 2018-2022, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/LexicalPath.h>
#include <AK/Platform.h>
#include <AK/ScopeGuard.h>
#include <LibCore/DirIterator.h>
#include <LibCore/File.h>
#include <LibCore/System.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utime.h>

#ifdef AK_OS_SERENITY
#    include <serenity.h>
#endif

// On Linux distros that use glibc `basename` is defined as a macro that expands to `__xpg_basename`, so we undefine it
#if defined(AK_OS_LINUX) && defined(basename)
#    undef basename
#endif

namespace Core {

ErrorOr<NonnullRefPtr<File>> File::open(DeprecatedString filename, OpenMode mode, mode_t permissions)
{
    auto file = File::construct(move(filename));
    if (!file->open_impl(mode, permissions))
        return Error::from_errno(file->error());
    return file;
}

File::File(DeprecatedString filename, Object* parent)
    : IODevice(parent)
    , m_filename(move(filename))
{
}

File::~File()
{
    if (m_should_close_file_descriptor == ShouldCloseFileDescriptor::Yes && mode() != OpenMode::NotOpen)
        close();
}

bool File::open(int fd, OpenMode mode, ShouldCloseFileDescriptor should_close)
{
    set_fd(fd);
    set_mode(mode);
    m_should_close_file_descriptor = should_close;
    return true;
}

bool File::open(OpenMode mode)
{
    return open_impl(mode, 0666);
}

bool File::open_impl(OpenMode mode, mode_t permissions)
{
    VERIFY(!m_filename.is_null());
    int flags = 0;
    if (has_flag(mode, OpenMode::ReadOnly) && has_flag(mode, OpenMode::WriteOnly)) {
        flags |= O_RDWR | O_CREAT;
    } else if (has_flag(mode, OpenMode::ReadOnly)) {
        flags |= O_RDONLY;
    } else if (has_flag(mode, OpenMode::WriteOnly)) {
        flags |= O_WRONLY | O_CREAT;
        bool should_truncate = !(has_flag(mode, OpenMode::Append) || has_flag(mode, OpenMode::MustBeNew));
        if (should_truncate)
            flags |= O_TRUNC;
    }
    if (has_flag(mode, OpenMode::Append))
        flags |= O_APPEND;
    if (has_flag(mode, OpenMode::Truncate))
        flags |= O_TRUNC;
    if (has_flag(mode, OpenMode::MustBeNew))
        flags |= O_EXCL;
    int fd = ::open(m_filename.characters(), flags, permissions);
    if (fd < 0) {
        set_error(errno);
        return false;
    }

    set_fd(fd);
    set_mode(mode);
    return true;
}

bool File::is_directory() const
{
    return is_directory(fd());
}
bool File::is_directory(DeprecatedString const& filename)
{
    struct stat st;
    if (stat(filename.characters(), &st) < 0)
        return false;
    return S_ISDIR(st.st_mode);
}
bool File::is_directory(int fd)
{
    struct stat st;
    if (fstat(fd, &st) < 0)
        return false;
    return S_ISDIR(st.st_mode);
}

bool File::is_device() const
{
    dbgln("File: is_device not implemented");
    VERIFY_NOT_REACHED();
}
bool File::is_device(DeprecatedString const&)
{
    dbgln("File: is_device not implemented");
    VERIFY_NOT_REACHED();
}
bool File::is_device(int)
{
    dbgln("File: is_device not implemented");
    VERIFY_NOT_REACHED();
}
bool File::is_block_device() const
{
    dbgln("File: is_block_device not implemented");
    VERIFY_NOT_REACHED();
}
bool File::is_block_device(DeprecatedString const&)
{
    dbgln("File: is_block_device not implemented");
    VERIFY_NOT_REACHED();
}
bool File::is_char_device() const
{
    dbgln("File: is_char_device not implemented");
    VERIFY_NOT_REACHED();
}
bool File::is_char_device(DeprecatedString const&)
{
    dbgln("File: is_char_device not implemented");
    VERIFY_NOT_REACHED();
}

bool File::is_link() const
{
    dbgln("File: is_link not implemented");
    VERIFY_NOT_REACHED();
}
bool File::is_link(DeprecatedString const&)
{
    dbgln("File: is_link not implemented");
    VERIFY_NOT_REACHED();
}

bool File::looks_like_shared_library() const
{
    dbgln("File: looks_like_shared_library not implemented");
    VERIFY_NOT_REACHED();
}
bool File::looks_like_shared_library(DeprecatedString const&)
{
    dbgln("File: looks_like_shared_library not implemented");
    VERIFY_NOT_REACHED();
}

bool File::exists(StringView filename)
{
    return !Core::System::stat(filename).is_error();
}
ErrorOr<size_t> File::size(DeprecatedString const&)
{
    dbgln("File: size not implemented");
    VERIFY_NOT_REACHED();
}
DeprecatedString File::current_working_directory()
{
    dbgln("File: current_working_directory not implemented");
    VERIFY_NOT_REACHED();
}
DeprecatedString File::absolute_path(DeprecatedString const&)
{
    dbgln("File: absolute_path not implemented");
    VERIFY_NOT_REACHED();
}
bool File::can_delete_or_move(StringView)
{
    dbgln("File: can_delete_or_move not implemented");
    VERIFY_NOT_REACHED();
}

ErrorOr<void, File::CopyError> File::copy_file(DeprecatedString const&, struct stat const&, File&, PreserveMode)
{
    dbgln("File: copy_file not implemented");
    VERIFY_NOT_REACHED();
}
ErrorOr<void, File::CopyError> File::copy_directory(DeprecatedString const&, DeprecatedString const&, struct stat const&, LinkMode, PreserveMode)
{
    dbgln("File: copy_directory not implemented");
    VERIFY_NOT_REACHED();
}
ErrorOr<void, File::CopyError> File::copy_file_or_directory(DeprecatedString const&, DeprecatedString const&, RecursionMode, LinkMode, AddDuplicateFileMarker, PreserveMode)
{
    dbgln("File: copy_file_or_directory not implemented");
    VERIFY_NOT_REACHED();
}

DeprecatedString File::real_path_for(DeprecatedString const& filename)
{
    TCHAR buffer[4096] = TEXT("");
    TCHAR** lppPart={NULL};

    if (filename.is_null())
        return {};

    auto retval = GetFullPathName(filename.characters(), 4096, buffer, lppPart);
	if (retval == 0)
		dbgln("GetFullPathName failed with error {}", GetLastError());
    DeprecatedString real_path(buffer);
    return real_path;
}
ErrorOr<DeprecatedString> File::read_link(DeprecatedString const&)
{
    dbgln("File: read_link not implemented");
    VERIFY_NOT_REACHED();
}
ErrorOr<void> File::link_file(DeprecatedString const&, DeprecatedString const&)
{
    dbgln("File: link_file not implemented");
    VERIFY_NOT_REACHED();
}

ErrorOr<void> File::remove(StringView, RecursionMode)
{
    dbgln("File: remove not implemented");
    VERIFY_NOT_REACHED();
}

[[nodiscard]] int File::leak_fd()
{
    dbgln("File: leak_fd not implemented");
    VERIFY_NOT_REACHED();
}

NonnullRefPtr<File> File::standard_input()
{
    dbgln("File: standard_input not implemented");
    VERIFY_NOT_REACHED();
}
NonnullRefPtr<File> File::standard_output()
{
    dbgln("File: standard_output not implemented");
    VERIFY_NOT_REACHED();
}
NonnullRefPtr<File> File::standard_error()
{
    dbgln("File: standard_error not implemented");
    VERIFY_NOT_REACHED();
}

Optional<DeprecatedString> File::resolve_executable_from_environment(StringView)
{
    dbgln("File: resolve_executable_from_environment not implemented");
    VERIFY_NOT_REACHED();
}
};
