/*
 * Copyright (c) 2018-2020, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibCore/Event.h>
#include <LibCore/EventLoop.h>
#include <LibCore/Notifier.h>

namespace Core {

#if !defined(AK_OS_WINDOWS)
Notifier::Notifier(int fd, unsigned event_mask, Object* parent)
    : Object(parent)
    , m_fd(fd)
    , m_event_mask(event_mask)
#else
Notifier::Notifier(HANDLE handle, unsigned event_mask, Object* parent)
    : Object(parent)
    , m_handle(handle)
    , m_event_mask(event_mask)
#endif
{
    set_enabled(true);
}

Notifier::~Notifier()
{
    set_enabled(false);
}

void Notifier::set_enabled(bool enabled)
{
#if !defined(AK_OS_WINDOWS)
    if (m_fd < 0)
#else
    if (m_handle == INVALID_HANDLE_VALUE)
#endif
        return;
    if (enabled)
        Core::EventLoop::register_notifier({}, *this);
    else
        Core::EventLoop::unregister_notifier({}, *this);
}

void Notifier::close()
{
#if !defined(AK_OS_WINDOWS)
    if (m_fd < 0)
        return;
    set_enabled(false);
    m_fd = -1;
#else
    if (m_handle == INVALID_HANDLE_VALUE)
        return;
    set_enabled(false);
    m_handle = INVALID_HANDLE_VALUE;
#endif
}

void Notifier::event(Core::Event& event)
{
    if (event.type() == Core::Event::NotifierRead && on_ready_to_read) {
        on_ready_to_read();
    } else if (event.type() == Core::Event::NotifierWrite && on_ready_to_write) {
        on_ready_to_write();
    } else {
        Object::event(event);
    }
}

}
