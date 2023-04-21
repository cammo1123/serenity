/*
 * Copyright (c) 2019-2020, Sergey Bugaev <bugaevc@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibThreading/Thread.h>
#include <string.h>

#if !defined(AK_OS_WINDOWS)
#include <unistd.h>
#include <pthread.h>
#else
#include <process.h>
#endif

namespace Threading {

Thread::Thread(Function<intptr_t()> action, StringView thread_name)
    : Core::Object(nullptr)
    , m_action(move(action))
    , m_thread_name(thread_name.is_null() ? ""sv : thread_name)
{
    register_property("thread_name", [&] { return JsonValue { m_thread_name }; });
#if defined(AK_OS_SERENITY) || defined(AK_OS_LINUX)
    // FIXME: Print out a pretty TID for BSD and macOS platforms, too
    register_property("tid", [&] { return JsonValue { m_tid }; });
#endif
}

Thread::~Thread()
{
    if (needs_to_be_joined()) {
        dbgln("Destroying {} while it is still running undetached!", *this);
        [[maybe_unused]] auto res = join();
    }
    if (m_state == ThreadState::Detached)
        dbgln("Bug! {} in state {} is being destroyed; AK/Function will crash shortly!", *this, m_state.load());
}

ErrorOr<void> Thread::set_priority(int priority)
{
#if !defined(AK_OS_WINDOWS)
    // MacOS has an extra __opaque field, so list initialization will not compile on MacOS Lagom.
    sched_param scheduling_parameters {};
    scheduling_parameters.sched_priority = priority;
    int result = pthread_setschedparam(m_tid, 0, &scheduling_parameters);
    if (result != 0)
        return Error::from_errno(result);
#else
    dbgln("Thread::set_priority({}) not supported yet", priority);
    VERIFY_NOT_REACHED();
#endif
    return {};
}

ErrorOr<int> Thread::get_priority() const
{
#if !defined(AK_OS_WINDOWS)
    sched_param scheduling_parameters {};
    int policy;
    int result = pthread_getschedparam(m_tid, &policy, &scheduling_parameters);
    if (result != 0)
        return Error::from_errno(result);
    return scheduling_parameters.sched_priority;
#else
    dbgln("Thread::get_priority() not supported yet");
    VERIFY_NOT_REACHED();
    return -1;
#endif
}

DeprecatedString Thread::thread_name() const { return m_thread_name; }

ThreadID Thread::tid() const { return m_tid; }

ThreadState Thread::state() const { return m_state; }

bool Thread::is_started() const { return m_state != ThreadState::Startable; }

bool Threading::Thread::needs_to_be_joined() const
{
    auto state = m_state.load();
    return state == ThreadState::Running || state == ThreadState::Exited;
}

bool Threading::Thread::has_exited() const
{
    auto state = m_state.load();
    return state == ThreadState::Joined || state == ThreadState::Exited || state == ThreadState::DetachedExited;
}

#if !defined(AK_OS_WINDOWS)
void Thread::start()
{
    VERIFY(!is_started());

    // Set this first so that the other thread starts out seeing m_state == Running.
    m_state = Threading::ThreadState::Running;

    int rc = pthread_create(
        &m_tid,
        // FIXME: Use pthread_attr_t to start a thread detached if that was requested by the user before the call to start().
        nullptr,
        [](void* arg) -> void* {
            Thread* self = static_cast<Thread*>(arg);
            auto exit_code = self->m_action();

            auto expected = Threading::ThreadState::Running;
            // This code might race with a call to detach().
            if (!self->m_state.compare_exchange_strong(expected, Threading::ThreadState::Exited)) {
                // If the original state was Detached, we need to set to DetachedExited instead.
                if (expected == Threading::ThreadState::Detached) {
                    if (!self->m_state.compare_exchange_strong(expected, Threading::ThreadState::DetachedExited)) {
                        dbgln("Thread logic bug: Found thread state {} while trying to set ExitedDetached state!", expected);
                        VERIFY_NOT_REACHED();
                    }
                } else {
                    dbgln("Thread logic bug: Found thread state {} while trying to set Exited state!", expected);
                    VERIFY_NOT_REACHED();
                }
            }

            return reinterpret_cast<void*>(exit_code);
        },
        static_cast<void*>(this));

    VERIFY(rc == 0);
#ifdef AK_OS_SERENITY
    if (!m_thread_name.is_empty()) {
        rc = pthread_setname_np(m_tid, m_thread_name.characters());
        VERIFY(rc == 0);
    }
#endif
    dbgln("Started {}", *this);
}
#else
void Thread::start()
{
    VERIFY(!is_started());

    // Set this first so that the other thread starts out seeing m_state == Running.
    m_state = Threading::ThreadState::Running;
    _beginthread( [](void* arg){
        Thread* self = static_cast<Thread*>(arg);
        auto exit_code = self->m_action();
        auto expected = Threading::ThreadState::Running;
        // This code might race with a call to detach().
        if (!self->m_state.compare_exchange_strong(expected, Threading::ThreadState::Exited)) {
            // If the original state was Detached, we need to set to DetachedExited instead.
            if (expected == Threading::ThreadState::Detached) {
                if (!self->m_state.compare_exchange_strong(expected, Threading::ThreadState::DetachedExited)) {
                    dbgln("Thread logic bug: Found thread state {} while trying to set ExitedDetached state!", expected);
                    VERIFY_NOT_REACHED();
                }
            } else {
                dbgln("Thread logic bug: Found thread state {} while trying to set Exited state!", expected);
                VERIFY_NOT_REACHED();
            }
        }
        // TODO - how to propagate this outside this method/class?
        dbgln("Thread exited with status {}", exit_code);
    }, 0, static_cast<void*>(this) );
}
#endif

void Thread::detach()
{
    auto expected = Threading::ThreadState::Running;
    // This code might race with the other thread exiting.
    if (!m_state.compare_exchange_strong(expected, Threading::ThreadState::Detached)) {
        // Always report a precise error before crashing. These kinds of bugs are hard to reproduce.
        if (expected == Threading::ThreadState::Exited)
            dbgln("Thread logic bug: {} is being detached after having exited!", this);
        else
            dbgln("Thread logic bug: trying to detach {} which is not in the Started state, but state {}!", this, expected);
        VERIFY_NOT_REACHED();
    }

    int rc = pthread_detach(m_tid);
    VERIFY(rc == 0);
}

}
