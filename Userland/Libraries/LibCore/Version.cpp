/*
 * Copyright (c) 2021, Mahmoud Mandour <ma.mandourr@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/String.h>
#include <LibCore/System.h>
#include <LibCore/Version.h>

namespace Core::Version {

ErrorOr<String> read_long_version_string()
{
#if !defined(AK_OS_WINDOWS)
    auto uname = TRY(Core::System::uname());

    auto const* version = uname.release;
    auto const* git_hash = uname.version;

    return String::formatted("Version {} revision {}", version, git_hash);
#else
    OSVERSIONINFO sys_version_info;
    GetVersionExW((LPOSVERSIONINFOW)&sys_version_info);

    return String::formatted("Version {}.{} revision {}", sys_version_info.dwMajorVersion, sys_version_info.dwMinorVersion, sys_version_info.dwBuildNumber);
#endif
}

}
