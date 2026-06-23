// formatter.cpp — reusable Z80 syntax formatting helpers.
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
//
#include <algorithm>
#include <cctype>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <xz80/formatter.h>

namespace xz80 {

namespace {

std::string to_lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
        [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return value;
}

bool iequals(std::string_view lhs, std::string_view rhs) {
    if (lhs.size() != rhs.size())
        return false;
    for (size_t i = 0; i < lhs.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(lhs[i]))
            != std::tolower(static_cast<unsigned char>(rhs[i]))) {
            return false;
        }
    }
    return true;
}

bool is_register_name(std::string_view value) {
    static constexpr std::string_view kRegisters[] = {
        "A", "B", "C", "D", "E", "H", "L", "F",
        "BC", "DE", "HL", "SP", "AF", "AF'",
        "IX", "IY", "IXH", "IXL", "IYH", "IYL",
        "I", "R",
        "NZ", "Z", "NC", "C", "PO", "PE", "P", "M"
    };
    for (auto reg : kRegisters) {
        if (iequals(value, reg))
            return true;
    }
    return false;
}

bool is_numberish(std::string_view value) {
    if (value.empty())
        return false;
    size_t start = 0;
    if (value[0] == '+' || value[0] == '-')
        start = 1;
    if (start >= value.size())
        return false;
    if (value.substr(start, 2) == "0x" || value.substr(start, 2) == "0X") {
        start += 2;
        if (start >= value.size())
            return false;
        return std::all_of(value.begin() + static_cast<long>(start), value.end(),
            [](unsigned char ch) { return std::isxdigit(ch) != 0; });
    }
    return std::all_of(value.begin() + static_cast<long>(start), value.end(),
        [](unsigned char ch) { return std::isalnum(ch) != 0 || ch == '_'; });
}

void append(formatted_line& out, fragment_kind kind, std::string text) {
    if (text.empty())
        return;
    out.push_back(fragment{kind, std::move(text)});
}

void append_expression_fragments(formatted_line& out, std::string_view text) {
    size_t pos = 0;
    while (pos < text.size()) {
        const char ch = text[pos];
        if (std::isspace(static_cast<unsigned char>(ch))) {
            const size_t start = pos;
            while (pos < text.size() && std::isspace(static_cast<unsigned char>(text[pos])))
                ++pos;
            append(out, fragment_kind::whitespace,
                   std::string(text.substr(start, pos - start)));
            continue;
        }

        if ((ch == '<' || ch == '>') && pos + 1 < text.size() && text[pos + 1] == ch) {
            append(out, fragment_kind::delimiter, std::string(text.substr(pos, 2)));
            pos += 2;
            continue;
        }

        if (std::string_view("(),+-*/%&|^#").find(ch) != std::string_view::npos) {
            append(out, fragment_kind::delimiter, std::string(1, ch));
            ++pos;
            continue;
        }

        const size_t start = pos;
        while (pos < text.size()) {
            const char cur = text[pos];
            if (std::isspace(static_cast<unsigned char>(cur)))
                break;
            if (std::string_view("(),+-*/%&|^#").find(cur) != std::string_view::npos)
                break;
            if ((cur == '<' || cur == '>') && pos + 1 < text.size() && text[pos + 1] == cur)
                break;
            ++pos;
        }

        std::string atom(text.substr(start, pos - start));
        if (is_register_name(atom)) {
            append(out, fragment_kind::register_name, to_lower(atom));
        } else if (is_numberish(atom)) {
            append(out, fragment_kind::value, atom);
        } else {
            append(out, fragment_kind::symbol, atom);
        }
    }
}

formatted_line format_indexed_operand(syntax_style style, std::string operand) {
    static const std::regex pattern(R"(\((IX|IY)([+-].+)\))",
                                    std::regex::icase);
    std::smatch match;
    formatted_line out;
    if (!std::regex_match(operand, match, pattern))
        return out;

    const std::string reg = to_lower(match[1].str());
    std::string expr = match[2].str();

    if (style == syntax_style::sdcc) {
        if (!expr.empty() && expr[0] == '+')
            expr.erase(expr.begin());
        append_expression_fragments(out, expr);
        append(out, fragment_kind::delimiter, "(");
        append(out, fragment_kind::register_name, reg);
        append(out, fragment_kind::delimiter, ")");
        return out;
    }

    append(out, fragment_kind::delimiter, "(");
    append(out, fragment_kind::register_name, reg);
    append_expression_fragments(out, expr);
    append(out, fragment_kind::delimiter, ")");
    return out;
}

formatted_line format_direct_operand(syntax_style style, std::string operand) {
    static const std::regex pattern(R"(\(#(.+)\))",
                                    std::regex::icase);
    std::smatch match;
    formatted_line out;
    if (!std::regex_match(operand, match, pattern))
        return out;

    append(out, fragment_kind::delimiter, "(");
    if (style == syntax_style::sdcc)
        append(out, fragment_kind::delimiter, "#");
    append_expression_fragments(out, match[1].str());
    append(out, fragment_kind::delimiter, ")");
    return out;
}

formatted_line format_immediate_operand(syntax_style style, std::string operand) {
    formatted_line out;
    if (operand.empty() || operand[0] != '#')
        return out;

    if (style == syntax_style::sdcc)
        append(out, fragment_kind::delimiter, "#");
    append_expression_fragments(out, operand.substr(1));
    return out;
}

formatted_line format_operand(syntax_style style, const std::string& operand) {
    if (auto indexed = format_indexed_operand(style, operand); !indexed.empty())
        return indexed;
    if (auto direct = format_direct_operand(style, operand); !direct.empty())
        return direct;
    if (auto immediate = format_immediate_operand(style, operand); !immediate.empty())
        return immediate;

    formatted_line out;
    append_expression_fragments(out, operand);
    return out;
}

} // namespace

formatted_line format_instruction(syntax_style style,
                                  const std::string& mnemonic,
                                  const std::vector<std::string>& operands) {
    formatted_line out;
    append(out, fragment_kind::mnemonic, to_lower(mnemonic));
    if (!operands.empty()) {
        append(out, fragment_kind::whitespace, "\t");
        for (size_t i = 0; i < operands.size(); ++i) {
            auto operand_fragments = format_operand(style, operands[i]);
            out.insert(out.end(),
                       std::make_move_iterator(operand_fragments.begin()),
                       std::make_move_iterator(operand_fragments.end()));
            if (i + 1 != operands.size()) {
                append(out, fragment_kind::delimiter, ",");
                append(out, fragment_kind::whitespace, " ");
            }
        }
    }
    return out;
}

std::string render_line(const formatted_line& line) {
    std::string out;
    for (const auto& fragment : line)
        out += fragment.text;
    return out;
}

std::string render_document(const formatted_document& document) {
    std::ostringstream out;
    for (const auto& line : document)
        out << render_line(line) << "\n";
    return out.str();
}

} // namespace xz80
