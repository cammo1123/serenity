/*
 * Copyright (c) 2021, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/String.h>
#include <AK/URL.h>
#include <AK/Vector.h>
#include <LibWeb/Forward.h>

namespace Web {

class LibWeb_API ContentFilter {
public:
    static ContentFilter& the();

    bool is_filtered(const AK::URL&) const;
    ErrorOr<void> set_patterns(ReadonlySpan<String>);

private:
    ContentFilter();
    ~ContentFilter();

    struct Pattern {
        String text;
    };
    Vector<Pattern> m_patterns;
};

}
