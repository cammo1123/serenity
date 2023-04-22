/*
 * Copyright (c) 2018-2020, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#ifndef KERNEL

#    include <AK/DeprecatedString.h>
#    include <AK/StringView.h>
#    if !defined(AK_OS_WINDOWS)
#        include <cxxabi.h>
#    else
#        include <windows.h>
// Windows must go first
#        include <dbghelp.h>
#    endif

namespace AK {

inline DeprecatedString demangle(StringView name)
{
#    if !defined(AK_OS_WINDOWS)
    int status = 0;
    auto* demangled_name = abi::__cxa_demangle(name.to_deprecated_string().characters(), nullptr, nullptr, &status);
    auto string = DeprecatedString(status == 0 ? StringView { demangled_name, strlen(demangled_name) } : name);
    if (status == 0)
#    else
    auto length = UnDecorateSymbolName(name.to_deprecated_string().characters(), nullptr, 0, UNDNAME_COMPLETE);
    char* demangled_name = (char*)malloc(length);
    UnDecorateSymbolName(name.to_deprecated_string().characters(), demangled_name, length, UNDNAME_COMPLETE);
    auto string = DeprecatedString { demangled_name, strlen(demangled_name) };
    if (demangled_name != 0)
#    endif
        free(demangled_name);
    return string;
}

}

#    if USING_AK_GLOBALLY
using AK::demangle;
#    endif

#endif
