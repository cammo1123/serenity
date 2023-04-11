/*
 * Copyright (c) 2023, Sam Atkins <atkinssj@serenityos.org>
 * Copyright (c) 2023, Cameron Youell <cameronyouell@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "DirectoryEntry.h"
#if !defined(AK_OS_WINDOWS)
#    include <dirent.h>
#endif

namespace Core {

#if !defined(AK_OS_WINDOWS)
static DirectoryEntry::Type directory_entry_type_from_posix(unsigned char dt_constant)
{
    switch (dt_constant) {
    case DT_UNKNOWN:
        return DirectoryEntry::Type::Unknown;
    case DT_FIFO:
        return DirectoryEntry::Type::NamedPipe;
    case DT_CHR:
        return DirectoryEntry::Type::CharacterDevice;
    case DT_DIR:
        return DirectoryEntry::Type::Directory;
    case DT_BLK:
        return DirectoryEntry::Type::BlockDevice;
    case DT_REG:
        return DirectoryEntry::Type::File;
    case DT_LNK:
        return DirectoryEntry::Type::SymbolicLink;
    case DT_SOCK:
        return DirectoryEntry::Type::Socket;
#    ifndef AK_OS_OPENBSD
    case DT_WHT:
        return DirectoryEntry::Type::Whiteout;
#    endif
    }
    VERIFY_NOT_REACHED();
}

DirectoryEntry DirectoryEntry::from_dirent(dirent const& de)
{
    return DirectoryEntry {
        .type = directory_entry_type_from_posix(de.d_type),
        .name = de.d_name,
    };
};
#else
static DirectoryEntry::Type directory_entry_type_from_win32(DWORD dwFileAttributes)
{
    if ((dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0u)
        return DirectoryEntry::Type::Directory;
    if ((dwFileAttributes & FILE_ATTRIBUTE_DEVICE) != 0u)
        return DirectoryEntry::Type::BlockDevice;
    if ((dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0u)
        return DirectoryEntry::Type::SymbolicLink;

    // FIXME: Check for other types here.
    return DirectoryEntry::Type::File;
}

DirectoryEntry DirectoryEntry::from_find_data(WIN32_FIND_DATA const& de)
{
    return DirectoryEntry {
        .type = directory_entry_type_from_win32(de.dwFileAttributes),
        .name = de.cFileName,
    };
};

#endif

}
