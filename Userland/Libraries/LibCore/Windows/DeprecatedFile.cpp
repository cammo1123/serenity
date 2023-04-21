/*
 * Copyright (c) 2018-2022, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "AK/Assertions.h"
#include <LibCore/DeprecatedFile.h>
#include <LibCore/System.h>
#include <io.h>

namespace Core {

ErrorOr<NonnullRefPtr<DeprecatedFile>> DeprecatedFile::open(DeprecatedString filename, OpenMode mode, mode_t permissions)
{
    auto file = DeprecatedFile::construct(move(filename));
    if (!file->open_impl(mode, permissions))
        return Error::from_errno(file->error());
    return file;
}

DeprecatedFile::DeprecatedFile(DeprecatedString filename, Object* parent)
    : IODevice(parent)
    , m_filename(move(filename))
{
}

DeprecatedFile::~DeprecatedFile()
{
    if (m_should_close_file_descriptor == ShouldCloseFileDescriptor::Yes && mode() != OpenMode::NotOpen)
        close();
}

bool DeprecatedFile::open(int fd, OpenMode mode, ShouldCloseFileDescriptor should_close)
{
    set_fd(fd);
    set_mode(mode);
    m_should_close_file_descriptor = should_close;
    return true;
}

bool DeprecatedFile::open(OpenMode mode)
{
    return open_impl(mode, 0666);
}

bool DeprecatedFile::open_impl(OpenMode mode, mode_t permissions)
{
    VERIFY(!m_filename.is_null());
    DWORD flags = 0;
    if (has_flag(mode, OpenMode::ReadOnly) && has_flag(mode, OpenMode::WriteOnly)) {
        flags |= GENERIC_READ | GENERIC_WRITE;
    } else if (has_flag(mode, OpenMode::ReadOnly)) {
        flags |= GENERIC_READ;
    } else if (has_flag(mode, OpenMode::WriteOnly)) {
        flags |= GENERIC_WRITE;
    }

    // NOTE: No should_truncate

    HANDLE handle = CreateFile(m_filename.characters(), flags, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (handle == INVALID_HANDLE_VALUE) {
        set_error(GetLastError());
        return false;
    }

    set_fd(_open_osfhandle((intptr_t)handle, 0));
    set_mode(mode);

    (void)permissions;
    return true;
}

bool DeprecatedFile::is_directory() const
{
    dbgln("DeprecatedFile: is_directory not implemented");
    VERIFY_NOT_REACHED();
}

DeprecatedString DeprecatedFile::real_path_for(DeprecatedString const& filename)
{
    TCHAR buffer[4096] = TEXT("");
    TCHAR** lppPart = { NULL };

    if (filename.is_null())
        return {};

    auto retval = GetFullPathName(filename.characters(), 4096, buffer, lppPart);
    if (retval == 0)
        dbgln("GetFullPathName failed with error {}", GetLastError());
    DeprecatedString real_path(buffer);
    return real_path;
}

bool DeprecatedFile::is_device() const
{
    dbgln("DeprecatedFile: is_device not implemented");
    VERIFY_NOT_REACHED();
}
bool DeprecatedFile::is_block_device() const
{
    dbgln("DeprecatedFile: is_block_device not implemented");
    VERIFY_NOT_REACHED();
}
bool DeprecatedFile::is_char_device() const
{
    dbgln("DeprecatedFile: is_char_device not implemented");
    VERIFY_NOT_REACHED();
}
bool DeprecatedFile::is_link() const
{
    dbgln("DeprecatedFile: is_link not implemented");
    VERIFY_NOT_REACHED();
}
bool DeprecatedFile::looks_like_shared_library() const
{
    dbgln("DeprecatedFile: looks_like_shared_library not implemented");
    VERIFY_NOT_REACHED();
}
DeprecatedString DeprecatedFile::current_working_directory()
{
    dbgln("DeprecatedFile: current_working_directory not implemented");
    VERIFY_NOT_REACHED();
}
DeprecatedString DeprecatedFile::absolute_path(DeprecatedString const&)
{
    dbgln("DeprecatedFile: absolute_path not implemented");
    VERIFY_NOT_REACHED();
}
ErrorOr<void, DeprecatedFile::CopyError> DeprecatedFile::copy_file(DeprecatedString const&, struct stat const&, DeprecatedFile&, PreserveMode)
{
    dbgln("DeprecatedFile: copy_file not implemented");
    VERIFY_NOT_REACHED();
}
ErrorOr<void, DeprecatedFile::CopyError> DeprecatedFile::copy_directory(DeprecatedString const&, DeprecatedString const&, struct stat const&, LinkMode, PreserveMode)
{
    dbgln("DeprecatedFile: copy_directory not implemented");
    VERIFY_NOT_REACHED();
}
ErrorOr<void, DeprecatedFile::CopyError> DeprecatedFile::copy_file_or_directory(DeprecatedString const&, DeprecatedString const&, RecursionMode, LinkMode, AddDuplicateFileMarker, PreserveMode)
{
    dbgln("DeprecatedFile: copy_file_or_directory not implemented");
    VERIFY_NOT_REACHED();
}

ErrorOr<DeprecatedString> DeprecatedFile::read_link(DeprecatedString const&)
{
    dbgln("DeprecatedFile: read_link not implemented");
    VERIFY_NOT_REACHED();
}
[[nodiscard]] int DeprecatedFile::leak_fd()
{
    dbgln("DeprecatedFile: leak_fd not implemented");
    VERIFY_NOT_REACHED();
}

NonnullRefPtr<DeprecatedFile> DeprecatedFile::standard_input()
{
    dbgln("DeprecatedFile: standard_input not implemented");
    VERIFY_NOT_REACHED();
}
NonnullRefPtr<DeprecatedFile> DeprecatedFile::standard_output()
{
    dbgln("DeprecatedFile: standard_output not implemented");
    VERIFY_NOT_REACHED();
}
NonnullRefPtr<DeprecatedFile> DeprecatedFile::standard_error()
{
    dbgln("DeprecatedFile: standard_error not implemented");
    VERIFY_NOT_REACHED();
}

Optional<DeprecatedString> DeprecatedFile::resolve_executable_from_environment(StringView)
{
    dbgln("DeprecatedFile: resolve_executable_from_environment not implemented");
    VERIFY_NOT_REACHED();
}
};
