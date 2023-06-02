/*
 * Copyright (c) 2022, Filiph Sandström <filiph.sandstrom@filfatstudios.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Settings.h"

namespace Browser {

static Settings* s_the;

Settings& Settings::the() {
    if (!s_the)
        s_the = new Settings();
    return *s_the;
}

Settings::Settings()
{
    s_the = this;
    m_qsettings = new QSettings("Serenity", "Ladybird", this);
}

QString Settings::new_tab_page()
{
    return m_qsettings->value("new_tab_page", "about:blank").toString();
}

void Settings::set_new_tab_page(QString const& page)
{
    m_qsettings->setValue("new_tab_page", page);
}

}
