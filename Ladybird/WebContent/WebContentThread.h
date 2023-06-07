/*
 * Copyright (c) 2022, Andrew Kaster <akaster@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <QThread>
#include <Windows.h>

class WebContentThread : public QThread {
    Q_OBJECT
public:
    WebContentThread(QObject* parent, HANDLE read_pipe, HANDLE write_pipe, HANDLE read_passing_pipe, HANDLE write_passing_pipe)
        : QThread(parent)
        , m_read_pipe(read_pipe)
        , m_write_pipe(write_pipe)
        , m_read_passing_pipe(read_passing_pipe)
        , m_write_passing_pipe(write_passing_pipe)
    {
    }
    virtual ~WebContentThread() override = default;

    // Expose exec() to web_content_main
    int exec_event_loop() { return exec(); }

private:
    HANDLE m_read_pipe;
    HANDLE m_write_pipe;
    HANDLE m_read_passing_pipe;
    HANDLE m_write_passing_pipe;

protected:
    void run() override;
};
