// macro_translate.cpp
//
// See macro_translate.h.
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
//
#include <xas/backend/macro_translate.h>
#include <xas/backend/source_emitter.h>
#include <xas/errors.h>
#include <xas/frontend/lexer.h>
#include <xas/frontend/parser.h>

#include <cctype>
#include <sstream>

namespace xas {

    namespace {

        const std::string INDENT = "        ";   // 8 spaces (house style)

        bool is_param(const std::vector<std::string>& params, const std::string& n)
        {
            for (const std::string& p : params) if (p == n) return true;
            return false;
        }

        // Replace whole-word identifier 'name' (bare, sdas style) with 'to'.
        std::string replace_word(const std::string& s, const std::string& name,
                                 const std::string& to)
        {
            std::string out;
            size_t i = 0;
            while (i < s.size()) {
                if (mtext::is_ident_char(s[i])) {
                    size_t j = i;
                    while (j < s.size() && mtext::is_ident_char(s[j])) ++j;
                    std::string w = s.substr(i, j - i);
                    bool boundary_ok =
                        (i == 0 || !mtext::is_ident_char(s[i - 1]));
                    out += (w == name && boundary_ok) ? to : w;
                    i = j;
                } else {
                    out += s[i++];
                }
            }
            return out;
        }

        // Replace gas '\name' with 'to'.
        std::string replace_bsref(const std::string& s, const std::string& name,
                                  const std::string& to)
        {
            std::string out;
            for (size_t i = 0; i < s.size(); ) {
                if (s[i] == '\\' && s.compare(i + 1, name.size(), name) == 0) {
                    size_t after = i + 1 + name.size();
                    if (after >= s.size() || !mtext::is_ident_char(s[after])) {
                        out += to; i = after; continue;
                    }
                }
                out += s[i++];
            }
            return out;
        }

        // Does the line contain '\' immediately before one of params?
        bool has_bslash_param(const std::string& s,
                              const std::vector<std::string>& params)
        {
            for (size_t i = 0; i + 1 < s.size(); ++i) {
                if (s[i] != '\\') continue;
                size_t j = i + 1;
                while (j < s.size() && mtext::is_ident_char(s[j])) ++j;
                if (is_param(params, s.substr(i + 1, j - (i + 1)))) return true;
            }
            return false;
        }

        // Does the line contain a single quote immediately adjacent to a param?
        bool has_quote_concat(const std::string& s,
                              const std::vector<std::string>& params)
        {
            for (size_t i = 0; i < s.size(); ++i) {
                if (s[i] != '\'') continue;
                // identifier immediately to the right
                if (i + 1 < s.size() && mtext::is_ident_char(s[i + 1])) {
                    size_t j = i + 1;
                    while (j < s.size() && mtext::is_ident_char(s[j])) ++j;
                    if (is_param(params, s.substr(i + 1, j - (i + 1)))) return true;
                }
                // identifier immediately to the left
                if (i > 0 && mtext::is_ident_char(s[i - 1])) {
                    size_t j = i;
                    while (j > 0 && mtext::is_ident_char(s[j - 1])) --j;
                    if (is_param(params, s.substr(j, i - j))) return true;
                }
            }
            return false;
        }

        std::string lower_dir(const std::string& line)
        {
            auto ls = mtext::split_label(mtext::trim(line));
            auto os = mtext::split_operator(ls.rest);
            return mtext::lower(os.op);
        }

        bool is_section_directive(const std::string& op)
        {
            return op == ".text" || op == ".data" || op == ".bss" ||
                   op == ".section" || op == ".area";
        }

        // Map a well-known section name across dialects.
        std::string xlate_section_name(const std::string& n, bool dst_gas)
        {
            if (dst_gas) {
                if (n == "_CODE") return ".text";
                if (n == "_DATA" || n == "_INITIALIZED") return ".data";
                if (n == "_BSS")  return ".bss";
            } else {
                if (n == ".text") return "_CODE";
                if (n == ".data") return "_DATA";
                if (n == ".bss")  return "_BSS";
            }
            return n;
        }

        // Translate a directive spelling to the target dialect.  Returns the
        // new directive (with leading dot) or empty if 'op' is not a directive
        // whose spelling differs between dialects.
        std::string xlate_directive_name(const std::string& op, bool dst_gas)
        {
            struct pair { const char* sdcc; const char* gnu; };
            static const pair pairs[] = {
                { ".db", ".byte" }, { ".dw", ".word" }, { ".dl", ".long" },
                { ".ds", ".space" }, { ".globl", ".global" },
                { ".module", ".file" }, { ".define", ".set" },
                { ".area", ".section" },
            };
            for (const pair& p : pairs) {
                if (op == p.sdcc || op == p.gnu)
                    return dst_gas ? p.gnu : p.sdcc;
            }
            return "";
        }

        // Group 'arg' for the target dialect if it contains separators.
        std::string group_arg(const std::string& arg, bool dst_gas)
        {
            bool needs = arg.find(' ')  != std::string::npos ||
                         arg.find('\t') != std::string::npos ||
                         arg.find(',')  != std::string::npos;
            if (!needs) return arg;
            if (dst_gas) return "\"" + arg + "\"";
            char d = (arg.find('/') == std::string::npos) ? '/' : '!';
            return std::string("^") + d + arg + d;
        }

    } // namespace

    // -------------------------------------------------------------------------

    macro_translator::macro_translator(asm_mode src_mode, output_format dst_format)
        : src_mode_(src_mode),
          dst_format_(dst_format),
          src_gas_(src_mode == asm_mode::gnu),
          dst_gas_(dst_format == output_format::gnu),
          same_dialect_(src_gas_ == dst_gas_),
          proc_(make_macro_dialect(src_mode)) {}

    std::string macro_translator::render_code(const src_lines& lines) const
    {
        if (lines.empty()) return "";
        std::string text;
        for (const src_line& l : lines) { text += l.text; text += '\n'; }
        std::istringstream in(text);
        lexer lex;
        auto toks = lex.tokenise("<convert>", in);
        parser par;
        auto stmts = par.parse(toks, src_mode_);
        auto emit = make_source_emitter(dst_format_);
        auto doc = emit->emit(stmts);
        std::ostringstream out;
        write_formatted_document(out, doc);
        return out.str();
    }

    size_t macro_translator::collect(const src_lines& lines, size_t start,
                                     macro_dialect::block kind,
                                     src_lines& body) const
    {
        std::vector<macro_dialect::block> stack{ kind };
        for (size_t i = start + 1; i < lines.size(); ++i) {
            std::string t = mtext::trim(dia().strip_comment(lines[i].text));
            auto ls = mtext::split_label(t);
            auto os = mtext::split_operator(ls.rest);
            std::string opl = mtext::lower(os.op);
            macro_dialect::block ob = dia().opener(opl);
            if (ob != macro_dialect::block::none) {
                stack.push_back(ob); body.push_back(lines[i]); continue;
            }
            if (dia().closes(opl, stack.back())) {
                stack.pop_back();
                if (stack.empty()) return i;
                body.push_back(lines[i]); continue;
            }
            body.push_back(lines[i]);
        }
        throw macro_error(lines[start].file, lines[start].line,
                          "unterminated macro / repeat block");
    }

    bool macro_translator::body_translatable(
        const src_lines& body, const std::vector<std::string>& params) const
    {
        for (const src_line& bl : body) {
            std::string op = lower_dir(bl.text);
            macro_dialect::block ob = dia().opener(op);
            if (ob == macro_dialect::block::macro)
                return false;                         // nested definition
            // Nested repeat blocks cannot be line-translated across dialects
            // (their .endr/.endm terminator is ambiguous); expand instead.
            if (!same_dialect_ && ob != macro_dialect::block::none)
                return false;
            if (src_gas_ && !dst_gas_) {              // gas -> sdas
                const std::string& s = bl.text;
                for (size_t i = 0; i + 1 < s.size(); ++i)
                    if (s[i] == '\\' &&
                        (s[i + 1] == '@' || s[i + 1] == '+' || s[i + 1] == '('))
                        return false;
            }
            if (!src_gas_ && dst_gas_) {              // sdas -> gas
                if (op == ".narg" || op == ".nchr" || op == ".nval")
                    return false;
                if (has_bslash_param(bl.text, params)) return false;
                if (has_quote_concat(bl.text, params)) return false;
            }
        }
        return true;
    }

    bool macro_translator::def_translatable(const macro_def& def) const
    {
        std::vector<std::string> params;
        for (const macro_param& p : def.params) {
            if (src_gas_ && !dst_gas_ && p.vararg) return false;
            if (!src_gas_ && dst_gas_ && p.autolocal) return false;
            params.push_back(p.name);
        }
        return body_translatable(def.body, params);
    }

    std::string macro_translator::convert_body(
        const src_lines& body, const std::vector<std::string>& params) const
    {
        std::string out;
        for (const src_line& bl : body) {
            if (same_dialect_) { out += bl.text + "\n"; continue; }

            // Macro bodies are converted line-by-line rather than through the
            // full emitter: a parameter's role (register vs. immediate) is
            // unknown, so its operand text must be preserved literally.  Only
            // the parameter-reference syntax and directive spellings differ
            // between dialects; z80 instruction syntax is shared and both
            // parsers tolerate the other's immediate prefix.
            std::string line = bl.text;
            for (const std::string& p : params)
                line = src_gas_ ? replace_bsref(line, p, p)        // \p -> p
                                : replace_word(line, p, "\\" + p); //  p -> \p

            auto ls = mtext::split_label(mtext::trim(line));
            auto os = mtext::split_operator(ls.rest);
            std::string newop = xlate_directive_name(mtext::lower(os.op), dst_gas_);
            if (!newop.empty()) {
                std::string operand = os.operand;
                if (is_section_directive(mtext::lower(os.op)) && !operand.empty()) {
                    auto sp = mtext::split_operator(operand);
                    operand = xlate_section_name(sp.op, dst_gas_) +
                              (sp.operand.empty() ? "" : " " + sp.operand);
                }
                std::string rebuilt = (ls.label.empty() ? INDENT : ls.label + " ");
                rebuilt += newop;
                if (!operand.empty()) rebuilt += " " + operand;
                out += rebuilt + "\n";
            } else {
                out += line + "\n";          // instruction / unchanged directive
            }
        }
        return out;
    }

    std::string macro_translator::emit_macro(const macro_def& def) const
    {
        std::string head = INDENT + ".macro " + def.name;
        if (!def.params.empty()) {
            if (dst_gas_) {
                for (const macro_param& p : def.params) {
                    head += " " + p.name;
                    if (p.has_default) head += "=" + p.default_val;
                    if (p.required)    head += ":req";
                    if (p.vararg)      head += ":vararg";
                }
            } else {
                head += " ";
                for (size_t i = 0; i < def.params.size(); ++i)
                    head += (i ? ", " : "") + def.params[i].name;
            }
        }
        std::vector<std::string> names;
        for (const macro_param& p : def.params) names.push_back(p.name);

        return head + "\n" + convert_body(def.body, names) + INDENT + ".endm\n";
    }

    std::string macro_translator::emit_repeat(macro_dialect::block kind,
                                              const std::string& operand,
                                              const src_lines& body) const
    {
        std::string head;
        std::vector<std::string> params;
        if (kind == macro_dialect::block::rept) {
            head = INDENT + ".rept " + mtext::trim(operand);
        } else {
            std::vector<std::string> fields = dia().split_args(operand);
            std::string sym = fields.empty() ? "" : fields.front();
            params.push_back(sym);
            std::string rest;
            if (kind == macro_dialect::block::irp) {
                for (size_t i = 1; i < fields.size(); ++i)
                    rest += (i > 1 ? ", " : "") + group_arg(fields[i], dst_gas_);
                head = INDENT + ".irp " + sym + ", " + rest;
            } else {
                std::string str = fields.size() > 1 ? fields[1] : std::string();
                head = INDENT + ".irpc " + sym + ", " + str;
            }
        }
        std::string term = INDENT + (dst_gas_ ? ".endr\n" : ".endm\n");
        return head + "\n" + convert_body(body, params) + term;
    }

    std::string macro_translator::emit_call(const std::string& label,
                                            const macro_def& def,
                                            const std::string& operand) const
    {
        std::vector<std::string> raw = dia().split_args(operand);
        std::vector<std::string> vals;

        if (src_gas_) {                               // resolve keyword/default
            vals.resize(def.params.size());
            for (size_t i = 0; i < def.params.size(); ++i)
                vals[i] = def.params[i].has_default ? def.params[i].default_val
                                                    : std::string();
            size_t pos = 0;
            for (const std::string& a : raw) {
                size_t eq = a.find('=');
                if (eq != std::string::npos) {
                    std::string key = a.substr(0, eq);
                    size_t idx = def.params.size();
                    for (size_t i = 0; i < def.params.size(); ++i)
                        if (def.params[i].name == key) { idx = i; break; }
                    if (idx < def.params.size()) { vals[idx] = a.substr(eq + 1); continue; }
                }
                if (pos < vals.size()) vals[pos++] = a; else vals.push_back(a);
            }
            while (!vals.empty() && vals.back().empty()) vals.pop_back();
        } else {
            vals = raw;
        }

        std::string out;
        if (!label.empty()) out += label + "\n";
        std::string line = INDENT + def.name;
        for (size_t i = 0; i < vals.size(); ++i)
            line += (i ? ", " : " ") + group_arg(vals[i], dst_gas_);
        return out + line + "\n";
    }

    // -------------------------------------------------------------------------

    std::string macro_translator::translate(const src_lines& lines)
    {
        proc_.scan_definitions(lines);

        std::string out;
        src_lines code_buf;
        auto flush = [&] {
            if (!code_buf.empty()) { out += render_code(code_buf); code_buf.clear(); }
        };

        for (size_t i = 0; i < lines.size(); ) {
            std::string t = mtext::trim(dia().strip_comment(lines[i].text));
            if (t.empty()) { code_buf.push_back(lines[i]); ++i; continue; }

            auto ls = mtext::split_label(t);
            auto os = mtext::split_operator(ls.rest);
            std::string opl = mtext::lower(os.op);
            macro_dialect::block ob = dia().opener(opl);

            // --- macro definition -------------------------------------------
            if (ob == macro_dialect::block::macro) {
                src_lines body;
                size_t endi = collect(lines, i, ob, body);
                macro_def def = dia().parse_macro_header(
                    { ls.rest, lines[i].file, lines[i].line });
                def.body = body;
                if (def_translatable(def)) {
                    flush();
                    out += emit_macro(def);
                    translated_[dia().case_sensitive_names()
                                    ? def.name : mtext::lower(def.name)] = def;
                }
                // non-translatable defs emit nothing; calls expand later.
                i = endi + 1;
                continue;
            }

            // --- repeat block -----------------------------------------------
            if (ob == macro_dialect::block::rept || ob == macro_dialect::block::irp ||
                ob == macro_dialect::block::irpc) {
                src_lines body;
                size_t endi = collect(lines, i, ob, body);
                std::vector<std::string> params;
                if (ob != macro_dialect::block::rept) {
                    auto f = dia().split_args(os.operand);
                    if (!f.empty()) params.push_back(f.front());
                }
                if (body_translatable(body, params)) {
                    flush();
                    out += emit_repeat(ob, os.operand, body);
                } else {
                    src_lines frag(lines.begin() + i, lines.begin() + endi + 1);
                    for (const src_line& e : proc_.expand_fragment(frag))
                        code_buf.push_back(e);
                }
                i = endi + 1;
                continue;
            }

            // --- macro call -------------------------------------------------
            std::string key = dia().case_sensitive_names() ? os.op
                                                           : mtext::lower(os.op);
            auto it = translated_.find(key);
            if (it != translated_.end()) {
                flush();
                out += emit_call(ls.label, it->second, os.operand);
                ++i;
                continue;
            }
            // Call of a non-translatable macro is expanded in place; anything
            // else is an ordinary line.
            if (proc_.knows_macro(os.op)) {
                src_lines frag{ lines[i] };
                for (const src_line& e : proc_.expand_fragment(frag))
                    code_buf.push_back(e);
                ++i;
                continue;
            }

            code_buf.push_back(lines[i]);
            ++i;
        }
        flush();
        return out;
    }

} // namespace xas
