// macro_dialect_gnu.cpp
//
// GNU as (gas) macro dialect.
//
//   .macro name a b=def a:req a:vararg   ... .endm   ... .exitm
//   body references: \arg   \@ (macro count)   \+ (iteration)   \() (separator)
//   calls: positional or key=value; args grouped by "" () [] ; "" are stripped
//   .rept n / .endr      .irp sym,list / .endr     .irpc sym,str / .endr
//
// Reference: binutils gas info manual (vendored under x/docs/reference/gnu).
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
//
#include <xas/frontend/macro.h>
#include <xas/errors.h>

#include <cctype>

namespace xas {

    namespace {

        class gnu_macro_dialect : public macro_dialect {
        public:
            const char* name() const override { return "gnu"; }
            bool case_sensitive_names() const override { return true; }

            std::string strip_comment(const std::string& line) const override
            {
                bool in_str = false;
                for (size_t i = 0; i < line.size(); ++i) {
                    char c = line[i];
                    if (c == '"') { in_str = !in_str; continue; }
                    if (in_str) continue;
                    if (c == ';') return line.substr(0, i);
                    if (c == '/' && i + 1 < line.size() && line[i + 1] == '/')
                        return line.substr(0, i);
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

            bool closes(const std::string& op, block b) const override
            {
                if (b == block::macro) return op == ".endm";
                return op == ".endr"; // rept / irp / irpc
            }

            bool is_exit(const std::string& op) const override
            {
                return op == ".exitm";
            }

            macro_def parse_macro_header(const src_line& line) const override
            {
                // line.text == ".macro name [params...]"
                auto os   = mtext::split_operator(line.text);          // ".macro"
                auto body = mtext::split_operator(os.operand);          // name + rest
                macro_def def;
                def.name = body.op;
                if (def.name.empty())
                    throw macro_error(line.file, line.line,
                                      ".macro requires a name");

                for (const std::string& tok : tokenize(body.operand)) {
                    macro_param p;
                    std::string t = tok;
                    // qualifiers / default may appear after ':' or '='
                    size_t eq = t.find('=');
                    size_t cl = t.find(':');
                    if (cl != std::string::npos &&
                        (eq == std::string::npos || cl < eq)) {
                        std::string q = t.substr(cl + 1);
                        p.name = t.substr(0, cl);
                        if (q == "req")    p.required = true;
                        else if (q == "vararg") p.vararg = true;
                    } else if (eq != std::string::npos) {
                        p.name        = t.substr(0, eq);
                        p.default_val = t.substr(eq + 1);
                        p.has_default = true;
                    } else {
                        p.name = t;
                    }
                    if (!p.name.empty()) def.params.push_back(p);
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
                    if (c == '"') {                       // quoted: strip quotes
                        ++i;
                        while (i < s.size()) {
                            if (s[i] == '"') {
                                if (i + 1 < s.size() && s[i + 1] == '"') {
                                    buf += '"'; i += 2; continue;
                                }
                                ++i; break;
                            }
                            if (s[i] == '\\' && i + 1 < s.size() && s[i + 1] == '"') {
                                buf += '"'; i += 2; continue;
                            }
                            buf += s[i++];
                        }
                        inarg = true; last = ch; continue;
                    }
                    if (c == '(' || c == '[') {           // balanced group: kept
                        char close = (c == '(') ? ')' : ']';
                        int d = 0;
                        do {
                            if (s[i] == c) ++d;
                            else if (s[i] == close) --d;
                            buf += s[i++];
                        } while (i < s.size() && d > 0);
                        inarg = true; last = ch; continue;
                    }
                    if (c == ',') {
                        if (inarg) flush();
                        else if (last == comma || last == none) args.push_back("");
                        last = comma; ++i; continue;
                    }
                    if (std::isspace(static_cast<unsigned char>(c))) {
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
                for (const macro_param& p : def.params)
                    b[p.name] = p.has_default ? p.default_val : std::string();

                size_t pos = 0;
                for (size_t a = 0; a < args.size(); ++a) {
                    const std::string& arg = args[a];
                    size_t eq = arg.find('=');
                    std::string key = eq == std::string::npos
                                          ? std::string() : arg.substr(0, eq);
                    if (eq != std::string::npos && is_param(def, key)) {
                        b[key] = arg.substr(eq + 1);          // keyword argument
                        continue;
                    }
                    // positional
                    while (pos < def.params.size()) {
                        const macro_param& p = def.params[pos];
                        if (p.vararg) {
                            std::string joined;
                            for (size_t k = a; k < args.size(); ++k) {
                                if (k > a) joined += ",";
                                joined += args[k];
                            }
                            b[p.name] = joined;
                            a = args.size();                  // consume the rest
                            break;
                        }
                        b[p.name] = arg; ++pos;
                        break;
                    }
                }
                ++ctx.global_count;                           // \@
                return b;
            }

            std::string expand_line(const std::string& line,
                                    const macro_binding& binding,
                                    const expand_context& ctx) const override
            {
                std::string out;
                for (size_t i = 0; i < line.size(); ) {
                    if (line[i] != '\\') { out += line[i++]; continue; }
                    if (i + 1 >= line.size()) { out += '\\'; ++i; continue; }
                    char d = line[i + 1];
                    if (d == '@') { out += std::to_string(ctx.global_count); i += 2; continue; }
                    if (d == '+') { out += std::to_string(ctx.plus_count);   i += 2; continue; }
                    if (d == '(' && i + 2 < line.size() && line[i + 2] == ')') {
                        i += 3; continue;                     // \() separator
                    }
                    if (std::isalpha(static_cast<unsigned char>(d)) || d == '_') {
                        size_t j = i + 1;
                        while (j < line.size() && is_name_char(line[j])) ++j;
                        std::string nm = line.substr(i + 1, j - (i + 1));
                        auto it = binding.find(nm);
                        if (it != binding.end()) { out += it->second; i = j; continue; }
                        out += '\\';                          // unknown: keep literal
                        ++i; continue;
                    }
                    out += '\\'; ++i;
                }
                return out;
            }

            bool irp_empty_emits_once() const override { return true; }

            std::optional<std::string>
            attr_directive(const std::string&, const std::string&,
                           const expand_context&) const override
            {
                return std::nullopt;
            }

        private:
            static bool is_name_char(char c)
            {
                return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
            }

            static bool is_param(const macro_def& def, const std::string& nm)
            {
                for (const macro_param& p : def.params)
                    if (p.name == nm) return true;
                return false;
            }

            // Split a definition's parameter list on commas/whitespace.
            static std::vector<std::string> tokenize(const std::string& s)
            {
                std::vector<std::string> out;
                std::string cur;
                for (char c : s) {
                    if (c == ',' || std::isspace(static_cast<unsigned char>(c))) {
                        if (!cur.empty()) { out.push_back(cur); cur.clear(); }
                    } else cur += c;
                }
                if (!cur.empty()) out.push_back(cur);
                return out;
            }
        };

    } // namespace

    std::unique_ptr<macro_dialect> make_gnu_macro_dialect()
    {
        return std::make_unique<gnu_macro_dialect>();
    }

} // namespace xas
