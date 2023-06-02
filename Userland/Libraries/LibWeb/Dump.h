/*
 * Copyright (c) 2018-2021, Andreas Kling <kling@serenityos.org>
 * Copyright (c) 2021, the SerenityOS developers.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Forward.h>
#include <LibWeb/Forward.h>

namespace Web {

LibWeb_API void dump_tree(StringBuilder&, DOM::Node const&);
LibWeb_API void dump_tree(DOM::Node const&);
LibWeb_API void dump_tree(StringBuilder&, Layout::Node const&, bool show_box_model = false, bool show_specified_style = false, bool colorize = false);
LibWeb_API void dump_tree(Layout::Node const&, bool show_box_model = true, bool show_specified_style = false);
LibWeb_API void dump_tree(StringBuilder&, Painting::Paintable const&, bool colorize = false, int indent = 0);
LibWeb_API void dump_tree(Painting::Paintable const&);
LibWeb_API ErrorOr<void> dump_sheet(StringBuilder&, CSS::StyleSheet const&);
LibWeb_API ErrorOr<void> dump_sheet(CSS::StyleSheet const&);
LibWeb_API ErrorOr<void> dump_rule(StringBuilder&, CSS::CSSRule const&, int indent_levels = 0);
LibWeb_API ErrorOr<void> dump_rule(CSS::CSSRule const&);
LibWeb_API void dump_font_face_rule(StringBuilder&, CSS::CSSFontFaceRule const&, int indent_levels = 0);
LibWeb_API void dump_import_rule(StringBuilder&, CSS::CSSImportRule const&, int indent_levels = 0);
LibWeb_API ErrorOr<void> dump_media_rule(StringBuilder&, CSS::CSSMediaRule const&, int indent_levels = 0);
LibWeb_API ErrorOr<void> dump_style_rule(StringBuilder&, CSS::CSSStyleRule const&, int indent_levels = 0);
LibWeb_API ErrorOr<void> dump_supports_rule(StringBuilder&, CSS::CSSSupportsRule const&, int indent_levels = 0);
LibWeb_API void dump_selector(StringBuilder&, CSS::Selector const&);
LibWeb_API void dump_selector(CSS::Selector const&);

}
