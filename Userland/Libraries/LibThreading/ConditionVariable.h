/*
 * Copyright (c) 2021, kleines Filmröllchen <filmroellchen@serenityos.org>.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Function.h>
#include <LibThreading/Mutex.h>
#if defined(AK_OS_WINDOWS)
#    include <synchapi.h>
#    include <windows.h>
#else
#    include <pthread.h>
#    include <sys/types.h>
#endif

namespace Threading {

class ConditionVariable {
    friend class Mutex;

public:
    ConditionVariable(Mutex& to_wait_on)
        : m_to_wait_on(to_wait_on)
    {
#if defined(AK_OS_WINDOWS)
        InitializeConditionVariable(&m_condition);
#else
        auto result = pthread_cond_init(&m_condition, nullptr);
        VERIFY(result == 0);
#endif
    }

    ALWAYS_INLINE ~ConditionVariable()
    {
#if defined(AK_OS_WINDOWS)
        // Nothing to do.
#else
        auto result = pthread_cond_destroy(&m_condition);
        VERIFY(result == 0);
#endif
    }

    ALWAYS_INLINE void wait()
    {
#if defined(AK_OS_WINDOWS)
        // Release the lock so another thread can acquire it.
        m_to_wait_on.unlock();

        // Wait for a signal on the condition variable.
        SleepConditionVariableCS(&m_condition, &m_to_wait_on.m_critical_section, INFINITE);

        // Re-acquire the lock.
        m_to_wait_on.lock();
#else
        auto result = pthread_cond_wait(&m_condition, &m_to_wait_on.m_mutex);
        VERIFY(result == 0);
#endif
    }

    ALWAYS_INLINE void wait_while(Function<bool()> condition)
    {
        while (condition())
            wait();
    }
    // Release at least one of the threads waiting on this variable.
    ALWAYS_INLINE void signal()
    {
#if defined(AK_OS_WINDOWS)
        WakeConditionVariable(&m_condition);
#else
        auto result = pthread_cond_signal(&m_condition);
        VERIFY(result == 0);
#endif
    }
    // Release all of the threads waiting on this variable.
    ALWAYS_INLINE void broadcast()
    {
#if defined(AK_OS_WINDOWS)
        WakeAllConditionVariable(&m_condition);
#else
        auto result = pthread_cond_broadcast(&m_condition);
        VERIFY(result == 0);
#endif
    }

private:
#if defined(AK_OS_WINDOWS)
    CONDITION_VARIABLE m_condition {};
#else
    pthread_cond_t m_condition;
#endif
    Mutex& m_to_wait_on;
};

}
