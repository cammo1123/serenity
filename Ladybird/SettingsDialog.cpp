/*
 * Copyright (c) 2022, Filiph Sandström <filiph.sandstrom@filfatstudios.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "SettingsDialog.h"
#include "Ladybird/Settings.h"
#include "Settings.h"
#include <QCloseEvent>
#include <QLabel>

SettingsDialog::SettingsDialog(QMainWindow* window)
    : m_window(window)
{
    m_layout = new QFormLayout(this);
    m_new_tab_page = new QLineEdit(this);
    m_ok_button = new QPushButton("&Save", this);

    m_layout->addRow(new QLabel("Page on New Tab", this), m_new_tab_page);
    m_layout->addWidget(m_ok_button);

    QObject::connect(m_ok_button, &QPushButton::released, this, [this] {
        close();
    });

    setWindowTitle("Settings");
    setFixedWidth(300);
    setFixedHeight(150);
    setLayout(m_layout);
    show();
    setFocus();
}

void SettingsDialog::closeEvent(QCloseEvent* event)
{
    save();
    event->accept();
}

void SettingsDialog::save()
{
    // FIXME: Validate data.
    Browser::Settings::the().set_new_tab_page(m_new_tab_page->text());
}
