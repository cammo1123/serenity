/*
 * Copyright (c) 2023, Sam Atkins <atkinssj@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "DirectoryEntry.h"
#include <dirent.h>

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
    case DT_WHT:
        return DirectoryEntry::Type::Whiteout;
    }
    VERIFY_NOT_REACHED();
}
#endif

DirectoryEntry DirectoryEntry::from_dirent(dirent const& de)
{
#if !defined(AK_OS_WINDOWS)
    return DirectoryEntry {
        .type = directory_entry_type_from_posix(de.d_type),
        .name = de.d_name,
    };
#else
    return DirectoryEntry {
        .type = DirectoryEntry::Type::File,
        .name = de.d_name,
    };
#endif
};

}
