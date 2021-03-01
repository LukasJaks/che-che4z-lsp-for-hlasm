/*
 * Copyright (c) 2019 Broadcom.
 * The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 *
 * Contributors:
 *   Broadcom, Inc. - initial API and implementation
 */

#include "text_data_ref_t.h"

#include "workspaces/file_impl.h"

namespace hlasm_plugin::parser_library::lsp {

text_data_ref_t::text_data_ref_t()
    : text(empty_text)
{}

text_data_ref_t::text_data_ref_t(const std::string& text)
    : text(text)
    , line_indices(workspaces::file_impl::create_line_indices(text))
{}

std::string_view text_data_ref_t::get_line(size_t line) const
{
    if (line >= line_indices.size())
        return "";
    size_t line_end_i = (line < line_indices.size() - 1) ? line_indices[line + 1] : text.size();
    size_t line_len = line_end_i - line_indices[line];
    return { &text[line_indices[line]], line_len };
}

std::string_view text_data_ref_t::get_line_beginning(position pos) const
{
    if (pos.line >= line_indices.size())
        return "";
    size_t line_end_i = workspaces::file_impl::index_from_position(text, line_indices, pos);
    size_t line_len = line_end_i - line_indices[pos.line];
    return { &text[line_indices[pos.line]], line_len };
}



char text_data_ref_t::get_character_before(position pos) const
{
    if (pos.column == 0)
        return '\0';
    size_t index = workspaces::file_impl::index_from_position(text, line_indices, { pos.line, pos.column - 1 });
    return text[index];
}

std::string text_data_ref_t::empty_text;

} // namespace hlasm_plugin::parser_library::lsp
