/*
 * Copyright (c) 2021, Andreas Kling <kling@serenityos.org>
 * Copyright (c) 2022-2023, MacDue <macdue@dueutil.tech>
 * Copyright (c) 2023, Sam Atkins <atkinssj@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/DeprecatedString.h>
#include <AK/ScopeGuard.h>
#include <AK/String.h>
#include <AK/Vector.h>
#include <LibCore/Process.h>
#include <LibCore/System.h>
#include <errno.h>

#ifdef AK_OS_SERENITY
#    include <serenity.h>
#    include <syscall.h>
#endif

#if defined(AK_OS_MACOS)
#    include <crt_externs.h>
#endif

static char** environment()
{
#if defined(AK_OS_MACOS)
    return *_NSGetEnviron();
#else
    extern char** environ;
    return environ;
#endif
}

namespace Core {

struct ArgvList {
    DeprecatedString m_path;
    DeprecatedString m_working_directory;
    Vector<char const*, 10> m_argv;

    ArgvList(DeprecatedString path, size_t size)
        : m_path { path }
    {
        m_argv.ensure_capacity(size + 2);
        m_argv.append(m_path.characters());
    }

    void append(char const* arg)
    {
        m_argv.append(arg);
    }

    Span<char const*> get()
    {
        if (m_argv.is_empty() || m_argv.last() != nullptr)
            m_argv.append(nullptr);
        return m_argv;
    }

    void set_working_directory(DeprecatedString const& working_directory)
    {
        m_working_directory = working_directory;
    }

    ErrorOr<pid_t> spawn(Process::KeepAsChild keep_as_child)
    {
        dbgln("Core::Process::spawn(): Spawning child process {}", m_path);
        (void)keep_as_child;
        auto argv = get();
        auto envp = environment();
        char const* cwd = m_working_directory.is_null() ? nullptr : m_working_directory.characters();

        dbgln("Core::Process::spawn(): Spawning child process {} with arguments: argc={}, argv={}", m_path, argv.size() - 1, argv);
        dbgln("Core::Process::spawn(): Spawning child process {} with environment: envp={}", m_path, envp);
        dbgln("Core::Process::spawn(): Spawning child process {} with working directory: cwd={}", m_path, cwd);

        VERIFY_NOT_REACHED();
    }
};

ErrorOr<pid_t> Process::spawn(StringView path, ReadonlySpan<DeprecatedString> arguments, DeprecatedString working_directory, KeepAsChild keep_as_child)
{
    ArgvList argv { path, arguments.size() };
    for (auto const& arg : arguments)
        argv.append(arg.characters());
    argv.set_working_directory(working_directory);
    return argv.spawn(keep_as_child);
}

ErrorOr<pid_t> Process::spawn(StringView path, ReadonlySpan<StringView> arguments, DeprecatedString working_directory, KeepAsChild keep_as_child)
{
    Vector<DeprecatedString> backing_strings;
    backing_strings.ensure_capacity(arguments.size());
    ArgvList argv { path, arguments.size() };
    for (auto const& arg : arguments) {
        backing_strings.append(arg);
        argv.append(backing_strings.last().characters());
    }
    argv.set_working_directory(working_directory);
    return argv.spawn(keep_as_child);
}

ErrorOr<pid_t> Process::spawn(StringView path, ReadonlySpan<char const*> arguments, DeprecatedString working_directory, KeepAsChild keep_as_child)
{
    ArgvList argv { path, arguments.size() };
    for (auto arg : arguments)
        argv.append(arg);
    argv.set_working_directory(working_directory);
    return argv.spawn(keep_as_child);
}

ErrorOr<String> Process::get_name()
{
#if defined(AK_OS_SERENITY)
    char buffer[BUFSIZ];
    int rc = get_process_name(buffer, BUFSIZ);
    if (rc != 0)
        return Error::from_syscall("get_process_name"sv, -rc);
    return String::from_utf8(StringView { buffer, strlen(buffer) });
#else
    // FIXME: Implement Process::get_name() for other platforms.
    return "???"_short_string;
#endif
}

ErrorOr<void> Process::set_name([[maybe_unused]] StringView name, [[maybe_unused]] SetThreadName set_thread_name)
{
#if defined(AK_OS_SERENITY)
    int rc = set_process_name(name.characters_without_null_termination(), name.length());
    if (rc != 0)
        return Error::from_syscall("set_process_name"sv, -rc);
    if (set_thread_name == SetThreadName::No)
        return {};

    rc = syscall(SC_set_thread_name, gettid(), name.characters_without_null_termination(), name.length());
    if (rc != 0)
        return Error::from_syscall("set_thread_name"sv, -rc);
    return {};
#else
    // FIXME: Implement Process::set_name() for other platforms.
    return {};
#endif
}

}
