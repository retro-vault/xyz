// source_emitter.cpp
//
// Emits formatted assembly source from the common xas AST.
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
//
#include <algorithm>
#include <cctype>
#include <map>
#include <memory>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include <xas/backend/source_emitter.h>

namespace xas {

namespace {

using fragment_kind = xz80::fragment_kind;
using formatted_line = xz80::formatted_line;
using formatted_document = xz80::formatted_document;

void append(formatted_line& out, fragment_kind kind, std::string text) {
    if (text.empty())
        return;
    out.push_back(xz80::fragment{kind, std::move(text)});
}

std::string to_lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
        [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return value;
}

int precedence(const expr& e) {
    switch (e.kind) {
    case expr_kind::binary:
        switch (e.op) {
        case '|': return 1;
        case '^': return 2;
        case '&': return 3;
        case '<':
        case '>': return 4;
        case '+':
        case '-': return 5;
        case '*':
        case '/':
        case '%': return 6;
        default:  return 7;
        }
    case expr_kind::unary:
        return 7;
    default:
        return 8;
    }
}

std::string expr_to_text_impl(const expr& e, int parent_prec) {
    switch (e.kind) {
    case expr_kind::integer:
        return std::to_string(e.int_val);
    case expr_kind::symbol:
        return e.name;
    case expr_kind::current_addr:
        return "$";
    case expr_kind::unary: {
        std::string text(1, e.op);
        text += expr_to_text_impl(*e.lhs, precedence(e));
        if (precedence(e) < parent_prec)
            return "(" + text + ")";
        return text;
    }
    case expr_kind::binary: {
        const int my_prec = precedence(e);
        std::string op = (e.op == '<') ? "<<" : (e.op == '>') ? ">>"
                                                         : std::string(1, e.op);
        std::string text = expr_to_text_impl(*e.lhs, my_prec)
                         + " " + op + " "
                         + expr_to_text_impl(*e.rhs, my_prec + 1);
        if (my_prec < parent_prec)
            return "(" + text + ")";
        return text;
    }
    }
    return "";
}

std::string expr_to_text(const expr& e) {
    return expr_to_text_impl(e, 0);
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
        [](unsigned char ch) { return std::isdigit(ch) != 0; });
}

void append_expr_fragments(formatted_line& out, std::string_view text) {
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
            append(out, fragment_kind::operator_token, std::string(text.substr(pos, 2)));
            pos += 2;
            continue;
        }

        if (std::string_view("(),").find(ch) != std::string_view::npos) {
            append(out, fragment_kind::delimiter, std::string(1, ch));
            ++pos;
            continue;
        }

        if (std::string_view("+-*/%&|^").find(ch) != std::string_view::npos) {
            append(out, fragment_kind::operator_token, std::string(1, ch));
            ++pos;
            continue;
        }

        const size_t start = pos;
        while (pos < text.size()) {
            const char cur = text[pos];
            if (std::isspace(static_cast<unsigned char>(cur)))
                break;
            if (std::string_view("(),+-*/%&|^").find(cur) != std::string_view::npos)
                break;
            if ((cur == '<' || cur == '>') && pos + 1 < text.size() && text[pos + 1] == cur)
                break;
            ++pos;
        }

        std::string atom(text.substr(start, pos - start));
        append(out, is_numberish(atom) ? fragment_kind::value : fragment_kind::symbol,
               std::move(atom));
    }
}

std::string map_section_to_gnu(std::string name) {
    if (name == "_CODE")  return ".text";
    if (name == "_DATA")  return ".data";
    if (name == "_CONST") return ".rodata";
    if (name == "_BSS")   return ".bss";
    if (name == "_TLS")   return ".tdata";
    return name;
}

std::string map_section_to_sdcc(std::string name) {
    if (name == ".text")   return "_CODE";
    if (name == ".data")   return "_DATA";
    if (name == ".rodata") return "_CONST";
    if (name == ".bss")    return "_BSS";
    if (name == ".tdata")  return "_TLS";
    if (!name.empty() && name.front() == '.')
        return name.substr(1);
    return name;
}

std::string quoted(std::string_view text) {
    std::string out;
    out += '"';
    for (char ch : text) {
        switch (ch) {
        case '\\': out += "\\\\"; break;
        case '"':  out += "\\\""; break;
        case '\n': out += "\\n"; break;
        case '\t': out += "\\t"; break;
        case '\r': out += "\\r"; break;
        case '\0': out += "\\0"; break;
        default:   out += ch; break;
        }
    }
    out += '"';
    return out;
}

void append_comment(formatted_line& line, const std::string& comment) {
    if (comment.empty())
        return;
    if (!line.empty())
        append(line, fragment_kind::whitespace, " ");
    append(line, fragment_kind::comment, "; " + comment);
}

bool immediate_prefix_for_instruction(std::string_view mnemonic, size_t index) {
    if (mnemonic == "JP" || mnemonic == "JR" || mnemonic == "CALL"
        || mnemonic == "DJNZ" || mnemonic == "RST" || mnemonic == "IM") {
        return false;
    }
    if ((mnemonic == "BIT" || mnemonic == "SET" || mnemonic == "RES") && index == 0)
        return false;
    return true;
}

std::string canonical_operand(std::string_view mnemonic,
                              size_t index,
                              const operand& op) {
    switch (op.kind) {
    case operand_kind::reg:
        return op.reg_name;
    case operand_kind::cond:
        return op.cond_name;
    case operand_kind::imm:
        if (immediate_prefix_for_instruction(mnemonic, index))
            return "#" + expr_to_text(*op.value);
        return expr_to_text(*op.value);
    case operand_kind::ind_reg:
        return "(" + op.reg_name + ")";
    case operand_kind::ind_expr:
        return "(#" + expr_to_text(*op.value) + ")";
    case operand_kind::ind_ix_off:
    case operand_kind::ind_iy_off: {
        std::string expr_text = expr_to_text(*op.value);
        if (expr_text == "0")
            return "(" + op.reg_name + ")";
        if (expr_text.empty() || expr_text.front() != '-')
            expr_text = "+" + expr_text;
        return "(" + op.reg_name + expr_text + ")";
    }
    }
    return "";
}

bool needs_explicit_accumulator(std::string_view mnemonic, size_t operand_count) {
    if (operand_count != 1)
        return false;
    return mnemonic == "ADD"
        || mnemonic == "ADC"
        || mnemonic == "SBC"
        || mnemonic == "SUB"
        || mnemonic == "AND"
        || mnemonic == "XOR"
        || mnemonic == "OR"
        || mnemonic == "CP";
}

bool is_gnu_shorthand_section(const std::string& section) {
    return section == ".text"
        || section == ".data"
        || section == ".bss";
}

std::string lowercase_copy(std::string value) {
    for (char& ch : value)
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    return value;
}

std::string gnu_section_flags(const std::string& section) {
    const std::string lower = lowercase_copy(section);
    const bool is_bss_like =
        lower == ".bss" || lower == "_bss" || lower == "_heap";
    const bool is_data_like =
        lower == ".data" || lower == ".tdata"
        || lower == "_data" || lower == "_initialized"
        || lower == "_initializer" || lower == "_dabs";
    const bool is_rodata_like =
        lower == ".rodata" || lower == ".rdata"
        || lower == "_rodata" || lower == "_const";

    if (is_bss_like)
        return "\"aw\",@nobits";
    if (is_data_like)
        return "\"aw\"";
    if (is_rodata_like)
        return "\"a\"";
    return "\"ax\"";
}

void append_expr_arg(formatted_line& line, const expr_ptr& arg) {
    append_expr_fragments(line, expr_to_text(*arg));
}

void append_directive_name(formatted_line& line, std::string name) {
    append(line, fragment_kind::directive, std::move(name));
}

void append_comma(formatted_line& line) {
    append(line, fragment_kind::delimiter, ",");
    append(line, fragment_kind::whitespace, " ");
}

class base_source_emitter : public source_emitter {
public:
    explicit base_source_emitter(output_format format) : format_(format) {}

    formatted_document emit(const stmt_list& stmts) const override {
        analyse_sections(stmts);
        formatted_document document;
        document.reserve(stmts.size() + gnu_section_break_before_.size());
        bool emitted_section = false;
        std::string current_output_section;
        for (size_t i = 0; i < stmts.size(); ++i) {
            if (format_ == output_format::gnu) {
                if (auto it = gnu_section_break_before_.find(i);
                    it != gnu_section_break_before_.end()) {
                    std::string mapped = map_section_to_gnu(it->second);
                    if (auto ov = gnu_section_overrides_.find(it->second);
                        ov != gnu_section_overrides_.end()) {
                        mapped = ov->second;
                    }
                    if (!emitted_section || current_output_section != mapped) {
                        document.push_back(emit_synthetic_section_stmt(it->second));
                        emitted_section = true;
                        current_output_section = mapped;
                    }
                }

                std::string section_name;
                if (is_section_directive(stmts[i], &section_name)) {
                    std::string mapped = map_section_to_gnu(section_name);
                    if (auto ov = gnu_section_overrides_.find(section_name);
                        ov != gnu_section_overrides_.end()) {
                        mapped = ov->second;
                    }
                    if (emitted_section && current_output_section == mapped)
                        continue;
                    document.push_back(emit_stmt(stmts[i]));
                    emitted_section = true;
                    current_output_section = mapped;
                    continue;
                }

                if (!emitted_section) {
                    const auto& stmt = stmts[i];
                    const bool needs_default_text =
                        stmt.kind == stmt_kind::instruction
                        || stmt.kind == stmt_kind::label
                        || (stmt.kind == stmt_kind::directive
                            && (stmt.directive_name == "globl"
                                || stmt.directive_name == "global"));
                    if (needs_default_text) {
                        document.push_back(emit_synthetic_section_stmt("_CODE"));
                        emitted_section = true;
                        current_output_section = ".text";
                    }
                }
            }
            document.push_back(emit_stmt(stmts[i]));
        }
        return document;
    }

protected:
    virtual formatted_line emit_stmt(const stmt& stmt) const {
        switch (stmt.kind) {
        case stmt_kind::comment:     return emit_comment_stmt(stmt);
        case stmt_kind::label:       return emit_label_stmt(stmt);
        case stmt_kind::instruction: return emit_instruction_stmt(stmt);
        case stmt_kind::directive:   return emit_directive_stmt(stmt);
        case stmt_kind::equ:         return emit_equ_stmt(stmt);
        }
        return {};
    }

    formatted_line emit_comment_stmt(const stmt& stmt) const {
        formatted_line line;
        append(line, fragment_kind::comment, "; " + stmt.comment_text);
        return line;
    }

    formatted_line emit_label_stmt(const stmt& stmt) const {
        formatted_line line;
        append(line, fragment_kind::label, stmt.label_name);
        append(line, fragment_kind::delimiter, ":");
        append_comment(line, stmt.trailing_comment);
        return line;
    }

    formatted_line emit_instruction_stmt(const stmt& stmt) const {
        std::vector<std::string> operands;
        operands.reserve(stmt.operands.size()
                         + ((format_ == output_format::gnu
                             && needs_explicit_accumulator(stmt.mnemonic, stmt.operands.size()))
                            ? 1u
                            : 0u));
        if (format_ == output_format::gnu
            && needs_explicit_accumulator(stmt.mnemonic, stmt.operands.size())) {
            operands.push_back("A");
        }
        for (size_t i = 0; i < stmt.operands.size(); ++i)
            operands.push_back(canonical_operand(stmt.mnemonic, i, stmt.operands[i]));

        const auto style = (format_ == output_format::sdcc)
            ? xz80::syntax_style::sdcc
            : xz80::syntax_style::gnu;
        auto line = xz80::format_instruction(style, stmt.mnemonic, operands);
        append_comment(line, stmt.trailing_comment);
        return line;
    }

    formatted_line emit_equ_stmt(const stmt& stmt) const {
        formatted_line line;
        if (format_ == output_format::sdcc) {
            append(line, fragment_kind::symbol, stmt.equ_name);
            append(line, fragment_kind::whitespace, " ");
            append_directive_name(line, ".equ");
            append(line, fragment_kind::whitespace, " ");
            append_expr_fragments(line, expr_to_text(*stmt.equ_value));
        } else {
            append_directive_name(line, ".equ");
            append(line, fragment_kind::whitespace, " ");
            append(line, fragment_kind::symbol, stmt.equ_name);
            append_comma(line);
            append_expr_fragments(line, expr_to_text(*stmt.equ_value));
        }
        append_comment(line, stmt.trailing_comment);
        return line;
    }

    formatted_line emit_directive_stmt(const stmt& stmt) const {
        formatted_line line;
        const auto& name = stmt.directive_name;

        if (name == "area" || name == "section"
            || name == "text" || name == "data"
            || name == "rodata" || name == "bss"
            || name == "tdata") {
            std::string section_name = stmt.string_arg;
            if (name == "text" || name == "data"
                || name == "rodata" || name == "bss"
                || name == "tdata") {
                section_name = "." + name;
            }
            if (format_ == output_format::sdcc) {
                append_directive_name(line, ".area");
                if (!section_name.empty()) {
                    append(line, fragment_kind::whitespace, " ");
                    append(line, fragment_kind::symbol, map_section_to_sdcc(section_name));
                }
            } else {
                std::string section = map_section_to_gnu(section_name);
                if (auto it = gnu_section_overrides_.find(section_name);
                    it != gnu_section_overrides_.end()) {
                    section = it->second;
                }
                if (is_gnu_shorthand_section(section)) {
                    append_directive_name(line, section);
                } else {
                    append_directive_name(line, ".section");
                    if (!section.empty()) {
                        append(line, fragment_kind::whitespace, " ");
                        append(line, fragment_kind::symbol, section);
                        append_comma(line);
                        append(line, fragment_kind::string_literal,
                               gnu_section_flags(section));
                    }
                }
            }
            append_comment(line, stmt.trailing_comment);
            return line;
        }

        if (name == "globl" || name == "global"
            || name == "extern" || name == "external"
            || name == "ref" || name == "xref") {
            append_directive_name(line, format_ == output_format::sdcc ? ".globl" : ".global");
            if (!stmt.args.empty())
                append(line, fragment_kind::whitespace, " ");
            for (size_t i = 0; i < stmt.args.size(); ++i) {
                append(line, fragment_kind::symbol, stmt.args[i]->name);
                if (i + 1 != stmt.args.size())
                    append_comma(line);
            }
            append_comment(line, stmt.trailing_comment);
            return line;
        }

        if (name == "module" || name == "file") {
            append_directive_name(line, format_ == output_format::sdcc ? ".module" : ".file");
            if (!stmt.string_arg.empty()) {
                append(line, fragment_kind::whitespace, " ");
                if (format_ == output_format::gnu)
                    append(line, fragment_kind::string_literal, quoted(stmt.string_arg));
                else
                    append(line, fragment_kind::symbol, stmt.string_arg);
            }
            append_comment(line, stmt.trailing_comment);
            return line;
        }

        if (name == "include") {
            append_directive_name(line, ".include");
            if (!stmt.string_arg.empty()) {
                append(line, fragment_kind::whitespace, " ");
                append(line, fragment_kind::string_literal, quoted(stmt.string_arg));
            }
            append_comment(line, stmt.trailing_comment);
            return line;
        }

        if (name == "equ" || name == "set") {
            append_directive_name(line, format_ == output_format::gnu ? ".equ" : ".equ");
            if (!stmt.string_arg.empty()) {
                append(line, fragment_kind::whitespace, " ");
                append(line, fragment_kind::symbol, stmt.string_arg);
            }
            if (!stmt.args.empty()) {
                append_comma(line);
                append_expr_arg(line, stmt.args.front());
            }
            append_comment(line, stmt.trailing_comment);
            return line;
        }

        if (name == "define") {
            if (format_ == output_format::gnu) {
                append_directive_name(line, ".set");
                append(line, fragment_kind::whitespace, " ");
                append(line, fragment_kind::symbol, stmt.string_arg);
                append_comma(line);
                append_expr_arg(line, stmt.args.front());
            } else {
                append_directive_name(line, ".define");
                append(line, fragment_kind::whitespace, " ");
                append(line, fragment_kind::symbol, stmt.string_arg);
                if (!stmt.args.empty()) {
                    append(line, fragment_kind::whitespace, " ");
                    append(line, fragment_kind::delimiter, "=");
                    append(line, fragment_kind::whitespace, " ");
                    append_expr_arg(line, stmt.args.front());
                }
            }
            append_comment(line, stmt.trailing_comment);
            return line;
        }

        if (name == "org" || name == "origin") {
            append_directive_name(line, ".org");
            if (!stmt.args.empty()) {
                append(line, fragment_kind::whitespace, " ");
                append_expr_arg(line, stmt.args.front());
            }
            append_comment(line, stmt.trailing_comment);
            return line;
        }

        if (name == "byte" || name == "db") {
            append_directive_name(line, format_ == output_format::sdcc ? ".db" : ".byte");
            bool need_sep = false;
            if (!stmt.string_arg.empty()) {
                append(line, fragment_kind::whitespace, " ");
                append(line, fragment_kind::string_literal, quoted(stmt.string_arg));
                need_sep = true;
            }
            for (const auto& arg : stmt.args) {
                if (!need_sep)
                    append(line, fragment_kind::whitespace, " ");
                else
                    append_comma(line);
                append_expr_arg(line, arg);
                need_sep = true;
            }
            append_comment(line, stmt.trailing_comment);
            return line;
        }

        if (name == "word" || name == "dw" || name == "2byte") {
            append_directive_name(line, format_ == output_format::sdcc ? ".dw" : ".word");
            if (!stmt.args.empty())
                append(line, fragment_kind::whitespace, " ");
            for (size_t i = 0; i < stmt.args.size(); ++i) {
                append_expr_arg(line, stmt.args[i]);
                if (i + 1 != stmt.args.size())
                    append_comma(line);
            }
            append_comment(line, stmt.trailing_comment);
            return line;
        }

        if (name == "dl" || name == "long") {
            append_directive_name(line, format_ == output_format::sdcc ? ".dl" : ".long");
            if (!stmt.args.empty())
                append(line, fragment_kind::whitespace, " ");
            for (size_t i = 0; i < stmt.args.size(); ++i) {
                append_expr_arg(line, stmt.args[i]);
                if (i + 1 != stmt.args.size())
                    append_comma(line);
            }
            append_comment(line, stmt.trailing_comment);
            return line;
        }

        if (name == "ds" || name == "space" || name == "blkb") {
            append_directive_name(line, format_ == output_format::sdcc ? ".ds" : ".space");
            if (!stmt.args.empty()) {
                append(line, fragment_kind::whitespace, " ");
                append_expr_arg(line, stmt.args.front());
                if (format_ == output_format::gnu && stmt.args.size() >= 2) {
                    append_comma(line);
                    append_expr_arg(line, stmt.args[1]);
                }
            }
            append_comment(line, stmt.trailing_comment);
            return line;
        }

        if (name == "blkw") {
            append_directive_name(line, format_ == output_format::sdcc ? ".blkw" : ".space");
            if (!stmt.args.empty()) {
                append(line, fragment_kind::whitespace, " ");
                if (format_ == output_format::sdcc) {
                    append_expr_arg(line, stmt.args.front());
                } else {
                    append(line, fragment_kind::delimiter, "(");
                    append_expr_arg(line, stmt.args.front());
                    append(line, fragment_kind::whitespace, " ");
                    append(line, fragment_kind::operator_token, "*");
                    append(line, fragment_kind::whitespace, " ");
                    append(line, fragment_kind::value, "2");
                    append(line, fragment_kind::delimiter, ")");
                }
            }
            append_comment(line, stmt.trailing_comment);
            return line;
        }

        if (name == "ascii" || name == "asciz" || name == "string") {
            const std::string directive =
                (name == "ascii") ? ".ascii" : ".asciz";
            append_directive_name(line, directive);
            append(line, fragment_kind::whitespace, " ");

            std::string payload = stmt.string_arg;
            if ((name == "asciz" || name == "string") && !payload.empty() && payload.back() == '\0')
                payload.pop_back();
            append(line, fragment_kind::string_literal, quoted(payload));
            append_comment(line, stmt.trailing_comment);
            return line;
        }

        if (name == "align" || name == "balign" || name == "p2align") {
            append_directive_name(line, name == "p2align" ? ".p2align" : ".align");
            if (!stmt.args.empty()) {
                append(line, fragment_kind::whitespace, " ");
                for (size_t i = 0; i < stmt.args.size(); ++i) {
                    if (i != 0)
                        append_comma(line);
                    append_expr_arg(line, stmt.args[i]);
                }
            }
            append_comment(line, stmt.trailing_comment);
            return line;
        }

        if (name == "type") {
            append_directive_name(line, ".type");
            if (!stmt.string_arg.empty()) {
                append(line, fragment_kind::whitespace, " ");
                append(line, fragment_kind::symbol, stmt.string_arg);
            }
            if (!stmt.string_arg2.empty()) {
                append_comma(line);
                append(line, fragment_kind::value, stmt.string_arg2);
            }
            append_comment(line, stmt.trailing_comment);
            return line;
        }

        if (name == "size") {
            append_directive_name(line, ".size");
            if (!stmt.string_arg.empty()) {
                append(line, fragment_kind::whitespace, " ");
                append(line, fragment_kind::symbol, stmt.string_arg);
            }
            if (!stmt.args.empty()) {
                append_comma(line);
                append_expr_arg(line, stmt.args.front());
            }
            append_comment(line, stmt.trailing_comment);
            return line;
        }

        if (name == "if" || name == "ifdef" || name == "ifndef"
            || name == "else" || name == "endif"
            || name == "optsdcc" || name == "24bit" || name == "32bit"
            || name == "end" || name == "title" || name == "sbttl"
            || name == "ident" || name == "list" || name == "nlist"
            || name == "page" || name == "local" || name == "type"
            || name == "size") {
            append_directive_name(line, "." + name);
            if (!stmt.args.empty()) {
                append(line, fragment_kind::whitespace, " ");
                append_expr_arg(line, stmt.args.front());
            } else if (!stmt.string_arg2.empty()) {
                append(line, fragment_kind::whitespace, " ");
                append(line, fragment_kind::value, stmt.string_arg2);
            }
            append_comment(line, stmt.trailing_comment);
            return line;
        }

        append_directive_name(line, "." + to_lower(name));
        if (!stmt.string_arg.empty()) {
            append(line, fragment_kind::whitespace, " ");
            append(line, fragment_kind::value, stmt.string_arg);
        }
        for (const auto& arg : stmt.args) {
            append(line, fragment_kind::whitespace, " ");
            append_expr_arg(line, arg);
        }
        append_comment(line, stmt.trailing_comment);
        return line;
    }

    formatted_line emit_synthetic_section_stmt(const std::string& section_name) const {
        stmt synthetic;
        synthetic.kind = stmt_kind::directive;
        synthetic.directive_name = "section";
        synthetic.string_arg = section_name;
        return emit_directive_stmt(synthetic);
    }

private:
    struct section_profile {
        bool only_reserve = true;
        bool saw_payload = false;
    };

    static bool is_section_directive(const stmt& stmt, std::string* out_name = nullptr) {
        if (stmt.kind != stmt_kind::directive)
            return false;
        const auto& name = stmt.directive_name;
        if (name == "area" || name == "section") {
            if (out_name)
                *out_name = stmt.string_arg.empty() ? "_CODE" : stmt.string_arg;
            return true;
        }
        if (name == "text" || name == "data"
            || name == "rodata" || name == "bss" || name == "tdata") {
            if (out_name)
                *out_name = "." + name;
            return true;
        }
        return false;
    }

    static bool is_reserve_only_directive(const stmt& stmt) {
        return stmt.kind == stmt_kind::directive
            && (stmt.directive_name == "ds"
                || stmt.directive_name == "space"
                || stmt.directive_name == "blkb"
                || stmt.directive_name == "blkw"
                || stmt.directive_name == "align"
                || stmt.directive_name == "balign"
                || stmt.directive_name == "p2align");
    }

    static bool is_payload_directive(const stmt& stmt) {
        return stmt.kind == stmt_kind::directive
            && (stmt.directive_name == "byte"
                || stmt.directive_name == "db"
                || stmt.directive_name == "word"
                || stmt.directive_name == "dw"
                || stmt.directive_name == "2byte"
                || stmt.directive_name == "dl"
                || stmt.directive_name == "ascii"
                || stmt.directive_name == "asciz"
                || stmt.directive_name == "string");
    }

    void analyse_sections(const stmt_list& stmts) const {
        gnu_section_overrides_.clear();
        gnu_section_break_before_.clear();
        if (format_ != output_format::gnu)
            return;

        std::map<std::string, section_profile> profiles;
        std::string current_section = "_CODE";
        profiles[current_section];
        std::vector<std::pair<std::string, std::vector<size_t>>> sections;
        sections.push_back({current_section, {}});

        for (size_t i = 0; i < stmts.size(); ++i) {
            const auto& stmt = stmts[i];
            std::string next_section;
            if (is_section_directive(stmt, &next_section)) {
                current_section = next_section;
                profiles[current_section];
                sections.push_back({current_section, {}});
                continue;
            }

            sections.back().second.push_back(i);

            auto& profile = profiles[current_section];
            if (stmt.kind == stmt_kind::instruction) {
                profile.only_reserve = false;
                continue;
            }
            if (is_payload_directive(stmt)) {
                profile.only_reserve = false;
                profile.saw_payload = true;
                continue;
            }
            if (is_reserve_only_directive(stmt))
                continue;
        }

        for (const auto& [name, profile] : profiles) {
            if (name == "_DATA" && profile.only_reserve && !profile.saw_payload)
                gnu_section_overrides_[name] = ".bss";
        }

        for (const auto& [name, indices] : sections) {
            if (name != "_DATA" && name != ".data")
                continue;
            auto it = profiles.find(name);
            if (it == profiles.end() || !it->second.saw_payload || it->second.only_reserve)
                continue;

            size_t split_index = static_cast<size_t>(-1);
            bool saw_reserve_tail = false;
            for (auto rit = indices.rbegin(); rit != indices.rend(); ++rit) {
                const auto& stmt = stmts[*rit];
                if (is_reserve_only_directive(stmt)) {
                    split_index = *rit;
                    saw_reserve_tail = true;
                    continue;
                }
                if (stmt.kind == stmt_kind::comment
                    || stmt.kind == stmt_kind::label
                    || stmt.kind == stmt_kind::equ) {
                    if (saw_reserve_tail)
                        split_index = *rit;
                    continue;
                }
                if (stmt.kind == stmt_kind::directive
                    && (stmt.directive_name == "equ"
                        || stmt.directive_name == "set"
                        || stmt.directive_name == "define")) {
                    if (saw_reserve_tail)
                        split_index = *rit;
                    continue;
                }
                break;
            }

            if (saw_reserve_tail && split_index != static_cast<size_t>(-1))
                gnu_section_break_before_[split_index] = ".bss";
        }
    }

    output_format format_;
    mutable std::map<std::string, std::string> gnu_section_overrides_;
    mutable std::map<size_t, std::string> gnu_section_break_before_;
};

class sdcc_source_emitter final : public base_source_emitter {
public:
    sdcc_source_emitter() : base_source_emitter(output_format::sdcc) {}
};

class gnu_source_emitter final : public base_source_emitter {
public:
    gnu_source_emitter() : base_source_emitter(output_format::gnu) {}
};

} // namespace

std::unique_ptr<source_emitter> make_source_emitter(output_format format)
{
    if (format == output_format::sdcc)
        return std::make_unique<sdcc_source_emitter>();
    if (format == output_format::gnu)
        return std::make_unique<gnu_source_emitter>();
    throw std::runtime_error("unsupported source emitter format");
}

void write_formatted_document(std::ostream& out,
                              const xz80::formatted_document& document)
{
    out << xz80::render_document(document);
}

} // namespace xas
