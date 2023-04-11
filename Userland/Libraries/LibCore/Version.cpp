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
#if defined(AK_OS_WINDOWS)
    return String::formatted("Version {} revision {}", "1.0.0"sv, "a"sv);
#else
    auto uname = TRY(Core::System::uname());

    auto const* version = uname.release;
    auto const* git_hash = uname.version;

    return String::formatted("Version {} revision {}", version, git_hash);
#endif
}

}
