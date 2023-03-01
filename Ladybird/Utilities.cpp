/*
 * Copyright (c) 2022, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#define AK_DONT_REPLACE_STD

#include "Utilities.h"
#include <AK/LexicalPath.h>
#include <AK/Platform.h>
#include <LibCore/File.h>
#include <LibCore/StandardPaths.h>
#include <QCoreApplication>

#ifdef AK_OS_WINDOWS
#    include <windows.h>
#endif

DeprecatedString s_serenity_resource_root;

AK::DeprecatedString ak_deprecated_string_from_qstring(QString const& qstring)
{
    return AK::DeprecatedString(qstring.toUtf8().data());
}

ErrorOr<String> ak_string_from_qstring(QString const& qstring)
{
    return String::from_utf8(StringView(qstring.toUtf8().data(), qstring.size()));
}

QString qstring_from_ak_deprecated_string(AK::DeprecatedString const& ak_deprecated_string)
{
    return QString::fromUtf8(ak_deprecated_string.characters(), ak_deprecated_string.length());
}

void platform_init()
{
#ifdef AK_OS_ANDROID
    extern void android_platform_init();
    android_platform_init();
#elif defined(AK_OS_WINDOWS)
    if (GetConsoleWindow() == nullptr) {
        if (AttachConsole(ATTACH_PARENT_PROCESS)) {
            freopen("CONOUT$", "w", stdout);
            freopen("CONOUT$", "w", stderr);
        }
    } else {
        // we're running in a console, so we can use the console
        // for debugging purposes
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);

        // The output is buffered by default, so we need to flush it manually
        setvbuf(stdout, nullptr, _IONBF, 0);
        setvbuf(stderr, nullptr, _IONBF, 0);
    }
#endif
    s_serenity_resource_root = [] {
        auto const* source_dir = getenv("SERENITY_SOURCE_DIR");
        if (source_dir) {
            return DeprecatedString::formatted("{}/Base", source_dir);
        }
        auto* home = getenv("XDG_CONFIG_HOME") ?: getenv("HOME") ?: getenv("USERPROFILE");
        VERIFY(home);
        auto home_lagom = DeprecatedString::formatted("{}/.lagom", home);
        if (Core::File::is_directory(home_lagom))
            return home_lagom;
        auto app_dir = ak_deprecated_string_from_qstring(QCoreApplication::applicationDirPath());
        return LexicalPath(app_dir).parent().append("share"sv).string();
    }();
}
