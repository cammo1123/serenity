/*
 * Copyright (c) 2018-2022, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibCore/DeprecatedFile.h>
#include <LibCore/System.h>

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

bool DeprecatedFile::is_directory() const
{
    return is_directory(fd());
}

bool DeprecatedFile::is_directory(DeprecatedString const& filename)
{
    struct stat st;
    if (stat(filename.characters(), &st) < 0)
        return false;
    return S_ISDIR(st.st_mode);
}

bool DeprecatedFile::is_directory(int fd)
{
    struct stat st;
    if (fstat(fd, &st) < 0)
        return false;
    return S_ISDIR(st.st_mode);
}

bool DeprecatedFile::exists(StringView DeprecatedFilename)
{
    return !Core::System::stat(DeprecatedFilename).is_error();
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
bool DeprecatedFile::is_device(DeprecatedString const&)
{
    dbgln("DeprecatedFile: is_device not implemented");
    VERIFY_NOT_REACHED();
}
bool DeprecatedFile::is_device(int)
{
    dbgln("DeprecatedFile: is_device not implemented");
    VERIFY_NOT_REACHED();
}
bool DeprecatedFile::is_block_device() const
{
    dbgln("DeprecatedFile: is_block_device not implemented");
    VERIFY_NOT_REACHED();
}
bool DeprecatedFile::is_block_device(DeprecatedString const&)
{
    dbgln("DeprecatedFile: is_block_device not implemented");
    VERIFY_NOT_REACHED();
}
bool DeprecatedFile::is_char_device() const
{
    dbgln("DeprecatedFile: is_char_device not implemented");
    VERIFY_NOT_REACHED();
}
bool DeprecatedFile::is_char_device(DeprecatedString const&)
{
    dbgln("DeprecatedFile: is_char_device not implemented");
    VERIFY_NOT_REACHED();
}

bool DeprecatedFile::is_link() const
{
    dbgln("DeprecatedFile: is_link not implemented");
    VERIFY_NOT_REACHED();
}
bool DeprecatedFile::is_link(DeprecatedString const&)
{
    dbgln("DeprecatedFile: is_link not implemented");
    VERIFY_NOT_REACHED();
}

bool DeprecatedFile::looks_like_shared_library() const
{
    dbgln("DeprecatedFile: looks_like_shared_library not implemented");
    VERIFY_NOT_REACHED();
}
bool DeprecatedFile::looks_like_shared_library(DeprecatedString const&)
{
    dbgln("DeprecatedFile: looks_like_shared_library not implemented");
    VERIFY_NOT_REACHED();
}

ErrorOr<size_t> DeprecatedFile::size(DeprecatedString const&)
{
    dbgln("DeprecatedFile: size not implemented");
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
bool DeprecatedFile::can_delete_or_move(StringView)
{
    dbgln("DeprecatedFile: can_delete_or_move not implemented");
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
ErrorOr<void> DeprecatedFile::link_file(DeprecatedString const&, DeprecatedString const&)
{
    dbgln("DeprecatedFile: link_file not implemented");
    VERIFY_NOT_REACHED();
}

ErrorOr<void> DeprecatedFile::remove(StringView, RecursionMode)
{
    dbgln("DeprecatedFile: remove not implemented");
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
