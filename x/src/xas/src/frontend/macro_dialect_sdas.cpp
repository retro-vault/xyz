// macro_dialect_sdas.cpp
//
// SDCC sdasz80 / ASxxxx macro dialect.
//
//   [label:] .macro name, a, b, ?c     ... .endm     ... .mexit
//   body references dummy args by bare name; ' concatenates; \arg inserts the
//   argument's numeric value; ?arg auto-generates a unique local label when the
//   real argument is null/missing.
//   calls: name a,b   (separators , space tab); args with separators grouped ^/ /
//   .rept n / .endm    .irp sym,list / .endm    .irpc sym,str / .endm
//   attribute directives: .narg  .nchr  .nval
//
// Reference: ASxxxx assembler manual chapter 2 "The Macro Processor"
// (vendored under x/docs/reference/sdcc/doc/sdas/asmlnk.txt).
//
// Note on auto-locals: ASxxxx emits temporary symbols of the form n$.  xas has
// no n$ local-symbol facility, so each auto-local is realised as a unique
// ordinary symbol (__macL<n>); the uniqueness contract is preserved.
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
//
#include <xas/frontend/macro.h>
#include <xas/errors.h>

#include <cctype>

namespace xas {

    namespace {

        std::string strip_caret_group(const std::string& s)
        {
            if (s.size() >= 3 && s[0] == '^') {
                char d = s[1];
                if (s.back() == d)
                    return s.substr(2, s.size() - 3);
            }
            return s;
        }

        std::string as_number(const std::string& v)
        {
            std::string t = mtext::trim(v);
            if (t.empty()) return v;
            try {
                size_t pos = 0;
                int base = 10;
                std::string body = t;
                if (t.size() > 2 && t[0] == '0' && (t[1] == 'x' || t[1] == 'X')) {
                    base = 16; body = t.substr(2);
                }
                long n = std::stol(body, &pos, base);
                if (pos == body.size()) return std::to_string(n);
            } catch (...) {}
            return v;
        }

        class sdas_macro_dialect : public macro_dialect {
        public:
            const char* name() const override { return "sdas"; }
            bool case_sensitive_names() const override { return false; }

            std::string strip_comment(const std::string& line) const override
            {
                bool in_str = false;
                for (size_t i = 0; i < line.size(); ++i) {
                    if (line[i] == '"') { in_str = !in_str; continue; }
                    if (!in_str && line[i] == ';') return line.substr(0, i);
                }
                return line;
            }

            block opener(const std::string& op) const override
            {
                if (op == ".macro") return block::macro;
                if (op == ".rept")  return block::rept;
                if (op == ".irp")   return block::irp;
                if (op == ".irpc")  return block::irpc;
                return block::none;
            }

            // In ASxxxx .endm closes macros AND repeat blocks alike.
            bool closes(const std::string& op, block) const override
            {
                return op == ".endm";
            }

            bool is_exit(const std::string& op) const override
            {
                return op == ".mexit";
            }

            macro_def parse_macro_header(const src_line& line) const override
            {
                auto os   = mtext::split_operator(line.text);     // ".macro"
                auto body = mtext::split_operator(os.operand);     // name + args
                macro_def def;
                def.name = body.op;
                if (def.name.empty())
                    throw macro_error(line.file, line.line,
                                      ".macro requires a name");
                for (std::string tok : tokenize(body.operand)) {
                    macro_param p;
                    if (!tok.empty() && tok[0] == '?') {
                        p.autolocal = true;
                        tok = tok.substr(1);
                    }
                    if (!tok.empty()) { p.name = tok; def.params.push_back(p); }
                }
                return def;
            }

            std::vector<std::string>
            split_args(const std::string& operand) const override
            {
                std::vector<std::string> args;
                std::string s = mtext::trim(operand);
                std::string buf;
                bool inarg = false;
                enum { none, space, comma, ch } last = none;
                auto flush = [&] { args.push_back(buf); buf.clear(); inarg = false; };

                for (size_t i = 0; i < s.size(); ) {
                    char c = s[i];
                    if (c == '^' && i + 1 < s.size()) {       // ^/ ... / group
                        char d = s[i + 1];
                        size_t j = i + 2;
                        std::string inner;
                        while (j < s.size() && s[j] != d) inner += s[j++];
                        if (j < s.size()) ++j;                // skip closing delim
                        buf += inner; inarg = true; last = ch; i = j; continue;
                    }
                    if (c == ',') {
                        if (inarg) flush();
                        else if (last == comma || last == none) args.push_back("");
                        last = comma; ++i; continue;
                    }
                    if (c == ' ' || c == '\t') {
                        if (inarg) { flush(); last = space; }
                        ++i; continue;
                    }
                    buf += c; inarg = true; last = ch; ++i;
                }
                if (inarg) flush();
                else if (last == comma) args.push_back("");
                return args;
            }

            macro_binding bind(const macro_def& def,
                               const std::vector<std::string>& args,
                               expand_context& ctx) const override
            {
                macro_binding b;
                for (size_t i = 0; i < def.params.size(); ++i) {
                    const macro_param& p = def.params[i];
                    std::string val = (i < args.size()) ? args[i] : std::string();
                    if (val.empty() && p.autolocal)
                        val = "__macL" + std::to_string(ctx.local_seq++);
                    b[p.name] = val;
                }
                return b;
            }

            std::string expand_line(const std::string& line,
                                    const macro_binding& binding,
                                    const expand_context&) const override
            {
                struct tok { int kind; std::string text; bool param; };
                std::vector<tok> toks;
                for (size_t i = 0; i < line.size(); ) {
                    char c = line[i];
                    if (mtext::is_ident_char(c)) {
                        size_t j = i;
                        while (j < line.size() && mtext::is_ident_char(line[j])) ++j;
                        std::string t = line.substr(i, j - i);
                        toks.push_back({ 0, t, binding.count(t) > 0 });
                        i = j;
                    } else if (c == '\'') {
                        toks.push_back({ 1, "'", false }); ++i;
                    } else if (c == '\\') {
                        toks.push_back({ 2, "\\", false }); ++i;
                    } else {
                        toks.push_back({ 3, std::string(1, c), false }); ++i;
                    }
                }

                std::string out;
                bool bslash = false;                          // pending '\'
                for (size_t k = 0; k < toks.size(); ++k) {
                    const tok& t = toks[k];
                    if (t.kind == 0) {                        // identifier
                        if (t.param) {
                            const std::string& v = binding.at(t.text);
                            out += bslash ? as_number(v) : v;
                        } else {
                            if (bslash) out += '\\';
                            out += t.text;
                        }
                        bslash = false;
                    } else if (t.kind == 1) {                 // single quote
                        bool dropL = k > 0 && toks[k - 1].kind == 0 &&
                                     toks[k - 1].param;
                        bool dropR = k + 1 < toks.size() && toks[k + 1].kind == 0 &&
                                     toks[k + 1].param;
                        if (!dropL && !dropR) out += '\'';
                        bslash = false;
                    } else if (t.kind == 2) {                 // backslash
                        bool next_param = k + 1 < toks.size() &&
                                          toks[k + 1].kind == 0 && toks[k + 1].param;
                        if (next_param) bslash = true;        // numeric substitution
                        else out += '\\';
                    } else {
                        out += t.text;
                        bslash = false;
                    }
                }
                return out;
            }

            // ASxxxx: an empty .irp/.irpc list takes no action.
            bool irp_empty_emits_once() const override { return false; }

            std::optional<std::string>
            attr_directive(const std::string& op, const std::string& operand,
                           const expand_context& ctx) const override
            {
                if (op == ".narg") {
                    std::string sym = mtext::trim(operand);
                    if (sym.empty()) return std::nullopt;
                    return sym + " = " + std::to_string(ctx.cur_argc);
                }
                if (op == ".nchr") {
                    auto comma = operand.find(',');
                    if (comma == std::string::npos) return std::nullopt;
                    std::string sym = mtext::trim(operand.substr(0, comma));
                    std::string str = strip_caret_group(
                        mtext::trim(operand.substr(comma + 1)));
                    return sym + " = " + std::to_string(str.size());
                }
                if (op == ".nval") {
                    auto comma = operand.find(',');
                    if (comma == std::string::npos) return std::nullopt;
                    std::string sym  = mtext::trim(operand.substr(0, comma));
                    std::string expr = mtext::trim(operand.substr(comma + 1));
                    return sym + " = " + expr;
                }
                return std::nullopt;
            }

        private:
            static std::vector<std::string> tokenize(const std::string& s)
            {
                std::vector<std::string> out;
                std::string cur;
                for (char c : s) {
                    if (c == ',' || c == ' ' || c == '\t') {
                        if (!cur.empty()) { out.push_back(cur); cur.clear(); }
                    } else cur += c;
                }
                if (!cur.empty()) out.push_back(cur);
                return out;
            }
        };

    } // namespace

    std::unique_ptr<macro_dialect> make_sdas_macro_dialect()
    {
        return std::make_unique<sdas_macro_dialect>();
    }

} // namespace xas
