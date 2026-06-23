//
// preproc.cpp — built-in C preprocessor for xcc.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//
#include "frontend/preproc.h"

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <system_error>

namespace xcc {

// ── Static helpers ────────────────────────────────────────────────────────────

bool preprocessor::is_id_start(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c == '$';
}

bool preprocessor::is_id_cont(char c) {
    return is_id_start(c) || (c >= '0' && c <= '9');
}

// Normalize C99 digraph tokens %: (→ #) and %:%: (→ ##) in a string.
static std::string normalize_digraph_hashes(const std::string &s) {
    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '%' && i + 1 < s.size() && s[i+1] == ':') {
            if (i + 3 < s.size() && s[i+2] == '%' && s[i+3] == ':') {
                out += '#'; out += '#'; i += 3;
            } else {
                out += '#'; ++i;
            }
        } else {
            out += s[i];
        }
    }
    return out;
}

std::string preprocessor::pp_trim(const std::string &s) {
    size_t a = s.find_first_not_of(" \t\r");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r");
    return s.substr(a, b - a + 1);
}

std::string preprocessor::read_ident(const std::string &s, size_t pos) {
    size_t start = pos;
    while (pos < s.size() && is_id_cont(s[pos])) ++pos;
    return s.substr(start, pos - start);
}

std::string preprocessor::read_file(const std::string &path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return "";
    std::ostringstream ss;
    ss << f.rdbuf();
    std::string text = ss.str();
    if (text.size() >= 3 &&
        (unsigned char)text[0] == 0xef &&
        (unsigned char)text[1] == 0xbb &&
        (unsigned char)text[2] == 0xbf) {
        text.erase(0, 3);
    }
    return text;
}

static std::string canonical_include_key(const std::string &path) {
    std::error_code ec;
    auto canonical = std::filesystem::weakly_canonical(path, ec);
    if (!ec)
        return canonical.string();

    auto absolute = std::filesystem::absolute(path, ec);
    if (!ec)
        return absolute.lexically_normal().string();

    return std::filesystem::path(path).lexically_normal().string();
}

// ── Constructor ───────────────────────────────────────────────────────────────

preprocessor::preprocessor(diag_engine                   &diag,
                           const std::vector<std::string> &include_paths,
                           const std::vector<std::string> &cmdline_defines)
    : diag_(diag), include_paths_(include_paths) {
    // Standard predefined macros
    auto predef = [&](const char *name, const char *body) {
        macro_def m; m.body = body; macros_[name] = m;
    };
    predef("__STDC__",                 "1");
    predef("__STDC_HOSTED__",          "1");
    predef("__STDC_VERSION__",         "202311L"); // C23
    predef("__XCC__",                  "1");
    predef("__xcc__",                  "1");
    // xcc accepts the common GNU attributes and keywords used by portable C
    // libraries, so advertise a conservative GCC-compatible preprocessor face.
    predef("__GNUC__",                 "4");
    predef("__GNUC_MINOR__",           "2");
    predef("__GNUC_PATCHLEVEL__",      "1");
    predef("__VERSION__",              "\"xcc\"");
    predef("__CHAR_BIT__",             "8");
    predef("__SCHAR_MAX__",            "127");
    predef("__SHRT_MAX__",             "32767");
    predef("__INT_MAX__",              "32767");
    predef("__LONG_MAX__",             "2147483647L");
    predef("__LONG_LONG_MAX__",        "9223372036854775807LL");
    predef("__SIZEOF_SHORT__",         "2");
    predef("__SIZEOF_INT__",           "2");
    predef("__SIZEOF_LONG__",          "4");
    predef("__SIZEOF_LONG_LONG__",     "8");
    predef("__SIZEOF_POINTER__",       "2");
    predef("__SIZE_TYPE__",            "unsigned int");
    predef("__PTRDIFF_TYPE__",         "int");
    predef("__WCHAR_TYPE__",           "int");
    // Width macros (bits)
    predef("__CHAR_WIDTH__",           "8");
    predef("__SHRT_WIDTH__",           "16");
    predef("__INT_WIDTH__",            "16");
    predef("__LONG_WIDTH__",           "32");
    predef("__LONG_LONG_WIDTH__",      "64");
    predef("__LLONG_WIDTH__",         "64"); // GCC alias
    predef("__INTMAX_WIDTH__",         "64");
    predef("__INTMAX_MAX__",           "9223372036854775807LL");
    predef("__UINTMAX_MAX__",          "18446744073709551615ULL");
    predef("__INT8_MAX__",             "127");
    predef("__INT16_MAX__",            "32767");
    predef("__INT32_MAX__",            "2147483647L");
    predef("__INT64_MAX__",            "9223372036854775807LL");
    predef("__UINT8_MAX__",            "255");
    predef("__UINT16_MAX__",           "65535U");
    predef("__UINT32_MAX__",           "4294967295UL");
    predef("__UINT64_MAX__",           "18446744073709551615ULL");
    predef("__UCHAR_MAX__",            "255");
    predef("__USHRT_MAX__",            "65535U");
    predef("__UINT_MAX__",             "65535U");
    predef("__ULONG_MAX__",            "4294967295UL");
    predef("__SIZE_MAX__",             "65535U");    // sizeof(size_t)=2 on Z80
    predef("__PTRDIFF_MAX__",          "32767");
    predef("__WCHAR_MAX__",            "32767");
    // C23 feature-test macros.  Do not define __STDC_NO_VLA__: xcc supports
    // basic block-scope VLAs and VLA parameters.
    predef("__STDC_UTF_16__",          "1");
    predef("__STDC_UTF_32__",          "1");

    for (auto &def : cmdline_defines) {
        size_t eq = def.find('=');
        std::string name = (eq == std::string::npos) ? def : def.substr(0, eq);
        std::string body = (eq == std::string::npos) ? "1"  : def.substr(eq + 1);
        macro_def m;
        m.body = body;
        macros_[name] = m;
    }
    // __FILE__, __LINE__, __DATE__, __TIME__ are injected at expansion time.
}

// ── Public entry point ────────────────────────────────────────────────────────

std::string preprocessor::process(const std::string &source,
                                  const std::string &filename) {
    std::string out;
    process_text(source, filename, out, 0);
    return out;
}

// ── find_include ──────────────────────────────────────────────────────────────

std::string preprocessor::find_include(const std::string &name,
                                       bool               system_include,
                                       const std::string &current_dir) const {
    auto try_open = [](const std::string &p) -> bool {
        std::ifstream f(p);
        return f.good();
    };

    // Absolute path or path that exists directly (handles __FILE__ computed includes).
    if (!name.empty() && (name[0] == '/' || try_open(name))) return name;

    if (!system_include) {
        std::string rel = current_dir.empty() ? name : current_dir + "/" + name;
        if (try_open(rel)) return rel;
    }
    for (auto &dir : include_paths_) {
        std::string p = dir + "/" + name;
        if (try_open(p)) return p;
    }
    return "";
}

// ── parse_args ────────────────────────────────────────────────────────────────

std::vector<std::string> preprocessor::parse_args(const std::string &text,
                                                   size_t &pos) {
    // pos must point at '('
    assert(text[pos] == '(');
    ++pos;

    std::vector<std::string> args;
    int depth = 0;
    std::string cur;

    while (pos < text.size()) {
        char c = text[pos++];
        // Skip string and char literals verbatim — don't treat '(' / ')' / ','
        // inside them as argument separators or paren depth changes.
        if (c == '"' || c == '\'') {
            char q = c;
            cur += c;
            while (pos < text.size()) {
                char ic = text[pos++];
                cur += ic;
                if (ic == '\\' && pos < text.size()) { cur += text[pos++]; }
                else if (ic == q) break;
            }
            continue;
        }
        if (c == '(' ) { ++depth; cur += c; }
        else if (c == ')') {
            if (depth == 0) {
                args.push_back(pp_trim(cur));
                return args;
            }
            --depth;
            cur += c;
        } else if (c == ',' && depth == 0) {
            args.push_back(pp_trim(cur));
            cur.clear();
        } else {
            cur += c;
        }
    }
    // Unterminated — return what we have
    args.push_back(pp_trim(cur));
    return args;
}

// ── expand ────────────────────────────────────────────────────────────────────

std::string preprocessor::expand(const std::string &text,
                                 std::vector<std::string> &guard) const {
    std::string result;
    size_t i = 0;

    // Build __DATE__ / __TIME__ once
    static std::string s_date, s_time;
    if (s_date.empty()) {
        time_t t = time(nullptr);
        char   buf[32];
        struct tm *tm = localtime(&t);
        strftime(buf, sizeof(buf), "\"%b %d %Y\"", tm);
        s_date = buf;
        strftime(buf, sizeof(buf), "\"%H:%M:%S\"", tm);
        s_time = buf;
    }

    while (i < text.size()) {
        // String literals: copy verbatim
        if (text[i] == '"' || text[i] == '\'') {
            char q = text[i];
            result += text[i++];
            while (i < text.size()) {
                char c = text[i++];
                result += c;
                if (c == '\\' && i < text.size()) { result += text[i++]; }
                else if (c == q) break;
            }
            continue;
        }

        // Line comments
        if (i + 1 < text.size() && text[i] == '/' && text[i+1] == '/') {
            while (i < text.size() && text[i] != '\n') ++i;
            continue;
        }
        // Block comments
        if (i + 1 < text.size() && text[i] == '/' && text[i+1] == '*') {
            i += 2;
            while (i + 1 < text.size() && !(text[i] == '*' && text[i+1] == '/'))
                ++i;
            if (i + 1 < text.size()) i += 2;
            result += ' ';
            continue;
        }

        if (!is_id_start(text[i])) { result += text[i++]; continue; }

        // Read identifier
        std::string id = read_ident(text, i);
        i += id.size();

        // C11 §6.10.1p4: defined(X) and defined X — the argument must NOT be
        // macro-expanded. Emit the entire defined(...) / defined X verbatim.
        if (id == "defined") {
            result += id;
            size_t j = i;
            while (j < text.size() && (text[j] == ' ' || text[j] == '\t')) ++j;
            if (j < text.size() && text[j] == '(') {
                // defined(X): copy through closing ')'
                while (i <= j) result += text[i++]; // up to and including '('
                int depth = 1;
                while (i < text.size() && depth > 0) {
                    char c = text[i++];
                    result += c;
                    if (c == '(') ++depth;
                    else if (c == ')') --depth;
                }
            } else if (j < text.size() && is_id_start(text[j])) {
                // defined X: copy whitespace + identifier verbatim
                while (i < j) result += text[i++];
                std::string arg = read_ident(text, j);
                result += arg;
                i = j + arg.size();
            }
            continue;
        }

        // Built-in dynamic macros — resolved using the call-site context set
        // in expand_lineno_ / expand_file_ before expand() is called.
        if (id == "__LINE__") { result += std::to_string(expand_lineno_); continue; }
        if (id == "__FILE__") { result += '"'; result += expand_file_; result += '"'; continue; }
        if (id == "__DATE__") { result += s_date; continue; }
        if (id == "__TIME__") { result += s_time; continue; }
        if (id == "__COUNTER__") { result += std::to_string(counter_++); continue; }

        auto it = macros_.find(id);
        if (it == macros_.end()) { result += id; continue; }

        // Recursion guard
        if (std::find(guard.begin(), guard.end(), id) != guard.end()) {
            result += id; continue;
        }

        const macro_def &m = it->second;

        if (!m.is_function_like) {
            guard.push_back(id);
            result += expand(m.body, guard);
            guard.pop_back();
            continue;
        }

        // Function-like: look for '('
        size_t j = i;
        while (j < text.size() && (text[j] == ' ' || text[j] == '\t')) ++j;
        if (j >= text.size() || text[j] != '(') {
            // No argument list — leave unexpanded
            result += id;
            continue;
        }
        i = j;
        std::vector<std::string> args = parse_args(text, i);

        // Build substituted body
        const auto &params = m.params;
        std::string body = m.body;

        // Handle variadic: __VA_ARGS__ and __VA_OPT__(tokens)
        if (m.is_variadic) {
            std::string va;
            size_t fixed = params.size();
            for (size_t k = fixed; k < args.size(); ++k) {
                if (k > fixed) va += ", ";
                va += args[k];
            }
            // C23 §6.10.3.6: __VA_OPT__ expands iff the token sequence produced
            // by expanding __VA_ARGS__ is non-empty.  Expand va before testing.
            std::vector<std::string> va_guard;
            std::string va_expanded = expand(va, va_guard);
            // Trim whitespace for the emptiness check.
            size_t s0 = va_expanded.find_first_not_of(" \t");
            bool va_nonempty = (s0 != std::string::npos);
            std::string tmp;
            size_t p = 0;
            while (p < body.size()) {
                // __VA_OPT__(tokens) — C23: expand to tokens if __VA_ARGS__ non-empty
                if (body.substr(p, 10) == "__VA_OPT__") {
                    p += 10;
                    // skip optional whitespace, consume '('
                    while (p < body.size() && body[p] == ' ') ++p;
                    if (p < body.size() && body[p] == '(') ++p;
                    std::string inner;
                    int depth = 1;
                    while (p < body.size() && depth > 0) {
                        if (body[p] == '(') { depth++; inner += body[p++]; }
                        else if (body[p] == ')') { if (--depth > 0) inner += body[p]; p++; }
                        else inner += body[p++];
                    }
                    if (va_nonempty) tmp += inner; // expand to inner content
                    // else: expand to nothing (omit)
                    continue;
                }
                if (body.substr(p, 11) == "__VA_ARGS__") {
                    tmp += va;
                    p += 11;
                } else {
                    tmp += body[p++];
                }
            }
            body = tmp;
        }

        // Substitute named parameters.
        // C standard §6.10.3.1: arguments adjacent to ## are NOT expanded
        // before substitution; all other arguments ARE fully expanded first.
        for (size_t pi = 0; pi < params.size(); ++pi) {
            const std::string &param = params[pi];
            const std::string &raw   = (pi < args.size()) ? args[pi] : "";
            // Pre-expand for non-## contexts
            std::vector<std::string> ag;
            std::string exp = expand(raw, ag);

            std::string sub;
            size_t p = 0;
            while (p < body.size()) {
                // Token-paste operator: pass ## through unchanged
                if (body[p] == '#' && p + 1 < body.size() && body[p+1] == '#') {
                    sub += '#'; sub += '#'; p += 2; continue;
                }
                // Stringify operator
                if (body[p] == '#' && p + 1 < body.size() && body[p+1] != '#') {
                    ++p;
                    while (p < body.size() && body[p] == ' ') ++p;
                    std::string tok = read_ident(body, p);
                    if (tok == param) {
                        sub += '"';
                        for (char c : raw) {   // stringify uses raw arg
                            if (c == '"' || c == '\\') sub += '\\';
                            sub += c;
                        }
                        sub += '"';
                        p += tok.size();
                    } else {
                        sub += '#';
                    }
                    continue;
                }
                if (is_id_start(body[p])) {
                    std::string tok = read_ident(body, p);
                    if (tok == param) {
                        // Check if adjacent to ## (before or after)
                        size_t after = p + tok.size();
                        while (after < body.size() && body[after] == ' ') ++after;
                        bool after_paste = (after + 1 < body.size() &&
                                            body[after] == '#' && body[after+1] == '#');
                        // Check before: walk backward in sub
                        size_t back = sub.size();
                        while (back > 0 && sub[back-1] == ' ') --back;
                        bool before_paste = (back >= 2 &&
                                             sub[back-2] == '#' && sub[back-1] == '#');
                        bool adjacent = after_paste || before_paste;
                        // Use raw arg when adjacent to ##, expanded otherwise.
                        // When adjacent and empty, use placemarker \x01 so that
                        // the ## is consumed but no tokens are pasted across it.
                        const std::string &val = adjacent ? raw : exp;
                        sub += (adjacent && val.empty()) ? "\x01" : val;
                        p += tok.size();
                    } else {
                        sub += tok;
                        p += tok.size();
                    }
                } else {
                    sub += body[p++];
                }
            }
            body = sub;
        }

        // Token-paste ##
        // \x01 is a placemarker for an empty argument adjacent to ##.
        // C99 §6.10.3.3: if either operand is a placemarker, the result
        // is the non-placemarker operand (no actual concatenation).
        {
            std::string pasted;
            size_t p = 0;
            while (p < body.size()) {
                if (p + 1 < body.size() && body[p] == '#' && body[p+1] == '#') {
                    bool left_mark = !pasted.empty() && pasted.back() == '\x01';
                    // Remove trailing whitespace and placemarker from left operand
                    while (!pasted.empty() && pasted.back() == ' ') pasted.pop_back();
                    if (!pasted.empty() && pasted.back() == '\x01') {
                        pasted.pop_back();
                        left_mark = true;
                    }
                    p += 2;
                    while (p < body.size() && body[p] == ' ') ++p;
                    // Check for right-side placemarker
                    bool right_mark = (p < body.size() && body[p] == '\x01');
                    if (right_mark) {
                        ++p;
                        // Placemarker on right: left side is kept as-is, no concat.
                        // Add a space to separate from next token.
                        pasted += ' ';
                    } else if (left_mark) {
                        // Placemarker on left: right side becomes the result.
                        // The right token will be appended naturally in next iteration.
                    }
                    // If neither is a placemarker, the ## concatenation already
                    // removed trailing whitespace; the next character appends directly.
                } else {
                    pasted += body[p++];
                }
            }
            // Remove any remaining placemarkers
            std::string clean;
            for (char c : pasted) if (c != '\x01') clean += c;
            body = clean;
        }

        guard.push_back(id);
        result += expand(body, guard);
        guard.pop_back();
    }
    return result;
}

// ── eval_if ───────────────────────────────────────────────────────────────────
// Tiny recursive-descent evaluator for #if/#elif expressions.
// Handles: integer literals, defined(X)/defined X, !, ~, *, /, %, +, -, <<,
// >>, <, <=, >, >=, ==, !=, &, ^, |, &&, ||, ternary ?: , parentheses.

namespace {

struct eval_ctx {
    const std::string &expr;
    size_t pos;
    const std::unordered_map<std::string, macro_def> &macros;
    const std::vector<std::string> *include_paths = nullptr;
    const std::string *current_dir = nullptr;

    void skip_ws() {
        while (pos < expr.size() && (expr[pos] == ' ' || expr[pos] == '\t'))
            ++pos;
    }

    bool try_open(const std::string &p) const {
        std::ifstream f(p);
        return f.good();
    }

    bool has_include_file(const std::string &name, bool system_include) const {
        if (!name.empty() && (name[0] == '/' || try_open(name))) return true;
        if (!system_include && current_dir) {
            std::string rel = current_dir->empty() ? name : (*current_dir + "/" + name);
            if (try_open(rel)) return true;
        }
        if (include_paths) {
            for (auto &dir : *include_paths) {
                if (try_open(dir + "/" + name)) return true;
            }
        }
        return false;
    }

    long long parse_primary() {
        skip_ws();
        if (pos >= expr.size()) return 0;

        // Parentheses
        if (expr[pos] == '(') {
            ++pos;
            long long v = parse_ternary();
            skip_ws();
            if (pos < expr.size() && expr[pos] == ')') ++pos;
            return v;
        }

        // Unary operators
        if (expr[pos] == '!') { ++pos; return !parse_primary(); }
        if (expr[pos] == '~') { ++pos; return ~parse_primary(); }
        if (expr[pos] == '-') { ++pos; return -parse_primary(); }
        if (expr[pos] == '+') { ++pos; return  parse_primary(); }

        // C23: __has_include("file") / __has_include(<file>)
        if (expr.substr(pos, 13) == "__has_include") {
            pos += 13;
            skip_ws();
            if (pos < expr.size() && expr[pos] == '(') ++pos;
            skip_ws();
            bool system_include = false;
            std::string name;
            if (pos < expr.size() && expr[pos] == '"') {
                ++pos;
                while (pos < expr.size() && expr[pos] != '"') name += expr[pos++];
                if (pos < expr.size() && expr[pos] == '"') ++pos;
            } else if (pos < expr.size() && expr[pos] == '<') {
                system_include = true;
                ++pos;
                while (pos < expr.size() && expr[pos] != '>') name += expr[pos++];
                if (pos < expr.size() && expr[pos] == '>') ++pos;
            }
            skip_ws();
            if (pos < expr.size() && expr[pos] == ')') ++pos;
            return has_include_file(name, system_include) ? 1 : 0;
        }

        // C23: __has_c_attribute([[ns::name]]) — 0 for unknown, 1 for known C23 attrs
        if (expr.substr(pos, 18) == "__has_c_attribute") {
            pos += 18;
            // Collect the attribute name from inside the parens
            std::string attr_arg;
            int depth = 0;
            while (pos < expr.size() && !(depth == 0 && expr[pos] == ')')) {
                if (expr[pos] == '(') depth++;
                else if (expr[pos] == ')') depth--;
                else attr_arg += expr[pos];
                ++pos;
            }
            if (pos < expr.size()) ++pos;
            // Known standard C23 attributes (return version number per spec)
            for (auto &ch : attr_arg) if (ch == ' ' || ch == '\t') ch = 0;
            if (attr_arg == "noreturn" || attr_arg == "deprecated" ||
                attr_arg == "nodiscard" || attr_arg == "maybe_unused" ||
                attr_arg == "fallthrough" || attr_arg == "unsequenced" ||
                attr_arg == "reproducible")
                return 202311L; // C23 version stamp
            return 0;
        }

        // defined(X) or defined X
        if (expr.substr(pos, 7) == "defined") {
            pos += 7;
            skip_ws();
            bool paren = (pos < expr.size() && expr[pos] == '(');
            if (paren) ++pos;
            skip_ws();
            std::string id;
            while (pos < expr.size() && (isalnum(expr[pos]) || expr[pos] == '_'))
                id += expr[pos++];
            skip_ws();
            if (paren && pos < expr.size() && expr[pos] == ')') ++pos;
            return macros.count(id) ? 1 : 0;
        }

        // Numeric literal (decimal, hex, octal)
        if (isdigit(expr[pos])) {
            unsigned long long u = 0;
            if (expr[pos] == '0' && pos + 1 < expr.size() &&
                (expr[pos+1] == 'x' || expr[pos+1] == 'X')) {
                pos += 2;
                while (pos < expr.size() && isxdigit((unsigned char)expr[pos])) {
                    u = u * 16 + (isdigit((unsigned char)expr[pos]) ? expr[pos]-'0'
                                                     : tolower((unsigned char)expr[pos])-'a'+10);
                    ++pos;
                }
            } else {
                bool octal = expr[pos] == '0';
                while (pos < expr.size() && isdigit((unsigned char)expr[pos]))
                    u = u * (octal ? 8 : 10) + (unsigned)(expr[pos++] - '0');
            }
            // Skip integer suffixes
            while (pos < expr.size() && (expr[pos]=='u'||expr[pos]=='U'||
                                          expr[pos]=='l'||expr[pos]=='L'))
                ++pos;
            return (long long)u;
        }

        // Character literal
        if (expr[pos] == '\'') {
            ++pos;
            long long v = 0;
            if (pos < expr.size() && expr[pos] == '\\') {
                ++pos;
                if (pos < expr.size()) {
                    switch (expr[pos++]) {
                    case 'n': v = '\n'; break;
                    case 't': v = '\t'; break;
                    case 'r': v = '\r'; break;
                    case '0': v = 0;   break;
                    default:  v = expr[pos-1]; break;
                    }
                }
            } else if (pos < expr.size()) {
                v = (unsigned char)expr[pos++];
            }
            if (pos < expr.size() && expr[pos] == '\'') ++pos;
            return v;
        }

        // Identifier (undefined macro → 0)
        if (isalpha(expr[pos]) || expr[pos] == '_') {
            while (pos < expr.size() && (isalnum(expr[pos]) || expr[pos]=='_'))
                ++pos;
            return 0;
        }

        return 0;
    }

    long long parse_mul() {
        long long v = parse_primary();
        for (;;) {
            skip_ws();
            if (pos >= expr.size()) break;
            char c = expr[pos];
            // Use unsigned wrapping to avoid signed-overflow UB (C11 §6.10.1).
            if (c == '*') { ++pos; v = (long long)((unsigned long long)v * (unsigned long long)parse_primary()); }
            else if (c == '/') { ++pos; long long r = parse_primary(); v = r ? v/r : 0; }
            else if (c == '%') { ++pos; long long r = parse_primary(); v = r ? v%r : 0; }
            else break;
        }
        return v;
    }

    long long parse_add() {
        long long v = parse_mul();
        for (;;) {
            skip_ws();
            if (pos >= expr.size()) break;
            char c = expr[pos];
            // Use unsigned wrapping to avoid signed-overflow UB (C11 §6.10.1).
            if (c == '+') { ++pos; v = (long long)((unsigned long long)v + (unsigned long long)parse_mul()); }
            else if (c == '-') { ++pos; v = (long long)((unsigned long long)v - (unsigned long long)parse_mul()); }
            else break;
        }
        return v;
    }

    long long parse_shift() {
        long long v = parse_add();
        for (;;) {
            skip_ws();
            if (pos + 1 >= expr.size()) break;
            if (expr[pos]=='<' && expr[pos+1]=='<') { pos+=2; v <<= parse_add(); }
            else if (expr[pos]=='>' && expr[pos+1]=='>') { pos+=2; v >>= parse_add(); }
            else break;
        }
        return v;
    }

    long long parse_rel() {
        long long v = parse_shift();
        for (;;) {
            skip_ws();
            if (pos >= expr.size()) break;
            if (pos+1 < expr.size() && expr[pos]=='<' && expr[pos+1]=='=')
                { pos+=2; v = (v <= parse_shift()); }
            else if (pos+1 < expr.size() && expr[pos]=='>' && expr[pos+1]=='=')
                { pos+=2; v = (v >= parse_shift()); }
            else if (expr[pos]=='<' && (pos+1>=expr.size()||expr[pos+1]!='<'))
                { ++pos; v = (v < parse_shift()); }
            else if (expr[pos]=='>' && (pos+1>=expr.size()||expr[pos+1]!='>'))
                { ++pos; v = (v > parse_shift()); }
            else break;
        }
        return v;
    }

    long long parse_eq() {
        long long v = parse_rel();
        for (;;) {
            skip_ws();
            if (pos + 1 >= expr.size()) break;
            if (expr[pos]=='=' && expr[pos+1]=='=') { pos+=2; v = (v == parse_rel()); }
            else if (expr[pos]=='!' && expr[pos+1]=='=') { pos+=2; v = (v != parse_rel()); }
            else break;
        }
        return v;
    }

    long long parse_bitand() {
        long long v = parse_eq();
        for (;;) {
            skip_ws();
            if (pos >= expr.size()) break;
            if (expr[pos]=='&' && (pos+1>=expr.size()||expr[pos+1]!='&'))
                { ++pos; v &= parse_eq(); }
            else break;
        }
        return v;
    }

    long long parse_bitxor() {
        long long v = parse_bitand();
        for (;;) {
            skip_ws();
            if (pos >= expr.size()) break;
            if (expr[pos]=='^') { ++pos; v ^= parse_bitand(); }
            else break;
        }
        return v;
    }

    long long parse_bitor() {
        long long v = parse_bitxor();
        for (;;) {
            skip_ws();
            if (pos >= expr.size()) break;
            if (expr[pos]=='|' && (pos+1>=expr.size()||expr[pos+1]!='|'))
                { ++pos; v |= parse_bitxor(); }
            else break;
        }
        return v;
    }

    long long parse_logand() {
        long long v = parse_bitor();
        for (;;) {
            skip_ws();
            if (pos + 1 >= expr.size()) break;
            if (expr[pos]=='&' && expr[pos+1]=='&')
                { pos+=2; long long r = parse_bitor(); v = (v && r); }
            else break;
        }
        return v;
    }

    long long parse_logor() {
        long long v = parse_logand();
        for (;;) {
            skip_ws();
            if (pos + 1 >= expr.size()) break;
            if (expr[pos]=='|' && expr[pos+1]=='|')
                { pos+=2; long long r = parse_logand(); v = (v || r); }
            else break;
        }
        return v;
    }

    long long parse_ternary() {
        long long cond = parse_logor();
        skip_ws();
        if (pos < expr.size() && expr[pos] == '?') {
            ++pos;
            long long t = parse_ternary();
            skip_ws();
            if (pos < expr.size() && expr[pos] == ':') ++pos;
            long long f = parse_ternary();
            return cond ? t : f;
        }
        return cond;
    }
};

} // anonymous namespace

long long preprocessor::eval_if(const std::string &expr,
                                const std::string &current_dir) const {
    eval_ctx ctx{expr, 0, macros_, &include_paths_, &current_dir};
    return ctx.parse_ternary();
}

// ── process_text ──────────────────────────────────────────────────────────────

void preprocessor::process_text(const std::string &source,
                                 const std::string &filename,
                                 std::string       &out,
                                 int                depth) {
    if (depth > 32) {
        diag_.fatal("#include too deeply nested (max 32)");
    }

    const std::string file_key = canonical_include_key(filename);

    // Conditional stack: each entry = {active, branch_taken, has_else}
    struct cond_frame {
        bool active;       // are we currently emitting lines?
        bool branch_taken; // has any branch in this #if chain been taken?
        bool has_else;     // did we see #else?
    };
    std::vector<cond_frame> cond_stack;

    auto emitting = [&]() -> bool {
        for (auto &f : cond_stack)
            if (!f.active) return false;
        return true;
    };

    // Current directory for relative #includes
    std::string cur_dir;
    {
        size_t slash = filename.rfind('/');
        if (slash != std::string::npos) cur_dir = filename.substr(0, slash);
    }

    // Date/time strings for __DATE__ and __TIME__
    static std::string s_date, s_time;
    if (s_date.empty()) {
        time_t t = time(nullptr);
        char buf[32];
        struct tm *tm = localtime(&t);
        strftime(buf, sizeof(buf), "\"%b %d %Y\"", tm);
        s_date = buf;
        strftime(buf, sizeof(buf), "\"%H:%M:%S\"", tm);
        s_time = buf;
    }

    // Emit a line marker so the lexer knows where we are.
    auto emit_line_marker = [&](int lineno, const std::string &fname) {
        out += "# ";
        out += std::to_string(lineno);
        out += " \"";
        out += fname;
        out += "\"\n";
    };

    emit_line_marker(1, filename);

    std::string input = source;
    if (input.size() >= 3 &&
        (unsigned char)input[0] == 0xef &&
        (unsigned char)input[1] == 0xbb &&
        (unsigned char)input[2] == 0xbf) {
        input.erase(0, 3);
    }

    // Strip multi-line block comments before line-by-line processing.
    // Replaces comment content with spaces, preserving newlines for line numbers.
    std::string stripped;
    stripped.reserve(input.size());
    {
        size_t n = input.size();
        size_t i = 0;
        while (i < n) {
            if (i + 1 < n && input[i] == '/' && input[i+1] == '*') {
                i += 2;
                while (i < n) {
                    if (i + 1 < n && input[i] == '*' && input[i+1] == '/') {
                        i += 2;
                        break;
                    }
                    stripped += (input[i] == '\n') ? '\n' : ' ';
                    ++i;
                }
                stripped += ' ';
            } else {
                stripped += input[i++];
            }
        }
    }

    std::istringstream ss(stripped);
    std::string line;
    int lineno = 0;

    while (std::getline(ss, line)) {
        ++lineno;
        int logical_lineno = lineno; // __LINE__ uses start of logical line

        // Handle line splicing (backslash-newline)
        while (!line.empty() && line.back() == '\\') {
            line.pop_back();
            std::string cont;
            if (!std::getline(ss, cont)) break;
            ++lineno;
            line += cont;
        }

        // Update expansion context for __LINE__ / __FILE__ in macro bodies.
        expand_lineno_ = logical_lineno;
        expand_file_   = filename;

        std::string trimmed = pp_trim(line);

        // Substitute __LINE__ / __FILE__ in any string (used for directives).
        auto subst_line_file = [&](const std::string &s) -> std::string {
            std::string out2;
            size_t sp = 0;
            while (sp < s.size()) {
                if (is_id_start(s[sp])) {
                    std::string id = read_ident(s, sp);
                    if (id == "__LINE__")      out2 += std::to_string(logical_lineno);
                    else if (id == "__FILE__") { out2 += '"'; out2 += filename; out2 += '"'; }
                    else                       out2 += id;
                    sp += id.size();
                } else {
                    out2 += s[sp++];
                }
            }
            return out2;
        };

        // Preprocessor directive?
        if (!trimmed.empty() && trimmed[0] == '#') {
            size_t p = 1;
            while (p < trimmed.size() && (trimmed[p] == ' ' || trimmed[p] == '\t')) ++p;
            std::string dir = read_ident(trimmed, p);
            p += dir.size();
            while (p < trimmed.size() && (trimmed[p] == ' ' || trimmed[p] == '\t')) ++p;
            // For #define, preserve __LINE__/__FILE__ verbatim so they resolve
            // at expansion time; for all other directives, substitute now.
            std::string raw_rest = pp_trim(trimmed.substr(p));
            std::string rest = (dir == "define") ? raw_rest : subst_line_file(raw_rest);

            // ── Conditionals that affect the stack regardless of emitting state ──

            if (dir == "ifdef") {
                bool defined = macros_.count(rest) > 0;
                bool act     = emitting() && defined;
                cond_stack.push_back({act, act, false});
                continue;
            }
            if (dir == "ifndef") {
                bool defined = macros_.count(rest) > 0;
                bool act     = emitting() && !defined;
                cond_stack.push_back({act, act, false});
                continue;
            }
            if (dir == "if") {
                bool act = false;
                if (emitting()) {
                    std::vector<std::string> guard;
                    std::string expanded = expand(rest, guard);
                    act = (eval_if(expanded, cur_dir) != 0);
                }
                cond_stack.push_back({act, act, false});
                continue;
            }
            if (dir == "elif") {
                if (cond_stack.empty()) {
                    diag_.fatal(filename.c_str(), lineno, "#elif without #if");
                }
                auto &f = cond_stack.back();
                if (f.has_else) {
                    diag_.fatal(filename.c_str(), lineno, "#elif after #else");
                }
                if (!f.branch_taken) {
                    // Check whether parent is emitting (all outer frames)
                    bool parent_emit = true;
                    for (size_t k = 0; k + 1 < cond_stack.size(); ++k)
                        if (!cond_stack[k].active) { parent_emit = false; break; }
                    if (parent_emit) {
                        std::vector<std::string> guard;
                        std::string expanded = expand(rest, guard);
                        bool act = (eval_if(expanded, cur_dir) != 0);
                        f.active = act;
                        f.branch_taken = act;
                    }
                } else {
                    f.active = false;
                }
                continue;
            }
            // C23: #elifdef sym == #elif defined(sym)
            if (dir == "elifdef" || dir == "elifndef") {
                if (cond_stack.empty())
                    diag_.fatal(filename.c_str(), lineno,
                                "#%s without #if", dir.c_str());
                auto &f = cond_stack.back();
                if (f.has_else)
                    diag_.fatal(filename.c_str(), lineno,
                                "#%s after #else", dir.c_str());
                if (!f.branch_taken) {
                    bool parent_emit = true;
                    for (size_t k = 0; k + 1 < cond_stack.size(); ++k)
                        if (!cond_stack[k].active) { parent_emit = false; break; }
                    if (parent_emit) {
                        bool defined = macros_.count(rest) > 0;
                        bool act = (dir == "elifdef") ? defined : !defined;
                        f.active = act;
                        f.branch_taken = act;
                    }
                } else {
                    f.active = false;
                }
                continue;
            }
            if (dir == "else") {
                if (cond_stack.empty()) {
                    diag_.fatal(filename.c_str(), lineno, "#else without #if");
                }
                auto &f = cond_stack.back();
                if (f.has_else) {
                    diag_.fatal(filename.c_str(), lineno, "duplicate #else");
                }
                f.has_else = true;
                // Parent must be emitting
                bool parent_emit = true;
                for (size_t k = 0; k + 1 < cond_stack.size(); ++k)
                    if (!cond_stack[k].active) { parent_emit = false; break; }
                f.active = parent_emit && !f.branch_taken;
                continue;
            }
            if (dir == "endif") {
                if (cond_stack.empty()) {
                    diag_.fatal(filename.c_str(), lineno, "#endif without #if");
                }
                cond_stack.pop_back();
                // Restore line marker after conditional block
                if (emitting()) emit_line_marker(lineno + 1, filename);
                continue;
            }

            // ── Directives only processed when emitting ──────────────────────

            if (!emitting()) continue;

            if (dir == "define") {
                // Parse: name [ ( params ) ] body
                size_t q = 0;
                std::string name = read_ident(rest, q);
                q += name.size();

                macro_def m;
                if (q < rest.size() && rest[q] == '(') {
                    m.is_function_like = true;
                    ++q; // skip '('
                    while (q < rest.size() && rest[q] != ')') {
                        while (q < rest.size() && rest[q] == ' ') ++q;
                        if (rest.substr(q, 3) == "...") {
                            m.is_variadic = true;
                            q += 3;
                            break;
                        }
                        std::string param = read_ident(rest, q);
                        q += param.size();
                        if (!param.empty()) m.params.push_back(param);
                        while (q < rest.size() && rest[q] == ' ') ++q;
                        if (q < rest.size() && rest[q] == ',') ++q;
                    }
                    if (q < rest.size() && rest[q] == ')') ++q;
                    while (q < rest.size() && rest[q] == ' ') ++q;
                    m.body = normalize_digraph_hashes(rest.substr(q));
                } else {
                    if (q < rest.size() && rest[q] == ' ') ++q;
                    m.body = normalize_digraph_hashes(q < rest.size() ? rest.substr(q) : "");
                }
                macros_[name] = m;
                continue;
            }

            if (dir == "undef") {
                macros_.erase(rest);
                continue;
            }

            if (dir == "include") {
                bool sys = false;
                std::string inc_name;
                if (!rest.empty() && rest[0] == '<') {
                    sys = true;
                    size_t end = rest.find('>');
                    inc_name = rest.substr(1, end == std::string::npos ? rest.size()-1 : end-1);
                } else if (!rest.empty() && rest[0] == '"') {
                    size_t end = rest.find('"', 1);
                    inc_name = rest.substr(1, end == std::string::npos ? rest.size()-1 : end-1);
                } else {
                    // Macro-expanded form
                    std::vector<std::string> guard;
                    std::string expanded = pp_trim(expand(rest, guard));
                    if (!expanded.empty() && expanded[0] == '<') {
                        sys = true;
                        size_t end = expanded.find('>');
                        inc_name = expanded.substr(1, end==std::string::npos ? expanded.size()-1 : end-1);
                    } else if (!expanded.empty() && expanded[0] == '"') {
                        size_t end = expanded.find('"', 1);
                        inc_name = expanded.substr(1, end==std::string::npos ? expanded.size()-1 : end-1);
                    } else {
                        inc_name = expanded;
                    }
                }

                std::string path = find_include(inc_name, sys, cur_dir);
                if (path.empty())
                    diag_.fatal(filename.c_str(), lineno,
                                "cannot find include file '%s'", inc_name.c_str());
                std::string include_key = canonical_include_key(path);
                if (pragma_once_files_.count(include_key)) {
                    emit_line_marker(lineno + 1, filename);
                    continue;
                }
                std::string inc_src = read_file(path);
                process_text(inc_src, path, out, depth + 1);
                // Resume marker after return
                emit_line_marker(lineno + 1, filename);
                continue;
            }

            if (dir == "error") {
                diag_.fatal(filename.c_str(), lineno, "#error %s", rest.c_str());
            }

            // C23: #warning emits a diagnostic but continues compilation.
            if (dir == "warning") {
                diag_.warning(warning_group::CPP, filename.c_str(), lineno,
                              "#warning %s", rest.c_str());
                if (emitting()) emit_line_marker(lineno + 1, filename);
                continue;
            }

            if (dir == "pragma") {
                if (pp_trim(rest) == "once") {
                    pragma_once_files_.insert(file_key);
                    if (emitting()) emit_line_marker(lineno + 1, filename);
                    continue;
                }
                diag_.handle_pragma(filename.c_str(), lineno, rest);
                if (emitting()) emit_line_marker(lineno + 1, filename);
                continue;
            }

            if (dir == "line") {
                // #line <num> ["file"] — update markers.
                // Expand macros in the rest first (#line line with #define line 1000).
                std::vector<std::string> guard;
                std::string xrest = expand(rest, guard);
                size_t q = 0;
                while (q < xrest.size() && isdigit((unsigned char)xrest[q])) ++q;
                if (q > 0) {
                    try { lineno = std::stoi(xrest.substr(0, q)) - 1; }
                    catch (...) { /* ignore malformed #line */ }
                }
                if (emitting()) emit_line_marker(lineno + 1, filename);
                continue;
            }

            // Unknown directive: warn and ignore
            diag_.warning(warning_group::CPP, filename.c_str(), lineno,
                          "unknown preprocessor directive '#%s'", dir.c_str());
            continue;
        }

        // Non-directive line
        if (!emitting()) continue;

        // Set call-site context so expand() can resolve __LINE__ / __FILE__
        // inside macro bodies at expansion time (C standard requirement).
        expand_lineno_ = logical_lineno;
        expand_file_   = filename;

        std::vector<std::string> guard;
        out += expand(line, guard);
        out += '\n';
    }

    if (!cond_stack.empty())
        diag_.fatal(filename.c_str(), lineno, "unterminated #if/#ifdef/#ifndef");
}

} // namespace xcc
