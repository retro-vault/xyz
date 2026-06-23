// macro.cpp
//
// Dialect-agnostic macro preprocessor engine plus shared lexical helpers
// and the dialect factory.  See macro.h for the overall design.
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
//
#include <xas/frontend/macro.h>
#include <xas/errors.h>

#include <cctype>
#include <cstdint>

namespace xas {

    // =========================================================================
    // Shared lexical helpers
    // =========================================================================
    namespace mtext {

        bool is_ident_char(char c)
        {
            return std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '$';
        }

        std::string ltrim(const std::string& s)
        {
            size_t i = 0;
            while (i < s.size() &&
                   std::isspace(static_cast<unsigned char>(s[i]))) ++i;
            return s.substr(i);
        }

        std::string rtrim(const std::string& s)
        {
            size_t i = s.size();
            while (i > 0 &&
                   std::isspace(static_cast<unsigned char>(s[i - 1]))) --i;
            return s.substr(0, i);
        }

        std::string trim(const std::string& s) { return rtrim(ltrim(s)); }

        std::string lower(const std::string& s)
        {
            std::string r = s;
            for (char& c : r) c = static_cast<char>(
                std::tolower(static_cast<unsigned char>(c)));
            return r;
        }

        // A label is a leading run of symbol characters followed by ':' or '::'.
        label_split split_label(const std::string& line)
        {
            std::string s = ltrim(line);
            size_t i = 0;
            while (i < s.size() && (is_ident_char(s[i]) || s[i] == '.' ||
                                    s[i] == '?')) ++i;
            if (i > 0 && i < s.size() && s[i] == ':') {
                size_t colon_end = i + 1;
                if (colon_end < s.size() && s[colon_end] == ':') ++colon_end;
                return { s.substr(0, colon_end), ltrim(s.substr(colon_end)) };
            }
            return { "", s };
        }

        op_split split_operator(const std::string& s)
        {
            std::string t = ltrim(s);
            size_t i = 0;
            while (i < t.size() &&
                   !std::isspace(static_cast<unsigned char>(t[i]))) ++i;
            return { t.substr(0, i), trim(t.substr(i)) };
        }

    } // namespace mtext

    // =========================================================================
    // Engine
    // =========================================================================

    namespace {
        constexpr int MAX_NEST = 100;
    }

    macro_processor::macro_processor(std::unique_ptr<macro_dialect> dialect)
        : dialect_(std::move(dialect)) {}

    void macro_processor::add_define(const std::string& name,
                                     const std::string& value)
    {
        defines_[name] = value;
    }

    bool macro_processor::has_macro_directives(const std::string& text)
    {
        std::string low = mtext::lower(text);
        return low.find(".macro") != std::string::npos ||
               low.find(".rept")  != std::string::npos ||
               low.find(".irp")   != std::string::npos;
    }

    const macro_def* macro_processor::find_macro(const std::string& op) const
    {
        std::string key = dialect_->case_sensitive_names() ? op : mtext::lower(op);
        auto it = macros_.find(key);
        return it == macros_.end() ? nullptr : &it->second;
    }

    src_lines macro_processor::run(const src_lines& input)
    {
        src_lines out;
        expand(input, out, 0);
        return out;
    }

    void macro_processor::scan_definitions(const src_lines& input)
    {
        for (size_t i = 0; i < input.size(); ) {
            std::string t = mtext::trim(dialect_->strip_comment(input[i].text));
            auto ls = mtext::split_label(t);
            auto os = mtext::split_operator(ls.rest);
            macro_dialect::block ob = dialect_->opener(mtext::lower(os.op));
            if (ob == macro_dialect::block::none) { ++i; continue; }

            src_lines body;
            size_t endi = collect_block(input, i, ob, body);
            if (ob == macro_dialect::block::macro) {
                macro_def def = dialect_->parse_macro_header(
                    { ls.rest, input[i].file, input[i].line });
                def.body = std::move(body);
                std::string key = dialect_->case_sensitive_names()
                                      ? def.name : mtext::lower(def.name);
                macros_[key] = std::move(def);
            }
            i = endi + 1;
        }
    }

    src_lines macro_processor::expand_fragment(const src_lines& fragment)
    {
        src_lines out;
        expand(fragment, out, 0);
        return out;
    }

    size_t macro_processor::collect_block(const src_lines& lines, size_t start,
                                          macro_dialect::block kind,
                                          src_lines& body) const
    {
        std::vector<macro_dialect::block> stack{ kind };
        for (size_t i = start + 1; i < lines.size(); ++i) {
            std::string t = mtext::trim(dialect_->strip_comment(lines[i].text));
            auto ls = mtext::split_label(t);
            auto os = mtext::split_operator(ls.rest);
            std::string opl = mtext::lower(os.op);

            macro_dialect::block ob = dialect_->opener(opl);
            if (ob != macro_dialect::block::none) {
                stack.push_back(ob);
                body.push_back(lines[i]);
                continue;
            }
            if (dialect_->closes(opl, stack.back())) {
                stack.pop_back();
                if (stack.empty())
                    return i;            // matching terminator (not part of body)
                body.push_back(lines[i]);
                continue;
            }
            body.push_back(lines[i]);
        }
        throw macro_error(lines[start].file, lines[start].line,
                          "unterminated macro / repeat block");
    }

    void macro_processor::expand(const src_lines& lines, src_lines& out, int depth)
    {
        if (depth > MAX_NEST)
            throw macro_error(lines.empty() ? "" : lines.front().file,
                              lines.empty() ? 0 : lines.front().line,
                              "macro expansion nested too deeply");

        for (size_t i = 0; i < lines.size(); ) {
            const src_line& sl = lines[i];
            std::string nocomment = dialect_->strip_comment(sl.text);
            std::string trimmed   = mtext::trim(nocomment);

            if (trimmed.empty()) { out.push_back(sl); ++i; continue; }

            auto ls  = mtext::split_label(trimmed);
            auto os  = mtext::split_operator(ls.rest);
            std::string opl = mtext::lower(os.op);

            // --- macro definition -------------------------------------------
            if (dialect_->opener(opl) == macro_dialect::block::macro) {
                src_lines body;
                size_t endi = collect_block(lines, i, macro_dialect::block::macro,
                                            body);
                macro_def def = dialect_->parse_macro_header(
                    { ls.rest, sl.file, sl.line });
                def.body = std::move(body);
                std::string key = dialect_->case_sensitive_names()
                                      ? def.name : mtext::lower(def.name);
                macros_[key] = std::move(def);
                i = endi + 1;
                continue;
            }

            // --- repeat blocks ----------------------------------------------
            macro_dialect::block ob = dialect_->opener(opl);
            if (ob == macro_dialect::block::rept ||
                ob == macro_dialect::block::irp  ||
                ob == macro_dialect::block::irpc) {
                src_lines body;
                size_t endi = collect_block(lines, i, ob, body);
                expand_repeat(ob, { os.operand, sl.file, sl.line }, body,
                              out, depth);
                i = endi + 1;
                continue;
            }

            // --- macro call -------------------------------------------------
            if (const macro_def* def = find_macro(os.op)) {
                expand_call(*def, { os.operand, sl.file, sl.line }, ls.label,
                            out, depth);
                ++i;
                continue;
            }

            // --- ordinary line: pass through verbatim -----------------------
            out.push_back(sl);
            ++i;
        }
    }

    void macro_processor::expand_call(const macro_def& def, const src_line& call,
                                      const std::string& label,
                                      src_lines& out, int depth)
    {
        std::vector<std::string> args = dialect_->split_args(call.text);

        long save_argc = ctx_.cur_argc;
        ctx_.cur_argc  = static_cast<long>(args.size());
        macro_binding binding = dialect_->bind(def, args, ctx_);

        if (!label.empty())
            out.push_back({ label, call.file, call.line });

        src_lines expanded;
        for (const src_line& bl : def.body) {
            // Detect early-exit before substitution.
            std::string bt = mtext::trim(dialect_->strip_comment(bl.text));
            auto bls = mtext::split_label(bt);
            auto bos = mtext::split_operator(bls.rest);
            if (dialect_->is_exit(mtext::lower(bos.op)))
                break;

            std::string subbed = dialect_->expand_line(bl.text, binding, ctx_);

            // Dialect attribute directives (sdas .narg/.nchr/.nval) are resolved
            // now, while the current call's argument count is in scope.
            std::string st = mtext::trim(dialect_->strip_comment(subbed));
            auto sls = mtext::split_label(st);
            auto sos = mtext::split_operator(sls.rest);
            if (auto rep = dialect_->attr_directive(mtext::lower(sos.op),
                                                    sos.operand, ctx_)) {
                std::string line = sls.label.empty()
                                       ? *rep : (sls.label + " " + *rep);
                expanded.push_back({ line, bl.file, bl.line });
                continue;
            }

            expanded.push_back({ subbed, bl.file, bl.line });
        }
        ctx_.cur_argc = save_argc;

        expand(expanded, out, depth + 1);
    }

    void macro_processor::expand_repeat(macro_dialect::block kind,
                                        const src_line& header,
                                        const src_lines& body,
                                        src_lines& out, int depth)
    {
        // Build the list of (symbol, values) to iterate over.
        std::string sym;
        std::vector<std::string> values;
        long count = 0;

        if (kind == macro_dialect::block::rept) {
            count = eval_count(header, header.text);
            if (count < 0) count = 0;
        } else {
            // .irp sym,list  /  .irpc sym,string
            std::vector<std::string> fields = dialect_->split_args(header.text);
            if (fields.empty())
                throw macro_error(header.file, header.line,
                                  "missing loop symbol in .irp/.irpc");
            sym = fields.front();
            if (kind == macro_dialect::block::irp) {
                values.assign(fields.begin() + 1, fields.end());
            } else {
                // .irpc: characters of the (single) value string.
                std::string s = fields.size() > 1 ? fields[1] : std::string();
                for (char c : s) values.emplace_back(1, c);
            }
            if (values.empty() && dialect_->irp_empty_emits_once())
                values.emplace_back("");
            count = static_cast<long>(values.size());
        }

        long save_plus = ctx_.plus_count;
        for (long n = 0; n < count; ++n) {
            ctx_.plus_count = n;
            src_lines iter;
            if (kind == macro_dialect::block::rept) {
                // Only \+ / counters vary; body lines may still contain macro
                // calls, so re-run them through the engine unchanged.
                iter = body;
            } else {
                macro_binding binding{ { sym, values[n] } };
                for (const src_line& bl : body)
                    iter.push_back({ dialect_->expand_line(bl.text, binding, ctx_),
                                     bl.file, bl.line });
            }
            expand(iter, out, depth + 1);
        }
        ctx_.plus_count = save_plus;
    }

    // -------------------------------------------------------------------------
    // Minimal integer evaluator for .rept counts: literals, object-like
    // defines, + - * / and parentheses.  Anything else is a hard error.
    // -------------------------------------------------------------------------
    long macro_processor::eval_count(const src_line& header,
                                     const std::string& expr) const
    {
        struct ev {
            const std::string& s;
            const std::map<std::string, std::string>& defs;
            size_t i = 0;
            bool ok = true;

            void ws() { while (i < s.size() &&
                               std::isspace((unsigned char)s[i])) ++i; }

            long parse() { long v = add(); ws(); if (i != s.size()) ok = false;
                           return v; }
            long add() {
                long v = mul();
                for (;;) { ws();
                    if (i < s.size() && (s[i] == '+' || s[i] == '-')) {
                        char op = s[i++];
                        long r = mul();
                        v = (op == '+') ? v + r : v - r;
                    } else break; }
                return v;
            }
            long mul() {
                long v = unary();
                for (;;) { ws();
                    if (i < s.size() && (s[i] == '*' || s[i] == '/')) {
                        char op = s[i++];
                        long r = unary();
                        if (op == '/') { if (r == 0) { ok = false; return 0; }
                                         v /= r; }
                        else v *= r;
                    } else break; }
                return v;
            }
            long unary() { ws();
                if (i < s.size() && s[i] == '-') { ++i; return -unary(); }
                if (i < s.size() && s[i] == '+') { ++i; return  unary(); }
                return prim();
            }
            long prim() { ws();
                if (i < s.size() && s[i] == '(') {
                    ++i; long v = add(); ws();
                    if (i < s.size() && s[i] == ')') ++i; else ok = false;
                    return v;
                }
                if (i < s.size() && std::isdigit((unsigned char)s[i])) {
                    int base = 10; size_t start = i;
                    if (s[i] == '0' && i + 1 < s.size() &&
                        (s[i + 1] == 'x' || s[i + 1] == 'X')) {
                        base = 16; i += 2; start = i;
                    }
                    while (i < s.size() && std::isalnum((unsigned char)s[i])) ++i;
                    try { return std::stol(s.substr(start, i - start), nullptr, base); }
                    catch (...) { ok = false; return 0; }
                }
                // identifier — resolve against object-like defines, recursively.
                if (i < s.size() && (std::isalpha((unsigned char)s[i]) ||
                                     s[i] == '_')) {
                    size_t start = i;
                    while (i < s.size() && mtext::is_ident_char(s[i])) ++i;
                    std::string nm = s.substr(start, i - start);
                    auto it = defs.find(nm);
                    if (it == defs.end()) { ok = false; return 0; }
                    ev sub{ it->second, defs };
                    long v = sub.parse();
                    if (!sub.ok) ok = false;
                    return v;
                }
                ok = false; return 0;
            }
        } e{ expr, defines_ };

        long v = e.parse();
        if (!e.ok)
            throw macro_error(header.file, header.line,
                              "non-constant .rept count: '" + expr + "'");
        return v;
    }

    // =========================================================================
    // Factory
    // =========================================================================

    std::unique_ptr<macro_dialect> make_gnu_macro_dialect();
    std::unique_ptr<macro_dialect> make_sdas_macro_dialect();

    std::unique_ptr<macro_dialect> make_macro_dialect(asm_mode mode)
    {
        switch (mode) {
        case asm_mode::gnu:  return make_gnu_macro_dialect();
        case asm_mode::sdcc: return make_sdas_macro_dialect();
        }
        return make_sdas_macro_dialect();
    }

} // namespace xas
