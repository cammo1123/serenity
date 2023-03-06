/*
 * Copyright (c) 2018-2020, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Function.h>
#include <LibCore/Object.h>
#if defined(AK_OS_WINDOWS)
#    include <AK/Windows.h>
#endif

namespace Core {

class Notifier : public Object {
    C_OBJECT(Notifier)
public:
    enum Event {
        None = 0,
        Read = 1,
        Write = 2,
        Exceptional = 4,
    };

    virtual ~Notifier() override;

    void set_enabled(bool);

    Function<void()> on_ready_to_read;
    Function<void()> on_ready_to_write;

    void close();
#if !defined(AK_OS_WINDOWS)
    int fd() const { return m_fd; }
#else
    HANDLE handle() const { return m_handle; }
#endif
    unsigned event_mask() const { return m_event_mask; }
    void set_event_mask(unsigned event_mask) { m_event_mask = event_mask; }

    void event(Core::Event&) override;

private:
#if !defined(AK_OS_WINDOWS)
    Notifier(int fd, unsigned event_mask, Object* parent = nullptr);
    int m_fd { -1 };
#else
    Notifier(HANDLE handle, unsigned event_mask, Object* parent = nullptr);
    HANDLE m_handle { INVALID_HANDLE_VALUE };
#endif
    unsigned m_event_mask { 0 };
};

}
