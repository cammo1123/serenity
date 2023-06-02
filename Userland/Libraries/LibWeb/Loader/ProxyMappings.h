/*
 * Copyright (c) 2022, Ali Mohammad Pur <mpfard@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/HashMap.h>
#include <AK/URL.h>
#include <AK/Vector.h>
#include <LibCore/Proxy.h>
#include <LibWeb/Forward.h>

namespace Web {

class LibWeb_API ProxyMappings {
public:
    static ProxyMappings& the();

    Core::ProxyData proxy_for_url(AK::URL const&) const;
    void set_mappings(Vector<DeprecatedString> proxies, OrderedHashMap<DeprecatedString, size_t> mappings);

private:
    ProxyMappings() = default;
    ~ProxyMappings() = default;

    Vector<DeprecatedString> m_proxies;
    OrderedHashMap<DeprecatedString, size_t> m_mappings;
};

}
