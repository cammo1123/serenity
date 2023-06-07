/*
 * Copyright (c) 2022, Andrew Kaster <akaster@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "WebContentThread.h"
#include <AK/Error.h>
#include <AK/Format.h>

extern ErrorOr<int> web_content_main(WebContentThread* context, HANDLE read_pipe, HANDLE write_pipe, HANDLE read_passing_pipe, HANDLE write_passing_pipe);

void WebContentThread::run()
{
    auto err = web_content_main(this, m_read_pipe, m_write_pipe, m_read_passing_pipe, m_write_passing_pipe);
    if (err.is_error()) {
        warnln("WebContent failed with error: {}", err.error());
        exit(err.error().code());
    } else {
        exit(err.value());
    }
}
