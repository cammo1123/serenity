/*
 * Copyright (c) 2018-2021, Andreas Kling <kling@serenityos.org>
 * Copyright (c) 2023, Cameron Youell <cameronyouell@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "AK/Assertions.h"
#include "AK/DeprecatedString.h"
#include "LibCore/DeprecatedFile.h"
#include <AK/Error.h>
#include <AK/Forward.h>
#include <AK/LexicalPath.h>
#include <AK/String.h>
#include <AK/StringView.h>
#include <LibCore/DirIterator.h>
#include <LibCore/System.h>
#include <LibFileSystem/FileSystem.h>
#include <limits.h>
#include <minwindef.h>

namespace FileSystem {

ErrorOr<String> current_working_directory()
{
    auto cwd = TRY(Core::System::getcwd());
    return TRY(String::from_deprecated_string({ cwd }));
}

ErrorOr<String> absolute_path(StringView path)
{
    if (exists(path))
        return TRY(real_path(path));

    if (path.starts_with("/"sv))
        return TRY(String::from_deprecated_string(LexicalPath::canonicalized_path(path)));

    auto working_directory = TRY(current_working_directory());
    auto full_path = LexicalPath::join(working_directory, path).string();

    return TRY(String::from_deprecated_string(LexicalPath::canonicalized_path(full_path)));
}

ErrorOr<String> real_path(StringView path)
{
    if (path.is_null())
        return Error::from_errno(ENOENT);

    char buffer[MAX_PATH];
    DeprecatedString dep_path = path;

    if (GetFullPathName(dep_path.characters(), MAX_PATH, buffer, nullptr) == 0)
        return Error::from_errno(errno);

    return TRY(String::from_deprecated_string(buffer));
}

bool exists(StringView path)
{
    return !Core::System::stat(path).is_error();
}

bool exists(int fd)
{
    return !Core::System::fstat(fd).is_error();
}

bool is_device(StringView path)
{
    dbgln("FileSystem: is_device not implemented: {}", path);
    VERIFY_NOT_REACHED();
}

bool is_device(int fd)
{
    dbgln("FileSystem: is_device not implemented: {}", fd);
    VERIFY_NOT_REACHED();
}

bool is_block_device(StringView path)
{
    dbgln("FileSystem: is_block_device not implemented: {}", path);
    VERIFY_NOT_REACHED();
}

bool is_block_device(int fd)
{
    dbgln("FileSystem: is_block_device not implemented: {}", fd);
    VERIFY_NOT_REACHED();
}

bool is_char_device(StringView path)
{
    dbgln("FileSystem: is_char_device not implemented: {}", path);
    VERIFY_NOT_REACHED();
}

bool is_char_device(int fd)
{
    dbgln("FileSystem: is_char_device not implemented: {}", fd);
    VERIFY_NOT_REACHED();
}

bool is_directory(StringView path)
{
    DWORD const attributes = GetFileAttributes(path.to_deprecated_string().characters());
    if (attributes == INVALID_FILE_ATTRIBUTES)
        return false;
    return (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0u;
}

bool is_directory(int fd)
{
    dbgln("FileSystem: is_directory not implemented: {}", fd);
    VERIFY_NOT_REACHED();
}

bool is_link(StringView path)
{
    dbgln("FileSystem: is_link not implemented: {}", path);
    VERIFY_NOT_REACHED();
}

bool is_link(int fd)
{
    dbgln("FileSystem: is_link not implemented: {}", fd);
    VERIFY_NOT_REACHED();
}

static ErrorOr<String> get_duplicate_file_name(StringView path)
{
    int duplicate_count = 0;
    LexicalPath lexical_path(path);
    auto parent_path = LexicalPath::canonicalized_path(lexical_path.dirname());
    auto basename = lexical_path.basename();
    auto current_name = TRY(String::from_deprecated_string(LexicalPath::join(parent_path, basename).string()));

    while (exists(current_name)) {
        ++duplicate_count;
        current_name = TRY(String::from_deprecated_string(LexicalPath::join(parent_path, TRY(String::formatted("{} ({})", basename, duplicate_count))).string()));
    }

    return current_name;
}

ErrorOr<void> copy_file(StringView destination_path, StringView source_path, struct stat const& source_stat, Core::File& source, PreserveMode)
{
    auto destination_or_error = Core::File::open(destination_path, Core::File::OpenMode::Write, 0666);
    if (destination_or_error.is_error()) {
        if (destination_or_error.error().code() != EISDIR)
            return destination_or_error.release_error();

        auto destination_dir_path = TRY(String::formatted("{}/{}", destination_path, LexicalPath::basename(source_path)));
        destination_or_error = TRY(Core::File::open(destination_dir_path, Core::File::OpenMode::Write, 0666));
    }
    auto destination = destination_or_error.release_value();

    if (source_stat.st_size > 0)
        TRY(destination->truncate(source_stat.st_size));

    while (true) {
        auto bytes_read = TRY(source.read_until_eof());

        if (bytes_read.is_empty())
            break;

        TRY(destination->write_until_depleted(bytes_read));
    }

    VERIFY_NOT_REACHED();
    return {};
}

ErrorOr<void> copy_directory(StringView destination_path, StringView source_path, struct stat const&, LinkMode link, PreserveMode preserve_mode)
{
    TRY(Core::System::mkdir(destination_path, 0755));

    auto source_rp = TRY(real_path(source_path));
    source_rp = TRY(String::formatted("{}/", source_rp));

    auto destination_rp = TRY(real_path(destination_path));
    destination_rp = TRY(String::formatted("{}/", destination_rp));

    if (!destination_rp.is_empty() && destination_rp.starts_with_bytes(source_rp))
        return Error::from_errno(EINVAL);

    Core::DirIterator di(source_path, Core::DirIterator::SkipParentAndBaseDir);
    if (di.has_error())
        return di.error();

    while (di.has_next()) {
        auto filename = TRY(String::from_deprecated_string(di.next_path()));
        TRY(copy_file_or_directory(
            TRY(String::formatted("{}/{}", destination_path, filename)),
            TRY(String::formatted("{}/{}", source_path, filename)),
            RecursionMode::Allowed, link, AddDuplicateFileMarker::Yes, preserve_mode));
    }

    VERIFY_NOT_REACHED();
    return {};
}

ErrorOr<void> copy_file_or_directory(StringView destination_path, StringView source_path, RecursionMode recursion_mode, LinkMode link_mode, AddDuplicateFileMarker add_duplicate_file_marker, PreserveMode preserve_mode)
{
    String final_destination_path;
    if (add_duplicate_file_marker == AddDuplicateFileMarker::Yes)
        final_destination_path = TRY(get_duplicate_file_name(destination_path));
    else
        final_destination_path = TRY(String::from_utf8(destination_path));

    auto source = TRY(Core::File::open(source_path, Core::File::OpenMode::Read));

    auto source_stat = TRY(Core::System::fstat(source->fd()));

    if (is_directory(source_path)) {
        if (recursion_mode == RecursionMode::Disallowed) {
            return Error::from_errno(EISDIR);
        }

        return copy_directory(final_destination_path, source_path, source_stat);
    }

    if (link_mode == LinkMode::Allowed)
        return TRY(Core::System::link(source_path, final_destination_path));

    return copy_file(final_destination_path, source_path, source_stat, *source, preserve_mode);
}

ErrorOr<void> remove(StringView path, RecursionMode mode)
{
    if (is_directory(path) && mode == RecursionMode::Allowed) {
        auto di = Core::DirIterator(path, Core::DirIterator::SkipParentAndBaseDir);
        if (di.has_error())
            return di.error();

        while (di.has_next())
            TRY(remove(di.next_full_path(), RecursionMode::Allowed));

        TRY(Core::System::rmdir(path));
    } else {
        TRY(Core::System::unlink(path));
    }

    return {};
}

ErrorOr<size_t> size(StringView path)
{
    auto st = TRY(Core::System::stat(path));
    return st.st_size;
}

bool can_delete_or_move(StringView path)
{
    VERIFY_NOT_REACHED();
    (void)path;
}

ErrorOr<String> read_link(StringView link_path)
{
    return TRY(String::from_deprecated_string(TRY(Core::System::readlink(link_path))));
}

ErrorOr<void> link_file(StringView destination_path, StringView source_path)
{
    return TRY(Core::System::symlink(source_path, TRY(get_duplicate_file_name(destination_path))));
}

ErrorOr<String> resolve_executable_from_environment(StringView filename)
{
    if (filename.is_empty())
        return Error::from_errno(ENOENT);

    VERIFY_NOT_REACHED();
}

bool looks_like_shared_library(StringView path)
{
    return path.ends_with(".so"sv) || path.contains(".so."sv);
}

}
