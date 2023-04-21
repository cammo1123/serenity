/*
 * Copyright (c) 2022, Peter Elliott <pelliott@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Error.h>
#include <AK/Types.h>

#if defined(AK_OS_WINDOWS)
#    include <windows.h>
#endif

namespace Core::SessionManagement {
#if !defined(AK_OS_WINDOWS)
ErrorOr<pid_t> root_session_id(Optional<pid_t> force_sid = {});
ErrorOr<void> logout(Optional<pid_t> force_sid = {});

ErrorOr<DeprecatedString> parse_path_with_sid(StringView general_path, Optional<pid_t> force_sid = {});
ErrorOr<void> create_session_temporary_directory_if_needed(uid_t uid, gid_t gid, Optional<pid_t> force_sid = {});

#else
ErrorOr<DWORD> root_session_id(Optional<DWORD> force_sid = {});
ErrorOr<void> logout(Optional<DWORD> force_sid = {});

ErrorOr<DeprecatedString> parse_path_with_sid(StringView general_path, Optional<DWORD> force_sid = {});
ErrorOr<void> create_session_temporary_directory_if_needed(uid_t uid, gid_t gid, Optional<DWORD> force_sid = {});
#endif

}
