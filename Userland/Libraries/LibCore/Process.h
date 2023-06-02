/*
 * Copyright (c) 2021, Andreas Kling <kling@serenityos.org>
 * Copyright (c) 2022, MacDue <macdue@dueutil.tech>
 * Copyright (c) 2023, Sam Atkins <atkinssj@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Forward.h>
#include <AK/Span.h>

#if defined(AK_OS_WINDOWS)
typedef int pid_t;
#    if defined(LibCore_EXPORTS)
#        define LIBCORE_API __declspec(dllexport)
#    else
#        define LIBCORE_API __declspec(dllimport)
#    endif
#else
#    define LIBCORE_API [[gnu::visibility("default")]]
#endif

namespace Core {

class Process {
public:
    enum class KeepAsChild {
        Yes,
        No
    };

    LIBCORE_API static ErrorOr<pid_t> spawn(StringView path, ReadonlySpan<DeprecatedString> arguments, DeprecatedString working_directory = {}, KeepAsChild keep_as_child = KeepAsChild::No);
    LIBCORE_API static ErrorOr<pid_t> spawn(StringView path, ReadonlySpan<StringView> arguments, DeprecatedString working_directory = {}, KeepAsChild keep_as_child = KeepAsChild::No);
    LIBCORE_API static ErrorOr<pid_t> spawn(StringView path, ReadonlySpan<char const*> arguments = {}, DeprecatedString working_directory = {}, KeepAsChild keep_as_child = KeepAsChild::No);

    static ErrorOr<String> get_name();
    enum class SetThreadName {
        No,
        Yes,
    };
    static ErrorOr<void> set_name(StringView, SetThreadName = SetThreadName::No);
};

}
