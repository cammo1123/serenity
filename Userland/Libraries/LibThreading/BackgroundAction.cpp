/*
 * Copyright (c) 2019-2020, Sergey Bugaev <bugaevc@serenityos.org>
 * Copyright (c) 2021, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/Queue.h>
#include <LibThreading/BackgroundAction.h>
#include <LibThreading/Mutex.h>
#include <LibThreading/Thread.h>

#ifdef AK_OS_WINDOWS
#    include <windows.h>
#else
static pthread_mutex_t s_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t s_condition = PTHREAD_COND_INITIALIZER;
#endif

static Queue<Function<void()>>* s_all_actions;
static Threading::Thread* s_background_thread;

static intptr_t background_thread_func()
{
    Vector<Function<void()>> actions;
    while (true) {
#ifdef AK_OS_WINDOWS
        HANDLE handles[1];
        handles[0] = s_background_thread->tid();
        DWORD result = WaitForMultipleObjects(1, handles, FALSE, INFINITE);
        if (result != WAIT_OBJECT_0) {
            dbgln("WaitForMultipleObjects failed with {}", GetLastError());
            VERIFY_NOT_REACHED();
        }
#else
        pthread_mutex_lock(&s_mutex);

        while (s_all_actions->is_empty())
            pthread_cond_wait(&s_condition, &s_mutex);
#endif

        while (!s_all_actions->is_empty())
            actions.append(s_all_actions->dequeue());

#if !defined(AK_OS_WINDOWS)
        pthread_mutex_unlock(&s_mutex);
#endif

        for (auto& action : actions)
            action();

        actions.clear();
    }
}

static void init()
{
    s_all_actions = new Queue<Function<void()>>;
    s_background_thread = &Threading::Thread::construct(background_thread_func, "Background Thread"sv).leak_ref();
    s_background_thread->start();
}

Threading::Thread& Threading::BackgroundActionBase::background_thread()
{
    if (s_background_thread == nullptr)
        init();
    return *s_background_thread;
}

void Threading::BackgroundActionBase::enqueue_work(Function<void()> work)
{
    if (s_all_actions == nullptr)
        init();

#ifdef AK_OS_WINDOWS
    QueueUserAPC(
        [](ULONG_PTR param) {
            auto& work = *(Function<void()>*)param;
            work();
        },
        s_background_thread->tid(),
        (ULONG_PTR)&work);
#else
    pthread_mutex_lock(&s_mutex);
    s_all_actions->enqueue(move(work));
    pthread_cond_broadcast(&s_condition);
    pthread_mutex_unlock(&s_mutex);
#endif
}
