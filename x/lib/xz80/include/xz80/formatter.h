// formatter.h — reusable Z80 syntax formatting helpers.
//
// Provides a structured fragment model for assembly rendering so callers can
// emit plain text today and richer styled output later.
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
#pragma once

#include <string>
#include <vector>

namespace xz80 {

enum class syntax_style {
    native,
    sdcc,
    gnu
};

enum class fragment_kind {
    text,
    whitespace,
    mnemonic,
    directive,
    register_name,
    value,
    delimiter,
    comment,
    label,
    string_literal,
    symbol,
    operator_token
};

struct fragment {
    fragment_kind kind = fragment_kind::text;
    std::string   text;
};

using formatted_line = std::vector<fragment>;
using formatted_document = std::vector<formatted_line>;

formatted_line format_instruction(syntax_style style,
                                  const std::string& mnemonic,
                                  const std::vector<std::string>& operands);

std::string render_line(const formatted_line& line);
std::string render_document(const formatted_document& document);

} // namespace xz80
