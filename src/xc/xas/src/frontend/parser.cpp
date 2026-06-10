// parser.cpp
//
// xas parser — converts a token stream into a statement list.
// Handles both SDCC and GNU directive dialects.
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
//
#include <algorithm>
#include <cctype>
#include <string>

#include <xas/errors.h>
#include <xas/frontend/parser.h>

namespace xas {

    // -------------------------------------------------------------------------
    // Token navigation
    // -------------------------------------------------------------------------

    static const token SENTINEL = {token_kind::eof, "", "", 0, "<eof>", 0};

    const token& parser::peek(size_t ahead) const
    {
        size_t idx = pos_ + ahead;
        return idx < tokens_->size() ? (*tokens_)[idx] : SENTINEL;
    }

    const token& parser::advance()
    {
        const token& t = (*tokens_)[pos_];
        if (pos_ < tokens_->size()) ++pos_;
        return t;
    }

    bool parser::at_end() const
    {
        return pos_ >= tokens_->size()
               || (*tokens_)[pos_].kind == token_kind::eof;
    }

    bool parser::eat(token_kind k)
    {
        if (peek().kind == k) { advance(); return true; }
        return false;
    }

    const token& parser::expect(token_kind k, const std::string& msg)
    {
        if (peek().kind != k) {
            const auto& t = peek();
            throw parse_error(t.file, t.line, msg);
        }
        return advance();
    }

    void parser::skip_to_newline()
    {
        while (!at_end() && peek().kind != token_kind::newline) advance();
        eat(token_kind::newline);
    }

    void parser::finish_stmt(stmt& s)
    {
        if (peek().kind == token_kind::comment)
            s.trailing_comment = advance().text;
        if (peek().kind == token_kind::newline) {
            advance();
            return;
        }
        if (peek().kind == token_kind::eof)
            return;
        throw parse_error(peek().file, peek().line, "expected end of line");
    }

    // -------------------------------------------------------------------------
    // Register and condition helpers
    // -------------------------------------------------------------------------

    static const std::string REGISTERS[] = {
        "A","B","C","D","E","H","L","F",
        "BC","DE","HL","SP","AF","AF'",
        "IX","IY","IXH","IXL","IYH","IYL",
        "I","R"
    };

    static bool is_register(const std::string& s)
    {
        for (auto& r : REGISTERS)
            if (r == s) return true;
        return false;
    }

    static const std::string CONDITIONS[] = {
        "NZ","Z","NC","C","PO","PE","P","M"
    };

    static bool is_condition(const std::string& s)
    {
        for (auto& c : CONDITIONS)
            if (c == s) return true;
        return false;
    }

    static bool is_unsupported_macro_directive(const std::string& s)
    {
        return s == "macro"
            || s == "endm"
            || s == "mend"
            || s == "rept"
            || s == "irp"
            || s == "irpc";
    }

    // -------------------------------------------------------------------------
    // Expression parsing (recursive descent, standard precedence)
    // -------------------------------------------------------------------------

    expr_ptr parser::parse_primary_expr()
    {
        const token& t = peek();

        if (t.kind == token_kind::integer) {
            advance();
            return expr::make_int(t.int_val, t.line);
        }
        if (t.kind == token_kind::char_lit) {
            advance();
            return expr::make_int(t.int_val, t.line);
        }
        if (t.kind == token_kind::dollar || t.kind == token_kind::dot) {
            advance();
            return expr::make_cur(t.line);
        }
        if (t.kind == token_kind::ident) {
            advance();
            return expr::make_sym(t.raw, t.line);
        }
        // Dot-prefixed local label reference (.name used as an expression).
        if (t.kind == token_kind::directive) {
            advance();
            std::string raw = t.raw.empty() ? t.text : t.raw;
            return expr::make_sym("." + raw, t.line);
        }
        if (t.kind == token_kind::hash || t.kind == token_kind::at) {
            // Immediate prefix (#n or @n) — skip and parse the full expression
            // (must go through unary so #-1 works).
            advance();
            return parse_unary_expr();
        }
        if (t.kind == token_kind::lparen) {
            advance();
            auto e = parse_expr();
            expect(token_kind::rparen, "expected ')'");
            return e;
        }
        throw parse_error(t.file, t.line,
            "unexpected token in expression: '" + t.text + "'");
    }

    expr_ptr parser::parse_unary_expr()
    {
        const token& t = peek();
        if (t.kind == token_kind::minus) {
            advance();
            return expr::make_unary('-', parse_unary_expr(), t.line);
        }
        if (t.kind == token_kind::tilde) {
            advance();
            return expr::make_unary('~', parse_unary_expr(), t.line);
        }
        return parse_primary_expr();
    }

    expr_ptr parser::parse_mul_expr()
    {
        auto lhs = parse_unary_expr();
        while (peek().kind == token_kind::star
               || peek().kind == token_kind::slash
               || peek().kind == token_kind::percent) {
            char op = peek().text[0];
            int ln = peek().line;
            advance();
            lhs = expr::make_binary(op, std::move(lhs), parse_unary_expr(), ln);
        }
        return lhs;
    }

    expr_ptr parser::parse_add_expr()
    {
        auto lhs = parse_mul_expr();
        while (peek().kind == token_kind::plus
               || peek().kind == token_kind::minus) {
            char op = peek().text[0];
            int ln = peek().line;
            advance();
            lhs = expr::make_binary(op, std::move(lhs), parse_mul_expr(), ln);
        }
        return lhs;
    }

    expr_ptr parser::parse_shift_expr()
    {
        auto lhs = parse_add_expr();
        while (peek().kind == token_kind::lshift
               || peek().kind == token_kind::rshift) {
            char op = (peek().kind == token_kind::lshift) ? '<' : '>';
            int ln = peek().line;
            advance();
            lhs = expr::make_binary(op, std::move(lhs), parse_add_expr(), ln);
        }
        return lhs;
    }

    expr_ptr parser::parse_and_expr()
    {
        auto lhs = parse_shift_expr();
        while (peek().kind == token_kind::amp) {
            int ln = peek().line; advance();
            lhs = expr::make_binary('&', std::move(lhs), parse_shift_expr(), ln);
        }
        return lhs;
    }

    expr_ptr parser::parse_xor_expr()
    {
        auto lhs = parse_and_expr();
        while (peek().kind == token_kind::caret) {
            int ln = peek().line; advance();
            lhs = expr::make_binary('^', std::move(lhs), parse_and_expr(), ln);
        }
        return lhs;
    }

    expr_ptr parser::parse_or_expr()
    {
        auto lhs = parse_xor_expr();
        while (peek().kind == token_kind::pipe) {
            int ln = peek().line; advance();
            lhs = expr::make_binary('|', std::move(lhs), parse_xor_expr(), ln);
        }
        return lhs;
    }

    expr_ptr parser::parse_expr()
    {
        return parse_or_expr();
    }

    // -------------------------------------------------------------------------
    // Operand parsing
    // -------------------------------------------------------------------------

    operand parser::parse_operand()
    {
        operand op;
        op.source_line = peek().line;

        const token& t = peek();

        // Register or condition code.
        if (t.kind == token_kind::ident) {
            if (is_register(t.text)) {
                op.kind = operand_kind::reg;
                op.reg_name = t.text;
                advance();
                return op;
            }
            if (is_condition(t.text)) {
                op.kind = operand_kind::cond;
                op.cond_name = t.text;
                advance();
                return op;
            }
        }

        // Indirect: (reg), (IX+expr), (IY+expr), (expr)
        if (t.kind == token_kind::lparen) {
            advance(); // consume '('
            const token& inner = peek();

            if (inner.kind == token_kind::ident) {
                std::string reg = inner.text;
                if (reg == "IX" || reg == "IY") {
                    advance(); // consume IX/IY
                    if (peek().kind == token_kind::plus
                        || peek().kind == token_kind::minus) {
                        char sign = peek().text[0]; advance();
                        auto off = parse_expr();
                        if (sign == '-')
                            off = expr::make_unary('-', std::move(off),
                                                   off->source_line);
                        expect(token_kind::rparen, "expected ')' after (IX/IY+d)");
                        op.kind     = (reg == "IX") ? operand_kind::ind_ix_off
                                                    : operand_kind::ind_iy_off;
                        op.reg_name = reg;
                        op.value    = std::move(off);
                    } else {
                        // (IX) or (IY) without offset → offset 0
                        expect(token_kind::rparen, "expected ')' after (IX/IY)");
                        op.kind     = (reg == "IX") ? operand_kind::ind_ix_off
                                                    : operand_kind::ind_iy_off;
                        op.reg_name = reg;
                        op.value    = expr::make_int(0, inner.line);
                    }
                    return op;
                }
                if (is_register(reg)) {
                    advance();
                    expect(token_kind::rparen, "expected ')' after (reg)");
                    op.kind     = operand_kind::ind_reg;
                    op.reg_name = reg;
                    return op;
                }
            }

            // (expr)
            op.kind  = operand_kind::ind_expr;
            op.value = parse_expr();
            expect(token_kind::rparen, "expected ')'");
            return op;
        }

        // Immediate expression — but may be SDCC d(IX) / d(IY) form.
        op.kind  = operand_kind::imm;
        op.value = parse_expr();

        // SDCC displacement syntax: expr(IX) or expr(IY)
        if (peek().kind == token_kind::lparen) {
            const token& inner = peek(1); // look ahead past '('
            if (inner.kind == token_kind::ident
                && (inner.text == "IX" || inner.text == "IY")) {
                advance(); // consume '('
                std::string reg = inner.text;
                advance(); // consume IX/IY
                expect(token_kind::rparen, "expected ')' after d(IX/IY)");
                op.kind     = (reg == "IX") ? operand_kind::ind_ix_off
                                            : operand_kind::ind_iy_off;
                op.reg_name = reg;
            }
        }
        return op;
    }

    // -------------------------------------------------------------------------
    // Directive parsing
    // -------------------------------------------------------------------------

    stmt parser::parse_directive(const std::string& name)
    {
        stmt s;
        s.kind           = stmt_kind::directive;
        s.source_line    = peek().line - 1; // already consumed directive token
        s.directive_name = name;

        if (is_unsupported_macro_directive(name)) {
            throw parse_error(peek().file,
                              s.source_line,
                              "macro directives are not supported in xas source mode");
        }

        // .module / .file — just a module name string
        if (name == "module" || name == "file") {
            if (peek().kind == token_kind::ident
                || peek().kind == token_kind::string_lit) {
                const token& a = advance(); s.string_arg = a.raw.empty() ? a.text : a.raw;
            }
            finish_stmt(s);
            return s;
        }

        // GNU shorthand section directives.
        if (name == "text" || name == "data"
            || name == "rodata" || name == "bss"
            || name == "tdata") {
            s.directive_name = "section";
            s.string_arg = "." + name;
            finish_stmt(s);
            return s;
        }

        // .area <NAME> [(flags)] — SDCC section declaration
        // .section <name>[,flags] — GNU section declaration
        if (name == "area" || name == "section") {
            if (peek().kind == token_kind::ident
                || peek().kind == token_kind::directive) {
                const token& a = advance(); s.string_arg = a.raw.empty() ? a.text : a.raw;
            } else if (peek().kind == token_kind::dot) {
                advance();
                if (peek().kind == token_kind::ident) {
                    const token& a = advance(); s.string_arg = "." + (a.raw.empty() ? a.text : a.raw);
                }
            }
            while (!at_end() && peek().kind != token_kind::newline
                   && peek().kind != token_kind::comment) {
                if (!s.string_arg2.empty())
                    s.string_arg2 += ' ';
                const token& extra = advance();
                s.string_arg2 += extra.raw.empty() ? extra.text : extra.raw;
            }
            finish_stmt(s);
            return s;
        }

        // .globl / .global — one or more symbol names
        if (name == "globl" || name == "global") {
            while (peek().kind == token_kind::ident) {
                const token& a = advance();
                s.args.push_back(expr::make_sym(a.raw.empty() ? a.text : a.raw, peek().line));
                eat(token_kind::comma);
            }
            finish_stmt(s);
            return s;
        }

        // .equ / .set — name and value (handled separately in parse_equ)
        // but may also appear as directive
        if (name == "equ" || name == "set") {
            if (peek().kind == token_kind::ident) {
                const token& a = advance(); s.string_arg = a.raw.empty() ? a.text : a.raw;
            }
            eat(token_kind::comma);
            s.args.push_back(parse_expr());
            finish_stmt(s);
            return s;
        }

        // .org / .origin — set origin
        if (name == "org" || name == "origin") {
            s.args.push_back(parse_expr());
            finish_stmt(s);
            return s;
        }

        // .byte / .db — emit byte values
        if (name == "byte" || name == "db") {
            do {
                if (peek().kind == token_kind::string_lit) {
                    s.string_arg += advance().text;
                } else {
                    s.args.push_back(parse_expr());
                }
            } while (eat(token_kind::comma));
            finish_stmt(s);
            return s;
        }

        // .word / .dw / .2byte — emit 16-bit words
        if (name == "word" || name == "dw" || name == "2byte") {
            do { s.args.push_back(parse_expr()); } while (eat(token_kind::comma));
            finish_stmt(s);
            return s;
        }

        // .ds / .space — reserve bytes
        if (name == "ds" || name == "space") {
            s.args.push_back(parse_expr());
            finish_stmt(s);
            return s;
        }

        // .ascii / .asciz — string data
        if (name == "ascii" || name == "asciz" || name == "string") {
            if (peek().kind == token_kind::string_lit)
                s.string_arg = advance().text;
            if (name == "asciz" || name == "string")
                s.string_arg += '\0';
            finish_stmt(s);
            return s;
        }

        // .include — handled at a higher level; store path
        if (name == "include") {
            if (peek().kind == token_kind::string_lit)
                s.string_arg = advance().text;
            finish_stmt(s);
            return s;
        }

        // .if / .else / .endif — store as directive with optional arg
        if (name == "if" || name == "ifdef" || name == "ifndef") {
            if (!at_end() && peek().kind != token_kind::newline)
                s.args.push_back(parse_expr());
            finish_stmt(s);
            return s;
        }
        if (name == "else" || name == "endif") {
            finish_stmt(s);
            return s;
        }

        // .dl / .long expr — emit 32-bit value
        if (name == "dl" || name == "long") {
            if (name == "long")
                s.directive_name = "dl"; // normalize GNU spelling
            do { s.args.push_back(parse_expr()); } while (eat(token_kind::comma));
            finish_stmt(s);
            return s;
        }

        // .blkb N — reserve N bytes (SDCC extension, alias for .ds)
        if (name == "blkb") {
            s.directive_name = "ds"; // normalise to .ds
            s.args.push_back(parse_expr());
            finish_stmt(s);
            return s;
        }

        // .blkw N — reserve N words = 2N bytes (SDCC extension)
        if (name == "blkw") {
            s.args.push_back(parse_expr());
            finish_stmt(s);
            return s;
        }

        // .define symbol [= value] — define a symbol (SDCC extension)
        if (name == "define") {
            if (peek().kind == token_kind::ident) {
                const token &a = advance();
                s.string_arg = a.raw.empty() ? a.text : a.raw;
            }
            if (eat(token_kind::eq) && !at_end() && peek().kind != token_kind::newline)
                s.args.push_back(parse_expr());
            else
                s.args.push_back(expr::make_int(1, s.source_line)); // default value 1
            finish_stmt(s);
            return s;
        }

        // .optsdcc ... — SDCC assembler option payload
        if (name == "optsdcc") {
            bool first = true;
            while (!at_end() && peek().kind != token_kind::newline) {
                if (!first) s.string_arg2 += ' ';
                s.string_arg2 += advance().text;
                first = false;
            }
            finish_stmt(s);
            return s;
        }

        // .24bit / .32bit — SDCC pointer-size hints (no-op for xas)
        if (name == "24bit" || name == "32bit") { finish_stmt(s); return s; }

        // Unknown directive — collect remaining tokens as string_arg.
        while (!at_end() && peek().kind != token_kind::newline) {
            if (peek().kind == token_kind::comment) {
                s.trailing_comment = advance().text;
                break;
            }
            s.string_arg += advance().text + " ";
        }
        if (peek().kind == token_kind::newline)
            advance();
        else if (peek().kind != token_kind::eof)
            throw parse_error(peek().file, peek().line, "expected end of line");
        return s;
    }

    // -------------------------------------------------------------------------
    // Instruction parsing
    // -------------------------------------------------------------------------

    stmt parser::parse_instruction(const std::string& mnemonic)
    {
        stmt s;
        s.kind     = stmt_kind::instruction;
        s.mnemonic = mnemonic;
        s.source_line = peek().line;

        // Collect comma-separated operands until newline/eof.
        if (!at_end()
            && peek().kind != token_kind::newline
            && peek().kind != token_kind::comment) {
            s.operands.push_back(parse_operand());
            while (eat(token_kind::comma)) {
                s.operands.push_back(parse_operand());
            }
        }
        finish_stmt(s);
        return s;
    }

    // -------------------------------------------------------------------------
    // EQU statement
    // -------------------------------------------------------------------------

    stmt parser::parse_equ(const std::string& name)
    {
        stmt s;
        s.kind      = stmt_kind::equ;
        s.equ_name  = name;
        s.source_line = peek().line;
        // consume EQU keyword
        advance();
        s.equ_value = parse_expr();
        finish_stmt(s);
        return s;
    }

    stmt parser::parse_comment()
    {
        const token& t = expect(token_kind::comment, "expected comment");
        stmt s;
        s.kind = stmt_kind::comment;
        s.source_line = t.line;
        s.comment_text = t.text;
        if (peek().kind == token_kind::newline)
            advance();
        else if (peek().kind != token_kind::eof)
            throw parse_error(peek().file, peek().line, "expected end of line");
        return s;
    }

    // -------------------------------------------------------------------------
    // Main statement parser
    // -------------------------------------------------------------------------

    stmt parser::parse_stmt()
    {
        // Skip blank lines.
        while (eat(token_kind::newline)) {
            if (at_end()) {
                stmt s; s.kind = stmt_kind::directive;
                s.directive_name = "_eof";
                return s;
            }
        }

        const token& t = peek();

        if (t.kind == token_kind::comment)
            return parse_comment();

        // Directive line — but may be a dot-prefixed local label (.name:).
        if (t.kind == token_kind::directive) {
            std::string raw  = t.raw.empty() ? t.text : t.raw;
            std::string name = t.text;
            advance();
            if (peek().kind == token_kind::colon) {
                stmt s;
                s.kind        = stmt_kind::label;
                s.label_name  = "." + raw;
                s.source_line = t.line;
                advance(); // consume ':'
                if (peek().kind == token_kind::comment
                    || peek().kind == token_kind::newline
                    || peek().kind == token_kind::eof) {
                    finish_stmt(s);
                }
                return s;
            }
            return parse_directive(name);
        }

        // Identifier: could be label, instruction, or EQU.
        if (t.kind == token_kind::ident) {
            std::string mnem = t.text;                      // uppercase for mnemonic
            std::string sym  = t.raw.empty() ? t.text : t.raw; // original case for symbols
            advance();

            // label: colon follows (single ':' or SDCC '::' global-export form)
            if (peek().kind == token_kind::colon) {
                stmt s;
                s.kind       = stmt_kind::label;
                s.label_name = sym;
                s.source_line= t.line;
                advance(); // consume ':'
                eat(token_kind::colon); // consume optional second ':' (SDCC '::')
                if (peek().kind == token_kind::comment
                    || peek().kind == token_kind::newline
                    || peek().kind == token_kind::eof) {
                    finish_stmt(s);
                }
                return s;
            }

            // EQU keyword: "name EQU expr" / "name DEFL expr" / "name SET expr"
            if (peek().kind == token_kind::ident
                && (peek().text == "EQU" || peek().text == "DEFL"
                    || peek().text == "SET")) {
                return parse_equ(sym);
            }

            // SDCC/GAS-style dotted absolute assignment: "name .equ expr".
            if (peek().kind == token_kind::directive
                && (peek().text == "equ" || peek().text == "set")) {
                return parse_equ(sym);
            }

            // Absolute symbol assignment: "name = expr" (SDCC / GNU syntax)
            if (peek().kind == token_kind::eq) {
                advance(); // consume '='
                stmt s;
                s.kind       = stmt_kind::equ;
                s.equ_name   = sym;
                s.source_line = t.line;
                s.equ_value  = parse_expr();
                finish_stmt(s);
                return s;
            }

            // Otherwise it's an instruction mnemonic.
            stmt s = parse_instruction(mnem);
            s.source_line = t.line;
            return s;
        }

        // Unexpected token.
        throw parse_error(t.file, t.line,
            "unexpected token: '" + t.text + "'");
    }

    // -------------------------------------------------------------------------
    // parse — public entry point
    // -------------------------------------------------------------------------

    stmt_list parser::parse(const std::vector<token>& tokens, asm_mode mode)
    {
        tokens_ = &tokens;
        pos_    = 0;
        mode_   = mode;

        stmt_list result;
        while (!at_end()) {
            if (peek().kind == token_kind::newline) { advance(); continue; }
            result.push_back(parse_stmt());
        }
        return result;
    }

} // namespace xas
