/*
 * Copyright (c) 2023, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/NonnullOwnPtr.h>
#include <LibCore/Event.h>
#include <LibCore/EventLoopImplementation.h>
#include <LibCore/ThreadEventQueue.h>

#if defined(AK_OS_WINDOWS)
#    include <LibCore/EventLoopImplementationWindows.h>
#else
#    include <LibCore/EventLoopImplementationUnix.h>
#endif

namespace Core {

EventLoopImplementation::EventLoopImplementation()
    : m_thread_event_queue(ThreadEventQueue::current())
{
}

EventLoopImplementation::~EventLoopImplementation() = default;

static EventLoopManager* s_event_loop_manager;
EventLoopManager& EventLoopManager::the()
{
    if (!s_event_loop_manager)
#if defined(AK_OS_WINDOWS)
        s_event_loop_manager = new EventLoopManagerWindows;
#else
        s_event_loop_manager = new EventLoopManagerUnix;
#endif
    return *s_event_loop_manager;
}

void EventLoopManager::install(Core::EventLoopManager& manager)
{
    s_event_loop_manager = &manager;
}

EventLoopManager::EventLoopManager() = default;

EventLoopManager::~EventLoopManager() = default;

}
