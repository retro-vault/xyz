//
// z80peep.cpp — Z80 peephole optimizer.
//
// Parses the assembly text produced by z80_gen into a vector of
// asm_line structs, then runs multiple passes of pattern-matching
// rules until no rule fires (fixed-point iteration).
//
// Each rule inspects a fixed window of adjacent lines and replaces or
// removes them when a known sub-optimal pattern is found.  All rules
// are purely syntactic — no liveness analysis is performed, except
// rule_temp_store_reload which scans forward for any later reference
// to the temp slot before eliding it.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//
#include "xopt/z80peep.h"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace xopt {

// ----- asm_line ------------------------------------------------------

static std::string trim(const std::string &s) {
    size_t b = s.find_first_not_of(" \t");
    if (b == std::string::npos) return "";
    size_t e = s.find_last_not_of(" \t");
    return s.substr(b, e - b + 1);
}

static std::string lowercase_ascii(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return (char)std::tolower(c); });
    return s;
}

asm_line asm_line::parse(const std::string &raw) {
    asm_line al;
    std::string s = raw;

    // Extract comment
    size_t semi = s.find(';');
    if (semi != std::string::npos) {
        al.comment = trim(s.substr(semi + 1));
        s          = s.substr(0, semi);
    }

    s = trim(s);
    if (s.empty()) {
        al.is_comment = true;
        return al;
    }

    // Label: ends with ':'
    size_t colon = s.find(':');
    if (colon != std::string::npos && colon > 0 &&
        s.find_first_of(" \t") > colon) {
        al.label    = trim(s.substr(0, colon));
        al.is_label = true;
        s           = trim(s.substr(colon + 1));
        if (!s.empty() && s[0] == ':') {
            al.is_global_label = true;
            s = trim(s.substr(1));
        }
        if (s.empty()) return al;
    }

    // SDCC-style equates are written as "NAME .equ value" without a colon.
    // Preserve the symbol spelling because assembler symbols are case-sensitive.
    size_t equ_space = s.find_first_of(" \t");
    if (equ_space != std::string::npos) {
        const std::string first = trim(s.substr(0, equ_space));
        const std::string rest = trim(s.substr(equ_space));
        size_t dir_space = rest.find_first_of(" \t");
        const std::string directive = dir_space == std::string::npos
            ? rest
            : trim(rest.substr(0, dir_space));
        const std::string directive_lower = lowercase_ascii(directive);
        if (!first.empty() && first[0] != '.' &&
            (directive_lower == ".equ" || directive_lower == "equ")) {
            al.label = first;
            al.is_label = true;
            al.label_has_colon = false;
            al.mnemonic = directive_lower;
            if (dir_space != std::string::npos)
                al.operands = trim(rest.substr(dir_space));
            return al;
        }
    }

    // Mnemonic
    size_t space = s.find_first_of(" \t");
    if (space == std::string::npos) {
        al.mnemonic = s;
    } else {
        al.mnemonic  = trim(s.substr(0, space));
        al.operands  = trim(s.substr(space));
    }

    // Normalize mnemonic to lowercase
    al.mnemonic = lowercase_ascii(al.mnemonic);

    return al;
}

std::string asm_line::to_string() const {
    std::ostringstream os;
    if (!label.empty()) {
        if (!label_has_colon) {
            os << label;
            if (!mnemonic.empty()) os << "\t";
        } else {
            os << label << (is_global_label ? "::\n" : ":\n");
        }
    }
    if (!mnemonic.empty()) {
        if (label.empty() || label_has_colon)
            os << "\t";
        os << mnemonic;
        if (!operands.empty()) os << "\t" << operands;
    }
    if (!comment.empty())  os << "\t; " << comment;
    if (!mnemonic.empty() || !comment.empty()) os << "\n";
    return os.str();
}

// ----- helpers -------------------------------------------------------

// Split "dst, src" at the first comma.
static bool split_ld(const std::string &ops,
                     std::string &dst, std::string &src) {
    size_t comma = ops.find(',');
    if (comma == std::string::npos) return false;
    dst = trim(ops.substr(0, comma));
    src = trim(ops.substr(comma + 1));
    return true;
}

// Parse an IX-relative operand like "-6(ix)" or "4(ix)".
static bool parse_ix_ref(const std::string &s, int &offset) {
    size_t paren = s.find("(ix)");
    if (paren == std::string::npos || paren == 0) return false;
    try { offset = std::stoi(s.substr(0, paren)); return true; }
    catch (...) { return false; }
}

// Parse an IY-relative operand like "-6(iy)" or "4(iy)".
static bool parse_iy_ref(const std::string &s, int &offset) {
    size_t paren = s.find("(iy)");
    if (paren == std::string::npos || paren == 0) return false;
    try { offset = std::stoi(s.substr(0, paren)); return true; }
    catch (...) { return false; }
}

static bool parse_ixiy_ref(const std::string &s, int &offset) {
    return parse_ix_ref(s, offset) || parse_iy_ref(s, offset);
}

struct asm_pattern_line {
    const char *label = nullptr;
    const char *mnemonic = nullptr;
    const char *operands = nullptr;
};

struct asm_template_line {
    const char *label = nullptr;
    const char *mnemonic = nullptr;
    const char *operands = nullptr;
};

struct asm_structural_rule {
    const char *name = "";
    const asm_pattern_line *match = nullptr;
    size_t match_count = 0;
    const asm_template_line *replace = nullptr;
    size_t replace_count = 0;
};

static bool is_capture_field(const char *field) {
    return field != nullptr && field[0] == '$' && field[1] != '\0';
}

static bool match_pattern_field(const char *pattern,
                                const std::string &actual,
                                std::unordered_map<std::string, std::string> &captures) {
    if (pattern == nullptr)
        return true;
    if (is_capture_field(pattern)) {
        const std::string key(pattern + 1);
        auto it = captures.find(key);
        if (it == captures.end()) {
            captures.emplace(key, actual);
            return true;
        }
        return it->second == actual;
    }
    return actual == pattern;
}

static bool expand_template_field(const char *templ,
                                  const std::unordered_map<std::string, std::string> &captures,
                                  std::string &out) {
    if (templ == nullptr) {
        out.clear();
        return true;
    }
    if (is_capture_field(templ)) {
        auto it = captures.find(templ + 1);
        if (it == captures.end())
            return false;
        out = it->second;
        return true;
    }
    out = templ;
    return true;
}

static void collect_capture_names(const char *field,
                                  std::unordered_set<std::string> &names) {
    if (is_capture_field(field))
        names.insert(field + 1);
}

static bool validate_structural_rule(const asm_structural_rule &rule) {
    std::unordered_set<std::string> bound;
    for (size_t i = 0; i < rule.match_count; ++i) {
        collect_capture_names(rule.match[i].label, bound);
        collect_capture_names(rule.match[i].mnemonic, bound);
        collect_capture_names(rule.match[i].operands, bound);
    }

    auto replacement_capture_known = [&](const char *field) {
        return !is_capture_field(field) || bound.count(field + 1) != 0;
    };

    for (size_t i = 0; i < rule.replace_count; ++i) {
        if (!replacement_capture_known(rule.replace[i].label) ||
            !replacement_capture_known(rule.replace[i].mnemonic) ||
            !replacement_capture_known(rule.replace[i].operands)) {
            return false;
        }
    }
    return true;
}

static bool apply_structural_rule_at(std::vector<asm_line> &lines, size_t index,
                                     const asm_structural_rule &rule) {
    if (index + rule.match_count > lines.size())
        return false;

    std::unordered_map<std::string, std::string> captures;
    for (size_t i = 0; i < rule.match_count; ++i) {
        const asm_line &line = lines[index + i];
        if (!match_pattern_field(rule.match[i].label, line.label, captures) ||
            !match_pattern_field(rule.match[i].mnemonic, line.mnemonic, captures) ||
            !match_pattern_field(rule.match[i].operands, line.operands, captures)) {
            return false;
        }
    }

    std::vector<asm_line> replacement;
    replacement.reserve(rule.replace_count);
    for (size_t i = 0; i < rule.replace_count; ++i) {
        asm_line line;
        if (!expand_template_field(rule.replace[i].label, captures, line.label) ||
            !expand_template_field(rule.replace[i].mnemonic, captures, line.mnemonic) ||
            !expand_template_field(rule.replace[i].operands, captures, line.operands)) {
            return false;
        }
        line.is_label = !line.label.empty();
        replacement.push_back(std::move(line));
    }

    lines.erase(lines.begin() + static_cast<std::ptrdiff_t>(index),
                lines.begin() + static_cast<std::ptrdiff_t>(index + rule.match_count));
    lines.insert(lines.begin() + static_cast<std::ptrdiff_t>(index),
                 replacement.begin(), replacement.end());
    return true;
}

static bool apply_structural_rules(std::vector<asm_line> &lines, size_t index) {
    static const asm_pattern_line k_push_pop_hl_match[] = {
        {"", "push", "hl"},
        {"", "pop",  "hl"},
    };
    static const asm_pattern_line k_jp_next_match[] = {
        {"",       "jp", "$target"},
        {"$target","",   ""},
    };
    static const asm_template_line k_jp_next_replace[] = {
        {"$target", "", ""},
    };
    static const asm_pattern_line k_call_ret_match[] = {
        {"", "call", "$target"},
        {"", "ret",  ""},
    };
    static const asm_template_line k_call_ret_replace[] = {
        {"", "jp", "$target"},
    };
    static const asm_pattern_line k_push_pop_bc_match[] = {
        {"", "push", "hl"},
        {"", "pop",  "bc"},
    };
    static const asm_template_line k_push_pop_bc_replace[] = {
        {"", "ld", "b, h"},
        {"", "ld", "c, l"},
    };

    static const asm_structural_rule k_rules[] = {
        {"push_pop_hl", k_push_pop_hl_match, 2, nullptr, 0},
        {"jp_next",     k_jp_next_match,     2, k_jp_next_replace, 1},
        {"call_ret_to_jp_simple", k_call_ret_match, 2, k_call_ret_replace, 1},
        {"push_pop_bc", k_push_pop_bc_match, 2, k_push_pop_bc_replace, 2},
    };

    static const bool validated = []() {
        for (const auto &rule : k_rules)
            if (!validate_structural_rule(rule))
                return false;
        return true;
    }();

    if (!validated)
        return false;

    for (const auto &rule : k_rules) {
        if (apply_structural_rule_at(lines, index, rule))
            return true;
    }
    return false;
}

static bool parse_conditional_jump(const asm_line &line,
                                   std::string &cc,
                                   std::string &target) {
    if (line.mnemonic != "jp" && line.mnemonic != "jr")
        return false;

    static const char *const ccs[] = {
        "nz,", "z,", "nc,", "c,", "m,", "p,", "pe,", "po,", nullptr
    };

    std::string ops = trim(line.operands);
    for (int k = 0; ccs[k]; ++k) {
        size_t len = std::strlen(ccs[k]);
        if (ops.size() <= len || ops.substr(0, len) != ccs[k])
            continue;
        cc = ops.substr(0, len - 1);
        target = trim(ops.substr(len));
        return !target.empty();
    }

    return false;
}

static bool parse_unconditional_jump(const asm_line &line,
                                     std::string &target) {
    if (line.mnemonic != "jp" && line.mnemonic != "jr")
        return false;

    target = trim(line.operands);
    return !target.empty() && target.find(',') == std::string::npos;
}

static bool is_numeric_literal(const std::string &s) {
    if (s.empty())
        return false;
    size_t i = 0;
    if (s[i] == '#')
        ++i;
    if (i < s.size() && (s[i] == '-' || s[i] == '+'))
        ++i;
    if (i >= s.size())
        return false;
    if (s[i] == '0' && i + 1 < s.size() &&
        (s[i + 1] == 'x' || s[i + 1] == 'X')) {
        i += 2;
        if (i >= s.size())
            return false;
        for (; i < s.size(); ++i) {
            if (!std::isxdigit(static_cast<unsigned char>(s[i])))
                return false;
        }
        return true;
    }
    for (; i < s.size(); ++i) {
        if (!std::isdigit(static_cast<unsigned char>(s[i])))
            return false;
    }
    return true;
}

static std::string lower_copy(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char ch) {
                       return static_cast<char>(std::tolower(ch));
                   });
    return s;
}

static bool parse_immediate_value(const std::string &raw, int &value) {
    std::string s = lower_copy(trim(raw));
    if (s.empty())
        return false;
    if (s.front() == '#')
        s.erase(s.begin());
    if (s.empty())
        return false;

    int base = 10;
    if (s.front() == '$') {
        s.erase(s.begin());
        base = 16;
    } else if (s.size() > 2 && s[0] == '0' && s[1] == 'x') {
        s = s.substr(2);
        base = 16;
    } else if (!s.empty() && s.back() == 'h') {
        s.pop_back();
        base = 16;
    }

    if (s.empty())
        return false;
    try {
        size_t parsed = 0;
        long v = std::stol(s, &parsed, base);
        if (parsed != s.size())
            return false;
        value = static_cast<int>(v);
        return true;
    } catch (...) {
        return false;
    }
}

static bool immediate_is(const std::string &operand, int expected) {
    int value = 0;
    return parse_immediate_value(operand, value) && value == expected;
}

static int u8_value(int value) {
    return value & 0xff;
}

static std::string imm8_text(int value) {
    return "#" + std::to_string(u8_value(value));
}

static std::string imm16_text(int value) {
    return "#" + std::to_string(value & 0xffff);
}

static bool is_immediate_operand(const std::string &s) {
    return !s.empty() && s.front() == '#';
}

static bool is_reg8(const std::string &s) {
    return s == "a" || s == "b" || s == "c" || s == "d" ||
           s == "e" || s == "h" || s == "l" ||
           s == "ixl" || s == "ixh" || s == "iyl" || s == "iyh";
}

static bool is_reg16(const std::string &s) {
    return s == "af" || s == "bc" || s == "de" || s == "hl" ||
           s == "ix" || s == "iy" || s == "sp";
}

static bool uses_ixiy_disp(const std::string &s) {
    return s.find("(ix") != std::string::npos ||
           s.find("(iy") != std::string::npos ||
           s.find("(ix)") != std::string::npos ||
           s.find("(iy)") != std::string::npos;
}

static bool uses_hl_indirect(const std::string &s) {
    return s == "(hl)";
}

static bool uses_abs_indirect(const std::string &s) {
    if (s.size() < 3 || s.front() != '(' || s.back() != ')')
        return false;
    return !uses_hl_indirect(s) && !uses_ixiy_disp(s);
}

static bool is_section_directive(const asm_line &line);

static bool operand_has_token(const std::string &operand, const std::string &token) {
    if (token.empty())
        return false;
    for (size_t i = 0; i < operand.size();) {
        if (!std::isalnum(static_cast<unsigned char>(operand[i])) &&
            operand[i] != '_') {
            ++i;
            continue;
        }
        size_t j = i + 1;
        while (j < operand.size() &&
               (std::isalnum(static_cast<unsigned char>(operand[j])) ||
                operand[j] == '_')) {
            ++j;
        }
        if (operand.substr(i, j - i) == token)
            return true;
        i = j;
    }
    return false;
}

static bool operand_mentions_pair_or_bytes(const std::string &operand,
                                           const std::string &pair,
                                           char lo,
                                           char hi) {
    return operand_has_token(operand, pair) ||
           operand_has_token(operand, std::string(1, lo)) ||
           operand_has_token(operand, std::string(1, hi));
}

static bool line_preserves_pair_and_sp(const asm_line &line,
                                       const std::string &pair,
                                       char lo,
                                       char hi) {
    if (!line.label.empty())
        return false;
    if (line.mnemonic.empty())
        return false;
    if (is_section_directive(line))
        return false;

    // Reject anything that definitely changes control flow, stack depth,
    // or swaps register banks/pairs in ways that are awkward to model
    // conservatively here.
    const std::string &m = line.mnemonic;
    if (m == "push" || m == "pop" || m == "call" || m == "ret" ||
        m == "reti" || m == "retn" || m == "rst" || m == "jp" ||
        m == "jr" || m == "djnz" || m == "ex" || m == "exx" ||
        m == "ldi" || m == "ldir" || m == "ldd" || m == "lddr" ||
        m == "cpi" || m == "cpir" || m == "cpd" || m == "cpdr") {
        return false;
    }

    if (operand_has_token(line.operands, "sp"))
        return false;

    return !operand_mentions_pair_or_bytes(line.operands, pair, lo, hi);
}

static bool line_preserves_pair_value(const asm_line &line,
                                      const std::string &pair,
                                      char lo,
                                      char hi) {
    if (!line.label.empty())
        return false;
    if (line.mnemonic.empty())
        return false;
    if (is_section_directive(line))
        return false;

    const std::string &m = line.mnemonic;
    if (m == "call" || m == "ret" || m == "reti" || m == "retn" ||
        m == "rst" || m == "jp" || m == "jr" || m == "djnz" ||
        m == "ex" || m == "exx" ||
        m == "ldi" || m == "ldir" || m == "ldd" || m == "lddr" ||
        m == "cpi" || m == "cpir" || m == "cpd" || m == "cpdr") {
        return false;
    }

    if ((m == "push" || m == "pop") &&
        !operand_mentions_pair_or_bytes(line.operands, pair, lo, hi)) {
        return true;
    }

    return !operand_mentions_pair_or_bytes(line.operands, pair, lo, hi);
}

static int count_csv_items(const std::string &ops) {
    if (trim(ops).empty())
        return 0;
    int count = 1;
    for (char ch : ops) {
        if (ch == ',')
            ++count;
    }
    return count;
}

static bool is_section_directive(const asm_line &line) {
    return line.mnemonic == ".area" || line.mnemonic == ".section" ||
           line.mnemonic == ".text" || line.mnemonic == ".data" ||
           line.mnemonic == ".module";
}

static bool is_spaghetti_helper_label(const asm_line &line) {
    return line.label.rfind("__xopt_spaghetti_", 0) == 0;
}

static bool is_outline_helper_transfer(const asm_line &line) {
    return (line.mnemonic == "call" || line.mnemonic == "jp") &&
           trim(line.operands).rfind("__xopt_outline_", 0) == 0;
}

static bool outline_helper_may_access_ix_offset(const asm_line &line,
                                                int offset) {
    if (!is_outline_helper_transfer(line))
        return false;
    constexpr const char *prefix = "xopt-ix:";
    if (line.comment.rfind(prefix, 0) != 0)
        return true;

    const std::string values = line.comment.substr(std::strlen(prefix));
    if (values == "any")
        return true;
    if (values == "none")
        return false;

    size_t start = 0;
    while (start <= values.size()) {
        const size_t comma = values.find(',', start);
        const size_t end = comma == std::string::npos ? values.size() : comma;
        try {
            if (std::stoi(values.substr(start, end - start)) == offset)
                return true;
        } catch (...) {
            return true;
        }
        if (comma == std::string::npos)
            break;
        start = comma + 1;
    }
    return false;
}

static bool is_or_a_self(const asm_line &line) {
    if (line.mnemonic != "or")
        return false;
    std::string ops = trim(line.operands);
    return ops == "a" || ops == "a, a" || ops == "a,a";
}

static bool is_and_a_self(const asm_line &line) {
    if (line.mnemonic != "and")
        return false;
    std::string ops = trim(line.operands);
    return ops == "a" || ops == "a, a" || ops == "a,a";
}

static bool is_xor_a_self(const asm_line &line) {
    if (line.mnemonic != "xor")
        return false;
    std::string ops = trim(line.operands);
    return ops == "a" || ops == "a, a" || ops == "a,a";
}

static bool operand_uses_memory(const std::string &operand) {
    return operand.find('(') != std::string::npos ||
           operand.find(')') != std::string::npos;
}

static bool parse_bit_reg_operands(const std::string &ops,
                                   int &bit,
                                   std::string &reg) {
    std::string bit_text;
    if (!split_ld(ops, bit_text, reg))
        return false;
    if (!parse_immediate_value(bit_text, bit) || bit < 0 || bit > 7)
        return false;
    reg = lower_copy(trim(reg));
    return !reg.empty();
}

static bool is_plain_8bit_reg(const std::string &reg) {
    return reg == "a" || reg == "b" || reg == "c" || reg == "d" ||
           reg == "e" || reg == "h" || reg == "l";
}

static bool bit_operand_supported(const std::string &operand) {
    const std::string op = lower_copy(trim(operand));
    int ignored = 0;
    return is_plain_8bit_reg(op) || parse_ixiy_ref(op, ignored) ||
           op == "(hl)";
}

static int single_u8_bit_index(int value) {
    value = u8_value(value);
    if (value == 0 || (value & (value - 1)) != 0)
        return -1;
    int bit = 0;
    while ((value >> bit) != 1)
        ++bit;
    return bit;
}

static int single_u16_bit_index(int value) {
    value &= 0xffff;
    if (value == 0 || (value & (value - 1)) != 0)
        return -1;
    int bit = 0;
    while ((value >> bit) != 1)
        ++bit;
    return bit;
}

static bool is_pure_register_load_source(const std::string &operand) {
    return is_plain_8bit_reg(operand) || is_immediate_operand(operand) ||
           is_numeric_literal(operand);
}

static bool source_reads_pair(const std::string &src,
                              const std::string &pair,
                              char lo,
                              char hi) {
    const std::string s = trim(src);
    return operand_mentions_pair_or_bytes(s, pair, lo, hi);
}

static bool is_ld_to_reg_without_pair_source(const asm_line &line,
                                             const std::string &dst_reg,
                                             const std::string &pair,
                                             char lo,
                                             char hi) {
    if (line.mnemonic != "ld" || !line.label.empty())
        return false;

    std::string dst, src;
    if (!split_ld(line.operands, dst, src))
        return false;

    return trim(dst) == dst_reg &&
           !source_reads_pair(src, pair, lo, hi);
}

static bool overwrites_pair_without_reading_it(
        const std::vector<asm_line> &lines,
        size_t i,
        const std::string &pair,
        char lo,
        char hi) {
    if (i >= lines.size())
        return false;

    if (lines[i].mnemonic == "pop" && trim(lines[i].operands) == pair)
        return true;

    if (is_ld_to_reg_without_pair_source(lines[i], pair, pair, lo, hi))
        return true;

    if (i + 1 >= lines.size())
        return false;
    if (!lines[i + 1].label.empty())
        return false;

    const bool low_high =
        is_ld_to_reg_without_pair_source(lines[i], std::string(1, lo),
                                         pair, lo, hi) &&
        is_ld_to_reg_without_pair_source(lines[i + 1], std::string(1, hi),
                                         pair, lo, hi);
    const bool high_low =
        is_ld_to_reg_without_pair_source(lines[i], std::string(1, hi),
                                         pair, lo, hi) &&
        is_ld_to_reg_without_pair_source(lines[i + 1], std::string(1, lo),
                                         pair, lo, hi);
    return low_high || high_low;
}

static bool overwrites_hl_without_reading_it(
        const std::vector<asm_line> &lines,
        size_t i) {
    return overwrites_pair_without_reading_it(lines, i, "hl", 'l', 'h');
}

static bool overwrites_hl_after_short_preserving_span(
        const std::vector<asm_line> &lines,
        size_t i) {
    const size_t end = std::min(lines.size(), i + 5);
    for (size_t k = i; k < end; ++k) {
        if (overwrites_hl_without_reading_it(lines, k))
            return true;
        if (!line_preserves_pair_and_sp(lines[k], "hl", 'l', 'h'))
            return false;
    }
    return false;
}

static bool straightline_overwrites_hl_before_read_allowing_sp(
        const std::vector<asm_line> &lines,
        size_t start,
        size_t budget = 32) {
    const size_t end = std::min(lines.size(), start + budget);
    for (size_t k = start; k < end; ++k) {
        const asm_line &line = lines[k];
        if (!line.label.empty() || line.mnemonic.empty() ||
            is_section_directive(line)) {
            return false;
        }
        if (overwrites_hl_without_reading_it(lines, k))
            return true;

        const std::string &m = line.mnemonic;
        if (m == "call" || m == "ret" || m == "reti" || m == "retn" ||
            m == "rst" || m == "jp" || m == "jr" || m == "djnz" ||
            m == "ex" || m == "exx" ||
            m == "ldi" || m == "ldir" || m == "ldd" || m == "lddr" ||
            m == "cpi" || m == "cpir" || m == "cpd" || m == "cpdr") {
            return false;
        }
        if (operand_mentions_pair_or_bytes(line.operands, "hl", 'l', 'h'))
            return false;
    }
    return false;
}

static bool split_conditional_branch_target(const asm_line &line,
                                            std::string &cc,
                                            std::string &target) {
    if (line.mnemonic != "jp" && line.mnemonic != "jr")
        return false;
    std::string ops = trim(line.operands);
    size_t comma = ops.find(',');
    if (comma == std::string::npos)
        return false;
    cc = trim(ops.substr(0, comma));
    target = trim(ops.substr(comma + 1));
    if (cc.empty() || target.empty())
        return false;
    static const char *const ccs[] = {
        "z", "nz", "c", "nc", "m", "p", "pe", "po", nullptr
    };
    for (int k = 0; ccs[k]; ++k) {
        if (cc == ccs[k])
            return true;
    }
    return false;
}

static size_t find_label_index(const std::vector<asm_line> &lines,
                               const std::string &label) {
    for (size_t i = 0; i < lines.size(); ++i) {
        if (lines[i].label == label)
            return i;
    }
    return lines.size();
}

static bool path_overwrites_pair_before_read(
        const std::vector<asm_line> &lines,
        size_t start,
        const std::string &pair,
        char lo,
        char hi,
        size_t budget,
        std::unordered_set<size_t> &active,
        bool call_clobbers_pair = false) {
    if (start >= lines.size() || budget == 0)
        return false;
    if (!active.insert(start).second)
        return pair == "bc";

    auto finish = [&](bool result) {
        active.erase(start);
        return result;
    };

    for (size_t k = start; k < lines.size() && budget > 0; ++k, --budget) {
        const auto &line = lines[k];
        if (line.mnemonic.empty())
            continue;
        if (overwrites_pair_without_reading_it(lines, k, pair, lo, hi))
            return finish(true);
        if (call_clobbers_pair &&
            pair == "bc" &&
            line.mnemonic == "call") {
            return finish(true);
        }

        if (pair == "bc") {
            if (line.mnemonic == "ret" && trim(line.operands).empty())
                return finish(true);
            if (line.mnemonic == "ex" && trim(line.operands) == "de, hl")
                continue;
            if (line.mnemonic == "pop" && trim(line.operands) == "ix")
                continue;
            if (line.mnemonic == "ld") {
                std::string dst, src;
                if (split_ld(line.operands, dst, src) &&
                    trim(dst) == "sp" && trim(src) == "ix") {
                    continue;
                }
            }
        }

        std::string cc;
        std::string target;
        if (split_conditional_branch_target(line, cc, target)) {
            const size_t target_idx = find_label_index(lines, target);
            if (target_idx == lines.size())
                return finish(false);
            const bool taken =
                path_overwrites_pair_before_read(lines, target_idx, pair, lo, hi,
                                                 budget, active,
                                                 call_clobbers_pair);
            const bool fallthrough =
                path_overwrites_pair_before_read(lines, k + 1, pair, lo, hi,
                                                 budget, active,
                                                 call_clobbers_pair);
            return finish(taken && fallthrough);
        }

        if (parse_unconditional_jump(line, target)) {
            const size_t target_idx = find_label_index(lines, target);
            if (target_idx == lines.size())
                return finish(false);
            return finish(path_overwrites_pair_before_read(
                lines, target_idx, pair, lo, hi, budget, active,
                call_clobbers_pair));
        }

        if (!line_preserves_pair_and_sp(line, pair, lo, hi))
            return finish(false);
    }

    return finish(false);
}

static bool path_overwrites_pair_before_read(
        const std::vector<asm_line> &lines,
        size_t start,
        const std::string &pair,
        char lo,
        char hi) {
    std::unordered_set<size_t> active;
    return path_overwrites_pair_before_read(
        lines, start, pair, lo, hi, 64, active);
}

static bool path_overwrites_bc_before_read_or_call(
        const std::vector<asm_line> &lines,
        size_t start) {
    std::unordered_set<size_t> active;
    return path_overwrites_pair_before_read(
        lines, start, "bc", 'c', 'b', 64, active, true);
}

static bool path_overwrites_hl_before_read(
        const std::vector<asm_line> &lines,
        size_t start) {
    return path_overwrites_pair_before_read(lines, start, "hl", 'l', 'h');
}

static bool line_overwrites_b_without_reading_it(const asm_line &line) {
    if (!line.label.empty())
        return false;
    if (line.mnemonic != "ld")
        return false;

    std::string dst, src;
    if (!split_ld(line.operands, dst, src))
        return false;
    dst = trim(dst);
    src = trim(src);

    return (dst == "b" || dst == "bc") &&
           !operand_has_token(src, "b") &&
           !operand_has_token(src, "bc");
}

static bool line_reads_b_or_bc(const asm_line &line) {
    if (line.mnemonic.empty())
        return false;
    if (line.mnemonic == "djnz")
        return true;
    if (line.mnemonic == "ldi" || line.mnemonic == "ldir" ||
        line.mnemonic == "ldd" || line.mnemonic == "lddr" ||
        line.mnemonic == "cpi" || line.mnemonic == "cpir" ||
        line.mnemonic == "cpd" || line.mnemonic == "cpdr") {
        return true;
    }
    return operand_has_token(line.operands, "b") ||
           operand_has_token(line.operands, "bc");
}

static bool line_overwrites_c_without_reading_it(const asm_line &line) {
    if (!line.label.empty())
        return false;
    if (line.mnemonic != "ld")
        return false;

    std::string dst, src;
    if (!split_ld(line.operands, dst, src))
        return false;
    dst = trim(dst);
    src = trim(src);

    return (dst == "c" || dst == "bc") &&
           !operand_has_token(src, "c") &&
           !operand_has_token(src, "bc");
}

static bool line_reads_c_or_bc(const asm_line &line) {
    if (line.mnemonic.empty())
        return false;
    if (line.mnemonic == "ldi" || line.mnemonic == "ldir" ||
        line.mnemonic == "ldd" || line.mnemonic == "lddr" ||
        line.mnemonic == "cpi" || line.mnemonic == "cpir" ||
        line.mnemonic == "cpd" || line.mnemonic == "cpdr") {
        return true;
    }
    return operand_has_token(line.operands, "c") ||
           operand_has_token(line.operands, "bc");
}

static bool block_transfer_or_compare_touches_bc(const std::string &m) {
    return m == "ldi" || m == "ldir" || m == "ldd" || m == "lddr" ||
           m == "cpi" || m == "cpir" || m == "cpd" || m == "cpdr";
}

static bool line_touches_byte_or_pair(const asm_line &line,
                                      char byte,
                                      const std::string &pair) {
    if (line.mnemonic.empty())
        return false;

    const std::string byte_text(1, byte);
    const std::string &m = line.mnemonic;

    if (byte == 'b' && m == "djnz")
        return true;
    if ((byte == 'b' || byte == 'c') &&
        block_transfer_or_compare_touches_bc(m)) {
        return true;
    }
    if (m == "exx" && (pair == "bc" || pair == "de" || pair == "hl"))
        return true;
    if (m == "ex") {
        const std::string ops = lower_copy(trim(line.operands));
        if ((pair == "de" || pair == "hl") &&
            (ops == "de, hl" || ops == "de,hl" ||
             ops == "hl, de" || ops == "hl,de")) {
            return true;
        }
    }

    return operand_has_token(line.operands, byte_text) ||
           operand_has_token(line.operands, pair);
}

static bool line_is_control_flow_boundary(const asm_line &line) {
    const std::string &m = line.mnemonic;
    return m == "jp" || m == "jr" || m == "call" || m == "ret" ||
           m == "reti" || m == "retn" || m == "rst" || m == "djnz" ||
           m == "exx";
}

static bool bc_dead_before_read_or_ret(
        const std::vector<asm_line> &lines,
        size_t start,
        size_t budget,
        std::unordered_set<size_t> &active) {
    if (start >= lines.size() || budget == 0)
        return false;
    if (!active.insert(start).second)
        return false;

    auto finish = [&](bool result) {
        active.erase(start);
        return result;
    };

    for (size_t k = start; k < lines.size() && budget > 0; ++k, --budget) {
        const auto &line = lines[k];
        if (line.mnemonic.empty())
            continue;
        if (overwrites_pair_without_reading_it(lines, k, "bc", 'c', 'b') ||
            line.mnemonic == "call" ||
            (line.mnemonic == "ret" && trim(line.operands).empty())) {
            return finish(true);
        }
        if (line_overwrites_b_without_reading_it(line) ||
            line_overwrites_c_without_reading_it(line)) {
            continue;
        }

        std::string cc;
        std::string target;
        if (split_conditional_branch_target(line, cc, target)) {
            const size_t target_idx = find_label_index(lines, target);
            if (target_idx == lines.size())
                return finish(false);
            const bool taken = bc_dead_before_read_or_ret(
                lines, target_idx, budget, active);
            const bool fallthrough = bc_dead_before_read_or_ret(
                lines, k + 1, budget, active);
            return finish(taken && fallthrough);
        }

        if (parse_unconditional_jump(line, target)) {
            const size_t target_idx = find_label_index(lines, target);
            if (target_idx == lines.size())
                return finish(false);
            return finish(bc_dead_before_read_or_ret(
                lines, target_idx, budget, active));
        }

        if (line_reads_b_or_bc(line) || line_reads_c_or_bc(line) ||
            line_touches_byte_or_pair(line, 'b', "bc") ||
            line_touches_byte_or_pair(line, 'c', "bc") ||
            line_is_control_flow_boundary(line)) {
            return finish(false);
        }
    }

    return finish(false);
}

static bool bc_dead_before_read_or_ret(
        const std::vector<asm_line> &lines,
        size_t start) {
    std::unordered_set<size_t> active;
    return bc_dead_before_read_or_ret(lines, start, 64, active);
}

static bool bc_overwritten_or_call_before_read_allowing_unrelated(
        const std::vector<asm_line> &lines,
        size_t start,
        size_t budget = 18) {
    const size_t end = std::min(lines.size(), start + budget);
    for (size_t k = start; k < end; ++k) {
        const asm_line &line = lines[k];
        if (line.mnemonic.empty())
            continue;
        if (overwrites_pair_without_reading_it(lines, k, "bc", 'c', 'b'))
            return true;
        if (line.mnemonic == "call")
            return true;
        if (!line.label.empty() || is_section_directive(line) ||
            line_is_control_flow_boundary(line)) {
            return false;
        }
        if (line_reads_b_or_bc(line) || line_reads_c_or_bc(line) ||
            line_touches_byte_or_pair(line, 'b', "bc") ||
            line_touches_byte_or_pair(line, 'c', "bc")) {
            return false;
        }
    }
    return false;
}

static bool b_overwritten_before_read(
        const std::vector<asm_line> &lines,
        size_t start) {
    const size_t end = std::min(lines.size(), start + 16);
    for (size_t k = start; k < end; ++k) {
        const auto &line = lines[k];
        if (!line.label.empty() || is_section_directive(line))
            return false;
        if (line.mnemonic.empty())
            continue;
        if (line_overwrites_b_without_reading_it(line))
            return true;
        if (line_is_control_flow_boundary(line) || line_reads_b_or_bc(line))
            return false;
    }
    return false;
}

static bool c_overwritten_before_read(
        const std::vector<asm_line> &lines,
        size_t start) {
    const size_t end = std::min(lines.size(), start + 16);
    for (size_t k = start; k < end; ++k) {
        const auto &line = lines[k];
        if (!line.label.empty() || is_section_directive(line))
            return false;
        if (line.mnemonic.empty())
            continue;
        if (line_overwrites_c_without_reading_it(line))
            return true;
        if (line_is_control_flow_boundary(line) || line_reads_c_or_bc(line))
            return false;
    }
    return false;
}

static bool c_overwritten_or_call_before_read(
        const std::vector<asm_line> &lines,
        size_t start,
        size_t budget,
        std::unordered_set<size_t> &active) {
    if (start >= lines.size() || budget == 0)
        return false;
    if (!active.insert(start).second)
        return false;

    auto finish = [&](bool result) {
        active.erase(start);
        return result;
    };

    for (size_t k = start; k < lines.size() && budget > 0; ++k, --budget) {
        const auto &line = lines[k];
        if (line.mnemonic.empty())
            continue;
        if (line_overwrites_c_without_reading_it(line) ||
            line.mnemonic == "call") {
            return finish(true);
        }

        std::string cc;
        std::string target;
        if (split_conditional_branch_target(line, cc, target)) {
            const size_t target_idx = find_label_index(lines, target);
            if (target_idx == lines.size())
                return finish(false);
            const bool taken = c_overwritten_or_call_before_read(
                lines, target_idx, budget, active);
            const bool fallthrough = c_overwritten_or_call_before_read(
                lines, k + 1, budget, active);
            return finish(taken && fallthrough);
        }

        if (parse_unconditional_jump(line, target)) {
            const size_t target_idx = find_label_index(lines, target);
            if (target_idx == lines.size())
                return finish(false);
            return finish(c_overwritten_or_call_before_read(
                lines, target_idx, budget, active));
        }

        if (line_is_control_flow_boundary(line) || line_reads_c_or_bc(line) ||
            line_touches_byte_or_pair(line, 'c', "bc")) {
            return finish(false);
        }
    }

    return finish(false);
}

static bool c_overwritten_or_call_before_read(
        const std::vector<asm_line> &lines,
        size_t start) {
    std::unordered_set<size_t> active;
    return c_overwritten_or_call_before_read(lines, start, 64, active);
}

static bool label_has_other_control_references(
        const std::vector<asm_line> &lines,
        const std::string &label,
        size_t allowed_ref) {
    for (size_t k = 0; k < lines.size(); ++k) {
        if (k == allowed_ref)
            continue;
        const auto &line = lines[k];
        if (line.mnemonic != "jp" && line.mnemonic != "jr" &&
            line.mnemonic != "call" && line.mnemonic != "djnz") {
            continue;
        }

        std::string target = trim(line.operands);
        const size_t comma = target.find(',');
        if (comma != std::string::npos)
            target = trim(target.substr(comma + 1));
        if (target == label)
            return true;
    }
    return false;
}

static bool branch_targets_label(const asm_line &line,
                                 const std::string &label) {
    if (line.mnemonic != "jp" && line.mnemonic != "jr" &&
        line.mnemonic != "call" && line.mnemonic != "djnz") {
        return false;
    }
    std::string target = trim(line.operands);
    const size_t comma = target.find(',');
    if (comma != std::string::npos)
        target = trim(target.substr(comma + 1));
    return target == label;
}

static bool is_ld_d_zero(const asm_line &line) {
    if (!line.label.empty() || line.mnemonic != "ld")
        return false;
    std::string dst, src;
    return split_ld(line.operands, dst, src) &&
           trim(dst) == "d" && immediate_is(src, 0);
}

static bool is_ld_hl(const asm_line &line, std::string &src) {
    if (line.mnemonic != "ld")
        return false;
    std::string dst;
    if (!split_ld(line.operands, dst, src))
        return false;
    src = trim(src);
    return trim(dst) == "hl";
}

static bool is_ex_de_hl(const asm_line &line) {
    if (line.mnemonic != "ex")
        return false;
    const std::string ops = trim(line.operands);
    return ops == "de, hl" || ops == "de,hl" ||
           ops == "hl, de" || ops == "hl,de";
}

static bool is_plain_ret(const asm_line &line) {
    return line.mnemonic == "ret" && trim(line.operands).empty();
}

static const asm_line *next_instruction(const std::vector<asm_line> &lines,
                                        size_t &pos) {
    while (pos < lines.size()) {
        const asm_line &line = lines[pos++];
        if (line.mnemonic == ".globl")
            continue;
        if (!line.mnemonic.empty())
            return &line;
    }
    return nullptr;
}

static bool is_modern_return_tail(const std::vector<asm_line> &lines,
                                  size_t start,
                                  size_t budget,
                                  std::unordered_set<size_t> &active) {
    if (start >= lines.size() || budget == 0)
        return false;
    if (!active.insert(start).second)
        return false;

    auto finish = [&](bool result) {
        active.erase(start);
        return result;
    };

    size_t pos = start;
    const asm_line *first = next_instruction(lines, pos);
    if (!first)
        return finish(false);
    if (is_plain_ret(*first))
        return finish(true);

    std::string target;
    if (parse_unconditional_jump(*first, target)) {
        if (target == "__sdcc_leave_ix" || target == "___sdcc_leave_ix")
            return finish(true);
        const size_t target_idx = find_label_index(lines, target);
        if (target_idx == lines.size())
            return finish(false);
        return finish(is_modern_return_tail(lines, target_idx, budget - 1,
                                            active));
    }

    if (first->mnemonic == "pop" && trim(first->operands) == "ix") {
        const asm_line *ret = next_instruction(lines, pos);
        return finish(ret && is_plain_ret(*ret));
    }

    if (first->mnemonic == "ld") {
        std::string dst, src;
        if (!split_ld(first->operands, dst, src) ||
            trim(dst) != "sp" || trim(src) != "ix") {
            return finish(false);
        }
        const asm_line *pop_ix = next_instruction(lines, pos);
        if (!pop_ix || pop_ix->mnemonic != "pop" ||
            trim(pop_ix->operands) != "ix") {
            return finish(false);
        }
        const asm_line *ret_or_callee = next_instruction(lines, pos);
        if (!ret_or_callee)
            return finish(false);
        if (is_plain_ret(*ret_or_callee))
            return finish(true);
        if (ret_or_callee->mnemonic == "pop" &&
            trim(ret_or_callee->operands) == "hl") {
            const asm_line *tail = next_instruction(lines, pos);
            while (tail && tail->mnemonic == "pop" &&
                   trim(tail->operands) != "hl") {
                tail = next_instruction(lines, pos);
            }
            return finish(tail &&
                          tail->mnemonic == "jp" &&
                          trim(tail->operands) == "(hl)");
        }
        return finish(false);
    }

    return finish(false);
}

static bool is_modern_return_tail(const std::vector<asm_line> &lines,
                                  size_t start) {
    std::unordered_set<size_t> active;
    return is_modern_return_tail(lines, start, 8, active);
}

static bool is_inside_sdcccall1_function(const std::vector<asm_line> &lines,
                                         size_t index) {
    for (size_t scan = std::min(index + 1, lines.size()); scan > 0;) {
        const asm_line &line = lines[--scan];
        if (line.comment.rfind("sdcccall(1) prologue:", 0) == 0)
            return true;
        if (line.comment.find(" prologue:") != std::string::npos ||
            is_section_directive(line) ||
            line.label.rfind("__xopt_", 0) == 0) {
            return false;
        }
    }
    return false;
}

static bool hl_dead_before_read_or_modern_return(
        const std::vector<asm_line> &lines,
        size_t start,
        size_t budget,
        std::unordered_set<size_t> &active) {
    if (start >= lines.size() || budget == 0)
        return false;
    if (!active.insert(start).second)
        // Revisiting the same live-value state without an intervening read
        // is a no-read cycle. Any exit path was already explored at the
        // branch entering the cycle, so the old value is dead here.
        return true;

    auto finish = [&](bool result) {
        active.erase(start);
        return result;
    };

    for (size_t k = start; k < lines.size() && budget > 0; ++k, --budget) {
        const auto &line = lines[k];
        if (line.mnemonic.empty())
            continue;
        if (overwrites_hl_without_reading_it(lines, k))
            return finish(true);
        if (is_modern_return_tail(lines, k))
            return finish(true);
        if (is_ex_de_hl(line) &&
            path_overwrites_pair_before_read(lines, k + 1, "de", 'e', 'd')) {
            return finish(true);
        }

        std::string cc;
        std::string target;
        if (split_conditional_branch_target(line, cc, target)) {
            const size_t target_idx = find_label_index(lines, target);
            if (target_idx == lines.size())
                return finish(false);
            const bool taken = hl_dead_before_read_or_modern_return(
                lines, target_idx, budget, active);
            const bool fallthrough = hl_dead_before_read_or_modern_return(
                lines, k + 1, budget, active);
            return finish(taken && fallthrough);
        }

        if (parse_unconditional_jump(line, target)) {
            const size_t target_idx = find_label_index(lines, target);
            if (target_idx == lines.size())
                return finish(false);
            return finish(hl_dead_before_read_or_modern_return(
                lines, target_idx, budget, active));
        }

        if (!line_preserves_pair_value(line, "hl", 'l', 'h'))
            return finish(false);
    }

    return finish(false);
}

static bool hl_dead_before_read_or_modern_return(
        const std::vector<asm_line> &lines,
        size_t start) {
    std::unordered_set<size_t> active;
    return hl_dead_before_read_or_modern_return(lines, start, 64, active);
}

static bool pair_value_dead_or_call_or_modern_return_before_read(
        const std::vector<asm_line> &lines,
        size_t start,
        const std::string &pair,
        char lo,
        char hi,
        size_t budget,
        std::unordered_set<size_t> &active) {
    if (start >= lines.size() || budget == 0)
        return false;
    if (!active.insert(start).second)
        return true;

    auto finish = [&](bool result) {
        active.erase(start);
        return result;
    };

    for (size_t k = start; k < lines.size() && budget > 0; ++k, --budget) {
        const auto &line = lines[k];
        if (line.mnemonic.empty())
            continue;
        if (overwrites_pair_without_reading_it(lines, k, pair, lo, hi) ||
            line.mnemonic == "call" || is_modern_return_tail(lines, k)) {
            return finish(true);
        }

        std::string cc;
        std::string target;
        if (split_conditional_branch_target(line, cc, target)) {
            const size_t target_idx = find_label_index(lines, target);
            if (target_idx == lines.size())
                return finish(false);
            const bool taken =
                pair_value_dead_or_call_or_modern_return_before_read(
                    lines, target_idx, pair, lo, hi, budget, active);
            const bool fallthrough =
                pair_value_dead_or_call_or_modern_return_before_read(
                    lines, k + 1, pair, lo, hi, budget, active);
            return finish(taken && fallthrough);
        }

        if (parse_unconditional_jump(line, target)) {
            const size_t target_idx = find_label_index(lines, target);
            if (target_idx == lines.size())
                return finish(false);
            return finish(
                pair_value_dead_or_call_or_modern_return_before_read(
                    lines, target_idx, pair, lo, hi, budget, active));
        }

        if (!line_preserves_pair_value(line, pair, lo, hi))
            return finish(false);
    }

    return finish(false);
}

static bool pair_value_dead_or_call_or_modern_return_before_read(
        const std::vector<asm_line> &lines,
        size_t start,
        const std::string &pair,
        char lo,
        char hi) {
    std::unordered_set<size_t> active;
    return pair_value_dead_or_call_or_modern_return_before_read(
        lines, start, pair, lo, hi, 64, active);
}

enum class pair_effect {
    reads,
    preserves,
    overwrites,
};

static pair_effect spaghetti_helper_pair_effect(
        const std::vector<asm_line> &lines,
        const std::string &helper,
        const std::string &pair,
        char lo,
        char hi) {
    const size_t helper_idx = find_label_index(lines, helper);
    if (helper_idx == lines.size())
        return pair_effect::reads;

    for (size_t k = helper_idx + 1; k < lines.size(); ++k) {
        const auto &line = lines[k];
        if (is_spaghetti_helper_label(line))
            return pair_effect::reads;
        if (!line.label.empty())
            return pair_effect::reads;
        if (line.mnemonic.empty())
            continue;
        if (overwrites_pair_without_reading_it(lines, k, pair, lo, hi))
            return pair_effect::overwrites;
        if (is_plain_ret(line))
            return pair_effect::preserves;
        if (pair == "bc" && is_ex_de_hl(line))
            continue;
        if (!line_preserves_pair_and_sp(line, pair, lo, hi))
            return pair_effect::reads;
    }

    return pair_effect::reads;
}

static bool pair_dead_before_read_allowing_spaghetti(
        const std::vector<asm_line> &lines,
        size_t start,
        const std::string &pair,
        char lo,
        char hi,
        bool allow_modern_return,
        size_t budget,
        std::unordered_set<size_t> &active) {
    if (start >= lines.size() || budget == 0)
        return false;
    if (!active.insert(start).second)
        return false;

    auto finish = [&](bool result) {
        active.erase(start);
        return result;
    };

    for (size_t k = start; k < lines.size() && budget > 0; ++k, --budget) {
        const auto &line = lines[k];
        if (line.mnemonic.empty())
            continue;
        if (overwrites_pair_without_reading_it(lines, k, pair, lo, hi))
            return finish(true);
        if (allow_modern_return && is_modern_return_tail(lines, k))
            return finish(true);

        std::string cc;
        std::string target;
        if (split_conditional_branch_target(line, cc, target)) {
            const size_t target_idx = find_label_index(lines, target);
            if (target_idx == lines.size())
                return finish(false);
            const bool taken = pair_dead_before_read_allowing_spaghetti(
                lines, target_idx, pair, lo, hi, allow_modern_return,
                budget, active);
            const bool fallthrough = pair_dead_before_read_allowing_spaghetti(
                lines, k + 1, pair, lo, hi, allow_modern_return,
                budget, active);
            return finish(taken && fallthrough);
        }

        if (parse_unconditional_jump(line, target)) {
            const size_t target_idx = find_label_index(lines, target);
            if (target_idx == lines.size())
                return finish(false);
            return finish(pair_dead_before_read_allowing_spaghetti(
                lines, target_idx, pair, lo, hi, allow_modern_return,
                budget, active));
        }

        if (line.mnemonic == "call") {
            const std::string callee = trim(line.operands);
            if (callee.rfind("__xopt_spaghetti_", 0) == 0) {
                const pair_effect effect = spaghetti_helper_pair_effect(
                    lines, callee, pair, lo, hi);
                if (effect == pair_effect::overwrites)
                    return finish(true);
                if (effect == pair_effect::preserves)
                    continue;
            }
            return finish(false);
        }

        if (!line_preserves_pair_and_sp(line, pair, lo, hi))
            return finish(false);
    }

    return finish(false);
}

static bool pair_dead_before_read_allowing_spaghetti(
        const std::vector<asm_line> &lines,
        size_t start,
        const std::string &pair,
        char lo,
        char hi,
        bool allow_modern_return) {
    std::unordered_set<size_t> active;
    return pair_dead_before_read_allowing_spaghetti(
        lines, start, pair, lo, hi, allow_modern_return, 64, active);
}

static bool in_sdcccall1_function(const std::vector<asm_line> &lines,
                                  size_t index) {
    for (size_t scan = index + 1; scan > 0; --scan) {
        const size_t k = scan - 1;
        const asm_line &line = lines[k];
        const std::string comment = lower_copy(line.comment);
        if (comment.find("prologue") != std::string::npos) {
            return comment.find("sdcccall(1)") != std::string::npos;
        }
        if (is_section_directive(line))
            return false;
    }
    return false;
}

static bool parse_frame_comment(const std::string &comment,
                                int &locals,
                                int &temp_frame) {
    const std::string text = lower_copy(comment);
    const size_t locals_pos = text.find("locals=");
    const size_t temp_pos = text.find("temp_frame=");
    if (locals_pos == std::string::npos || temp_pos == std::string::npos)
        return false;

    auto parse_after = [&](size_t pos, const char *key, int &out) {
        pos += std::strlen(key);
        if (pos >= text.size() ||
            !std::isdigit(static_cast<unsigned char>(text[pos]))) {
            return false;
        }
        int value = 0;
        while (pos < text.size() &&
               std::isdigit(static_cast<unsigned char>(text[pos]))) {
            value = value * 10 + (text[pos] - '0');
            ++pos;
        }
        out = value;
        return true;
    };

    return parse_after(locals_pos, "locals=", locals) &&
           parse_after(temp_pos, "temp_frame=", temp_frame);
}

static bool current_function_frame(const std::vector<asm_line> &lines,
                                   size_t index,
                                   int &locals,
                                   int &temp_frame,
                                   size_t &prologue_index) {
    for (size_t scan = index + 1; scan > 0; --scan) {
        const size_t k = scan - 1;
        const asm_line &line = lines[k];
        if (parse_frame_comment(line.comment, locals, temp_frame)) {
            prologue_index = k;
            return true;
        }
        if (is_section_directive(line))
            return false;
    }
    return false;
}

static bool ix_offset_in_temp_frame(int offset, int locals, int temp_frame) {
    if (offset >= 0 || temp_frame <= 0)
        return false;
    const int depth = -offset;
    return depth > locals && depth <= locals + temp_frame;
}

static bool line_writes_ix_offset(const asm_line &line, int offset) {
    if (line.mnemonic.empty())
        return false;
    if (outline_helper_may_access_ix_offset(line, offset))
        return true;
    if (line.mnemonic == "ld") {
        std::string dst;
        std::string src;
        int dst_offset = 0;
        return split_ld(line.operands, dst, src) &&
               parse_ix_ref(trim(dst), dst_offset) &&
               dst_offset == offset;
    }
    if (line.mnemonic == "inc" || line.mnemonic == "dec") {
        int op_offset = 0;
        return parse_ix_ref(trim(line.operands), op_offset) &&
               op_offset == offset;
    }
    return false;
}

static bool line_writes_any_ix_offset(const asm_line &line) {
    std::string dst;
    std::string src;
    int offset = 0;
    if (line.mnemonic == "ld" && split_ld(line.operands, dst, src))
        return parse_ix_ref(trim(dst), offset);
    if (line.mnemonic == "inc" || line.mnemonic == "dec")
        return parse_ix_ref(trim(line.operands), offset);
    return false;
}

static size_t function_end_after_prologue(const std::vector<asm_line> &lines,
                                          size_t prologue_index);

static bool line_reads_ix_offset(const asm_line &line, int offset);
static bool ix_slot_read_in_function(const std::vector<asm_line> &lines,
                                     size_t begin,
                                     size_t end,
                                     int offset,
                                     size_t ignore_index);
static bool ix_slot_not_read_before_rewrite(const std::vector<asm_line> &lines,
                                            size_t begin,
                                            size_t end,
                                            int offset);
static bool ix_value_may_be_read_before_rewrite(
    const std::vector<asm_line> &lines,
    size_t function_begin,
    size_t function_end,
    size_t start,
    int offset);
static bool ix_slot_referenced_after(const std::vector<asm_line> &lines,
                                     size_t start,
                                     int offset);

static bool match_const_hl_temp_pair_store(
        const std::vector<asm_line> &lines,
        size_t i,
        int lo_offset,
        std::string &immediate) {
    if (i + 2 >= lines.size())
        return false;
    for (size_t k = 0; k < 3; ++k) {
        if (!lines[i + k].label.empty())
            return false;
    }

    std::string dst;
    std::string src;
    if (lines[i].mnemonic != "ld" ||
        !split_ld(lines[i].operands, dst, src) ||
        trim(dst) != "hl") {
        return false;
    }
    src = trim(src);
    if (!is_immediate_operand(src))
        return false;

    int store_lo = 0;
    if (lines[i + 1].mnemonic != "ld" ||
        !split_ld(lines[i + 1].operands, dst, immediate) ||
        !parse_ix_ref(trim(dst), store_lo) ||
        store_lo != lo_offset ||
        trim(immediate) != "l") {
        return false;
    }

    int store_hi = 0;
    if (lines[i + 2].mnemonic != "ld" ||
        !split_ld(lines[i + 2].operands, dst, immediate) ||
        !parse_ix_ref(trim(dst), store_hi) ||
        store_hi != lo_offset + 1 ||
        trim(immediate) != "h") {
        return false;
    }

    immediate = src;
    return true;
}

static bool find_const_temp_word_before_use(
        const std::vector<asm_line> &lines,
        size_t prologue_index,
        size_t use_index,
        int lo_offset,
        std::string &immediate) {
    bool found = false;
    bool found_before_use = false;
    std::string found_immediate;

    size_t end_index = lines.size();
    for (size_t k = prologue_index + 1; k < lines.size(); ++k) {
        int ignored_locals = 0;
        int ignored_temp_frame = 0;
        if (parse_frame_comment(lines[k].comment, ignored_locals,
                                ignored_temp_frame)) {
            end_index = k;
            break;
        }
        if (is_section_directive(lines[k])) {
            end_index = k;
            break;
        }
    }

    for (size_t k = prologue_index + 1; k < end_index; ++k) {
        std::string candidate;
        if (match_const_hl_temp_pair_store(lines, k, lo_offset, candidate)) {
            if (found && candidate != found_immediate)
                return false;
            found = true;
            if (k < use_index)
                found_before_use = true;
            found_immediate = candidate;
            k += 2;
            continue;
        }

        if (line_writes_ix_offset(lines[k], lo_offset) ||
            line_writes_ix_offset(lines[k], lo_offset + 1)) {
            return false;
        }
    }

    if (!found_before_use)
        return false;
    immediate = found_immediate;
    return true;
}

static bool match_frameaddr_hl_temp_pair_store(
        const std::vector<asm_line> &lines,
        size_t i,
        int lo_offset,
        int sp_ix_delta,
        int &ix_offset) {
    if (i + 3 >= lines.size())
        return false;

    std::string dst;
    std::string src;
    int immediate = 0;
    if (!lines[i].label.empty() || !lines[i + 1].label.empty())
        return false;
    if (lines[i].mnemonic != "ld" ||
        !split_ld(lines[i].operands, dst, src) ||
        trim(dst) != "hl" ||
        !parse_immediate_value(src, immediate)) {
        return false;
    }
    if (lines[i + 1].mnemonic != "add" ||
        !split_ld(lines[i + 1].operands, dst, src) ||
        trim(dst) != "hl" || trim(src) != "sp") {
        return false;
    }

    int adjust = 0;
    size_t store_lo_idx = i + 2;
    while (store_lo_idx < lines.size() &&
           lines[store_lo_idx].label.empty() &&
           (lines[store_lo_idx].mnemonic == "inc" ||
            lines[store_lo_idx].mnemonic == "dec") &&
           trim(lines[store_lo_idx].operands) == "hl") {
        adjust += lines[store_lo_idx].mnemonic == "inc" ? 1 : -1;
        if (adjust > 64 || adjust < -64)
            return false;
        ++store_lo_idx;
    }
    if (store_lo_idx + 1 >= lines.size())
        return false;

    std::string store_src;
    int store_lo = 0;
    if (lines[store_lo_idx].mnemonic != "ld" ||
        !split_ld(lines[store_lo_idx].operands, dst, store_src) ||
        !parse_ix_ref(trim(dst), store_lo) ||
        store_lo != lo_offset ||
        trim(store_src) != "l") {
        return false;
    }

    int store_hi = 0;
    if (lines[store_lo_idx + 1].mnemonic != "ld" ||
        !split_ld(lines[store_lo_idx + 1].operands, dst, store_src) ||
        !parse_ix_ref(trim(dst), store_hi) ||
        store_hi != lo_offset + 1 ||
        trim(store_src) != "h") {
        return false;
    }

    ix_offset = immediate + sp_ix_delta + adjust;
    return ix_offset >= -128 && ix_offset <= 127;
}

static bool find_frameaddr_temp_word_before_use(
        const std::vector<asm_line> &lines,
        size_t prologue_index,
        size_t use_index,
        int lo_offset,
        int sp_ix_delta,
        int &ix_offset) {
    bool found = false;
    bool found_before_use = false;
    int found_offset = 0;

    size_t end_index = lines.size();
    for (size_t k = prologue_index + 1; k < lines.size(); ++k) {
        int ignored_locals = 0;
        int ignored_temp_frame = 0;
        if (parse_frame_comment(lines[k].comment, ignored_locals,
                                ignored_temp_frame) ||
            is_section_directive(lines[k])) {
            end_index = k;
            break;
        }
    }

    for (size_t k = prologue_index + 1; k < end_index; ++k) {
        int candidate = 0;
        if (match_frameaddr_hl_temp_pair_store(lines, k, lo_offset,
                                               sp_ix_delta, candidate)) {
            if (found && candidate != found_offset)
                return false;
            found = true;
            if (k < use_index)
                found_before_use = true;
            found_offset = candidate;

            // Skip over the matched variable-length materialization.
            k += 2;
            while (k < end_index &&
                   (lines[k].mnemonic == "inc" ||
                    lines[k].mnemonic == "dec") &&
                   trim(lines[k].operands) == "hl") {
                ++k;
            }
            if (k < end_index)
                ++k;
            continue;
        }

        if (line_writes_ix_offset(lines[k], lo_offset) ||
            line_writes_ix_offset(lines[k], lo_offset + 1)) {
            return false;
        }
    }

    if (!found_before_use)
        return false;
    ix_offset = found_offset;
    return true;
}

static bool parse_accumulator_immediate_alu(const asm_line &line,
                                            const char *mnemonic,
                                            int &value) {
    if (line.mnemonic != mnemonic)
        return false;

    if (line.mnemonic == "add" || line.mnemonic == "adc" ||
        line.mnemonic == "sbc") {
        std::string dst, src;
        return split_ld(line.operands, dst, src) &&
               trim(dst) == "a" && parse_immediate_value(src, value);
    }

    return parse_immediate_value(line.operands, value);
}

static bool is_accumulator_reg_alu(const asm_line &line,
                                   const char *mnemonic,
                                   const std::string &reg) {
    if (line.mnemonic != mnemonic)
        return false;

    const std::string ops = trim(line.operands);
    if (ops == reg)
        return true;

    std::string dst, src;
    return split_ld(ops, dst, src) &&
           trim(dst) == "a" && trim(src) == reg;
}

static bool line_is_flag_only_setter(const asm_line &line) {
    if (line.mnemonic == "ccf" || line.mnemonic == "scf")
        return trim(line.operands).empty();
    if (is_or_a_self(line) || is_and_a_self(line))
        return true;

    const std::string &m = line.mnemonic;
    if (m == "cp")
        return !operand_uses_memory(line.operands);
    if (m == "bit") {
        int bit = 0;
        std::string reg;
        return parse_bit_reg_operands(line.operands, bit, reg) &&
               !operand_uses_memory(reg);
    }
    if (m == "xor")
        return immediate_is(line.operands, 0);
    if (m == "and")
        return immediate_is(line.operands, 255);
    if (m == "sub")
        return immediate_is(line.operands, 0);
    if (m == "add") {
        std::string dst, src;
        return split_ld(line.operands, dst, src) &&
               trim(dst) == "a" && immediate_is(src, 0);
    }

    return false;
}

static bool line_overwrites_flags_without_reading_carry(const asm_line &line) {
    const std::string &m = line.mnemonic;
    if (m == "pop")
        return trim(line.operands) == "af";
    if (m == "neg")
        return trim(line.operands).empty();
    if (m == "cp" || m == "sub" || m == "and" || m == "or" || m == "xor")
        return true;
    if (m == "add") {
        std::string dst, src;
        return split_ld(line.operands, dst, src) && trim(dst) == "a";
    }
    if (m == "sla" || m == "sra" || m == "srl" ||
        m == "rlc" || m == "rrc") {
        return true;
    }
    return false;
}

static bool line_may_read_flags_or_escape(const asm_line &line) {
    const std::string &m = line.mnemonic;
    const std::string ops = trim(line.operands);

    if (m == "adc" || m == "sbc" || m == "rl" || m == "rr" ||
        m == "rla" || m == "rra" || m == "ccf" || m == "daa") {
        return true;
    }
    if (m == "push" && ops == "af")
        return true;
    if (m == "ex" && (ops == "af, af'" || ops == "af', af"))
        return true;

    auto has_condition = [](const std::string &text) {
        static const char *const conds[] = {
            "z", "nz", "c", "nc", "m", "p", "pe", "po", nullptr
        };
        for (int k = 0; conds[k]; ++k) {
            const std::string cc = conds[k];
            if (text == cc)
                return true;
            if (text.size() > cc.size() &&
                text.compare(0, cc.size(), cc) == 0 &&
                (text[cc.size()] == ',' ||
                 std::isspace(static_cast<unsigned char>(text[cc.size()])))) {
                return true;
            }
        }
        return false;
    };

    if ((m == "jp" || m == "jr" || m == "call" || m == "ret") &&
        has_condition(ops)) {
        return true;
    }

    // Any control-flow escape is a conservative boundary: the target may
    // observe the current flags even if this particular instruction does not.
    return m == "jp" || m == "jr" || m == "call" || m == "ret" ||
           m == "reti" || m == "retn" || m == "rst" || m == "djnz";
}

static bool flags_overwritten_before_read_or_escape(
        const std::vector<asm_line> &lines,
        size_t start,
        size_t budget,
        std::unordered_set<size_t> &active) {
    if (start >= lines.size() || budget == 0)
        return false;
    if (!active.insert(start).second)
        return false;

    auto finish = [&](bool result) {
        active.erase(start);
        return result;
    };

    for (size_t k = start; k < lines.size() && budget > 0; ++k, --budget) {
        const auto &line = lines[k];
        if (line.mnemonic.empty())
            continue;
        if (line_overwrites_flags_without_reading_carry(line))
            return finish(true);

        std::string target;
        if (parse_unconditional_jump(line, target)) {
            const size_t target_idx = find_label_index(lines, target);
            if (target_idx == lines.size())
                return finish(false);
            return finish(flags_overwritten_before_read_or_escape(
                lines, target_idx, budget, active));
        }

        if (line_may_read_flags_or_escape(line))
            return finish(false);
    }

    return finish(false);
}

static bool flags_overwritten_before_read_or_escape(
        const std::vector<asm_line> &lines,
        size_t start) {
    std::unordered_set<size_t> active;
    return flags_overwritten_before_read_or_escape(lines, start, 64, active);
}

static bool flags_overwritten_or_de_return_before_read(
        const std::vector<asm_line> &lines,
        size_t start,
        size_t budget,
        std::unordered_set<size_t> &active) {
    if (start >= lines.size() || budget == 0)
        return false;
    if (!active.insert(start).second)
        return false;

    auto finish = [&](bool result) {
        active.erase(start);
        return result;
    };

    auto next_instruction = [&](size_t &pos) -> const asm_line * {
        while (pos < lines.size()) {
            const asm_line &line = lines[pos++];
            if (!line.mnemonic.empty())
                return &line;
        }
        return nullptr;
    };

    auto source_reads_a = [](const std::string &src) {
        return operand_has_token(src, "a") || operand_has_token(src, "af");
    };

    auto de_return_tail_from = [&](size_t pos) {
        const asm_line *first = next_instruction(pos);
        if (!first || first->mnemonic != "ld")
            return false;

        std::string dst;
        std::string src;
        if (!split_ld(first->operands, dst, src))
            return false;
        dst = trim(dst);
        src = trim(src);
        if (dst == "de") {
            if (source_reads_a(src))
                return false;
            return is_modern_return_tail(lines, pos);
        }
        if (dst != "e" || source_reads_a(src))
            return false;

        const asm_line *second = next_instruction(pos);
        if (!second || second->mnemonic != "ld")
            return false;
        if (!split_ld(second->operands, dst, src))
            return false;
        dst = trim(dst);
        src = trim(src);
        if (dst != "d" || source_reads_a(src))
            return false;
        return is_modern_return_tail(lines, pos);
    };

    for (size_t k = start; k < lines.size() && budget > 0; ++k, --budget) {
        const auto &line = lines[k];
        if (line.mnemonic.empty())
            continue;
        if (line_overwrites_flags_without_reading_carry(line))
            return finish(true);
        if (is_modern_return_tail(lines, k))
            return finish(true);
        if (de_return_tail_from(k))
            return finish(true);

        std::string target;
        if (parse_unconditional_jump(line, target)) {
            const size_t target_idx = find_label_index(lines, target);
            if (target_idx == lines.size())
                return finish(false);
            return finish(flags_overwritten_or_de_return_before_read(
                lines, target_idx, budget, active));
        }

        if (line_may_read_flags_or_escape(line))
            return finish(false);
    }

    return finish(false);
}

[[maybe_unused]] static bool flags_overwritten_or_de_return_before_read(
        const std::vector<asm_line> &lines,
        size_t start) {
    std::unordered_set<size_t> active;
    return flags_overwritten_or_de_return_before_read(lines, start, 64, active);
}

static bool condition_reads_carry(const std::string &cc) {
    return cc == "c" || cc == "nc";
}

static bool condition_reads_noncarry(const std::string &cc) {
    return !condition_reads_carry(cc);
}

static bool is_8bit_inc_dec_target(const std::string &operand) {
    const std::string op = lower_copy(trim(operand));
    if (op == "bc" || op == "de" || op == "hl" || op == "sp" ||
        op == "ix" || op == "iy") {
        return false;
    }
    int ignored = 0;
    return is_plain_8bit_reg(op) || op == "(hl)" || parse_ixiy_ref(op, ignored);
}

static bool line_overwrites_noncarry_flags_without_reading_flags(
        const asm_line &line) {
    const std::string &m = line.mnemonic;
    if (line_overwrites_flags_without_reading_carry(line))
        return true;
    if ((m == "inc" || m == "dec") &&
        is_8bit_inc_dec_target(line.operands)) {
        return true;
    }
    if (m == "bit") {
        int bit = 0;
        std::string reg;
        return parse_bit_reg_operands(line.operands, bit, reg);
    }
    return false;
}

static bool flags_from_compare_dead_before_read(
        const std::vector<asm_line> &lines,
        size_t start,
        bool carry_live,
        bool noncarry_live,
        size_t budget,
        std::unordered_set<size_t> &active) {
    if (!carry_live && !noncarry_live)
        return true;
    if (start >= lines.size() || budget == 0)
        return false;

    const size_t state_key =
        (start << 2) | (carry_live ? 1u : 0u) |
        (noncarry_live ? 2u : 0u);
    if (!active.insert(state_key).second)
        return true;

    auto finish = [&](bool result) {
        active.erase(state_key);
        return result;
    };

    for (size_t k = start; k < lines.size() && budget > 0; ++k, --budget) {
        const auto &line = lines[k];
        if (line.mnemonic.empty())
            continue;
        if (is_modern_return_tail(lines, k))
            return finish(true);

        if (line_overwrites_flags_without_reading_carry(line))
            return finish(true);

        if (line_overwrites_noncarry_flags_without_reading_flags(line)) {
            noncarry_live = false;
            if (!carry_live)
                return finish(true);
            continue;
        }

        if (line.mnemonic == "scf" && trim(line.operands).empty()) {
            carry_live = false;
            if (!noncarry_live)
                return finish(true);
            continue;
        }

        std::string cc;
        std::string target;
        if (split_conditional_branch_target(line, cc, target)) {
            if ((condition_reads_carry(cc) && carry_live) ||
                (condition_reads_noncarry(cc) && noncarry_live)) {
                return finish(false);
            }

            const size_t target_idx = find_label_index(lines, target);
            if (target_idx == lines.size())
                return finish(false);
            const bool taken = flags_from_compare_dead_before_read(
                lines, target_idx, carry_live, noncarry_live, budget, active);
            const bool fallthrough = flags_from_compare_dead_before_read(
                lines, k + 1, carry_live, noncarry_live, budget, active);
            return finish(taken && fallthrough);
        }

        if (parse_unconditional_jump(line, target)) {
            const size_t target_idx = find_label_index(lines, target);
            if (target_idx == lines.size())
                return finish(false);
            return finish(flags_from_compare_dead_before_read(
                lines, target_idx, carry_live, noncarry_live, budget, active));
        }

        if (line_may_read_flags_or_escape(line))
            return finish(false);
    }

    return finish(false);
}

static bool flags_from_compare_dead_before_read(
        const std::vector<asm_line> &lines,
        size_t start) {
    std::unordered_set<size_t> active;
    return flags_from_compare_dead_before_read(
        lines, start, true, true, 96, active);
}

static bool flags_overwritten_or_call_before_read(
        const std::vector<asm_line> &lines,
        size_t start,
        size_t budget,
        std::unordered_set<size_t> &active) {
    if (start >= lines.size() || budget == 0)
        return false;
    if (!active.insert(start).second)
        return false;

    auto finish = [&](bool result) {
        active.erase(start);
        return result;
    };

    for (size_t k = start; k < lines.size() && budget > 0; ++k, --budget) {
        const auto &line = lines[k];
        if (line.mnemonic.empty())
            continue;
        if (line_overwrites_flags_without_reading_carry(line) ||
            line.mnemonic == "call") {
            return finish(true);
        }

        std::string target;
        if (parse_unconditional_jump(line, target)) {
            const size_t target_idx = find_label_index(lines, target);
            if (target_idx == lines.size())
                return finish(false);
            return finish(flags_overwritten_or_call_before_read(
                lines, target_idx, budget, active));
        }

        if (line_may_read_flags_or_escape(line))
            return finish(false);
    }

    return finish(false);
}

static bool flags_overwritten_or_call_before_read(
        const std::vector<asm_line> &lines,
        size_t start) {
    std::unordered_set<size_t> active;
    return flags_overwritten_or_call_before_read(lines, start, 64, active);
}

static bool line_overwrites_a_without_reading_it(const asm_line &line) {
    if (!line.label.empty())
        return false;
    if (line.mnemonic == "pop")
        return trim(line.operands) == "af";
    if (line.mnemonic == "xor") {
        const std::string ops = trim(line.operands);
        return ops == "a" || ops == "a,a" || ops == "a, a";
    }
    if (line.mnemonic != "ld")
        return false;

    std::string dst, src;
    if (!split_ld(line.operands, dst, src))
        return false;
    return trim(dst) == "a" &&
           !operand_has_token(src, "a") &&
           !operand_has_token(src, "af");
}

static bool line_reads_a_or_af(const asm_line &line) {
    const std::string &m = line.mnemonic;
    const std::string ops = trim(line.operands);
    if (m.empty())
        return false;
    if (m == "call" || m == "ret" || m == "reti" ||
        m == "retn" || m == "rst") {
        return true;
    }
    if (m == "push" && ops == "af")
        return true;
    if (m == "ex" && (ops == "af, af'" || ops == "af', af"))
        return true;
    if (m == "daa" || m == "cpl" || m == "neg" ||
        m == "rla" || m == "rra" || m == "rlca" || m == "rrca" ||
        m == "cp") {
        return true;
    }
    if (m == "add" || m == "adc" || m == "sbc") {
        std::string dst, src;
        if (split_ld(line.operands, dst, src)) {
            dst = trim(dst);
            if (dst == "hl" || dst == "ix" || dst == "iy")
                return false;
        }
        return !line_overwrites_a_without_reading_it(line);
    }
    if (m == "sub" || m == "and" || m == "or" || m == "xor") {
        return !line_overwrites_a_without_reading_it(line);
    }
    if ((m == "inc" || m == "dec") && ops == "a")
        return true;
    if (m == "rl" || m == "rr" || m == "sla" || m == "sra" ||
        m == "srl" || m == "rlc" || m == "rrc" ||
        m == "bit" || m == "set" || m == "res") {
        return operand_has_token(ops, "a");
    }
    if (m == "ld") {
        std::string dst, src;
        if (!split_ld(line.operands, dst, src))
            return operand_has_token(ops, "a") || operand_has_token(ops, "af");
        dst = trim(dst);
        src = trim(src);
        if (dst == "a")
            return operand_has_token(src, "a") || operand_has_token(src, "af");
        return operand_has_token(src, "a") || operand_has_token(src, "af");
    }
    return operand_has_token(ops, "a") || operand_has_token(ops, "af");
}

static bool a_dead_before_read_or_call(const std::vector<asm_line> &lines,
                                       size_t start,
                                       size_t budget = 32) {
    bool a_live = true;
    const size_t end = std::min(lines.size(), start + budget);
    for (size_t k = start; k < end; ++k) {
        const asm_line &line = lines[k];
        if (line.mnemonic.empty())
            continue;
        if (!line.label.empty())
            return false;
        if (is_section_directive(line))
            return false;
        if (!line.mnemonic.empty() && line.mnemonic[0] == '.')
            continue;
        if (line.mnemonic == "call")
            return true;
        if (line.mnemonic == "ret" || line.mnemonic == "reti" ||
            line.mnemonic == "retn" || line.mnemonic == "rst" ||
            line.mnemonic == "jp" || line.mnemonic == "jr" ||
            line.mnemonic == "djnz") {
            return false;
        }
        if (a_live && line_reads_a_or_af(line))
            return false;
        if (line_overwrites_a_without_reading_it(line))
            a_live = false;
    }
    return false;
}

static bool a_overwritten_before_read(
        const std::vector<asm_line> &lines,
        size_t start,
        size_t budget,
        std::unordered_set<size_t> &active) {
    if (start >= lines.size() || budget == 0)
        return false;
    if (!active.insert(start).second)
        return false;

    auto finish = [&](bool result) {
        active.erase(start);
        return result;
    };

    for (size_t k = start; k < lines.size() && budget > 0; ++k, --budget) {
        const auto &line = lines[k];
        if (line.mnemonic.empty())
            continue;
        if (line_overwrites_a_without_reading_it(line))
            return finish(true);

        std::string cc;
        std::string target;
        if (split_conditional_branch_target(line, cc, target)) {
            const size_t target_idx = find_label_index(lines, target);
            if (target_idx == lines.size())
                return finish(false);
            const bool taken =
                a_overwritten_before_read(lines, target_idx, budget, active);
            const bool fallthrough =
                a_overwritten_before_read(lines, k + 1, budget, active);
            return finish(taken && fallthrough);
        }

        if (parse_unconditional_jump(line, target)) {
            const size_t target_idx = find_label_index(lines, target);
            if (target_idx == lines.size())
                return finish(false);
            return finish(a_overwritten_before_read(
                lines, target_idx, budget, active));
        }

        if (line_reads_a_or_af(line))
            return finish(false);
    }

    return finish(false);
}

static bool a_overwritten_before_read(
        const std::vector<asm_line> &lines,
        size_t start) {
    std::unordered_set<size_t> active;
    return a_overwritten_before_read(lines, start, 64, active);
}

static bool a_overwritten_or_de_return_before_read(
        const std::vector<asm_line> &lines,
        size_t start,
        size_t budget,
        std::unordered_set<size_t> &active) {
    if (start >= lines.size() || budget == 0)
        return false;
    if (!active.insert(start).second)
        return false;

    auto finish = [&](bool result) {
        active.erase(start);
        return result;
    };

    auto next_instruction = [&](size_t &pos) -> const asm_line * {
        while (pos < lines.size()) {
            const asm_line &line = lines[pos++];
            if (!line.mnemonic.empty())
                return &line;
        }
        return nullptr;
    };

    auto source_reads_a = [](const std::string &src) {
        return operand_has_token(src, "a") || operand_has_token(src, "af");
    };

    auto de_return_tail_from = [&](size_t pos) {
        const asm_line *first = next_instruction(pos);
        if (!first || first->mnemonic != "ld")
            return false;

        std::string dst;
        std::string src;
        if (!split_ld(first->operands, dst, src))
            return false;
        dst = trim(dst);
        src = trim(src);

        if (dst == "de") {
            if (source_reads_a(src))
                return false;
            return is_modern_return_tail(lines, pos);
        }

        if (dst != "e" || source_reads_a(src))
            return false;

        const asm_line *second = next_instruction(pos);
        if (!second || second->mnemonic != "ld")
            return false;
        if (!split_ld(second->operands, dst, src))
            return false;
        dst = trim(dst);
        src = trim(src);
        if (dst != "d" || source_reads_a(src))
            return false;

        return is_modern_return_tail(lines, pos);
    };

    for (size_t k = start; k < lines.size() && budget > 0; ++k, --budget) {
        const auto &line = lines[k];
        if (line.mnemonic.empty())
            continue;
        if (line_overwrites_a_without_reading_it(line))
            return finish(true);
        if (is_modern_return_tail(lines, k))
            return finish(true);
        if (de_return_tail_from(k))
            return finish(true);

        std::string cc;
        std::string target;
        if (split_conditional_branch_target(line, cc, target)) {
            const size_t target_idx = find_label_index(lines, target);
            if (target_idx == lines.size())
                return finish(false);
            const bool taken =
                a_overwritten_or_de_return_before_read(lines, target_idx,
                                                       budget, active);
            const bool fallthrough =
                a_overwritten_or_de_return_before_read(lines, k + 1,
                                                       budget, active);
            return finish(taken && fallthrough);
        }

        if (parse_unconditional_jump(line, target)) {
            const size_t target_idx = find_label_index(lines, target);
            if (target_idx == lines.size())
                return finish(false);
            return finish(a_overwritten_or_de_return_before_read(
                lines, target_idx, budget, active));
        }

        if (line_reads_a_or_af(line))
            return finish(false);
    }

    return finish(false);
}

static bool a_overwritten_or_de_return_before_read(
        const std::vector<asm_line> &lines,
        size_t start) {
    std::unordered_set<size_t> active;
    return a_overwritten_or_de_return_before_read(lines, start, 64, active);
}

static bool a_overwritten_or_call_before_read(
        const std::vector<asm_line> &lines,
        size_t start,
        size_t budget,
        std::unordered_set<size_t> &active) {
    if (start >= lines.size() || budget == 0)
        return false;
    if (!active.insert(start).second)
        return false;

    auto finish = [&](bool result) {
        active.erase(start);
        return result;
    };

    for (size_t k = start; k < lines.size() && budget > 0; ++k, --budget) {
        const auto &line = lines[k];
        if (line.mnemonic.empty())
            continue;
        if (line_overwrites_a_without_reading_it(line) ||
            line.mnemonic == "call") {
            return finish(true);
        }

        std::string cc;
        std::string target;
        if (split_conditional_branch_target(line, cc, target)) {
            const size_t target_idx = find_label_index(lines, target);
            if (target_idx == lines.size())
                return finish(false);
            const bool taken = a_overwritten_or_call_before_read(
                lines, target_idx, budget, active);
            const bool fallthrough = a_overwritten_or_call_before_read(
                lines, k + 1, budget, active);
            return finish(taken && fallthrough);
        }

        if (parse_unconditional_jump(line, target)) {
            const size_t target_idx = find_label_index(lines, target);
            if (target_idx == lines.size())
                return finish(false);
            return finish(a_overwritten_or_call_before_read(
                lines, target_idx, budget, active));
        }

        if (line_reads_a_or_af(line))
            return finish(false);
    }

    return finish(false);
}

static bool a_overwritten_or_call_before_read(
        const std::vector<asm_line> &lines,
        size_t start) {
    std::unordered_set<size_t> active;
    return a_overwritten_or_call_before_read(lines, start, 64, active);
}

static bool is_unconditional_escape(const asm_line &line) {
    if (line.mnemonic == "ret" || line.mnemonic == "reti" ||
        line.mnemonic == "retn" || line.mnemonic == "rst") {
        return true;
    }
    std::string target;
    return parse_unconditional_jump(line, target);
}

static size_t previous_instruction_index(const std::vector<asm_line> &lines,
                                         size_t before) {
    while (before > 0) {
        --before;
        if (!lines[before].mnemonic.empty())
            return before;
    }
    return lines.size();
}

static bool line_preserves_a_value(const asm_line &line) {
    if (line.mnemonic.empty())
        return true;
    if (is_section_directive(line))
        return false;

    const std::string &m = line.mnemonic;
    const std::string ops = trim(line.operands);
    if (m == ".globl" || m == ".global" || m == ".optsdcc" || m == ".set")
        return true;
    if (m == "cp" || is_or_a_self(line) || is_and_a_self(line))
        return true;
    if ((m == "jp" || m == "jr") && ops.find(',') != std::string::npos)
        return true;
    if ((m == "inc" || m == "dec") && ops != "a")
        return true;
    if ((m == "set" || m == "res" || m == "bit") &&
        !operand_has_token(ops, "a") &&
        !operand_has_token(ops, "af")) {
        return true;
    }
    if (m == "ld") {
        std::string dst;
        std::string src;
        if (!split_ld(line.operands, dst, src))
            return !operand_has_token(ops, "a") &&
                   !operand_has_token(ops, "af");
        dst = trim(dst);
        return dst != "a" && dst != "af";
    }
    if (m == "push")
        return ops != "af";
    if (m == "pop")
        return ops != "af";
    if (m == "add") {
        std::string dst;
        std::string src;
        return split_ld(line.operands, dst, src) && trim(dst) != "a";
    }

    return !line_reads_a_or_af(line) &&
           !line_overwrites_a_without_reading_it(line);
}

static bool is_accumulator_shift_without_carry_input(const asm_line &line) {
    if (line.mnemonic != "sla" && line.mnemonic != "sra" &&
        line.mnemonic != "srl" && line.mnemonic != "rlc" &&
        line.mnemonic != "rrc") {
        return false;
    }
    return trim(line.operands) == "a";
}

static int estimate_instruction_size(const asm_line &line) {
    if (line.mnemonic.empty())
        return 0;

    if (is_section_directive(line) ||
        line.mnemonic == ".globl" || line.mnemonic == ".global" ||
        line.mnemonic == ".optsdcc" || line.mnemonic == ".set") {
        return 0;
    }

    if (line.mnemonic == ".db" || line.mnemonic == ".byte")
        return count_csv_items(line.operands);
    if (line.mnemonic == ".dw" || line.mnemonic == ".short")
        return 2 * count_csv_items(line.operands);
    if (line.mnemonic == ".dl" || line.mnemonic == ".long")
        return 4 * count_csv_items(line.operands);
    if (line.mnemonic == ".ds" || line.mnemonic == ".space") {
        std::string n = trim(line.operands);
        return is_numeric_literal(n) ? std::stoi(n[0] == '#' ? n.substr(1) : n) : 0;
    }

    if (line.mnemonic == "ret" || line.mnemonic == "reti" || line.mnemonic == "retn" ||
        line.mnemonic == "nop" || line.mnemonic == "halt" || line.mnemonic == "di" ||
        line.mnemonic == "ei" || line.mnemonic == "cpl" || line.mnemonic == "ccf" ||
        line.mnemonic == "scf" || line.mnemonic == "neg" || line.mnemonic == "rla" ||
        line.mnemonic == "rlca" || line.mnemonic == "rra" || line.mnemonic == "rrca" ||
        line.mnemonic == "rld" || line.mnemonic == "rrd" || line.mnemonic == "exx") {
        return 1;
    }

    if (line.mnemonic == "jp" || line.mnemonic == "call")
        return 3;
    if (line.mnemonic == "jr" || line.mnemonic == "djnz")
        return 2;
    if (line.mnemonic == "ldir" || line.mnemonic == "ldi" ||
        line.mnemonic == "ldd" || line.mnemonic == "cpir" ||
        line.mnemonic == "cpdr" || line.mnemonic == "cpi" ||
        line.mnemonic == "cpd") {
        return 2;
    }

    if (line.mnemonic == "push" || line.mnemonic == "pop") {
        std::string op = trim(line.operands);
        return (op == "ix" || op == "iy") ? 2 : 1;
    }

    if (line.mnemonic == "ex") {
        std::string ops = trim(line.operands);
        return (ops == "de, hl" || ops == "hl, de" ||
                ops == "af, af'" || ops == "af', af") ? 1 : 2;
    }

    if (line.mnemonic == "inc" || line.mnemonic == "dec") {
        std::string op = trim(line.operands);
        if (op == "ix" || op == "iy")
            return 2;
        if (uses_ixiy_disp(op))
            return 3;
        return 1;
    }

    if (line.mnemonic == "add" || line.mnemonic == "adc" || line.mnemonic == "sbc" ||
        line.mnemonic == "sub" || line.mnemonic == "and" || line.mnemonic == "or" ||
        line.mnemonic == "xor" || line.mnemonic == "cp") {
        std::string dst, src;
        if (!split_ld(line.operands, dst, src)) {
            std::string op = trim(line.operands);
            if (uses_ixiy_disp(op))
                return 3;
            if (uses_hl_indirect(op) || is_reg8(op) || is_reg16(op))
                return 1;
            if (is_immediate_operand(op))
                return 2;
            return is_numeric_literal(op) ? 2 : 3;
        }

        if ((trim(dst) == "ix" || trim(dst) == "iy") && is_reg16(trim(src)))
            return 2;
        if (trim(dst) == "hl" && is_reg16(trim(src)))
            return 1;
        if (uses_ixiy_disp(trim(src)))
            return 3;
        if (uses_hl_indirect(trim(src)))
            return 1;
        if (is_immediate_operand(trim(src)))
            return 2;
        if (is_numeric_literal(trim(src)))
            return 2;
        return 1;
    }

    if (line.mnemonic == "rr" || line.mnemonic == "rl" || line.mnemonic == "sra" ||
        line.mnemonic == "srl" || line.mnemonic == "sla" || line.mnemonic == "rlc" ||
        line.mnemonic == "rrc" || line.mnemonic == "bit" || line.mnemonic == "set" ||
        line.mnemonic == "res") {
        std::string op = trim(line.operands);
        if (uses_ixiy_disp(op))
            return 4;
        return 2;
    }

    if (line.mnemonic == "ld") {
        std::string dst, src;
        if (!split_ld(line.operands, dst, src))
            return 4;
        dst = trim(dst);
        src = trim(src);

        if ((dst == "sp" && src == "ix") || (dst == "sp" && src == "iy"))
            return 2;
        if ((dst == "ix" || dst == "iy") && is_numeric_literal(src))
            return 4;
        if (is_reg16(dst) && is_numeric_literal(src))
            return 3;
        if ((dst == "ix" || dst == "iy") && uses_abs_indirect(src))
            return 4;
        if ((src == "ix" || src == "iy") && uses_abs_indirect(dst))
            return 4;
        if (uses_ixiy_disp(dst) || uses_ixiy_disp(src))
            return 3;
        if (dst == "(hl)" && is_numeric_literal(src))
            return 2;
        if ((uses_abs_indirect(dst) && (src == "a" || src == "hl")) ||
            ((dst == "a" || dst == "hl") && uses_abs_indirect(src)))
            return 3;
        if (uses_hl_indirect(dst) || uses_hl_indirect(src))
            return 1;
        if (is_reg8(dst) && is_numeric_literal(src))
            return 2;
        if (is_reg8(dst) && is_reg8(src))
            return 1;
        if (is_reg16(dst) && is_reg16(src))
            return 2;
        if (is_reg16(dst))
            return 3;
        return 2;
    }

    // Conservative fallback: prefer missing a relaxation over emitting an
    // out-of-range JR due to underestimating instruction sizes.
    return 4;
}

struct code_layout_entry {
    int section = 0;
    int offset = 0;
};

struct code_layout {
    std::vector<code_layout_entry> line_pos;
    std::unordered_map<std::string, code_layout_entry> label_pos;
};

static code_layout compute_code_layout(const std::vector<asm_line> &lines) {
    code_layout layout;
    layout.line_pos.resize(lines.size());

    int section = 0;
    int offset = 0;
    for (size_t i = 0; i < lines.size(); ++i) {
        layout.line_pos[i] = {section, offset};
        if (!lines[i].label.empty())
            layout.label_pos[lines[i].label] = {section, offset};

        if (is_section_directive(lines[i])) {
            ++section;
            offset = 0;
            continue;
        }

        offset += estimate_instruction_size(lines[i]);
    }

    return layout;
}

// jp z,L_true; jp m,L_true; jp L_false; L_true:
//   -> jp m,L_true; jp nz,L_false; L_true:
//
// Signed <= branches leave "greater than zero" as the only false case after
// `sbc hl,de`. When the true block is the immediate fallthrough, a pair of
// branches is enough: negative jumps true, positive-nonzero jumps false, and
// equal falls through into the true block.
static bool rule_signed_le_fallthrough(std::vector<asm_line> &lines, size_t i) {
    if (i + 3 >= lines.size())
        return false;

    auto &l0 = lines[i];
    auto &l1 = lines[i + 1];
    auto &l2 = lines[i + 2];
    auto &l3 = lines[i + 3];

    if (!l0.label.empty() || !l1.label.empty() || !l2.label.empty())
        return false;

    std::string cc0, true_lbl;
    if (!parse_conditional_jump(l0, cc0, true_lbl) || cc0 != "z")
        return false;

    std::string cc1, true_lbl_2;
    if (!parse_conditional_jump(l1, cc1, true_lbl_2) ||
        cc1 != "m" || true_lbl_2 != true_lbl) {
        return false;
    }

    std::string false_lbl;
    if (!parse_unconditional_jump(l2, false_lbl) || false_lbl == true_lbl)
        return false;

    if (l3.label != true_lbl || !l3.mnemonic.empty())
        return false;

    lines[i] = asm_line::parse("\tjp\tm, " + true_lbl);
    lines[i + 1] = asm_line::parse("\tjp\tnz, " + false_lbl);
    lines.erase(lines.begin() + static_cast<std::ptrdiff_t>(i + 2));
    return true;
}

// ----- z80_peep ------------------------------------------------------

void z80_peep::load(const std::string &text) {
    lines_.clear();
    std::istringstream iss(text);
    std::string line;
    while (std::getline(iss, line))
        lines_.push_back(asm_line::parse(line));
}

std::string z80_peep::dump() const {
    std::string out;
    for (auto &l : lines_)
        out += l.to_string();
    return out;
}

bool z80_peep::apply_once() {
    bool changed = false;
    const bool pareto_bias = speed_bias_ || size_bias_;
    for (size_t i = 0; i + 1 < lines_.size(); ++i) {
        if (apply_structural_rules(lines_, i)) { changed = true; continue; }
        if (rule_bool_ifx_shortcircuit(i)) { changed = true; continue; }
        if (rule_signed_le_fallthrough(lines_, i)) { changed = true; continue; }
        if (rule_invert_branch_skip(i))  { changed = true; continue; }
        if (rule_cp_threshold_branch_fold(i)) { changed = true; continue; }
        if (pareto_bias && rule_cp_zero_branch_to_or(i)) { changed = true; continue; }
        if (rule_zero_cmp_optimize(i))   { changed = true; continue; }
        if (rule_signed_zero_branch_from_high_bit(i)) { changed = true; continue; }
        if (rule_de_eq_small_const_branch(i)) { changed = true; continue; }
        if (rule_de_word_ne_branch_split(i)) { changed = true; continue; }
        if (rule_signed_byte_zero_branch(i)) { changed = true; continue; }
        if (rule_page_aligned_word_bound_branch(i)) { changed = true; continue; }
        if (rule_hl_high_byte_mask_branch(i)) { changed = true; continue; }
        if (rule_word_mask_zero_high_branch(i)) { changed = true; continue; }
        if (rule_bit7_shift_xor_diamond(i)) { changed = true; continue; }
        if (rule_zero_extended_bit7_shift_xor_diamond(i)) { changed = true; continue; }
        if (rule_signed_byte_low_arith(i)) { changed = true; continue; }
        if (rule_de_byte_alu_shuttle(i)) { changed = true; continue; }
        if (rule_c_byte_alu_shuttle(i)) { changed = true; continue; }
        if (rule_zero_extended_byte_shr_to_a(i)) { changed = true; continue; }
        if (rule_low_byte_shift_add_to_a(i)) { changed = true; continue; }
        if (rule_dead_a_store_before_join_store(i)) { changed = true; continue; }
        if (rule_ix_byte_branch_temp_to_d(i)) { changed = true; continue; }
        if (rule_bit15_shift_xor_diamond(i)) { changed = true; continue; }
        if (rule_masked_bit15_shift_xor_diamond(i)) { changed = true; continue; }
        if (rule_lsb32_shift_xor_diamond(i)) { changed = true; continue; }
        if (rule_redundant_u8_self_mask(i)) { changed = true; continue; }
        if (rule_zero_extended_byte_mask_branch(i)) { changed = true; continue; }
        if (rule_zero_extended_byte_store_direct(i)) { changed = true; continue; }
        if (rule_zero_extended_mask_inc_word_store(i)) { changed = true; continue; }
        if (rule_word_reload_low_byte_to_a(i)) { changed = true; continue; }
        if (rule_hl_nonzero_materialize(i)) { changed = true; continue; }
        if (rule_ix_word_store_zero_test_from_pair(i)) { changed = true; continue; }
        if (rule_ix_hl_store_zero_test_reload_elide(i)) { changed = true; continue; }
        if (rule_ix_word_zero_test_direct(i)) { changed = true; continue; }
        if (rule_ex_de_hl_load_double(i)) { changed = true; continue; }
        if (rule_ix_store_reload(i))     { changed = true; continue; }
        if (rule_ix_store32_low_reload(i)) { changed = true; continue; }
        if (rule_ix_postinc_indirect_load_a(i)) { changed = true; continue; }
        if (rule_ix_postinc_indexed_store_a(i)) { changed = true; continue; }
        if (rule_adjacent_postinc_indexed_stores_direct(i)) { changed = true; continue; }
        if (rule_ix_indexed_stack_immediate_store_run(i)) { changed = true; continue; }
        if (size_bias_ && rule_small_stack_alloc_push_af(i)) { changed = true; continue; }
        if (rule_ix_postinc_local_immediate_store(i)) { changed = true; continue; }
        if (rule_dead_a_indexed_load_shuttle(i)) { changed = true; continue; }
        if (rule_dead_a_indexed_immediate_store(i)) { changed = true; continue; }
        if (rule_a_temp_reload_after_preserving_branch(i)) { changed = true; continue; }
        if (rule_byte_temp_zero_extend_after_test(i)) { changed = true; continue; }
        if (rule_dead_de_spill_reload_exchange(i)) { changed = true; continue; }
        if (rule_call_result_byte_commute_direct(i)) { changed = true; continue; }
        if (rule_hl_to_de_before_hl_reload_exchange(i)) { changed = true; continue; }
        if (rule_adjacent_byte_arg_push_pair(i)) { changed = true; continue; }
        if (rule_ix_local_byte_store_direct(i)) { changed = true; continue; }
        if (rule_ix_temp_ptr_immediate_store_direct(i)) { changed = true; continue; }
        if (rule_ix_addr_materialize_sp_relative(i)) { changed = true; continue; }
        if (rule_dead_hl_sp_frameaddr_calc(i)) { changed = true; continue; }
        if (rule_ix_byte_left_shift_xor_temp_elide(i)) { changed = true; continue; }
        if (rule_ix_byte_right_shift_xor_temp_elide(i)) { changed = true; continue; }
        if (rule_truncated_promoted_byte_xor_elide(i)) { changed = true; continue; }
        if (rule_ix_word_temp_switch_key_de_direct(i)) { changed = true; continue; }
        if (rule_de_word_temp_reload_to_hl_stack_preserve(i)) { changed = true; continue; }
        if (rule_de_word_temp_reload_after_address_calc_elide(i)) { changed = true; continue; }
        if (rule_hl_word_temp_reload_after_address_calc_elide(i)) { changed = true; continue; }
        if (rule_ix_scaled_offset_temp_elide(i)) { changed = true; continue; }
        if (rule_ix_scaled_offset_immediate_base_elide(i)) { changed = true; continue; }
        if (rule_de_scaled_offset_immediate_base_elide(i)) { changed = true; continue; }
        if (rule_ix_pointer_scan_temp_to_hl_loop(i)) { changed = true; continue; }
        if (rule_ix_index14_scaled_base_temp_elide(i)) { changed = true; continue; }
        if (rule_dead_hl_zero_extend_before_pair_load(i)) { changed = true; continue; }
        if (rule_dead_hl_pair_load(i)) { changed = true; continue; }
        if (rule_dead_hl_ix_load(i))     { changed = true; continue; }
        if (rule_temp_store_reload(i))   { changed = true; continue; }
        if (rule_hl_immediate_store_direct(i)) { changed = true; continue; }
        if (rule_pair_copy_offset_materialize(i)) { changed = true; continue; }
        if (rule_pair_immediate_copy_reload_elide(i)) { changed = true; continue; }
        if (rule_temp_base_subtract_result_exchange(i)) { changed = true; continue; }
        if (rule_bc_immediate_copy_to_pair_direct(i)) { changed = true; continue; }
        if (rule_const_add_bc_de_fold(i)) { changed = true; continue; }
        if (rule_push_hl_ix_pop_de(i))   { changed = true; continue; }
        if (rule_push_hl_load_pop_de(i)) { changed = true; continue; }
        if (rule_push_hl_pop_bc(i))      { changed = true; continue; }
        if (rule_dead_hl_de_stack_copy_to_bc(i)) { changed = true; continue; }
        if (rule_de_result_hl_forward(i)) { changed = true; continue; }
        if (rule_push_de_pop_hl_to_ex(i)) { changed = true; continue; }
        if (rule_pop_bc_run_sp_adjust(i)) { changed = true; continue; }
        if (rule_dead_bc_zero_extend_from_a(i)) { changed = true; continue; }
        if (rule_bc_indirect_through_hl(i)) { changed = true; continue; }
        if (rule_dead_bc_copy_from_hl(i)) { changed = true; continue; }
        if (rule_dead_bc_hl_roundtrip(i)) { changed = true; continue; }
        if (rule_bc_base_add_direct(i)) { changed = true; continue; }
        if (rule_bc_offset_base_add_de_direct(i)) { changed = true; continue; }
        if (rule_bc_index_add_hl_word_load_direct(i)) { changed = true; continue; }
        if (rule_bc_index_add_reloaded_hl_to_de(i)) { changed = true; continue; }
        if (rule_bc_saved_hl_push_word_to_de_direct(i)) { changed = true; continue; }
        if (rule_dead_bc_hl_to_de_copy(i)) { changed = true; continue; }
        if (rule_de_hl_equal_load_exchange(i)) { changed = true; continue; }
        if (rule_adjacent_indexed_byte_stores_postinc(i)) { changed = true; continue; }
        if (rule_ix_pair_compare_load_de_direct(i)) { changed = true; continue; }
        if (speed_bias_ && rule_push_pair_copy_adjacent_speed(i)) {
            changed = true;
            continue;
        }
        if (pareto_bias && rule_add_a_one_to_inc(i)) {
            changed = true;
            continue;
        }
        if (pareto_bias && rule_redundant_a_reload_after_zero_extend(i)) {
            changed = true;
            continue;
        }
        if (pareto_bias && rule_superopt_accumulator_sequences(i)) {
            changed = true;
            continue;
        }
        if (pareto_bias && rule_superopt_lowbit_pair_sequences(i)) {
            changed = true;
            continue;
        }
        if (pareto_bias && rule_superopt_srl_a_const_shift(i)) {
            changed = true;
            continue;
        }
        if (speed_bias_ && rule_superopt_shift5_pair_loop_unroll(i)) {
            changed = true;
            continue;
        }
        if (speed_bias_ && rule_superopt_hoist_d_zero_index_loop(i)) {
            changed = true;
            continue;
        }
        if (pareto_bias && rule_superopt_dead_flag_setter(i)) {
            changed = true;
            continue;
        }
        if (rule_superopt_register_move_sequences(i)) {
            changed = true;
            continue;
        }
        if (pareto_bias && rule_superopt_separated_pair_immediate_load(i)) {
            changed = true;
            continue;
        }
        if (rule_superopt_temp_const_word_load_direct(i)) {
            changed = true;
            continue;
        }
        if (rule_pair_immediate_store_direct(i)) {
            changed = true;
            continue;
        }
        if (pareto_bias && rule_superopt_call_result_store_de_direct(i)) {
            changed = true;
            continue;
        }
        if (pareto_bias && rule_superopt_dead_bc_store_copy(i)) {
            changed = true;
            continue;
        }
        if (pareto_bias && rule_superopt_lowbyte_zero_extend_to_de(i)) {
            changed = true;
            continue;
        }
        if (rule_superopt_ix_word_inc_direct(i)) {
            changed = true;
            continue;
        }
        if (rule_superopt_ix_word_add1_direct(i)) {
            changed = true;
            continue;
        }
        if (rule_add_hl_de_one_to_inc(i)) {
            changed = true;
            continue;
        }
        if (rule_superopt_ix_byte_inc_direct(i)) {
            changed = true;
            continue;
        }
        if (rule_ix_byte_inc_test_direct(i)) {
            changed = true;
            continue;
        }
        if (rule_reg_byte_inc_dec_direct(i)) {
            changed = true;
            continue;
        }
        if (rule_divuint_remainder_bc_restore_elide(i)) {
            changed = true;
            continue;
        }
        if (rule_ix_byte_dec_direct(i)) {
            changed = true;
            continue;
        }
        if (rule_reg_byte_add_shuttle_elide(i)) {
            changed = true;
            continue;
        }
        if (rule_byte_addsub_shuttle_elide(i)) {
            changed = true;
            continue;
        }
        if (rule_byte_addsub_a_to_d_shuttle_elide(i)) {
            changed = true;
            continue;
        }
        if (rule_dead_d_in_byte_mul10(i)) {
            changed = true;
            continue;
        }
        if (rule_ix_byte_or_mask_set_direct(i)) {
            changed = true;
            continue;
        }
        if (rule_ix_byte_mul10_spill_elide(i)) {
            changed = true;
            continue;
        }
        if (rule_ix_byte_add_spill_elide(i)) {
            changed = true;
            continue;
        }
        if (rule_ix_byte_add_spill_branch_elide(i)) {
            changed = true;
            continue;
        }
        if (rule_unsigned_le_branch_fold(i)) {
            changed = true;
            continue;
        }
        if (pareto_bias && rule_superopt_ix_byte_load_forward(i)) {
            changed = true;
            continue;
        }
        if (pareto_bias && rule_superopt_ix_byte_alu_forward(i)) {
            changed = true;
            continue;
        }
        if (pareto_bias && rule_superopt_dead_a_zero_reg(i)) {
            changed = true;
            continue;
        }
        if (rule_superopt_compare_fallthrough_reload(i)) {
            changed = true;
            continue;
        }
        if (rule_superopt_xor_compare_fallthrough_reload(i)) {
            changed = true;
            continue;
        }
        if (pareto_bias && rule_superopt_redundant_zero_store_chain(i)) {
            changed = true;
            continue;
        }
        if (pareto_bias && rule_superopt_zero_extend_pair_test_shortcut(i)) {
            changed = true;
            continue;
        }
        if (pareto_bias && rule_superopt_low_byte_xor_forward(i)) {
            changed = true;
            continue;
        }
        if (pareto_bias && rule_superopt_dead_bc_xor_hl_forward(i)) {
            changed = true;
            continue;
        }
        if (pareto_bias && rule_superopt_dead_c_xor_hl_forward(i)) {
            changed = true;
            continue;
        }
        if (pareto_bias && rule_superopt_de_xor_right5(i)) {
            changed = true;
            continue;
        }
        if (pareto_bias && rule_superopt_hl_xor_right5_stack(i)) {
            changed = true;
            continue;
        }
        if (rule_superopt_zero_extend_src_to_bc(i)) {
            changed = true;
            continue;
        }
        if (rule_superopt_zero_extend_a_to_bc(i)) {
            changed = true;
            continue;
        }
        if (pareto_bias && rule_superopt_zero_extend_truth_test(i)) {
            changed = true;
            continue;
        }
        if (pareto_bias && rule_superopt_zero_extend_dead_hl_truth_test(i)) {
            changed = true;
            continue;
        }
        if (pareto_bias && rule_superopt_zero_extend_dead_bc_truth_test(i)) {
            changed = true;
            continue;
        }
        if (pareto_bias && rule_superopt_hl_byte_load_forward(i)) {
            changed = true;
            continue;
        }
        if (rule_superopt_de_to_hl_dead_de_copy(i)) {
            changed = true;
            continue;
        }
        if (rule_superopt_dead_bc_return_copy(i)) {
            changed = true;
            continue;
        }
        if (rule_superopt_hl_via_bc_return_de_copy(i)) {
            changed = true;
            continue;
        }
        if (rule_superopt_hl_load_return_de_direct(i)) {
            changed = true;
            continue;
        }
        if (rule_superopt_ix_word_return_de_direct(i)) {
            changed = true;
            continue;
        }
        if (pareto_bias && rule_superopt_hl_return_de_exchange(i)) {
            changed = true;
            continue;
        }
        if (rule_superopt_lowbyte_sum_return_direct(i)) {
            changed = true;
            continue;
        }
        if (pareto_bias && rule_superopt_bc_to_de_alu_forward(i)) {
            changed = true;
            continue;
        }
        if (pareto_bias && rule_superopt_dead_hl_de_return_store_forward(i)) {
            changed = true;
            continue;
        }
        if (pareto_bias && rule_superopt_modern_const_return_direct(i)) {
            changed = true;
            continue;
        }
        if (pareto_bias && rule_superopt_cancel_exx_pair(i)) {
            changed = true;
            continue;
        }
        if (pareto_bias && rule_superopt_call_arg_de_direct(i)) {
            changed = true;
            continue;
        }
        if (rule_superopt_dead_hl_exchange_to_de_load(i)) {
            changed = true;
            continue;
        }
        if (pareto_bias && rule_superopt_equal_de_hl_exchange(i)) {
            changed = true;
            continue;
        }
        if (rule_superopt_exchange_sandwich_de_load(i)) {
            changed = true;
            continue;
        }
        if (pareto_bias && rule_superopt_dead_pair_stack_discard_pop(i)) {
            changed = true;
            continue;
        }
        if (pareto_bias && rule_superopt_dead_pair_pop_push(i)) {
            changed = true;
            continue;
        }
        if (speed_bias_ && rule_superopt_long_inc_sp_run(i)) {
            changed = true;
            continue;
        }
        if (speed_bias_ &&
            rule_superopt_spaghetti_load_helper_preserve_af_bc(i)) {
            changed = true;
            continue;
        }
        if (speed_bias_ &&
            rule_superopt_spaghetti_store_helper_preserve_regs(i)) {
            changed = true;
            continue;
        }
        if (speed_bias_ && rule_superopt_spaghetti_flag_helper_inline(i)) {
            changed = true;
            continue;
        }
        if (rule_push_hl_de_load(i))     { changed = true; continue; }
        if (rule_push_pair_copy_span(i)) { changed = true; continue; }
        if (rule_push_pop_same_reg_span(i)) { changed = true; continue; }
        if (rule_ix_byte_store_reload(i)) { changed = true; continue; }
        if (rule_ix_byte_store_forward(i)) { changed = true; continue; }
        if (rule_redundant_ld(i))        { changed = true; continue; }
        if (rule_self_store(i))          { changed = true; continue; }
        if (rule_dead_temp_ix_store(i))  { changed = true; continue; }
        if (rule_dead_hl_load(i))        { changed = true; continue; }
        if (rule_push_pop_hl(i))         { changed = true; continue; }
        if (rule_jp_next(i))             { changed = true; continue; }
        if (rule_jp_to_jr(i))            { changed = true; continue; }
        if (rule_or_a_or_a(i))           { changed = true; continue; }
        if (rule_ld_a_zero(i))           { changed = true; continue; }
    }
    return changed;
}

void z80_peep::apply_passes(int passes) {
    for (int p = 0; p < passes; ++p) {
        if (!apply_once()) break;
    }
    // Remove empty lines left by deletions
    lines_.erase(std::remove_if(lines_.begin(), lines_.end(),
        [](const asm_line &l) {
            return l.is_comment && l.label.empty() &&
                   l.mnemonic.empty() && l.comment.empty();
	        }), lines_.end());
}

std::string z80_peep::optimize(const std::string &asm_text,
	                               bool speed_bias,
	                                   int normal_passes,
                                       bool size_bias) {
    z80_peep p;
    p.speed_bias_ = speed_bias;
    p.size_bias_ = size_bias;
    p.load(asm_text);
    if (normal_passes > 0)
        p.apply_passes(normal_passes);
    return p.dump();
}

std::string z80_peep::optimize_outlined_layout(const std::string &asm_text) {
    z80_peep p;
    p.load(asm_text);

    for (int pass = 0; pass < 4; ++pass) {
        bool changed = false;
        for (size_t i = 0; i < p.lines_.size();) {
            asm_line &line = p.lines_[i];

            if (i + 1 < p.lines_.size() &&
                line.mnemonic == "call" &&
                trim(line.operands).rfind("__xopt_outline_", 0) == 0 &&
                p.lines_[i + 1].label.empty() &&
                is_plain_ret(p.lines_[i + 1])) {
                line.mnemonic = "jp";
                p.lines_.erase(p.lines_.begin() +
                               static_cast<std::ptrdiff_t>(i + 1));
                changed = true;
                continue;
            }

            if (i + 1 < p.lines_.size() && line.label.empty() &&
                (line.mnemonic == "jp" || line.mnemonic == "jr") &&
                line.operands.find(',') == std::string::npos &&
                !p.lines_[i + 1].label.empty() &&
                trim(line.operands) == p.lines_[i + 1].label) {
                p.lines_.erase(p.lines_.begin() +
                               static_cast<std::ptrdiff_t>(i));
                changed = true;
                continue;
            }

            if (p.rule_jp_to_jr(i))
                changed = true;
            ++i;
        }
        if (!changed)
            break;
    }
    return p.dump();
}

// ----- Rules ---------------------------------------------------------

// Remove: ld r, r  (load register from itself)
bool z80_peep::rule_redundant_ld(size_t i) {
    auto &l = lines_[i];
    if (l.mnemonic != "ld") return false;
    std::string dst, src;
    if (!split_ld(l.operands, dst, src)) return false;
    if (dst == src) {
        lines_.erase(lines_.begin() + i);
        return true;
    }
    return false;
}

// Remove: push hl; pop hl
bool z80_peep::rule_push_pop_hl(size_t i) {
    if (i + 1 >= lines_.size()) return false;
    auto &a = lines_[i];
    auto &b = lines_[i + 1];
    if (a.mnemonic == "push" && b.mnemonic == "pop" &&
        trim(a.operands) == "hl" && trim(b.operands) == "hl") {
        lines_.erase(lines_.begin() + i, lines_.begin() + i + 2);
        return true;
    }
    return false;
}

bool z80_peep::rule_cp_threshold_branch_fold(size_t i) {
    if (i + 2 >= lines_.size())
        return false;
    if (!lines_[i].label.empty() || lines_[i].mnemonic != "cp")
        return false;

    int value = 0;
    if (!parse_immediate_value(lines_[i].operands, value))
        return false;
    value = u8_value(value);

    std::string cc1;
    std::string target1;
    if (!lines_[i + 1].label.empty() ||
        !split_conditional_branch_target(lines_[i + 1], cc1, target1) ||
        cc1 != "z") {
        return false;
    }

    std::string cc2;
    std::string target2;
    if (!lines_[i + 2].label.empty() ||
        !split_conditional_branch_target(lines_[i + 2], cc2, target2)) {
        return false;
    }

    if (target1 == target2 && cc2 == "nc") {
        lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 1));
        return true;
    }

    if (target1 == target2 && cc2 == "c" && value < 255) {
        lines_[i].operands = imm8_text(value + 1);
        lines_[i + 1].mnemonic = lines_[i + 2].mnemonic;
        lines_[i + 1].operands = "c, " + target1;
        lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 2));
        return true;
    }

    if (i + 3 >= lines_.size() || !lines_[i + 3].label.empty())
        return false;
    if (cc2 == "nc" && value < 255) {
        std::string target3;
        if (parse_unconditional_jump(lines_[i + 3], target3) &&
            target3 == target1) {
            lines_[i].operands = imm8_text(value + 1);
            lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 1));
            return true;
        }
    }

    return false;
}

bool z80_peep::rule_cp_zero_branch_to_or(size_t i) {
    if (i >= lines_.size() || !lines_[i].label.empty())
        return false;

    int value = 0;
    if (!parse_accumulator_immediate_alu(lines_[i], "cp", value) ||
        u8_value(value) != 0) {
        return false;
    }

    const size_t end = std::min(lines_.size(), i + 6);
    for (size_t branch = i + 1; branch < end; ++branch) {
        const asm_line &line = lines_[branch];
        if (!line.label.empty())
            return false;
        if (line.mnemonic.empty())
            continue;

        std::string cc;
        std::string target;
        if (split_conditional_branch_target(line, cc, target)) {
            if (cc != "z" && cc != "nz" && cc != "c" && cc != "nc" &&
                cc != "m" && cc != "p") {
                return false;
            }

            const size_t target_idx = find_label_index(lines_, target);
            if (target_idx == lines_.size() ||
                !flags_from_compare_dead_before_read(lines_, branch + 1) ||
                !flags_from_compare_dead_before_read(lines_, target_idx)) {
                return false;
            }

            lines_[i].mnemonic = "or";
            lines_[i].operands = "a, a";
            return true;
        }

        const std::string operand = lower_copy(trim(line.operands));
        const bool preserves_flags =
            line.mnemonic == "ld" || line.mnemonic == "push" ||
            (line.mnemonic == "pop" && operand != "af") ||
            line.mnemonic == "exx" || line.mnemonic == "nop" ||
            (line.mnemonic == "ex" &&
             operand != "af, af'" && operand != "af', af") ||
            ((line.mnemonic == "inc" || line.mnemonic == "dec") &&
             (operand == "bc" || operand == "de" || operand == "hl" ||
              operand == "sp" || operand == "ix" || operand == "iy"));
        if (!preserves_flags)
            return false;
    }
    return false;
}

// Remove: jp label\n<label>:  (jump to next line)
bool z80_peep::rule_jp_next(size_t i) {
    if (i + 1 >= lines_.size()) return false;
    auto &a = lines_[i];
    auto &b = lines_[i + 1];
    if (a.mnemonic == "jp" && b.is_label) {
        std::string ops = trim(a.operands);
        if (ops == b.label) {
            lines_.erase(lines_.begin() + i);
            return true;
        }
    }
    return false;
}

// Remove duplicate: or a,a; or a,a
bool z80_peep::rule_or_a_or_a(size_t i) {
    if (i + 1 >= lines_.size()) return false;
    auto &a = lines_[i];
    auto &b = lines_[i + 1];
    if (a.mnemonic == "or" && b.mnemonic == "or" &&
        trim(a.operands) == "a, a" && trim(b.operands) == "a, a") {
        lines_.erase(lines_.begin() + i + 1);
        return true;
    }
    return false;
}

// Elide: dec sp; dec sp; ld N(ix),l; ld N+1(ix),h; ld l,N(ix); ld h,N+1(ix)
// The pattern allocates a temp slot, stores HL to it, then immediately reloads
// it back into HL — a complete no-op when the slot is never used afterwards.
bool z80_peep::rule_temp_store_reload(size_t i) {
    if (i + 5 >= lines_.size()) return false;

    auto &l0 = lines_[i];
    auto &l1 = lines_[i + 1];
    auto &l2 = lines_[i + 2];
    auto &l3 = lines_[i + 3];
    auto &l4 = lines_[i + 4];
    auto &l5 = lines_[i + 5];

    if (l0.mnemonic != "dec" || trim(l0.operands) != "sp") return false;
    if (l1.mnemonic != "dec" || trim(l1.operands) != "sp") return false;

    // l2: ld -N(ix), l
    if (l2.mnemonic != "ld") return false;
    std::string l2d, l2s;
    if (!split_ld(l2.operands, l2d, l2s) || l2s != "l") return false;
    int off_lo;
    if (!parse_ix_ref(l2d, off_lo)) return false;

    // l3: ld -(N-1)(ix), h  (next byte, H = high byte)
    if (l3.mnemonic != "ld") return false;
    std::string l3d, l3s;
    if (!split_ld(l3.operands, l3d, l3s) || l3s != "h") return false;
    int off_hi;
    if (!parse_ix_ref(l3d, off_hi) || off_hi != off_lo + 1) return false;

    // l4: ld l, -N(ix)
    if (l4.mnemonic != "ld") return false;
    std::string l4d, l4s;
    if (!split_ld(l4.operands, l4d, l4s) || l4d != "l") return false;
    int off_lo2;
    if (!parse_ix_ref(l4s, off_lo2) || off_lo2 != off_lo) return false;

    // l5: ld h, -(N-1)(ix)
    if (l5.mnemonic != "ld") return false;
    std::string l5d, l5s;
    if (!split_ld(l5.operands, l5d, l5s) || l5d != "h") return false;
    int off_hi2;
    if (!parse_ix_ref(l5s, off_hi2) || off_hi2 != off_hi) return false;

    // Only safe if the slot is never referenced after this block.
    std::string lo_ref = std::to_string(off_lo) + "(ix)";
    std::string hi_ref = std::to_string(off_hi) + "(ix)";
    for (size_t j = i + 6; j < lines_.size(); ++j) {
        if (lines_[j].operands.find(lo_ref) != std::string::npos) return false;
        if (lines_[j].operands.find(hi_ref) != std::string::npos) return false;
    }

    lines_.erase(lines_.begin() + i, lines_.begin() + i + 6);
    return true;
}

// push hl; ld hl,X; pop de  →  ex de,hl; ld hl,X
// Fires for any single "ld hl,<anything>" between the push and pop,
// as long as the source does not address DE.
bool z80_peep::rule_push_hl_load_pop_de(size_t i) {
    if (i + 2 >= lines_.size()) return false;
    auto &a = lines_[i];
    auto &b = lines_[i + 1];
    auto &c = lines_[i + 2];

    if (a.mnemonic != "push" || trim(a.operands) != "hl") return false;
    if (b.mnemonic != "ld") return false;
    std::string b_dst, b_src;
    if (!split_ld(b.operands, b_dst, b_src)) return false;
    if (b_dst != "hl") return false;
    // Reject if source involves DE (would read clobbered register after ex).
    if (b_src.find('d') != std::string::npos || b_src.find('e') != std::string::npos) return false;
    if (c.mnemonic != "pop" || trim(c.operands) != "de") return false;

    a.mnemonic = "ex";
    a.operands = "de, hl";
    lines_.erase(lines_.begin() + i + 2);
    return true;
}

// push hl; ld l,N(ix); ld h,N+1(ix); pop de  →  ex de,hl; ld l,N(ix); ld h,N+1(ix)
// Fires on every 16-bit load of an IX-local into HL when the old HL is saved for DE.
// This is the dominant pattern in 16- and 32-bit arithmetic.
bool z80_peep::rule_push_hl_ix_pop_de(size_t i) {
    if (i + 3 >= lines_.size()) return false;
    auto &a = lines_[i];
    auto &b = lines_[i + 1];
    auto &c = lines_[i + 2];
    auto &d = lines_[i + 3];

    if (a.mnemonic != "push" || trim(a.operands) != "hl") return false;
    if (d.mnemonic != "pop"  || trim(d.operands) != "de") return false;

    // b: ld l, N(ix)
    if (b.mnemonic != "ld") return false;
    std::string b_dst, b_src;
    if (!split_ld(b.operands, b_dst, b_src)) return false;
    if (b_dst != "l") return false;
    int off_lo;
    if (!parse_ix_ref(b_src, off_lo)) return false;

    // c: ld h, N+1(ix)  (must be consecutive byte)
    if (c.mnemonic != "ld") return false;
    std::string c_dst, c_src;
    if (!split_ld(c.operands, c_dst, c_src)) return false;
    if (c_dst != "h") return false;
    int off_hi;
    if (!parse_ix_ref(c_src, off_hi)) return false;
    if (off_hi != off_lo + 1) return false;

    a.mnemonic = "ex";
    a.operands = "de, hl";
    lines_.erase(lines_.begin() + i + 3);
    return true;
}

// push hl; ld de,#imm; pop hl  →  ld de,#imm
// Used in struct stores: HL holds a pointer and we need DE=value before
// writing through HL.  ld de,#imm does not touch HL, so push/pop is
// redundant.
bool z80_peep::rule_push_hl_de_load(size_t i) {
    if (i + 2 >= lines_.size()) return false;
    auto &a = lines_[i];
    auto &b = lines_[i + 1];
    auto &c = lines_[i + 2];

    if (a.mnemonic != "push" || trim(a.operands) != "hl") return false;
    if (b.mnemonic != "ld") return false;
    std::string b_dst, b_src;
    if (!split_ld(b.operands, b_dst, b_src)) return false;
    if (b_dst != "de" || b_src.empty() || b_src[0] != '#') return false;
    if (c.mnemonic != "pop" || trim(c.operands) != "hl") return false;

    // Remove pop hl first (higher index), then push hl.
    lines_.erase(lines_.begin() + i + 2);
    lines_.erase(lines_.begin() + i);
    return true;
}

// ld a,(ix+N); ld (ix+N),a  →  nothing  (load then store same byte)
bool z80_peep::rule_self_store(size_t i) {
    if (i + 1 >= lines_.size()) return false;
    auto &a = lines_[i];
    auto &b = lines_[i + 1];

    if (a.mnemonic != "ld" || b.mnemonic != "ld") return false;
    std::string a_dst, a_src, b_dst, b_src;
    if (!split_ld(a.operands, a_dst, a_src)) return false;
    if (!split_ld(b.operands, b_dst, b_src)) return false;

    if (a_dst != "a" || b_src != "a") return false;
    int off_a, off_b;
    if (!parse_ix_ref(a_src, off_a)) return false;
    if (!parse_ix_ref(b_dst, off_b)) return false;
    if (off_a != off_b) return false;

    lines_.erase(lines_.begin() + i, lines_.begin() + i + 2);
    return true;
}

// ld hl,#imm; ld hl,X  →  ld hl,X  (first load is dead)
// Only fires when the first load is a pure immediate (no side effects)
// and has no label (not a branch target).
bool z80_peep::rule_dead_hl_load(size_t i) {
    if (i + 1 >= lines_.size()) return false;
    auto &a = lines_[i];
    auto &b = lines_[i + 1];

    if (!a.label.empty()) return false;  // could be a branch target
    if (a.mnemonic != "ld" || b.mnemonic != "ld") return false;

    std::string a_dst, a_src, b_dst, b_src;
    if (!split_ld(a.operands, a_dst, a_src)) return false;
    if (!split_ld(b.operands, b_dst, b_src)) return false;

    if (a_dst != "hl" || a_src.empty() || a_src[0] != '#') return false;
    if (b_dst != "hl") return false;

    lines_.erase(lines_.begin() + i);
    return true;
}

bool z80_peep::rule_dead_hl_pair_load(size_t i) {
    if (i >= lines_.size())
        return false;
    if (!lines_[i].label.empty())
        return false;

    // ld hl,#imm; ld l,X; ld h,Y  ->  ld l,X; ld h,Y
    // The immediate is dead once both bytes are overwritten.  Keep the
    // matcher narrow: only fold pure byte sources that cannot read old HL.
    if (i + 2 < lines_.size() &&
        lines_[i].mnemonic == "ld" &&
        lines_[i + 1].mnemonic == "ld" &&
        lines_[i + 2].mnemonic == "ld" &&
        lines_[i + 1].label.empty() &&
        lines_[i + 2].label.empty()) {
        std::string d0, s0, d1, s1, d2, s2;
        if (split_ld(lines_[i].operands, d0, s0) &&
            split_ld(lines_[i + 1].operands, d1, s1) &&
            split_ld(lines_[i + 2].operands, d2, s2) &&
            trim(d0) == "hl" &&
            (is_immediate_operand(trim(s0)) || is_numeric_literal(trim(s0)))) {
            d1 = trim(d1);
            d2 = trim(d2);
            auto pure_byte_source_without_old_hl = [](const std::string &src) {
                const std::string s = lower_copy(trim(src));
                if (s == "h" || s == "l" || s == "hl" || s == "(hl)")
                    return false;
                int ignored = 0;
                return is_plain_8bit_reg(s) || is_immediate_operand(s) ||
                       is_numeric_literal(s) || parse_ixiy_ref(s, ignored);
            };
            const bool full_hl_overwrite =
                ((d1 == "l" && d2 == "h") || (d1 == "h" && d2 == "l"));
            if (full_hl_overwrite &&
                pure_byte_source_without_old_hl(s1) &&
                pure_byte_source_without_old_hl(s2)) {
                lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i));
                return true;
            }
        }
    }

    size_t span = 0;
    if (lines_[i].mnemonic == "ld") {
        std::string dst;
        std::string src;
        if (split_ld(lines_[i].operands, dst, src) &&
            trim(dst) == "hl" &&
            (is_immediate_operand(trim(src)) || is_numeric_literal(trim(src)))) {
            span = 1;
        }
    }

    if (span == 0) {
        if (i + 1 >= lines_.size() || !lines_[i + 1].label.empty())
            return false;
        if (!overwrites_hl_without_reading_it(lines_, i))
            return false;

        std::string d0, s0, d1, s1;
        if (!split_ld(lines_[i].operands, d0, s0) ||
            !split_ld(lines_[i + 1].operands, d1, s1)) {
            return false;
        }
        s0 = lower_copy(trim(s0));
        s1 = lower_copy(trim(s1));

        int ignored = 0;
        auto side_effect_free_byte_source = [&](const std::string &src) {
            return is_plain_8bit_reg(src) || is_immediate_operand(src) ||
                   is_numeric_literal(src) || parse_ixiy_ref(src, ignored);
        };
        if (!side_effect_free_byte_source(s0) ||
            !side_effect_free_byte_source(s1)) {
            return false;
        }
        span = 2;
    }

    if (!hl_dead_before_read_or_modern_return(lines_, i + span))
        return false;

    bool nearby_explicit_overwrite = false;
    const size_t end = std::min(lines_.size(), i + span + 24);
    for (size_t k = i + span; k < end; ++k) {
        if (lines_[k].mnemonic.empty())
            continue;
        if (lines_[k].mnemonic == "call" || lines_[k].mnemonic == "ret" ||
            lines_[k].mnemonic == "reti" || lines_[k].mnemonic == "retn" ||
            lines_[k].mnemonic == "rst") {
            break;
        }
        if (overwrites_hl_without_reading_it(lines_, k)) {
            nearby_explicit_overwrite = true;
            break;
        }
    }
    if (!nearby_explicit_overwrite)
        return false;

    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + span));
    return true;
}

bool z80_peep::rule_signed_zero_branch_from_high_bit(size_t i) {
    if (i + 3 < lines_.size()) {
        for (size_t j = i; j <= i + 3; ++j) {
            if (!lines_[j].label.empty())
                goto full_load_form;
        }

        std::string dst;
        std::string src;
        std::string cc;
        std::string target;
        if (lines_[i].mnemonic == "ld" &&
            lines_[i + 1].mnemonic == "or" &&
            lines_[i + 2].mnemonic == "sbc" &&
            split_ld(lines_[i].operands, dst, src) &&
            trim(dst) == "de" &&
            immediate_is(src, 0) &&
            is_or_a_self(lines_[i + 1]) &&
            split_ld(lines_[i + 2].operands, dst, src) &&
            trim(dst) == "hl" &&
            trim(src) == "de" &&
            split_conditional_branch_target(lines_[i + 3], cc, target) &&
            (cc == "p" || cc == "m")) {
            const size_t target_idx = find_label_index(lines_, target);
            if (target_idx == lines_.size())
                return false;
            const size_t fallthrough = i + 4;
            if (!flags_from_compare_dead_before_read(lines_, fallthrough) ||
                !flags_from_compare_dead_before_read(lines_, target_idx)) {
                return false;
            }
            if (!pair_value_dead_or_call_or_modern_return_before_read(
                    lines_, fallthrough, "de", 'e', 'd') ||
                !pair_value_dead_or_call_or_modern_return_before_read(
                    lines_, target_idx, "de", 'e', 'd')) {
                return false;
            }

            lines_[i] = asm_line::parse("\tbit\t7, h");
            lines_[i + 1] = asm_line::parse(
                "\t" + lines_[i + 3].mnemonic + "\t" +
                std::string(cc == "p" ? "z, " : "nz, ") + target);
            lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 2),
                         lines_.begin() + static_cast<std::ptrdiff_t>(i + 4));
            return true;
        }
    }

full_load_form:
    if (i + 5 >= lines_.size())
        return false;
    for (size_t j = i; j <= i + 5; ++j) {
        if (!lines_[j].label.empty())
            return false;
    }

    if (lines_[i].mnemonic != "ld" ||
        lines_[i + 1].mnemonic != "ld" ||
        lines_[i + 2].mnemonic != "ld" ||
        lines_[i + 3].mnemonic != "or" ||
        lines_[i + 4].mnemonic != "sbc") {
        return false;
    }

    std::string d0, s0, d1, s1, d2, s2, d4, s4;
    if (!split_ld(lines_[i].operands, d0, s0) ||
        !split_ld(lines_[i + 1].operands, d1, s1) ||
        !split_ld(lines_[i + 2].operands, d2, s2) ||
        !split_ld(lines_[i + 4].operands, d4, s4)) {
        return false;
    }
    d0 = trim(d0); s0 = lower_copy(trim(s0));
    d1 = trim(d1); s1 = lower_copy(trim(s1));
    d2 = trim(d2); s2 = trim(s2);
    d4 = trim(d4); s4 = trim(s4);

    if (d0 != "l" || d1 != "h" || d2 != "de" ||
        !immediate_is(s2, 0) ||
        trim(lines_[i + 3].operands) != "a, a" ||
        d4 != "hl" || s4 != "de") {
        return false;
    }

    int ignored = 0;
    auto safe_load_source = [&](const std::string &src) {
        return is_plain_8bit_reg(src) || is_immediate_operand(src) ||
               is_numeric_literal(src) || parse_ixiy_ref(src, ignored);
    };
    if (!safe_load_source(s0) || !bit_operand_supported(s1))
        return false;

    std::string cc;
    std::string target;
    if (!split_conditional_branch_target(lines_[i + 5], cc, target))
        return false;
    if (cc != "p" && cc != "m")
        return false;

    const size_t target_idx = find_label_index(lines_, target);
    if (target_idx == lines_.size())
        return false;

    const size_t fallthrough = i + 6;
    if (!flags_from_compare_dead_before_read(lines_, fallthrough) ||
        !flags_from_compare_dead_before_read(lines_, target_idx)) {
        return false;
    }
    if (!hl_dead_before_read_or_modern_return(lines_, fallthrough) ||
        !hl_dead_before_read_or_modern_return(lines_, target_idx)) {
        return false;
    }
    if (!pair_value_dead_or_call_or_modern_return_before_read(
            lines_, fallthrough, "de", 'e', 'd') ||
        !pair_value_dead_or_call_or_modern_return_before_read(
            lines_, target_idx, "de", 'e', 'd')) {
        return false;
    }

    lines_[i] = asm_line::parse("\tbit\t7, " + s1);
    lines_[i + 1] = asm_line::parse(
        "\t" + lines_[i + 5].mnemonic + "\t" +
        std::string(cc == "p" ? "z, " : "nz, ") + target);
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 2),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 6));
    return true;
}

bool z80_peep::rule_de_eq_small_const_branch(size_t i) {
    if (i + 3 >= lines_.size())
        return false;

    bool has_ex = false;
    size_t ld_idx = i;
    if (is_ex_de_hl(lines_[i])) {
        has_ex = true;
        ld_idx = i + 1;
    }

    if (ld_idx + 3 >= lines_.size())
        return false;
    for (size_t j = i; j <= ld_idx + 3; ++j) {
        if (!lines_[j].label.empty())
            return false;
    }

    std::string dst;
    std::string src;
    int value = 0;
    if (lines_[ld_idx].mnemonic != "ld" ||
        !split_ld(lines_[ld_idx].operands, dst, src) ||
        trim(dst) != "hl" ||
        !parse_immediate_value(src, value) ||
        value < 0 || value > 0xff ||
        !is_or_a_self(lines_[ld_idx + 1]) ||
        lines_[ld_idx + 2].mnemonic != "sbc" ||
        !split_ld(lines_[ld_idx + 2].operands, dst, src) ||
        trim(dst) != "hl" ||
        trim(src) != "de") {
        return false;
    }

    std::string cc;
    std::string target;
    if (!split_conditional_branch_target(lines_[ld_idx + 3], cc, target))
        return false;
    if (cc != "z" && cc != "nz")
        return false;

    const size_t target_idx = find_label_index(lines_, target);
    if (target_idx == lines_.size())
        return false;

    const size_t fallthrough = ld_idx + 4;
    const bool a_dead_fallthrough =
        a_overwritten_or_call_before_read(lines_, fallthrough) ||
        a_overwritten_or_de_return_before_read(lines_, fallthrough);
    const bool a_dead_target =
        a_overwritten_or_call_before_read(lines_, target_idx) ||
        a_overwritten_or_de_return_before_read(lines_, target_idx);
    if (!a_dead_fallthrough ||
        !a_dead_target ||
        !hl_dead_before_read_or_modern_return(lines_, fallthrough) ||
        !hl_dead_before_read_or_modern_return(lines_, target_idx) ||
        !flags_from_compare_dead_before_read(lines_, fallthrough) ||
        !flags_from_compare_dead_before_read(lines_, target_idx)) {
        return false;
    }

    if (has_ex) {
        lines_[i] = asm_line::parse("\tex\tde, hl");
        lines_[i + 1] = asm_line::parse("\tld\ta, e");
        lines_[i + 2] = asm_line::parse("\tsub\t#" + std::to_string(value));
        lines_[i + 3] = asm_line::parse("\tor\td");
        lines_[i + 4] = lines_[ld_idx + 3];
        lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 5),
                     lines_.begin() + static_cast<std::ptrdiff_t>(ld_idx + 4));
    } else {
        lines_[i] = asm_line::parse("\tld\ta, e");
        lines_[i + 1] = asm_line::parse("\tsub\t#" + std::to_string(value));
        lines_[i + 2] = asm_line::parse("\tor\td");
        lines_[i + 3] = lines_[ld_idx + 3];
        lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 4),
                     lines_.begin() + static_cast<std::ptrdiff_t>(ld_idx + 4));
    }
    return true;
}

bool z80_peep::rule_de_word_ne_branch_split(size_t i) {
    if (i + 4 >= lines_.size())
        return false;
    for (size_t j = i; j <= i + 4; ++j) {
        if (!lines_[j].label.empty())
            return false;
    }

    if (lines_[i].mnemonic != "ld" ||
        lines_[i + 1].mnemonic != "ld" ||
        !is_or_a_self(lines_[i + 2]) ||
        lines_[i + 3].mnemonic != "sbc" ||
        lines_[i + 4].mnemonic != "jr") {
        return false;
    }

    std::string d0;
    std::string lo_src;
    std::string d1;
    std::string hi_src;
    std::string d3;
    std::string s3;
    if (!split_ld(lines_[i].operands, d0, lo_src) ||
        !split_ld(lines_[i + 1].operands, d1, hi_src) ||
        !split_ld(lines_[i + 3].operands, d3, s3) ||
        trim(d0) != "l" ||
        trim(d1) != "h" ||
        trim(d3) != "hl" ||
        trim(s3) != "de") {
        return false;
    }

    lo_src = lower_copy(trim(lo_src));
    hi_src = lower_copy(trim(hi_src));
    auto safe_byte_source = [](const std::string &src) {
        int ignored = 0;
        if (src == "(hl)")
            return false;
        if (operand_mentions_pair_or_bytes(src, "hl", 'l', 'h') ||
            operand_mentions_pair_or_bytes(src, "de", 'e', 'd')) {
            return false;
        }
        return is_plain_8bit_reg(src) || is_immediate_operand(src) ||
               is_numeric_literal(src) || parse_ixiy_ref(src, ignored);
    };
    if (!safe_byte_source(lo_src) || !safe_byte_source(hi_src))
        return false;

    std::string cc;
    std::string target;
    if (!split_conditional_branch_target(lines_[i + 4], cc, target) ||
        cc != "nz") {
        return false;
    }

    const size_t target_idx = find_label_index(lines_, target);
    if (target_idx == lines_.size())
        return false;
    const size_t fallthrough = i + 5;
    if (!a_overwritten_or_de_return_before_read(lines_, fallthrough) ||
        !a_overwritten_or_de_return_before_read(lines_, target_idx) ||
        !hl_dead_before_read_or_modern_return(lines_, fallthrough) ||
        !hl_dead_before_read_or_modern_return(lines_, target_idx) ||
        !flags_from_compare_dead_before_read(lines_, fallthrough) ||
        !flags_from_compare_dead_before_read(lines_, target_idx)) {
        return false;
    }

    lines_[i] = asm_line::parse("\tld\ta, " + lo_src);
    lines_[i + 1] = asm_line::parse("\tcp\te");
    lines_[i + 2] = lines_[i + 4];
    lines_[i + 3] = asm_line::parse("\tld\ta, " + hi_src);
    lines_[i + 4] = asm_line::parse("\tcp\td");
    lines_.insert(lines_.begin() + static_cast<std::ptrdiff_t>(i + 5),
                  lines_[i + 2]);
    return true;
}

bool z80_peep::rule_signed_byte_zero_branch(size_t i) {
    auto rewrite_branch = [&](size_t xor_idx,
                              const std::string &bit_operand,
                              size_t branch_idx) {
        int xor_value = 0;
        int cp_value = 0;
        if (lines_[xor_idx].mnemonic != "xor" ||
            !parse_immediate_value(lines_[xor_idx].operands, xor_value) ||
            u8_value(xor_value) != 0x80 ||
            xor_idx + 1 >= lines_.size() ||
            lines_[xor_idx + 1].mnemonic != "cp" ||
            !parse_immediate_value(lines_[xor_idx + 1].operands, cp_value) ||
            u8_value(cp_value) != 0x80) {
            return false;
        }

        std::string cc;
        std::string target;
        if (!split_conditional_branch_target(lines_[branch_idx], cc, target))
            return false;
        if (cc != "c" && cc != "nc" && cc != "m" && cc != "p")
            return false;

        const size_t target_idx = find_label_index(lines_, target);
        if (target_idx == lines_.size())
            return false;

        const size_t fallthrough = branch_idx + 1;
        if (!a_overwritten_before_read(lines_, fallthrough) ||
            !a_overwritten_before_read(lines_, target_idx) ||
            !flags_overwritten_before_read_or_escape(lines_, fallthrough) ||
            !flags_overwritten_before_read_or_escape(lines_, target_idx)) {
            return false;
        }

        const bool branch_on_negative = (cc == "c" || cc == "m");
        lines_[xor_idx] = asm_line::parse("\tbit\t7, " + bit_operand);
        lines_[xor_idx + 1] = asm_line::parse(
            "\t" + lines_[branch_idx].mnemonic + "\t" +
            std::string(branch_on_negative ? "nz, " : "z, ") + target);
        lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(xor_idx + 2),
                     lines_.begin() + static_cast<std::ptrdiff_t>(branch_idx + 1));
        return true;
    };

    if (i + 2 < lines_.size()) {
        bool labels = false;
        for (size_t j = i; j <= i + 2; ++j)
            labels = labels || !lines_[j].label.empty();
        if (!labels && rewrite_branch(i, "a", i + 2))
            return true;
    }

    if (i + 3 >= lines_.size())
        return false;
    for (size_t j = i; j <= i + 3; ++j) {
        if (!lines_[j].label.empty())
            return false;
    }

    std::string dst;
    std::string src;
    if (lines_[i].mnemonic != "ld" ||
        !split_ld(lines_[i].operands, dst, src) ||
        trim(dst) != "a") {
        return false;
    }
    src = lower_copy(trim(src));
    if (!bit_operand_supported(src))
        return false;

    return rewrite_branch(i + 1, src, i + 3);
}

bool z80_peep::rule_page_aligned_word_bound_branch(size_t i) {
    if (i + 5 >= lines_.size())
        return false;
    for (size_t j = i; j <= i + 5; ++j) {
        if (!lines_[j].label.empty())
            return false;
    }

    if (lines_[i].mnemonic != "ld" ||
        lines_[i + 1].mnemonic != "ld" ||
        lines_[i + 2].mnemonic != "ld" ||
        lines_[i + 3].mnemonic != "or" ||
        lines_[i + 4].mnemonic != "sbc") {
        return false;
    }

    std::string d0, s0, d1, s1, d2, s2, d4, s4;
    if (!split_ld(lines_[i].operands, d0, s0) ||
        !split_ld(lines_[i + 1].operands, d1, s1) ||
        !split_ld(lines_[i + 2].operands, d2, s2) ||
        !split_ld(lines_[i + 4].operands, d4, s4)) {
        return false;
    }

    d0 = trim(d0);
    d1 = trim(d1);
    d2 = trim(d2);
    d4 = trim(d4);
    s0 = lower_copy(trim(s0));
    s1 = lower_copy(trim(s1));
    s2 = trim(s2);
    s4 = trim(s4);

    if (d0 != "l" || d1 != "h" || d2 != "de" ||
        !is_or_a_self(lines_[i + 3]) ||
        d4 != "hl" || s4 != "de") {
        return false;
    }

    int bound = 0;
    if (!parse_immediate_value(s2, bound))
        return false;
    if (bound <= 0 || bound > 0xff00 || (bound & 0xff) != 0)
        return false;
    const int page = (bound >> 8) & 0xff;

    int ignored = 0;
    auto safe_load_source = [&](const std::string &src) {
        return is_plain_8bit_reg(src) || is_immediate_operand(src) ||
               is_numeric_literal(src) || parse_ixiy_ref(src, ignored);
    };
    if (!safe_load_source(s0) || !safe_load_source(s1))
        return false;

    std::string cc;
    std::string target;
    if (!split_conditional_branch_target(lines_[i + 5], cc, target))
        return false;
    if (cc != "c" && cc != "nc" && cc != "p" && cc != "m")
        return false;

    const size_t target_idx = find_label_index(lines_, target);
    if (target_idx == lines_.size())
        return false;

    const size_t fallthrough = i + 6;
    if (!flags_from_compare_dead_before_read(lines_, fallthrough) ||
        !flags_from_compare_dead_before_read(lines_, target_idx)) {
        return false;
    }
    if (!hl_dead_before_read_or_modern_return(lines_, fallthrough) ||
        !hl_dead_before_read_or_modern_return(lines_, target_idx)) {
        return false;
    }
    if (!pair_value_dead_or_call_or_modern_return_before_read(
            lines_, fallthrough, "de", 'e', 'd') ||
        !pair_value_dead_or_call_or_modern_return_before_read(
            lines_, target_idx, "de", 'e', 'd')) {
        return false;
    }
    if (s1 != "a" &&
        (!a_overwritten_or_de_return_before_read(lines_, fallthrough) ||
         !a_overwritten_or_de_return_before_read(lines_, target_idx))) {
        return false;
    }

    if (s1 == "a") {
        lines_[i] = asm_line::parse("\tcp\t#" + std::to_string(page));
        lines_[i + 1] = lines_[i + 5];
        lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 2),
                     lines_.begin() + static_cast<std::ptrdiff_t>(i + 6));
    } else {
        lines_[i] = asm_line::parse("\tld\ta, " + s1);
        lines_[i + 1] = asm_line::parse("\tcp\t#" + std::to_string(page));
        lines_[i + 2] = lines_[i + 5];
        lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 3),
                     lines_.begin() + static_cast<std::ptrdiff_t>(i + 6));
    }
    return true;
}

bool z80_peep::rule_hl_high_byte_mask_branch(size_t i) {
    if (i + 7 >= lines_.size())
        return false;

    auto no_labels = [&](size_t end_inclusive) {
        if (end_inclusive >= lines_.size())
            return false;
        for (size_t k = i; k <= end_inclusive; ++k) {
            if (!lines_[k].label.empty())
                return false;
        }
        return true;
    };

    if (lines_[i].mnemonic != "ld" ||
        lines_[i + 1].mnemonic != "and" ||
        lines_[i + 2].mnemonic != "ld" ||
        lines_[i + 3].mnemonic != "ld" ||
        lines_[i + 4].mnemonic != "and" ||
        lines_[i + 5].mnemonic != "ld") {
        return false;
    }

    std::string dst;
    std::string src;
    if (!split_ld(lines_[i].operands, dst, src) ||
        trim(dst) != "a" || trim(src) != "l") {
        return false;
    }
    if (!immediate_is(lines_[i + 1].operands, 0))
        return false;
    if (!split_ld(lines_[i + 2].operands, dst, src) ||
        trim(dst) != "l" || trim(src) != "a") {
        return false;
    }
    if (!split_ld(lines_[i + 3].operands, dst, src) ||
        trim(dst) != "a" || trim(src) != "h") {
        return false;
    }

    int mask = 0;
    if (!parse_immediate_value(lines_[i + 4].operands, mask))
        return false;
    const int bit = single_u8_bit_index(mask);
    if (bit < 0)
        return false;

    if (!split_ld(lines_[i + 5].operands, dst, src) ||
        trim(dst) != "h" || trim(src) != "a") {
        return false;
    }

    size_t cursor = i + 6;
    bool copied_to_bc = false;
    if (cursor + 1 < lines_.size() &&
        lines_[cursor].mnemonic == "ld" &&
        lines_[cursor + 1].mnemonic == "ld") {
        std::string dst0;
        std::string src0;
        std::string dst1;
        std::string src1;
        if (split_ld(lines_[cursor].operands, dst0, src0) &&
            split_ld(lines_[cursor + 1].operands, dst1, src1) &&
            trim(dst0) == "b" && trim(src0) == "h" &&
            trim(dst1) == "c" && trim(src1) == "l") {
            copied_to_bc = true;
            cursor += 2;
        }
    }

    if (cursor + 2 >= lines_.size() || !no_labels(cursor + 2))
        return false;
    if (lines_[cursor].mnemonic != "ld" ||
        !split_ld(lines_[cursor].operands, dst, src) ||
        trim(dst) != "a" || trim(src) != "h") {
        return false;
    }
    if (lines_[cursor + 1].mnemonic != "or")
        return false;
    const std::string or_ops = trim(lines_[cursor + 1].operands);
    if (or_ops != "a, l" && or_ops != "a,l" && or_ops != "l")
        return false;

    std::string cc;
    std::string target;
    if (!split_conditional_branch_target(lines_[cursor + 2], cc, target))
        return false;
    if (cc != "z" && cc != "nz")
        return false;

    const size_t target_idx = find_label_index(lines_, target);
    if (target_idx == lines_.size())
        return false;
    const size_t fallthrough = cursor + 3;
    if (!a_overwritten_before_read(lines_, fallthrough) ||
        !a_overwritten_before_read(lines_, target_idx) ||
        !path_overwrites_hl_before_read(lines_, fallthrough) ||
        !path_overwrites_hl_before_read(lines_, target_idx) ||
        !flags_overwritten_before_read_or_escape(lines_, fallthrough) ||
        !flags_overwritten_before_read_or_escape(lines_, target_idx)) {
        return false;
    }
    if (copied_to_bc &&
        (!path_overwrites_bc_before_read_or_call(lines_, fallthrough) ||
         !path_overwrites_bc_before_read_or_call(lines_, target_idx))) {
        return false;
    }

    lines_[i] = asm_line::parse("\tbit\t" + std::to_string(bit) + ", h");
    lines_[i + 1] = lines_[cursor + 2];
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 2),
                 lines_.begin() + static_cast<std::ptrdiff_t>(cursor + 3));
    return true;
}

bool z80_peep::rule_word_mask_zero_high_branch(size_t i) {
    if (i + 24 >= lines_.size())
        return false;
    for (size_t j = i; j <= i + 24; ++j) {
        if (!lines_[j].label.empty())
            return false;
    }

    auto and_a_reg = [](const asm_line &line, const std::string &reg) {
        if (line.mnemonic != "and")
            return false;
        const std::string ops = trim(line.operands);
        return ops == reg || ops == "a, " + reg || ops == "a," + reg;
    };
    auto or_a_operand = [](const asm_line &line, const std::string &operand) {
        if (line.mnemonic != "or")
            return false;
        const std::string ops = trim(line.operands);
        return ops == operand || ops == "a, " + operand ||
               ops == "a," + operand;
    };

    std::string dst;
    std::string src;
    if (lines_[i].mnemonic != "ld" ||
        !split_ld(lines_[i].operands, dst, src) ||
        trim(dst) != "l") {
        return false;
    }
    const std::string lo_src = lower_copy(trim(src));

    if (lines_[i + 1].mnemonic != "ld" ||
        !split_ld(lines_[i + 1].operands, dst, src) ||
        trim(dst) != "h") {
        return false;
    }
    const std::string hi_src = lower_copy(trim(src));

    if (lines_[i + 2].mnemonic != "ex" ||
        trim(lines_[i + 2].operands) != "de, hl") {
        return false;
    }

    int mask = 0;
    if (lines_[i + 3].mnemonic != "ld" ||
        !split_ld(lines_[i + 3].operands, dst, src) ||
        trim(dst) != "hl" ||
        !parse_immediate_value(src, mask)) {
        return false;
    }
    const int bit = single_u16_bit_index(mask);
    if (bit < 0)
        return false;
    const std::string bit_src = bit < 8 ? lo_src : hi_src;
    if (!bit_operand_supported(bit_src))
        return false;

    if (lines_[i + 4].mnemonic != "ld" ||
        !split_ld(lines_[i + 4].operands, dst, src) ||
        trim(dst) != "a" || trim(src) != "l" ||
        !and_a_reg(lines_[i + 5], "e") ||
        lines_[i + 6].mnemonic != "ld" ||
        !split_ld(lines_[i + 6].operands, dst, src) ||
        trim(dst) != "l" || trim(src) != "a" ||
        lines_[i + 7].mnemonic != "ld" ||
        !split_ld(lines_[i + 7].operands, dst, src) ||
        trim(dst) != "a" || trim(src) != "h" ||
        !and_a_reg(lines_[i + 8], "d") ||
        lines_[i + 9].mnemonic != "ld" ||
        !split_ld(lines_[i + 9].operands, dst, src) ||
        trim(dst) != "h" || trim(src) != "a") {
        return false;
    }

    int temp_lo = 0;
    int temp_hi = 0;
    if (lines_[i + 10].mnemonic != "ld" ||
        !split_ld(lines_[i + 10].operands, dst, src) ||
        !parse_ix_ref(trim(dst), temp_lo) ||
        trim(src) != "l" ||
        lines_[i + 11].mnemonic != "ld" ||
        !split_ld(lines_[i + 11].operands, dst, src) ||
        !parse_ix_ref(trim(dst), temp_hi) ||
        temp_hi != temp_lo + 1 ||
        trim(src) != "h") {
        return false;
    }

    if (lines_[i + 12].mnemonic != "ld" ||
        !split_ld(lines_[i + 12].operands, dst, src) ||
        trim(dst) != "l" ||
        lines_[i + 13].mnemonic != "ld" ||
        !split_ld(lines_[i + 13].operands, dst, src) ||
        trim(dst) != "h" ||
        lines_[i + 14].mnemonic != "ex" ||
        trim(lines_[i + 14].operands) != "de, hl") {
        return false;
    }

    if (lines_[i + 15].mnemonic != "ld" ||
        !split_ld(lines_[i + 15].operands, dst, src) ||
        trim(dst) != "hl" ||
        !immediate_is(src, 0) ||
        lines_[i + 16].mnemonic != "ld" ||
        !split_ld(lines_[i + 16].operands, dst, src) ||
        trim(dst) != "a" || trim(src) != "l" ||
        !and_a_reg(lines_[i + 17], "e") ||
        lines_[i + 18].mnemonic != "ld" ||
        !split_ld(lines_[i + 18].operands, dst, src) ||
        trim(dst) != "l" || trim(src) != "a" ||
        lines_[i + 19].mnemonic != "ld" ||
        !split_ld(lines_[i + 19].operands, dst, src) ||
        trim(dst) != "a" || trim(src) != "h" ||
        !and_a_reg(lines_[i + 20], "d") ||
        lines_[i + 21].mnemonic != "ld" ||
        !split_ld(lines_[i + 21].operands, dst, src) ||
        trim(dst) != "h" || trim(src) != "a") {
        return false;
    }

    int reload_hi = 0;
    if (lines_[i + 22].mnemonic != "ld" ||
        !split_ld(lines_[i + 22].operands, dst, src) ||
        trim(dst) != "a" ||
        !parse_ix_ref(trim(src), reload_hi) ||
        reload_hi != temp_hi) {
        return false;
    }

    const std::string temp_lo_operand =
        std::to_string(temp_lo) + "(ix)";
    if (!or_a_operand(lines_[i + 23], temp_lo_operand))
        return false;

    std::string cc;
    std::string target;
    if (!split_conditional_branch_target(lines_[i + 24], cc, target))
        return false;
    if (cc != "z" && cc != "nz")
        return false;

    const size_t target_idx = find_label_index(lines_, target);
    if (target_idx == lines_.size() || target_idx < i + 25)
        return false;

    int locals = 0;
    int temp_frame = 0;
    size_t prologue_index = 0;
    if (!current_function_frame(lines_, i, locals, temp_frame, prologue_index))
        return false;
    if (!ix_offset_in_temp_frame(temp_lo, locals, temp_frame) ||
        !ix_offset_in_temp_frame(temp_hi, locals, temp_frame)) {
        return false;
    }
    const size_t fn_end = function_end_after_prologue(lines_, prologue_index);
    if (ix_slot_read_in_function(lines_, prologue_index, fn_end,
                                 temp_lo, i + 23) ||
        ix_slot_read_in_function(lines_, prologue_index, fn_end,
                                 temp_hi, i + 22)) {
        return false;
    }

    const size_t fallthrough = i + 25;
    if (!a_overwritten_before_read(lines_, fallthrough) ||
        !a_overwritten_before_read(lines_, target_idx) ||
        !path_overwrites_hl_before_read(lines_, fallthrough) ||
        !path_overwrites_hl_before_read(lines_, target_idx) ||
        !path_overwrites_pair_before_read(lines_, fallthrough, "de", 'e', 'd') ||
        !path_overwrites_pair_before_read(lines_, target_idx, "de", 'e', 'd') ||
        !flags_overwritten_before_read_or_escape(lines_, fallthrough) ||
        !flags_overwritten_before_read_or_escape(lines_, target_idx)) {
        return false;
    }

    lines_[i] = asm_line::parse("\tbit\t" +
                                std::to_string(bit & 7) + ", " + bit_src);
    lines_[i + 1] = lines_[i + 24];
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 2),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 25));
    return true;
}

bool z80_peep::rule_bit7_shift_xor_diamond(size_t i) {
    if (i >= lines_.size())
        return false;
    if (!lines_[i].label.empty())
        return false;

    auto is_add_a_a = [](const asm_line &line) {
        if (line.mnemonic != "add")
            return false;
        const std::string ops = trim(line.operands);
        return ops == "a, a" || ops == "a,a";
    };

    // Same register recurrence when both arms end by jumping to a common
    // continuation rather than meeting at a local join label.
    if (i + 13 < lines_.size()) {
        int bit = -1;
        int xor_value = 0;
        std::string reg;
        std::string cc;
        std::string else_label;
        std::string true_target;
        std::string false_target;
        std::string dst;
        std::string src;
        auto is_ld = [&](size_t index, const std::string &want_dst,
                         const std::string &want_src) {
            if (lines_[index].mnemonic != "ld" ||
                !split_ld(lines_[index].operands, dst, src)) {
                return false;
            }
            return lower_copy(trim(dst)) == want_dst &&
                   lower_copy(trim(src)) == want_src;
        };

        const bool matched =
            lines_[i].mnemonic == "bit" &&
            parse_bit_reg_operands(lines_[i].operands, bit, reg) &&
            bit == 7 && is_plain_8bit_reg(reg) && reg != "a" &&
            split_conditional_branch_target(lines_[i + 1], cc, else_label) &&
            cc == "z" &&
            !lines_[i + 2].label.empty() &&
            lines_[i + 2].mnemonic.empty() &&
            is_ld(i + 3, "a", reg) &&
            is_add_a_a(lines_[i + 4]) &&
            lines_[i + 5].mnemonic == "xor" &&
            parse_immediate_value(lines_[i + 5].operands, xor_value) &&
            is_ld(i + 6, reg, "a") &&
            parse_unconditional_jump(lines_[i + 7], true_target) &&
            lines_[i + 8].label == else_label &&
            lines_[i + 8].mnemonic.empty() &&
            is_ld(i + 9, "a", reg) &&
            is_add_a_a(lines_[i + 10]) &&
            is_ld(i + 11, reg, "a") &&
            !lines_[i + 12].label.empty() &&
            lines_[i + 12].mnemonic.empty() &&
            parse_unconditional_jump(lines_[i + 13], false_target) &&
            true_target == false_target &&
            lines_[i + 2].label != else_label &&
            !label_has_other_control_references(
                lines_, lines_[i + 2].label, lines_.size()) &&
            !label_has_other_control_references(lines_, else_label, i + 1) &&
            !label_has_other_control_references(
                lines_, lines_[i + 12].label, lines_.size());

        if (matched) {
            const std::string xor_operand = trim(lines_[i + 5].operands);
            const std::string final_comment =
                !lines_[i + 11].comment.empty()
                    ? lines_[i + 11].comment
                    : lines_[i + 6].comment;
            lines_[i] = asm_line::parse("\tld\ta, " + reg);
            lines_[i + 1] = asm_line::parse("\tadd\ta, a");
            lines_[i + 2] = asm_line::parse("\tjr\tnc, " + else_label);
            lines_[i + 3] = asm_line::parse("\txor\t" + xor_operand);
            lines_[i + 4] = lines_[i + 8];
            lines_[i + 5] = asm_line::parse("\tld\t" + reg + ", a");
            lines_[i + 5].comment = final_comment;
            lines_[i + 6] = lines_[i + 13];
            lines_.erase(
                lines_.begin() + static_cast<std::ptrdiff_t>(i + 7),
                lines_.begin() + static_cast<std::ptrdiff_t>(i + 14));
            return true;
        }
    }

    // Register-carried form of the same truncating byte recurrence:
    //
    //   bit 7,r; jr z,else
    //   ld a,r; add a,a; xor #K; ld r,a; jr join
    // else: ld a,r; add a,a; ld r,a
    // join:
    //
    // ADD exposes the tested bit as carry, so the duplicated shift can be
    // shared without changing the final A, r, or flags on either path.
    if (i + 12 < lines_.size()) {
        int bit = -1;
        int xor_value = 0;
        std::string reg;
        std::string cc;
        std::string else_label;
        std::string join_label;
        std::string dst;
        std::string src;
        auto is_ld = [&](size_t index, const std::string &want_dst,
                         const std::string &want_src) {
            if (lines_[index].mnemonic != "ld" ||
                !split_ld(lines_[index].operands, dst, src)) {
                return false;
            }
            return lower_copy(trim(dst)) == want_dst &&
                   lower_copy(trim(src)) == want_src;
        };

        const bool matched =
            lines_[i].mnemonic == "bit" &&
            parse_bit_reg_operands(lines_[i].operands, bit, reg) &&
            bit == 7 && is_plain_8bit_reg(reg) && reg != "a" &&
            split_conditional_branch_target(lines_[i + 1], cc, else_label) &&
            cc == "z" &&
            !lines_[i + 2].label.empty() &&
            lines_[i + 2].mnemonic.empty() &&
            is_ld(i + 3, "a", reg) &&
            is_add_a_a(lines_[i + 4]) &&
            lines_[i + 5].mnemonic == "xor" &&
            parse_immediate_value(lines_[i + 5].operands, xor_value) &&
            is_ld(i + 6, reg, "a") &&
            parse_unconditional_jump(lines_[i + 7], join_label) &&
            lines_[i + 8].label == else_label &&
            lines_[i + 8].mnemonic.empty() &&
            is_ld(i + 9, "a", reg) &&
            is_add_a_a(lines_[i + 10]) &&
            is_ld(i + 11, reg, "a") &&
            lines_[i + 12].label == join_label &&
            lines_[i + 12].mnemonic.empty() &&
            lines_[i + 2].label != else_label &&
            !label_has_other_control_references(
                lines_, lines_[i + 2].label, lines_.size()) &&
            !label_has_other_control_references(lines_, else_label, i + 1) &&
            !label_has_other_control_references(lines_, join_label, i + 7);

        if (matched) {
            const std::string xor_operand = trim(lines_[i + 5].operands);
            const std::string final_comment =
                !lines_[i + 11].comment.empty()
                    ? lines_[i + 11].comment
                    : lines_[i + 6].comment;
            lines_[i] = asm_line::parse("\tld\ta, " + reg);
            lines_[i + 1] = asm_line::parse("\tadd\ta, a");
            lines_[i + 2] = asm_line::parse("\tjr\tnc, " + join_label);
            lines_[i + 3] = asm_line::parse("\txor\t" + xor_operand);
            lines_[i + 4] = lines_[i + 12];
            lines_[i + 5] = asm_line::parse("\tld\t" + reg + ", a");
            lines_[i + 5].comment = final_comment;
            lines_.erase(
                lines_.begin() + static_cast<std::ptrdiff_t>(i + 6),
                lines_.begin() + static_cast<std::ptrdiff_t>(i + 13));
            return true;
        }
    }

    // Coalescing can write both arms directly back to the same byte slot:
    //
    //   ld slot,a; bit 7,a; jr z,else
    //   ld a,slot; add a,a; xor #K; ld slot,a; jr join
    // else: ld a,slot; add a,a; ld slot,a
    // join:
    //
    // ADD exposes the old top bit in carry, so the whole diamond can keep
    // the value in A. A later generic rule removes the now-redundant leading
    // store when another update or the final store follows.
    if (i + 13 < lines_.size()) {
        std::string dst;
        std::string src;
        if (lines_[i].mnemonic == "ld" &&
            split_ld(lines_[i].operands, dst, src) &&
            trim(src) == "a") {
            const std::string slot = lower_copy(trim(dst));
            int slot_off = 0;
            int bit = -1;
            std::string bit_reg;
            std::string cc;
            std::string else_label;
            std::string join_label;
            std::string true_load_dst;
            std::string true_load_src;
            std::string true_store_dst;
            std::string true_store_src;
            std::string else_load_dst;
            std::string else_load_src;
            std::string else_store_dst;
            std::string else_store_src;
            int xor_value = 0;

            const bool slot_supported =
                parse_ix_ref(slot, slot_off) ||
                (is_plain_8bit_reg(slot) && slot != "a");
            const bool matched =
                slot_supported &&
                lines_[i + 1].mnemonic == "bit" &&
                parse_bit_reg_operands(lines_[i + 1].operands,
                                       bit, bit_reg) &&
                bit == 7 && bit_reg == "a" &&
                split_conditional_branch_target(lines_[i + 2], cc,
                                                else_label) &&
                cc == "z" &&
                !lines_[i + 3].label.empty() &&
                lines_[i + 3].mnemonic.empty() &&
                lines_[i + 4].mnemonic == "ld" &&
                split_ld(lines_[i + 4].operands,
                         true_load_dst, true_load_src) &&
                trim(true_load_dst) == "a" &&
                lower_copy(trim(true_load_src)) == slot &&
                is_add_a_a(lines_[i + 5]) &&
                lines_[i + 6].mnemonic == "xor" &&
                parse_immediate_value(lines_[i + 6].operands, xor_value) &&
                lines_[i + 7].mnemonic == "ld" &&
                split_ld(lines_[i + 7].operands,
                         true_store_dst, true_store_src) &&
                lower_copy(trim(true_store_dst)) == slot &&
                trim(true_store_src) == "a" &&
                parse_unconditional_jump(lines_[i + 8], join_label) &&
                lines_[i + 9].label == else_label &&
                lines_[i + 9].mnemonic.empty() &&
                lines_[i + 10].mnemonic == "ld" &&
                split_ld(lines_[i + 10].operands,
                         else_load_dst, else_load_src) &&
                trim(else_load_dst) == "a" &&
                lower_copy(trim(else_load_src)) == slot &&
                is_add_a_a(lines_[i + 11]) &&
                lines_[i + 12].mnemonic == "ld" &&
                split_ld(lines_[i + 12].operands,
                         else_store_dst, else_store_src) &&
                lower_copy(trim(else_store_dst)) == slot &&
                trim(else_store_src) == "a" &&
                lines_[i + 13].label == join_label &&
                lines_[i + 13].mnemonic.empty() &&
                lines_[i + 3].label != else_label &&
                !label_has_other_control_references(lines_,
                                                    lines_[i + 3].label,
                                                    lines_.size()) &&
                !label_has_other_control_references(lines_, else_label,
                                                    i + 2) &&
                !label_has_other_control_references(lines_, join_label,
                                                    i + 8);

            if (matched) {
                const std::string xor_operand = trim(lines_[i + 6].operands);
                const std::string final_comment =
                    !lines_[i + 12].comment.empty()
                        ? lines_[i + 12].comment
                        : lines_[i + 7].comment;
                lines_[i + 1] = asm_line::parse("\tadd\ta, a");
                lines_[i + 2] =
                    asm_line::parse("\tjr\tnc, " + join_label);
                lines_[i + 3] =
                    asm_line::parse("\txor\t" + xor_operand);
                lines_[i + 4] = lines_[i + 13];
                lines_[i + 5] =
                    asm_line::parse("\tld\t" + slot + ", a");
                lines_[i + 5].comment = final_comment;
                lines_.erase(
                    lines_.begin() + static_cast<std::ptrdiff_t>(i + 6),
                    lines_.begin() + static_cast<std::ptrdiff_t>(i + 14));
                return true;
            }
        }
    }

    if (i + 15 >= lines_.size())
        return false;

    std::string dst;
    std::string src;
    if (lines_[i].mnemonic != "ld" ||
        !split_ld(lines_[i].operands, dst, src) ||
        trim(src) != "a") {
        return false;
    }
    const std::string slot = lower_copy(trim(dst));
    int slot_off = 0;
    if (!parse_ix_ref(slot, slot_off))
        return false;

    int bit = -1;
    std::string bit_reg;
    if (lines_[i + 1].mnemonic != "bit" ||
        !parse_bit_reg_operands(lines_[i + 1].operands, bit, bit_reg) ||
        bit != 7 || bit_reg != "a") {
        return false;
    }

    std::string cc;
    std::string else_label;
    if (!split_conditional_branch_target(lines_[i + 2], cc, else_label) ||
        cc != "z") {
        return false;
    }

    if (lines_[i + 3].label.empty() ||
        !lines_[i + 3].mnemonic.empty() ||
        label_has_other_control_references(lines_, lines_[i + 3].label,
                                           lines_.size())) {
        return false;
    }

    if (lines_[i + 4].mnemonic != "ld" ||
        !split_ld(lines_[i + 4].operands, dst, src) ||
        trim(dst) != "a" || lower_copy(trim(src)) != slot ||
        !is_add_a_a(lines_[i + 5]) ||
        lines_[i + 6].mnemonic != "xor") {
        return false;
    }
    const std::string xor_operand = trim(lines_[i + 6].operands);
    int ignored_xor = 0;
    if (!parse_immediate_value(xor_operand, ignored_xor))
        return false;

    int temp_off = 0;
    if (lines_[i + 7].mnemonic != "ld" ||
        !split_ld(lines_[i + 7].operands, dst, src) ||
        !parse_ix_ref(trim(dst), temp_off) ||
        trim(src) != "a") {
        return false;
    }

    std::string join_label;
    if (!parse_unconditional_jump(lines_[i + 8], join_label))
        return false;
    if (lines_[i + 9].label != else_label ||
        !lines_[i + 9].mnemonic.empty()) {
        return false;
    }
    if (label_has_other_control_references(lines_, else_label, i + 2))
        return false;

    if (lines_[i + 10].mnemonic != "ld" ||
        !split_ld(lines_[i + 10].operands, dst, src) ||
        trim(dst) != "a" || lower_copy(trim(src)) != slot ||
        !is_add_a_a(lines_[i + 11])) {
        return false;
    }

    int else_temp_off = 0;
    if (lines_[i + 12].mnemonic != "ld" ||
        !split_ld(lines_[i + 12].operands, dst, src) ||
        !parse_ix_ref(trim(dst), else_temp_off) ||
        else_temp_off != temp_off ||
        trim(src) != "a") {
        return false;
    }

    if (lines_[i + 13].label != join_label ||
        !lines_[i + 13].mnemonic.empty()) {
        return false;
    }
    if (label_has_other_control_references(lines_, join_label, i + 8))
        return false;

    int reload_temp_off = 0;
    if (lines_[i + 14].mnemonic != "ld" ||
        !split_ld(lines_[i + 14].operands, dst, src) ||
        trim(dst) != "a" ||
        !parse_ix_ref(trim(src), reload_temp_off) ||
        reload_temp_off != temp_off) {
        return false;
    }

    if (lines_[i + 15].mnemonic != "ld" ||
        !split_ld(lines_[i + 15].operands, dst, src) ||
        lower_copy(trim(dst)) != slot ||
        trim(src) != "a") {
        return false;
    }

    int locals = 0;
    int temp_frame = 0;
    size_t prologue_index = 0;
    if (!current_function_frame(lines_, i, locals, temp_frame, prologue_index))
        return false;
    if (!ix_offset_in_temp_frame(temp_off, locals, temp_frame))
        return false;
    const size_t fn_end = function_end_after_prologue(lines_, prologue_index);
    if (ix_slot_read_in_function(lines_, prologue_index, fn_end,
                                 temp_off, i + 14)) {
        return false;
    }

    const std::string final_store_comment =
        !lines_[i + 15].comment.empty() ? lines_[i + 15].comment
                                        : lines_[i].comment;

    lines_[i] = asm_line::parse("\tadd\ta, a");
    lines_[i + 1] = asm_line::parse("\tjr\tnc, " + join_label);
    lines_[i + 2] = asm_line::parse("\txor\t" + xor_operand);
    lines_[i + 3] = lines_[i + 13];
    lines_[i + 4] = asm_line::parse("\tld\t" + slot + ", a");
    lines_[i + 4].comment = final_store_comment;
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 5),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 16));
    return true;
}

bool z80_peep::rule_zero_extended_bit7_shift_xor_diamond(size_t i) {
    if (i + 15 >= lines_.size())
        return false;
    if (!lines_[i].label.empty())
        return false;

    auto is_add_a_a = [](const asm_line &line) {
        if (line.mnemonic != "add")
            return false;
        const std::string ops = trim(line.operands);
        return ops == "a, a" || ops == "a,a";
    };
    auto is_ld_l_a = [](const asm_line &line) {
        if (line.mnemonic != "ld")
            return false;
        std::string dst;
        std::string src;
        return split_ld(line.operands, dst, src) &&
               trim(dst) == "l" && trim(src) == "a";
    };
    auto is_ld_h_zero = [](const asm_line &line) {
        if (line.mnemonic != "ld")
            return false;
        std::string dst;
        std::string src;
        return split_ld(line.operands, dst, src) &&
               trim(dst) == "h" && immediate_is(src, 0);
    };
    auto parse_temp_store = [&](size_t pos, const std::string &src_reg,
                                int &off) {
        if (pos >= lines_.size() || lines_[pos].mnemonic != "ld")
            return false;
        std::string dst;
        std::string src;
        return split_ld(lines_[pos].operands, dst, src) &&
               trim(src) == src_reg &&
               parse_ix_ref(lower_copy(trim(dst)), off);
    };
    auto parse_shift_store = [&](size_t &pos, int &temp_off) {
        if (pos >= lines_.size() || !is_add_a_a(lines_[pos]))
            return false;
        ++pos;
        if (parse_temp_store(pos, "a", temp_off)) {
            ++pos;
            return true;
        }
        if (pos + 2 >= lines_.size() ||
            !is_ld_l_a(lines_[pos]) ||
            !is_ld_h_zero(lines_[pos + 1]) ||
            !parse_temp_store(pos + 2, "l", temp_off)) {
            return false;
        }
        pos += 3;
        return true;
    };
    auto parse_shift_xor_store = [&](size_t &pos, int &temp_off,
                                     int &xor_value) {
        if (pos >= lines_.size() || !is_add_a_a(lines_[pos]))
            return false;
        ++pos;
        if (pos + 1 < lines_.size() &&
            lines_[pos].mnemonic == "xor" &&
            parse_immediate_value(lines_[pos].operands, xor_value) &&
            parse_temp_store(pos + 1, "a", temp_off)) {
            pos += 2;
            return true;
        }
        if (pos + 8 >= lines_.size() ||
            !is_ld_l_a(lines_[pos]) ||
            !is_ld_h_zero(lines_[pos + 1])) {
            return false;
        }
        std::string dst;
        std::string src;
        if (lines_[pos + 2].mnemonic != "ld" ||
            !split_ld(lines_[pos + 2].operands, dst, src) ||
            trim(dst) != "a" || trim(src) != "l" ||
            lines_[pos + 3].mnemonic != "xor" ||
            !parse_immediate_value(lines_[pos + 3].operands, xor_value) ||
            lines_[pos + 4].mnemonic != "ld" ||
            !split_ld(lines_[pos + 4].operands, dst, src) ||
            trim(dst) != "l" || trim(src) != "a" ||
            lines_[pos + 5].mnemonic != "ld" ||
            !split_ld(lines_[pos + 5].operands, dst, src) ||
            trim(dst) != "a" || trim(src) != "h" ||
            lines_[pos + 6].mnemonic != "xor" ||
            !immediate_is(lines_[pos + 6].operands, 0) ||
            lines_[pos + 7].mnemonic != "ld" ||
            !split_ld(lines_[pos + 7].operands, dst, src) ||
            trim(dst) != "h" || trim(src) != "a" ||
            !parse_temp_store(pos + 8, "l", temp_off)) {
            return false;
        }
        pos += 9;
        return true;
    };

    if (!is_ld_l_a(lines_[i]) || !is_ld_h_zero(lines_[i + 1]))
        return false;
    int bit = -1;
    std::string bit_reg;
    if (lines_[i + 2].mnemonic != "bit" ||
        !parse_bit_reg_operands(lines_[i + 2].operands, bit, bit_reg) ||
        bit != 7 || bit_reg != "l") {
        return false;
    }

    std::string cc;
    std::string else_label;
    if (!split_conditional_branch_target(lines_[i + 3], cc, else_label) ||
        cc != "z") {
        return false;
    }

    size_t pos = i + 4;
    const size_t true_label_idx = pos;
    const bool has_true_label = pos < lines_.size() &&
                                !lines_[pos].label.empty() &&
                                lines_[pos].mnemonic.empty();
    if (has_true_label) {
        if (label_has_other_control_references(lines_, lines_[pos].label,
                                               lines_.size())) {
            return false;
        }
        ++pos;
    }

    int true_temp_off = 0;
    int xor_value = 0;
    if (!parse_shift_xor_store(pos, true_temp_off, xor_value))
        return false;

    const size_t goto_idx = pos;
    std::string join_label;
    if (!parse_unconditional_jump(lines_[pos], join_label))
        return false;
    ++pos;

    const size_t else_label_idx = pos;
    if (pos >= lines_.size() ||
        lines_[pos].label != else_label ||
        !lines_[pos].mnemonic.empty()) {
        return false;
    }
    if (label_has_other_control_references(lines_, else_label, i + 3))
        return false;
    ++pos;

    int false_temp_off = 0;
    if (!parse_shift_store(pos, false_temp_off) ||
        false_temp_off != true_temp_off) {
        return false;
    }

    const size_t join_label_idx = pos;
    if (pos >= lines_.size() ||
        lines_[pos].label != join_label ||
        !lines_[pos].mnemonic.empty()) {
        return false;
    }
    if (label_has_other_control_references(lines_, join_label, goto_idx))
        return false;
    ++pos;

    int reload_temp_off = 0;
    std::string dst;
    std::string src;
    if (pos + 1 >= lines_.size() ||
        lines_[pos].mnemonic != "ld" ||
        !split_ld(lines_[pos].operands, dst, src) ||
        trim(dst) != "a" ||
        !parse_ix_ref(lower_copy(trim(src)), reload_temp_off) ||
        reload_temp_off != true_temp_off) {
        return false;
    }
    ++pos;

    const size_t reload_idx = pos - 1;
    bool has_final_store = false;
    std::string final_slot;
    size_t end_idx = reload_idx;
    size_t after = reload_idx + 1;
    std::string final_store_comment;
    if (pos < lines_.size() && lines_[pos].mnemonic == "ld" &&
        split_ld(lines_[pos].operands, dst, src) &&
        trim(src) == "a") {
        int final_slot_off = 0;
        if (parse_ix_ref(lower_copy(trim(dst)), final_slot_off)) {
            has_final_store = true;
            final_slot = lower_copy(trim(dst));
            end_idx = pos;
            after = pos + 1;
            final_store_comment = lines_[pos].comment;
        }
    }

    for (size_t k = i; k <= end_idx; ++k) {
        const bool allowed_label =
            (has_true_label && k == true_label_idx) ||
            k == else_label_idx ||
            k == join_label_idx;
        if (!allowed_label && !lines_[k].label.empty())
            return false;
    }

    int locals = 0;
    int temp_frame = 0;
    size_t prologue_index = 0;
    if (!current_function_frame(lines_, i, locals, temp_frame, prologue_index))
        return false;
    if (!ix_offset_in_temp_frame(true_temp_off, locals, temp_frame))
        return false;
    const size_t fn_end = function_end_after_prologue(lines_, prologue_index);
    if (ix_slot_read_in_function(lines_, prologue_index, fn_end,
                                 true_temp_off, reload_idx)) {
        return false;
    }
    const bool preserve_hl = !path_overwrites_hl_before_read(lines_, after);
    if (!flags_overwritten_before_read_or_escape(lines_, after)) {
        return false;
    }

    lines_[i] = asm_line::parse("\tadd\ta, a");
    lines_[i + 1] = asm_line::parse("\tjr\tnc, " + join_label);
    lines_[i + 2] = asm_line::parse("\txor\t" + imm8_text(xor_value));
    lines_[i + 3] = lines_[join_label_idx];
    if (has_final_store) {
        lines_[i + 4] = asm_line::parse("\tld\t" + final_slot + ", a");
        lines_[i + 4].comment = final_store_comment;
        size_t erase_begin = i + 5;
        if (preserve_hl) {
            lines_[i + 5] = asm_line::parse("\tld\tl, a");
            lines_[i + 6] = asm_line::parse("\tld\th, #0");
            erase_begin = i + 7;
        }
        lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(erase_begin),
                     lines_.begin() + static_cast<std::ptrdiff_t>(end_idx + 1));
    } else {
        size_t erase_begin = i + 4;
        if (preserve_hl) {
            lines_[i + 4] = asm_line::parse("\tld\tl, a");
            lines_[i + 5] = asm_line::parse("\tld\th, #0");
            erase_begin = i + 6;
        }
        lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(erase_begin),
                     lines_.begin() + static_cast<std::ptrdiff_t>(end_idx + 1));
    }
    return true;
}

bool z80_peep::rule_signed_byte_low_arith(size_t i) {
    if (i + 12 >= lines_.size())
        return false;

    auto no_labels = [&](size_t end_inclusive) {
        if (end_inclusive >= lines_.size())
            return false;
        for (size_t k = i; k <= end_inclusive; ++k) {
            if (!lines_[k].label.empty())
                return false;
        }
        return true;
    };
    auto is_ld_reg_reg = [](const asm_line &line,
                            const std::string &dst_reg,
                            const std::string &src_reg) {
        if (line.mnemonic != "ld")
            return false;
        std::string dst;
        std::string src;
        return split_ld(line.operands, dst, src) &&
               trim(dst) == dst_reg && trim(src) == src_reg;
    };
    auto is_or_a_a = [](const asm_line &line) {
        if (line.mnemonic != "or")
            return false;
        const std::string ops = trim(line.operands);
        return ops == "a, a" || ops == "a,a" || ops == "a";
    };
    auto is_add_hl_hl = [](const asm_line &line) {
        if (line.mnemonic != "add")
            return false;
        const std::string ops = trim(line.operands);
        return ops == "hl, hl" || ops == "hl,hl";
    };
    auto parse_signext_hl = [&](size_t pos, std::string &src) {
        if (pos + 4 >= lines_.size())
            return false;
        std::string dst;
        std::string value;
        if (lines_[pos].mnemonic != "ld" ||
            !split_ld(lines_[pos].operands, dst, value) ||
            trim(dst) != "a") {
            return false;
        }
        src = lower_copy(trim(value));
        return is_ld_reg_reg(lines_[pos + 1], "l", "a") &&
               lines_[pos + 2].mnemonic == "rlca" &&
               lines_[pos + 3].mnemonic == "sbc" &&
               trim(lines_[pos + 3].operands) == "a, a" &&
               is_ld_reg_reg(lines_[pos + 4], "h", "a");
    };
    auto parse_store_a_to = [&](size_t pos, const std::string &slot) {
        if (pos >= lines_.size() || lines_[pos].mnemonic != "ld")
            return false;
        std::string dst;
        std::string src;
        return split_ld(lines_[pos].operands, dst, src) &&
               lower_copy(trim(dst)) == slot &&
               trim(src) == "a";
    };
    auto parse_store_hl_to_temp = [&](size_t pos, int &lo, int &hi) {
        if (pos + 1 >= lines_.size() ||
            lines_[pos].mnemonic != "ld" ||
            lines_[pos + 1].mnemonic != "ld") {
            return false;
        }
        std::string dst;
        std::string src;
        if (!split_ld(lines_[pos].operands, dst, src) ||
            trim(src) != "l" ||
            !parse_ix_ref(lower_copy(trim(dst)), lo)) {
            return false;
        }
        if (!split_ld(lines_[pos + 1].operands, dst, src) ||
            trim(src) != "h" ||
            !parse_ix_ref(lower_copy(trim(dst)), hi)) {
            return false;
        }
        return hi == lo + 1;
    };
    auto is_ld_reg_from_ix = [](const asm_line &line,
                                const std::string &reg,
                                int offset) {
        if (line.mnemonic != "ld")
            return false;
        std::string dst;
        std::string src;
        int parsed = 0;
        return split_ld(line.operands, dst, src) &&
               trim(dst) == reg &&
               parse_ix_ref(lower_copy(trim(src)), parsed) &&
               parsed == offset;
    };

    auto validate_low_byte_slots = [&](const std::string &acc,
                                       const std::string &rhs) {
        int acc_off = 0;
        int rhs_off = 0;
        return parse_ix_ref(acc, acc_off) && parse_ix_ref(rhs, rhs_off);
    };
    auto validate_tail_dead = [&](size_t after) {
        return path_overwrites_hl_before_read(lines_, after) &&
               path_overwrites_pair_before_read(lines_, after, "bc", 'c', 'b') &&
               flags_overwritten_before_read_or_escape(lines_, after);
    };

    std::string acc0;
    std::string rhs0;
    std::string acc1;

    // (signed char)acc +/- (signed char)rhs, immediately stored as a byte.
    if (i + 27 < lines_.size() &&
        parse_signext_hl(i, acc0) &&
        parse_signext_hl(i + 5, rhs0) &&
        is_ld_reg_reg(lines_[i + 10], "b", "h") &&
        is_ld_reg_reg(lines_[i + 11], "c", "l") &&
        parse_signext_hl(i + 12, acc1) &&
        acc1 == acc0 &&
        validate_low_byte_slots(acc0, rhs0)) {
        const bool is_sub =
            lines_[i + 17].mnemonic == "push" &&
            trim(lines_[i + 17].operands) == "hl" &&
            lines_[i + 18].mnemonic == "ld" &&
            is_ld_reg_reg(lines_[i + 19], "e", "a") &&
            lines_[i + 20].mnemonic == "rlca" &&
            lines_[i + 21].mnemonic == "sbc" &&
            trim(lines_[i + 21].operands) == "a, a" &&
            is_ld_reg_reg(lines_[i + 22], "d", "a") &&
            lines_[i + 23].mnemonic == "pop" &&
            trim(lines_[i + 23].operands) == "hl" &&
            is_or_a_a(lines_[i + 24]) &&
            lines_[i + 25].mnemonic == "sbc" &&
            trim(lines_[i + 25].operands) == "hl, de" &&
            is_ld_reg_reg(lines_[i + 26], "a", "l") &&
            parse_store_a_to(i + 27, acc0);
        if (is_sub && no_labels(i + 27)) {
            std::string dst;
            std::string src;
            split_ld(lines_[i + 18].operands, dst, src);
            if (lower_copy(trim(src)) == rhs0 && validate_tail_dead(i + 28)) {
                lines_[i] = asm_line::parse("\tld\ta, " + acc0);
                lines_[i + 1] = asm_line::parse("\tsub\ta, " + rhs0);
                lines_[i + 2] = asm_line::parse("\tld\t" + acc0 + ", a");
                lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 3),
                             lines_.begin() + static_cast<std::ptrdiff_t>(i + 28));
                return true;
            }
        }

        const bool is_add =
            lines_[i + 17].mnemonic == "add" &&
            trim(lines_[i + 17].operands) == "hl, bc" &&
            is_ld_reg_reg(lines_[i + 18], "a", "l") &&
            parse_store_a_to(i + 19, acc0);
        if (is_add && no_labels(i + 19) && validate_tail_dead(i + 20)) {
            lines_[i] = asm_line::parse("\tld\ta, " + acc0);
            lines_[i + 1] = asm_line::parse("\tadd\ta, " + rhs0);
            lines_[i + 2] = asm_line::parse("\tld\t" + acc0 + ", a");
            lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 3),
                         lines_.begin() + static_cast<std::ptrdiff_t>(i + 20));
            return true;
        }
    }

    // ((signed char)acc << 1) ^ (signed char)rhs, immediately stored as byte.
    if (i + 24 < lines_.size() && no_labels(i + 24) &&
        parse_signext_hl(i, acc0) &&
        is_add_hl_hl(lines_[i + 5])) {
        int temp_lo = 0;
        int temp_hi = 0;
        if (parse_store_hl_to_temp(i + 6, temp_lo, temp_hi) &&
            parse_signext_hl(i + 8, rhs0) &&
            validate_low_byte_slots(acc0, rhs0) &&
            is_ld_reg_reg(lines_[i + 13], "b", "h") &&
            is_ld_reg_reg(lines_[i + 14], "c", "l") &&
            is_ld_reg_from_ix(lines_[i + 15], "l", temp_lo) &&
            is_ld_reg_from_ix(lines_[i + 16], "h", temp_hi) &&
            is_ld_reg_reg(lines_[i + 17], "a", "l") &&
            lines_[i + 18].mnemonic == "xor" &&
            trim(lines_[i + 18].operands) == "a, c" &&
            is_ld_reg_reg(lines_[i + 19], "l", "a") &&
            is_ld_reg_reg(lines_[i + 20], "a", "h") &&
            lines_[i + 21].mnemonic == "xor" &&
            trim(lines_[i + 21].operands) == "a, b" &&
            is_ld_reg_reg(lines_[i + 22], "h", "a") &&
            is_ld_reg_reg(lines_[i + 23], "a", "l") &&
            parse_store_a_to(i + 24, acc0) &&
            validate_tail_dead(i + 25)) {
            lines_[i] = asm_line::parse("\tld\ta, " + acc0);
            lines_[i + 1] = asm_line::parse("\tadd\ta, a");
            lines_[i + 2] = asm_line::parse("\txor\ta, " + rhs0);
            lines_[i + 3] = asm_line::parse("\tld\t" + acc0 + ", a");
            lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 4),
                         lines_.begin() + static_cast<std::ptrdiff_t>(i + 25));
            return true;
        }
    }

    return false;
}

bool z80_peep::rule_de_byte_alu_shuttle(size_t i) {
    if (i + 3 >= lines_.size())
        return false;
    for (size_t j = i; j <= i + 3; ++j) {
        if (!lines_[j].label.empty())
            return false;
    }

    std::string dst_e;
    std::string src_e;
    std::string dst_d;
    std::string src_d;
    std::string dst_a;
    std::string src_a;
    if (lines_[i].mnemonic != "ld" ||
        !split_ld(lines_[i].operands, dst_e, src_e) ||
        trim(dst_e) != "e" ||
        lines_[i + 1].mnemonic != "ld" ||
        !split_ld(lines_[i + 1].operands, dst_d, src_d) ||
        trim(dst_d) != "d" ||
        lines_[i + 2].mnemonic != "ld" ||
        !split_ld(lines_[i + 2].operands, dst_a, src_a) ||
        trim(dst_a) != "a" ||
        trim(src_a) != "e") {
        return false;
    }

    src_e = lower_copy(trim(src_e));
    src_d = lower_copy(trim(src_d));
    if (operand_mentions_pair_or_bytes(src_e, "de", 'e', 'd') ||
        operand_mentions_pair_or_bytes(src_d, "de", 'e', 'd') ||
        operand_has_token(src_d, "a") ||
        operand_has_token(src_d, "af")) {
        return false;
    }

    auto byte_source_supported = [](const std::string &src) {
        int ignored = 0;
        return is_plain_8bit_reg(src) || is_immediate_operand(src) ||
               is_numeric_literal(src) || parse_ixiy_ref(src, ignored) ||
               src == "(hl)";
    };
    if (!byte_source_supported(src_e) || !byte_source_supported(src_d))
        return false;

    const std::string alu = lines_[i + 3].mnemonic;
    if (alu != "xor" && alu != "add" && alu != "adc" && alu != "sub" &&
        alu != "sbc" && alu != "and" && alu != "or" && alu != "cp") {
        return false;
    }

    const std::string ops = trim(lines_[i + 3].operands);
    const bool d_operand =
        ops == "d" || ops == "a, d" || ops == "a,d";
    if (!d_operand)
        return false;

    if (!path_overwrites_pair_before_read(lines_, i + 4, "de", 'e', 'd'))
        return false;

    lines_[i] = asm_line::parse("\tld\ta, " + src_e);
    lines_[i + 1] = asm_line::parse("\t" + alu + "\ta, " + src_d);
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 2),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 4));
    return true;
}

bool z80_peep::rule_c_byte_alu_shuttle(size_t i) {
    if (i + 1 >= lines_.size())
        return false;
    if (!lines_[i].label.empty() || !lines_[i + 1].label.empty())
        return false;

    std::string dst;
    std::string src;
    if (lines_[i].mnemonic != "ld" ||
        !split_ld(lines_[i].operands, dst, src) ||
        trim(dst) != "c") {
        return false;
    }
    src = lower_copy(trim(src));
    if (operand_mentions_pair_or_bytes(src, "bc", 'c', 'b'))
        return false;

    auto byte_source_supported = [](const std::string &value) {
        int ignored = 0;
        return is_plain_8bit_reg(value) || is_immediate_operand(value) ||
               is_numeric_literal(value) || parse_ixiy_ref(value, ignored) ||
               value == "(hl)";
    };
    if (!byte_source_supported(src))
        return false;

    const std::string alu = lines_[i + 1].mnemonic;
    if (alu != "xor" && alu != "add" && alu != "adc" && alu != "sub" &&
        alu != "sbc" && alu != "and" && alu != "or" && alu != "cp") {
        return false;
    }

    const std::string ops = trim(lines_[i + 1].operands);
    if (ops != "c" && ops != "a, c" && ops != "a,c")
        return false;

    if (!c_overwritten_or_call_before_read(lines_, i + 2) &&
        !bc_dead_before_read_or_ret(lines_, i + 2)) {
        return false;
    }

    if (alu == "add" || alu == "adc" || alu == "sub" || alu == "sbc") {
        lines_[i] = asm_line::parse("\t" + alu + "\ta, " + src);
    } else {
        lines_[i] = asm_line::parse("\t" + alu + "\t" + src);
    }
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 1));
    return true;
}

bool z80_peep::rule_zero_extended_byte_shr_to_a(size_t i) {
    if (i + 4 >= lines_.size())
        return false;

    auto byte_source_supported = [](const std::string &value) {
        int ignored = 0;
        return is_plain_8bit_reg(value) || is_immediate_operand(value) ||
               is_numeric_literal(value) || parse_ixiy_ref(value, ignored) ||
               value == "(hl)";
    };

    std::string dst;
    std::string src;
    if (!lines_[i].label.empty() ||
        lines_[i].mnemonic != "ld" ||
        !split_ld(lines_[i].operands, dst, src) ||
        trim(dst) != "l") {
        return false;
    }
    const std::string value_src = lower_copy(trim(src));
    if (!byte_source_supported(value_src))
        return false;

    if (!lines_[i + 1].label.empty() ||
        lines_[i + 1].mnemonic != "ld" ||
        !split_ld(lines_[i + 1].operands, dst, src) ||
        trim(dst) != "h" ||
        !immediate_is(src, 0)) {
        return false;
    }

    size_t pos = i + 2;
    int shifts = 0;
    while (pos + 1 < lines_.size() && shifts < 8) {
        if (!lines_[pos].label.empty() || !lines_[pos + 1].label.empty())
            return false;
        if (lines_[pos].mnemonic != "sra" ||
            trim(lines_[pos].operands) != "h" ||
            lines_[pos + 1].mnemonic != "rr" ||
            trim(lines_[pos + 1].operands) != "l") {
            break;
        }
        pos += 2;
        ++shifts;
    }
    if (shifts == 0 || pos >= lines_.size())
        return false;
    if (!lines_[pos].label.empty() ||
        lines_[pos].mnemonic != "ld" ||
        !split_ld(lines_[pos].operands, dst, src) ||
        trim(dst) != "a" ||
        trim(src) != "l") {
        return false;
    }

    if (!path_overwrites_hl_before_read(lines_, pos + 1))
        return false;

    std::vector<asm_line> replacement;
    if (value_src != "a")
        replacement.push_back(asm_line::parse("\tld\ta, " + value_src));
    for (int n = 0; n < shifts; ++n)
        replacement.push_back(asm_line::parse("\tsrl\ta"));

    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                 lines_.begin() + static_cast<std::ptrdiff_t>(pos + 1));
    lines_.insert(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                  replacement.begin(), replacement.end());
    return true;
}

bool z80_peep::rule_low_byte_shift_add_to_a(size_t i) {
    if (i + 9 >= lines_.size())
        return false;

    auto byte_source_supported = [](const std::string &value) {
        int ignored = 0;
        return is_plain_8bit_reg(value) || is_immediate_operand(value) ||
               is_numeric_literal(value) || parse_ixiy_ref(value, ignored) ||
               value == "(hl)";
    };
    auto is_ld = [&](size_t pos,
                     const std::string &want_dst,
                     const std::string &want_src) {
        if (pos >= lines_.size() || !lines_[pos].label.empty() ||
            lines_[pos].mnemonic != "ld") {
            return false;
        }
        std::string dst;
        std::string src;
        return split_ld(lines_[pos].operands, dst, src) &&
               trim(dst) == want_dst && trim(src) == want_src;
    };
    auto is_add_hl_hl = [&](size_t pos) {
        return pos < lines_.size() && lines_[pos].label.empty() &&
               lines_[pos].mnemonic == "add" &&
               (trim(lines_[pos].operands) == "hl, hl" ||
                trim(lines_[pos].operands) == "hl,hl");
    };
    auto is_add_hl_source = [&](size_t pos) {
        if (pos >= lines_.size() || !lines_[pos].label.empty() ||
            lines_[pos].mnemonic != "add") {
            return false;
        }
        const std::string ops = trim(lines_[pos].operands);
        return ops == "hl, de" || ops == "hl,de" ||
               ops == "hl, bc" || ops == "hl,bc";
    };

    std::string dst;
    std::string src;
    if (!lines_[i].label.empty() ||
        lines_[i].mnemonic != "ld" ||
        !split_ld(lines_[i].operands, dst, src) ||
        trim(dst) != "a") {
        return false;
    }
    const std::string value_src = lower_copy(trim(src));
    if (!byte_source_supported(value_src))
        return false;

    if (!is_ld(i + 1, "c", "a") ||
        !is_ld(i + 2, "b", "#0") ||
        !is_ld(i + 3, "e", "a") ||
        !is_ld(i + 4, "d", "#0")) {
        return false;
    }
    if (!is_ld(i + 5, "h", "d") &&
        !is_ld(i + 5, "h", "#0")) {
        return false;
    }
    if (!is_ld(i + 6, "l", "e"))
        return false;

    std::vector<std::string> ops;
    size_t pos = i + 7;
    while (pos < lines_.size() && ops.size() < 24) {
        if (!lines_[pos].label.empty())
            return false;
        if (is_add_hl_hl(pos)) {
            ops.push_back("double");
            ++pos;
            continue;
        }
        if (is_add_hl_source(pos)) {
            ops.push_back("add_source");
            ++pos;
            continue;
        }
        if (lines_[pos].mnemonic == "inc" &&
            trim(lines_[pos].operands) == "hl") {
            ops.push_back("inc");
            ++pos;
            continue;
        }
        break;
    }
    if (ops.empty() || pos >= lines_.size())
        return false;
    if (!lines_[pos].label.empty() ||
        lines_[pos].mnemonic != "ld" ||
        !split_ld(lines_[pos].operands, dst, src) ||
        trim(dst) != "a" ||
        trim(src) != "l") {
        return false;
    }

    const size_t after = pos + 1;
    if (!path_overwrites_hl_before_read(lines_, after) ||
        !path_overwrites_pair_before_read(lines_, after, "de", 'e', 'd') ||
        !path_overwrites_pair_before_read(lines_, after, "bc", 'c', 'b') ||
        !flags_overwritten_before_read_or_escape(lines_, after)) {
        return false;
    }

    std::vector<asm_line> replacement;
    replacement.push_back(asm_line::parse("\tld\ta, " + value_src));
    replacement.push_back(asm_line::parse("\tld\te, a"));
    for (const std::string &op : ops) {
        if (op == "double") {
            replacement.push_back(asm_line::parse("\tadd\ta, a"));
        } else if (op == "add_source") {
            replacement.push_back(asm_line::parse("\tadd\ta, e"));
        } else if (op == "inc") {
            replacement.push_back(asm_line::parse("\tinc\ta"));
        }
    }

    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                 lines_.begin() + static_cast<std::ptrdiff_t>(pos + 1));
    lines_.insert(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                  replacement.begin(), replacement.end());
    return true;
}

bool z80_peep::rule_dead_a_store_before_join_store(size_t i) {
    if (i + 5 >= lines_.size())
        return false;

    std::string dst;
    std::string src;
    if (lines_[i].mnemonic != "ld" ||
        !split_ld(lines_[i].operands, dst, src) ||
        !lines_[i].label.empty() ||
        trim(src) != "a") {
        return false;
    }
    const std::string slot = lower_copy(trim(dst));
    int ignored = 0;
    if (!parse_ixiy_ref(slot, ignored))
        return false;

    size_t pos = i + 1;
    bool saw_diamond = false;
    for (int diamonds = 0; diamonds < 6; ++diamonds) {
        if (pos + 3 >= lines_.size())
            return false;

        for (size_t k = pos; k <= pos + 2; ++k) {
            if (!lines_[k].label.empty())
                return false;
        }

        if (lines_[pos].mnemonic != "add")
            return false;
        const std::string add_ops = trim(lines_[pos].operands);
        if (add_ops != "a, a" && add_ops != "a,a")
            return false;

        std::string cc;
        std::string target;
        if (!split_conditional_branch_target(lines_[pos + 1], cc, target))
            return false;

        int xor_value = 0;
        if (lines_[pos + 2].mnemonic != "xor" ||
            !parse_immediate_value(lines_[pos + 2].operands, xor_value)) {
            return false;
        }

        if (lines_[pos + 3].label != target ||
            !lines_[pos + 3].mnemonic.empty() ||
            label_has_other_control_references(lines_, target, pos + 1)) {
            return false;
        }

        saw_diamond = true;
        pos += 4;

        if (pos >= lines_.size())
            return false;
        if (!lines_[pos].label.empty())
            return false;
        if (lines_[pos].mnemonic == "ld" &&
            split_ld(lines_[pos].operands, dst, src) &&
            lower_copy(trim(dst)) == slot &&
            trim(src) == "a") {
            break;
        }
    }

    if (!saw_diamond)
        return false;
    if (pos >= lines_.size() || lines_[pos].mnemonic != "ld" ||
        !split_ld(lines_[pos].operands, dst, src) ||
        lower_copy(trim(dst)) != slot ||
        trim(src) != "a") {
        return false;
    }

    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i));
    return true;
}

bool z80_peep::rule_ix_byte_branch_temp_to_d(size_t i) {
    if (i + 16 >= lines_.size())
        return false;

    auto unexpected_label = [&](size_t k) {
        return k != i + 3 && k != i + 8 && k != i + 12 &&
               !lines_[k].label.empty();
    };
    for (size_t k = i; k <= i + 16; ++k) {
        if (unexpected_label(k))
            return false;
    }

    std::string dst;
    std::string src;
    if (lines_[i].mnemonic != "ld" ||
        !split_ld(lines_[i].operands, dst, src) ||
        trim(src) != "a") {
        return false;
    }
    const std::string byte_slot = lower_copy(trim(dst));
    int ignored = 0;
    if (!parse_ixiy_ref(byte_slot, ignored))
        return false;

    int bit = -1;
    std::string bit_reg;
    if (lines_[i + 1].mnemonic != "bit" ||
        !parse_bit_reg_operands(lines_[i + 1].operands, bit, bit_reg) ||
        bit != 7 || bit_reg != "a") {
        return false;
    }

    std::string cc;
    std::string positive_label;
    if (!split_conditional_branch_target(lines_[i + 2], cc, positive_label))
        return false;

    if (!lines_[i + 3].label.empty() &&
        label_has_other_control_references(lines_, lines_[i + 3].label,
                                           lines_.size())) {
        return false;
    }

    if (lines_[i + 8].label != positive_label ||
        !lines_[i + 8].mnemonic.empty() ||
        label_has_other_control_references(lines_, positive_label, i + 2)) {
        return false;
    }

    std::string join_label;
    if (!parse_unconditional_jump(lines_[i + 7], join_label))
        return false;
    if (lines_[i + 12].label != join_label ||
        !lines_[i + 12].mnemonic.empty() ||
        label_has_other_control_references(lines_, join_label, i + 7)) {
        return false;
    }

    auto parse_ld_a_from = [&](size_t pos, std::string &source) {
        if (lines_[pos].mnemonic != "ld")
            return false;
        std::string local_dst;
        std::string local_src;
        if (!split_ld(lines_[pos].operands, local_dst, local_src) ||
            trim(local_dst) != "a") {
            return false;
        }
        source = lower_copy(trim(local_src));
        return true;
    };

    auto parse_ld_to_same = [&](size_t pos, const std::string &target) {
        if (lines_[pos].mnemonic != "ld")
            return false;
        std::string local_dst;
        std::string local_src;
        return split_ld(lines_[pos].operands, local_dst, local_src) &&
               lower_copy(trim(local_dst)) == target &&
               trim(local_src) == "a";
    };

    auto alu_uses_slot = [&](size_t pos, const char *mnemonic) {
        if (lines_[pos].mnemonic != mnemonic)
            return false;
        const std::string ops = lower_copy(trim(lines_[pos].operands));
        return ops == byte_slot || ops == std::string("a, ") + byte_slot ||
               ops == std::string("a,") + byte_slot;
    };

    std::string acc_slot;
    if (!parse_ld_a_from(i + 4, acc_slot) ||
        !parse_ixiy_ref(acc_slot, ignored) ||
        !alu_uses_slot(i + 5, "sub") ||
        !parse_ld_to_same(i + 6, acc_slot) ||
        !parse_ld_a_from(i + 9, src) ||
        src != acc_slot ||
        !alu_uses_slot(i + 10, "add") ||
        !parse_ld_to_same(i + 11, acc_slot) ||
        !parse_ld_a_from(i + 13, src) ||
        src != acc_slot) {
        return false;
    }

    const std::string add_ops = trim(lines_[i + 14].operands);
    if (lines_[i + 14].mnemonic != "add" ||
        (add_ops != "a, a" && add_ops != "a,a")) {
        return false;
    }
    if (!alu_uses_slot(i + 15, "xor") ||
        !parse_ld_to_same(i + 16, acc_slot)) {
        return false;
    }

    if (!path_overwrites_pair_before_read(lines_, i + 17, "de", 'e', 'd'))
        return false;

    lines_[i] = asm_line::parse("\tld\td, a");
    lines_[i + 5] = asm_line::parse("\tsub\ta, d");
    lines_[i + 10] = asm_line::parse("\tadd\ta, d");
    lines_[i + 15] = asm_line::parse("\txor\ta, d");
    return true;
}

bool z80_peep::rule_bit15_shift_xor_diamond(size_t i) {
    if (i + 27 >= lines_.size())
        return false;

    auto is_add_hl_hl = [](const asm_line &line) {
        if (line.mnemonic != "add")
            return false;
        const std::string ops = trim(line.operands);
        return ops == "hl, hl" || ops == "hl,hl";
    };

    auto parse_ld_ix_from_reg = [](const asm_line &line,
                                   const std::string &reg,
                                   int &offset) {
        if (line.mnemonic != "ld")
            return false;
        std::string dst;
        std::string src;
        return split_ld(line.operands, dst, src) &&
               trim(src) == reg &&
               parse_ix_ref(lower_copy(trim(dst)), offset);
    };

    auto is_ld_reg_from_ix = [](const asm_line &line,
                                const std::string &reg,
                                int offset) {
        if (line.mnemonic != "ld")
            return false;
        std::string dst;
        std::string src;
        int parsed = 0;
        return split_ld(line.operands, dst, src) &&
               trim(dst) == reg &&
               parse_ix_ref(lower_copy(trim(src)), parsed) &&
               parsed == offset;
    };

    auto is_ld_reg_reg = [](const asm_line &line,
                            const std::string &dst_reg,
                            const std::string &src_reg) {
        if (line.mnemonic != "ld")
            return false;
        std::string dst;
        std::string src;
        return split_ld(line.operands, dst, src) &&
               trim(dst) == dst_reg && trim(src) == src_reg;
    };

    int slot_lo = 0;
    int slot_hi = 0;
    if (!parse_ld_ix_from_reg(lines_[i], "l", slot_lo) ||
        !parse_ld_ix_from_reg(lines_[i + 1], "h", slot_hi) ||
        slot_hi != slot_lo + 1) {
        return false;
    }
    const std::string slot_lo_ref = std::to_string(slot_lo) + "(ix)";
    const std::string slot_hi_ref = std::to_string(slot_hi) + "(ix)";

    int bit = -1;
    std::string bit_reg;
    if (lines_[i + 2].mnemonic != "bit" ||
        !parse_bit_reg_operands(lines_[i + 2].operands, bit, bit_reg) ||
        bit != 7 || (bit_reg != "h" && bit_reg != slot_hi_ref)) {
        return false;
    }

    std::string cc;
    std::string else_label;
    if (!split_conditional_branch_target(lines_[i + 3], cc, else_label) ||
        cc != "z") {
        return false;
    }

    if (lines_[i + 4].label.empty() ||
        !lines_[i + 4].mnemonic.empty() ||
        label_has_other_control_references(lines_, lines_[i + 4].label,
                                           lines_.size())) {
        return false;
    }

    size_t tail_shift = 0;
    int dead_shift_lo = 0;
    int dead_shift_hi = 0;
    if (parse_ld_ix_from_reg(lines_[i + 8], "l", dead_shift_lo) &&
        parse_ld_ix_from_reg(lines_[i + 9], "h", dead_shift_hi) &&
        dead_shift_hi == dead_shift_lo + 1) {
        tail_shift = 2;
    }
    if (i + 27 + tail_shift >= lines_.size())
        return false;

    auto unexpected_label = [&](size_t k) {
        return k != i + 4 &&
               k != i + 17 + tail_shift &&
               k != i + 23 + tail_shift &&
               !lines_[k].label.empty();
    };
    for (size_t k = i; k <= i + 27 + tail_shift; ++k) {
        if (unexpected_label(k))
            return false;
    }

    if (!is_ld_reg_from_ix(lines_[i + 5], "l", slot_lo) ||
        !is_ld_reg_from_ix(lines_[i + 6], "h", slot_hi) ||
        !is_add_hl_hl(lines_[i + 7]) ||
        !is_ld_reg_reg(lines_[i + 8 + tail_shift], "a", "l") ||
        lines_[i + 9 + tail_shift].mnemonic != "xor" ||
        !is_ld_reg_reg(lines_[i + 10 + tail_shift], "l", "a") ||
        !is_ld_reg_reg(lines_[i + 11 + tail_shift], "a", "h") ||
        lines_[i + 12 + tail_shift].mnemonic != "xor" ||
        !is_ld_reg_reg(lines_[i + 13 + tail_shift], "h", "a")) {
        return false;
    }

    const std::string xor_lo_operand =
        trim(lines_[i + 9 + tail_shift].operands);
    const std::string xor_hi_operand =
        trim(lines_[i + 12 + tail_shift].operands);
    int ignored = 0;
    if (!parse_immediate_value(xor_lo_operand, ignored) ||
        !parse_immediate_value(xor_hi_operand, ignored)) {
        return false;
    }

    int temp_lo = 0;
    int temp_hi = 0;
    if (!parse_ld_ix_from_reg(lines_[i + 14 + tail_shift], "l", temp_lo) ||
        !parse_ld_ix_from_reg(lines_[i + 15 + tail_shift], "h", temp_hi) ||
        temp_hi != temp_lo + 1 ||
        temp_lo == slot_lo) {
        return false;
    }

    std::string join_label;
    if (!parse_unconditional_jump(lines_[i + 16 + tail_shift], join_label))
        return false;

    if (lines_[i + 17 + tail_shift].label != else_label ||
        !lines_[i + 17 + tail_shift].mnemonic.empty() ||
        label_has_other_control_references(lines_, else_label, i + 3)) {
        return false;
    }

    int else_temp_lo = 0;
    int else_temp_hi = 0;
    if (!is_ld_reg_from_ix(lines_[i + 18 + tail_shift], "l", slot_lo) ||
        !is_ld_reg_from_ix(lines_[i + 19 + tail_shift], "h", slot_hi) ||
        !is_add_hl_hl(lines_[i + 20 + tail_shift]) ||
        !parse_ld_ix_from_reg(lines_[i + 21 + tail_shift], "l", else_temp_lo) ||
        !parse_ld_ix_from_reg(lines_[i + 22 + tail_shift], "h", else_temp_hi) ||
        else_temp_lo != temp_lo ||
        else_temp_hi != temp_hi) {
        return false;
    }

    if (lines_[i + 23 + tail_shift].label != join_label ||
        !lines_[i + 23 + tail_shift].mnemonic.empty() ||
        label_has_other_control_references(lines_, join_label,
                                           i + 16 + tail_shift)) {
        return false;
    }

    int final_slot_lo = 0;
    int final_slot_hi = 0;
    if (!is_ld_reg_from_ix(lines_[i + 24 + tail_shift], "l", temp_lo) ||
        !is_ld_reg_from_ix(lines_[i + 25 + tail_shift], "h", temp_hi) ||
        !parse_ld_ix_from_reg(lines_[i + 26 + tail_shift], "l", final_slot_lo) ||
        !parse_ld_ix_from_reg(lines_[i + 27 + tail_shift], "h", final_slot_hi) ||
        final_slot_lo != slot_lo ||
        final_slot_hi != slot_hi) {
        return false;
    }

    int locals = 0;
    int temp_frame = 0;
    size_t prologue_index = 0;
    if (!current_function_frame(lines_, i, locals, temp_frame, prologue_index))
        return false;
    if (!ix_offset_in_temp_frame(temp_lo, locals, temp_frame) ||
        !ix_offset_in_temp_frame(temp_hi, locals, temp_frame)) {
        return false;
    }
    if (tail_shift != 0 &&
        (!ix_offset_in_temp_frame(dead_shift_lo, locals, temp_frame) ||
         !ix_offset_in_temp_frame(dead_shift_hi, locals, temp_frame))) {
        return false;
    }

    lines_[i] = asm_line::parse("\tadd\thl, hl");
    lines_[i + 1] = asm_line::parse("\tjr\tnc, " + join_label);
    lines_[i + 2] = asm_line::parse("\tld\ta, l");
    lines_[i + 3] = asm_line::parse("\txor\t" + xor_lo_operand);
    lines_[i + 4] = asm_line::parse("\tld\tl, a");
    lines_[i + 5] = asm_line::parse("\tld\ta, h");
    lines_[i + 6] = asm_line::parse("\txor\t" + xor_hi_operand);
    lines_[i + 7] = asm_line::parse("\tld\th, a");
    lines_[i + 8] = lines_[i + 23 + tail_shift];
    lines_[i + 9] = asm_line::parse("\tld\t" + slot_lo_ref + ", l");
    lines_[i + 10] = asm_line::parse("\tld\t" + slot_hi_ref + ", h");
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 11),
                 lines_.begin() +
                     static_cast<std::ptrdiff_t>(i + 28 + tail_shift));
    return true;
}

bool z80_peep::rule_masked_bit15_shift_xor_diamond(size_t i) {
    if (i + 36 >= lines_.size())
        return false;

    for (size_t k = i; k <= i + 36; ++k) {
        if (k != i + 13 && k != i + 26 && k != i + 32 &&
            !lines_[k].label.empty()) {
            return false;
        }
    }

    auto is_add_hl_hl = [](const asm_line &line) {
        if (line.mnemonic != "add")
            return false;
        const std::string ops = trim(line.operands);
        return ops == "hl, hl" || ops == "hl,hl";
    };

    auto parse_ld_ix_from_reg = [](const asm_line &line,
                                   const std::string &reg,
                                   int &offset) {
        if (line.mnemonic != "ld")
            return false;
        std::string dst;
        std::string src;
        return split_ld(line.operands, dst, src) &&
               trim(src) == reg &&
               parse_ix_ref(lower_copy(trim(dst)), offset);
    };

    auto is_ld_reg_from_ix = [](const asm_line &line,
                                const std::string &reg,
                                int offset) {
        if (line.mnemonic != "ld")
            return false;
        std::string dst;
        std::string src;
        int parsed = 0;
        return split_ld(line.operands, dst, src) &&
               trim(dst) == reg &&
               parse_ix_ref(lower_copy(trim(src)), parsed) &&
               parsed == offset;
    };

    auto is_ld_reg_reg = [](const asm_line &line,
                            const std::string &dst_reg,
                            const std::string &src_reg) {
        if (line.mnemonic != "ld")
            return false;
        std::string dst;
        std::string src;
        return split_ld(line.operands, dst, src) &&
               trim(dst) == dst_reg && trim(src) == src_reg;
    };

    int slot_lo = 0;
    int slot_hi = 0;
    if (!parse_ld_ix_from_reg(lines_[i], "l", slot_lo) ||
        !parse_ld_ix_from_reg(lines_[i + 1], "h", slot_hi) ||
        slot_hi != slot_lo + 1) {
        return false;
    }

    if (!is_ld_reg_reg(lines_[i + 2], "a", "l") ||
        lines_[i + 3].mnemonic != "and" ||
        !immediate_is(lines_[i + 3].operands, 0) ||
        !is_ld_reg_reg(lines_[i + 4], "l", "a") ||
        !is_ld_reg_reg(lines_[i + 5], "a", "h") ||
        lines_[i + 6].mnemonic != "and" ||
        !immediate_is(lines_[i + 6].operands, 128) ||
        !is_ld_reg_reg(lines_[i + 7], "h", "a") ||
        !is_ld_reg_reg(lines_[i + 8], "b", "h") ||
        !is_ld_reg_reg(lines_[i + 9], "c", "l") ||
        !is_ld_reg_reg(lines_[i + 10], "a", "h") ||
        lines_[i + 11].mnemonic != "or") {
        return false;
    }
    const std::string or_ops = trim(lines_[i + 11].operands);
    if (or_ops != "a, l" && or_ops != "a,l" && or_ops != "l")
        return false;

    std::string cc;
    std::string else_label;
    if (!split_conditional_branch_target(lines_[i + 12], cc, else_label) ||
        cc != "z") {
        return false;
    }

    if (lines_[i + 13].label.empty() ||
        !lines_[i + 13].mnemonic.empty() ||
        label_has_other_control_references(lines_, lines_[i + 13].label,
                                           lines_.size())) {
        return false;
    }

    if (!is_ld_reg_from_ix(lines_[i + 14], "l", slot_lo) ||
        !is_ld_reg_from_ix(lines_[i + 15], "h", slot_hi) ||
        !is_add_hl_hl(lines_[i + 16]) ||
        !is_ld_reg_reg(lines_[i + 17], "a", "l") ||
        lines_[i + 18].mnemonic != "xor" ||
        !is_ld_reg_reg(lines_[i + 19], "l", "a") ||
        !is_ld_reg_reg(lines_[i + 20], "a", "h") ||
        lines_[i + 21].mnemonic != "xor" ||
        !is_ld_reg_reg(lines_[i + 22], "h", "a")) {
        return false;
    }

    const std::string xor_lo_operand = trim(lines_[i + 18].operands);
    const std::string xor_hi_operand = trim(lines_[i + 21].operands);
    int ignored = 0;
    if (!parse_immediate_value(xor_lo_operand, ignored) ||
        !parse_immediate_value(xor_hi_operand, ignored)) {
        return false;
    }

    int temp_lo = 0;
    int temp_hi = 0;
    if (!parse_ld_ix_from_reg(lines_[i + 23], "l", temp_lo) ||
        !parse_ld_ix_from_reg(lines_[i + 24], "h", temp_hi) ||
        temp_hi != temp_lo + 1 ||
        temp_lo == slot_lo) {
        return false;
    }

    std::string join_label;
    if (!parse_unconditional_jump(lines_[i + 25], join_label))
        return false;

    if (lines_[i + 26].label != else_label ||
        !lines_[i + 26].mnemonic.empty() ||
        label_has_other_control_references(lines_, else_label, i + 12)) {
        return false;
    }

    int else_temp_lo = 0;
    int else_temp_hi = 0;
    if (!is_ld_reg_from_ix(lines_[i + 27], "l", slot_lo) ||
        !is_ld_reg_from_ix(lines_[i + 28], "h", slot_hi) ||
        !is_add_hl_hl(lines_[i + 29]) ||
        !parse_ld_ix_from_reg(lines_[i + 30], "l", else_temp_lo) ||
        !parse_ld_ix_from_reg(lines_[i + 31], "h", else_temp_hi) ||
        else_temp_lo != temp_lo ||
        else_temp_hi != temp_hi) {
        return false;
    }

    if (lines_[i + 32].label != join_label ||
        !lines_[i + 32].mnemonic.empty() ||
        label_has_other_control_references(lines_, join_label, i + 25)) {
        return false;
    }

    int final_slot_lo = 0;
    int final_slot_hi = 0;
    if (!is_ld_reg_from_ix(lines_[i + 33], "l", temp_lo) ||
        !is_ld_reg_from_ix(lines_[i + 34], "h", temp_hi) ||
        !parse_ld_ix_from_reg(lines_[i + 35], "l", final_slot_lo) ||
        !parse_ld_ix_from_reg(lines_[i + 36], "h", final_slot_hi) ||
        final_slot_lo != slot_lo ||
        final_slot_hi != slot_hi) {
        return false;
    }

    const size_t after_window = i + 37;
    if (!a_overwritten_before_read(lines_, after_window) ||
        !path_overwrites_bc_before_read_or_call(lines_, after_window)) {
        return false;
    }

    int locals = 0;
    int temp_frame = 0;
    size_t prologue_index = 0;
    if (!current_function_frame(lines_, i, locals, temp_frame, prologue_index))
        return false;
    if (!ix_offset_in_temp_frame(temp_lo, locals, temp_frame) ||
        !ix_offset_in_temp_frame(temp_hi, locals, temp_frame)) {
        return false;
    }
    const size_t fn_end = function_end_after_prologue(lines_, prologue_index);
    if (ix_slot_read_in_function(lines_, prologue_index, fn_end,
                                 temp_lo, i + 33) ||
        ix_slot_read_in_function(lines_, prologue_index, fn_end,
                                 temp_hi, i + 34)) {
        return false;
    }

    const std::string slot_lo_ref = std::to_string(slot_lo) + "(ix)";
    const std::string slot_hi_ref = std::to_string(slot_hi) + "(ix)";
    lines_[i] = asm_line::parse("\tadd\thl, hl");
    lines_[i + 1] = asm_line::parse("\tjr\tnc, " + join_label);
    lines_[i + 2] = asm_line::parse("\tld\ta, l");
    lines_[i + 3] = asm_line::parse("\txor\t" + xor_lo_operand);
    lines_[i + 4] = asm_line::parse("\tld\tl, a");
    lines_[i + 5] = asm_line::parse("\tld\ta, h");
    lines_[i + 6] = asm_line::parse("\txor\t" + xor_hi_operand);
    lines_[i + 7] = asm_line::parse("\tld\th, a");
    lines_[i + 8] = lines_[i + 32];
    lines_[i + 9] = asm_line::parse("\tld\t" + slot_lo_ref + ", l");
    lines_[i + 10] = asm_line::parse("\tld\t" + slot_hi_ref + ", h");
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 11),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 37));
    return true;
}

bool z80_peep::rule_lsb32_shift_xor_diamond(size_t i) {
    if (i + 77 >= lines_.size())
        return false;

    for (size_t k = i; k <= i + 77; ++k) {
        if (k != i + 2 && k != i + 48 && k != i + 69 &&
            !lines_[k].label.empty()) {
            return false;
        }
    }

    auto is_shift_right_32 = [&](size_t base) {
        return base + 3 < lines_.size() &&
               lines_[base].mnemonic == "srl" &&
               trim(lines_[base].operands) == "d" &&
               lines_[base + 1].mnemonic == "rr" &&
               trim(lines_[base + 1].operands) == "e" &&
               lines_[base + 2].mnemonic == "rr" &&
               trim(lines_[base + 2].operands) == "h" &&
               lines_[base + 3].mnemonic == "rr" &&
               trim(lines_[base + 3].operands) == "l";
    };

    auto parse_ld_ix_from_reg = [](const asm_line &line,
                                   const std::string &reg,
                                   int &offset) {
        if (line.mnemonic != "ld")
            return false;
        std::string dst;
        std::string src;
        return split_ld(line.operands, dst, src) &&
               trim(src) == reg &&
               parse_ix_ref(lower_copy(trim(dst)), offset);
    };

    auto is_ld_reg_from_ix = [](const asm_line &line,
                                const std::string &reg,
                                int offset) {
        if (line.mnemonic != "ld")
            return false;
        std::string dst;
        std::string src;
        int parsed = 0;
        return split_ld(line.operands, dst, src) &&
               trim(dst) == reg &&
               parse_ix_ref(lower_copy(trim(src)), parsed) &&
               parsed == offset;
    };

    auto is_ld_reg_reg = [](const asm_line &line,
                            const std::string &dst_reg,
                            const std::string &src_reg) {
        if (line.mnemonic != "ld")
            return false;
        std::string dst;
        std::string src;
        return split_ld(line.operands, dst, src) &&
               trim(dst) == dst_reg && trim(src) == src_reg;
    };

    auto is_xor_a_reg = [](const asm_line &line, const std::string &reg) {
        if (line.mnemonic != "xor")
            return false;
        const std::string ops = trim(line.operands);
        return ops == reg || ops == "a, " + reg || ops == "a," + reg;
    };

    auto parse_ld_pair_immediate = [](const asm_line &line,
                                      const std::string &pair,
                                      int &value) {
        if (line.mnemonic != "ld")
            return false;
        std::string dst;
        std::string src;
        return split_ld(line.operands, dst, src) &&
               trim(dst) == pair &&
               parse_immediate_value(src, value);
    };

    int bit = -1;
    std::string bit_operand;
    int src0 = 0;
    if (lines_[i].mnemonic != "bit" ||
        !parse_bit_reg_operands(lines_[i].operands, bit, bit_operand) ||
        bit != 0 ||
        !parse_ix_ref(lower_copy(trim(bit_operand)), src0)) {
        return false;
    }
    const int src1 = src0 + 1;
    const int src2 = src0 + 2;
    const int src3 = src0 + 3;

    std::string cc;
    std::string else_label;
    if (!split_conditional_branch_target(lines_[i + 1], cc, else_label) ||
        cc != "z") {
        return false;
    }

    if (lines_[i + 2].label.empty() ||
        !lines_[i + 2].mnemonic.empty() ||
        label_has_other_control_references(lines_, lines_[i + 2].label,
                                           lines_.size())) {
        return false;
    }

    if (!is_ld_reg_from_ix(lines_[i + 3], "e", src2) ||
        !is_ld_reg_from_ix(lines_[i + 4], "d", src3) ||
        !is_ld_reg_from_ix(lines_[i + 5], "l", src0) ||
        !is_ld_reg_from_ix(lines_[i + 6], "h", src1) ||
        !is_shift_right_32(i + 7)) {
        return false;
    }

    int t0 = 0;
    int t1 = 0;
    int t2 = 0;
    int t3 = 0;
    if (!parse_ld_ix_from_reg(lines_[i + 11], "l", t0) ||
        !parse_ld_ix_from_reg(lines_[i + 12], "h", t1) ||
        !parse_ld_ix_from_reg(lines_[i + 13], "e", t2) ||
        !parse_ld_ix_from_reg(lines_[i + 14], "d", t3)) {
        return false;
    }

    int low_poly = 0;
    if (!is_ld_reg_from_ix(lines_[i + 15], "l", t0) ||
        !is_ld_reg_from_ix(lines_[i + 16], "h", t1) ||
        lines_[i + 17].mnemonic != "ex" ||
        trim(lines_[i + 17].operands) != "de, hl" ||
        !parse_ld_pair_immediate(lines_[i + 18], "hl", low_poly) ||
        !is_ld_reg_reg(lines_[i + 19], "a", "l") ||
        !is_xor_a_reg(lines_[i + 20], "e") ||
        !is_ld_reg_reg(lines_[i + 21], "l", "a") ||
        !is_ld_reg_reg(lines_[i + 22], "a", "h") ||
        !is_xor_a_reg(lines_[i + 23], "d") ||
        !is_ld_reg_reg(lines_[i + 24], "h", "a")) {
        return false;
    }

    int x0 = 0;
    int x1 = 0;
    if (!parse_ld_ix_from_reg(lines_[i + 25], "l", x0) ||
        !parse_ld_ix_from_reg(lines_[i + 26], "h", x1)) {
        return false;
    }

    int high_poly = 0;
    if (!is_ld_reg_from_ix(lines_[i + 27], "l", t2) ||
        !is_ld_reg_from_ix(lines_[i + 28], "h", t3) ||
        lines_[i + 29].mnemonic != "ex" ||
        trim(lines_[i + 29].operands) != "de, hl" ||
        !parse_ld_pair_immediate(lines_[i + 30], "hl", high_poly) ||
        !is_ld_reg_reg(lines_[i + 31], "a", "l") ||
        !is_xor_a_reg(lines_[i + 32], "e") ||
        !is_ld_reg_reg(lines_[i + 33], "l", "a") ||
        !is_ld_reg_reg(lines_[i + 34], "a", "h") ||
        !is_xor_a_reg(lines_[i + 35], "d") ||
        !is_ld_reg_reg(lines_[i + 36], "h", "a")) {
        return false;
    }

    int x2 = 0;
    int x3 = 0;
    if (!parse_ld_ix_from_reg(lines_[i + 37], "l", x2) ||
        !parse_ld_ix_from_reg(lines_[i + 38], "h", x3) ||
        !is_ld_reg_from_ix(lines_[i + 39], "l", x0) ||
        !is_ld_reg_from_ix(lines_[i + 40], "h", x1)) {
        return false;
    }

    int out0 = 0;
    int out1 = 0;
    if (!parse_ld_ix_from_reg(lines_[i + 41], "l", out0) ||
        !parse_ld_ix_from_reg(lines_[i + 42], "h", out1) ||
        !is_ld_reg_from_ix(lines_[i + 43], "l", x2) ||
        !is_ld_reg_from_ix(lines_[i + 44], "h", x3)) {
        return false;
    }

    int out2 = 0;
    int out3 = 0;
    if (!parse_ld_ix_from_reg(lines_[i + 45], "l", out2) ||
        !parse_ld_ix_from_reg(lines_[i + 46], "h", out3)) {
        return false;
    }

    std::string join_label;
    if (!parse_unconditional_jump(lines_[i + 47], join_label))
        return false;

    if (lines_[i + 48].label != else_label ||
        !lines_[i + 48].mnemonic.empty() ||
        label_has_other_control_references(lines_, else_label, i + 1)) {
        return false;
    }

    if (!is_ld_reg_from_ix(lines_[i + 49], "e", src2) ||
        !is_ld_reg_from_ix(lines_[i + 50], "d", src3) ||
        !is_ld_reg_from_ix(lines_[i + 51], "l", src0) ||
        !is_ld_reg_from_ix(lines_[i + 52], "h", src1) ||
        !is_shift_right_32(i + 53)) {
        return false;
    }

    int e0 = 0;
    int e1 = 0;
    int e2 = 0;
    int e3 = 0;
    if (!parse_ld_ix_from_reg(lines_[i + 57], "l", e0) ||
        !parse_ld_ix_from_reg(lines_[i + 58], "h", e1) ||
        !parse_ld_ix_from_reg(lines_[i + 59], "e", e2) ||
        !parse_ld_ix_from_reg(lines_[i + 60], "d", e3) ||
        !is_ld_reg_from_ix(lines_[i + 61], "l", e0) ||
        !is_ld_reg_from_ix(lines_[i + 62], "h", e1)) {
        return false;
    }

    int else_out0 = 0;
    int else_out1 = 0;
    if (!parse_ld_ix_from_reg(lines_[i + 63], "l", else_out0) ||
        !parse_ld_ix_from_reg(lines_[i + 64], "h", else_out1) ||
        else_out0 != out0 ||
        else_out1 != out1 ||
        !is_ld_reg_from_ix(lines_[i + 65], "l", e2) ||
        !is_ld_reg_from_ix(lines_[i + 66], "h", e3)) {
        return false;
    }

    int else_out2 = 0;
    int else_out3 = 0;
    if (!parse_ld_ix_from_reg(lines_[i + 67], "l", else_out2) ||
        !parse_ld_ix_from_reg(lines_[i + 68], "h", else_out3) ||
        else_out2 != out2 ||
        else_out3 != out3) {
        return false;
    }

    if (lines_[i + 69].label != join_label ||
        !lines_[i + 69].mnemonic.empty() ||
        label_has_other_control_references(lines_, join_label, i + 47)) {
        return false;
    }

    int final0 = 0;
    int final1 = 0;
    int final2 = 0;
    int final3 = 0;
    if (!is_ld_reg_from_ix(lines_[i + 70], "l", out0) ||
        !is_ld_reg_from_ix(lines_[i + 71], "h", out1) ||
        !parse_ld_ix_from_reg(lines_[i + 72], "l", final0) ||
        !parse_ld_ix_from_reg(lines_[i + 73], "h", final1) ||
        final0 != src0 ||
        final1 != src1 ||
        !is_ld_reg_from_ix(lines_[i + 74], "l", out2) ||
        !is_ld_reg_from_ix(lines_[i + 75], "h", out3) ||
        !parse_ld_ix_from_reg(lines_[i + 76], "l", final2) ||
        !parse_ld_ix_from_reg(lines_[i + 77], "h", final3) ||
        final2 != src2 ||
        final3 != src3) {
        return false;
    }

    std::vector<int> temp_offsets = {
        t0, t1, t2, t3, x0, x1, x2, x3,
        out0, out1, out2, out3, e0, e1, e2, e3
    };
    std::sort(temp_offsets.begin(), temp_offsets.end());
    temp_offsets.erase(std::unique(temp_offsets.begin(), temp_offsets.end()),
                       temp_offsets.end());

    int locals = 0;
    int temp_frame = 0;
    size_t prologue_index = 0;
    if (!current_function_frame(lines_, i, locals, temp_frame, prologue_index))
        return false;
    const size_t fn_end = function_end_after_prologue(lines_, prologue_index);
    for (int off : temp_offsets) {
        if (off == src0 || off == src1 || off == src2 || off == src3)
            return false;
        if (!ix_offset_in_temp_frame(off, locals, temp_frame))
            return false;
        for (size_t k = prologue_index; k < fn_end; ++k) {
            if (k >= i && k <= i + 77)
                continue;
            if (line_reads_ix_offset(lines_[k], off))
                return false;
        }
    }

    const std::string src0_ref = std::to_string(src0) + "(ix)";
    const std::string src1_ref = std::to_string(src1) + "(ix)";
    const std::string src2_ref = std::to_string(src2) + "(ix)";
    const std::string src3_ref = std::to_string(src3) + "(ix)";
    const int low0 = low_poly & 0xff;
    const int low1 = (low_poly >> 8) & 0xff;
    const int high0 = high_poly & 0xff;
    const int high1 = (high_poly >> 8) & 0xff;

    lines_[i] = asm_line::parse("\tld\te, " + src2_ref);
    lines_[i + 1] = asm_line::parse("\tld\td, " + src3_ref);
    lines_[i + 2] = asm_line::parse("\tld\tl, " + src0_ref);
    lines_[i + 3] = asm_line::parse("\tld\th, " + src1_ref);
    lines_[i + 4] = asm_line::parse("\tsrl\td");
    lines_[i + 5] = asm_line::parse("\trr\te");
    lines_[i + 6] = asm_line::parse("\trr\th");
    lines_[i + 7] = asm_line::parse("\trr\tl");
    lines_[i + 8] = asm_line::parse("\tjr\tnc, " + join_label);
    lines_[i + 9] = asm_line::parse("\tld\ta, l");
    lines_[i + 10] = asm_line::parse("\txor\t#" + std::to_string(low0));
    lines_[i + 11] = asm_line::parse("\tld\tl, a");
    lines_[i + 12] = asm_line::parse("\tld\ta, h");
    lines_[i + 13] = asm_line::parse("\txor\t#" + std::to_string(low1));
    lines_[i + 14] = asm_line::parse("\tld\th, a");
    lines_[i + 15] = asm_line::parse("\tld\ta, e");
    lines_[i + 16] = asm_line::parse("\txor\t#" + std::to_string(high0));
    lines_[i + 17] = asm_line::parse("\tld\te, a");
    lines_[i + 18] = asm_line::parse("\tld\ta, d");
    lines_[i + 19] = asm_line::parse("\txor\t#" + std::to_string(high1));
    lines_[i + 20] = asm_line::parse("\tld\td, a");
    lines_[i + 21] = lines_[i + 69];
    lines_[i + 22] = asm_line::parse("\tld\t" + src0_ref + ", l");
    lines_[i + 23] = asm_line::parse("\tld\t" + src1_ref + ", h");
    lines_[i + 24] = asm_line::parse("\tld\t" + src2_ref + ", e");
    lines_[i + 25] = asm_line::parse("\tld\t" + src3_ref + ", d");
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 26),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 78));
    return true;
}

bool z80_peep::rule_redundant_u8_self_mask(size_t i) {
    if (i + 2 >= lines_.size())
        return false;
    for (size_t j = i; j <= i + 2; ++j) {
        if (!lines_[j].label.empty())
            return false;
    }
    if (lines_[i].mnemonic != "ld" ||
        lines_[i + 1].mnemonic != "and" ||
        lines_[i + 2].mnemonic != "ld") {
        return false;
    }

    std::string dst;
    std::string src;
    if (!split_ld(lines_[i].operands, dst, src))
        return false;
    dst = trim(dst);
    src = trim(src);
    if (dst != "a" || !is_plain_8bit_reg(src))
        return false;
    if (!immediate_is(lines_[i + 1].operands, 255))
        return false;

    std::string dst2;
    std::string src2;
    if (!split_ld(lines_[i + 2].operands, dst2, src2))
        return false;
    if (trim(dst2) != src || trim(src2) != "a")
        return false;

    if (!a_overwritten_before_read(lines_, i + 3))
        return false;
    if (!flags_overwritten_before_read_or_escape(lines_, i + 3))
        return false;

    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 3));
    return true;
}

bool z80_peep::rule_zero_extended_byte_mask_branch(size_t i) {
    if (i + 9 >= lines_.size())
        return false;
    for (size_t j = i; j <= i + 9; ++j) {
        if (!lines_[j].label.empty())
            return false;
    }

    if (lines_[i].mnemonic != "ld" ||
        lines_[i + 1].mnemonic != "ld" ||
        lines_[i + 2].mnemonic != "ld" ||
        lines_[i + 3].mnemonic != "and" ||
        lines_[i + 4].mnemonic != "ld" ||
        lines_[i + 5].mnemonic != "ld" ||
        lines_[i + 6].mnemonic != "and" ||
        lines_[i + 7].mnemonic != "ld" ||
        lines_[i + 8].mnemonic != "or") {
        return false;
    }

    std::string dst;
    std::string src;
    if (!split_ld(lines_[i].operands, dst, src) ||
        trim(dst) != "l") {
        return false;
    }
    const std::string test_src = lower_copy(trim(src));
    if (!bit_operand_supported(test_src))
        return false;

    if (!split_ld(lines_[i + 1].operands, dst, src) ||
        trim(dst) != "h" || !immediate_is(src, 0)) {
        return false;
    }
    if (!split_ld(lines_[i + 2].operands, dst, src) ||
        trim(dst) != "a" || trim(src) != "l") {
        return false;
    }

    int mask = 0;
    if (!parse_immediate_value(lines_[i + 3].operands, mask))
        return false;
    const int bit = single_u8_bit_index(mask);
    if (bit < 0)
        return false;

    if (!split_ld(lines_[i + 4].operands, dst, src) ||
        trim(dst) != "l" || trim(src) != "a") {
        return false;
    }
    if (!split_ld(lines_[i + 5].operands, dst, src) ||
        trim(dst) != "a" || trim(src) != "h") {
        return false;
    }
    if (!immediate_is(lines_[i + 6].operands, 0))
        return false;
    if (!split_ld(lines_[i + 7].operands, dst, src) ||
        trim(dst) != "h" || trim(src) != "a") {
        return false;
    }
    if (trim(lines_[i + 8].operands) != "a, l" &&
        trim(lines_[i + 8].operands) != "a,l" &&
        trim(lines_[i + 8].operands) != "l") {
        return false;
    }

    std::string cc;
    std::string target;
    if (!split_conditional_branch_target(lines_[i + 9], cc, target))
        return false;
    if (cc != "z" && cc != "nz")
        return false;

    const size_t target_idx = find_label_index(lines_, target);
    if (target_idx == lines_.size())
        return false;
    const size_t fallthrough = i + 10;
    if (!a_overwritten_before_read(lines_, fallthrough) ||
        !a_overwritten_before_read(lines_, target_idx) ||
        !path_overwrites_hl_before_read(lines_, fallthrough) ||
        !path_overwrites_hl_before_read(lines_, target_idx) ||
        !flags_overwritten_before_read_or_escape(lines_, fallthrough) ||
        !flags_overwritten_before_read_or_escape(lines_, target_idx)) {
        return false;
    }

    lines_[i] = asm_line::parse("\tbit\t" + std::to_string(bit) + ", " + test_src);
    lines_[i + 1] = lines_[i + 9];
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 2),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 10));
    return true;
}

bool z80_peep::rule_zero_extended_byte_store_direct(size_t i) {
    if (i + 5 >= lines_.size())
        return false;

    auto no_labels = [&](size_t end_inclusive) {
        if (end_inclusive >= lines_.size())
            return false;
        for (size_t k = i; k <= end_inclusive; ++k) {
            if (!lines_[k].label.empty())
                return false;
        }
        return true;
    };

    auto is_add_a_a = [&](const asm_line &line) {
        if (line.mnemonic != "add")
            return false;
        std::string dst;
        std::string src;
        return split_ld(line.operands, dst, src) &&
               trim(dst) == "a" && trim(src) == "a";
    };

    auto parse_adjacent_indexed_stores = [&](size_t lo_idx,
                                             size_t hi_idx,
                                             int &lo_off,
                                             int &hi_off) {
        std::string dst;
        std::string src;
        if (lines_[lo_idx].mnemonic != "ld" ||
            !split_ld(lines_[lo_idx].operands, dst, src) ||
            trim(src) != "l" ||
            !parse_ix_ref(trim(dst), lo_off)) {
            return false;
        }
        if (lines_[hi_idx].mnemonic != "ld" ||
            !split_ld(lines_[hi_idx].operands, dst, src) ||
            trim(src) != "h" ||
            !parse_ix_ref(trim(dst), hi_off)) {
            return false;
        }
        return hi_off == lo_off + 1;
    };

    auto common_prefix_matches = [&]() {
        std::string dst;
        std::string src;
        if (lines_[i].mnemonic != "ld" ||
            !split_ld(lines_[i].operands, dst, src) ||
            trim(dst) != "a") {
            return false;
        }
        if (!is_add_a_a(lines_[i + 1]))
            return false;
        if (lines_[i + 2].mnemonic != "ld" ||
            !split_ld(lines_[i + 2].operands, dst, src) ||
            trim(dst) != "l" || trim(src) != "a") {
            return false;
        }
        return lines_[i + 3].mnemonic == "ld" &&
               split_ld(lines_[i + 3].operands, dst, src) &&
               trim(dst) == "h" && immediate_is(src, 0);
    };

    if (!common_prefix_matches())
        return false;

    int lo_off = 0;
    int hi_off = 0;

    if (i + 11 < lines_.size() && no_labels(i + 11)) {
        std::string dst;
        std::string src;
        int xor_value = 0;
        if (lines_[i + 4].mnemonic == "ld" &&
            split_ld(lines_[i + 4].operands, dst, src) &&
            trim(dst) == "a" && trim(src) == "l" &&
            lines_[i + 5].mnemonic == "xor" &&
            parse_immediate_value(lines_[i + 5].operands, xor_value) &&
            lines_[i + 6].mnemonic == "ld" &&
            split_ld(lines_[i + 6].operands, dst, src) &&
            trim(dst) == "l" && trim(src) == "a" &&
            lines_[i + 7].mnemonic == "ld" &&
            split_ld(lines_[i + 7].operands, dst, src) &&
            trim(dst) == "a" && trim(src) == "h" &&
            lines_[i + 8].mnemonic == "xor" &&
            immediate_is(lines_[i + 8].operands, 0) &&
            lines_[i + 9].mnemonic == "ld" &&
            split_ld(lines_[i + 9].operands, dst, src) &&
            trim(dst) == "h" && trim(src) == "a" &&
            parse_adjacent_indexed_stores(i + 10, i + 11, lo_off, hi_off) &&
            a_overwritten_before_read(lines_, i + 12) &&
            path_overwrites_hl_before_read(lines_, i + 12) &&
            flags_overwritten_before_read_or_escape(lines_, i + 12)) {
            lines_[i + 2] = asm_line::parse("\txor\t" + imm8_text(xor_value));
            lines_[i + 3] = asm_line::parse(
                "\tld\t" + std::to_string(lo_off) + "(ix), a");
            lines_[i + 4] = asm_line::parse(
                "\tld\t" + std::to_string(hi_off) + "(ix), #0");
            lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 5),
                         lines_.begin() + static_cast<std::ptrdiff_t>(i + 12));
            return true;
        }
    }

    if (!no_labels(i + 5))
        return false;
    if (!parse_adjacent_indexed_stores(i + 4, i + 5, lo_off, hi_off))
        return false;
    if (!path_overwrites_hl_before_read(lines_, i + 6) ||
        !flags_overwritten_before_read_or_escape(lines_, i + 6)) {
        return false;
    }

    lines_[i + 2] = asm_line::parse(
        "\tld\t" + std::to_string(lo_off) + "(ix), a");
    lines_[i + 3] = asm_line::parse(
        "\tld\t" + std::to_string(hi_off) + "(ix), #0");
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 4),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 6));
    return true;
}

bool z80_peep::rule_zero_extended_mask_inc_word_store(size_t i) {
    if (i + 7 >= lines_.size())
        return false;
    for (size_t k = i; k <= i + 7; ++k) {
        if (!lines_[k].label.empty())
            return false;
    }

    if (lines_[i].mnemonic != "ld" ||
        lines_[i + 1].mnemonic != "ld" ||
        lines_[i + 2].mnemonic != "ld" ||
        lines_[i + 3].mnemonic != "and" ||
        lines_[i + 4].mnemonic != "ld" ||
        lines_[i + 5].mnemonic != "inc" ||
        lines_[i + 6].mnemonic != "ld" ||
        lines_[i + 7].mnemonic != "ld") {
        return false;
    }

    std::string dst;
    std::string src;
    if (!split_ld(lines_[i].operands, dst, src) ||
        trim(dst) != "l") {
        return false;
    }
    const std::string value_src = trim(src);
    if (!is_plain_8bit_reg(value_src) && !is_immediate_operand(value_src) &&
        !is_numeric_literal(value_src)) {
        int ignored = 0;
        if (!parse_ixiy_ref(value_src, ignored) && value_src != "(hl)")
            return false;
    }

    if (!split_ld(lines_[i + 1].operands, dst, src) ||
        trim(dst) != "h" || !immediate_is(src, 0)) {
        return false;
    }
    if (!split_ld(lines_[i + 2].operands, dst, src) ||
        trim(dst) != "a" || trim(src) != "l") {
        return false;
    }

    int mask = 0;
    if (!parse_immediate_value(lines_[i + 3].operands, mask))
        return false;
    mask = u8_value(mask);
    if (mask == 0xff)
        return false;

    if (!split_ld(lines_[i + 4].operands, dst, src) ||
        trim(dst) != "l" || trim(src) != "a") {
        return false;
    }
    if (trim(lines_[i + 5].operands) != "hl")
        return false;

    std::string lo_slot;
    std::string hi_slot;
    int lo_off = 0;
    int hi_off = 0;
    if (!split_ld(lines_[i + 6].operands, dst, src) ||
        trim(src) != "l" ||
        !parse_ix_ref(trim(dst), lo_off)) {
        return false;
    }
    lo_slot = trim(dst);
    if (!split_ld(lines_[i + 7].operands, dst, src) ||
        trim(src) != "h" ||
        !parse_ix_ref(trim(dst), hi_off) ||
        hi_off != lo_off + 1) {
        return false;
    }
    hi_slot = trim(dst);

    const size_t after = i + 8;
    const bool preserve_hl = !path_overwrites_hl_before_read(lines_, after);
    if (!a_overwritten_before_read(lines_, after) ||
        !flags_overwritten_before_read_or_escape(lines_, after)) {
        return false;
    }

    size_t out = i;
    if (value_src != "a") {
        lines_[out++] = asm_line::parse("\tld\ta, " + value_src);
    }
    lines_[out++] = asm_line::parse("\tand\t" + imm8_text(mask));
    lines_[out++] = asm_line::parse("\tinc\ta");
    if (preserve_hl) {
        lines_[out++] = asm_line::parse("\tld\tl, a");
        lines_[out++] = asm_line::parse("\tld\th, #0");
        lines_[out++] = asm_line::parse("\tld\t" + lo_slot + ", l");
        lines_[out++] = asm_line::parse("\tld\t" + hi_slot + ", h");
    } else {
        lines_[out++] = asm_line::parse("\tld\t" + lo_slot + ", a");
        lines_[out++] = asm_line::parse("\tld\t" + hi_slot + ", #0");
    }

    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(out),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 8));
    return true;
}

bool z80_peep::rule_word_reload_low_byte_to_a(size_t i) {
    if (i + 2 >= lines_.size())
        return false;
    for (size_t k = i; k <= i + 2; ++k) {
        if (!lines_[k].label.empty())
            return false;
    }

    if (lines_[i].mnemonic != "ld" ||
        lines_[i + 1].mnemonic != "ld" ||
        lines_[i + 2].mnemonic != "ld") {
        return false;
    }

    std::string lo_dst;
    std::string lo_src;
    std::string hi_dst;
    std::string hi_src;
    std::string a_dst;
    std::string a_src;
    if (!split_ld(lines_[i].operands, lo_dst, lo_src) ||
        !split_ld(lines_[i + 1].operands, hi_dst, hi_src) ||
        !split_ld(lines_[i + 2].operands, a_dst, a_src)) {
        return false;
    }

    lo_dst = trim(lo_dst);
    lo_src = trim(lo_src);
    hi_dst = trim(hi_dst);
    hi_src = trim(hi_src);
    a_dst = trim(a_dst);
    a_src = trim(a_src);
    if (lo_dst != "l" || hi_dst != "h" || a_dst != "a" || a_src != "l")
        return false;

    int lo_off = 0;
    int hi_off = 0;
    if (parse_ix_ref(lo_src, lo_off)) {
        if (!parse_ix_ref(hi_src, hi_off))
            return false;
    } else if (parse_iy_ref(lo_src, lo_off)) {
        if (!parse_iy_ref(hi_src, hi_off))
            return false;
    } else {
        return false;
    }
    if (hi_off != lo_off + 1)
        return false;
    if (!path_overwrites_hl_before_read(lines_, i + 3))
        return false;

    lines_[i] = asm_line::parse("\tld\ta, " + lo_src);
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 1),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 3));
    return true;
}

bool z80_peep::rule_dead_hl_zero_extend_before_pair_load(size_t i) {
    if (i + 3 >= lines_.size())
        return false;
    for (size_t j = i; j <= i + 3; ++j) {
        if (!lines_[j].label.empty())
            return false;
    }
    if (lines_[i].mnemonic != "ld" ||
        lines_[i + 1].mnemonic != "ld" ||
        lines_[i + 2].mnemonic != "ld" ||
        lines_[i + 3].mnemonic != "ld") {
        return false;
    }

    std::string d0, s0, d1, s1, d2, s2, d3, s3;
    if (!split_ld(lines_[i].operands, d0, s0) ||
        !split_ld(lines_[i + 1].operands, d1, s1) ||
        !split_ld(lines_[i + 2].operands, d2, s2) ||
        !split_ld(lines_[i + 3].operands, d3, s3)) {
        return false;
    }
    d0 = trim(d0); s0 = trim(s0);
    d1 = trim(d1); s1 = trim(s1);
    d2 = trim(d2); s2 = trim(s2);
    d3 = trim(d3); s3 = trim(s3);

    if (d0 != "l" || d1 != "h" || !immediate_is(s1, 0))
        return false;
    if (d2 != "l" || d3 != "h")
        return false;

    // The dead low-byte load must be side-effect-free.  Keep this deliberately
    // narrow; indexed-memory forms can be handled by the existing liveness
    // rules that already model those loads.
    if (!is_plain_8bit_reg(s0) && !is_immediate_operand(s0) &&
        !is_numeric_literal(s0)) {
        return false;
    }

    auto reads_old_hl = [](const std::string &operand) {
        return operand_has_token(operand, "hl") ||
               operand_has_token(operand, "h") ||
               operand_has_token(operand, "l");
    };
    if (reads_old_hl(s2) || reads_old_hl(s3))
        return false;

    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 2));
    return true;
}

// ld a,#0  →  xor a
// xor a sets Z/S/P/H/N/C; ld a,0 does not change flags.
// Safe only when the next instruction does not rely on pre-existing flags
// (i.e. is not a conditional branch).
bool z80_peep::rule_ld_a_zero(size_t i) {
    auto &a = lines_[i];
    if (a.mnemonic != "ld") return false;
    std::string dst, src;
    if (!split_ld(a.operands, dst, src)) return false;
    if (dst != "a") return false;
    if (src != "#0" && src != "0") return false;

    // Don't fire if the next non-empty line is a conditional branch or
    // directly consumes carry/borrow. `ld a,#0` preserves flags; `xor a`
    // clears carry, so `adc`/`sbc` would change meaning.
    for (size_t j = i + 1; j < lines_.size(); ++j) {
        if (lines_[j].mnemonic.empty()) continue;
        if (is_conditional_branch(lines_[j])) return false;
        if (lines_[j].mnemonic == "adc" || lines_[j].mnemonic == "sbc")
            return false;
        break;
    }

    a.mnemonic = "xor";
    a.operands = "a";
    return true;
}

bool z80_peep::rule_hl_immediate_store_direct(size_t i) {
    if (i + 1 >= lines_.size())
        return false;
    if (!lines_[i].label.empty() || !lines_[i + 1].label.empty())
        return false;

    std::string dst;
    std::string src;
    if (lines_[i].mnemonic != "ld" ||
        !split_ld(lines_[i].operands, dst, src) ||
        trim(dst) != "a" ||
        (!is_immediate_operand(trim(src)) && !is_numeric_literal(trim(src)))) {
        return false;
    }
    if (lines_[i + 1].mnemonic != "ld" ||
        !split_ld(lines_[i + 1].operands, dst, src) ||
        trim(dst) != "(hl)" || trim(src) != "a") {
        return false;
    }
    if (!a_overwritten_before_read(lines_, i + 2))
        return false;

    std::string imm_dst;
    std::string imm_src;
    split_ld(lines_[i].operands, imm_dst, imm_src);
    lines_[i].operands = "(hl), " + trim(imm_src);
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 1));
    return true;
}

bool z80_peep::rule_dead_a_indexed_load_shuttle(size_t i) {
    if (i + 1 >= lines_.size())
        return false;
    if (!lines_[i].label.empty() || !lines_[i + 1].label.empty())
        return false;

    if (lines_[i].mnemonic != "ld" || lines_[i + 1].mnemonic != "ld")
        return false;

    std::string a_dst;
    std::string a_src;
    if (!split_ld(lines_[i].operands, a_dst, a_src) ||
        trim(a_dst) != "a") {
        return false;
    }

    a_src = trim(a_src);
    int ignored_offset = 0;
    if (!parse_ixiy_ref(a_src, ignored_offset))
        return false;

    std::string dst;
    std::string src;
    if (!split_ld(lines_[i + 1].operands, dst, src) ||
        trim(src) != "a") {
        return false;
    }

    dst = trim(dst);
    if (!is_plain_8bit_reg(dst) || dst == "a")
        return false;

    if (!a_overwritten_before_read(lines_, i + 2))
        return false;

    lines_[i].operands = dst + ", " + a_src;
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 1));
    return true;
}

bool z80_peep::rule_dead_a_indexed_immediate_store(size_t i) {
    if (i + 1 >= lines_.size())
        return false;
    if (!lines_[i].label.empty() || !lines_[i + 1].label.empty())
        return false;

    if (lines_[i].mnemonic != "ld" || lines_[i + 1].mnemonic != "ld")
        return false;

    std::string dst;
    std::string imm_src;
    if (!split_ld(lines_[i].operands, dst, imm_src) ||
        trim(dst) != "a") {
        return false;
    }
    imm_src = trim(imm_src);
    if (!is_immediate_operand(imm_src) && !is_numeric_literal(imm_src))
        return false;

    std::string store_dst;
    std::string store_src;
    if (!split_ld(lines_[i + 1].operands, store_dst, store_src) ||
        trim(store_src) != "a") {
        return false;
    }
    store_dst = trim(store_dst);

    int ignored_offset = 0;
    if (!parse_ixiy_ref(store_dst, ignored_offset))
        return false;

    if (!a_overwritten_before_read(lines_, i + 2))
        return false;

    lines_[i].operands = store_dst + ", " + imm_src;
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 1));
    return true;
}

bool z80_peep::rule_a_temp_reload_after_preserving_branch(size_t i) {
    if (i + 3 >= lines_.size())
        return false;
    if (!lines_[i].label.empty() || lines_[i].mnemonic != "ld")
        return false;

    std::string dst;
    std::string src;
    int temp_off = 0;
    if (!split_ld(lines_[i].operands, dst, src) ||
        trim(src) != "a" ||
        !parse_ix_ref(trim(dst), temp_off)) {
        return false;
    }

    int locals = 0;
    int temp_frame = 0;
    size_t prologue_index = 0;
    const bool have_frame =
        current_function_frame(lines_, i, locals, temp_frame, prologue_index);
    const size_t function_end = have_frame
        ? function_end_after_prologue(lines_, prologue_index)
        : lines_.size();

    auto is_reload_of_temp = [&](size_t idx) {
        if (idx >= lines_.size() || lines_[idx].mnemonic != "ld")
            return false;
        std::string reload_dst;
        std::string reload_src;
        int reload_off = 0;
        return split_ld(lines_[idx].operands, reload_dst, reload_src) &&
               trim(reload_dst) == "a" &&
               parse_ix_ref(trim(reload_src), reload_off) &&
               reload_off == temp_off;
    };

    auto erase_reload_and_maybe_store = [&](size_t reload_idx) {
        const bool temp_frame_store =
            have_frame && ix_offset_in_temp_frame(temp_off, locals, temp_frame);
        const bool store_is_dead =
            temp_frame_store &&
            ix_slot_not_read_before_rewrite(lines_, reload_idx + 1,
                                            function_end, temp_off);
        lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(reload_idx));
        if (store_is_dead) {
            lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i));
        }
        return true;
    };

    auto first_instruction_after_labels = [&](size_t idx,
                                             const std::string &allowed_label,
                                             size_t allowed_ref,
                                             size_t &out_idx) {
        for (; idx < lines_.size(); ++idx) {
            const asm_line &line = lines_[idx];
            if (line.label.empty()) {
                if (line.mnemonic.empty())
                    continue;
                out_idx = idx;
                return true;
            }

            const bool allowed =
                !allowed_label.empty() && line.label == allowed_label &&
                !label_has_other_control_references(lines_, line.label,
                                                    allowed_ref);
            const bool unreferenced =
                label_has_other_control_references(lines_, line.label,
                                                   lines_.size()) == false;
            if (!allowed && !unreferenced)
                return false;

            if (!line.mnemonic.empty()) {
                out_idx = idx;
                return true;
            }
        }
        return false;
    };

    const size_t end = std::min(lines_.size(), i + 32);
    for (size_t k = i + 1; k < end; ++k) {
        const asm_line &line = lines_[k];
        if (!line.label.empty() &&
            label_has_other_control_references(lines_, line.label,
                                               lines_.size())) {
            return false;
        }
        if (is_reload_of_temp(k)) {
            return erase_reload_and_maybe_store(k);
        }

        std::string cc;
        std::string target;
        if (split_conditional_branch_target(line, cc, target)) {
            const size_t target_idx = find_label_index(lines_, target);
            // A backward edge can execute arbitrary code (including calls)
            // before returning to this store. The local A-preservation scan
            // cannot prove that path, so keep both the spill and reload.
            if (target_idx != lines_.size() && target_idx <= i)
                return false;

            size_t reload_idx = lines_.size();
            if (first_instruction_after_labels(k + 1, "", lines_.size(),
                                               reload_idx) &&
                is_reload_of_temp(reload_idx)) {
                return erase_reload_and_maybe_store(reload_idx);
            }

            if (target_idx > k && target_idx < end &&
                !label_has_other_control_references(lines_, target, k)) {
                const size_t prev_idx =
                    previous_instruction_index(lines_, target_idx);
                const bool no_fallthrough =
                    target_idx == k + 1 ||
                    (prev_idx != lines_.size() &&
                     is_unconditional_escape(lines_[prev_idx]));
                if (no_fallthrough &&
                    first_instruction_after_labels(target_idx, target, k,
                                                   reload_idx) &&
                    is_reload_of_temp(reload_idx)) {
                    return erase_reload_and_maybe_store(reload_idx);
                }
            }
        }

        std::string jump_target;
        const bool unconditional =
            parse_unconditional_jump(line, jump_target) ||
            (line.mnemonic == "djnz" &&
             !(jump_target = trim(line.operands)).empty());
        if (unconditional) {
            const size_t target_idx = find_label_index(lines_, jump_target);
            if (target_idx != lines_.size() && target_idx <= i)
                return false;
        }

        if (line_writes_ix_offset(line, temp_off) || !line_preserves_a_value(line))
            return false;
    }

    return false;
}

bool z80_peep::rule_dead_de_spill_reload_exchange(size_t i) {
    if (i + 4 >= lines_.size())
        return false;
    for (size_t j = i; j <= i + 4; ++j)
        if (!lines_[j].label.empty())
            return false;

    std::string dst;
    std::string src;
    int lo_off = 0;
    int hi_off = 0;
    if (lines_[i].mnemonic != "ld" ||
        !split_ld(lines_[i].operands, dst, src) ||
        trim(src) != "e" ||
        !parse_ix_ref(trim(dst), lo_off)) {
        return false;
    }
    if (lines_[i + 1].mnemonic != "ld" ||
        !split_ld(lines_[i + 1].operands, dst, src) ||
        trim(src) != "d" ||
        !parse_ix_ref(trim(dst), hi_off) ||
        hi_off != lo_off + 1) {
        return false;
    }
    int reload_lo_off = 0;
    if (lines_[i + 2].mnemonic != "ld" ||
        !split_ld(lines_[i + 2].operands, dst, src) ||
        trim(dst) != "l" ||
        !parse_ix_ref(trim(src), reload_lo_off) ||
        reload_lo_off != lo_off) {
        return false;
    }
    int reload_hi_off = 0;
    if (lines_[i + 3].mnemonic != "ld" ||
        !split_ld(lines_[i + 3].operands, dst, src) ||
        trim(dst) != "h" ||
        !parse_ix_ref(trim(src), reload_hi_off) ||
        reload_hi_off != hi_off) {
        return false;
    }
    if (!is_ex_de_hl(lines_[i + 4]))
        return false;

    if (!hl_dead_before_read_or_modern_return(lines_, i + 5))
        return false;

    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 2),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 5));
    return true;
}

bool z80_peep::rule_call_result_byte_commute_direct(size_t i) {
    if (i + 4 >= lines_.size() || lines_[i].mnemonic != "call")
        return false;
    for (size_t k = i + 1; k <= i + 4; ++k) {
        if (!lines_[k].label.empty())
            return false;
    }

    std::string dst;
    std::string src;
    int spill_off = 0;
    if (lines_[i + 1].mnemonic != "ld" ||
        !split_ld(lines_[i + 1].operands, dst, src) ||
        trim(src) != "a" || !parse_ix_ref(trim(dst), spill_off)) {
        return false;
    }

    int accumulator_off = 0;
    if (lines_[i + 2].mnemonic != "ld" ||
        !split_ld(lines_[i + 2].operands, dst, src) ||
        trim(dst) != "a" ||
        !parse_ix_ref(trim(src), accumulator_off) ||
        accumulator_off == spill_off) {
        return false;
    }

    const std::string alu = lines_[i + 3].mnemonic;
    if (alu != "add" && alu != "and" && alu != "or" && alu != "xor")
        return false;
    std::string alu_dst;
    std::string alu_src;
    if (split_ld(lines_[i + 3].operands, alu_dst, alu_src)) {
        if (trim(alu_dst) != "a")
            return false;
        alu_src = trim(alu_src);
    } else {
        alu_src = trim(lines_[i + 3].operands);
    }
    int alu_off = 0;
    if (!parse_ix_ref(alu_src, alu_off) || alu_off != spill_off)
        return false;

    int store_off = 0;
    if (lines_[i + 4].mnemonic != "ld" ||
        !split_ld(lines_[i + 4].operands, dst, src) ||
        trim(src) != "a" || !parse_ix_ref(trim(dst), store_off) ||
        store_off != accumulator_off) {
        return false;
    }

    int locals = 0;
    int temp_frame = 0;
    size_t prologue_index = 0;
    if (!current_function_frame(lines_, i, locals, temp_frame,
                                prologue_index) ||
        !ix_offset_in_temp_frame(spill_off, locals, temp_frame)) {
        return false;
    }
    const size_t fn_end = function_end_after_prologue(lines_, prologue_index);
    if (ix_value_may_be_read_before_rewrite(
            lines_, prologue_index, fn_end, i + 5, spill_off))
        return false;

    const std::string accumulator_ref =
        std::to_string(accumulator_off) + "(ix)";
    lines_[i + 1] = asm_line::parse(
        "\t" + alu + "\ta, " + accumulator_ref);
    lines_[i + 2] = asm_line::parse(
        "\tld\t" + accumulator_ref + ", a");
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 3),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 5));
    return true;
}

bool z80_peep::rule_hl_to_de_before_hl_reload_exchange(size_t i) {
    if (i + 2 >= lines_.size())
        return false;
    if (!lines_[i].label.empty() || !lines_[i + 1].label.empty())
        return false;
    if (lines_[i].mnemonic != "ld" || lines_[i + 1].mnemonic != "ld")
        return false;

    std::string d0, s0, d1, s1;
    if (!split_ld(lines_[i].operands, d0, s0) ||
        !split_ld(lines_[i + 1].operands, d1, s1)) {
        return false;
    }
    d0 = trim(d0); s0 = trim(s0);
    d1 = trim(d1); s1 = trim(s1);
    const bool high_low = d0 == "d" && s0 == "h" &&
                          d1 == "e" && s1 == "l";
    const bool low_high = d0 == "e" && s0 == "l" &&
                          d1 == "d" && s1 == "h";
    if (!high_low && !low_high)
        return false;

    if (!overwrites_hl_without_reading_it(lines_, i + 2))
        return false;

    lines_[i] = asm_line::parse("\tex\tde, hl");
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 1));
    return true;
}

bool z80_peep::rule_pair_copy_offset_materialize(size_t i) {
    if (i + 4 >= lines_.size())
        return false;
    if (!lines_[i].label.empty() || !lines_[i + 1].label.empty())
        return false;
    if (lines_[i].mnemonic != "ld" || lines_[i + 1].mnemonic != "ld")
        return false;

    std::string d0;
    std::string s0;
    std::string d1;
    std::string s1;
    if (!split_ld(lines_[i].operands, d0, s0) ||
        !split_ld(lines_[i + 1].operands, d1, s1)) {
        return false;
    }
    d0 = trim(d0); s0 = trim(s0);
    d1 = trim(d1); s1 = trim(s1);

    std::string pair;
    if ((d0 == "h" && s0 == "b" && d1 == "l" && s1 == "c") ||
        (d0 == "l" && s0 == "c" && d1 == "h" && s1 == "b")) {
        pair = "bc";
    } else if ((d0 == "h" && s0 == "d" && d1 == "l" && s1 == "e") ||
               (d0 == "l" && s0 == "e" && d1 == "h" && s1 == "d")) {
        pair = "de";
    } else {
        return false;
    }

    size_t k = i + 2;
    int offset = 0;
    std::string direction;
    for (; k < lines_.size(); ++k) {
        if (!lines_[k].label.empty())
            return false;
        if (lines_[k].mnemonic != "inc" && lines_[k].mnemonic != "dec")
            break;
        if (trim(lines_[k].operands) != "hl")
            return false;
        if (direction.empty())
            direction = lines_[k].mnemonic;
        if (lines_[k].mnemonic != direction)
            return false;
        offset += lines_[k].mnemonic == "inc" ? 1 : -1;
    }

    const int steps = std::abs(offset);
    if (steps < 3)
        return false;
    if (!flags_overwritten_before_read_or_escape(lines_, k))
        return false;

    lines_[i] = asm_line::parse("\tld\thl, " + imm16_text(offset));
    lines_[i + 1] = asm_line::parse("\tadd\thl, " + pair);
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 2),
                 lines_.begin() + static_cast<std::ptrdiff_t>(k));
    return true;
}

bool z80_peep::rule_pair_immediate_copy_reload_elide(size_t i) {
    if (i + 3 >= lines_.size())
        return false;

    struct pair_desc {
        const char *pair;
        const char *hi;
        const char *lo;
    };
    static const pair_desc pairs[] = {
        {"bc", "b", "c"},
        {"de", "d", "e"},
        {"hl", "h", "l"},
    };

    auto find_pair = [&](const std::string &name) -> const pair_desc * {
        for (const auto &pair : pairs) {
            if (name == pair.pair)
                return &pair;
        }
        return nullptr;
    };

    auto ld_pair_immediate = [&](size_t idx,
                                 const pair_desc *&pair,
                                 std::string &imm) {
        if (idx >= lines_.size() || !lines_[idx].label.empty() ||
            lines_[idx].mnemonic != "ld") {
            return false;
        }
        std::string dst, src;
        if (!split_ld(lines_[idx].operands, dst, src))
            return false;
        dst = trim(dst);
        src = trim(src);
        pair = find_pair(dst);
        if (pair == nullptr)
            return false;
        if (!is_immediate_operand(src) && !is_numeric_literal(src))
            return false;
        imm = src;
        return true;
    };

    auto match_pair_copy = [&](size_t idx,
                               const pair_desc *src_pair,
                               const pair_desc *dst_pair) {
        if (idx + 1 >= lines_.size() || src_pair == nullptr ||
            dst_pair == nullptr) {
            return false;
        }
        if (!lines_[idx].label.empty() || !lines_[idx + 1].label.empty())
            return false;
        if (lines_[idx].mnemonic != "ld" || lines_[idx + 1].mnemonic != "ld")
            return false;

        std::string d0, s0, d1, s1;
        if (!split_ld(lines_[idx].operands, d0, s0) ||
            !split_ld(lines_[idx + 1].operands, d1, s1)) {
            return false;
        }
        d0 = trim(d0); s0 = trim(s0);
        d1 = trim(d1); s1 = trim(s1);

        const bool high_low =
            d0 == dst_pair->hi && s0 == src_pair->hi &&
            d1 == dst_pair->lo && s1 == src_pair->lo;
        const bool low_high =
            d0 == dst_pair->lo && s0 == src_pair->lo &&
            d1 == dst_pair->hi && s1 == src_pair->hi;
        return high_low || low_high;
    };

    const pair_desc *src_pair = nullptr;
    std::string first_imm;
    if (!ld_pair_immediate(i, src_pair, first_imm))
        return false;

    // Temp form seen after lowering symbolic address arithmetic:
    //   ld rr,#A; ld b,rr_hi; ld c,rr_lo; ld rr,#B; ld dd_hi,b; ld dd_lo,c
    // If BC is not observed afterwards, materialize DD directly.
    if (i + 5 < lines_.size()) {
        const pair_desc *bc_pair = find_pair("bc");
        if (match_pair_copy(i + 1, src_pair, bc_pair)) {
            const pair_desc *reload_pair = nullptr;
            std::string reload_imm;
            if (ld_pair_immediate(i + 3, reload_pair, reload_imm) &&
                reload_pair == src_pair) {
                for (const auto &dst_candidate : pairs) {
                    if (dst_candidate.pair == src_pair->pair ||
                        dst_candidate.pair == bc_pair->pair) {
                        continue;
                    }
                    if (!match_pair_copy(i + 4, bc_pair, &dst_candidate))
                        continue;
                    if (!bc_overwritten_or_call_before_read_allowing_unrelated(
                            lines_, i + 6)) {
                        continue;
                    }

                    lines_[i] = asm_line::parse(
                        "\tld\t" + std::string(dst_candidate.pair) + ", " +
                        first_imm);
                    lines_[i + 1] = lines_[i + 3];
                    lines_.erase(
                        lines_.begin() + static_cast<std::ptrdiff_t>(i + 2),
                        lines_.begin() + static_cast<std::ptrdiff_t>(i + 6));
                    return true;
                }
            }
        }
    }

    // Direct form:
    //   ld rr,#A; ld dd_hi,rr_hi; ld dd_lo,rr_lo; ld rr,#B
    for (const auto &dst_candidate : pairs) {
        if (dst_candidate.pair == src_pair->pair)
            continue;
        if (!match_pair_copy(i + 1, src_pair, &dst_candidate))
            continue;

        const pair_desc *reload_pair = nullptr;
        std::string reload_imm;
        if (!ld_pair_immediate(i + 3, reload_pair, reload_imm) ||
            reload_pair != src_pair) {
            continue;
        }

        lines_[i] = asm_line::parse(
            "\tld\t" + std::string(dst_candidate.pair) + ", " + first_imm);
        lines_[i + 1] = lines_[i + 3];
        lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 2),
                     lines_.begin() + static_cast<std::ptrdiff_t>(i + 4));
        return true;
    }

    return false;
}

bool z80_peep::rule_temp_base_subtract_result_exchange(size_t i) {
    if (i + 10 >= lines_.size())
        return false;
    for (size_t j = i; j <= i + 10; ++j) {
        if (!lines_[j].label.empty())
            return false;
    }

    std::string dst, src;
    std::string base_imm;
    std::string limit_imm;
    if (lines_[i].mnemonic != "ld" ||
        !split_ld(lines_[i].operands, dst, src) ||
        trim(dst) != "hl") {
        return false;
    }
    base_imm = trim(src);
    if (!is_immediate_operand(base_imm) && !is_numeric_literal(base_imm))
        return false;

    int lo_off = 0;
    int hi_off = 0;
    if (lines_[i + 1].mnemonic != "ld" ||
        !split_ld(lines_[i + 1].operands, dst, src) ||
        !parse_ix_ref(trim(dst), lo_off) ||
        trim(src) != "l") {
        return false;
    }
    if (lines_[i + 2].mnemonic != "ld" ||
        !split_ld(lines_[i + 2].operands, dst, src) ||
        !parse_ix_ref(trim(dst), hi_off) ||
        hi_off != lo_off + 1 ||
        trim(src) != "h") {
        return false;
    }

    if (lines_[i + 3].mnemonic != "ld" ||
        !split_ld(lines_[i + 3].operands, dst, src) ||
        trim(dst) != "hl") {
        return false;
    }
    limit_imm = trim(src);
    if (!is_immediate_operand(limit_imm) && !is_numeric_literal(limit_imm))
        return false;

    if (lines_[i + 4].mnemonic != "ld" ||
        !split_ld(lines_[i + 4].operands, dst, src) ||
        trim(dst) != "de" ||
        trim(src) != base_imm) {
        return false;
    }
    if (!is_or_a_self(lines_[i + 5]))
        return false;
    if (lines_[i + 6].mnemonic != "sbc" ||
        !split_ld(lines_[i + 6].operands, dst, src) ||
        trim(dst) != "hl" || trim(src) != "de") {
        return false;
    }

    std::string d_dst, d_src, e_dst, e_src;
    if (lines_[i + 7].mnemonic != "ld" ||
        lines_[i + 8].mnemonic != "ld" ||
        !split_ld(lines_[i + 7].operands, d_dst, d_src) ||
        !split_ld(lines_[i + 8].operands, e_dst, e_src) ||
        trim(d_dst) != "d" || trim(d_src) != "h" ||
        trim(e_dst) != "e" || trim(e_src) != "l") {
        return false;
    }

    int reload_lo = 0;
    int reload_hi = 0;
    if (lines_[i + 9].mnemonic != "ld" ||
        !split_ld(lines_[i + 9].operands, dst, src) ||
        trim(dst) != "l" ||
        !parse_ix_ref(trim(src), reload_lo) ||
        reload_lo != lo_off) {
        return false;
    }
    if (lines_[i + 10].mnemonic != "ld" ||
        !split_ld(lines_[i + 10].operands, dst, src) ||
        trim(dst) != "h" ||
        !parse_ix_ref(trim(src), reload_hi) ||
        reload_hi != hi_off) {
        return false;
    }

    int locals = 0;
    int temp_frame = 0;
    size_t prologue_index = 0;
    if (!current_function_frame(lines_, i, locals, temp_frame, prologue_index))
        return false;
    if (!ix_offset_in_temp_frame(lo_off, locals, temp_frame) ||
        !ix_offset_in_temp_frame(hi_off, locals, temp_frame)) {
        return false;
    }

    const size_t function_end = function_end_after_prologue(lines_,
                                                            prologue_index);
    for (size_t k = i + 11; k < function_end; ++k) {
        if (line_reads_ix_offset(lines_[k], lo_off) ||
            line_reads_ix_offset(lines_[k], hi_off)) {
            return false;
        }
    }

    lines_[i] = asm_line::parse("\tld\thl, " + limit_imm);
    lines_[i + 1] = asm_line::parse("\tld\tde, " + base_imm);
    lines_[i + 2] = lines_[i + 5];
    lines_[i + 3] = lines_[i + 6];
    lines_[i + 4] = asm_line::parse("\tex\tde, hl");
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 5),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 11));
    return true;
}

bool z80_peep::rule_bc_immediate_copy_to_pair_direct(size_t i) {
    if (i + 2 >= lines_.size())
        return false;
    if (!lines_[i].label.empty() || lines_[i].mnemonic != "ld")
        return false;

    std::string dst, src;
    if (!split_ld(lines_[i].operands, dst, src))
        return false;
    dst = trim(dst);
    src = trim(src);
    if (dst != "bc" || (!is_immediate_operand(src) && !is_numeric_literal(src)))
        return false;

    struct pair_desc {
        const char *pair;
        const char *hi;
        const char *lo;
        char hi_ch;
        char lo_ch;
    };
    static const pair_desc dst_pairs[] = {
        {"de", "d", "e", 'd', 'e'},
        {"hl", "h", "l", 'h', 'l'},
    };

    auto match_bc_copy = [&](size_t idx, const pair_desc &pair) {
        if (idx + 1 >= lines_.size())
            return false;
        if (!lines_[idx].label.empty() || !lines_[idx + 1].label.empty())
            return false;
        if (lines_[idx].mnemonic != "ld" || lines_[idx + 1].mnemonic != "ld")
            return false;

        std::string d0, s0, d1, s1;
        if (!split_ld(lines_[idx].operands, d0, s0) ||
            !split_ld(lines_[idx + 1].operands, d1, s1)) {
            return false;
        }
        d0 = trim(d0); s0 = trim(s0);
        d1 = trim(d1); s1 = trim(s1);

        const bool high_low =
            d0 == pair.hi && s0 == "b" &&
            d1 == pair.lo && s1 == "c";
        const bool low_high =
            d0 == pair.lo && s0 == "c" &&
            d1 == pair.hi && s1 == "b";
        return high_low || low_high;
    };

    const size_t max_scan = std::min(lines_.size(), i + 7);
    for (size_t j = i + 1; j + 1 < max_scan; ++j) {
        for (const auto &pair : dst_pairs) {
            if (!match_bc_copy(j, pair))
                continue;

            bool safe_span = true;
            for (size_t k = i + 1; k < j; ++k) {
                const asm_line &span = lines_[k];
                if (span.mnemonic.empty())
                    continue;
                if (!line_preserves_pair_and_sp(span, pair.pair,
                                                pair.lo_ch, pair.hi_ch)) {
                    safe_span = false;
                    break;
                }
            }
            if (!safe_span)
                continue;
            if (!bc_overwritten_or_call_before_read_allowing_unrelated(
                    lines_, j + 2)) {
                continue;
            }

            lines_[i] = asm_line::parse(
                "\tld\t" + std::string(pair.pair) + ", " + src);
            lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(j),
                         lines_.begin() + static_cast<std::ptrdiff_t>(j + 2));
            return true;
        }

        const asm_line &line = lines_[j];
        if (!line.mnemonic.empty()) {
            if (!line.label.empty() || is_section_directive(line) ||
                line_is_control_flow_boundary(line)) {
                return false;
            }
            if (line_touches_byte_or_pair(line, 'b', "bc") ||
                line_touches_byte_or_pair(line, 'c', "bc")) {
                return false;
            }
        }
    }

    return false;
}

bool z80_peep::rule_pair_immediate_store_direct(size_t i) {
    if (i + 2 >= lines_.size())
        return false;
    if (!lines_[i + 1].label.empty() || !lines_[i + 2].label.empty()) {
        return false;
    }
    if (lines_[i].mnemonic != "ld" ||
        lines_[i + 1].mnemonic != "ld" ||
        lines_[i + 2].mnemonic != "ld") {
        return false;
    }

    struct pair_desc {
        const char *pair;
        const char *lo;
        const char *hi;
        char lo_ch;
        char hi_ch;
    };
    static const pair_desc pairs[] = {
        {"hl", "l", "h", 'l', 'h'},
        {"de", "e", "d", 'e', 'd'},
        {"bc", "c", "b", 'c', 'b'},
    };

    std::string pair_dst;
    std::string imm_src;
    if (!split_ld(lines_[i].operands, pair_dst, imm_src))
        return false;
    pair_dst = trim(pair_dst);
    imm_src = trim(imm_src);

    const pair_desc *pair = nullptr;
    for (const auto &candidate : pairs) {
        if (pair_dst == candidate.pair) {
            pair = &candidate;
            break;
        }
    }
    if (!pair)
        return false;
    if (!is_immediate_operand(imm_src) && !is_numeric_literal(imm_src))
        return false;

    std::string lo_dst;
    std::string lo_src;
    std::string hi_dst;
    std::string hi_src;
    int lo_off = 0;
    int hi_off = 0;
    if (!split_ld(lines_[i + 1].operands, lo_dst, lo_src) ||
        !split_ld(lines_[i + 2].operands, hi_dst, hi_src) ||
        !parse_ix_ref(trim(lo_dst), lo_off) ||
        !parse_ix_ref(trim(hi_dst), hi_off) ||
        hi_off != lo_off + 1 ||
        trim(lo_src) != pair->lo ||
        trim(hi_src) != pair->hi) {
        return false;
    }

    if (!pair_dead_before_read_allowing_spaghetti(lines_, i + 3,
                                                  pair->pair,
                                                  pair->lo_ch,
                                                  pair->hi_ch,
                                                  true)) {
        return false;
    }

    int value = 0;
    if (!parse_immediate_value(imm_src, value))
        return false;
    const std::string lo_imm = imm8_text(value);
    const std::string hi_imm = imm8_text(value >> 8);

    const std::string saved_label = lines_[i].label;
    const bool saved_is_label = lines_[i].is_label;
    const bool saved_is_global_label = lines_[i].is_global_label;
    const bool saved_label_has_colon = lines_[i].label_has_colon;
    const std::string saved_comment = lines_[i].comment;
    lines_[i] = asm_line::parse(
        "\tld\t" + std::to_string(lo_off) + "(ix), " + lo_imm);
    lines_[i].label = saved_label;
    lines_[i].is_label = saved_is_label;
    lines_[i].is_global_label = saved_is_global_label;
    lines_[i].label_has_colon = saved_label_has_colon;
    lines_[i].comment = saved_comment;
    lines_[i + 1] = asm_line::parse(
        "\tld\t" + std::to_string(hi_off) + "(ix), " + hi_imm);
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 2));
    return true;
}

bool z80_peep::rule_const_add_bc_de_fold(size_t i) {
    if (i + 3 >= lines_.size())
        return false;
    for (size_t j = i; j <= i + 3; ++j) {
        if (!lines_[j].label.empty())
            return false;
    }

    std::string dst;
    std::string src;
    int bc_value = 0;
    int de_value = 0;
    if (lines_[i].mnemonic != "ld" ||
        !split_ld(lines_[i].operands, dst, src) ||
        trim(dst) != "bc" ||
        !parse_immediate_value(src, bc_value)) {
        return false;
    }
    if (lines_[i + 1].mnemonic != "add" ||
        !split_ld(lines_[i + 1].operands, dst, src) ||
        trim(dst) != "hl" || trim(src) != "bc") {
        return false;
    }
    if (lines_[i + 2].mnemonic != "ld" ||
        !split_ld(lines_[i + 2].operands, dst, src) ||
        trim(dst) != "de" ||
        !parse_immediate_value(src, de_value)) {
        return false;
    }
    if (lines_[i + 3].mnemonic != "add" ||
        !split_ld(lines_[i + 3].operands, dst, src) ||
        trim(dst) != "hl" || trim(src) != "de") {
        return false;
    }

    if (!flags_overwritten_before_read_or_escape(lines_, i + 4))
        return false;
    if (!pair_value_dead_or_call_or_modern_return_before_read(
            lines_, i + 4, "bc", 'c', 'b'))
        return false;
    if (!pair_value_dead_or_call_or_modern_return_before_read(
            lines_, i + 4, "de", 'e', 'd'))
        return false;

    const int folded = bc_value + de_value;
    if (folded < -32768 || folded > 65535)
        return false;

    lines_[i].operands = "bc, #" + std::to_string(folded);
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 2),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 4));
    return true;
}

// ld N(ix),l; ld N+1(ix),h; ld l,N(ix); ld h,N+1(ix)
// The last two lines reload HL from a slot it was just stored to, which is
// a no-op.  Remove them.  No intermediate labels allowed.
bool z80_peep::rule_ix_store_reload(size_t i) {
    if (i + 3 >= lines_.size()) return false;
    auto &l0 = lines_[i];
    auto &l1 = lines_[i + 1];
    auto &l2 = lines_[i + 2];
    auto &l3 = lines_[i + 3];

    for (int k = 1; k <= 3; ++k)
        if (!lines_[i + k].label.empty()) return false;

    if (l0.mnemonic != "ld" || l1.mnemonic != "ld" ||
        l2.mnemonic != "ld" || l3.mnemonic != "ld") return false;

    std::string d0, s0, d1, s1, d2, s2, d3, s3;
    if (!split_ld(l0.operands, d0, s0) || trim(s0) != "l") return false;
    d0 = trim(d0);
    int off_lo; if (!parse_ix_ref(d0, off_lo)) return false;

    if (!split_ld(l1.operands, d1, s1) || trim(s1) != "h") return false;
    d1 = trim(d1);
    int off_hi; if (!parse_ix_ref(d1, off_hi) || off_hi != off_lo + 1) return false;

    if (!split_ld(l2.operands, d2, s2) || trim(d2) != "l") return false;
    s2 = trim(s2);
    int off_lo2; if (!parse_ix_ref(s2, off_lo2) || off_lo2 != off_lo) return false;

    if (!split_ld(l3.operands, d3, s3) || trim(d3) != "h") return false;
    s3 = trim(s3);
    int off_hi2; if (!parse_ix_ref(s3, off_hi2) || off_hi2 != off_hi) return false;

    lines_.erase(lines_.begin() + i + 2, lines_.begin() + i + 4);
    return true;
}

bool z80_peep::rule_ix_store32_low_reload(size_t i) {
    if (i + 5 >= lines_.size())
        return false;

    auto parse_ld_ix_from_reg = [&](size_t idx, const char *reg, int &off) {
        if (idx >= lines_.size() || !lines_[idx].label.empty() ||
            lines_[idx].mnemonic != "ld") {
            return false;
        }
        std::string dst;
        std::string src;
        if (!split_ld(lines_[idx].operands, dst, src) ||
            trim(src) != reg) {
            return false;
        }
        return parse_ix_ref(lower_copy(trim(dst)), off);
    };

    auto parse_ld_reg_from_ix = [&](size_t idx, const char *reg, int &off) {
        if (idx >= lines_.size() || !lines_[idx].label.empty() ||
            lines_[idx].mnemonic != "ld") {
            return false;
        }
        std::string dst;
        std::string src;
        if (!split_ld(lines_[idx].operands, dst, src) ||
            trim(dst) != reg) {
            return false;
        }
        return parse_ix_ref(lower_copy(trim(src)), off);
    };

    int lo = 0;
    int hi = 0;
    int upper_lo = 0;
    int upper_hi = 0;
    if (!parse_ld_ix_from_reg(i, "l", lo) ||
        !parse_ld_ix_from_reg(i + 1, "h", hi) ||
        !parse_ld_ix_from_reg(i + 2, "e", upper_lo) ||
        !parse_ld_ix_from_reg(i + 3, "d", upper_hi) ||
        hi != lo + 1 ||
        upper_lo != lo + 2 ||
        upper_hi != lo + 3) {
        return false;
    }

    size_t reload = i + 4;
    while (reload < lines_.size() &&
           lines_[reload].label.empty() &&
           (lines_[reload].is_comment ||
            lines_[reload].mnemonic.empty())) {
        ++reload;
    }
    if (reload + 1 >= lines_.size())
        return false;

    int reload_lo = 0;
    int reload_hi = 0;
    if (!parse_ld_reg_from_ix(reload, "l", reload_lo) ||
        !parse_ld_reg_from_ix(reload + 1, "h", reload_hi) ||
        reload_lo != lo ||
        reload_hi != hi) {
        return false;
    }

    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(reload),
                 lines_.begin() + static_cast<std::ptrdiff_t>(reload + 2));
    return true;
}

bool z80_peep::rule_ix_postinc_indirect_load_a(size_t i) {
    if (i + 10 >= lines_.size())
        return false;

    for (size_t k = i; k <= i + 10; ++k) {
        if (k == i + 7)
            continue;
        if (!lines_[k].label.empty())
            return false;
    }
    if (lines_[i + 7].label.empty() || !lines_[i + 7].mnemonic.empty())
        return false;

    auto parse_ld = [&](size_t pos, std::string &dst, std::string &src) {
        if (lines_[pos].mnemonic != "ld")
            return false;
        if (!split_ld(lines_[pos].operands, dst, src))
            return false;
        dst = trim(dst);
        src = trim(src);
        return true;
    };

    std::string dst;
    std::string src;
    int ptr_lo = 0;
    int ptr_hi = 0;
    int temp_lo = 0;
    int temp_hi = 0;
    if (!parse_ld(i, dst, src) ||
        dst != "l" ||
        !parse_ix_ref(src, ptr_lo) ||
        !parse_ld(i + 1, dst, src) ||
        dst != "h" ||
        !parse_ix_ref(src, ptr_hi) ||
        ptr_hi != ptr_lo + 1 ||
        !parse_ld(i + 2, dst, src) ||
        src != "l" ||
        !parse_ix_ref(dst, temp_lo) ||
        !parse_ld(i + 3, dst, src) ||
        src != "h" ||
        !parse_ix_ref(dst, temp_hi) ||
        temp_hi != temp_lo + 1) {
        return false;
    }

    int inc_lo = 0;
    int inc_hi = 0;
    if (lines_[i + 4].mnemonic != "inc" ||
        !parse_ix_ref(trim(lines_[i + 4].operands), inc_lo) ||
        inc_lo != ptr_lo) {
        return false;
    }

    std::string cc;
    std::string target;
    if (!split_conditional_branch_target(lines_[i + 5], cc, target) ||
        cc != "nz" ||
        trim(target) != lines_[i + 7].label) {
        return false;
    }

    if (lines_[i + 6].mnemonic != "inc" ||
        !parse_ix_ref(trim(lines_[i + 6].operands), inc_hi) ||
        inc_hi != ptr_hi) {
        return false;
    }

    if (!parse_ld(i + 8, dst, src) ||
        dst != "l" ||
        !parse_ix_ref(src, inc_lo) ||
        inc_lo != temp_lo ||
        !parse_ld(i + 9, dst, src) ||
        dst != "h" ||
        !parse_ix_ref(src, inc_hi) ||
        inc_hi != temp_hi ||
        !parse_ld(i + 10, dst, src) ||
        dst != "a" ||
        src != "(hl)") {
        return false;
    }

    int locals = 0;
    int temp_frame = 0;
    size_t prologue_index = 0;
    const bool have_frame =
        current_function_frame(lines_, i, locals, temp_frame, prologue_index);
    if (have_frame) {
        if (!ix_offset_in_temp_frame(temp_lo, locals, temp_frame) ||
            !ix_offset_in_temp_frame(temp_hi, locals, temp_frame)) {
            return false;
        }
    }
    const size_t fn_end = have_frame
        ? function_end_after_prologue(lines_, prologue_index)
        : lines_.size();
    auto slot_referenced_between = [&](int offset) {
        const std::string ref = std::to_string(offset) + "(ix)";
        for (size_t k = i + 11; k < fn_end && k < lines_.size(); ++k) {
            if (lines_[k].operands.find(ref) != std::string::npos)
                return true;
        }
        return false;
    };
    if (slot_referenced_between(temp_lo) || slot_referenced_between(temp_hi)) {
        return false;
    }

    std::vector<asm_line> replacement;
    replacement.push_back(lines_[i]);
    replacement.push_back(lines_[i + 1]);
    replacement.push_back(asm_line::parse("\tld\ta, (hl)"));
    replacement.push_back(lines_[i + 4]);
    replacement.push_back(lines_[i + 5]);
    replacement.push_back(lines_[i + 6]);
    replacement.push_back(lines_[i + 7]);

    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 11));
    lines_.insert(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                  replacement.begin(), replacement.end());
    return true;
}

bool z80_peep::rule_ix_postinc_indexed_store_a(size_t i) {
    if (i + 12 >= lines_.size())
        return false;

    for (size_t k = i; k <= i + 12; ++k) {
        if (k == i + 7)
            continue;
        if (!lines_[k].label.empty())
            return false;
    }
    if (lines_[i + 7].label.empty() || !lines_[i + 7].mnemonic.empty())
        return false;

    auto parse_ld = [&](size_t pos, std::string &dst, std::string &src) {
        if (lines_[pos].mnemonic != "ld")
            return false;
        if (!split_ld(lines_[pos].operands, dst, src))
            return false;
        dst = trim(dst);
        src = trim(src);
        return true;
    };

    std::string dst;
    std::string src;
    int index_lo = 0;
    int index_hi = 0;
    int temp_lo = 0;
    int temp_hi = 0;
    if (!parse_ld(i, dst, src) ||
        dst != "l" ||
        !parse_ix_ref(src, index_lo) ||
        !parse_ld(i + 1, dst, src) ||
        dst != "h" ||
        !parse_ix_ref(src, index_hi) ||
        index_hi != index_lo + 1 ||
        !parse_ld(i + 2, dst, src) ||
        src != "l" ||
        !parse_ix_ref(dst, temp_lo) ||
        !parse_ld(i + 3, dst, src) ||
        src != "h" ||
        !parse_ix_ref(dst, temp_hi) ||
        temp_hi != temp_lo + 1) {
        return false;
    }

    int parsed = 0;
    if (lines_[i + 4].mnemonic != "inc" ||
        !parse_ix_ref(trim(lines_[i + 4].operands), parsed) ||
        parsed != index_lo) {
        return false;
    }

    std::string cc;
    std::string target;
    if (!split_conditional_branch_target(lines_[i + 5], cc, target) ||
        cc != "nz" ||
        trim(target) != lines_[i + 7].label) {
        return false;
    }

    if (lines_[i + 6].mnemonic != "inc" ||
        !parse_ix_ref(trim(lines_[i + 6].operands), parsed) ||
        parsed != index_hi) {
        return false;
    }

    std::string base_src;
    if (!parse_ld(i + 8, dst, base_src) || dst != "hl")
        return false;
    if (!parse_ld(i + 9, dst, src) ||
        dst != "e" ||
        !parse_ix_ref(src, parsed) ||
        parsed != temp_lo ||
        !parse_ld(i + 10, dst, src) ||
        dst != "d" ||
        !parse_ix_ref(src, parsed) ||
        parsed != temp_hi) {
        return false;
    }
    if (lines_[i + 11].mnemonic != "add" ||
        !split_ld(lines_[i + 11].operands, dst, src) ||
        trim(dst) != "hl" ||
        trim(src) != "de") {
        return false;
    }

    bool has_value_load = false;
    asm_line value_load;
    size_t store_idx = i + 12;
    if (lines_[store_idx].mnemonic == "ld" &&
        split_ld(lines_[store_idx].operands, dst, src) &&
        trim(dst) == "a") {
        has_value_load = true;
        value_load = lines_[store_idx];
        src = trim(src);
        int value_off = 0;
        if (parse_ix_ref(src, value_off) &&
            (value_off == index_lo || value_off == index_hi ||
             value_off == temp_lo || value_off == temp_hi)) {
            return false;
        }
        ++store_idx;
    }

    if (store_idx >= lines_.size() ||
        !lines_[store_idx].label.empty() ||
        !parse_ld(store_idx, dst, src) ||
        dst != "(hl)" ||
        src != "a") {
        return false;
    }

    int locals = 0;
    int temp_frame = 0;
    size_t prologue_index = 0;
    const bool have_frame =
        current_function_frame(lines_, i, locals, temp_frame, prologue_index);
    if (have_frame) {
        if (!ix_offset_in_temp_frame(temp_lo, locals, temp_frame) ||
            !ix_offset_in_temp_frame(temp_hi, locals, temp_frame)) {
            return false;
        }
    }
    const size_t fn_end = have_frame
        ? function_end_after_prologue(lines_, prologue_index)
        : lines_.size();
    auto slot_referenced_between = [&](int offset) {
        const std::string ref = std::to_string(offset) + "(ix)";
        for (size_t k = store_idx + 1; k < fn_end && k < lines_.size(); ++k) {
            if (lines_[k].operands.find(ref) != std::string::npos)
                return true;
        }
        return false;
    };
    if (slot_referenced_between(temp_lo) || slot_referenced_between(temp_hi))
        return false;

    if (!flags_from_compare_dead_before_read(lines_, store_idx + 1))
        return false;

    std::vector<asm_line> replacement;
    replacement.push_back(lines_[i + 8]);
    replacement.push_back(asm_line::parse(
        "\tld\te, " + std::to_string(index_lo) + "(ix)"));
    replacement.push_back(asm_line::parse(
        "\tld\td, " + std::to_string(index_hi) + "(ix)"));
    replacement.push_back(lines_[i + 11]);
    if (has_value_load)
        replacement.push_back(value_load);
    replacement.push_back(lines_[store_idx]);
    replacement.push_back(lines_[i + 4]);
    replacement.push_back(lines_[i + 5]);
    replacement.push_back(lines_[i + 6]);
    replacement.push_back(lines_[i + 7]);

    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                 lines_.begin() + static_cast<std::ptrdiff_t>(store_idx + 1));
    lines_.insert(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                  replacement.begin(), replacement.end());
    return true;
}

bool z80_peep::rule_adjacent_postinc_indexed_stores_direct(size_t i) {
    if (i + 17 >= lines_.size())
        return false;

    auto parse_ld = [&](size_t pos, std::string &dst, std::string &src) {
        if (pos >= lines_.size() || lines_[pos].mnemonic != "ld")
            return false;
        if (!split_ld(lines_[pos].operands, dst, src))
            return false;
        dst = trim(dst);
        src = trim(src);
        return true;
    };

    auto value_load_is_safe = [&](const asm_line &line,
                                  int index_lo,
                                  int index_hi) {
        std::string dst;
        std::string src;
        if (!split_ld(line.operands, dst, src) || trim(dst) != "a")
            return false;
        src = trim(src);
        int off = 0;
        if (parse_ix_ref(src, off) && (off == index_lo || off == index_hi))
            return false;
        return !operand_mentions_pair_or_bytes(src, "hl", 'l', 'h') &&
               !operand_mentions_pair_or_bytes(src, "de", 'e', 'd');
    };

    struct store_match {
        size_t value_idx = static_cast<size_t>(-1);
        size_t store_idx = 0;
        size_t label_idx = 0;
        size_t next = 0;
        std::string base_src;
        int index_lo = 0;
        int index_hi = 0;
    };

    auto match_store = [&](size_t pos,
                           const std::string &expected_base,
                           int expected_lo,
                           int expected_hi,
                           store_match &out) {
        if (pos + 8 >= lines_.size())
            return false;
        std::string dst;
        std::string src;
        int parsed = 0;
        if (!parse_ld(pos, dst, out.base_src) || dst != "hl")
            return false;
        if (!expected_base.empty() && out.base_src != expected_base)
            return false;
        if (!parse_ld(pos + 1, dst, src) ||
            dst != "e" ||
            !parse_ix_ref(src, out.index_lo)) {
            return false;
        }
        if (!parse_ld(pos + 2, dst, src) ||
            dst != "d" ||
            !parse_ix_ref(src, out.index_hi) ||
            out.index_hi != out.index_lo + 1) {
            return false;
        }
        if (expected_lo != 0x7fffffff &&
            (out.index_lo != expected_lo || out.index_hi != expected_hi)) {
            return false;
        }
        if (lines_[pos + 3].mnemonic != "add" ||
            !split_ld(lines_[pos + 3].operands, dst, src) ||
            trim(dst) != "hl" ||
            trim(src) != "de") {
            return false;
        }

        size_t store_idx = pos + 4;
        if (lines_[store_idx].mnemonic == "ld" &&
            value_load_is_safe(lines_[store_idx], out.index_lo, out.index_hi)) {
            out.value_idx = store_idx;
            ++store_idx;
        }
        if (!parse_ld(store_idx, dst, src) ||
            dst != "(hl)" ||
            src != "a") {
            return false;
        }
        out.store_idx = store_idx;

        if (store_idx + 4 >= lines_.size() ||
            lines_[store_idx + 1].mnemonic != "inc" ||
            !parse_ix_ref(trim(lines_[store_idx + 1].operands), parsed) ||
            parsed != out.index_lo) {
            return false;
        }
        std::string cc;
        std::string target;
        if (!split_conditional_branch_target(lines_[store_idx + 2], cc, target) ||
            cc != "nz") {
            return false;
        }
        if (lines_[store_idx + 3].mnemonic != "inc" ||
            !parse_ix_ref(trim(lines_[store_idx + 3].operands), parsed) ||
            parsed != out.index_hi) {
            return false;
        }
        if (lines_[store_idx + 4].label.empty() ||
            !lines_[store_idx + 4].mnemonic.empty() ||
            trim(target) != lines_[store_idx + 4].label) {
            return false;
        }
        out.label_idx = store_idx + 4;
        out.next = store_idx + 5;

        for (size_t k = pos; k < out.next; ++k) {
            if (k == out.label_idx)
                continue;
            if (!lines_[k].label.empty())
                return false;
        }
        return true;
    };

    store_match first;
    if (!match_store(i, "", 0x7fffffff, 0x7fffffff, first))
        return false;
    store_match second;
    if (!match_store(first.next, first.base_src,
                     first.index_lo, first.index_hi, second)) {
        return false;
    }

    const size_t after = second.next;
    if (!flags_from_compare_dead_before_read(lines_, after) ||
        !hl_dead_before_read_or_modern_return(lines_, after) ||
        !pair_value_dead_or_call_or_modern_return_before_read(
            lines_, after, "de", 'e', 'd')) {
        return false;
    }

    std::vector<asm_line> replacement;
    replacement.push_back(lines_[i]);
    replacement.push_back(lines_[i + 1]);
    replacement.push_back(lines_[i + 2]);
    replacement.push_back(lines_[i + 3]);
    if (first.value_idx != static_cast<size_t>(-1))
        replacement.push_back(lines_[first.value_idx]);
    replacement.push_back(lines_[first.store_idx]);
    replacement.push_back(asm_line::parse("\tinc\thl"));
    if (second.value_idx != static_cast<size_t>(-1))
        replacement.push_back(lines_[second.value_idx]);
    replacement.push_back(lines_[second.store_idx]);
    replacement.push_back(asm_line::parse(
        "\tld\tl, " + std::to_string(first.index_lo) + "(ix)"));
    replacement.push_back(asm_line::parse(
        "\tld\th, " + std::to_string(first.index_hi) + "(ix)"));
    replacement.push_back(asm_line::parse("\tinc\thl"));
    replacement.push_back(asm_line::parse("\tinc\thl"));
    replacement.push_back(asm_line::parse(
        "\tld\t" + std::to_string(first.index_lo) + "(ix), l"));
    replacement.push_back(asm_line::parse(
        "\tld\t" + std::to_string(first.index_hi) + "(ix), h"));

    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                 lines_.begin() + static_cast<std::ptrdiff_t>(after));
    lines_.insert(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                  replacement.begin(), replacement.end());
    return true;
}

bool z80_peep::rule_ix_postinc_local_immediate_store(size_t i) {
    auto no_labels = [&](size_t first, size_t last) {
        if (last >= lines_.size())
            return false;
        for (size_t j = first; j <= last; ++j)
            if (!lines_[j].label.empty())
                return false;
        return true;
    };

    auto match_ld = [&](size_t idx, const char *want_dst,
                        std::string *src_out = nullptr) {
        if (idx >= lines_.size() || lines_[idx].mnemonic != "ld")
            return false;
        std::string dst;
        std::string src;
        if (!split_ld(lines_[idx].operands, dst, src))
            return false;
        if (trim(dst) != want_dst)
            return false;
        if (src_out)
            *src_out = trim(src);
        return true;
    };

    auto parse_ld_ix_src = [&](size_t idx, const char *want_dst, int &off) {
        std::string src;
        if (!match_ld(idx, want_dst, &src))
            return false;
        return parse_ix_ref(src, off);
    };

    auto parse_ld_ix_dst = [&](size_t idx, const char *want_src, int &off) {
        if (idx >= lines_.size() || lines_[idx].mnemonic != "ld")
            return false;
        std::string dst;
        std::string src;
        if (!split_ld(lines_[idx].operands, dst, src) || trim(src) != want_src)
            return false;
        return parse_ix_ref(trim(dst), off);
    };

    auto match_address_tail = [&](size_t start, int &base_offset,
                                  std::string &imm_src) {
        if (start + 7 >= lines_.size())
            return false;
        if (!no_labels(start, start + 7))
            return false;
        std::string src;
        if (lines_[start].mnemonic != "push" ||
            trim(lines_[start].operands) != "ix" ||
            lines_[start + 1].mnemonic != "pop" ||
            trim(lines_[start + 1].operands) != "hl" ||
            !match_ld(start + 2, "bc", &src) ||
            !parse_immediate_value(src, base_offset)) {
            return false;
        }
        std::string dst;
        if (lines_[start + 3].mnemonic != "add" ||
            !split_ld(lines_[start + 3].operands, dst, src) ||
            trim(dst) != "hl" || trim(src) != "bc" ||
            !match_ld(start + 4, "e", &src) || trim(src) != "a" ||
            !match_ld(start + 5, "d", &src) || !immediate_is(src, 0) ||
            lines_[start + 6].mnemonic != "add" ||
            !split_ld(lines_[start + 6].operands, dst, src) ||
            trim(dst) != "hl" || trim(src) != "de" ||
            !match_ld(start + 7, "(hl)", &imm_src) ||
            (!is_immediate_operand(imm_src) && !is_numeric_literal(imm_src))) {
            return false;
        }
        (void)base_offset;
        return true;
    };

    auto match_sp_address_tail = [&](size_t start, int &sp_offset,
                                     std::string &imm_src) {
        if (start + 5 >= lines_.size())
            return false;
        if (!no_labels(start, start + 5))
            return false;

        std::string dst;
        std::string src;
        if (!match_ld(start, "hl", &src) ||
            !parse_immediate_value(src, sp_offset) ||
            lines_[start + 1].mnemonic != "add" ||
            !split_ld(lines_[start + 1].operands, dst, src) ||
            trim(dst) != "hl" || trim(src) != "sp" ||
            !match_ld(start + 2, "e", &src) || trim(src) != "a" ||
            !match_ld(start + 3, "d", &src) || !immediate_is(src, 0) ||
            lines_[start + 4].mnemonic != "add" ||
            !split_ld(lines_[start + 4].operands, dst, src) ||
            trim(dst) != "hl" || trim(src) != "de" ||
            !match_ld(start + 5, "(hl)", &imm_src) ||
            (!is_immediate_operand(imm_src) && !is_numeric_literal(imm_src))) {
            return false;
        }
        return true;
    };

    auto infer_sp_offset_for_ix_offset = [&](int ix_offset,
                                             int &sp_offset) {
        size_t alloc_sp_idx = lines_.size();
        int frame_delta = 0;
        for (size_t k = i; k >= 2; --k) {
            const size_t alloc = k - 2;
            if (!lines_[alloc].label.empty() ||
                !lines_[alloc + 1].label.empty() ||
                !lines_[alloc + 2].label.empty()) {
                if (alloc == 0)
                    break;
                continue;
            }
            std::string dst;
            std::string src;
            int amount = 0;
            if (lines_[alloc].mnemonic == "ld" &&
                split_ld(lines_[alloc].operands, dst, src) &&
                trim(dst) == "hl" &&
                parse_immediate_value(src, amount) &&
                lines_[alloc + 1].mnemonic == "add" &&
                split_ld(lines_[alloc + 1].operands, dst, src) &&
                trim(dst) == "hl" && trim(src) == "sp" &&
                lines_[alloc + 2].mnemonic == "ld" &&
                split_ld(lines_[alloc + 2].operands, dst, src) &&
                trim(dst) == "sp" && trim(src) == "hl") {
                alloc_sp_idx = alloc + 2;
                frame_delta = amount;
                break;
            }
            if (alloc == 0)
                break;
        }
        if (alloc_sp_idx == lines_.size() || frame_delta >= 0)
            return false;

        int current_delta = frame_delta;
        for (size_t k = alloc_sp_idx + 1; k < i && k < lines_.size(); ++k) {
            const asm_line &line = lines_[k];
            if (line.mnemonic.empty() || is_section_directive(line))
                continue;
            if (line.mnemonic == "push") {
                current_delta -= 2;
                continue;
            }
            if (line.mnemonic == "pop") {
                current_delta += 2;
                continue;
            }
            if (line.mnemonic == "inc" && trim(line.operands) == "sp") {
                current_delta += 1;
                continue;
            }
            if (line.mnemonic == "dec" && trim(line.operands) == "sp") {
                current_delta -= 1;
                continue;
            }
            if (line.mnemonic == "ld") {
                std::string dst;
                std::string src;
                if (split_ld(line.operands, dst, src) &&
                    trim(dst) == "sp") {
                    if (trim(src) == "ix") {
                        current_delta = 0;
                        continue;
                    }
                    return false;
                }
            }
            if (line.mnemonic == "call" && current_delta != frame_delta)
                return false;
        }
        if (current_delta != frame_delta)
            return false;

        sp_offset = ix_offset - frame_delta;
        return true;
    };

    auto emit_replacement = [&](size_t erase_end, int index_off,
                                int base_offset, const std::string &imm_src,
                                bool increment_index) {
        std::vector<asm_line> replacement;
        replacement.push_back(asm_line::parse(
            "\tld\ta, " + std::to_string(index_off) + "(ix)"));
        if (increment_index) {
            replacement.push_back(asm_line::parse(
                "\tinc\t" + std::to_string(index_off) + "(ix)"));
        }
        replacement.push_back(asm_line::parse("\tld\te, a"));
        replacement.push_back(asm_line::parse("\tld\td, #0"));
        int sp_offset = 0;
        if (infer_sp_offset_for_ix_offset(base_offset, sp_offset)) {
            replacement.push_back(asm_line::parse(
                "\tld\thl, #" + std::to_string(sp_offset)));
            replacement.push_back(asm_line::parse("\tadd\thl, sp"));
        } else {
            replacement.push_back(asm_line::parse("\tpush\tix"));
            replacement.push_back(asm_line::parse("\tpop\thl"));
            replacement.push_back(asm_line::parse(
                "\tld\tbc, #" + std::to_string(base_offset)));
            replacement.push_back(asm_line::parse("\tadd\thl, bc"));
        }
        replacement.push_back(asm_line::parse("\tadd\thl, de"));
        replacement.push_back(asm_line::parse("\tld\t(hl), " + imm_src));
        lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                     lines_.begin() + static_cast<std::ptrdiff_t>(erase_end));
        lines_.insert(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                      replacement.begin(), replacement.end());
    };

    auto emit_sp_replacement = [&](size_t erase_end, int index_off,
                                   int sp_offset, const std::string &imm_src,
                                   bool increment_index) {
        std::vector<asm_line> replacement;
        replacement.push_back(asm_line::parse(
            "\tld\ta, " + std::to_string(index_off) + "(ix)"));
        if (increment_index) {
            replacement.push_back(asm_line::parse(
                "\tinc\t" + std::to_string(index_off) + "(ix)"));
        }
        replacement.push_back(asm_line::parse("\tld\te, a"));
        replacement.push_back(asm_line::parse("\tld\td, #0"));
        replacement.push_back(asm_line::parse(
            "\tld\thl, #" + std::to_string(sp_offset)));
        replacement.push_back(asm_line::parse("\tadd\thl, sp"));
        replacement.push_back(asm_line::parse("\tadd\thl, de"));
        replacement.push_back(asm_line::parse("\tld\t(hl), " + imm_src));
        lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                     lines_.begin() + static_cast<std::ptrdiff_t>(erase_end));
        lines_.insert(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                      replacement.begin(), replacement.end());
    };

    if (i + 11 < lines_.size() && no_labels(i, i + 11)) {
        int index_off = 0;
        int tmp_off = 0;
        int tmp_reload_off = 0;
        int sp_offset = 0;
        std::string src;
        std::string imm_src;
        if (parse_ld_ix_src(i, "a", index_off) &&
            parse_ld_ix_dst(i + 1, "a", tmp_off) &&
            lines_[i + 2].mnemonic == "inc" &&
            parse_ix_ref(trim(lines_[i + 2].operands), tmp_reload_off) &&
            tmp_reload_off == index_off &&
            parse_ld_ix_src(i + 3, "a", tmp_reload_off) &&
            tmp_reload_off == tmp_off &&
            match_ld(i + 4, "c", &src) && trim(src) == "a" &&
            match_ld(i + 5, "b", &src) && immediate_is(src, 0) &&
            match_sp_address_tail(i + 6, sp_offset, imm_src) &&
            path_overwrites_bc_before_read_or_call(lines_, i + 12)) {
            emit_sp_replacement(i + 12, index_off, sp_offset, imm_src, true);
            return true;
        }
    }

    if (i + 8 < lines_.size() && no_labels(i, i + 8)) {
        int index_off = 0;
        int sp_offset = 0;
        std::string src;
        std::string imm_src;
        if (parse_ld_ix_src(i, "a", index_off) &&
            match_ld(i + 1, "c", &src) && trim(src) == "a" &&
            match_ld(i + 2, "b", &src) && immediate_is(src, 0) &&
            match_sp_address_tail(i + 3, sp_offset, imm_src) &&
            path_overwrites_bc_before_read_or_call(lines_, i + 9)) {
            emit_sp_replacement(i + 9, index_off, sp_offset, imm_src, false);
            return true;
        }
    }

    if (i + 13 < lines_.size() && no_labels(i, i + 13)) {
        int index_off = 0;
        int tmp_off = 0;
        int reload_off = 0;
        int store_off = 0;
        int sp_offset = 0;
        std::string src;
        std::string imm_src;
        if (parse_ld_ix_src(i, "a", index_off) &&
            parse_ld_ix_dst(i + 1, "a", tmp_off) &&
            parse_ld_ix_src(i + 2, "a", reload_off) &&
            reload_off == index_off &&
            lines_[i + 3].mnemonic == "add" &&
            split_ld(lines_[i + 3].operands, src, imm_src) &&
            trim(src) == "a" && immediate_is(imm_src, 1) &&
            parse_ld_ix_dst(i + 4, "a", store_off) &&
            store_off == index_off &&
            parse_ld_ix_src(i + 5, "a", reload_off) &&
            reload_off == tmp_off &&
            match_ld(i + 6, "c", &src) && trim(src) == "a" &&
            match_ld(i + 7, "b", &src) && immediate_is(src, 0) &&
            match_sp_address_tail(i + 8, sp_offset, imm_src) &&
            path_overwrites_bc_before_read_or_call(lines_, i + 14)) {
            emit_sp_replacement(i + 14, index_off, sp_offset, imm_src, true);
            return true;
        }
    }

    if (i + 12 < lines_.size() && no_labels(i, i + 12)) {
        int value_tmp_off = 0;
        int index_off = 0;
        int sp_offset = 0;
        int inc_off = 0;
        std::string dst;
        std::string src;
        if (lines_[i].mnemonic == "ld" &&
            split_ld(lines_[i].operands, dst, src) &&
            trim(dst) == "a" && trim(src) == "(hl)" &&
            parse_ld_ix_dst(i + 1, "a", value_tmp_off) &&
            parse_ld_ix_src(i + 2, "a", index_off) &&
            match_ld(i + 3, "c", &src) && trim(src) == "a" &&
            match_ld(i + 4, "b", &src) && immediate_is(src, 0) &&
            match_ld(i + 5, "hl", &src) &&
            parse_immediate_value(src, sp_offset) &&
            lines_[i + 6].mnemonic == "add" &&
            split_ld(lines_[i + 6].operands, dst, src) &&
            trim(dst) == "hl" && trim(src) == "sp" &&
            match_ld(i + 7, "e", &src) && trim(src) == "a" &&
            match_ld(i + 8, "d", &src) && immediate_is(src, 0) &&
            lines_[i + 9].mnemonic == "add" &&
            split_ld(lines_[i + 9].operands, dst, src) &&
            trim(dst) == "hl" && trim(src) == "de" &&
            parse_ld_ix_src(i + 10, "a", value_tmp_off) &&
            lines_[i + 11].mnemonic == "ld" &&
            split_ld(lines_[i + 11].operands, dst, src) &&
            trim(dst) == "(hl)" && trim(src) == "a" &&
            lines_[i + 12].mnemonic == "inc" &&
            parse_ix_ref(trim(lines_[i + 12].operands), inc_off) &&
            inc_off == index_off &&
            a_overwritten_before_read(lines_, i + 13) &&
            path_overwrites_bc_before_read_or_call(lines_, i + 13)) {
            std::vector<asm_line> replacement;
            replacement.push_back(asm_line::parse("\tld\tc, (hl)"));
            replacement.push_back(asm_line::parse(
                "\tld\ta, " + std::to_string(index_off) + "(ix)"));
            replacement.push_back(asm_line::parse("\tld\te, a"));
            replacement.push_back(asm_line::parse("\tld\td, #0"));
            replacement.push_back(asm_line::parse(
                "\tld\thl, #" + std::to_string(sp_offset)));
            replacement.push_back(asm_line::parse("\tadd\thl, sp"));
            replacement.push_back(asm_line::parse("\tadd\thl, de"));
            replacement.push_back(asm_line::parse("\tld\t(hl), c"));
            replacement.push_back(asm_line::parse(
                "\tinc\t" + std::to_string(index_off) + "(ix)"));
            lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                         lines_.begin() + static_cast<std::ptrdiff_t>(i + 13));
            lines_.insert(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                          replacement.begin(), replacement.end());
            return true;
        }
    }

    if (i + 14 < lines_.size() && no_labels(i, i + 14)) {
        int index_off = 0;
        int tmp_off = 0;
        int tmp_reload_off = 0;
        int base_offset = 0;
        std::string src;
        std::string imm_src;
        std::string dst;
        if (parse_ld_ix_src(i, "a", index_off) &&
            parse_ld_ix_dst(i + 1, "a", tmp_off) &&
            lines_[i + 2].mnemonic == "inc" &&
            parse_ix_ref(trim(lines_[i + 2].operands), tmp_reload_off) &&
            tmp_reload_off == index_off &&
            parse_ld_ix_src(i + 3, "a", tmp_reload_off) &&
            tmp_reload_off == tmp_off &&
            match_ld(i + 4, "l", &src) && trim(src) == "a" &&
            match_ld(i + 5, "h", &src) && immediate_is(src, 0) &&
            match_ld(i + 6, "b", &src) && trim(src) == "h" &&
            match_ld(i + 7, "c", &src) && trim(src) == "l" &&
            match_address_tail(i + 8, base_offset, imm_src)) {
            emit_replacement(i + 16, index_off, base_offset, imm_src, true);
            return true;
        }
    }

    if (i + 12 < lines_.size() && no_labels(i, i + 12)) {
        int index_off = 0;
        int base_offset = 0;
        std::string src;
        std::string imm_src;
        if (parse_ld_ix_src(i, "a", index_off) &&
            match_ld(i + 1, "l", &src) && trim(src) == "a" &&
            match_ld(i + 2, "h", &src) && immediate_is(src, 0) &&
            match_ld(i + 3, "b", &src) && trim(src) == "h" &&
            match_ld(i + 4, "c", &src) && trim(src) == "l" &&
            match_address_tail(i + 5, base_offset, imm_src)) {
            emit_replacement(i + 13, index_off, base_offset, imm_src, false);
            return true;
        }
    }

    if (i + 13 < lines_.size() && no_labels(i, i + 13)) {
        int index_off = 0;
        int tmp_off = 0;
        int reload_off = 0;
        int store_off = 0;
        int base_offset = 0;
        std::string src;
        std::string imm_src;
        if (parse_ld_ix_src(i, "a", index_off) &&
            parse_ld_ix_dst(i + 1, "a", tmp_off) &&
            parse_ld_ix_src(i + 2, "a", reload_off) &&
            reload_off == index_off &&
            lines_[i + 3].mnemonic == "add" &&
            split_ld(lines_[i + 3].operands, src, imm_src) &&
            trim(src) == "a" && immediate_is(imm_src, 1) &&
            parse_ld_ix_dst(i + 4, "a", store_off) &&
            store_off == index_off &&
            parse_ld_ix_src(i + 5, "a", reload_off) &&
            reload_off == tmp_off &&
            match_ld(i + 6, "e", &src) && trim(src) == "a" &&
            match_ld(i + 7, "d", &src) && immediate_is(src, 0) &&
            match_address_tail(i + 8, base_offset, imm_src)) {
            emit_replacement(i + 16, index_off, base_offset, imm_src, true);
            return true;
        }
    }

    return false;
}

bool z80_peep::rule_ix_indexed_stack_immediate_store_run(size_t i) {
    struct store_step {
        std::string imm;
        bool increments_index = false;
        size_t next = 0;
    };

    auto no_label = [&](size_t idx) {
        return idx < lines_.size() && lines_[idx].label.empty();
    };

    auto match_ld = [&](size_t idx, const char *want_dst,
                        std::string *src_out = nullptr) {
        if (idx >= lines_.size() || !lines_[idx].label.empty() ||
            lines_[idx].mnemonic != "ld") {
            return false;
        }
        std::string dst;
        std::string src;
        if (!split_ld(lines_[idx].operands, dst, src))
            return false;
        if (trim(dst) != want_dst)
            return false;
        if (src_out)
            *src_out = trim(src);
        return true;
    };

    auto match_add = [&](size_t idx, const char *dst_want,
                         const char *src_want) {
        if (idx >= lines_.size() || !lines_[idx].label.empty() ||
            lines_[idx].mnemonic != "add") {
            return false;
        }
        std::string dst;
        std::string src;
        return split_ld(lines_[idx].operands, dst, src) &&
               trim(dst) == dst_want && trim(src) == src_want;
    };

    auto match_store = [&](size_t pos, int &index_off, std::string &base_src,
                           store_step &step) {
        if (pos + 6 >= lines_.size())
            return false;

        std::string src;
        int parsed_index = 0;
        if (!match_ld(pos, "a", &src) || !parse_ix_ref(src, parsed_index))
            return false;

        size_t p = pos + 1;
        bool inc_index = false;
        if (p < lines_.size() && no_label(p) && lines_[p].mnemonic == "inc") {
            int inc_off = 0;
            if (!parse_ix_ref(trim(lines_[p].operands), inc_off) ||
                inc_off != parsed_index) {
                return false;
            }
            inc_index = true;
            ++p;
        }

        std::string base;
        std::string imm;
        bool matched_tail = false;

        if (p + 5 < lines_.size() &&
            match_ld(p, "hl", &base) &&
            match_add(p + 1, "hl", "sp") &&
            match_ld(p + 2, "e", &src) && trim(src) == "a" &&
            match_ld(p + 3, "d", &src) && immediate_is(src, 0) &&
            match_add(p + 4, "hl", "de") &&
            match_ld(p + 5, "(hl)", &imm) &&
            (is_immediate_operand(imm) || is_numeric_literal(imm))) {
            matched_tail = true;
        } else if (p + 5 < lines_.size() &&
                   match_ld(p, "e", &src) && trim(src) == "a" &&
                   match_ld(p + 1, "d", &src) && immediate_is(src, 0) &&
                   match_ld(p + 2, "hl", &base) &&
                   match_add(p + 3, "hl", "sp") &&
                   match_add(p + 4, "hl", "de") &&
                   match_ld(p + 5, "(hl)", &imm) &&
                   (is_immediate_operand(imm) || is_numeric_literal(imm))) {
            matched_tail = true;
        }

        if (!matched_tail)
            return false;

        if (base_src.empty())
            base_src = trim(base);
        else if (base_src != trim(base))
            return false;

        if (index_off == 0x7fffffff)
            index_off = parsed_index;
        else if (index_off != parsed_index)
            return false;

        step.imm = trim(imm);
        step.increments_index = inc_index;
        step.next = p + 6;
        return true;
    };

    auto flags_dead_before_observation = [&](size_t start) {
        if (flags_overwritten_before_read_or_escape(lines_, start))
            return true;
        const size_t end = std::min(lines_.size(), start + 48);
        for (size_t k = start; k < end; ++k) {
            const asm_line &line = lines_[k];
            if (line.mnemonic.empty())
                continue;
            if (line_overwrites_flags_without_reading_carry(line))
                return true;
            if (line.mnemonic == "call" &&
                trim(line.operands).find(',') == std::string::npos) {
                return true;
            }
            if (line_may_read_flags_or_escape(line))
                return false;
        }
        return false;
    };

    auto de_overwritten_straightline_before_read = [&](size_t start) {
        bool d_written = false;
        bool e_written = false;
        const size_t end = std::min(lines_.size(), start + 48);
        for (size_t k = start; k < end; ++k) {
            const asm_line &line = lines_[k];
            if (line.mnemonic.empty())
                continue;
            if (!line.label.empty())
                return false;
            if (is_section_directive(line))
                return false;
            if (!line.mnemonic.empty() && line.mnemonic[0] == '.')
                continue;

            if (overwrites_pair_without_reading_it(lines_, k, "de", 'e', 'd'))
                return true;

            if (line.mnemonic == "ld") {
                std::string dst;
                std::string src;
                if (!split_ld(line.operands, dst, src))
                    return false;
                dst = trim(dst);
                src = trim(src);

                if (operand_mentions_pair_or_bytes(src, "de", 'e', 'd') ||
                    dst == "(de)") {
                    return false;
                }
                if (dst == "d") {
                    d_written = true;
                } else if (dst == "e") {
                    e_written = true;
                } else if (operand_mentions_pair_or_bytes(dst, "de", 'e', 'd')) {
                    return false;
                }
                if (d_written && e_written)
                    return true;
                continue;
            }

            if (line.mnemonic == "call" || line.mnemonic == "ret" ||
                line.mnemonic == "reti" || line.mnemonic == "retn" ||
                line.mnemonic == "rst" || line.mnemonic == "jp" ||
                line.mnemonic == "jr" || line.mnemonic == "djnz" ||
                line.mnemonic == "ex" || line.mnemonic == "exx") {
                return false;
            }
            if (operand_mentions_pair_or_bytes(line.operands, "de", 'e', 'd'))
                return false;
        }
        return false;
    };

    int index_off = 0x7fffffff;
    std::string base_src;
    std::vector<store_step> steps;
    size_t pos = i;
    bool saw_final = false;
    for (size_t guard = 0; guard < 8; ++guard) {
        store_step step;
        if (!match_store(pos, index_off, base_src, step))
            break;
        steps.push_back(step);
        pos = step.next;
        if (!step.increments_index) {
            saw_final = true;
            break;
        }
    }

    if (!saw_final || steps.size() < 3)
        return false;
    for (size_t k = 0; k + 1 < steps.size(); ++k)
        if (!steps[k].increments_index)
            return false;

    int locals = 0;
    int temp_frame = 0;
    size_t prologue_index = 0;
    if (!current_function_frame(lines_, i, locals, temp_frame, prologue_index))
        return false;
    const size_t fn_end = function_end_after_prologue(lines_, prologue_index);

    if (!ix_slot_not_read_before_rewrite(lines_, pos, fn_end, index_off))
        return false;
    if (!a_overwritten_before_read(lines_, pos))
        return false;
    if (!de_overwritten_straightline_before_read(pos))
        return false;
    if (!flags_dead_before_observation(pos))
        return false;

    std::vector<asm_line> replacement;
    replacement.push_back(asm_line::parse(
        "\tld\ta, " + std::to_string(index_off) + "(ix)"));
    replacement.push_back(asm_line::parse("\tld\te, a"));
    replacement.push_back(asm_line::parse("\tld\td, #0"));
    replacement.push_back(asm_line::parse("\tld\thl, " + base_src));
    replacement.push_back(asm_line::parse("\tadd\thl, sp"));
    replacement.push_back(asm_line::parse("\tadd\thl, de"));
    replacement.push_back(asm_line::parse("\tld\t(hl), " + steps.front().imm));
    for (size_t k = 1; k < steps.size(); ++k) {
        replacement.push_back(asm_line::parse("\tinc\thl"));
        replacement.push_back(asm_line::parse("\tld\t(hl), " + steps[k].imm));
    }

    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                 lines_.begin() + static_cast<std::ptrdiff_t>(pos));
    lines_.insert(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                  replacement.begin(), replacement.end());
    return true;
}

bool z80_peep::rule_small_stack_alloc_push_af(size_t i) {
    if (i + 2 >= lines_.size())
        return false;
    for (size_t k = i; k <= i + 2; ++k) {
        if (!lines_[k].label.empty())
            return false;
    }
    if (lines_[i].mnemonic != "ld" ||
        lines_[i + 1].mnemonic != "add" ||
        lines_[i + 2].mnemonic != "ld") {
        return false;
    }

    std::string dst;
    std::string src;
    int amount = 0;
    if (!split_ld(lines_[i].operands, dst, src) ||
        trim(dst) != "hl" ||
        !parse_immediate_value(src, amount) ||
        amount >= 0) {
        return false;
    }
    const int bytes = -amount;
    if (bytes <= 0 || bytes > 12)
        return false;

    if (!split_ld(lines_[i + 1].operands, dst, src) ||
        trim(dst) != "hl" || trim(src) != "sp") {
        return false;
    }
    if (!split_ld(lines_[i + 2].operands, dst, src) ||
        trim(dst) != "sp" || trim(src) != "hl") {
        return false;
    }

    const int replacement_bytes = bytes / 2 + (bytes & 1);
    if (replacement_bytes >= 7)
        return false;

    int locals = 0;
    int temp_frame = 0;
    size_t prologue_index = 0;
    const bool have_frame =
        current_function_frame(lines_, i, locals, temp_frame, prologue_index);
    bool in_prologue_alloc = have_frame && i > prologue_index &&
                             i - prologue_index < 24;
    if (in_prologue_alloc) {
        for (size_t k = prologue_index + 1; k < i; ++k) {
            const asm_line &line = lines_[k];
            if (!line.label.empty() || is_section_directive(line)) {
                in_prologue_alloc = false;
                break;
            }
            // PUSH allocates by writing each newly claimed word.  Codegen can
            // spill incoming registers to their final deep IX offsets before
            // performing the remaining SP adjustment; replacing that adjust
            // with PUSH would overwrite those already-live values.
            if (line_writes_any_ix_offset(line)) {
                return false;
            }
            if (line.mnemonic == "call" &&
                trim(line.operands) != "__sdcc_enter_ix" &&
                trim(line.operands) != "___sdcc_enter_ix") {
                in_prologue_alloc = false;
                break;
            }
            if (line.mnemonic == "ret" || line.mnemonic == "reti" ||
                line.mnemonic == "retn" || line.mnemonic == "rst" ||
                line.mnemonic == "jp" || line.mnemonic == "jr" ||
                line.mnemonic == "djnz") {
                in_prologue_alloc = false;
                break;
            }
        }
    }

    if (!in_prologue_alloc &&
        !flags_overwritten_before_read_or_escape(lines_, i + 3)) {
        return false;
    }

    std::vector<asm_line> replacement;
    replacement.reserve(static_cast<size_t>(replacement_bytes));
    for (int k = 0; k < bytes / 2; ++k)
        replacement.push_back(asm_line::parse("\tpush\taf"));
    if (bytes & 1)
        replacement.push_back(asm_line::parse("\tdec\tsp"));

    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 3));
    lines_.insert(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                  replacement.begin(), replacement.end());
    return true;
}

bool z80_peep::rule_ix_addr_materialize_sp_relative(size_t i) {
    if (i + 3 >= lines_.size())
        return false;
    if (!lines_[i].label.empty() || !lines_[i + 1].label.empty())
        return false;
    if (lines_[i].mnemonic != "push" || trim(lines_[i].operands) != "ix" ||
        lines_[i + 1].mnemonic != "pop" || trim(lines_[i + 1].operands) != "hl") {
        return false;
    }

    auto infer_current_delta = [&](int &delta_out) {
        size_t anchor = lines_.size();
        int frame_delta = 0;
        for (size_t k = i; k > 0; --k) {
            const size_t idx = k - 1;
            std::string dst;
            std::string src;
            if (idx >= 2) {
                int amount = 0;
                if (lines_[idx - 2].mnemonic == "ld" &&
                    split_ld(lines_[idx - 2].operands, dst, src) &&
                    trim(dst) == "hl" &&
                    parse_immediate_value(src, amount) &&
                    lines_[idx - 1].mnemonic == "add" &&
                    split_ld(lines_[idx - 1].operands, dst, src) &&
                    trim(dst) == "hl" && trim(src) == "sp" &&
                    lines_[idx].mnemonic == "ld" &&
                    split_ld(lines_[idx].operands, dst, src) &&
                    trim(dst) == "sp" && trim(src) == "hl") {
                    anchor = idx;
                    frame_delta = amount;
                    break;
                }
            }

            if (lines_[idx].mnemonic == "call" &&
                (trim(lines_[idx].operands) == "__sdcc_enter_ix" ||
                 trim(lines_[idx].operands) == "___sdcc_enter_ix")) {
                anchor = idx;
                frame_delta = 0;
                break;
            }

            if (idx >= 2 &&
                lines_[idx - 2].mnemonic == "push" &&
                trim(lines_[idx - 2].operands) == "ix" &&
                lines_[idx - 1].mnemonic == "ld" &&
                split_ld(lines_[idx - 1].operands, dst, src) &&
                trim(dst) == "ix" && immediate_is(src, 0) &&
                lines_[idx].mnemonic == "add" &&
                split_ld(lines_[idx].operands, dst, src) &&
                trim(dst) == "ix" && trim(src) == "sp") {
                anchor = idx;
                frame_delta = 0;
                break;
            }
        }

        if (anchor == lines_.size())
            return false;

        int current_delta = frame_delta;
        for (size_t k = anchor + 1; k < i; ++k) {
            const asm_line &line = lines_[k];
            if (line.mnemonic.empty() || is_section_directive(line))
                continue;
            if (line.mnemonic == "push") {
                current_delta -= 2;
                continue;
            }
            if (line.mnemonic == "pop") {
                current_delta += 2;
                continue;
            }
            if (line.mnemonic == "inc" && trim(line.operands) == "sp") {
                current_delta += 1;
                continue;
            }
            if (line.mnemonic == "dec" && trim(line.operands) == "sp") {
                current_delta -= 1;
                continue;
            }
            if (line.mnemonic == "ld") {
                std::string dst;
                std::string src;
                if (split_ld(line.operands, dst, src) &&
                    trim(dst) == "sp") {
                    if (trim(src) == "ix") {
                        current_delta = 0;
                        continue;
                    }
                    return false;
                }
            }
            if (line.mnemonic == "call" && current_delta != frame_delta)
                return false;
        }

        delta_out = current_delta;
        return true;
    };

    int offset = 0;
    size_t end = i + 2;
    bool saw_add = false;
    if (end + 1 < lines_.size() &&
        lines_[end].label.empty() &&
        lines_[end + 1].label.empty()) {
        std::string dst;
        std::string src;
        if (lines_[end].mnemonic == "ld" &&
            split_ld(lines_[end].operands, dst, src) &&
            trim(dst) == "bc" &&
            parse_immediate_value(src, offset) &&
            lines_[end + 1].mnemonic == "add" &&
            split_ld(lines_[end + 1].operands, dst, src) &&
            trim(dst) == "hl" && trim(src) == "bc") {
            end += 2;
            saw_add = true;
        }
    }
    if (end + 1 < lines_.size() &&
        lines_[end].label.empty() &&
        lines_[end + 1].label.empty()) {
        std::string dst;
        std::string src;
        int extra = 0;
        if (lines_[end].mnemonic == "ld" &&
            split_ld(lines_[end].operands, dst, src) &&
            trim(dst) == "de" &&
            parse_immediate_value(src, extra) &&
            lines_[end + 1].mnemonic == "add" &&
            split_ld(lines_[end + 1].operands, dst, src) &&
            trim(dst) == "hl" && trim(src) == "de") {
            offset += extra;
            end += 2;
        }
    }

    int adjust = 0;
    while (end < lines_.size() &&
           lines_[end].label.empty() &&
           (lines_[end].mnemonic == "inc" ||
            lines_[end].mnemonic == "dec") &&
           trim(lines_[end].operands) == "hl") {
        adjust += lines_[end].mnemonic == "inc" ? 1 : -1;
        if (adjust > 64 || adjust < -64)
            return false;
        ++end;
    }
    offset += adjust;
    if (!saw_add && adjust > -2 && adjust < 2)
        return false;

    int current_delta = 0;
    if (!infer_current_delta(current_delta))
        return false;

    const int sp_offset = offset - current_delta;
    std::vector<asm_line> replacement;
    replacement.push_back(asm_line::parse(
        "\tld\thl, #" + std::to_string(sp_offset)));
    replacement.push_back(asm_line::parse("\tadd\thl, sp"));
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                 lines_.begin() + static_cast<std::ptrdiff_t>(end));
    lines_.insert(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                  replacement.begin(), replacement.end());
    return true;
}

bool z80_peep::rule_adjacent_byte_arg_push_pair(size_t i) {
    if (i + 5 >= lines_.size())
        return false;

    for (size_t j = i; j <= i + 5; ++j) {
        if (!lines_[j].label.empty())
            return false;
    }

    auto match_push_byte = [&](size_t idx, std::string &src_out) {
        if (idx + 2 >= lines_.size())
            return false;
        if (lines_[idx].mnemonic != "ld")
            return false;
        std::string dst;
        std::string src;
        if (!split_ld(lines_[idx].operands, dst, src) ||
            trim(dst) != "a") {
            return false;
        }
        if (lines_[idx + 1].mnemonic != "push" ||
            trim(lines_[idx + 1].operands) != "af" ||
            lines_[idx + 2].mnemonic != "inc" ||
            trim(lines_[idx + 2].operands) != "sp") {
            return false;
        }
        src = trim(src);
        if (operand_has_token(src, "a") || operand_has_token(src, "af") ||
            operand_has_token(src, "b") || operand_has_token(src, "bc") ||
            operand_has_token(src, "c")) {
            return false;
        }
        if (!is_reg8(src) && !is_immediate_operand(src) &&
            !is_numeric_literal(src) && !uses_ixiy_disp(src) &&
            !uses_hl_indirect(src)) {
            return false;
        }
        src_out = src;
        return true;
    };

    std::string first_src;
    std::string second_src;
    if (!match_push_byte(i, first_src) ||
        !match_push_byte(i + 3, second_src)) {
        return false;
    }

    auto bc_dead_after_pack = [&](size_t start) {
        const size_t end = std::min(lines_.size(), start + 32);
        for (size_t k = start; k < end; ++k) {
            const asm_line &line = lines_[k];
            if (line.mnemonic.empty())
                continue;
            if (!line.label.empty() || is_section_directive(line))
                return false;
            if (overwrites_pair_without_reading_it(lines_, k, "bc", 'c', 'b'))
                return true;
            if (line.mnemonic == "call")
                return true;
            if (line.mnemonic == "ret" && trim(line.operands).empty())
                return true;
            const std::string &m = line.mnemonic;
            if (m == "jp" || m == "jr" || m == "djnz" ||
                m == "rst" || m == "reti" || m == "retn" ||
                m == "exx" || m == "ldi" || m == "ldir" ||
                m == "ldd" || m == "lddr" || m == "cpi" ||
                m == "cpir" || m == "cpd" || m == "cpdr") {
                return false;
            }
            if (operand_mentions_pair_or_bytes(line.operands, "bc", 'c', 'b'))
                return false;
        }
        return false;
    };

    if (!a_dead_before_read_or_call(lines_, i + 6))
        return false;
    if (!bc_dead_after_pack(i + 6))
        return false;

    std::vector<asm_line> replacement;
    replacement.push_back(asm_line::parse("\tld\tb, " + first_src));
    replacement.push_back(asm_line::parse("\tld\tc, " + second_src));
    replacement.push_back(asm_line::parse("\tpush\tbc"));
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 6));
    lines_.insert(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                  replacement.begin(), replacement.end());
    return true;
}

bool z80_peep::rule_ix_local_byte_store_direct(size_t i) {
    if (i + 3 >= lines_.size())
        return false;

    for (size_t j = i; j <= i + 1; ++j) {
        if (!lines_[j].label.empty())
            return false;
    }

    if (lines_[i].mnemonic != "push" || trim(lines_[i].operands) != "ix")
        return false;
    if (lines_[i + 1].mnemonic != "pop" ||
        trim(lines_[i + 1].operands) != "hl") {
        return false;
    }

    std::string dst;
    std::string src;
    int base_offset = 0;
    size_t value_idx = i + 2;
    if (value_idx + 1 < lines_.size() &&
        lines_[value_idx].label.empty() &&
        lines_[value_idx + 1].label.empty() &&
        lines_[value_idx].mnemonic == "ld" &&
        split_ld(lines_[value_idx].operands, dst, src) &&
        trim(dst) == "bc" &&
        parse_immediate_value(src, base_offset) &&
        lines_[value_idx + 1].mnemonic == "add" &&
        split_ld(lines_[value_idx + 1].operands, dst, src) &&
        trim(dst) == "hl" && trim(src) == "bc") {
        value_idx += 2;
    }

    int offset_adjust = 0;
    while (value_idx < lines_.size() &&
           lines_[value_idx].label.empty() &&
           (lines_[value_idx].mnemonic == "inc" ||
            lines_[value_idx].mnemonic == "dec") &&
           trim(lines_[value_idx].operands) == "hl") {
        offset_adjust += lines_[value_idx].mnemonic == "inc" ? 1 : -1;
        if (offset_adjust > 64 || offset_adjust < -64)
            return false;
        ++value_idx;
    }
    if (value_idx >= lines_.size())
        return false;

    for (size_t j = i + 2; j <= value_idx; ++j) {
        if (!lines_[j].label.empty())
            return false;
    }

    const asm_line &value = lines_[value_idx];
    bool produces_a = false;
    bool value_already_in_a = false;
    size_t store_idx = value_idx + 1;
    if (value.mnemonic == "ld" &&
        split_ld(value.operands, dst, src) &&
        trim(dst) == "a" &&
        (is_immediate_operand(trim(src)) || is_numeric_literal(trim(src)))) {
        produces_a = true;
    } else if (is_xor_a_self(value)) {
        produces_a = true;
    } else if (value.mnemonic == "ld" &&
               split_ld(value.operands, dst, src) &&
               trim(dst) == "(hl)" && trim(src) == "a") {
        value_already_in_a = true;
        store_idx = value_idx;
    }
    if (!produces_a && !value_already_in_a)
        return false;

    if (produces_a) {
        if (store_idx >= lines_.size() ||
            !lines_[store_idx].label.empty() ||
            lines_[store_idx].mnemonic != "ld" ||
            !split_ld(lines_[store_idx].operands, dst, src) ||
            trim(dst) != "(hl)" || trim(src) != "a") {
            return false;
        }
    }

    const int final_offset = base_offset + offset_adjust;
    if (final_offset < -128 || final_offset > 127)
        return false;

    const size_t after = store_idx + 1;
    const bool next_push_ix_pop_hl =
        after + 1 < lines_.size() &&
        lines_[after].label.empty() &&
        lines_[after + 1].label.empty() &&
        lines_[after].mnemonic == "push" &&
        trim(lines_[after].operands) == "ix" &&
        lines_[after + 1].mnemonic == "pop" &&
        trim(lines_[after + 1].operands) == "hl";
    if (!next_push_ix_pop_hl &&
        !path_overwrites_hl_before_read(lines_, after)) {
        return false;
    }

    std::vector<asm_line> replacement;
    if (produces_a)
        replacement.push_back(value);
    replacement.push_back(asm_line::parse(
        "\tld\t" + std::to_string(final_offset) + "(ix), a"));

    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                 lines_.begin() + static_cast<std::ptrdiff_t>(store_idx + 1));
    lines_.insert(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                  replacement.begin(), replacement.end());
    return true;
}

bool z80_peep::rule_ix_temp_ptr_immediate_store_direct(size_t i) {
    if (i + 2 >= lines_.size())
        return false;

    for (size_t j = i; j <= i + 2; ++j) {
        if (!lines_[j].label.empty())
            return false;
    }

    std::string dst;
    std::string src;
    if (lines_[i].mnemonic != "ld" ||
        !split_ld(lines_[i].operands, dst, src) ||
        trim(dst) != "l") {
        return false;
    }
    int lo_offset = 0;
    if (!parse_ix_ref(trim(src), lo_offset))
        return false;

    if (lines_[i + 1].mnemonic != "ld" ||
        !split_ld(lines_[i + 1].operands, dst, src) ||
        trim(dst) != "h") {
        return false;
    }
    int hi_offset = 0;
    if (!parse_ix_ref(trim(src), hi_offset) ||
        hi_offset != lo_offset + 1) {
        return false;
    }

    if (lines_[i + 2].mnemonic != "ld" ||
        !split_ld(lines_[i + 2].operands, dst, src) ||
        trim(dst) != "(hl)" ||
        (!is_immediate_operand(trim(src)) && !is_numeric_literal(trim(src)))) {
        return false;
    }

    int locals = 0;
    int temp_frame = 0;
    size_t prologue_index = 0;
    if (!current_function_frame(lines_, i, locals, temp_frame, prologue_index))
        return false;
    if (!ix_offset_in_temp_frame(lo_offset, locals, temp_frame) ||
        !ix_offset_in_temp_frame(hi_offset, locals, temp_frame)) {
        return false;
    }

    int ix_offset = 0;
    const int sp_ix_delta = -(locals + temp_frame);
    if (!find_frameaddr_temp_word_before_use(lines_, prologue_index, i,
                                             lo_offset, sp_ix_delta,
                                             ix_offset)) {
        return false;
    }

    const size_t after = i + 3;
    if (!straightline_overwrites_hl_before_read_allowing_sp(lines_, after))
        return false;

    lines_[i] = asm_line::parse(
        "\tld\t" + std::to_string(ix_offset) + "(ix), " + trim(src));
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 1),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 3));
    return true;
}

bool z80_peep::rule_dead_hl_sp_frameaddr_calc(size_t i) {
    if (i + 1 >= lines_.size())
        return false;
    if (!lines_[i].label.empty() || !lines_[i + 1].label.empty())
        return false;

    std::string dst;
    std::string src;
    int ignored = 0;
    if (lines_[i].mnemonic != "ld" ||
        !split_ld(lines_[i].operands, dst, src) ||
        trim(dst) != "hl" ||
        !parse_immediate_value(src, ignored)) {
        return false;
    }
    if (lines_[i + 1].mnemonic != "add" ||
        !split_ld(lines_[i + 1].operands, dst, src) ||
        trim(dst) != "hl" || trim(src) != "sp") {
        return false;
    }

    size_t after = i + 2;
    while (after < lines_.size() &&
           lines_[after].label.empty() &&
           (lines_[after].mnemonic == "inc" ||
            lines_[after].mnemonic == "dec") &&
           trim(lines_[after].operands) == "hl") {
        ++after;
    }

    size_t check = after;
    while (check < lines_.size() &&
           !lines_[check].label.empty() &&
           lines_[check].mnemonic.empty()) {
        ++check;
    }

    if (!straightline_overwrites_hl_before_read_allowing_sp(lines_, check))
        return false;

    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                 lines_.begin() + static_cast<std::ptrdiff_t>(after));
    return true;
}

static bool ix_slot_referenced_after(const std::vector<asm_line> &lines,
                                     size_t start,
                                     int offset) {
    const std::string ref = std::to_string(offset) + "(ix)";
    for (size_t j = start; j < lines.size(); ++j) {
        if (lines[j].operands.find(ref) != std::string::npos)
            return true;
    }
    return false;
}

static size_t function_end_after_prologue(const std::vector<asm_line> &lines,
                                          size_t prologue_index) {
    for (size_t k = prologue_index + 1; k < lines.size(); ++k) {
        int ignored_locals = 0;
        int ignored_temp_frame = 0;
        if (parse_frame_comment(lines[k].comment, ignored_locals,
                                ignored_temp_frame) ||
            is_section_directive(lines[k])) {
            return k;
        }
    }
    return lines.size();
}

static bool operand_is_ix_offset(const std::string &operand, int offset) {
    int parsed = 0;
    return parse_ix_ref(trim(operand), parsed) && parsed == offset;
}

static bool operands_read_ix_offset(const std::string &operands, int offset) {
    size_t start = 0;
    while (start <= operands.size()) {
        const size_t comma = operands.find(',', start);
        const size_t end = comma == std::string::npos ? operands.size() : comma;
        if (operand_is_ix_offset(operands.substr(start, end - start), offset))
            return true;
        if (comma == std::string::npos)
            break;
        start = comma + 1;
    }
    return false;
}

static bool line_reads_ix_offset(const asm_line &line, int offset) {
    if (line.mnemonic.empty())
        return false;
    if (outline_helper_may_access_ix_offset(line, offset))
        return true;

    if (line.mnemonic == "ld") {
        std::string dst;
        std::string src;
        if (!split_ld(line.operands, dst, src))
            return operands_read_ix_offset(line.operands, offset);
        return operand_is_ix_offset(src, offset);
    }

    if (line.mnemonic == "inc" || line.mnemonic == "dec")
        return operand_is_ix_offset(line.operands, offset);

    return operands_read_ix_offset(line.operands, offset);
}

static bool ix_slot_read_in_function(const std::vector<asm_line> &lines,
                                     size_t begin,
                                     size_t end,
                                     int offset,
                                     size_t ignore_index) {
    for (size_t k = begin; k < end; ++k) {
        if (k == ignore_index)
            continue;
        if (line_reads_ix_offset(lines[k], offset))
            return true;
    }
    return false;
}

static bool line_branches_to_before(const std::vector<asm_line> &lines,
                                    size_t line_index,
                                    size_t before_index) {
    if (line_index >= lines.size())
        return false;

    const asm_line &line = lines[line_index];
    std::string target;
    std::string cc;
    bool has_target = split_conditional_branch_target(line, cc, target) ||
                      parse_unconditional_jump(line, target);
    if (!has_target && line.mnemonic == "djnz") {
        target = trim(line.operands);
        has_target = !target.empty();
    }
    if (!has_target)
        return false;

    const size_t target_index = find_label_index(lines, target);
    return target_index != lines.size() &&
           target_index < before_index &&
           target_index < line_index;

}

static bool ix_slot_not_read_before_rewrite(const std::vector<asm_line> &lines,
                                            size_t begin,
                                            size_t end,
                                            int offset) {
    for (size_t k = begin; k < end; ++k) {
        if (line_branches_to_before(lines, k, begin))
            return false;
        if (line_reads_ix_offset(lines[k], offset))
            return false;
        if (line_writes_ix_offset(lines[k], offset))
            return true;
    }
    return true;
}

static bool ix_value_may_be_read_before_rewrite(
        const std::vector<asm_line> &lines,
        size_t function_begin,
        size_t function_end,
        size_t start,
        int offset) {
    if (function_begin >= function_end || start >= function_end)
        return false;

    const size_t count = function_end - function_begin;
    std::vector<unsigned char> live_in(count, 0);

    auto target_is_live = [&](const std::string &target) {
        const size_t target_index = find_label_index(lines, target);
        return target_index >= function_begin && target_index < function_end &&
               live_in[target_index - function_begin] != 0;
    };

    bool changed = true;
    while (changed) {
        changed = false;
        for (size_t scan = function_end; scan > function_begin; --scan) {
            const size_t k = scan - 1;
            const asm_line &line = lines[k];
            bool needed = line_reads_ix_offset(line, offset);

            if (!needed && !line_writes_ix_offset(line, offset)) {
                std::string cc;
                std::string target;
                if (split_conditional_branch_target(line, cc, target)) {
                    needed = target_is_live(target) ||
                             (k + 1 < function_end &&
                              live_in[k + 1 - function_begin] != 0);
                } else if (line.mnemonic == "djnz") {
                    target = trim(line.operands);
                    needed = target_is_live(target) ||
                             (k + 1 < function_end &&
                              live_in[k + 1 - function_begin] != 0);
                } else if (parse_unconditional_jump(line, target)) {
                    const size_t target_index = find_label_index(lines, target);
                    if (target_index >= function_begin &&
                        target_index < function_end) {
                        needed = live_in[target_index - function_begin] != 0;
                    } else if (!target.empty() && target.front() == '(') {
                        // An indirect jump may select any local jump-table
                        // destination.  Joining all in-function states is
                        // conservative without requiring table recognition.
                        needed = std::any_of(live_in.begin(), live_in.end(),
                                             [](unsigned char value) {
                                                 return value != 0;
                                             });
                    }
                } else if (line.mnemonic == "ret") {
                    // A conditional return can still fall through.
                    needed = !trim(line.operands).empty() &&
                             k + 1 < function_end &&
                             live_in[k + 1 - function_begin] != 0;
                } else if (line.mnemonic != "reti" &&
                           line.mnemonic != "retn") {
                    needed = k + 1 < function_end &&
                             live_in[k + 1 - function_begin] != 0;
                }
            }

            unsigned char &entry = live_in[k - function_begin];
            if (needed && entry == 0) {
                entry = 1;
                changed = true;
            }
        }
    }

    return live_in[start - function_begin] != 0;
}

bool z80_peep::rule_byte_temp_zero_extend_after_test(size_t i) {
    if (i + 5 >= lines_.size())
        return false;
    if (lines_[i + 1].label.empty() == false ||
        lines_[i + 2].label.empty() == false ||
        lines_[i + 3].label.empty() == false) {
        return false;
    }

    std::string dst;
    std::string src;
    if (lines_[i].mnemonic != "ld" ||
        !split_ld(lines_[i].operands, dst, src) ||
        trim(dst) != "a" || trim(src) != "(hl)") {
        return false;
    }

    int temp_off = 0;
    if (lines_[i + 1].mnemonic != "ld" ||
        !split_ld(lines_[i + 1].operands, dst, src) ||
        trim(src) != "a" ||
        !parse_ix_ref(trim(dst), temp_off)) {
        return false;
    }

    if (!is_or_a_self(lines_[i + 2]))
        return false;

    std::string cc;
    std::string target;
    if (!split_conditional_branch_target(lines_[i + 3], cc, target))
        return false;

    size_t reload_idx = i + 4;
    while (reload_idx < lines_.size() &&
           lines_[reload_idx].mnemonic.empty() &&
           !lines_[reload_idx].label.empty()) {
        if (label_has_other_control_references(lines_,
                                               lines_[reload_idx].label,
                                               lines_.size())) {
            return false;
        }
        ++reload_idx;
    }
    if (reload_idx + 1 >= lines_.size())
        return false;
    if (!lines_[reload_idx].label.empty() ||
        !lines_[reload_idx + 1].label.empty()) {
        return false;
    }

    int reload_off = 0;
    if (lines_[reload_idx].mnemonic != "ld" ||
        !split_ld(lines_[reload_idx].operands, dst, src) ||
        trim(dst) != "l" ||
        !parse_ix_ref(trim(src), reload_off) ||
        reload_off != temp_off) {
        return false;
    }

    if (lines_[reload_idx + 1].mnemonic != "ld" ||
        !split_ld(lines_[reload_idx + 1].operands, dst, src) ||
        trim(dst) != "h" ||
        !immediate_is(trim(src), 0)) {
        return false;
    }

    int locals = 0;
    int temp_frame = 0;
    size_t prologue_index = 0;
    if (!current_function_frame(lines_, i, locals, temp_frame, prologue_index))
        return false;
    if (!ix_offset_in_temp_frame(temp_off, locals, temp_frame))
        return false;

    const size_t fn_end = function_end_after_prologue(lines_, prologue_index);
    if (!ix_slot_not_read_before_rewrite(lines_, reload_idx + 2, fn_end,
                                         temp_off)) {
        return false;
    }

    lines_[reload_idx].operands = "l, a";
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 1));
    return true;
}

static bool is_add_a_a(const asm_line &line) {
    if (line.mnemonic != "add")
        return false;
    const std::string ops = trim(line.operands);
    return ops == "a, a" || ops == "a,a";
}

static bool is_srl_a(const asm_line &line) {
    return line.mnemonic == "srl" && trim(line.operands) == "a";
}

bool z80_peep::rule_ix_byte_left_shift_xor_temp_elide(size_t i) {
    if (i + 9 >= lines_.size()) return false;
    for (size_t j = i + 1; j <= i + 9; ++j) {
        if (!lines_[j].label.empty())
            return false;
    }

    std::string dst, src;
    if (lines_[i].mnemonic != "ld" ||
        !split_ld(lines_[i].operands, dst, src) ||
        trim(dst) != "a") {
        return false;
    }
    int seed_off = 0;
    if (!parse_ix_ref(trim(src), seed_off))
        return false;
    if (!is_add_a_a(lines_[i + 1]) ||
        !is_add_a_a(lines_[i + 2]) ||
        !is_add_a_a(lines_[i + 3])) {
        return false;
    }
    if (lines_[i + 4].mnemonic != "ld" ||
        !split_ld(lines_[i + 4].operands, dst, src) ||
        trim(src) != "a") {
        return false;
    }
    int temp_off = 0;
    if (!parse_ix_ref(trim(dst), temp_off))
        return false;
    if (lines_[i + 5].mnemonic != "ld" ||
        !split_ld(lines_[i + 5].operands, dst, src) ||
        trim(dst) != "e") {
        return false;
    }
    int reload_seed_off = 0;
    if (!parse_ix_ref(trim(src), reload_seed_off) ||
        reload_seed_off != seed_off) {
        return false;
    }
    if (lines_[i + 6].mnemonic != "ld" ||
        !split_ld(lines_[i + 6].operands, dst, src) ||
        trim(dst) != "d") {
        return false;
    }
    int reload_temp_off = 0;
    if (!parse_ix_ref(trim(src), reload_temp_off) ||
        reload_temp_off != temp_off) {
        return false;
    }
    if (lines_[i + 7].mnemonic != "ld" ||
        !split_ld(lines_[i + 7].operands, dst, src) ||
        trim(dst) != "a" || trim(src) != "e") {
        return false;
    }
    if (!is_accumulator_reg_alu(lines_[i + 8], "xor", "d"))
        return false;
    if (lines_[i + 9].mnemonic != "ld" ||
        !split_ld(lines_[i + 9].operands, dst, src) ||
        trim(src) != "a") {
        return false;
    }
    int store_seed_off = 0;
    if (!parse_ix_ref(trim(dst), store_seed_off) ||
        store_seed_off != seed_off) {
        return false;
    }
    if (ix_slot_referenced_after(lines_, i + 10, temp_off))
        return false;

    const asm_line add0 = lines_[i + 1];
    const asm_line add1 = lines_[i + 2];
    const asm_line add2 = lines_[i + 3];
    const asm_line store_seed = lines_[i + 9];
    lines_[i + 1] = asm_line::parse("\tld\te, a");
    lines_[i + 2] = add0;
    lines_[i + 3] = add1;
    lines_[i + 4] = add2;
    lines_[i + 5] = asm_line::parse("\tld\td, a");
    lines_[i + 6] = asm_line::parse("\txor\te");
    lines_[i + 7] = store_seed;
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 8),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 10));
    return true;
}

bool z80_peep::rule_ix_byte_right_shift_xor_temp_elide(size_t i) {
    if (i + 10 >= lines_.size()) return false;
    for (size_t j = i + 1; j <= i + 10; ++j) {
        if (!lines_[j].label.empty())
            return false;
    }

    std::string dst, src;
    if (lines_[i].mnemonic != "ld" ||
        !split_ld(lines_[i].operands, dst, src) ||
        trim(src) != "a") {
        return false;
    }
    int seed_off = 0;
    if (!parse_ix_ref(trim(dst), seed_off))
        return false;
    for (size_t k = 1; k <= 5; ++k) {
        if (!is_srl_a(lines_[i + k]))
            return false;
    }
    if (lines_[i + 6].mnemonic != "ld" ||
        !split_ld(lines_[i + 6].operands, dst, src) ||
        trim(src) != "a") {
        return false;
    }
    int temp_off = 0;
    if (!parse_ix_ref(trim(dst), temp_off))
        return false;
    if (lines_[i + 7].mnemonic != "ld" ||
        !split_ld(lines_[i + 7].operands, dst, src) ||
        trim(dst) != "e") {
        return false;
    }
    int reload_seed_off = 0;
    if (!parse_ix_ref(trim(src), reload_seed_off) ||
        reload_seed_off != seed_off) {
        return false;
    }
    if (lines_[i + 8].mnemonic != "ld" ||
        !split_ld(lines_[i + 8].operands, dst, src) ||
        trim(dst) != "d") {
        return false;
    }
    int reload_temp_off = 0;
    if (!parse_ix_ref(trim(src), reload_temp_off) ||
        reload_temp_off != temp_off) {
        return false;
    }
    if (lines_[i + 9].mnemonic != "ld" ||
        !split_ld(lines_[i + 9].operands, dst, src) ||
        trim(dst) != "a" || trim(src) != "e") {
        return false;
    }
    if (!is_accumulator_reg_alu(lines_[i + 10], "xor", "d"))
        return false;
    if (i + 11 >= lines_.size() || !lines_[i + 11].label.empty() ||
        lines_[i + 11].mnemonic != "ld" ||
        !split_ld(lines_[i + 11].operands, dst, src) ||
        trim(src) != "a") {
        return false;
    }
    int store_seed_off = 0;
    if (!parse_ix_ref(trim(dst), store_seed_off) ||
        store_seed_off != seed_off) {
        return false;
    }
    if (ix_slot_referenced_after(lines_, i + 12, temp_off))
        return false;

    const asm_line store_seed_before_shift = lines_[i];
    const asm_line srl0 = lines_[i + 1];
    const asm_line srl1 = lines_[i + 2];
    const asm_line srl2 = lines_[i + 3];
    const asm_line srl3 = lines_[i + 4];
    const asm_line srl4 = lines_[i + 5];
    const asm_line store_seed_after_xor = lines_[i + 11];
    lines_[i] = store_seed_before_shift;
    lines_[i + 1] = asm_line::parse("\tld\te, a");
    lines_[i + 2] = srl0;
    lines_[i + 3] = srl1;
    lines_[i + 4] = srl2;
    lines_[i + 5] = srl3;
    lines_[i + 6] = srl4;
    lines_[i + 7] = asm_line::parse("\tld\td, a");
    lines_[i + 8] = asm_line::parse("\txor\te");
    lines_[i + 9] = store_seed_after_xor;
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 10),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 12));
    return true;
}

bool z80_peep::rule_truncated_promoted_byte_xor_elide(size_t i) {
    if (i + 24 >= lines_.size())
        return false;

    for (size_t j = i; j <= i + 24; ++j) {
        if (!lines_[j].label.empty())
            return false;
    }

    auto match_ld = [&](size_t idx, const char *want_dst, const char *want_src) {
        if (idx >= lines_.size() || lines_[idx].mnemonic != "ld")
            return false;
        std::string dst;
        std::string src;
        if (!split_ld(lines_[idx].operands, dst, src))
            return false;
        return trim(dst) == want_dst && trim(src) == want_src;
    };
    auto match_ld_ix_to_a = [&](size_t idx, int &off) {
        if (idx >= lines_.size() || lines_[idx].mnemonic != "ld")
            return false;
        std::string dst;
        std::string src;
        if (!split_ld(lines_[idx].operands, dst, src) || trim(dst) != "a")
            return false;
        return parse_ix_ref(trim(src), off);
    };
    auto match_ld_a_to_ix = [&](size_t idx, int &off) {
        if (idx >= lines_.size() || lines_[idx].mnemonic != "ld")
            return false;
        std::string dst;
        std::string src;
        if (!split_ld(lines_[idx].operands, dst, src) || trim(src) != "a")
            return false;
        return parse_ix_ref(trim(dst), off);
    };
    auto match_sbc_a_a = [&](size_t idx) {
        return idx < lines_.size() && lines_[idx].mnemonic == "sbc" &&
               (trim(lines_[idx].operands) == "a, a" ||
                trim(lines_[idx].operands) == "a,a");
    };
    auto match_sign_extend_ix_byte_to_hl = [&](size_t start, int &off) {
        return match_ld_ix_to_a(start, off) &&
               match_ld(start + 1, "l", "a") &&
               lines_[start + 2].mnemonic == "rlca" &&
               match_sbc_a_a(start + 3) &&
               match_ld(start + 4, "h", "a");
    };

    int dead_left_off = 0;
    int right_off = 0;
    int left_off = 0;
    int dst_off = 0;
    if (!match_sign_extend_ix_byte_to_hl(i, dead_left_off) ||
        !match_sign_extend_ix_byte_to_hl(i + 5, right_off) ||
        !match_ld(i + 10, "b", "h") ||
        !match_ld(i + 11, "c", "l") ||
        !match_sign_extend_ix_byte_to_hl(i + 12, left_off) ||
        !match_ld(i + 17, "a", "l") ||
        lines_[i + 18].mnemonic != "xor" ||
        (trim(lines_[i + 18].operands) != "a, c" &&
         trim(lines_[i + 18].operands) != "a,c") ||
        !match_ld(i + 19, "l", "a") ||
        !match_ld(i + 20, "a", "h") ||
        lines_[i + 21].mnemonic != "xor" ||
        (trim(lines_[i + 21].operands) != "a, b" &&
         trim(lines_[i + 21].operands) != "a,b") ||
        !match_ld(i + 22, "h", "a") ||
        !match_ld(i + 23, "a", "l") ||
        !match_ld_a_to_ix(i + 24, dst_off)) {
        return false;
    }
    if (dead_left_off != left_off)
        return false;
    const std::string left_ref = std::to_string(left_off) + "(ix)";
    const std::string right_ref = std::to_string(right_off) + "(ix)";
    const std::string dst_ref = std::to_string(dst_off) + "(ix)";
    lines_[i] = asm_line::parse("\tld\ta, " + left_ref);
    lines_[i + 1] = asm_line::parse("\txor\ta, " + right_ref);
    lines_[i + 2] = asm_line::parse("\tld\t" + dst_ref + ", a");
    lines_[i + 3] = asm_line::parse("\tld\tl, a");
    lines_[i + 4] = asm_line::parse("\trlca");
    lines_[i + 5] = asm_line::parse("\tsbc\ta, a");
    lines_[i + 6] = asm_line::parse("\tld\th, a");
    lines_[i + 7] = asm_line::parse("\tor\ta, a");
    lines_[i + 8] = asm_line::parse("\tld\ta, l");
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 9),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 25));
    return true;
}

bool z80_peep::rule_ix_word_temp_switch_key_de_direct(size_t i) {
    if (i + 7 >= lines_.size())
        return false;

    std::vector<size_t> pos;
    pos.reserve(8);
    for (size_t j = i; j < lines_.size() && pos.size() < 8; ++j) {
        if (!lines_[j].label.empty())
            return false;
        if (lines_[j].mnemonic.empty())
            continue;
        pos.push_back(j);
    }
    if (pos.size() < 8 || pos[0] != i || pos[1] != i + 1)
        return false;

    auto match_ld = [&](size_t idx, const char *want_dst, const char *want_src) {
        if (idx >= lines_.size() || lines_[idx].mnemonic != "ld")
            return false;
        std::string dst;
        std::string src;
        if (!split_ld(lines_[idx].operands, dst, src))
            return false;
        return trim(dst) == want_dst && trim(src) == want_src;
    };
    auto parse_ld_ix_dst = [&](size_t idx, const char *want_src, int &off) {
        if (idx >= lines_.size() || lines_[idx].mnemonic != "ld")
            return false;
        std::string dst;
        std::string src;
        if (!split_ld(lines_[idx].operands, dst, src) || trim(src) != want_src)
            return false;
        return parse_ix_ref(trim(dst), off);
    };
    auto parse_ld_ix_src = [&](size_t idx, const char *want_dst, int &off) {
        if (idx >= lines_.size() || lines_[idx].mnemonic != "ld")
            return false;
        std::string dst;
        std::string src;
        if (!split_ld(lines_[idx].operands, dst, src) || trim(dst) != want_dst)
            return false;
        return parse_ix_ref(trim(src), off);
    };

    int temp_lo = 0;
    int temp_hi = 0;
    int reload_lo = 0;
    int reload_hi = 0;
    if (!parse_ld_ix_dst(pos[0], "e", temp_lo) ||
        !parse_ld_ix_dst(pos[1], "d", temp_hi) ||
        !parse_ld_ix_src(pos[2], "l", reload_lo) ||
        !parse_ld_ix_src(pos[3], "h", reload_hi) ||
        reload_lo != temp_lo ||
        reload_hi != temp_hi ||
        !match_ld(pos[4], "a", "h") ||
        !is_or_a_self(lines_[pos[5]]) ||
        !match_ld(pos[7], "a", "l")) {
        return false;
    }

    std::string cc;
    std::string target;
    if (!split_conditional_branch_target(lines_[pos[6]], cc, target) ||
        cc != "nz") {
        return false;
    }

    int locals = 0;
    int temp_frame = 0;
    size_t prologue_index = 0;
    if (!current_function_frame(lines_, i, locals, temp_frame, prologue_index))
        return false;
    if (!ix_offset_in_temp_frame(temp_lo, locals, temp_frame) ||
        !ix_offset_in_temp_frame(temp_hi, locals, temp_frame)) {
        return false;
    }
    const size_t fn_end = function_end_after_prologue(lines_, prologue_index);
    if (!ix_slot_not_read_before_rewrite(lines_, pos[7] + 1, fn_end, temp_lo) ||
        !ix_slot_not_read_before_rewrite(lines_, pos[7] + 1, fn_end, temp_hi)) {
        return false;
    }

    lines_[i] = asm_line::parse("\tld\ta, d");
    lines_[i + 1] = lines_[pos[5]];
    lines_[i + 2] = lines_[pos[6]];
    lines_[i + 3] = asm_line::parse("\tld\ta, e");
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 4),
                 lines_.begin() + static_cast<std::ptrdiff_t>(pos[7] + 1));
    return true;
}

bool z80_peep::rule_de_word_temp_reload_to_hl_stack_preserve(size_t i) {
    if (i + 4 >= lines_.size())
        return false;
    if (!lines_[i].label.empty() || !lines_[i + 1].label.empty() ||
        lines_[i].mnemonic != "ld" || lines_[i + 1].mnemonic != "ld") {
        return false;
    }

    std::string dst;
    std::string src;
    int temp_lo = 0;
    int temp_hi = 0;
    if (!split_ld(lines_[i].operands, dst, src) ||
        trim(src) != "e" ||
        !parse_ix_ref(trim(dst), temp_lo) ||
        !split_ld(lines_[i + 1].operands, dst, src) ||
        trim(src) != "d" ||
        !parse_ix_ref(trim(dst), temp_hi)) {
        return false;
    }

    int locals = 0;
    int temp_frame = 0;
    size_t prologue_index = 0;
    if (!current_function_frame(lines_, i, locals, temp_frame, prologue_index))
        return false;
    if (!ix_offset_in_temp_frame(temp_lo, locals, temp_frame) ||
        !ix_offset_in_temp_frame(temp_hi, locals, temp_frame)) {
        return false;
    }
    const size_t fn_end = function_end_after_prologue(lines_, prologue_index);

    auto span_preserves_sp = [&](const asm_line &line) {
        if (!line.label.empty())
            return false;
        if (line.mnemonic.empty())
            return true;
        if (!line.mnemonic.empty() && line.mnemonic[0] == '.')
            return line.mnemonic == ".globl" || line.mnemonic == ".global";
        if (line.mnemonic == "push" || line.mnemonic == "pop" ||
            line.mnemonic == "call" || line.mnemonic == "ret" ||
            line.mnemonic == "rst" || line.mnemonic == "jp" ||
            line.mnemonic == "jr" || line.mnemonic == "djnz") {
            return false;
        }
        return line.operands.find("sp") == std::string::npos;
    };

    size_t reload = lines_.size();
    const size_t end = std::min(fn_end, i + 48);
    for (size_t k = i + 2; k + 1 < end; ++k) {
        if (lines_[k].mnemonic == "ld" &&
            lines_[k + 1].mnemonic == "ld" &&
            lines_[k + 1].label.empty()) {
            std::string d0, s0, d1, s1;
            int reload_lo = 0;
            int reload_hi = 0;
            if (split_ld(lines_[k].operands, d0, s0) &&
                split_ld(lines_[k + 1].operands, d1, s1) &&
                trim(d0) == "l" &&
                trim(d1) == "h" &&
                parse_ix_ref(trim(s0), reload_lo) &&
                parse_ix_ref(trim(s1), reload_hi) &&
                reload_lo == temp_lo &&
                reload_hi == temp_hi) {
                reload = k;
                break;
            }
        }

        if (line_reads_ix_offset(lines_[k], temp_lo) ||
            line_reads_ix_offset(lines_[k], temp_hi) ||
            line_writes_ix_offset(lines_[k], temp_lo) ||
            line_writes_ix_offset(lines_[k], temp_hi) ||
            !span_preserves_sp(lines_[k])) {
            return false;
        }
    }
    if (reload == lines_.size())
        return false;

    if (!ix_slot_not_read_before_rewrite(lines_, reload + 2, fn_end, temp_lo) ||
        !ix_slot_not_read_before_rewrite(lines_, reload + 2, fn_end, temp_hi)) {
        return false;
    }

    lines_[i] = asm_line::parse("\tpush\tde");
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 1));
    const size_t adjusted_reload = reload - 1;
    lines_[adjusted_reload] = asm_line::parse("\tpop\thl");
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(adjusted_reload + 1));
    return true;
}

bool z80_peep::rule_de_word_temp_reload_after_address_calc_elide(size_t i) {
    if (i + 5 >= lines_.size())
        return false;
    if (!lines_[i].label.empty() || !lines_[i + 1].label.empty() ||
        lines_[i].mnemonic != "ld" || lines_[i + 1].mnemonic != "ld") {
        return false;
    }

    std::string dst;
    std::string src;
    int temp_lo = 0;
    int temp_hi = 0;
    if (!split_ld(lines_[i].operands, dst, src) ||
        trim(src) != "e" ||
        !parse_ix_ref(trim(dst), temp_lo) ||
        !split_ld(lines_[i + 1].operands, dst, src) ||
        trim(src) != "d" ||
        !parse_ix_ref(trim(dst), temp_hi)) {
        return false;
    }

    int locals = 0;
    int temp_frame = 0;
    size_t prologue_index = 0;
    if (!current_function_frame(lines_, i, locals, temp_frame, prologue_index))
        return false;
    if (!ix_offset_in_temp_frame(temp_lo, locals, temp_frame) ||
        !ix_offset_in_temp_frame(temp_hi, locals, temp_frame)) {
        return false;
    }
    const size_t fn_end = function_end_after_prologue(lines_, prologue_index);

    size_t reload = lines_.size();
    const size_t end = std::min(lines_.size(), i + 18);
    for (size_t j = i + 2; j + 1 < end; ++j) {
        if (lines_[j].mnemonic.empty())
            continue;
        if (!lines_[j].label.empty())
            return false;

        std::string d0, s0, d1, s1;
        int reload_lo = 0;
        int reload_hi = 0;
        if (lines_[j].mnemonic == "ld" &&
            lines_[j + 1].mnemonic == "ld" &&
            lines_[j + 1].label.empty() &&
            split_ld(lines_[j].operands, d0, s0) &&
            split_ld(lines_[j + 1].operands, d1, s1) &&
            trim(d0) == "e" &&
            trim(d1) == "d" &&
            parse_ix_ref(trim(s0), reload_lo) &&
            parse_ix_ref(trim(s1), reload_hi) &&
            reload_lo == temp_lo &&
            reload_hi == temp_hi) {
            reload = j;
            break;
        }

        if (!line_preserves_pair_and_sp(lines_[j], "de", 'e', 'd'))
            return false;
    }
    if (reload == lines_.size())
        return false;

    if (!ix_slot_not_read_before_rewrite(lines_, reload + 2, fn_end, temp_lo) ||
        !ix_slot_not_read_before_rewrite(lines_, reload + 2, fn_end, temp_hi)) {
        return false;
    }

    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(reload),
                 lines_.begin() + static_cast<std::ptrdiff_t>(reload + 2));
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 2));
    return true;
}

bool z80_peep::rule_hl_word_temp_reload_after_address_calc_elide(size_t i) {
    if (i + 9 >= lines_.size())
        return false;
    if (!lines_[i].label.empty() || !lines_[i + 1].label.empty() ||
        lines_[i].mnemonic != "ld" || lines_[i + 1].mnemonic != "ld") {
        return false;
    }

    std::string dst;
    std::string src;
    int temp_lo = 0;
    int temp_hi = 0;
    if (!split_ld(lines_[i].operands, dst, src) ||
        trim(src) != "l" ||
        !parse_ix_ref(trim(dst), temp_lo) ||
        !split_ld(lines_[i + 1].operands, dst, src) ||
        trim(src) != "h" ||
        !parse_ix_ref(trim(dst), temp_hi)) {
        return false;
    }

    int locals = 0;
    int temp_frame = 0;
    size_t prologue_index = 0;
    if (!current_function_frame(lines_, i, locals, temp_frame, prologue_index))
        return false;
    if (!ix_offset_in_temp_frame(temp_lo, locals, temp_frame) ||
        !ix_offset_in_temp_frame(temp_hi, locals, temp_frame)) {
        return false;
    }

    size_t p = i + 2;
    int index_lo = 0;
    int index_hi = 0;
    if (p + 1 >= lines_.size() ||
        !lines_[p].label.empty() ||
        !lines_[p + 1].label.empty() ||
        lines_[p].mnemonic != "ld" ||
        lines_[p + 1].mnemonic != "ld" ||
        !split_ld(lines_[p].operands, dst, src) ||
        trim(dst) != "l" ||
        !parse_ix_ref(trim(src), index_lo) ||
        !split_ld(lines_[p + 1].operands, dst, src) ||
        trim(dst) != "h" ||
        !parse_ix_ref(trim(src), index_hi)) {
        return false;
    }
    p += 2;

    if (p < lines_.size() &&
        (lines_[p].mnemonic == "inc" || lines_[p].mnemonic == "dec") &&
        trim(lines_[p].operands) == "hl") {
        if (!lines_[p].label.empty())
            return false;
        ++p;
    }

    if (p + 1 < lines_.size() &&
        lines_[p].mnemonic == "ld" &&
        lines_[p + 1].mnemonic == "ld" &&
        lines_[p].label.empty() &&
        lines_[p + 1].label.empty()) {
        std::string d0, s0, d1, s1;
        int write_lo = 0;
        int write_hi = 0;
        if (split_ld(lines_[p].operands, d0, s0) &&
            split_ld(lines_[p + 1].operands, d1, s1) &&
            parse_ix_ref(trim(d0), write_lo) &&
            parse_ix_ref(trim(d1), write_hi) &&
            write_lo == index_lo &&
            write_hi == index_hi &&
            trim(s0) == "l" &&
            trim(s1) == "h") {
            p += 2;
        }
    }

    if (p + 7 >= lines_.size())
        return false;
    const size_t add_scale_pos = p;
    const size_t base_pos = p + 1;
    const size_t add_base_pos = p + 2;
    const size_t reload_lo_pos = p + 3;
    const size_t reload_hi_pos = p + 4;
    const size_t store_lo_pos = p + 5;
    const size_t inc_pos = p + 6;
    const size_t store_hi_pos = p + 7;
    for (size_t j = i; j <= store_hi_pos; ++j) {
        if (!lines_[j].label.empty())
            return false;
    }

    const std::string scale_ops = trim(lines_[add_scale_pos].operands);
    if (lines_[add_scale_pos].mnemonic != "add" ||
        (scale_ops != "hl, hl" && scale_ops != "hl,hl")) {
        return false;
    }

    std::string base_dst;
    std::string base_src;
    if (lines_[base_pos].mnemonic != "ld" ||
        !split_ld(lines_[base_pos].operands, base_dst, base_src)) {
        return false;
    }
    base_dst = trim(base_dst);
    base_src = trim(base_src);
    if ((base_dst != "de" && base_dst != "bc") ||
        (!is_immediate_operand(base_src) && !is_numeric_literal(base_src))) {
        return false;
    }

    const std::string add_base_ops = trim(lines_[add_base_pos].operands);
    if (lines_[add_base_pos].mnemonic != "add" ||
        !((base_dst == "de" &&
           (add_base_ops == "hl, de" || add_base_ops == "hl,de")) ||
          (base_dst == "bc" &&
           (add_base_ops == "hl, bc" || add_base_ops == "hl,bc")))) {
        return false;
    }

    int reload_lo = 0;
    int reload_hi = 0;
    if (lines_[reload_lo_pos].mnemonic != "ld" ||
        !split_ld(lines_[reload_lo_pos].operands, dst, src) ||
        trim(dst) != "e" ||
        !parse_ix_ref(trim(src), reload_lo) ||
        reload_lo != temp_lo ||
        lines_[reload_hi_pos].mnemonic != "ld" ||
        !split_ld(lines_[reload_hi_pos].operands, dst, src) ||
        trim(dst) != "d" ||
        !parse_ix_ref(trim(src), reload_hi) ||
        reload_hi != temp_hi) {
        return false;
    }

    auto match_hl_store = [&](const asm_line &line, const char *want_src) {
        if (line.mnemonic != "ld")
            return false;
        std::string store_dst;
        std::string store_src;
        if (!split_ld(line.operands, store_dst, store_src))
            return false;
        return trim(store_dst) == "(hl)" && trim(store_src) == want_src;
    };

    if (!match_hl_store(lines_[store_lo_pos], "e") ||
        lines_[inc_pos].mnemonic != "inc" ||
        trim(lines_[inc_pos].operands) != "hl" ||
        !match_hl_store(lines_[store_hi_pos], "d")) {
        return false;
    }

    const size_t fn_end = function_end_after_prologue(lines_, prologue_index);
    if (!ix_slot_not_read_before_rewrite(lines_, store_hi_pos + 1, fn_end, temp_lo) ||
        !ix_slot_not_read_before_rewrite(lines_, store_hi_pos + 1, fn_end, temp_hi)) {
        return false;
    }
    if (base_dst == "de" &&
        !path_overwrites_bc_before_read_or_call(lines_, store_hi_pos + 1)) {
        return false;
    }

    std::vector<asm_line> repl;
    repl.reserve(store_hi_pos - i + 1);
    repl.push_back(asm_line::parse("\tex\tde, hl"));
    for (size_t j = i + 2; j <= add_scale_pos; ++j)
        repl.push_back(lines_[j]);
    repl.push_back(asm_line::parse("\tld\tbc, " + base_src));
    repl.push_back(asm_line::parse("\tadd\thl, bc"));
    repl.push_back(lines_[store_lo_pos]);
    repl.push_back(lines_[inc_pos]);
    repl.push_back(lines_[store_hi_pos]);

    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                 lines_.begin() + static_cast<std::ptrdiff_t>(store_hi_pos + 1));
    lines_.insert(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                  repl.begin(), repl.end());
    return true;
}

bool z80_peep::rule_ix_scaled_offset_temp_elide(size_t i) {
    if (i + 9 >= lines_.size()) return false;

    for (size_t j = i + 1; j <= i + 9; ++j) {
        if (!lines_[j].label.empty())
            return false;
    }

    std::string dst, src;
    if (lines_[i].mnemonic != "ld" ||
        !split_ld(lines_[i].operands, dst, src) ||
        trim(dst) != "l") {
        return false;
    }
    int index_off = 0;
    if (!parse_ix_ref(trim(src), index_off))
        return false;

    if (lines_[i + 1].mnemonic != "ld" ||
        !split_ld(lines_[i + 1].operands, dst, src) ||
        trim(dst) != "h" || !immediate_is(trim(src), 0)) {
        return false;
    }

    const std::string scale_add_ops = trim(lines_[i + 2].operands);
    if (lines_[i + 2].mnemonic != "add" ||
        (scale_add_ops != "hl, hl" && scale_add_ops != "hl,hl")) {
        return false;
    }

    std::string store_lo_dst, store_lo_src;
    std::string store_hi_dst, store_hi_src;
    if (lines_[i + 3].mnemonic != "ld" ||
        !split_ld(lines_[i + 3].operands, store_lo_dst, store_lo_src) ||
        trim(store_lo_src) != "l") {
        return false;
    }
    if (lines_[i + 4].mnemonic != "ld" ||
        !split_ld(lines_[i + 4].operands, store_hi_dst, store_hi_src) ||
        trim(store_hi_src) != "h") {
        return false;
    }
    int temp_lo_off = 0;
    int temp_hi_off = 0;
    if (!parse_ix_ref(trim(store_lo_dst), temp_lo_off) ||
        !parse_ix_ref(trim(store_hi_dst), temp_hi_off)) {
        return false;
    }

    std::string base_lo_dst, base_lo_src;
    std::string base_hi_dst, base_hi_src;
    if (lines_[i + 5].mnemonic != "ld" ||
        !split_ld(lines_[i + 5].operands, base_lo_dst, base_lo_src) ||
        trim(base_lo_dst) != "l") {
        return false;
    }
    if (lines_[i + 6].mnemonic != "ld" ||
        !split_ld(lines_[i + 6].operands, base_hi_dst, base_hi_src) ||
        trim(base_hi_dst) != "h") {
        return false;
    }
    int base_lo_off = 0;
    int base_hi_off = 0;
    if (!parse_ix_ref(trim(base_lo_src), base_lo_off) ||
        !parse_ix_ref(trim(base_hi_src), base_hi_off)) {
        return false;
    }
    (void)index_off;
    (void)base_lo_off;
    (void)base_hi_off;

    std::string reload_lo_dst, reload_lo_src;
    std::string reload_hi_dst, reload_hi_src;
    if (lines_[i + 7].mnemonic != "ld" ||
        !split_ld(lines_[i + 7].operands, reload_lo_dst, reload_lo_src) ||
        trim(reload_lo_dst) != "e") {
        return false;
    }
    if (lines_[i + 8].mnemonic != "ld" ||
        !split_ld(lines_[i + 8].operands, reload_hi_dst, reload_hi_src) ||
        trim(reload_hi_dst) != "d") {
        return false;
    }
    int reload_lo_off = 0;
    int reload_hi_off = 0;
    if (!parse_ix_ref(trim(reload_lo_src), reload_lo_off) ||
        !parse_ix_ref(trim(reload_hi_src), reload_hi_off) ||
        reload_lo_off != temp_lo_off ||
        reload_hi_off != temp_hi_off) {
        return false;
    }

    const std::string final_add_ops = trim(lines_[i + 9].operands);
    if (lines_[i + 9].mnemonic != "add" ||
        (final_add_ops != "hl, de" && final_add_ops != "hl,de")) {
        return false;
    }

    const std::string temp_lo_ref = std::to_string(temp_lo_off) + "(ix)";
    const std::string temp_hi_ref = std::to_string(temp_hi_off) + "(ix)";
    for (size_t j = i + 10; j < lines_.size(); ++j) {
        if (lines_[j].operands.find(temp_lo_ref) != std::string::npos ||
            lines_[j].operands.find(temp_hi_ref) != std::string::npos) {
            return false;
        }
    }

    const asm_line base_lo = lines_[i + 5];
    const asm_line base_hi = lines_[i + 6];
    const asm_line final_add = lines_[i + 9];
    lines_[i + 3] = asm_line::parse("\tex\tde, hl");
    lines_[i + 4] = base_lo;
    lines_[i + 5] = base_hi;
    lines_[i + 6] = final_add;
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 7),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 10));
    return true;
}

bool z80_peep::rule_ix_scaled_offset_immediate_base_elide(size_t i) {
    if (i + 9 >= lines_.size())
        return false;

    for (size_t j = i; j <= i + 9; ++j) {
        if (!lines_[j].label.empty())
            return false;
    }

    std::string dst, src;
    if (lines_[i].mnemonic != "ld" ||
        !split_ld(lines_[i].operands, dst, src) ||
        trim(dst) != "l") {
        return false;
    }
    int index_lo_off = 0;
    if (!parse_ix_ref(trim(src), index_lo_off))
        return false;

    if (lines_[i + 1].mnemonic != "ld" ||
        !split_ld(lines_[i + 1].operands, dst, src) ||
        trim(dst) != "h") {
        return false;
    }
    int index_hi_off = 0;
    if (!parse_ix_ref(trim(src), index_hi_off))
        return false;
    (void)index_lo_off;
    (void)index_hi_off;

    size_t scale_pos = i + 2;
    if (lines_[scale_pos].mnemonic == "inc" || lines_[scale_pos].mnemonic == "dec") {
        if (trim(lines_[scale_pos].operands) != "hl")
            return false;
        ++scale_pos;
        if (i + 10 >= lines_.size())
            return false;
        if (!lines_[scale_pos].label.empty())
            return false;
    }

    const std::string scale_add_ops = trim(lines_[scale_pos].operands);
    if (lines_[scale_pos].mnemonic != "add" ||
        (scale_add_ops != "hl, hl" && scale_add_ops != "hl,hl")) {
        return false;
    }

    const size_t store_lo_pos = scale_pos + 1;
    const size_t store_hi_pos = scale_pos + 2;
    const size_t base_pos = scale_pos + 3;
    const size_t reload_lo_pos = scale_pos + 4;
    const size_t reload_hi_pos = scale_pos + 5;
    const size_t final_add_pos = scale_pos + 6;
    if (final_add_pos >= lines_.size())
        return false;
    for (size_t j = store_lo_pos; j <= final_add_pos; ++j) {
        if (!lines_[j].label.empty())
            return false;
    }

    std::string store_lo_dst, store_lo_src;
    std::string store_hi_dst, store_hi_src;
    if (lines_[store_lo_pos].mnemonic != "ld" ||
        !split_ld(lines_[store_lo_pos].operands, store_lo_dst, store_lo_src) ||
        trim(store_lo_src) != "l") {
        return false;
    }
    if (lines_[store_hi_pos].mnemonic != "ld" ||
        !split_ld(lines_[store_hi_pos].operands, store_hi_dst, store_hi_src) ||
        trim(store_hi_src) != "h") {
        return false;
    }
    int temp_lo_off = 0;
    int temp_hi_off = 0;
    if (!parse_ix_ref(trim(store_lo_dst), temp_lo_off) ||
        !parse_ix_ref(trim(store_hi_dst), temp_hi_off)) {
        return false;
    }

    int locals = 0;
    int temp_frame = 0;
    size_t prologue_index = 0;
    if (!current_function_frame(lines_, i, locals, temp_frame, prologue_index))
        return false;
    if (!ix_offset_in_temp_frame(temp_lo_off, locals, temp_frame) ||
        !ix_offset_in_temp_frame(temp_hi_off, locals, temp_frame)) {
        return false;
    }

    std::string base_dst, base_src;
    if (lines_[base_pos].mnemonic != "ld" ||
        !split_ld(lines_[base_pos].operands, base_dst, base_src) ||
        trim(base_dst) != "hl") {
        return false;
    }
    base_src = trim(base_src);
    if (!is_immediate_operand(base_src) && !is_numeric_literal(base_src))
        return false;

    std::string reload_lo_dst, reload_lo_src;
    std::string reload_hi_dst, reload_hi_src;
    if (lines_[reload_lo_pos].mnemonic != "ld" ||
        !split_ld(lines_[reload_lo_pos].operands, reload_lo_dst, reload_lo_src) ||
        trim(reload_lo_dst) != "e") {
        return false;
    }
    if (lines_[reload_hi_pos].mnemonic != "ld" ||
        !split_ld(lines_[reload_hi_pos].operands, reload_hi_dst, reload_hi_src) ||
        trim(reload_hi_dst) != "d") {
        return false;
    }
    int reload_lo_off = 0;
    int reload_hi_off = 0;
    if (!parse_ix_ref(trim(reload_lo_src), reload_lo_off) ||
        !parse_ix_ref(trim(reload_hi_src), reload_hi_off) ||
        reload_lo_off != temp_lo_off ||
        reload_hi_off != temp_hi_off) {
        return false;
    }

    const std::string final_add_ops = trim(lines_[final_add_pos].operands);
    if (lines_[final_add_pos].mnemonic != "add" ||
        (final_add_ops != "hl, de" && final_add_ops != "hl,de")) {
        return false;
    }

    const std::string temp_lo_ref = std::to_string(temp_lo_off) + "(ix)";
    const std::string temp_hi_ref = std::to_string(temp_hi_off) + "(ix)";
    for (size_t j = final_add_pos + 1; j < lines_.size(); ++j) {
        if (lines_[j].operands.find(temp_lo_ref) != std::string::npos ||
            lines_[j].operands.find(temp_hi_ref) != std::string::npos) {
            return false;
        }
    }

    lines_[store_lo_pos] = asm_line::parse("\tld\tde, " + base_src);
    lines_[store_hi_pos] = asm_line::parse("\tadd\thl, de");
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(base_pos),
                 lines_.begin() + static_cast<std::ptrdiff_t>(final_add_pos + 1));
    return true;
}

bool z80_peep::rule_de_scaled_offset_immediate_base_elide(size_t i) {
    if (i + 10 >= lines_.size())
        return false;

    for (size_t j = i; j <= i + 10; ++j) {
        if (!lines_[j].label.empty())
            return false;
    }

    auto match_ld = [&](size_t idx, const char *want_dst, const char *want_src) {
        if (idx >= lines_.size() || lines_[idx].mnemonic != "ld")
            return false;
        std::string dst;
        std::string src;
        if (!split_ld(lines_[idx].operands, dst, src))
            return false;
        return trim(dst) == want_dst && trim(src) == want_src;
    };

    if (!match_ld(i, "b", "d") ||
        !match_ld(i + 1, "c", "e") ||
        !match_ld(i + 2, "h", "b") ||
        !match_ld(i + 3, "l", "c")) {
        return false;
    }

    const std::string scale_add_ops = trim(lines_[i + 4].operands);
    if (lines_[i + 4].mnemonic != "add" ||
        (scale_add_ops != "hl, hl" && scale_add_ops != "hl,hl")) {
        return false;
    }

    std::string dst;
    std::string src;
    int temp_lo_off = 0;
    int temp_hi_off = 0;
    if (lines_[i + 5].mnemonic != "ld" ||
        !split_ld(lines_[i + 5].operands, dst, src) ||
        trim(src) != "l" ||
        !parse_ix_ref(trim(dst), temp_lo_off)) {
        return false;
    }
    if (lines_[i + 6].mnemonic != "ld" ||
        !split_ld(lines_[i + 6].operands, dst, src) ||
        trim(src) != "h" ||
        !parse_ix_ref(trim(dst), temp_hi_off)) {
        return false;
    }

    int locals = 0;
    int temp_frame = 0;
    size_t prologue_index = 0;
    if (!current_function_frame(lines_, i, locals, temp_frame, prologue_index))
        return false;
    if (!ix_offset_in_temp_frame(temp_lo_off, locals, temp_frame) ||
        !ix_offset_in_temp_frame(temp_hi_off, locals, temp_frame)) {
        return false;
    }

    std::string base_dst;
    std::string base_src;
    if (lines_[i + 7].mnemonic != "ld" ||
        !split_ld(lines_[i + 7].operands, base_dst, base_src) ||
        trim(base_dst) != "hl") {
        return false;
    }
    base_src = trim(base_src);
    if (!is_immediate_operand(base_src) && !is_numeric_literal(base_src))
        return false;

    int reload_lo_off = 0;
    int reload_hi_off = 0;
    if (lines_[i + 8].mnemonic != "ld" ||
        !split_ld(lines_[i + 8].operands, dst, src) ||
        trim(dst) != "e" ||
        !parse_ix_ref(trim(src), reload_lo_off) ||
        reload_lo_off != temp_lo_off) {
        return false;
    }
    if (lines_[i + 9].mnemonic != "ld" ||
        !split_ld(lines_[i + 9].operands, dst, src) ||
        trim(dst) != "d" ||
        !parse_ix_ref(trim(src), reload_hi_off) ||
        reload_hi_off != temp_hi_off) {
        return false;
    }

    const std::string final_add_ops = trim(lines_[i + 10].operands);
    if (lines_[i + 10].mnemonic != "add" ||
        (final_add_ops != "hl, de" && final_add_ops != "hl,de")) {
        return false;
    }

    const size_t fn_end = function_end_after_prologue(lines_, prologue_index);
    if (!ix_slot_not_read_before_rewrite(lines_, i + 11, fn_end, temp_lo_off) ||
        !ix_slot_not_read_before_rewrite(lines_, i + 11, fn_end, temp_hi_off)) {
        return false;
    }
    auto de_dead_before_read = [&](size_t start) {
        if (pair_value_dead_or_call_or_modern_return_before_read(
                lines_, start, "de", 'e', 'd')) {
            return true;
        }

        bool e_overwritten = false;
        bool d_overwritten = false;
        const size_t end = std::min(lines_.size(), start + 12);
        for (size_t k = start; k < end; ++k) {
            const asm_line &line = lines_[k];
            if (line.mnemonic.empty())
                continue;
            if (!line.label.empty())
                return false;

            if (line.mnemonic == "ld") {
                std::string ld_dst;
                std::string ld_src;
                if (split_ld(line.operands, ld_dst, ld_src)) {
                    ld_dst = trim(ld_dst);
                    ld_src = trim(ld_src);
                    if ((ld_dst == "e" || ld_dst == "d") &&
                        !operand_mentions_pair_or_bytes(ld_src, "de", 'e', 'd')) {
                        if (ld_dst == "e")
                            e_overwritten = true;
                        else
                            d_overwritten = true;
                        if (e_overwritten && d_overwritten)
                            return true;
                        continue;
                    }
                }
            }

            if (operand_mentions_pair_or_bytes(line.operands, "de", 'e', 'd'))
                return false;
            if (!line_preserves_pair_value(line, "de", 'e', 'd'))
                return false;
        }
        return false;
    };

    if (!path_overwrites_bc_before_read_or_call(lines_, i + 11))
        return false;
    if (!de_dead_before_read(i + 11)) {
        return false;
    }

    lines_[i] = asm_line::parse("\tld\th, d");
    lines_[i + 1] = asm_line::parse("\tld\tl, e");
    lines_[i + 2] = lines_[i + 4];
    lines_[i + 3] = asm_line::parse("\tld\tde, " + base_src);
    lines_[i + 4] = lines_[i + 10];
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 5),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 11));
    return true;
}

bool z80_peep::rule_ix_pointer_scan_temp_to_hl_loop(size_t i) {
    if (i + 12 >= lines_.size())
        return false;
    for (size_t j = i; j <= i + 3; ++j) {
        if (!lines_[j].label.empty() || lines_[j].mnemonic != "ld")
            return false;
    }

    std::string dst, src;
    int src_lo = 0, src_hi = 0, tmp_lo = 0, tmp_hi = 0;
    if (!split_ld(lines_[i].operands, dst, src) ||
        trim(dst) != "l" ||
        !parse_ix_ref(trim(src), src_lo)) {
        return false;
    }
    if (!split_ld(lines_[i + 1].operands, dst, src) ||
        trim(dst) != "h" ||
        !parse_ix_ref(trim(src), src_hi) ||
        src_hi != src_lo + 1) {
        return false;
    }
    if (!split_ld(lines_[i + 2].operands, dst, src) ||
        !parse_ix_ref(trim(dst), tmp_lo) ||
        trim(src) != "l") {
        return false;
    }
    if (!split_ld(lines_[i + 3].operands, dst, src) ||
        !parse_ix_ref(trim(dst), tmp_hi) ||
        tmp_hi != tmp_lo + 1 ||
        trim(src) != "h") {
        return false;
    }

    const asm_line &loop_label = lines_[i + 4];
    if (loop_label.label.empty() || !loop_label.mnemonic.empty())
        return false;
    const std::string loop = loop_label.label;

    if (!lines_[i + 5].label.empty() || lines_[i + 5].mnemonic != "ld" ||
        !split_ld(lines_[i + 5].operands, dst, src) ||
        trim(dst) != "l" ||
        !operand_is_ix_offset(trim(src), tmp_lo)) {
        return false;
    }
    if (!lines_[i + 6].label.empty() || lines_[i + 6].mnemonic != "ld" ||
        !split_ld(lines_[i + 6].operands, dst, src) ||
        trim(dst) != "h" ||
        !operand_is_ix_offset(trim(src), tmp_hi)) {
        return false;
    }
    if (!lines_[i + 7].label.empty() ||
        lines_[i + 7].mnemonic != "ld" ||
        trim(lines_[i + 7].operands) != "a, (hl)") {
        return false;
    }
    if (!lines_[i + 8].label.empty() ||
        lines_[i + 8].mnemonic != "or" ||
        trim(lines_[i + 8].operands) != "a, a") {
        return false;
    }

    std::string cc, exit_label;
    if (!split_conditional_branch_target(lines_[i + 9], cc, exit_label) ||
        cc != "z") {
        return false;
    }

    size_t inc_idx = lines_.size();
    const size_t scan_end = std::min(lines_.size(), i + 64);
    for (size_t k = i + 10; k + 4 < scan_end; ++k) {
        if (lines_[k].mnemonic == "inc" &&
            operand_is_ix_offset(lines_[k].operands, tmp_lo) &&
            lines_[k + 1].label.empty() &&
            lines_[k + 1].mnemonic == "jr") {
            std::string inc_cc, carry_label;
            if (!split_conditional_branch_target(lines_[k + 1],
                                                 inc_cc, carry_label) ||
                inc_cc != "nz") {
                continue;
            }
            if (!lines_[k + 2].label.empty() ||
                lines_[k + 2].mnemonic != "inc" ||
                !operand_is_ix_offset(lines_[k + 2].operands, tmp_hi)) {
                continue;
            }
            if (lines_[k + 3].label != carry_label ||
                !lines_[k + 3].mnemonic.empty()) {
                continue;
            }
            if (label_has_other_control_references(lines_, carry_label, k + 1))
                continue;
            std::string back_target;
            if (!parse_unconditional_jump(lines_[k + 4], back_target) ||
                back_target != loop) {
                continue;
            }
            inc_idx = k;
            break;
        }
    }
    if (inc_idx == lines_.size())
        return false;
    if (label_has_other_control_references(lines_, loop, inc_idx + 4))
        return false;

    const size_t exit_idx = find_label_index(lines_, exit_label);
    if (exit_idx != inc_idx + 5)
        return false;

    for (size_t k = i + 10; k < inc_idx; ++k) {
        const asm_line &line = lines_[k];
        if (!line.label.empty() && line.mnemonic.empty())
            continue;
        if (!line_preserves_pair_value(line, "hl", 'l', 'h'))
            return false;
        if (line_reads_ix_offset(line, tmp_lo) ||
            line_reads_ix_offset(line, tmp_hi) ||
            line_writes_ix_offset(line, tmp_lo) ||
            line_writes_ix_offset(line, tmp_hi)) {
            return false;
        }
    }

    int locals = 0, temp_frame = 0;
    size_t prologue_index = 0;
    if (!current_function_frame(lines_, i, locals, temp_frame, prologue_index))
        return false;
    const size_t fn_end = function_end_after_prologue(lines_, prologue_index);
    if (!ix_slot_not_read_before_rewrite(lines_, exit_idx, fn_end, tmp_lo) ||
        !ix_slot_not_read_before_rewrite(lines_, exit_idx, fn_end, tmp_hi)) {
        return false;
    }

    lines_[inc_idx] = asm_line::parse("\tinc\thl");
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(inc_idx + 1),
                 lines_.begin() + static_cast<std::ptrdiff_t>(inc_idx + 4));
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 5),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 7));
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 2),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 4));
    return true;
}

bool z80_peep::rule_ix_index14_scaled_base_temp_elide(size_t i) {
    if (i + 35 >= lines_.size())
        return false;
    for (size_t j = i; j <= i + 35; ++j) {
        if (!lines_[j].label.empty())
            return false;
    }

    auto match_ld = [&](size_t idx, const std::string &want_dst,
                        const std::string &want_src) {
        if (lines_[idx].mnemonic != "ld")
            return false;
        std::string dst;
        std::string src;
        return split_ld(lines_[idx].operands, dst, src) &&
               trim(dst) == want_dst && trim(src) == want_src;
    };
    auto match_ix_ld = [&](size_t idx, const std::string &want_dst,
                           int &off_out) {
        if (lines_[idx].mnemonic != "ld")
            return false;
        std::string dst;
        std::string src;
        if (!split_ld(lines_[idx].operands, dst, src) ||
            trim(dst) != want_dst ||
            !parse_ix_ref(trim(src), off_out)) {
            return false;
        }
        return true;
    };
    auto match_ix_store = [&](size_t idx, int want_off,
                              const std::string &want_src) {
        if (lines_[idx].mnemonic != "ld")
            return false;
        std::string dst;
        std::string src;
        int off = 0;
        return split_ld(lines_[idx].operands, dst, src) &&
               parse_ix_ref(trim(dst), off) &&
               off == want_off && trim(src) == want_src;
    };
    auto match_add = [&](size_t idx, const std::string &rhs) {
        if (lines_[idx].mnemonic != "add")
            return false;
        std::string dst;
        std::string src;
        return split_ld(lines_[idx].operands, dst, src) &&
               trim(dst) == "hl" && trim(src) == rhs;
    };

    int index_off = 0;
    if (!match_ix_ld(i, "l", index_off))
        return false;
    if (!match_ld(i + 1, "h", "#0") && !match_ld(i + 1, "h", "0"))
        return false;

    std::string dst;
    std::string src;
    int t0 = 0;
    if (lines_[i + 2].mnemonic != "ld" ||
        !split_ld(lines_[i + 2].operands, dst, src) ||
        !parse_ix_ref(trim(dst), t0) || trim(src) != "l" ||
        !match_ix_store(i + 3, t0 + 1, "h") ||
        !match_add(i + 4, "hl")) {
        return false;
    }

    int t2 = 0;
    if (lines_[i + 5].mnemonic != "ld" ||
        !split_ld(lines_[i + 5].operands, dst, src) ||
        !parse_ix_ref(trim(dst), t2) || trim(src) != "l" ||
        !match_ix_store(i + 6, t2 + 1, "h")) {
        return false;
    }
    int reload = 0;
    if (!match_ix_ld(i + 7, "l", reload) || reload != t0 ||
        !match_ix_ld(i + 8, "h", reload) || reload != t0 + 1 ||
        !match_add(i + 9, "hl") ||
        !match_add(i + 10, "hl")) {
        return false;
    }

    int t4 = 0;
    if (lines_[i + 11].mnemonic != "ld" ||
        !split_ld(lines_[i + 11].operands, dst, src) ||
        !parse_ix_ref(trim(dst), t4) || trim(src) != "l" ||
        !match_ix_store(i + 12, t4 + 1, "h")) {
        return false;
    }
    if (!match_ix_ld(i + 13, "l", reload) || reload != t0 ||
        !match_ix_ld(i + 14, "h", reload) || reload != t0 + 1 ||
        !match_add(i + 15, "hl") ||
        !match_add(i + 16, "hl") ||
        !match_add(i + 17, "hl")) {
        return false;
    }

    int t8 = 0;
    if (lines_[i + 18].mnemonic != "ld" ||
        !split_ld(lines_[i + 18].operands, dst, src) ||
        !parse_ix_ref(trim(dst), t8) || trim(src) != "l" ||
        !match_ix_store(i + 19, t8 + 1, "h")) {
        return false;
    }
    if (!match_ix_ld(i + 20, "l", reload) || reload != t2 ||
        !match_ix_ld(i + 21, "h", reload) || reload != t2 + 1 ||
        !match_ix_ld(i + 22, "e", reload) || reload != t4 ||
        !match_ix_ld(i + 23, "d", reload) || reload != t4 + 1 ||
        !match_add(i + 24, "de") ||
        !match_ix_ld(i + 25, "e", reload) || reload != t8 ||
        !match_ix_ld(i + 26, "d", reload) || reload != t8 + 1 ||
        !match_add(i + 27, "de")) {
        return false;
    }

    int tsum = 0;
    if (lines_[i + 28].mnemonic != "ld" ||
        !split_ld(lines_[i + 28].operands, dst, src) ||
        !parse_ix_ref(trim(dst), tsum) || trim(src) != "l" ||
        !match_ix_store(i + 29, tsum + 1, "h")) {
        return false;
    }

    std::string base_imm;
    if (lines_[i + 30].mnemonic != "ld" ||
        !split_ld(lines_[i + 30].operands, dst, base_imm) ||
        trim(dst) != "hl" || !is_immediate_operand(trim(base_imm))) {
        return false;
    }
    base_imm = trim(base_imm);
    if (!match_ix_ld(i + 31, "e", reload) || reload != tsum ||
        !match_ix_ld(i + 32, "d", reload) || reload != tsum + 1 ||
        !match_add(i + 33, "de")) {
        return false;
    }

    int out = 0;
    if (lines_[i + 34].mnemonic != "ld" ||
        !split_ld(lines_[i + 34].operands, dst, src) ||
        !parse_ix_ref(trim(dst), out) || trim(src) != "l" ||
        !match_ix_store(i + 35, out + 1, "h")) {
        return false;
    }

    if (!bc_overwritten_or_call_before_read_allowing_unrelated(lines_, i + 36, 32))
        return false;

    std::vector<asm_line> repl;
    repl.push_back(asm_line::parse("\tld\tl, " + std::to_string(index_off) + "(ix)"));
    repl.push_back(asm_line::parse("\tld\th, #0"));
    repl.push_back(asm_line::parse("\tadd\thl, hl"));
    repl.push_back(asm_line::parse("\tld\tb, h"));
    repl.push_back(asm_line::parse("\tld\tc, l"));
    repl.push_back(asm_line::parse("\tadd\thl, hl"));
    repl.push_back(asm_line::parse("\tld\td, h"));
    repl.push_back(asm_line::parse("\tld\te, l"));
    repl.push_back(asm_line::parse("\tadd\thl, hl"));
    repl.push_back(asm_line::parse("\tadd\thl, de"));
    repl.push_back(asm_line::parse("\tadd\thl, bc"));
    repl.push_back(asm_line::parse("\tld\tde, " + base_imm));
    repl.push_back(asm_line::parse("\tadd\thl, de"));
    repl.push_back(asm_line::parse("\tld\t" + std::to_string(out) + "(ix), l"));
    repl.push_back(asm_line::parse("\tld\t" + std::to_string(out + 1) + "(ix), h"));

    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 36));
    lines_.insert(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                  repl.begin(), repl.end());
    return true;
}

// push hl; ld hl,#0; pop de; or a,a; sbc hl,de; jp z/nz,L
// →  ld a,h; or a,l; jp z/nz,L
// Zero-testing HL via the standard compare-with-zero sequence is a 5-instruction
// overhead.  OR-ing H and L into A directly tests for zero in 2 instructions.
bool z80_peep::rule_zero_cmp_optimize(size_t i) {
    if (i + 5 >= lines_.size()) return false;

    for (int k = 1; k <= 5; ++k)
        if (!lines_[i + k].label.empty()) return false;

    auto &l0 = lines_[i];
    auto &l1 = lines_[i + 1];
    auto &l2 = lines_[i + 2];
    auto &l3 = lines_[i + 3];
    auto &l4 = lines_[i + 4];
    auto &l5 = lines_[i + 5];

    if (l0.mnemonic != "push" || trim(l0.operands) != "hl") return false;

    if (l1.mnemonic != "ld") return false;
    { std::string d, s; if (!split_ld(l1.operands, d, s)) return false;
      if (d != "hl" || (s != "#0" && s != "0")) return false; }

    if (l2.mnemonic != "pop" || trim(l2.operands) != "de") return false;

    if (l3.mnemonic != "or") return false;
    { std::string ops = trim(l3.operands);
      if (ops != "a, a" && ops != "a,a" && ops != "a") return false; }

    if (l4.mnemonic != "sbc") return false;
    { std::string d, s; if (!split_ld(l4.operands, d, s)) return false;
      if (d != "hl" || s != "de") return false; }

    if (l5.mnemonic != "jp" && l5.mnemonic != "jr") return false;
    { std::string ops = trim(l5.operands);
      size_t c = ops.find(',');
      if (c == std::string::npos) return false;
      std::string cc = trim(ops.substr(0, c));
      if (cc != "z" && cc != "nz") return false; }

    l0.mnemonic = "ld";  l0.operands = "a, h";
    l1.mnemonic = "or";  l1.operands = "a, l";
    // Keep l5 (the jp z/nz) at new position i+2 after removing l2..l4.
    lines_.erase(lines_.begin() + i + 2, lines_.begin() + i + 5);
    return true;
}

bool z80_peep::rule_hl_nonzero_materialize(size_t i) {
    auto is_end_label = [&](size_t idx, const std::string &expected) {
        return idx < lines_.size() &&
               lines_[idx].label == expected &&
               lines_[idx].mnemonic.empty();
    };

    auto rewrite_common_tail = [&](size_t start,
                                   size_t branch_idx,
                                   const std::string &end_lbl) {
        if (branch_idx + 2 >= lines_.size())
            return false;
        const asm_line &branch = lines_[branch_idx];
        if ((branch.mnemonic != "jr" && branch.mnemonic != "jp") ||
            lines_[branch_idx + 1].mnemonic != "dec" ||
            trim(lines_[branch_idx + 1].operands) != "hl" ||
            !is_end_label(branch_idx + 2, end_lbl))
            return false;

        std::string ops = trim(branch.operands);
        size_t comma = ops.find(',');
        if (comma == std::string::npos)
            return false;
        if (trim(ops.substr(0, comma)) != "nz" ||
            trim(ops.substr(comma + 1)) != end_lbl)
            return false;

        asm_line l0; l0.mnemonic = "ld"; l0.operands = "a, h";
        asm_line l1; l1.mnemonic = "or"; l1.operands = "a, l";
        asm_line l2; l2.mnemonic = "ld"; l2.operands = "hl, #1";
        asm_line l3 = branch;
        asm_line l4; l4.mnemonic = "dec"; l4.operands = "hl";
        asm_line l5; l5.label = end_lbl; l5.is_label = true;

        lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(start),
                     lines_.begin() + static_cast<std::ptrdiff_t>(branch_idx + 3));
        lines_.insert(lines_.begin() + static_cast<std::ptrdiff_t>(start),
                      {l0, l1, l2, l3, l4, l5});
        return true;
    };

    // push hl; ld hl,#0; pop de; or a,a; sbc hl,de; ld hl,#1; jr/jp nz,L; dec hl; L:
    if (i + 8 < lines_.size() &&
        lines_[i].mnemonic == "push" && trim(lines_[i].operands) == "hl" &&
        lines_[i + 1].mnemonic == "ld" && trim(lines_[i + 1].operands) == "hl, #0" &&
        lines_[i + 2].mnemonic == "pop" && trim(lines_[i + 2].operands) == "de" &&
        lines_[i + 3].mnemonic == "or" && trim(lines_[i + 3].operands) == "a, a" &&
        lines_[i + 4].mnemonic == "sbc" && trim(lines_[i + 4].operands) == "hl, de" &&
        lines_[i + 5].mnemonic == "ld" && trim(lines_[i + 5].operands) == "hl, #1") {
        std::string ops = trim(lines_[i + 6].operands);
        size_t comma = ops.find(',');
        if (comma != std::string::npos) {
            std::string end_lbl = trim(ops.substr(comma + 1));
            if (rewrite_common_tail(i, i + 6, end_lbl))
                return true;
        }
    }

    // ld b,h; ld c,l; ex de,hl; ld hl,#0; or a,a; sbc hl,de; ld hl,#1; jr/jp nz,L; dec hl; L:
    if (i + 9 < lines_.size() &&
        lines_[i].mnemonic == "ld" && trim(lines_[i].operands) == "b, h" &&
        lines_[i + 1].mnemonic == "ld" && trim(lines_[i + 1].operands) == "c, l" &&
        lines_[i + 2].mnemonic == "ex" && trim(lines_[i + 2].operands) == "de, hl" &&
        lines_[i + 3].mnemonic == "ld" && trim(lines_[i + 3].operands) == "hl, #0" &&
        lines_[i + 4].mnemonic == "or" && trim(lines_[i + 4].operands) == "a, a" &&
        lines_[i + 5].mnemonic == "sbc" && trim(lines_[i + 5].operands) == "hl, de" &&
        lines_[i + 6].mnemonic == "ld" && trim(lines_[i + 6].operands) == "hl, #1") {
        std::string ops = trim(lines_[i + 7].operands);
        size_t comma = ops.find(',');
        if (comma != std::string::npos) {
            std::string end_lbl = trim(ops.substr(comma + 1));
            if (rewrite_common_tail(i, i + 7, end_lbl))
                return true;
        }
    }

    return false;
}

static bool hl_reloaded_from_ix_pair_before_read(
        const std::vector<asm_line> &lines,
        size_t start,
        int lo_off,
        int hi_off,
        size_t budget = 16) {
    for (size_t k = start; k < lines.size() && budget > 0; ++k, --budget) {
        const auto &line = lines[k];
        if (line.mnemonic.empty())
            continue;
        if (is_section_directive(line))
            return false;

        if (line.mnemonic == "ld") {
            std::string dst;
            std::string src;
            int off = 0;
            if (split_ld(line.operands, dst, src) &&
                trim(dst) == "l" &&
                parse_ix_ref(trim(src), off) &&
                off == lo_off) {
                for (size_t h = k + 1; h < lines.size() && budget > 0; ++h, --budget) {
                    const auto &hi_line = lines[h];
                    if (hi_line.mnemonic.empty())
                        continue;
                    if (hi_line.mnemonic != "ld" ||
                        !split_ld(hi_line.operands, dst, src) ||
                        trim(dst) != "h" ||
                        !parse_ix_ref(trim(src), off) ||
                        off != hi_off) {
                        return false;
                    }
                    return true;
                }
                return false;
            }
        }

        if (!line_preserves_pair_and_sp(line, "hl", 'l', 'h'))
            return false;
    }

    return false;
}

bool z80_peep::rule_ix_word_zero_test_direct(size_t i) {
    if (i + 4 >= lines_.size())
        return false;

    for (size_t j = i; j <= i + 4; ++j) {
        if (!lines_[j].label.empty())
            return false;
    }

    std::string dst;
    std::string src;
    int lo_off = 0;
    int hi_off = 0;
    if (lines_[i].mnemonic != "ld" ||
        !split_ld(lines_[i].operands, dst, src) ||
        trim(dst) != "l" ||
        !parse_ix_ref(trim(src), lo_off)) {
        return false;
    }
    if (lines_[i + 1].mnemonic != "ld" ||
        !split_ld(lines_[i + 1].operands, dst, src) ||
        trim(dst) != "h" ||
        !parse_ix_ref(trim(src), hi_off) ||
        hi_off != lo_off + 1) {
        return false;
    }
    if (lines_[i + 2].mnemonic != "ld" ||
        !split_ld(lines_[i + 2].operands, dst, src) ||
        trim(dst) != "a" || trim(src) != "h") {
        return false;
    }

    bool or_l = false;
    if (lines_[i + 3].mnemonic == "or") {
        const std::string ops = trim(lines_[i + 3].operands);
        if (ops == "l") {
            or_l = true;
        } else if (split_ld(ops, dst, src) &&
                   trim(dst) == "a" && trim(src) == "l") {
            or_l = true;
        }
    }
    if (!or_l)
        return false;

    std::string cc;
    std::string target;
    if (!split_conditional_branch_target(lines_[i + 4], cc, target) ||
        (cc != "z" && cc != "nz")) {
        return false;
    }

    const size_t target_idx = find_label_index(lines_, target);
    if (target_idx == lines_.size())
        return false;

    const bool taken_hl_dead =
        hl_dead_before_read_or_modern_return(lines_, target_idx) ||
        hl_reloaded_from_ix_pair_before_read(lines_, target_idx, lo_off, hi_off);
    const bool fallthrough_hl_dead =
        hl_dead_before_read_or_modern_return(lines_, i + 5) ||
        hl_reloaded_from_ix_pair_before_read(lines_, i + 5, lo_off, hi_off);
    if (!taken_hl_dead || !fallthrough_hl_dead)
        return false;

    const std::string lo_ref = std::to_string(lo_off) + "(ix)";
    const std::string hi_ref = std::to_string(hi_off) + "(ix)";
    lines_[i].operands = "a, " + hi_ref;
    lines_[i + 1].mnemonic = "or";
    lines_[i + 1].operands = "a, " + lo_ref;
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 2),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 4));
    return true;
}

bool z80_peep::rule_ix_word_store_zero_test_from_pair(size_t i) {
    if (i + 4 >= lines_.size())
        return false;

    for (size_t j = i; j <= i + 4; ++j) {
        if (!lines_[j].label.empty())
            return false;
    }

    std::string dst;
    std::string src;
    int lo_off = 0;
    int hi_off = 0;
    std::string lo_reg;
    std::string hi_reg;
    if (lines_[i].mnemonic != "ld" ||
        !split_ld(lines_[i].operands, dst, src) ||
        !parse_ix_ref(trim(dst), lo_off)) {
        return false;
    }
    lo_reg = trim(src);
    if (lines_[i + 1].mnemonic != "ld" ||
        !split_ld(lines_[i + 1].operands, dst, src) ||
        !parse_ix_ref(trim(dst), hi_off) ||
        hi_off != lo_off + 1) {
        return false;
    }
    hi_reg = trim(src);

    const bool known_pair =
        (lo_reg == "c" && hi_reg == "b") ||
        (lo_reg == "e" && hi_reg == "d") ||
        (lo_reg == "l" && hi_reg == "h");
    if (!known_pair)
        return false;

    int test_hi = 0;
    if (lines_[i + 2].mnemonic != "ld" ||
        !split_ld(lines_[i + 2].operands, dst, src) ||
        trim(dst) != "a" ||
        !parse_ix_ref(trim(src), test_hi) ||
        test_hi != hi_off) {
        return false;
    }

    bool tests_lo = false;
    if (lines_[i + 3].mnemonic == "or") {
        const std::string ops = trim(lines_[i + 3].operands);
        int test_lo = 0;
        if (parse_ix_ref(ops, test_lo)) {
            tests_lo = test_lo == lo_off;
        } else if (split_ld(ops, dst, src) &&
                   trim(dst) == "a" &&
                   parse_ix_ref(trim(src), test_lo)) {
            tests_lo = test_lo == lo_off;
        }
    }
    if (!tests_lo)
        return false;

    std::string cc;
    std::string target;
    if (!split_conditional_branch_target(lines_[i + 4], cc, target) ||
        (cc != "z" && cc != "nz")) {
        return false;
    }

    lines_[i + 2].operands = "a, " + hi_reg;
    lines_[i + 3].operands = "a, " + lo_reg;
    return true;
}

bool z80_peep::rule_ix_hl_store_zero_test_reload_elide(size_t i) {
    if (i + 4 >= lines_.size())
        return false;

    for (size_t j = i; j <= i + 4; ++j) {
        if (!lines_[j].label.empty())
            return false;
    }

    std::string dst;
    std::string src;
    int lo_off = 0;
    int hi_off = 0;
    if (lines_[i].mnemonic != "ld" ||
        !split_ld(lines_[i].operands, dst, src) ||
        trim(dst) != "l" ||
        !parse_ix_ref(trim(src), lo_off)) {
        return false;
    }
    if (lines_[i + 1].mnemonic != "ld" ||
        !split_ld(lines_[i + 1].operands, dst, src) ||
        trim(dst) != "h" ||
        !parse_ix_ref(trim(src), hi_off) ||
        hi_off != lo_off + 1) {
        return false;
    }
    if (lines_[i + 2].mnemonic != "ld" ||
        !split_ld(lines_[i + 2].operands, dst, src) ||
        trim(dst) != "a" || trim(src) != "h") {
        return false;
    }

    bool or_l = false;
    if (lines_[i + 3].mnemonic == "or") {
        const std::string ops = trim(lines_[i + 3].operands);
        if (ops == "l") {
            or_l = true;
        } else if (split_ld(ops, dst, src) &&
                   trim(dst) == "a" && trim(src) == "l") {
            or_l = true;
        }
    }
    if (!or_l)
        return false;

    std::string cc;
    std::string target;
    if (!split_conditional_branch_target(lines_[i + 4], cc, target) ||
        (cc != "z" && cc != "nz")) {
        return false;
    }

    auto preserves_hl_value = [](const asm_line &line) {
        if (!line.label.empty() || is_section_directive(line))
            return false;
        if (line.mnemonic.empty())
            return true;

        const std::string &m = line.mnemonic;
        if (m == "push" || m == "pop" || m == "call" || m == "ret" ||
            m == "reti" || m == "retn" || m == "rst" || m == "jp" ||
            m == "jr" || m == "djnz" || m == "ex" || m == "exx" ||
            m == "ldi" || m == "ldir" || m == "ldd" || m == "lddr" ||
            m == "cpi" || m == "cpir" || m == "cpd" || m == "cpdr") {
            return false;
        }

        std::string d;
        std::string s;
        if (m == "ld") {
            if (!split_ld(line.operands, d, s))
                return false;
            d = trim(d);
            // A write through HL/BC/DE/absolute memory could alias the IX temp
            // whose reload we are trying to remove. IX/IY-local writes are
            // checked separately against the exact temp offsets.
            if (operand_uses_memory(d) && !uses_ixiy_disp(d))
                return false;
            return d != "h" && d != "l" && d != "hl";
        }
        if ((m == "inc" || m == "dec") &&
            operand_mentions_pair_or_bytes(line.operands, "hl", 'l', 'h')) {
            return false;
        }
        if ((m == "add" || m == "adc" || m == "sbc") &&
            split_ld(line.operands, d, s) &&
            trim(d) == "hl") {
            return false;
        }

        return !operand_mentions_pair_or_bytes(line.operands, "hl", 'l', 'h');
    };

    const std::string lo_ref = std::to_string(lo_off) + "(ix)";
    const std::string hi_ref = std::to_string(hi_off) + "(ix)";
    const size_t lower = i > 10 ? i - 10 : 0;
    for (size_t j = lower; j + 1 < i; ++j) {
        if (!lines_[j].label.empty() || !lines_[j + 1].label.empty())
            continue;
        int store_lo = 0;
        int store_hi = 0;
        if (lines_[j].mnemonic != "ld" ||
            !split_ld(lines_[j].operands, dst, src) ||
            !parse_ix_ref(trim(dst), store_lo) ||
            store_lo != lo_off ||
            trim(src) != "l") {
            continue;
        }
        if (lines_[j + 1].mnemonic != "ld" ||
            !split_ld(lines_[j + 1].operands, dst, src) ||
            !parse_ix_ref(trim(dst), store_hi) ||
            store_hi != hi_off ||
            trim(src) != "h") {
            continue;
        }

        bool safe_span = true;
        for (size_t k = j + 2; k < i; ++k) {
            if (!preserves_hl_value(lines_[k]) ||
                lines_[k].operands.find(lo_ref) != std::string::npos ||
                lines_[k].operands.find(hi_ref) != std::string::npos) {
                safe_span = false;
                break;
            }
        }
        if (!safe_span)
            continue;

        lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                     lines_.begin() + static_cast<std::ptrdiff_t>(i + 2));
        return true;
    }

    return false;
}

// jp [cc,] L  →  jr [cc,] L
//
// This pass computes a conservative section-local code layout from the final
// assembly text and only shortens branches whose estimated final JR
// displacement is in range. That makes it much closer to what SDCC does in
// practice than the old line-count heuristic.
bool z80_peep::rule_jp_to_jr(size_t i) {
    if (i >= lines_.size())
        return false;

    auto &line = lines_[i];
    if (line.mnemonic != "jp")
        return false;

    std::string cc;
    std::string target;
    bool conditional = parse_conditional_jump(line, cc, target);
    if (!conditional) {
        if (!parse_unconditional_jump(line, target))
            return false;
    } else if (cc != "z" && cc != "nz" && cc != "c" && cc != "nc") {
        return false;
    }

    const auto layout = compute_code_layout(lines_);
    auto target_it = layout.label_pos.find(target);
    if (target_it == layout.label_pos.end())
        return false;

    const auto &src = layout.line_pos[i];
    const auto &dst = target_it->second;
    if (src.section != dst.section)
        return false;

    // If the target is after the shortened branch, it moves back by one byte
    // when JP (3 bytes) becomes JR (2 bytes). Backward targets do not move.
    int disp = 0;
    if (dst.offset > src.offset)
        disp = dst.offset - (src.offset + 3);
    else
        disp = dst.offset - (src.offset + 2);

    // Keep generous headroom because this pass still relies on an internal
    // size estimator rather than the assembler's final encoded addresses.
    if (disp < -118 || disp > 118)
        return false;

    line.mnemonic = "jr";
    return true;
}

// push hl; pop bc  →  ld b,h; ld c,l
// Identical code size but avoids stack traffic (8 cycles vs 21 cycles).
bool z80_peep::rule_push_hl_pop_bc(size_t i) {
    if (i + 1 >= lines_.size()) return false;
    auto &a = lines_[i];
    auto &b = lines_[i + 1];
    if (a.mnemonic != "push" || trim(a.operands) != "hl") return false;
    if (b.mnemonic != "pop"  || trim(b.operands) != "bc") return false;
    if (!b.label.empty()) return false;
    a.mnemonic = "ld";  a.operands = "b, h";
    b.mnemonic = "ld";  b.operands = "c, l";
    return true;
}

bool z80_peep::rule_push_de_pop_hl_to_ex(size_t i) {
    if (i + 1 >= lines_.size())
        return false;
    if (!lines_[i].label.empty() || !lines_[i + 1].label.empty())
        return false;
    if (lines_[i].mnemonic != "push" || trim(lines_[i].operands) != "de")
        return false;
    if (lines_[i + 1].mnemonic != "pop" ||
        trim(lines_[i + 1].operands) != "hl") {
        return false;
    }
    if (!path_overwrites_pair_before_read(lines_, i + 2, "de", 'e', 'd'))
        return false;

    lines_[i] = asm_line::parse("\tex\tde, hl");
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 1));
    return true;
}

bool z80_peep::rule_pop_bc_run_sp_adjust(size_t i) {
    if (i >= lines_.size())
        return false;
    size_t n = 0;
    while (i + n < lines_.size()) {
        const asm_line &line = lines_[i + n];
        if (!line.label.empty() ||
            line.mnemonic != "pop" ||
            trim(line.operands) != "bc") {
            break;
        }
        ++n;
    }
    if (n < 6)
        return false;

    const size_t after = i + n;
    if (!overwrites_hl_without_reading_it(lines_, after))
        return false;
    if (!flags_overwritten_before_read_or_escape(lines_, after))
        return false;

    const int bytes = static_cast<int>(n * 2);
    lines_[i] = asm_line::parse("\tld\thl, #" + std::to_string(bytes));
    lines_[i + 1] = asm_line::parse("\tadd\thl, sp");
    lines_[i + 2] = asm_line::parse("\tld\tsp, hl");
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 3),
                 lines_.begin() + static_cast<std::ptrdiff_t>(after));
    return true;
}

bool z80_peep::rule_dead_hl_de_stack_copy_to_bc(size_t i) {
    if (i + 3 >= lines_.size())
        return false;

    for (size_t j = i; j <= i + 3; ++j) {
        if (!lines_[j].label.empty())
            return false;
    }

    if (lines_[i].mnemonic != "push" || trim(lines_[i].operands) != "de")
        return false;
    if (lines_[i + 1].mnemonic != "pop" ||
        trim(lines_[i + 1].operands) != "hl") {
        return false;
    }

    std::string dst, src;
    if (lines_[i + 2].mnemonic != "ld" ||
        !split_ld(lines_[i + 2].operands, dst, src) ||
        trim(dst) != "b" || trim(src) != "h") {
        return false;
    }
    if (lines_[i + 3].mnemonic != "ld" ||
        !split_ld(lines_[i + 3].operands, dst, src) ||
        trim(dst) != "c" || trim(src) != "l") {
        return false;
    }

    const bool copies_low_to_a =
        i + 4 < lines_.size() &&
        lines_[i + 4].label.empty() &&
        lines_[i + 4].mnemonic == "ld" &&
        split_ld(lines_[i + 4].operands, dst, src) &&
        trim(dst) == "a" && trim(src) == "l";

    const size_t next = i + (copies_low_to_a ? 5 : 4);
    if (!hl_dead_before_read_or_modern_return(lines_, next))
        return false;

    lines_[i].mnemonic = "ld";
    lines_[i].operands = "b, d";
    lines_[i + 1].mnemonic = "ld";
    lines_[i + 1].operands = "c, e";
    if (copies_low_to_a) {
        lines_[i + 2].mnemonic = "ld";
        lines_[i + 2].operands = "a, e";
        lines_.erase(lines_.begin() + i + 3, lines_.begin() + i + 5);
    } else {
        lines_.erase(lines_.begin() + i + 2, lines_.begin() + i + 4);
    }
    return true;
}

bool z80_peep::rule_de_result_hl_forward(size_t i) {
    if (i + 2 >= lines_.size())
        return false;
    if (!lines_[i].label.empty() || !lines_[i + 1].label.empty())
        return false;
    if (lines_[i].mnemonic != "push" || trim(lines_[i].operands) != "de")
        return false;
    if (lines_[i + 1].mnemonic != "pop" ||
        trim(lines_[i + 1].operands) != "hl") {
        return false;
    }

    struct rewrite {
        size_t index;
        std::string operands;
    };
    std::vector<rewrite> rewrites;
    size_t k = i + 2;
    const size_t limit = std::min(lines_.size(), i + 14);
    for (; k < limit; ++k) {
        asm_line &line = lines_[k];
        if (!line.label.empty())
            break;

        if (line.mnemonic == "ld") {
            std::string dst, src;
            if (!split_ld(line.operands, dst, src))
                break;
            dst = trim(dst);
            src = trim(src);

            if (src == "h") {
                rewrites.push_back({k, dst + ", d"});
                continue;
            }
            if (src == "l") {
                rewrites.push_back({k, dst + ", e"});
                continue;
            }

            if (operand_mentions_pair_or_bytes(line.operands, "hl", 'l', 'h'))
                break;
            continue;
        }

        if (line.mnemonic == "or") {
            std::string dst, src;
            if (!split_ld(line.operands, dst, src))
                break;
            dst = trim(dst);
            src = trim(src);
            if (src == "h") {
                rewrites.push_back({k, dst + ", d"});
                continue;
            }
            if (src == "l") {
                rewrites.push_back({k, dst + ", e"});
                continue;
            }
            if (operand_mentions_pair_or_bytes(line.operands, "hl", 'l', 'h'))
                break;
            continue;
        }

        if (!line_preserves_pair_value(line, "hl", 'l', 'h'))
            break;
    }

    if (rewrites.empty())
        return false;
    if (!hl_dead_before_read_or_modern_return(lines_, k)) {
        return false;
    }

    for (const auto &rw : rewrites)
        lines_[rw.index].operands = rw.operands;
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 2));
    return true;
}

bool z80_peep::rule_adjacent_indexed_byte_stores_postinc(size_t i) {
    if (i + 20 >= lines_.size())
        return false;
    for (size_t j = i; j <= i + 20; ++j) {
        if (!lines_[j].label.empty())
            return false;
    }

    auto parse_ld_at = [&](size_t pos, std::string &dst, std::string &src) {
        if (lines_[pos].mnemonic != "ld")
            return false;
        if (!split_ld(lines_[pos].operands, dst, src))
            return false;
        dst = trim(dst);
        src = trim(src);
        return true;
    };

    auto match_ld = [&](size_t pos, const std::string &want_dst,
                        std::string &src) {
        std::string dst;
        if (!parse_ld_at(pos, dst, src))
            return false;
        return dst == want_dst;
    };

    std::string idx_lo;
    std::string idx_hi;
    std::string tmp_lo;
    std::string tmp_hi;
    std::string base0;
    std::string base1;
    std::string src;
    std::string first_value;
    std::string second_value;
    const std::string add0 = lower_copy(trim(lines_[i + 8].operands));
    const std::string add1 = lower_copy(trim(lines_[i + 18].operands));

    if (!match_ld(i, "l", idx_lo) ||
        !match_ld(i + 1, "h", idx_hi) ||
        lines_[i + 2].mnemonic != "inc" ||
        trim(lines_[i + 2].operands) != "hl" ||
        !parse_ld_at(i + 3, tmp_lo, src) ||
        src != "l" ||
        !parse_ld_at(i + 4, tmp_hi, src) ||
        src != "h" ||
        !match_ld(i + 5, "hl", base0) ||
        !match_ld(i + 6, "e", src) ||
        src != idx_lo ||
        !match_ld(i + 7, "d", src) ||
        src != idx_hi ||
        lines_[i + 8].mnemonic != "add" ||
        (add0 != "hl, de" && add0 != "hl,de") ||
        !match_ld(i + 9, "(hl)", first_value) ||
        !match_ld(i + 10, "l", src) ||
        src != tmp_lo ||
        !match_ld(i + 11, "h", src) ||
        src != tmp_hi ||
        lines_[i + 12].mnemonic != "inc" ||
        trim(lines_[i + 12].operands) != "hl" ||
        !match_ld(i + 13, idx_lo, src) ||
        src != "l" ||
        !match_ld(i + 14, idx_hi, src) ||
        src != "h" ||
        !match_ld(i + 15, "hl", base1) ||
        base1 != base0 ||
        !match_ld(i + 16, "e", src) ||
        src != tmp_lo ||
        !match_ld(i + 17, "d", src) ||
        src != tmp_hi ||
        lines_[i + 18].mnemonic != "add" ||
        (add1 != "hl, de" && add1 != "hl,de") ||
        !match_ld(i + 19, "a", second_value) ||
        !match_ld(i + 20, "(hl)", src) ||
        src != "a") {
        return false;
    }

    int tmp_lo_off = 0;
    int tmp_hi_off = 0;
    if (!parse_ix_ref(tmp_lo, tmp_lo_off) ||
        !parse_ix_ref(tmp_hi, tmp_hi_off)) {
        return false;
    }
    if (ix_slot_referenced_after(lines_, i + 21, tmp_lo_off) ||
        ix_slot_referenced_after(lines_, i + 21, tmp_hi_off)) {
        return false;
    }

    if (operand_mentions_pair_or_bytes(second_value, "hl", 'l', 'h') ||
        operand_mentions_pair_or_bytes(second_value, "de", 'e', 'd')) {
        return false;
    }

    const size_t after = i + 21;
    if (!pair_value_dead_or_call_or_modern_return_before_read(
            lines_, after, "hl", 'l', 'h') ||
        !pair_value_dead_or_call_or_modern_return_before_read(
            lines_, after, "de", 'e', 'd') ||
        !flags_overwritten_before_read_or_escape(lines_, after)) {
        return false;
    }

    std::vector<asm_line> repl;
    repl.push_back(asm_line::parse("\tld\thl, " + base0));
    repl.push_back(asm_line::parse("\tld\te, " + idx_lo));
    repl.push_back(asm_line::parse("\tld\td, " + idx_hi));
    repl.push_back(asm_line::parse("\tadd\thl, de"));
    repl.push_back(asm_line::parse("\tld\t(hl), " + first_value));
    repl.push_back(asm_line::parse("\tinc\thl"));
    repl.push_back(asm_line::parse("\tld\ta, " + second_value));
    repl.push_back(asm_line::parse("\tld\t(hl), a"));
    repl.push_back(asm_line::parse("\tld\tl, " + idx_lo));
    repl.push_back(asm_line::parse("\tld\th, " + idx_hi));
    repl.push_back(asm_line::parse("\tinc\thl"));
    repl.push_back(asm_line::parse("\tinc\thl"));
    repl.push_back(asm_line::parse("\tld\t" + idx_lo + ", l"));
    repl.push_back(asm_line::parse("\tld\t" + idx_hi + ", h"));

    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 21));
    lines_.insert(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                  repl.begin(), repl.end());
    return true;
}

bool z80_peep::rule_dead_bc_zero_extend_from_a(size_t i) {
    if (i + 1 >= lines_.size())
        return false;
    if (!lines_[i].label.empty() || !lines_[i + 1].label.empty())
        return false;
    if (lines_[i].mnemonic != "ld" || lines_[i + 1].mnemonic != "ld")
        return false;

    std::string dst;
    std::string src;
    if (!split_ld(lines_[i].operands, dst, src) ||
        trim(dst) != "c" || trim(src) != "a") {
        return false;
    }
    if (!split_ld(lines_[i + 1].operands, dst, src) ||
        trim(dst) != "b" || !immediate_is(src, 0)) {
        return false;
    }
    if (!bc_dead_before_read_or_ret(lines_, i + 2))
        return false;

    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 2));
    return true;
}

bool z80_peep::rule_dead_bc_copy_from_hl(size_t i) {
    if (i + 1 >= lines_.size())
        return false;
    if (!lines_[i].label.empty() || !lines_[i + 1].label.empty())
        return false;
    if (lines_[i].mnemonic != "ld" || lines_[i + 1].mnemonic != "ld")
        return false;

    std::string dst;
    std::string src;
    if (!split_ld(lines_[i].operands, dst, src) ||
        trim(dst) != "b" || trim(src) != "h") {
        return false;
    }
    if (!split_ld(lines_[i + 1].operands, dst, src) ||
        trim(dst) != "c" || trim(src) != "l") {
        return false;
    }
    if (!path_overwrites_pair_before_read(lines_, i + 2, "bc", 'c', 'b'))
        return false;

    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 2));
    return true;
}

bool z80_peep::rule_bc_indirect_through_hl(size_t i) {
    if (i + 2 >= lines_.size())
        return false;

    for (size_t j = i; j <= i + 2; ++j) {
        if (!lines_[j].label.empty())
            return false;
    }
    if (lines_[i].mnemonic != "ld" ||
        lines_[i + 1].mnemonic != "ld" ||
        lines_[i + 2].mnemonic != "ld") {
        return false;
    }

    std::string dst;
    std::string src;
    if (!split_ld(lines_[i].operands, dst, src) ||
        trim(dst) != "b" || trim(src) != "h") {
        return false;
    }
    if (!split_ld(lines_[i + 1].operands, dst, src) ||
        trim(dst) != "c" || trim(src) != "l") {
        return false;
    }
    if (!split_ld(lines_[i + 2].operands, dst, src))
        return false;

    dst = trim(dst);
    src = trim(src);
    std::string replacement;
    if (dst == "a" && src == "(bc)") {
        replacement = "a, (hl)";
    } else if (dst == "(bc)" && src == "a") {
        replacement = "(hl), a";
    } else {
        return false;
    }

    if (!path_overwrites_bc_before_read_or_call(lines_, i + 3))
        return false;

    lines_[i].operands = replacement;
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 1),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 3));
    return true;
}

bool z80_peep::rule_dead_bc_hl_roundtrip(size_t i) {
    if (i + 3 >= lines_.size())
        return false;

    for (size_t j = i; j <= i + 3; ++j) {
        if (!lines_[j].label.empty() || lines_[j].mnemonic != "ld")
            return false;
    }

    std::string dst, src;
    if (!split_ld(lines_[i].operands, dst, src) ||
        trim(dst) != "b" || trim(src) != "h") {
        return false;
    }
    if (!split_ld(lines_[i + 1].operands, dst, src) ||
        trim(dst) != "c" || trim(src) != "l") {
        return false;
    }
    if (!split_ld(lines_[i + 2].operands, dst, src) ||
        trim(dst) != "h" || trim(src) != "b") {
        return false;
    }
    if (!split_ld(lines_[i + 3].operands, dst, src) ||
        trim(dst) != "l" || trim(src) != "c") {
        return false;
    }

    if (!bc_dead_before_read_or_ret(lines_, i + 4))
        return false;

    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 4));
    return true;
}

bool z80_peep::rule_bc_base_add_direct(size_t i) {
    if (i + 3 >= lines_.size())
        return false;

    for (size_t j = i; j <= i + 3; ++j) {
        if (!lines_[j].label.empty())
            return false;
    }
    if (lines_[i].mnemonic != "ld" || lines_[i + 1].mnemonic != "ld" ||
        lines_[i + 2].mnemonic != "ld" || lines_[i + 3].mnemonic != "add") {
        return false;
    }

    std::string dst, src;
    if (!split_ld(lines_[i].operands, dst, src) ||
        trim(dst) != "b" || trim(src) != "h") {
        return false;
    }
    if (!split_ld(lines_[i + 1].operands, dst, src) ||
        trim(dst) != "c" || trim(src) != "l") {
        return false;
    }
    if (!split_ld(lines_[i + 2].operands, dst, src) ||
        trim(dst) != "hl") {
        return false;
    }
    src = trim(src);
    if (!is_immediate_operand(src) && !is_numeric_literal(src))
        return false;
    if (trim(lines_[i + 3].operands) != "hl,bc" &&
        trim(lines_[i + 3].operands) != "hl, bc") {
        return false;
    }

    if (!path_overwrites_bc_before_read_or_call(lines_, i + 4))
        return false;

    lines_[i].operands = "bc, " + src;
    lines_[i + 1].mnemonic = "add";
    lines_[i + 1].operands = "hl, bc";
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 2),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 4));
    return true;
}

bool z80_peep::rule_bc_offset_base_add_de_direct(size_t i) {
    if (i + 5 >= lines_.size())
        return false;

    for (size_t j = i; j <= i + 5; ++j) {
        if (!lines_[j].label.empty())
            return false;
    }
    for (size_t j = i; j <= i + 4; ++j) {
        if (lines_[j].mnemonic != "ld")
            return false;
    }
    if (lines_[i + 5].mnemonic != "add")
        return false;

    std::string dst;
    std::string src;
    if (!split_ld(lines_[i].operands, dst, src) ||
        trim(dst) != "b" || trim(src) != "h") {
        return false;
    }
    if (!split_ld(lines_[i + 1].operands, dst, src) ||
        trim(dst) != "c" || trim(src) != "l") {
        return false;
    }
    if (!split_ld(lines_[i + 2].operands, dst, src) ||
        trim(dst) != "hl") {
        return false;
    }
    src = trim(src);
    if (!is_immediate_operand(src) && !is_numeric_literal(src))
        return false;
    if (!split_ld(lines_[i + 3].operands, dst, src) ||
        trim(dst) != "d" || trim(src) != "b") {
        return false;
    }
    if (!split_ld(lines_[i + 4].operands, dst, src) ||
        trim(dst) != "e" || trim(src) != "c") {
        return false;
    }
    if (trim(lines_[i + 5].operands) != "hl,de" &&
        trim(lines_[i + 5].operands) != "hl, de") {
        return false;
    }

    if (!path_overwrites_bc_before_read_or_call(lines_, i + 6))
        return false;

    lines_[i].mnemonic = "ex";
    lines_[i].operands = "de, hl";
    lines_[i + 1] = lines_[i + 2];
    lines_[i + 2].mnemonic = "add";
    lines_[i + 2].operands = "hl, de";
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 3),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 6));
    return true;
}

bool z80_peep::rule_bc_index_add_hl_word_load_direct(size_t i) {
    if (i + 7 >= lines_.size())
        return false;

    for (size_t j = i; j <= i + 3; ++j) {
        if (!lines_[j].label.empty() || lines_[j].mnemonic != "ld")
            return false;
    }
    if (lines_[i + 4].mnemonic != "add")
        return false;

    std::string dst;
    std::string src;
    if (!split_ld(lines_[i].operands, dst, src) ||
        trim(dst) != "b" || trim(src) != "h") {
        return false;
    }
    if (!split_ld(lines_[i + 1].operands, dst, src) ||
        trim(dst) != "c" || trim(src) != "l") {
        return false;
    }

    std::string lo_src;
    std::string hi_src;
    if (!split_ld(lines_[i + 2].operands, dst, lo_src) ||
        trim(dst) != "l") {
        return false;
    }
    if (!split_ld(lines_[i + 3].operands, dst, hi_src) ||
        trim(dst) != "h") {
        return false;
    }
    lo_src = trim(lo_src);
    hi_src = trim(hi_src);
    if (operand_has_token(lo_src, "d") || operand_has_token(lo_src, "e") ||
        operand_has_token(lo_src, "de") || operand_has_token(hi_src, "d") ||
        operand_has_token(hi_src, "e") || operand_has_token(hi_src, "de")) {
        return false;
    }
    if (trim(lines_[i + 4].operands) != "hl,bc" &&
        trim(lines_[i + 4].operands) != "hl, bc") {
        return false;
    }

    size_t load_lo = i + 5;
    while (load_lo < lines_.size() &&
           load_lo <= i + 9 &&
           lines_[load_lo].label.empty() &&
           lines_[load_lo].mnemonic == "inc" &&
           trim(lines_[load_lo].operands) == "hl") {
        ++load_lo;
    }
    if (load_lo + 2 >= lines_.size())
        return false;
    for (size_t j = i + 5; j <= load_lo + 2; ++j) {
        if (!lines_[j].label.empty())
            return false;
    }

    if (lines_[load_lo].mnemonic != "ld" ||
        !split_ld(lines_[load_lo].operands, dst, src) ||
        trim(dst) != "e" || trim(src) != "(hl)") {
        return false;
    }
    if (lines_[load_lo + 1].mnemonic != "inc" ||
        trim(lines_[load_lo + 1].operands) != "hl") {
        return false;
    }
    if (lines_[load_lo + 2].mnemonic != "ld" ||
        !split_ld(lines_[load_lo + 2].operands, dst, src) ||
        trim(dst) != "d" || trim(src) != "(hl)") {
        return false;
    }

    if (!path_overwrites_bc_before_read_or_call(lines_, load_lo + 3))
        return false;

    lines_[i].mnemonic = "ex";
    lines_[i].operands = "de, hl";
    lines_[i + 1].operands = "l, " + lo_src;
    lines_[i + 2].operands = "h, " + hi_src;
    lines_[i + 3].mnemonic = "add";
    lines_[i + 3].operands = "hl, de";
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 4));
    return true;
}

bool z80_peep::rule_bc_index_add_reloaded_hl_to_de(size_t i) {
    if (i + 4 >= lines_.size())
        return false;

    for (size_t j = i; j <= i + 4; ++j) {
        if (!lines_[j].label.empty())
            return false;
    }
    if (lines_[i].mnemonic != "ld" || lines_[i + 1].mnemonic != "ld" ||
        lines_[i + 2].mnemonic != "ld" || lines_[i + 3].mnemonic != "ld" ||
        lines_[i + 4].mnemonic != "add") {
        return false;
    }

    std::string dst, src;
    if (!split_ld(lines_[i].operands, dst, src) ||
        trim(dst) != "b" || trim(src) != "h") {
        return false;
    }
    if (!split_ld(lines_[i + 1].operands, dst, src) ||
        trim(dst) != "c" || trim(src) != "l") {
        return false;
    }

    std::string lo_src;
    if (!split_ld(lines_[i + 2].operands, dst, lo_src) || trim(dst) != "l")
        return false;
    std::string hi_src;
    if (!split_ld(lines_[i + 3].operands, dst, hi_src) || trim(dst) != "h")
        return false;
    lo_src = trim(lo_src);
    hi_src = trim(hi_src);
    if (operand_has_token(lo_src, "d") || operand_has_token(lo_src, "e") ||
        operand_has_token(lo_src, "de") || operand_has_token(hi_src, "d") ||
        operand_has_token(hi_src, "e") || operand_has_token(hi_src, "de")) {
        return false;
    }
    if (trim(lines_[i + 4].operands) != "hl,bc" &&
        trim(lines_[i + 4].operands) != "hl, bc") {
        return false;
    }

    if (!path_overwrites_bc_before_read_or_call(lines_, i + 5))
        return false;
    if (!path_overwrites_pair_before_read(lines_, i + 5, "de", 'e', 'd'))
        return false;

    lines_[i].mnemonic = "ex";
    lines_[i].operands = "de, hl";
    lines_[i + 1].operands = "l, " + lo_src;
    lines_[i + 2].operands = "h, " + hi_src;
    lines_[i + 3].mnemonic = "add";
    lines_[i + 3].operands = "hl, de";
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 4));
    return true;
}

bool z80_peep::rule_bc_saved_hl_push_word_to_de_direct(size_t i) {
    if (i + 7 >= lines_.size())
        return false;

    for (size_t j = i; j <= i + 7; ++j) {
        if (!lines_[j].label.empty())
            return false;
    }
    if (lines_[i].mnemonic != "ld" || lines_[i + 1].mnemonic != "ld" ||
        lines_[i + 2].mnemonic != "ld" || lines_[i + 3].mnemonic != "ld" ||
        lines_[i + 4].mnemonic != "push" ||
        lines_[i + 5].mnemonic != "ld" || lines_[i + 6].mnemonic != "ld" ||
        lines_[i + 7].mnemonic != "pop") {
        return false;
    }

    std::string dst;
    std::string src;
    if (!split_ld(lines_[i].operands, dst, src) ||
        trim(dst) != "b" || trim(src) != "h") {
        return false;
    }
    if (!split_ld(lines_[i + 1].operands, dst, src) ||
        trim(dst) != "c" || trim(src) != "l") {
        return false;
    }

    std::string lo_src;
    std::string hi_src;
    if (!split_ld(lines_[i + 2].operands, dst, lo_src) || trim(dst) != "l")
        return false;
    if (!split_ld(lines_[i + 3].operands, dst, hi_src) || trim(dst) != "h")
        return false;
    lo_src = trim(lo_src);
    hi_src = trim(hi_src);

    if (trim(lines_[i + 4].operands) != "hl")
        return false;
    if (!split_ld(lines_[i + 5].operands, dst, src) ||
        trim(dst) != "h" || trim(src) != "b") {
        return false;
    }
    if (!split_ld(lines_[i + 6].operands, dst, src) ||
        trim(dst) != "l" || trim(src) != "c") {
        return false;
    }
    if (trim(lines_[i + 7].operands) != "de")
        return false;

    // The original sequence loads the pushed word through HL before restoring
    // HL from BC. Keep this narrow: only rewrite memory/immediate sources that
    // do not depend on the pair registers being shuffled here.
    for (const std::string &operand : {lo_src, hi_src}) {
        if (operand_has_token(operand, "bc") ||
            operand_has_token(operand, "de") ||
            operand_has_token(operand, "hl") ||
            operand_has_token(operand, "b") ||
            operand_has_token(operand, "c") ||
            operand_has_token(operand, "d") ||
            operand_has_token(operand, "e") ||
            operand_has_token(operand, "h") ||
            operand_has_token(operand, "l")) {
            return false;
        }
    }

    lines_[i + 2].operands = "e, " + lo_src;
    lines_[i + 3].operands = "d, " + hi_src;
    lines_[i + 4] = lines_[i + 5];
    lines_[i + 5] = lines_[i + 6];
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 6),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 8));
    return true;
}

bool z80_peep::rule_dead_bc_hl_to_de_copy(size_t i) {
    if (i + 3 >= lines_.size())
        return false;

    for (size_t j = i; j <= i + 3; ++j) {
        if (!lines_[j].label.empty() || lines_[j].mnemonic != "ld")
            return false;
    }

    std::string dst, src;
    if (!split_ld(lines_[i].operands, dst, src) ||
        trim(dst) != "b" || trim(src) != "h") {
        return false;
    }
    if (!split_ld(lines_[i + 1].operands, dst, src) ||
        trim(dst) != "c" || trim(src) != "l") {
        return false;
    }
    if (!split_ld(lines_[i + 2].operands, dst, src) ||
        trim(dst) != "d" || trim(src) != "b") {
        return false;
    }
    if (!split_ld(lines_[i + 3].operands, dst, src) ||
        trim(dst) != "e" || trim(src) != "c") {
        return false;
    }
    if (!path_overwrites_bc_before_read_or_call(lines_, i + 4))
        return false;

    lines_[i] = asm_line::parse("\tld\td, h");
    lines_[i + 1] = asm_line::parse("\tld\te, l");
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 2),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 4));
    return true;
}

bool z80_peep::rule_de_hl_equal_load_exchange(size_t i) {
    if (i + 4 >= lines_.size())
        return false;
    if (!lines_[i].label.empty() || !lines_[i + 1].label.empty())
        return false;
    if (lines_[i].mnemonic != "push" || trim(lines_[i].operands) != "de" ||
        lines_[i + 1].mnemonic != "pop" || trim(lines_[i + 1].operands) != "hl") {
        return false;
    }

    auto preserves_hl_de_copy = [](const asm_line &line) {
        if (!line.label.empty() || is_section_directive(line))
            return false;
        if (line.mnemonic.empty())
            return true;
        if (line.mnemonic != "ld")
            return false;

        std::string dst;
        std::string src;
        if (!split_ld(line.operands, dst, src))
            return false;
        dst = trim(dst);
        return !operand_mentions_pair_or_bytes(dst, "hl", 'l', 'h') &&
               !operand_mentions_pair_or_bytes(dst, "de", 'e', 'd') &&
               dst != "sp";
    };

    size_t ex_idx = lines_.size();
    const size_t end = std::min(lines_.size(), i + 8);
    auto is_ex_de_hl_text = [](const asm_line &line) {
        const std::string ops = trim(line.operands);
        return line.mnemonic == "ex" && (ops == "de, hl" || ops == "de,hl");
    };

    for (size_t j = i + 2; j + 2 < end; ++j) {
        if (!lines_[j].label.empty())
            return false;
        if (is_ex_de_hl_text(lines_[j])) {
            ex_idx = j;
            break;
        }
        if (!preserves_hl_de_copy(lines_[j]))
            return false;
    }
    if (ex_idx == lines_.size())
        return false;

    for (size_t j = i + 2; j < ex_idx; ++j) {
        if (!preserves_hl_de_copy(lines_[j]))
            return false;
    }

    std::string dst;
    std::string src;
    if (lines_[ex_idx + 1].mnemonic != "ld" ||
        !split_ld(lines_[ex_idx + 1].operands, dst, src) ||
        trim(dst) != "hl" ||
        trim(src).empty() || trim(src)[0] != '#') {
        return false;
    }
    const std::string imm = trim(src);
    if (!is_ex_de_hl_text(lines_[ex_idx + 2]) ||
        !lines_[ex_idx + 1].label.empty() ||
        !lines_[ex_idx + 2].label.empty()) {
        return false;
    }

    lines_[ex_idx] = asm_line::parse("\tld\tde, " + imm);
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(ex_idx + 1),
                 lines_.begin() + static_cast<std::ptrdiff_t>(ex_idx + 3));
    return true;
}

bool z80_peep::rule_ix_pair_compare_load_de_direct(size_t i) {
    if (i + 6 >= lines_.size())
        return false;
    for (size_t k = i; k <= i + 6; ++k) {
        if (!lines_[k].label.empty())
            return false;
    }

    auto load_ix_byte = [&](size_t idx, const char *dst_want,
                            int &offset_out) {
        if (lines_[idx].mnemonic != "ld")
            return false;
        std::string dst;
        std::string src;
        return split_ld(lines_[idx].operands, dst, src) &&
               trim(dst) == dst_want &&
               parse_ix_ref(trim(src), offset_out);
    };

    int left_lo = 0;
    int left_hi = 0;
    int right_lo = 0;
    int right_hi = 0;
    if (!load_ix_byte(i, "l", left_lo) ||
        !load_ix_byte(i + 1, "h", left_hi) ||
        left_hi != left_lo + 1) {
        return false;
    }
    if (!is_ex_de_hl(lines_[i + 2]))
        return false;
    if (!load_ix_byte(i + 3, "l", right_lo) ||
        !load_ix_byte(i + 4, "h", right_hi) ||
        right_hi != right_lo + 1) {
        return false;
    }
    if (!is_or_a_self(lines_[i + 5]) ||
        lines_[i + 6].mnemonic != "sbc" ||
        trim(lines_[i + 6].operands) != "hl, de") {
        return false;
    }

    lines_[i].operands = "e, " + std::to_string(left_lo) + "(ix)";
    lines_[i + 1].operands = "d, " + std::to_string(left_hi) + "(ix)";
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 2));
    return true;
}

bool z80_peep::rule_push_pair_copy_adjacent_speed(size_t i) {
    if (i + 1 >= lines_.size()) return false;

    struct copy_desc {
        const char *src;
        char src_lo;
        char src_hi;
        const char *dst;
        char dst_lo;
        char dst_hi;
    };
    static const copy_desc copies[] = {
        {"de", 'e', 'd', "hl", 'l', 'h'},
        {"de", 'e', 'd', "bc", 'c', 'b'},
        {"bc", 'c', 'b', "hl", 'l', 'h'},
        {"bc", 'c', 'b', "de", 'e', 'd'},
    };

    auto &push = lines_[i];
    auto &pop = lines_[i + 1];
    if (push.mnemonic != "push" || pop.mnemonic != "pop")
        return false;
    if (!pop.label.empty())
        return false;

    const std::string pushed = trim(push.operands);
    const std::string popped = trim(pop.operands);
    for (const auto &copy : copies) {
        if (pushed != copy.src || popped != copy.dst)
            continue;
        push.mnemonic = "ld";
        push.operands = std::string(1, copy.dst_hi) + ", " +
                        std::string(1, copy.src_hi);
        pop.mnemonic = "ld";
        pop.operands = std::string(1, copy.dst_lo) + ", " +
                       std::string(1, copy.src_lo);
        return true;
    }
    return false;
}

bool z80_peep::rule_add_a_one_to_inc(size_t i) {
    if (i >= lines_.size() || !lines_[i].label.empty())
        return false;

    int value = 0;
    if (!parse_accumulator_immediate_alu(lines_[i], "add", value) ||
        u8_value(value) != 1 ||
        !flags_overwritten_before_read_or_escape(lines_, i + 1)) {
        return false;
    }

    lines_[i].mnemonic = "inc";
    lines_[i].operands = "a";
    return true;
}

bool z80_peep::rule_redundant_a_reload_after_zero_extend(size_t i) {
    if (i + 2 >= lines_.size())
        return false;
    for (size_t j = i; j <= i + 2; ++j)
        if (!lines_[j].label.empty())
            return false;

    std::string dst;
    std::string src;
    if (lines_[i].mnemonic != "ld" ||
        !split_ld(lines_[i].operands, dst, src) ||
        trim(dst) != "l" || trim(src) != "a") {
        return false;
    }
    if (lines_[i + 1].mnemonic != "ld" ||
        !split_ld(lines_[i + 1].operands, dst, src) ||
        trim(dst) != "h" || !immediate_is(src, 0)) {
        return false;
    }
    if (lines_[i + 2].mnemonic != "ld" ||
        !split_ld(lines_[i + 2].operands, dst, src) ||
        trim(dst) != "a" || trim(src) != "l") {
        return false;
    }

    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 2));
    return true;
}

bool z80_peep::rule_superopt_accumulator_sequences(size_t i) {
    if (i + 1 >= lines_.size())
        return false;

    auto &a = lines_[i];
    auto &b = lines_[i + 1];
    if (!b.label.empty())
        return false;

    // and #0; neg  ->  sub a
    if (a.mnemonic == "and" && immediate_is(a.operands, 0) &&
        b.mnemonic == "neg" && trim(b.operands).empty()) {
        a.mnemonic = "sub";
        a.operands = "a";
        lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 1));
        return true;
    }

    // add a,a; rr a  ->  or a
    if (a.mnemonic == "add") {
        std::string dst, src;
        if (split_ld(a.operands, dst, src) &&
            trim(dst) == "a" && trim(src) == "a" &&
            b.mnemonic == "rr" && trim(b.operands) == "a") {
            a.mnemonic = "or";
            a.operands = "a";
            lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 1));
            return true;
        }
    }

    // sla a; rr a  ->  or a
    if (a.mnemonic == "sla" && trim(a.operands) == "a" &&
        b.mnemonic == "rr" && trim(b.operands) == "a") {
        a.mnemonic = "or";
        a.operands = "a";
        lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 1));
        return true;
    }

    // srl a; rl a  ->  or a
    if (a.mnemonic == "srl" && trim(a.operands) == "a" &&
        b.mnemonic == "rl" && trim(b.operands) == "a") {
        a.mnemonic = "or";
        a.operands = "a";
        lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 1));
        return true;
    }

    // and #255; rr a  ->  srl a
    if (a.mnemonic == "and" && immediate_is(a.operands, 255) &&
        b.mnemonic == "rr" && trim(b.operands) == "a") {
        a.mnemonic = "srl";
        a.operands = "a";
        lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 1));
        return true;
    }

    // add a,#128; or a  ->  xor #128
    if (a.mnemonic == "add" && is_or_a_self(b)) {
        std::string dst, src;
        if (split_ld(a.operands, dst, src) &&
            trim(dst) == "a" && immediate_is(src, 128)) {
            a.mnemonic = "xor";
            a.operands = "#128";
            lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 1));
            return true;
        }
    }

    // scf; adc a,#0  ->  add a,#1
    if (a.mnemonic == "scf" && trim(a.operands).empty() &&
        b.mnemonic == "adc") {
        std::string dst, src;
        if (split_ld(b.operands, dst, src) &&
            trim(dst) == "a" && immediate_is(src, 0)) {
            a.mnemonic = "add";
            a.operands = "a, #1";
            lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 1));
            return true;
        }
    }

    // cpl; xor #255  ->  or a
    if (a.mnemonic == "cpl" && trim(a.operands).empty() &&
        b.mnemonic == "xor" && immediate_is(b.operands, 255)) {
        a.mnemonic = "or";
        a.operands = "a";
        lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 1));
        return true;
    }

    // xor x; xor x  ->  or a  (or a single xor a for the zeroing case)
    if (a.mnemonic == "xor" && b.mnemonic == "xor") {
        std::string lhs = lower_copy(trim(a.operands));
        std::string rhs = lower_copy(trim(b.operands));
        if (lhs == rhs && !operand_uses_memory(lhs)) {
            if (lhs == "a") {
                lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 1));
            } else {
                a.mnemonic = "or";
                a.operands = "a";
                lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 1));
            }
            return true;
        }
    }

    // and/or/xor immediate pairs form a tiny exact boolean-algebra
    // superoptimizer.  The final flags are those of the second logical op,
    // so a single logical op with the composed immediate is equivalent.
    int av = 0;
    int bv = 0;
    if (parse_accumulator_immediate_alu(a, "and", av) &&
        parse_accumulator_immediate_alu(b, "and", bv)) {
        a.operands = imm8_text(av & bv);
        lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 1));
        return true;
    }
    if (parse_accumulator_immediate_alu(a, "or", av) &&
        parse_accumulator_immediate_alu(b, "or", bv)) {
        const int combined = u8_value(av | bv);
        if (combined == 0) {
            a.operands = "a";
        } else {
            a.operands = imm8_text(combined);
        }
        lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 1));
        return true;
    }
    if (parse_accumulator_immediate_alu(a, "xor", av) &&
        parse_accumulator_immediate_alu(b, "xor", bv)) {
        const int combined = u8_value(av ^ bv);
        if (combined == 0) {
            a.mnemonic = "or";
            a.operands = "a";
        } else {
            a.operands = imm8_text(combined);
        }
        lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 1));
        return true;
    }

    // ld a,#k; xor a  ->  xor a
    // ld a,#0; or a   ->  xor a
    // The final zeroing/test instruction fully determines A and flags.
    if (a.mnemonic == "ld") {
        std::string dst, src;
        int loaded = 0;
        if (split_ld(a.operands, dst, src) &&
            trim(dst) == "a" && parse_immediate_value(src, loaded)) {
            if (b.mnemonic == "xor" && trim(b.operands) == "a") {
                lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i));
                return true;
            }
            if (u8_value(loaded) == 0 && is_or_a_self(b)) {
                a.mnemonic = "xor";
                a.operands = "a";
                lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 1));
                return true;
            }
        }
    }

    // ld a,#k; logical-immediate  ->  ld a,#folded; one-byte flag probe
    // This keeps exact logical-op flags while saving an immediate byte and
    // a few cycles.  AND needs AND A because Z80 AND sets H; OR/XOR use OR A.
    if (a.mnemonic == "ld") {
        std::string dst, src;
        int loaded = 0;
        if (split_ld(a.operands, dst, src) &&
            trim(dst) == "a" && parse_immediate_value(src, loaded)) {
            if (parse_accumulator_immediate_alu(b, "and", bv)) {
                a.operands = "a, " + imm8_text(loaded & bv);
                b.operands = "a";
                return true;
            }
            if (parse_accumulator_immediate_alu(b, "or", bv)) {
                a.operands = "a, " + imm8_text(loaded | bv);
                b.operands = "a";
                return true;
            }
            if (parse_accumulator_immediate_alu(b, "xor", bv)) {
                const int folded = u8_value(loaded ^ bv);
                if (folded == 0) {
                    a.mnemonic = "xor";
                    a.operands = "a";
                    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 1));
                } else {
                    a.operands = "a, " + imm8_text(folded);
                    b.mnemonic = "or";
                    b.operands = "a";
                }
                return true;
            }
        }
    }

    // xor #255; sbc a,#255  ->  neg
    if (a.mnemonic == "xor" && immediate_is(a.operands, 255) &&
        b.mnemonic == "sbc") {
        std::string dst, src;
        if (split_ld(b.operands, dst, src) &&
            trim(dst) == "a" && immediate_is(src, 255)) {
            a.mnemonic = "neg";
            a.operands.clear();
            lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 1));
            return true;
        }
    }

    // cpl; neg  ->  sub #255
    if (a.mnemonic == "cpl" && trim(a.operands).empty() &&
        b.mnemonic == "neg" && trim(b.operands).empty()) {
        a.mnemonic = "sub";
        a.operands = "#255";
        lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 1));
        return true;
    }

    // add a,#0; rl a  ->  sla a
    if (a.mnemonic == "add" && b.mnemonic == "rl" &&
        trim(b.operands) == "a") {
        std::string dst, src;
        if (split_ld(a.operands, dst, src) &&
            trim(dst) == "a" && immediate_is(src, 0)) {
            a.mnemonic = "sla";
            a.operands = "a";
            lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 1));
            return true;
        }
    }

    // res n,a; and a  ->  and #mask
    if (a.mnemonic == "res" && is_and_a_self(b)) {
        int bit = 0;
        std::string reg;
        if (parse_bit_reg_operands(a.operands, bit, reg) && reg == "a") {
            const int mask = 255 & ~(1 << bit);
            a.mnemonic = "and";
            a.operands = "#" + std::to_string(mask);
            lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 1));
            return true;
        }
    }

    // res n,a; and #k  ->  and #(k & ~bit)
    if (a.mnemonic == "res" &&
        parse_accumulator_immediate_alu(b, "and", bv)) {
        int bit = 0;
        std::string reg;
        if (parse_bit_reg_operands(a.operands, bit, reg) && reg == "a") {
            a.mnemonic = "and";
            a.operands = imm8_text(bv & ~(1 << bit));
            lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 1));
            return true;
        }
    }

    // set n,a; or a  ->  or #mask
    if (a.mnemonic == "set" && is_or_a_self(b)) {
        int bit = 0;
        std::string reg;
        if (parse_bit_reg_operands(a.operands, bit, reg) && reg == "a") {
            const int mask = 1 << bit;
            a.mnemonic = "or";
            a.operands = "#" + std::to_string(mask);
            lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 1));
            return true;
        }
    }

    // set n,a; or #k  ->  or #(k | bit)
    if (a.mnemonic == "set" &&
        parse_accumulator_immediate_alu(b, "or", bv)) {
        int bit = 0;
        std::string reg;
        if (parse_bit_reg_operands(a.operands, bit, reg) && reg == "a") {
            a.mnemonic = "or";
            a.operands = imm8_text(bv | (1 << bit));
            lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 1));
            return true;
        }
    }

    // Same-bit SET/RES chains overwrite the first operation.  Limit this to
    // registers so memory-mapped writes are never silently removed.
    if ((a.mnemonic == "set" || a.mnemonic == "res") &&
        (b.mnemonic == "set" || b.mnemonic == "res")) {
        int bit_a = 0;
        int bit_b = 0;
        std::string reg_a;
        std::string reg_b;
        if (parse_bit_reg_operands(a.operands, bit_a, reg_a) &&
            parse_bit_reg_operands(b.operands, bit_b, reg_b) &&
            bit_a == bit_b && reg_a == reg_b && is_plain_8bit_reg(reg_a)) {
            lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i));
            return true;
        }
    }

    // ld a,#0; shift/rotate-without-carry-input a  ->  xor a
    if (a.mnemonic == "ld" && is_accumulator_shift_without_carry_input(b)) {
        std::string dst, src;
        if (split_ld(a.operands, dst, src) &&
            trim(dst) == "a" && immediate_is(src, 0)) {
            a.mnemonic = "xor";
            a.operands = "a";
            lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 1));
            return true;
        }
    }

    return false;
}

bool z80_peep::rule_superopt_lowbit_pair_sequences(size_t i) {
    if (i + 1 >= lines_.size())
        return false;

    auto &bitop = lines_[i];
    auto &pair_op = lines_[i + 1];
    if (!pair_op.label.empty())
        return false;

    struct lowbit_pair {
        const char *lo;
        const char *pair;
    };
    static const lowbit_pair pairs[] = {
        {"l", "hl"},
        {"e", "de"},
        {"c", "bc"},
    };

    std::string bit, reg;
    if (!split_ld(bitop.operands, bit, reg) || trim(bit) != "0")
        return false;
    reg = trim(reg);

    for (const auto &p : pairs) {
        if (reg != p.lo || trim(pair_op.operands) != p.pair)
            continue;

        // set 0,l; dec hl  ->  res 0,l
        if (bitop.mnemonic == "set" && pair_op.mnemonic == "dec") {
            bitop.mnemonic = "res";
            bitop.operands = std::string("0, ") + p.lo;
            lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 1));
            return true;
        }

        // res 0,l; inc hl  ->  set 0,l
        if (bitop.mnemonic == "res" && pair_op.mnemonic == "inc") {
            bitop.mnemonic = "set";
            bitop.operands = std::string("0, ") + p.lo;
            lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 1));
            return true;
        }
    }

    return false;
}

bool z80_peep::rule_superopt_srl_a_const_shift(size_t i) {
    if (i >= lines_.size())
        return false;

    size_t count = 0;
    while (i + count < lines_.size() && count < 7) {
        const auto &line = lines_[i + count];
        if (!line.label.empty() ||
            line.mnemonic != "srl" || trim(line.operands) != "a") {
            break;
        }
        ++count;
    }
    if (count < 2)
        return false;

    const int mask = 0xff >> static_cast<int>(count);
    const size_t next_idx = i + count;
    bool fold_following_and = false;
    int following_and = 0;

    if (next_idx < lines_.size() && lines_[next_idx].label.empty() &&
        lines_[next_idx].mnemonic == "and" &&
        parse_immediate_value(lines_[next_idx].operands, following_and)) {
        fold_following_and = true;
    } else {
        if (next_idx >= lines_.size() || !lines_[next_idx].label.empty())
            return false;
        if (!line_overwrites_flags_without_reading_carry(lines_[next_idx]))
            return false;
    }

    std::vector<asm_line> replacement;
    const bool rotate_right = count <= 4;
    const size_t rotate_count = rotate_right ? count : 8 - count;
    const char *rotate = rotate_right ? "\trrca" : "\trlca";
    for (size_t n = 0; n < rotate_count; ++n)
        replacement.push_back(asm_line::parse(rotate));

    const int final_mask = fold_following_and
        ? (following_and & mask)
        : mask;
    replacement.push_back(asm_line::parse("\tand\t" + imm8_text(final_mask)));

    const size_t erase_end = next_idx + (fold_following_and ? 1 : 0);
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                 lines_.begin() + static_cast<std::ptrdiff_t>(erase_end));
    lines_.insert(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                  replacement.begin(), replacement.end());
    return true;
}

bool z80_peep::rule_superopt_shift5_pair_loop_unroll(size_t i) {
    if (i + 4 >= lines_.size())
        return false;

    const auto &setup = lines_[i];
    const auto &label = lines_[i + 1];
    const auto &high = lines_[i + 2];
    const auto &low = lines_[i + 3];
    const auto &loop = lines_[i + 4];

    if (!setup.label.empty() || setup.mnemonic != "ld")
        return false;
    std::string dst, src;
    if (!split_ld(setup.operands, dst, src) ||
        trim(dst) != "b" || !immediate_is(src, 5)) {
        return false;
    }

    if (label.label.empty() || !label.mnemonic.empty())
        return false;
    if (!high.label.empty() || high.mnemonic != "srl" ||
        trim(high.operands) != "l") {
        return false;
    }
    if (!low.label.empty() || low.mnemonic != "rr" ||
        trim(low.operands) != "a") {
        return false;
    }
    if (!loop.label.empty() || loop.mnemonic != "djnz" ||
        trim(loop.operands) != label.label) {
        return false;
    }
    if (label_has_other_control_references(lines_, label.label, i + 4))
        return false;
    if (!b_overwritten_before_read(lines_, i + 5))
        return false;

    std::vector<asm_line> replacement;
    replacement.reserve(10);
    for (int n = 0; n < 5; ++n) {
        replacement.push_back(asm_line::parse("\tsrl\tl"));
        replacement.push_back(asm_line::parse("\trr\ta"));
    }

    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 5));
    lines_.insert(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                  replacement.begin(), replacement.end());
    return true;
}

bool z80_peep::rule_superopt_hoist_d_zero_index_loop(size_t i) {
    if (i >= lines_.size())
        return false;

    const auto &label = lines_[i];
    if (label.label.empty() || !label.mnemonic.empty())
        return false;

    const size_t max_scan = std::min(lines_.size(), i + 64);
    size_t backedge = lines_.size();
    for (size_t k = i + 1; k < max_scan; ++k) {
        if (branch_targets_label(lines_[k], label.label)) {
            if (backedge != lines_.size())
                return false;
            backedge = k;
        }
    }
    if (backedge == lines_.size())
        return false;
    if (label_has_other_control_references(lines_, label.label, backedge))
        return false;

    size_t zero_idx = lines_.size();
    for (size_t k = i + 1; k + 2 < backedge; ++k) {
        const auto &load_base = lines_[k];
        const auto &zero = lines_[k + 1];
        const auto &add = lines_[k + 2];
        if (load_base.label.empty() && load_base.mnemonic == "ld" &&
            zero.label.empty() && is_ld_d_zero(zero) &&
            add.label.empty() && add.mnemonic == "add" &&
            trim(add.operands) == "hl, de") {
            zero_idx = k + 1;
            break;
        }
    }
    if (zero_idx == lines_.size())
        return false;

    for (size_t k = i + 1; k < backedge; ++k) {
        if (k == zero_idx)
            continue;
        const auto &line = lines_[k];
        if (!line.label.empty())
            return false;
        if (line_is_control_flow_boundary(line))
            return false;
        if (k == zero_idx + 1 &&
            line.mnemonic == "add" && trim(line.operands) == "hl, de") {
            continue;
        }
        if (operand_has_token(line.operands, "d") ||
            operand_has_token(line.operands, "de")) {
            return false;
        }
    }

    asm_line zero = lines_[zero_idx];
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(zero_idx));
    lines_.insert(lines_.begin() + static_cast<std::ptrdiff_t>(i), zero);
    return true;
}

bool z80_peep::rule_superopt_dead_flag_setter(size_t i) {
    if (i + 1 >= lines_.size())
        return false;

    const auto &flag_setter = lines_[i];
    if (!flag_setter.label.empty())
        return false;
    if (!line_is_flag_only_setter(flag_setter))
        return false;

    const size_t max_scan = std::min(lines_.size(), i + 33);
    for (size_t j = i + 1; j < max_scan; ++j) {
        const auto &line = lines_[j];
        if (!line.label.empty())
            return false;
        if (line.mnemonic.empty())
            continue;
        if (line_overwrites_flags_without_reading_carry(line)) {
            lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i));
            return true;
        }
        if (line_may_read_flags_or_escape(line))
            return false;
    }

    return false;
}

bool z80_peep::rule_superopt_register_move_sequences(size_t i) {
    if (i + 1 >= lines_.size())
        return false;

    auto &a = lines_[i];
    auto &b = lines_[i + 1];
    if (!a.label.empty() || !b.label.empty())
        return false;
    if (a.mnemonic != "ld" || b.mnemonic != "ld")
        return false;

    std::string a_dst, a_src, b_dst, b_src;
    if (!split_ld(a.operands, a_dst, a_src) ||
        !split_ld(b.operands, b_dst, b_src)) {
        return false;
    }
    a_dst = trim(a_dst);
    a_src = trim(a_src);
    b_dst = trim(b_dst);
    b_src = trim(b_src);

    // ld r1,r2; ld r2,r1  ->  ld r1,r2
    // After the first move, the second writes r2 with the value it already
    // has.  Both instructions preserve flags, so this is an exact cleanup.
    if (a_dst != a_src &&
        is_plain_8bit_reg(a_dst) && is_plain_8bit_reg(a_src) &&
        b_dst == a_src && b_src == a_dst) {
        lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 1));
        return true;
    }

    // ld r,pure; ld r,anything  ->  ld r,anything
    // The first load has no side effects, does not set flags, and is
    // immediately overwritten.
    if (a_dst == b_dst && is_plain_8bit_reg(a_dst) &&
        is_pure_register_load_source(a_src) &&
        is_pure_register_load_source(b_src) &&
        b_src != a_dst) {
        lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i));
        return true;
    }

    // ld l,pure; ld hl,#N -> ld hl,#N (and likewise for DE/BC).
    // A separated pair-immediate fold can expose this form after moving the
    // second byte load earlier.  The pair load overwrites both component
    // bytes, so retaining the preceding pure byte move only costs space and
    // cycles.
    int pair_immediate = 0;
    const bool component_overwritten =
        (b_dst == "hl" && (a_dst == "h" || a_dst == "l")) ||
        (b_dst == "de" && (a_dst == "d" || a_dst == "e")) ||
        (b_dst == "bc" && (a_dst == "b" || a_dst == "c"));
    if (component_overwritten && is_pure_register_load_source(a_src) &&
        parse_immediate_value(b_src, pair_immediate)) {
        lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i));
        return true;
    }

    return false;
}

bool z80_peep::rule_superopt_separated_pair_immediate_load(size_t i) {
    if (i >= lines_.size())
        return false;

    const auto &first = lines_[i];
    if (!first.label.empty() || first.mnemonic != "ld")
        return false;

    struct pair_desc {
        const char *pair;
        char hi;
        char lo;
    };
    static const pair_desc pairs[] = {
        {"bc", 'b', 'c'},
        {"de", 'd', 'e'},
        {"hl", 'h', 'l'},
    };

    std::string first_dst, first_src;
    int first_value = 0;
    if (!split_ld(first.operands, first_dst, first_src) ||
        !parse_immediate_value(first_src, first_value)) {
        return false;
    }
    first_dst = trim(first_dst);
    if (first_dst.size() != 1)
        return false;

    for (const auto &pair : pairs) {
        const bool first_is_hi = first_dst[0] == pair.hi;
        const bool first_is_lo = first_dst[0] == pair.lo;
        if (!first_is_hi && !first_is_lo)
            continue;

        const char delayed = first_is_hi ? pair.lo : pair.hi;
        const char loaded = first_is_hi ? pair.hi : pair.lo;
        const size_t max_scan = std::min(lines_.size(), i + 9);

        for (size_t j = i + 1; j < max_scan; ++j) {
            const auto &line = lines_[j];
            if (!line.label.empty() || is_section_directive(line))
                break;
            if (line.mnemonic.empty())
                continue;
            if (line_is_control_flow_boundary(line))
                break;

            std::string dst, src;
            int second_value = 0;
            if (line.mnemonic == "ld" &&
                split_ld(line.operands, dst, src) &&
                trim(dst) == std::string(1, delayed) &&
                parse_immediate_value(src, second_value)) {
                const int hi_value = first_is_hi ? first_value : second_value;
                const int lo_value = first_is_hi ? second_value : first_value;
                lines_[i] = asm_line::parse(
                    "\tld\t" + std::string(pair.pair) + ", " +
                    imm16_text((u8_value(hi_value) << 8) | u8_value(lo_value)));
                lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(j));
                return true;
            }

            // If the first component is replaced without being read, leave
            // the two loads separate.  The normal dead-load cleanup can then
            // discard the first load; folding it into a pair load would keep
            // an otherwise dead initialization alive.
            if (is_ld_to_reg_without_pair_source(
                    line, std::string(1, loaded), pair.pair,
                    pair.lo, pair.hi)) {
                break;
            }

            if (line_touches_byte_or_pair(line, delayed, pair.pair))
                break;
            // The already-loaded byte may be used freely, but a whole-pair
            // operation would also read/write the delayed byte.
            if (line_touches_byte_or_pair(line, loaded, pair.pair) &&
                operand_has_token(line.operands, pair.pair)) {
                break;
            }
        }
    }

    return false;
}

bool z80_peep::rule_superopt_temp_const_word_load_direct(size_t i) {
    if (i + 1 >= lines_.size())
        return false;
    if (!lines_[i].label.empty() || !lines_[i + 1].label.empty())
        return false;
    if (lines_[i].mnemonic != "ld" || lines_[i + 1].mnemonic != "ld")
        return false;

    struct pair_desc {
        const char *pair;
        const char *lo;
        const char *hi;
    };
    static const pair_desc pairs[] = {
        {"hl", "l", "h"},
        {"de", "e", "d"},
        {"bc", "c", "b"},
    };

    std::string lo_dst;
    std::string lo_src;
    std::string hi_dst;
    std::string hi_src;
    if (!split_ld(lines_[i].operands, lo_dst, lo_src) ||
        !split_ld(lines_[i + 1].operands, hi_dst, hi_src)) {
        return false;
    }
    lo_dst = trim(lo_dst);
    hi_dst = trim(hi_dst);

    int lo_offset = 0;
    int hi_offset = 0;
    if (!parse_ix_ref(trim(lo_src), lo_offset) ||
        !parse_ix_ref(trim(hi_src), hi_offset) ||
        hi_offset != lo_offset + 1) {
        return false;
    }

    const pair_desc *matched_pair = nullptr;
    for (const auto &pair : pairs) {
        if (lo_dst == pair.lo && hi_dst == pair.hi) {
            matched_pair = &pair;
            break;
        }
    }
    if (!matched_pair)
        return false;

    int locals = 0;
    int temp_frame = 0;
    size_t prologue_index = 0;
    if (!current_function_frame(lines_, i, locals, temp_frame, prologue_index))
        return false;
    if (!ix_offset_in_temp_frame(lo_offset, locals, temp_frame) ||
        !ix_offset_in_temp_frame(hi_offset, locals, temp_frame)) {
        return false;
    }

    std::string immediate;
    if (!find_const_temp_word_before_use(lines_, prologue_index, i,
                                         lo_offset, immediate)) {
        return false;
    }

    lines_[i] = asm_line::parse(
        "\tld\t" + std::string(matched_pair->pair) + ", " + immediate);
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 1));
    return true;
}

bool z80_peep::rule_superopt_call_result_store_de_direct(size_t i) {
    if (i + 3 >= lines_.size())
        return false;
    if (!lines_[i].label.empty() || lines_[i].mnemonic != "call")
        return false;

    size_t store_index = i + 2;
    const bool has_bc_copy =
        i + 5 < lines_.size() &&
        lines_[i + 1].label.empty() &&
        is_ex_de_hl(lines_[i + 1]) &&
        lines_[i + 2].label.empty() &&
        lines_[i + 3].label.empty() &&
        lines_[i + 2].mnemonic == "ld" &&
        lines_[i + 3].mnemonic == "ld";

    if (has_bc_copy) {
        std::string dst;
        std::string src;
        if (!split_ld(lines_[i + 2].operands, dst, src) ||
            trim(dst) != "b" || trim(src) != "h") {
            return false;
        }
        if (!split_ld(lines_[i + 3].operands, dst, src) ||
            trim(dst) != "c" || trim(src) != "l") {
            return false;
        }
        store_index = i + 4;
    } else {
        if (lines_[i + 1].label.empty() && is_ex_de_hl(lines_[i + 1])) {
            store_index = i + 2;
        } else {
            return false;
        }
    }

    if (store_index + 1 >= lines_.size())
        return false;
    if (!lines_[store_index].label.empty() ||
        !lines_[store_index + 1].label.empty()) {
        return false;
    }

    std::string lo_dst;
    std::string lo_src;
    std::string hi_dst;
    std::string hi_src;
    if (lines_[store_index].mnemonic != "ld" ||
        !split_ld(lines_[store_index].operands, lo_dst, lo_src) ||
        trim(lo_src) != "l") {
        return false;
    }
    if (lines_[store_index + 1].mnemonic != "ld" ||
        !split_ld(lines_[store_index + 1].operands, hi_dst, hi_src) ||
        trim(hi_src) != "h") {
        return false;
    }

    lo_dst = trim(lo_dst);
    hi_dst = trim(hi_dst);
    if (is_plain_8bit_reg(lo_dst) || is_plain_8bit_reg(hi_dst))
        return false;
    if (operand_mentions_pair_or_bytes(lo_dst, "de", 'e', 'd') ||
        operand_mentions_pair_or_bytes(hi_dst, "de", 'e', 'd') ||
        operand_mentions_pair_or_bytes(lo_dst, "hl", 'l', 'h') ||
        operand_mentions_pair_or_bytes(hi_dst, "hl", 'l', 'h')) {
        return false;
    }

    const size_t after = store_index + 2;
    if (!path_overwrites_hl_before_read(lines_, after))
        return false;
    if (!path_overwrites_pair_before_read(lines_, after, "de", 'e', 'd'))
        return false;

    if (has_bc_copy) {
        if (path_overwrites_pair_before_read(lines_, after, "bc", 'c', 'b')) {
            lines_[i + 1] = asm_line::parse("\tld\t" + lo_dst + ", e");
            lines_[i + 2] = asm_line::parse("\tld\t" + hi_dst + ", d");
            lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 3),
                         lines_.begin() + static_cast<std::ptrdiff_t>(after));
            return true;
        }

        lines_[i + 1] = asm_line::parse("\tld\tb, d");
        lines_[i + 2] = asm_line::parse("\tld\tc, e");
        lines_[i + 3] = asm_line::parse("\tld\t" + lo_dst + ", e");
        lines_[i + 4] = asm_line::parse("\tld\t" + hi_dst + ", d");
        lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 5),
                     lines_.begin() + static_cast<std::ptrdiff_t>(after));
        return true;
    }

    lines_[i + 1] = asm_line::parse("\tld\t" + lo_dst + ", e");
    lines_[i + 2] = asm_line::parse("\tld\t" + hi_dst + ", d");
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 3),
                 lines_.begin() + static_cast<std::ptrdiff_t>(after));
    return true;
}

bool z80_peep::rule_superopt_dead_bc_store_copy(size_t i) {
    if (i + 3 >= lines_.size())
        return false;
    for (size_t k = i; k <= i + 3; ++k) {
        if (!lines_[k].label.empty())
            return false;
    }

    std::string dst;
    std::string src;
    if (lines_[i].mnemonic != "ld" ||
        !split_ld(lines_[i].operands, dst, src) ||
        trim(dst) != "b" || trim(src) != "h") {
        return false;
    }
    if (lines_[i + 1].mnemonic != "ld" ||
        !split_ld(lines_[i + 1].operands, dst, src) ||
        trim(dst) != "c" || trim(src) != "l") {
        return false;
    }

    std::string lo_dst;
    std::string lo_src;
    std::string hi_dst;
    std::string hi_src;
    if (lines_[i + 2].mnemonic != "ld" ||
        !split_ld(lines_[i + 2].operands, lo_dst, lo_src) ||
        trim(lo_src) != "l") {
        return false;
    }
    if (lines_[i + 3].mnemonic != "ld" ||
        !split_ld(lines_[i + 3].operands, hi_dst, hi_src) ||
        trim(hi_src) != "h") {
        return false;
    }

    lo_dst = trim(lo_dst);
    hi_dst = trim(hi_dst);
    if (is_plain_8bit_reg(lo_dst) || is_plain_8bit_reg(hi_dst))
        return false;
    if (operand_mentions_pair_or_bytes(lo_dst, "bc", 'c', 'b') ||
        operand_mentions_pair_or_bytes(hi_dst, "bc", 'c', 'b') ||
        operand_mentions_pair_or_bytes(lo_dst, "hl", 'l', 'h') ||
        operand_mentions_pair_or_bytes(hi_dst, "hl", 'l', 'h')) {
        return false;
    }
    if (!path_overwrites_pair_before_read(lines_, i + 4, "bc", 'c', 'b'))
        return false;

    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 2));
    return true;
}

bool z80_peep::rule_superopt_lowbyte_zero_extend_to_de(size_t i) {
    if (i + 6 >= lines_.size())
        return false;
    for (size_t k = i; k <= i + 6; ++k) {
        if (!lines_[k].label.empty())
            return false;
    }

    auto is_ld_exact = [&](size_t idx,
                           const std::string &want_dst,
                           const std::string &want_src) {
        if (lines_[idx].mnemonic != "ld")
            return false;
        std::string dst;
        std::string src;
        return split_ld(lines_[idx].operands, dst, src) &&
               trim(dst) == want_dst && trim(src) == want_src;
    };

    if (!is_ld_exact(i, "a", "h"))
        return false;
    if (lines_[i + 1].mnemonic != "and" ||
        !immediate_is(lines_[i + 1].operands, 0)) {
        return false;
    }
    if (!is_ld_exact(i + 2, "h", "a") ||
        !is_ld_exact(i + 3, "b", "h") ||
        !is_ld_exact(i + 4, "c", "l") ||
        !is_ld_exact(i + 5, "d", "b") ||
        !is_ld_exact(i + 6, "e", "c")) {
        return false;
    }

    if (!path_overwrites_hl_before_read(lines_, i + 7))
        return false;
    if (!path_overwrites_bc_before_read_or_call(lines_, i + 7))
        return false;

    lines_[i] = asm_line::parse("\tand\t#0");
    lines_[i + 1] = asm_line::parse("\tld\td, a");
    lines_[i + 2] = asm_line::parse("\tld\te, l");
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 3),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 7));
    return true;
}

bool z80_peep::rule_superopt_ix_word_inc_direct(size_t i) {
    if (i + 4 >= lines_.size())
        return false;

    std::string dst, src;
    int lo_load = 0;
    int hi_load = 0;
    int lo_store = 0;
    int hi_store = 0;

    if (lines_[i].mnemonic != "ld" ||
        !split_ld(lines_[i].operands, dst, src) ||
        trim(dst) != "l" ||
        !parse_ix_ref(trim(src), lo_load)) {
        return false;
    }
    if (lines_[i + 1].mnemonic != "ld" ||
        !split_ld(lines_[i + 1].operands, dst, src) ||
        trim(dst) != "h" ||
        !parse_ix_ref(trim(src), hi_load)) {
        return false;
    }
    if (lines_[i + 2].mnemonic != "inc" ||
        trim(lines_[i + 2].operands) != "hl") {
        return false;
    }

    auto parse_pair_store = [&](size_t pos, int &lo, int &hi) {
        if (pos + 1 >= lines_.size() ||
            lines_[pos].mnemonic != "ld" ||
            lines_[pos + 1].mnemonic != "ld") {
            return false;
        }
        std::string d0, s0, d1, s1;
        return split_ld(lines_[pos].operands, d0, s0) &&
               split_ld(lines_[pos + 1].operands, d1, s1) &&
               parse_ix_ref(trim(d0), lo) && trim(s0) == "l" &&
               parse_ix_ref(trim(d1), hi) && trim(s1) == "h" &&
               hi == lo + 1;
    };

    size_t cursor = i + 3;
    int temp_lo = 0;
    int temp_hi = 0;
    const bool has_temp_copy =
        parse_pair_store(cursor, temp_lo, temp_hi) &&
        (temp_lo != lo_load || temp_hi != hi_load);
    if (has_temp_copy)
        cursor += 2;

    const bool has_bc_copy =
        cursor + 1 < lines_.size() &&
        lines_[cursor].mnemonic == "ld" &&
        trim(lines_[cursor].operands) == "b, h" &&
        lines_[cursor + 1].mnemonic == "ld" &&
        trim(lines_[cursor + 1].operands) == "c, l";
    if (has_bc_copy)
        cursor += 2;

    const size_t store_index = cursor;
    const size_t end_index = store_index + 2;
    if (end_index > lines_.size())
        return false;
    for (size_t k = i; k < end_index; ++k) {
        if (!lines_[k].label.empty())
            return false;
    }

    if (lines_[store_index].mnemonic != "ld" ||
        !split_ld(lines_[store_index].operands, dst, src) ||
        !parse_ix_ref(trim(dst), lo_store) ||
        trim(src) != "l") {
        return false;
    }
    if (lines_[store_index + 1].mnemonic != "ld" ||
        !split_ld(lines_[store_index + 1].operands, dst, src) ||
        !parse_ix_ref(trim(dst), hi_store) ||
        trim(src) != "h") {
        return false;
    }
    if (hi_load != lo_load + 1 ||
        lo_store != lo_load ||
        hi_store != hi_load) {
        return false;
    }
    if (has_temp_copy) {
        int locals = 0;
        int temp_frame = 0;
        size_t prologue_index = 0;
        if (!current_function_frame(lines_, i, locals, temp_frame,
                                    prologue_index) ||
            !ix_offset_in_temp_frame(temp_lo, locals, temp_frame) ||
            !ix_offset_in_temp_frame(temp_hi, locals, temp_frame)) {
            return false;
        }
    }
    if (has_bc_copy && !bc_dead_before_read_or_ret(lines_, end_index))
        return false;
    if (!flags_overwritten_before_read_or_escape(lines_, end_index))
        return false;
    if (!hl_dead_before_read_or_modern_return(lines_, end_index))
        return false;

    std::string done;
    for (int n = 0;; ++n) {
        done = "__xopt_inc16_" + std::to_string(n);
        if (find_label_index(lines_, done) == lines_.size())
            break;
    }

    const std::string lo_ref = std::to_string(lo_load) + "(ix)";
    const std::string hi_ref = std::to_string(hi_load) + "(ix)";
    std::vector<asm_line> repl;
    repl.push_back(asm_line::parse("\tinc\t" + lo_ref));
    repl.push_back(asm_line::parse("\tjr\tnz, " + done));
    repl.push_back(asm_line::parse("\tinc\t" + hi_ref));
    repl.push_back(asm_line::parse(done + ":"));

    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                 lines_.begin() + static_cast<std::ptrdiff_t>(end_index));
    lines_.insert(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                  repl.begin(), repl.end());
    return true;
}

bool z80_peep::rule_superopt_ix_word_add1_direct(size_t i) {
    if (i + 5 >= lines_.size())
        return false;
    const bool has_bc_copy =
        i + 7 < lines_.size() &&
        lines_[i + 4].mnemonic == "ld" && trim(lines_[i + 4].operands) == "b, h" &&
        lines_[i + 5].mnemonic == "ld" && trim(lines_[i + 5].operands) == "c, l";
    const size_t store_index = has_bc_copy ? i + 6 : i + 4;
    const size_t end_index = has_bc_copy ? i + 8 : i + 6;
    if (end_index > lines_.size())
        return false;
    for (size_t k = i; k < end_index; ++k) {
        if (!lines_[k].label.empty())
            return false;
    }

    std::string dst, src;
    int lo_load = 0;
    int hi_load = 0;
    int lo_store = 0;
    int hi_store = 0;

    if (lines_[i].mnemonic != "ld" ||
        !split_ld(lines_[i].operands, dst, src) ||
        trim(dst) != "l" ||
        !parse_ix_ref(trim(src), lo_load)) {
        return false;
    }
    if (lines_[i + 1].mnemonic != "ld" ||
        !split_ld(lines_[i + 1].operands, dst, src) ||
        trim(dst) != "h" ||
        !parse_ix_ref(trim(src), hi_load)) {
        return false;
    }
    if (lines_[i + 2].mnemonic != "ld" ||
        !split_ld(lines_[i + 2].operands, dst, src) ||
        trim(dst) != "de" || !immediate_is(src, 1)) {
        return false;
    }
    if (lines_[i + 3].mnemonic != "add" ||
        !split_ld(lines_[i + 3].operands, dst, src) ||
        trim(dst) != "hl" || trim(src) != "de") {
        return false;
    }
    if (lines_[store_index].mnemonic != "ld" ||
        !split_ld(lines_[store_index].operands, dst, src) ||
        !parse_ix_ref(trim(dst), lo_store) ||
        trim(src) != "l") {
        return false;
    }
    if (lines_[store_index + 1].mnemonic != "ld" ||
        !split_ld(lines_[store_index + 1].operands, dst, src) ||
        !parse_ix_ref(trim(dst), hi_store) ||
        trim(src) != "h") {
        return false;
    }
    if (hi_load != lo_load + 1 ||
        lo_store != lo_load ||
        hi_store != hi_load) {
        return false;
    }
    if (has_bc_copy && !bc_dead_before_read_or_ret(lines_, end_index))
        return false;
    if (!flags_overwritten_before_read_or_escape(lines_, end_index))
        return false;
    if (!hl_dead_before_read_or_modern_return(lines_, end_index))
        return false;
    if (!path_overwrites_pair_before_read(lines_, end_index, "de", 'e', 'd'))
        return false;

    std::string done;
    for (int n = 0;; ++n) {
        done = "__xopt_inc16_" + std::to_string(n);
        if (find_label_index(lines_, done) == lines_.size())
            break;
    }

    const std::string lo_ref = std::to_string(lo_load) + "(ix)";
    const std::string hi_ref = std::to_string(hi_load) + "(ix)";
    std::vector<asm_line> repl;
    repl.push_back(asm_line::parse("\tinc\t" + lo_ref));
    repl.push_back(asm_line::parse("\tjr\tnz, " + done));
    repl.push_back(asm_line::parse("\tinc\t" + hi_ref));
    repl.push_back(asm_line::parse(done + ":"));

    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                 lines_.begin() + static_cast<std::ptrdiff_t>(end_index));
    lines_.insert(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                  repl.begin(), repl.end());
    return true;
}

bool z80_peep::rule_add_hl_de_one_to_inc(size_t i) {
    if (i + 1 >= lines_.size())
        return false;
    if (!lines_[i].label.empty() || !lines_[i + 1].label.empty())
        return false;

    std::string dst, src;
    int value = 0;
    if (lines_[i].mnemonic != "ld" ||
        !split_ld(lines_[i].operands, dst, src) ||
        !parse_immediate_value(src, value)) {
        return false;
    }
    const std::string pair = trim(dst);
    if (pair != "bc" && pair != "de")
        return false;
    if (value < -3 || value > 3 || value == 0)
        return false;

    if (lines_[i + 1].mnemonic != "add" ||
        !split_ld(lines_[i + 1].operands, dst, src) ||
        trim(dst) != "hl" || trim(src) != pair) {
        return false;
    }
    if (!flags_overwritten_before_read_or_escape(lines_, i + 2))
        return false;

    const char lo = pair == "bc" ? 'c' : 'e';
    const char hi = pair == "bc" ? 'b' : 'd';
    if (!pair_value_dead_or_call_or_modern_return_before_read(
            lines_, i + 2, pair, lo, hi)) {
        return false;
    }

    const int count = value > 0 ? value : -value;
    std::vector<asm_line> repl;
    repl.reserve(static_cast<size_t>(count));
    const char *op = value > 0 ? "inc" : "dec";
    for (int n = 0; n < count; ++n)
        repl.push_back(asm_line::parse(std::string("\t") + op + "\thl"));

    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 2));
    lines_.insert(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                  repl.begin(), repl.end());
    return true;
}

bool z80_peep::rule_superopt_ix_byte_inc_direct(size_t i) {
    if (i + 2 >= lines_.size())
        return false;
    for (size_t k = 0; k < 3; ++k) {
        if (!lines_[i + k].label.empty())
            return false;
    }

    std::string dst, src;
    int load_off = 0;
    int store_off = 0;
    int value = 0;

    if (lines_[i].mnemonic != "ld" ||
        !split_ld(lines_[i].operands, dst, src) ||
        trim(dst) != "a" ||
        !parse_ix_ref(trim(src), load_off)) {
        return false;
    }
    if (lines_[i + 1].mnemonic != "add" ||
        !parse_accumulator_immediate_alu(lines_[i + 1], "add", value) ||
        u8_value(value) != 1) {
        return false;
    }
    if (lines_[i + 2].mnemonic != "ld" ||
        !split_ld(lines_[i + 2].operands, dst, src) ||
        !parse_ix_ref(trim(dst), store_off) ||
        trim(src) != "a" ||
        store_off != load_off) {
        return false;
    }

    if (!a_overwritten_or_call_before_read(lines_, i + 3) ||
        !flags_overwritten_or_call_before_read(lines_, i + 3)) {
        return false;
    }

    lines_[i] = asm_line::parse("\tinc\t" + std::to_string(load_off) + "(ix)");
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 1),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 3));
    return true;
}

bool z80_peep::rule_ix_byte_inc_test_direct(size_t i) {
    if (i + 4 >= lines_.size())
        return false;
    for (size_t k = 0; k < 5; ++k) {
        if (!lines_[i + k].label.empty())
            return false;
    }

    std::string dst;
    std::string src;
    int load_off = 0;
    int store_off = 0;
    int value = 0;
    if (lines_[i].mnemonic != "ld" ||
        !split_ld(lines_[i].operands, dst, src) ||
        trim(dst) != "a" ||
        !parse_ix_ref(trim(src), load_off)) {
        return false;
    }
    if (lines_[i + 1].mnemonic != "add" ||
        !parse_accumulator_immediate_alu(lines_[i + 1], "add", value) ||
        u8_value(value) != 1) {
        return false;
    }
    if (lines_[i + 2].mnemonic != "ld" ||
        !split_ld(lines_[i + 2].operands, dst, src) ||
        !parse_ix_ref(trim(dst), store_off) ||
        trim(src) != "a" ||
        store_off != load_off) {
        return false;
    }
    if (!is_or_a_self(lines_[i + 3]))
        return false;

    std::string cc;
    std::string target;
    if (!split_conditional_branch_target(lines_[i + 4], cc, target) ||
        (cc != "z" && cc != "nz")) {
        return false;
    }

    const std::string ref = std::to_string(load_off) + "(ix)";
    lines_[i] = asm_line::parse("\tinc\t" + ref);
    lines_[i + 1] = asm_line::parse("\tld\ta, " + ref);
    lines_[i + 2] = lines_[i + 3];
    lines_[i + 3] = lines_[i + 4];
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 4));
    return true;
}

bool z80_peep::rule_reg_byte_inc_dec_direct(size_t i) {
    if (i + 2 >= lines_.size())
        return false;
    for (size_t k = 0; k < 3; ++k) {
        if (!lines_[i + k].label.empty())
            return false;
    }

    std::string dst;
    std::string src;
    if (lines_[i].mnemonic != "ld" ||
        !split_ld(lines_[i].operands, dst, src) ||
        trim(dst) != "a") {
        return false;
    }
    const std::string reg = trim(src);
    if (!is_plain_8bit_reg(reg) || reg == "a")
        return false;

    const bool is_inc = lines_[i + 1].mnemonic == "add";
    const bool is_dec = lines_[i + 1].mnemonic == "sub";
    if (!is_inc && !is_dec)
        return false;
    if (is_inc) {
        std::string alu_dst;
        std::string alu_src;
        if (!split_ld(lines_[i + 1].operands, alu_dst, alu_src) ||
            trim(alu_dst) != "a" || !immediate_is(alu_src, 1)) {
            return false;
        }
    } else if (!immediate_is(lines_[i + 1].operands, 1)) {
        return false;
    }

    if (lines_[i + 2].mnemonic != "ld" ||
        !split_ld(lines_[i + 2].operands, dst, src) ||
        trim(dst) != reg || trim(src) != "a") {
        return false;
    }

    if (!a_overwritten_or_call_before_read(lines_, i + 3) ||
        !flags_overwritten_or_call_before_read(lines_, i + 3)) {
        return false;
    }

    lines_[i] = asm_line::parse(
        std::string("\t") + (is_inc ? "inc\t" : "dec\t") + reg);
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 1),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 3));
    return true;
}

bool z80_peep::rule_divuint_remainder_bc_restore_elide(size_t i) {
    if (i + 8 >= lines_.size())
        return false;
    for (size_t k = 1; k <= 8; ++k) {
        if (!lines_[i + k].label.empty())
            return false;
    }
    if (lines_[i].mnemonic != "call" ||
        trim(lines_[i].operands) != "__divuint") {
        return false;
    }

    auto is_ld = [&](size_t idx, const char *dst_expected,
                     const char *src_expected) {
        if (lines_[idx].mnemonic != "ld")
            return false;
        std::string dst;
        std::string src;
        if (!split_ld(lines_[idx].operands, dst, src))
            return false;
        return trim(dst) == dst_expected && trim(src) == src_expected;
    };

    if (!is_ld(i + 1, "b", "h") ||
        !is_ld(i + 2, "c", "l") ||
        !is_ld(i + 5, "h", "b") ||
        !is_ld(i + 6, "l", "c") ||
        !is_ld(i + 7, "a", "l")) {
        return false;
    }
    std::string final_dst;
    std::string final_src;
    if (lines_[i + 8].mnemonic != "ld" ||
        !split_ld(lines_[i + 8].operands, final_dst, final_src)) {
        return false;
    }
    final_dst = trim(final_dst);
    final_src = trim(final_src);
    if ((final_dst != "b" && final_dst != "c") || final_src != "a")
        return false;

    std::string dst3, src3, dst4, src4;
    if (lines_[i + 3].mnemonic != "ld" ||
        lines_[i + 4].mnemonic != "ld" ||
        !split_ld(lines_[i + 3].operands, dst3, src3) ||
        !split_ld(lines_[i + 4].operands, dst4, src4) ||
        trim(src3) != "e" || trim(src4) != "d") {
        return false;
    }
    dst3 = trim(dst3);
    dst4 = trim(dst4);
    if (operand_has_token(dst3, "a") || operand_has_token(dst3, "b") ||
        operand_has_token(dst3, "c") || operand_has_token(dst3, "h") ||
        operand_has_token(dst3, "l") || operand_has_token(dst3, "bc") ||
        operand_has_token(dst3, "hl") ||
        operand_has_token(dst4, "a") || operand_has_token(dst4, "b") ||
        operand_has_token(dst4, "c") || operand_has_token(dst4, "h") ||
        operand_has_token(dst4, "l") || operand_has_token(dst4, "bc") ||
        operand_has_token(dst4, "hl")) {
        return false;
    }

    if (final_dst == "c") {
        lines_[i + 2] = lines_[i + 3];
        lines_[i + 3] = lines_[i + 4];
        lines_[i + 4] = lines_[i + 7];
        lines_[i + 5] = lines_[i + 8];
        lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 6),
                     lines_.begin() + static_cast<std::ptrdiff_t>(i + 9));
        return true;
    }

    const bool c_dead_after = c_overwritten_or_call_before_read(lines_, i + 9);
    lines_[i + 1] = c_dead_after ? lines_[i + 3] : lines_[i + 2];
    lines_[i + 2] = c_dead_after ? lines_[i + 4] : lines_[i + 3];
    lines_[i + 3] = c_dead_after ? lines_[i + 7] : lines_[i + 4];
    lines_[i + 4] = c_dead_after ? lines_[i + 8] : lines_[i + 7];
    if (!c_dead_after)
        lines_[i + 5] = lines_[i + 8];
    lines_.erase(lines_.begin() +
                     static_cast<std::ptrdiff_t>(i + (c_dead_after ? 5 : 6)),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 9));
    return true;
}

bool z80_peep::rule_ix_byte_dec_direct(size_t i) {
    if (i + 2 >= lines_.size())
        return false;
    for (size_t k = 0; k < 3; ++k) {
        if (!lines_[i + k].label.empty())
            return false;
    }

    std::string dst;
    std::string src;
    int load_off = 0;
    int store_off = 0;

    if (lines_[i].mnemonic != "ld" ||
        !split_ld(lines_[i].operands, dst, src) ||
        trim(dst) != "a" ||
        !parse_ix_ref(trim(src), load_off)) {
        return false;
    }
    if (lines_[i + 1].mnemonic != "sub" ||
        !immediate_is(lines_[i + 1].operands, 1)) {
        return false;
    }
    if (lines_[i + 2].mnemonic != "ld" ||
        !split_ld(lines_[i + 2].operands, dst, src) ||
        !parse_ix_ref(trim(dst), store_off) ||
        trim(src) != "a" ||
        store_off != load_off) {
        return false;
    }

    if (!a_overwritten_or_call_before_read(lines_, i + 3) ||
        !flags_overwritten_or_call_before_read(lines_, i + 3)) {
        return false;
    }

    lines_[i] = asm_line::parse("\tdec\t" + std::to_string(load_off) + "(ix)");
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 1),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 3));
    return true;
}

bool z80_peep::rule_reg_byte_add_shuttle_elide(size_t i) {
    if (i + 5 >= lines_.size())
        return false;
    for (size_t k = 0; k < 6; ++k) {
        if (!lines_[i + k].label.empty())
            return false;
    }

    std::string d0, s0, d1, s1, d2, s2, d3, s3, d4, s4, d5, s5;
    if (lines_[i].mnemonic != "ld" ||
        !split_ld(lines_[i].operands, d0, s0) ||
        lines_[i + 1].mnemonic != "ld" ||
        !split_ld(lines_[i + 1].operands, d1, s1) ||
        lines_[i + 2].mnemonic != "ld" ||
        !split_ld(lines_[i + 2].operands, d2, s2) ||
        lines_[i + 3].mnemonic != "ld" ||
        !split_ld(lines_[i + 3].operands, d3, s3) ||
        lines_[i + 4].mnemonic != "add" ||
        !split_ld(lines_[i + 4].operands, d4, s4) ||
        lines_[i + 5].mnemonic != "ld" ||
        !split_ld(lines_[i + 5].operands, d5, s5)) {
        return false;
    }

    d0 = trim(d0); s0 = trim(s0);
    d1 = trim(d1); s1 = trim(s1);
    d2 = trim(d2); s2 = trim(s2);
    d3 = trim(d3); s3 = trim(s3);
    d4 = trim(d4); s4 = trim(s4);
    d5 = trim(d5); s5 = trim(s5);

    if (d0 != "a" || !is_plain_8bit_reg(s0) || s0 == "a" ||
        d1 != "e" || s1 != "a" ||
        d2 != "d" ||
        d3 != "a" || s3 != "e" ||
        d4 != "a" || s4 != "d" ||
        d5 != s0 || s5 != "a") {
        return false;
    }
    if (operand_has_token(s2, "a") || operand_has_token(s2, "d") ||
        operand_has_token(s2, "e")) {
        return false;
    }

    int ignored = 0;
    const bool supported_src =
        is_plain_8bit_reg(s2) || is_immediate_operand(s2) ||
        is_numeric_literal(s2) || parse_ixiy_ref(s2, ignored) ||
        s2 == "(hl)";
    if (!supported_src)
        return false;

    if (!pair_value_dead_or_call_or_modern_return_before_read(
            lines_, i + 6, "de", 'e', 'd')) {
        return false;
    }

    lines_[i + 1] = asm_line::parse("\tadd\ta, " + s2);
    lines_[i + 2] = asm_line::parse("\tld\t" + s0 + ", a");
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 3),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 6));
    return true;
}

bool z80_peep::rule_byte_addsub_shuttle_elide(size_t i) {
    if (i + 4 >= lines_.size())
        return false;
    for (size_t k = 0; k < 5; ++k) {
        if (!lines_[i + k].label.empty())
            return false;
    }

    std::string d0, s0, d1, s1, d2, s2, d3, s3, d4, s4;
    if (lines_[i].mnemonic != "ld" ||
        !split_ld(lines_[i].operands, d0, s0) ||
        lines_[i + 1].mnemonic != "ld" ||
        !split_ld(lines_[i + 1].operands, d1, s1) ||
        lines_[i + 2].mnemonic != "ld" ||
        !split_ld(lines_[i + 2].operands, d2, s2) ||
        !split_ld(lines_[i + 3].operands, d3, s3) ||
        lines_[i + 4].mnemonic != "ld" ||
        !split_ld(lines_[i + 4].operands, d4, s4)) {
        return false;
    }

    d0 = trim(d0); s0 = trim(s0);
    d1 = trim(d1); s1 = trim(s1);
    d2 = trim(d2); s2 = trim(s2);
    d3 = trim(d3); s3 = trim(s3);
    d4 = trim(d4); s4 = trim(s4);

    if (d0 != "e" || d1 != "d" ||
        d2 != "a" || s2 != "e" ||
        d3 != "a" || s3 != "d" ||
        d4 != s0 || s4 != "a") {
        return false;
    }
    if (lines_[i + 3].mnemonic != "add" &&
        lines_[i + 3].mnemonic != "sub") {
        return false;
    }
    if (operand_has_token(s1, "d") || operand_has_token(s1, "e"))
        return false;

    int ignored = 0;
    const bool supported_dst =
        is_plain_8bit_reg(s0) || parse_ixiy_ref(s0, ignored) ||
        s0 == "(hl)";
    const bool supported_rhs =
        is_plain_8bit_reg(s1) || is_immediate_operand(s1) ||
        is_numeric_literal(s1) || parse_ixiy_ref(s1, ignored) ||
        s1 == "(hl)";
    if (!supported_dst || !supported_rhs)
        return false;

    if (!pair_value_dead_or_call_or_modern_return_before_read(
            lines_, i + 5, "de", 'e', 'd')) {
        return false;
    }

    lines_[i] = asm_line::parse("\tld\ta, " + s0);
    lines_[i + 1] = asm_line::parse(
        "\t" + lines_[i + 3].mnemonic + "\ta, " + s1);
    lines_[i + 2] = asm_line::parse("\tld\t" + s0 + ", a");
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 3),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 5));
    return true;
}

bool z80_peep::rule_byte_addsub_a_to_d_shuttle_elide(size_t i) {
    if (i + 5 >= lines_.size())
        return false;
    for (size_t k = 0; k < 6; ++k) {
        if (!lines_[i + k].label.empty())
            return false;
    }

    std::string d0, s0, d1, s1, d2, s2, d3, s3, d4, s4, d5, s5;
    if (lines_[i].mnemonic != "ld" ||
        !split_ld(lines_[i].operands, d0, s0) ||
        lines_[i + 1].mnemonic != "ld" ||
        !split_ld(lines_[i + 1].operands, d1, s1) ||
        lines_[i + 2].mnemonic != "ld" ||
        !split_ld(lines_[i + 2].operands, d2, s2) ||
        lines_[i + 3].mnemonic != "ld" ||
        !split_ld(lines_[i + 3].operands, d3, s3) ||
        !split_ld(lines_[i + 4].operands, d4, s4) ||
        lines_[i + 5].mnemonic != "ld" ||
        !split_ld(lines_[i + 5].operands, d5, s5)) {
        return false;
    }

    d0 = trim(d0); s0 = trim(s0);
    d1 = trim(d1); s1 = trim(s1);
    d2 = trim(d2); s2 = trim(s2);
    d3 = trim(d3); s3 = trim(s3);
    d4 = trim(d4); s4 = trim(s4);
    d5 = trim(d5); s5 = trim(s5);

    if (d0 != "e" ||
        d1 != "a" ||
        d2 != "d" || s2 != "a" ||
        d3 != "a" || s3 != "e" ||
        d4 != "a" || s4 != "d" ||
        d5 != s0 || s5 != "a") {
        return false;
    }
    if (lines_[i + 4].mnemonic != "add" &&
        lines_[i + 4].mnemonic != "sub") {
        return false;
    }
    if (operand_has_token(s0, "a") || operand_has_token(s0, "d") ||
        operand_has_token(s0, "e") || operand_has_token(s1, "a") ||
        operand_has_token(s1, "d") || operand_has_token(s1, "e")) {
        return false;
    }

    int ignored = 0;
    const bool supported_dst =
        is_plain_8bit_reg(s0) || parse_ixiy_ref(s0, ignored) ||
        s0 == "(hl)";
    const bool supported_rhs =
        (is_plain_8bit_reg(s1) && s1 != "a") ||
        is_immediate_operand(s1) || is_numeric_literal(s1) ||
        parse_ixiy_ref(s1, ignored) || s1 == "(hl)";
    if (!supported_dst || !supported_rhs)
        return false;
    if (s0 == "(hl)" && s1 == "(hl)")
        return false;

    if (!pair_value_dead_or_call_or_modern_return_before_read(
            lines_, i + 6, "de", 'e', 'd')) {
        return false;
    }

    lines_[i] = asm_line::parse("\tld\ta, " + s0);
    lines_[i + 1] = asm_line::parse(
        "\t" + lines_[i + 4].mnemonic + "\ta, " + s1);
    lines_[i + 2] = asm_line::parse("\tld\t" + s0 + ", a");
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 3),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 6));
    return true;
}

bool z80_peep::rule_dead_d_in_byte_mul10(size_t i) {
    if (i + 4 >= lines_.size())
        return false;
    for (size_t k = 0; k < 5; ++k) {
        if (!lines_[i + k].label.empty())
            return false;
    }
    if (lines_[i].mnemonic != "ld" || trim(lines_[i].operands) != "e, a" ||
        lines_[i + 1].mnemonic != "add" ||
        trim(lines_[i + 1].operands) != "a, a" ||
        lines_[i + 2].mnemonic != "add" ||
        trim(lines_[i + 2].operands) != "a, a" ||
        lines_[i + 3].mnemonic != "ld" ||
        trim(lines_[i + 3].operands) != "d, a" ||
        lines_[i + 4].mnemonic != "add" ||
        trim(lines_[i + 4].operands) != "a, e") {
        return false;
    }
    if (!pair_value_dead_or_call_or_modern_return_before_read(
            lines_, i + 5, "de", 'e', 'd')) {
        return false;
    }
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 3));
    return true;
}

bool z80_peep::rule_ix_byte_or_mask_set_direct(size_t i) {
    if (i + 2 >= lines_.size())
        return false;
    for (size_t k = 0; k < 3; ++k) {
        if (!lines_[i + k].label.empty())
            return false;
    }

    std::string dst;
    std::string src;
    int load_off = 0;
    int store_off = 0;
    int mask = 0;
    if (lines_[i].mnemonic != "ld" ||
        !split_ld(lines_[i].operands, dst, src) ||
        trim(dst) != "a" ||
        !parse_ix_ref(trim(src), load_off)) {
        return false;
    }
    if (lines_[i + 1].mnemonic != "or")
        return false;
    const std::string ops = trim(lines_[i + 1].operands);
    if (!parse_immediate_value(ops, mask)) {
        if (!split_ld(ops, dst, src) ||
            trim(dst) != "a" ||
            !parse_immediate_value(src, mask)) {
            return false;
        }
    }
    mask = u8_value(mask);
    if (mask == 0 || (mask & (mask - 1)) != 0)
        return false;

    if (lines_[i + 2].mnemonic != "ld" ||
        !split_ld(lines_[i + 2].operands, dst, src) ||
        !parse_ix_ref(trim(dst), store_off) ||
        trim(src) != "a" ||
        store_off != load_off) {
        return false;
    }
    if (!a_overwritten_before_read(lines_, i + 3) ||
        !flags_overwritten_before_read_or_escape(lines_, i + 3)) {
        return false;
    }

    int bit = 0;
    while (((mask >> bit) & 1) == 0)
        ++bit;
    lines_[i] = asm_line::parse(
        "\tset\t" + std::to_string(bit) + ", " +
        std::to_string(load_off) + "(ix)");
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 1),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 3));
    return true;
}

bool z80_peep::rule_ix_byte_mul10_spill_elide(size_t i) {
    if (i + 14 >= lines_.size())
        return false;
    for (size_t k = 0; k < 15; ++k) {
        if (!lines_[i + k].label.empty())
            return false;
    }

    auto is_add_a_a = [](const asm_line &line) {
        if (line.mnemonic != "add")
            return false;
        const std::string ops = trim(line.operands);
        return ops == "a, a" || ops == "a,a";
    };
    auto match_ld = [&](size_t idx, const std::string &want_dst,
                        const std::string &want_src) {
        std::string dst;
        std::string src;
        return lines_[idx].mnemonic == "ld" &&
               split_ld(lines_[idx].operands, dst, src) &&
               trim(dst) == want_dst &&
               trim(src) == want_src;
    };

    if (i + 17 < lines_.size()) {
        bool labels_ok = true;
        for (size_t k = 15; k < 18; ++k) {
            if (!lines_[i + k].label.empty()) {
                labels_ok = false;
                break;
            }
        }
        std::string dst2;
        std::string src2;
        int value2_off = 0;
        int digit_off = 0;
        int digit_tmp_off = 0;
        if (labels_ok &&
            lines_[i].mnemonic == "ld" &&
            split_ld(lines_[i].operands, dst2, src2) &&
            trim(dst2) == "a" &&
            parse_ix_ref(trim(src2), value2_off)) {
            const std::string value2_ref = std::to_string(value2_off) + "(ix)";
            if (is_add_a_a(lines_[i + 1]) &&
                match_ld(i + 2, "e", "a") &&
                is_add_a_a(lines_[i + 3]) &&
                is_add_a_a(lines_[i + 4]) &&
                match_ld(i + 5, "d", "a") &&
                lines_[i + 6].mnemonic == "add" &&
                (trim(lines_[i + 6].operands) == "a, e" ||
                 trim(lines_[i + 6].operands) == "a,e") &&
                match_ld(i + 7, value2_ref, "a") &&
                lines_[i + 8].mnemonic == "ld" &&
                split_ld(lines_[i + 8].operands, dst2, src2) &&
                trim(dst2) == "a" &&
                parse_ix_ref(trim(src2), digit_off) &&
                lines_[i + 9].mnemonic == "sub" &&
                immediate_is(trim(lines_[i + 9].operands), 48) &&
                lines_[i + 10].mnemonic == "ld" &&
                split_ld(lines_[i + 10].operands, dst2, src2) &&
                parse_ix_ref(trim(dst2), digit_tmp_off) &&
                trim(src2) == "a") {
                const std::string digit_ref = std::to_string(digit_off) + "(ix)";
                const std::string digit_tmp_ref =
                    std::to_string(digit_tmp_off) + "(ix)";
                if (digit_tmp_off != value2_off &&
                    digit_tmp_off != digit_off &&
                    match_ld(i + 11, "a", value2_ref) &&
                    match_ld(i + 12, "e", "a") &&
                    match_ld(i + 13, "a", digit_tmp_ref) &&
                    match_ld(i + 14, "d", "a") &&
                    match_ld(i + 15, "a", "e") &&
                    lines_[i + 16].mnemonic == "add" &&
                    (trim(lines_[i + 16].operands) == "a, d" ||
                     trim(lines_[i + 16].operands) == "a,d") &&
                    match_ld(i + 17, value2_ref, "a")) {
                    bool tmp_dead = true;
                    for (size_t j = i + 18; j < lines_.size(); ++j) {
                        if (lines_[j].operands.find(digit_tmp_ref) !=
                            std::string::npos) {
                            tmp_dead = false;
                            break;
                        }
                    }
                    if (tmp_dead) {
                        std::vector<asm_line> repl = {
                            asm_line::parse("\tld\ta, " + value2_ref),
                            asm_line::parse("\tadd\ta, a"),
                            asm_line::parse("\tld\te, a"),
                            asm_line::parse("\tadd\ta, a"),
                            asm_line::parse("\tadd\ta, a"),
                            asm_line::parse("\tadd\ta, e"),
                            asm_line::parse("\tld\te, a"),
                            asm_line::parse("\tld\ta, " + digit_ref),
                            asm_line::parse("\tsub\t#48"),
                            asm_line::parse("\tld\td, a"),
                            asm_line::parse("\tadd\ta, e"),
                            asm_line::parse("\tld\t" + value2_ref + ", a"),
                        };
                        lines_.erase(lines_.begin() +
                                         static_cast<std::ptrdiff_t>(i),
                                     lines_.begin() +
                                         static_cast<std::ptrdiff_t>(i + 18));
                        lines_.insert(lines_.begin() +
                                          static_cast<std::ptrdiff_t>(i),
                                      repl.begin(), repl.end());
                        return true;
                    }
                }
            }
        }
    }

    std::string dst;
    std::string src;
    int value_off = 0;
    int temp2_off = 0;
    int temp8_off = 0;
    if (lines_[i].mnemonic != "ld" ||
        !split_ld(lines_[i].operands, dst, src) ||
        trim(dst) != "a" ||
        !parse_ix_ref(trim(src), value_off)) {
        return false;
    }
    const std::string value_ref = std::to_string(value_off) + "(ix)";
    if (!is_add_a_a(lines_[i + 1]))
        return false;
    if (lines_[i + 2].mnemonic != "ld" ||
        !split_ld(lines_[i + 2].operands, dst, src) ||
        !parse_ix_ref(trim(dst), temp2_off) ||
        trim(src) != "a") {
        return false;
    }
    if (!match_ld(i + 3, "a", value_ref) ||
        !is_add_a_a(lines_[i + 4]) ||
        !is_add_a_a(lines_[i + 5]) ||
        !is_add_a_a(lines_[i + 6])) {
        return false;
    }
    if (lines_[i + 7].mnemonic != "ld" ||
        !split_ld(lines_[i + 7].operands, dst, src) ||
        !parse_ix_ref(trim(dst), temp8_off) ||
        trim(src) != "a") {
        return false;
    }
    if (temp2_off == value_off || temp8_off == value_off || temp2_off == temp8_off)
        return false;

    const std::string temp2_ref = std::to_string(temp2_off) + "(ix)";
    const std::string temp8_ref = std::to_string(temp8_off) + "(ix)";
    if (!match_ld(i + 8, "a", temp2_ref) ||
        !match_ld(i + 9, "e", "a") ||
        !match_ld(i + 10, "a", temp8_ref) ||
        !match_ld(i + 11, "d", "a") ||
        !match_ld(i + 12, "a", "e") ||
        !match_ld(i + 14, value_ref, "a")) {
        return false;
    }
    if (lines_[i + 13].mnemonic != "add") {
        return false;
    }
    {
        const std::string ops = trim(lines_[i + 13].operands);
        if (ops != "a, d" && ops != "a,d")
            return false;
    }

    for (size_t j = i + 15; j < lines_.size(); ++j) {
        if (lines_[j].operands.find(temp2_ref) != std::string::npos ||
            lines_[j].operands.find(temp8_ref) != std::string::npos) {
            return false;
        }
    }

    std::vector<asm_line> repl = {
        asm_line::parse("\tld\ta, " + value_ref),
        asm_line::parse("\tadd\ta, a"),
        asm_line::parse("\tld\te, a"),
        asm_line::parse("\tadd\ta, a"),
        asm_line::parse("\tadd\ta, a"),
        asm_line::parse("\tld\td, a"),
        asm_line::parse("\tadd\ta, e"),
        asm_line::parse("\tld\t" + value_ref + ", a"),
    };
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 15));
    lines_.insert(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                  repl.begin(), repl.end());
    return true;
}

bool z80_peep::rule_ix_byte_add_spill_elide(size_t i) {
    if (i + 6 >= lines_.size())
        return false;
    for (size_t k = 0; k < 7; ++k) {
        if (!lines_[i + k].label.empty())
            return false;
    }

    auto match_ld = [&](size_t idx, const std::string &want_dst,
                        const std::string &want_src) {
        std::string dst;
        std::string src;
        return lines_[idx].mnemonic == "ld" &&
               split_ld(lines_[idx].operands, dst, src) &&
               trim(dst) == want_dst &&
               trim(src) == want_src;
    };

    std::string dst;
    std::string src;
    int lhs_off = 0;
    int rhs_off = 0;
    int out_off = 0;
    if (lines_[i].mnemonic != "ld" ||
        !split_ld(lines_[i].operands, dst, src) ||
        trim(dst) != "a" ||
        !parse_ix_ref(trim(src), lhs_off)) {
        return false;
    }
    if (!match_ld(i + 1, "e", "a"))
        return false;
    if (lines_[i + 2].mnemonic != "ld" ||
        !split_ld(lines_[i + 2].operands, dst, src) ||
        trim(dst) != "a" ||
        !parse_ix_ref(trim(src), rhs_off)) {
        return false;
    }
    if (!match_ld(i + 3, "d", "a") ||
        !match_ld(i + 4, "a", "e")) {
        return false;
    }
    if (lines_[i + 5].mnemonic != "add") {
        return false;
    }
    {
        const std::string ops = trim(lines_[i + 5].operands);
        if (ops != "a, d" && ops != "a,d")
            return false;
    }
    if (lines_[i + 6].mnemonic != "ld" ||
        !split_ld(lines_[i + 6].operands, dst, src) ||
        !parse_ix_ref(trim(dst), out_off) ||
        trim(src) != "a") {
        return false;
    }
    auto de_overwritten_before_read_linear = [&](size_t start) {
        const size_t end = std::min(lines_.size(), start + 16);
        for (size_t k = start; k < end; ++k) {
            const asm_line &line = lines_[k];
            if (!line.label.empty() || is_section_directive(line))
                return false;
            if (line.mnemonic.empty())
                continue;
            if (overwrites_pair_without_reading_it(lines_, k, "de", 'e', 'd'))
                return true;

            const std::string ops = trim(line.operands);
            const std::string &m = line.mnemonic;
            if (m == "push" || m == "pop") {
                if (operand_mentions_pair_or_bytes(ops, "de", 'e', 'd'))
                    return false;
                continue;
            }
            if (m == "call" || m == "ret" || m == "reti" || m == "retn" ||
                m == "rst" || m == "jp" || m == "jr" || m == "djnz" ||
                m == "ex" || m == "exx") {
                return false;
            }
            if (operand_mentions_pair_or_bytes(ops, "de", 'e', 'd'))
                return false;
        }
        return false;
    };

    if (!path_overwrites_pair_before_read(lines_, i + 7, "de", 'e', 'd') &&
        !de_overwritten_before_read_linear(i + 7)) {
        return false;
    }

    const std::string lhs_ref = std::to_string(lhs_off) + "(ix)";
    const std::string rhs_ref = std::to_string(rhs_off) + "(ix)";
    const std::string out_ref = std::to_string(out_off) + "(ix)";
    std::vector<asm_line> repl = {
        asm_line::parse("\tld\ta, " + lhs_ref),
        asm_line::parse("\tadd\ta, " + rhs_ref),
        asm_line::parse("\tld\t" + out_ref + ", a"),
    };
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 7));
    lines_.insert(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                  repl.begin(), repl.end());
    return true;
}

bool z80_peep::rule_ix_byte_add_spill_branch_elide(size_t i) {
    if (i + 7 >= lines_.size())
        return false;
    for (size_t k = 0; k < 8; ++k) {
        if (!lines_[i + k].label.empty())
            return false;
    }

    auto match_ld = [&](size_t idx, const std::string &want_dst,
                        const std::string &want_src) {
        std::string dst;
        std::string src;
        return lines_[idx].mnemonic == "ld" &&
               split_ld(lines_[idx].operands, dst, src) &&
               trim(dst) == want_dst &&
               trim(src) == want_src;
    };

    std::string dst;
    std::string src;
    int lhs_off = 0;
    int rhs_off = 0;
    if (lines_[i].mnemonic != "ld" ||
        !split_ld(lines_[i].operands, dst, src) ||
        trim(dst) != "a" ||
        !parse_ix_ref(trim(src), lhs_off)) {
        return false;
    }
    if (!match_ld(i + 1, "e", "a"))
        return false;
    if (lines_[i + 2].mnemonic != "ld" ||
        !split_ld(lines_[i + 2].operands, dst, src) ||
        trim(dst) != "a" ||
        !parse_ix_ref(trim(src), rhs_off)) {
        return false;
    }
    if (!match_ld(i + 3, "d", "a") ||
        !match_ld(i + 4, "a", "e")) {
        return false;
    }
    if (lines_[i + 5].mnemonic != "add") {
        return false;
    }
    {
        const std::string ops = trim(lines_[i + 5].operands);
        if (ops != "a, d" && ops != "a,d")
            return false;
    }
    if (!is_or_a_self(lines_[i + 6]))
        return false;
    std::string cc;
    std::string target;
    if (!split_conditional_branch_target(lines_[i + 7], cc, target) ||
        (cc != "z" && cc != "nz")) {
        return false;
    }
    if (!path_overwrites_pair_before_read(lines_, i + 7, "de", 'e', 'd'))
        return false;

    const std::string lhs_ref = std::to_string(lhs_off) + "(ix)";
    const std::string rhs_ref = std::to_string(rhs_off) + "(ix)";
    std::vector<asm_line> repl = {
        asm_line::parse("\tld\ta, " + lhs_ref),
        asm_line::parse("\tadd\ta, " + rhs_ref),
        lines_[i + 6],
        lines_[i + 7],
    };
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 8));
    lines_.insert(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                  repl.begin(), repl.end());
    return true;
}

bool z80_peep::rule_unsigned_le_branch_fold(size_t i) {
    if (i + 3 >= lines_.size())
        return false;
    for (size_t k = 0; k < 4; ++k) {
        if (!lines_[i + k].label.empty())
            return false;
    }
    if (lines_[i].mnemonic != "cp")
        return false;

    int limit = 0;
    if (!parse_immediate_value(lines_[i].operands, limit))
        return false;
    limit = u8_value(limit);
    if (limit == 0xff)
        return false;

    std::string cc_z;
    std::string true_target;
    if (!split_conditional_branch_target(lines_[i + 1], cc_z, true_target) ||
        cc_z != "z") {
        return false;
    }
    std::string cc_c;
    std::string true_target_2;
    if (!split_conditional_branch_target(lines_[i + 2], cc_c, true_target_2) ||
        cc_c != "c" || true_target_2 != true_target) {
        return false;
    }
    std::string false_target;
    if (!parse_unconditional_jump(lines_[i + 3], false_target) ||
        false_target == true_target) {
        return false;
    }

    const size_t true_idx = find_label_index(lines_, true_target);
    const size_t false_idx = find_label_index(lines_, false_target);
    if (true_idx == lines_.size() || false_idx == lines_.size())
        return false;
    if (!flags_overwritten_before_read_or_escape(lines_, true_idx) ||
        !flags_overwritten_before_read_or_escape(lines_, false_idx)) {
        return false;
    }

    lines_[i].operands = "#" + std::to_string(limit + 1);
    lines_[i + 1] = lines_[i + 2];
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 2));
    return true;
}

bool z80_peep::rule_superopt_ix_byte_load_forward(size_t i) {
    if (i + 1 >= lines_.size())
        return false;
    auto &load = lines_[i];
    auto &copy = lines_[i + 1];
    if (!load.label.empty() || !copy.label.empty())
        return false;
    if (load.mnemonic != "ld" || copy.mnemonic != "ld")
        return false;

    std::string dst, src;
    int offset = 0;
    if (!split_ld(load.operands, dst, src) ||
        trim(dst) != "a" ||
        !parse_ix_ref(trim(src), offset)) {
        return false;
    }

    std::string reg;
    if (!split_ld(copy.operands, reg, src))
        return false;
    reg = trim(reg);
    if (!is_plain_8bit_reg(reg) || reg == "a" || trim(src) != "a")
        return false;
    if (!a_overwritten_before_read(lines_, i + 2))
        return false;

    load = asm_line::parse(
        "\tld\t" + reg + ", " + std::to_string(offset) + "(ix)");
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 1));
    return true;
}

bool z80_peep::rule_superopt_ix_byte_alu_forward(size_t i) {
    if (i + 3 >= lines_.size())
        return false;
    for (size_t k = 0; k < 4; ++k) {
        if (!lines_[i + k].label.empty())
            return false;
    }

    std::string dst;
    std::string src;
    int d_offset = 0;
    if (lines_[i].mnemonic != "ld" ||
        !split_ld(lines_[i].operands, dst, src) ||
        trim(dst) != "e") {
        return false;
    }
    const std::string lhs_src = trim(src);
    if (lines_[i + 1].mnemonic != "ld" ||
        !split_ld(lines_[i + 1].operands, dst, src) ||
        trim(dst) != "d" ||
        !parse_ix_ref(trim(src), d_offset)) {
        return false;
    }
    if (lines_[i + 2].mnemonic != "ld" ||
        !split_ld(lines_[i + 2].operands, dst, src) ||
        trim(dst) != "a" || trim(src) != "e") {
        return false;
    }

    const std::string op = lines_[i + 3].mnemonic;
    const std::string ops = trim(lines_[i + 3].operands);
    if (op != "add" && op != "xor" && op != "sub" &&
        op != "and" && op != "or" && op != "cp") {
        return false;
    }
    if (op == "add") {
        if (!is_accumulator_reg_alu(lines_[i + 3], "add", "d"))
            return false;
    } else if (ops != "d" && ops != "a,d" && ops != "a, d") {
        return false;
    }

    if (!path_overwrites_pair_before_read(lines_, i + 4, "de", 'e', 'd'))
        return false;

    const std::string d_ref = std::to_string(d_offset) + "(ix)";
    auto emit_op = [&](size_t idx) {
        if (op == "add") {
            lines_[idx] = asm_line::parse("\tadd\ta, " + d_ref);
        } else {
            lines_[idx] = asm_line::parse("\t" + op + "\t" + d_ref);
        }
    };

    int e_offset = 0;
    if (parse_ix_ref(lhs_src, e_offset)) {
        const std::string e_ref = std::to_string(e_offset) + "(ix)";
        lines_[i] = asm_line::parse("\tld\ta, " + e_ref);
        emit_op(i + 1);
        lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 2),
                     lines_.begin() + static_cast<std::ptrdiff_t>(i + 4));
        return true;
    }

    if (operand_has_token(lhs_src, "d") || operand_has_token(lhs_src, "e") ||
        operand_has_token(lhs_src, "de") || operand_has_token(lhs_src, "af")) {
        return false;
    }

    if (lhs_src == "a") {
        emit_op(i);
        lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 1),
                     lines_.begin() + static_cast<std::ptrdiff_t>(i + 4));
        return true;
    }

    if (!is_plain_8bit_reg(lhs_src) && !is_immediate_operand(lhs_src) &&
        !is_numeric_literal(lhs_src) && !uses_hl_indirect(lhs_src) &&
        !uses_abs_indirect(lhs_src)) {
        return false;
    }

    lines_[i] = asm_line::parse("\tld\ta, " + lhs_src);
    emit_op(i + 1);
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 2),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 4));
    return true;
}

bool z80_peep::rule_superopt_dead_a_zero_reg(size_t i) {
    if (i + 2 >= lines_.size())
        return false;
    for (size_t k = 0; k < 3; ++k) {
        if (!lines_[i + k].label.empty())
            return false;
    }

    std::string dst;
    std::string src;
    if (lines_[i].mnemonic != "ld" ||
        !split_ld(lines_[i].operands, dst, src) ||
        trim(dst) != "a") {
        return false;
    }
    const std::string reg = trim(src);
    if (!is_plain_8bit_reg(reg) || reg == "a")
        return false;

    bool and_zero = false;
    if (lines_[i + 1].mnemonic == "and") {
        if (immediate_is(lines_[i + 1].operands, 0)) {
            and_zero = true;
        } else {
            std::string and_dst;
            std::string and_src;
            and_zero = split_ld(lines_[i + 1].operands, and_dst, and_src) &&
                       trim(and_dst) == "a" && immediate_is(and_src, 0);
        }
    }
    if (!and_zero)
        return false;

    if (lines_[i + 2].mnemonic != "ld" ||
        !split_ld(lines_[i + 2].operands, dst, src) ||
        trim(dst) != reg || trim(src) != "a") {
        return false;
    }

    if (!a_overwritten_before_read(lines_, i + 3) ||
        !flags_overwritten_before_read_or_escape(lines_, i + 3)) {
        return false;
    }

    lines_[i] = asm_line::parse("\tld\t" + reg + ", #0");
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 1),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 3));
    return true;
}

bool z80_peep::rule_superopt_hl_byte_load_forward(size_t i) {
    if (i + 1 >= lines_.size())
        return false;
    auto &load = lines_[i];
    auto &copy = lines_[i + 1];
    if (!load.label.empty() || !copy.label.empty())
        return false;
    if (load.mnemonic != "ld" || copy.mnemonic != "ld")
        return false;

    std::string dst, src;
    if (!split_ld(load.operands, dst, src) ||
        trim(dst) != "a" ||
        trim(src) != "(hl)") {
        return false;
    }

    std::string reg;
    if (!split_ld(copy.operands, reg, src))
        return false;
    reg = trim(reg);
    if (!is_plain_8bit_reg(reg) || reg == "a" || trim(src) != "a")
        return false;

    auto is_ld_reg = [&](size_t idx, const char *want_dst,
                         const char *want_src) {
        if (idx >= lines_.size() || lines_[idx].mnemonic != "ld")
            return false;
        std::string line_dst, line_src;
        return split_ld(lines_[idx].operands, line_dst, line_src) &&
               trim(line_dst) == want_dst && trim(line_src) == want_src;
    };
    auto is_ld_zero = [&](size_t idx, const char *want_dst) {
        if (idx >= lines_.size() || lines_[idx].mnemonic != "ld")
            return false;
        std::string line_dst, line_src;
        return split_ld(lines_[idx].operands, line_dst, line_src) &&
               trim(line_dst) == want_dst && immediate_is(line_src, 0);
    };

    if (reg == "c" && is_ld_zero(i + 2, "b") &&
        ((is_ld_reg(i + 3, "h", "b") && is_ld_reg(i + 4, "l", "c")) ||
         (i + 3 < lines_.size() && is_or_a_self(lines_[i + 3])))) {
        return false;
    }
    if (reg == "l" && is_ld_zero(i + 2, "h") &&
        is_ld_reg(i + 3, "b", "h") &&
        is_ld_reg(i + 4, "c", "l")) {
        return false;
    }
    if (!a_overwritten_before_read(lines_, i + 2))
        return false;

    load = asm_line::parse("\tld\t" + reg + ", (hl)");
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 1));
    return true;
}

bool z80_peep::rule_superopt_compare_fallthrough_reload(size_t i) {
    if (i + 3 >= lines_.size())
        return false;
    const auto &load = lines_[i];
    const auto &compare = lines_[i + 1];
    const auto &branch = lines_[i + 2];
    if (!load.label.empty() || !compare.label.empty() || !branch.label.empty())
        return false;
    if (load.mnemonic != "ld" || compare.mnemonic != "cp")
        return false;

    std::string dst;
    std::string src;
    if (!split_ld(load.operands, dst, src) || trim(dst) != "a")
        return false;
    src = trim(src);
    if (operand_has_token(src, "a") || operand_has_token(src, "af"))
        return false;

    std::string cc;
    std::string target;
    if (!split_conditional_branch_target(branch, cc, target))
        return false;

    size_t reload_idx = i + 3;
    while (reload_idx < lines_.size() &&
           !lines_[reload_idx].label.empty() &&
           lines_[reload_idx].mnemonic.empty()) {
        if (label_has_other_control_references(
                lines_, lines_[reload_idx].label, lines_.size())) {
            return false;
        }
        ++reload_idx;
    }
    if (reload_idx >= lines_.size())
        return false;

    const auto &reload = lines_[reload_idx];
    if (!reload.label.empty() || reload.mnemonic != "ld")
        return false;
    std::string reload_dst;
    std::string reload_src;
    if (!split_ld(reload.operands, reload_dst, reload_src) ||
        trim(reload_dst) != "a" || trim(reload_src) != src) {
        return false;
    }

    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(reload_idx));
    return true;
}

bool z80_peep::rule_superopt_xor_compare_fallthrough_reload(size_t i) {
    if (i + 5 >= lines_.size())
        return false;

    const auto &load = lines_[i];
    const auto &transform = lines_[i + 1];
    const auto &compare = lines_[i + 2];
    const auto &branch = lines_[i + 3];
    if (!load.label.empty() || !transform.label.empty() ||
        !compare.label.empty() || !branch.label.empty()) {
        return false;
    }
    if (load.mnemonic != "ld" || transform.mnemonic != "xor" ||
        compare.mnemonic != "cp") {
        return false;
    }

    std::string dst;
    std::string src;
    if (!split_ld(load.operands, dst, src) || trim(dst) != "a")
        return false;
    src = trim(src);
    if (operand_has_token(src, "a") || operand_has_token(src, "af"))
        return false;

    int transform_value = 0;
    if (!parse_immediate_value(transform.operands, transform_value))
        return false;

    std::string cc;
    std::string target;
    if (!split_conditional_branch_target(branch, cc, target))
        return false;

    size_t reload_idx = i + 4;
    while (reload_idx < lines_.size() &&
           !lines_[reload_idx].label.empty() &&
           lines_[reload_idx].mnemonic.empty()) {
        if (label_has_other_control_references(
                lines_, lines_[reload_idx].label, lines_.size())) {
            return false;
        }
        ++reload_idx;
    }
    if (reload_idx + 2 >= lines_.size())
        return false;

    const auto &reload = lines_[reload_idx];
    const auto &repeat_transform = lines_[reload_idx + 1];
    const auto &next_compare = lines_[reload_idx + 2];
    if (!reload.label.empty() || !repeat_transform.label.empty() ||
        !next_compare.label.empty()) {
        return false;
    }
    if (reload.mnemonic != "ld" || repeat_transform.mnemonic != "xor" ||
        next_compare.mnemonic != "cp") {
        return false;
    }

    std::string reload_dst;
    std::string reload_src;
    if (!split_ld(reload.operands, reload_dst, reload_src) ||
        trim(reload_dst) != "a" || trim(reload_src) != src) {
        return false;
    }
    int repeat_value = 0;
    if (!parse_immediate_value(repeat_transform.operands, repeat_value) ||
        u8_value(repeat_value) != u8_value(transform_value)) {
        return false;
    }

    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(reload_idx),
                 lines_.begin() + static_cast<std::ptrdiff_t>(reload_idx + 2));
    return true;
}

bool z80_peep::rule_superopt_redundant_zero_store_chain(size_t i) {
    if (i + 2 >= lines_.size())
        return false;
    const auto &zero = lines_[i];
    const auto &store = lines_[i + 1];
    const auto &repeat_zero = lines_[i + 2];
    if (!zero.label.empty() || !store.label.empty() ||
        !repeat_zero.label.empty()) {
        return false;
    }
    if (!is_xor_a_self(zero) || !is_xor_a_self(repeat_zero))
        return false;
    if (store.mnemonic != "ld")
        return false;

    std::string dst;
    std::string src;
    if (!split_ld(store.operands, dst, src) || trim(src) != "a")
        return false;
    dst = trim(dst);
    if (dst == "a" || dst == "af" || dst == "sp")
        return false;

    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 2));
    return true;
}

bool z80_peep::rule_superopt_zero_extend_pair_test_shortcut(size_t i) {
    if (i + 7 >= lines_.size())
        return false;
    for (size_t j = i; j <= i + 7; ++j) {
        if (!lines_[j].label.empty())
            return false;
    }

    std::string dst, src;
    if (lines_[i].mnemonic != "ld" ||
        !split_ld(lines_[i].operands, dst, src) ||
        trim(dst) != "l") {
        return false;
    }
    if (trim(src) == "a")
        return false;
    if (lines_[i + 1].mnemonic != "ld" ||
        !split_ld(lines_[i + 1].operands, dst, src) ||
        trim(dst) != "h" || !immediate_is(src, 0)) {
        return false;
    }
    if (lines_[i + 2].mnemonic != "ld" ||
        !split_ld(lines_[i + 2].operands, dst, src) ||
        trim(dst) != "b" || trim(src) != "h") {
        return false;
    }
    if (lines_[i + 3].mnemonic != "ld" ||
        !split_ld(lines_[i + 3].operands, dst, src) ||
        trim(dst) != "c" || trim(src) != "l") {
        return false;
    }
    if (lines_[i + 4].mnemonic != "ld" ||
        !split_ld(lines_[i + 4].operands, dst, src) ||
        trim(dst) != "h" || trim(src) != "b") {
        return false;
    }
    if (lines_[i + 5].mnemonic != "ld" ||
        !split_ld(lines_[i + 5].operands, dst, src) ||
        trim(dst) != "l" || trim(src) != "c") {
        return false;
    }
    if (lines_[i + 6].mnemonic != "ld" ||
        !split_ld(lines_[i + 6].operands, dst, src) ||
        trim(dst) != "a" || trim(src) != "h") {
        return false;
    }

    const std::string test_ops = trim(lines_[i + 7].operands);
    if (lines_[i + 7].mnemonic != "or" ||
        (test_ops != "l" && test_ops != "a,l" && test_ops != "a, l")) {
        return false;
    }

    lines_[i + 4] = asm_line::parse("\tld\ta, l");
    lines_[i + 5] = asm_line::parse("\tor\ta, a");
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 6),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 8));
    return true;
}

bool z80_peep::rule_superopt_low_byte_xor_forward(size_t i) {
    if (i + 3 >= lines_.size())
        return false;

    for (size_t j = i; j <= i + 3; ++j) {
        if (!lines_[j].label.empty())
            return false;
    }

    std::string dst, src;
    if (lines_[i].mnemonic != "ld" ||
        !split_ld(lines_[i].operands, dst, src) ||
        trim(dst) != "l" || trim(src) != "c") {
        return false;
    }
    if (lines_[i + 1].mnemonic != "ld" ||
        !split_ld(lines_[i + 1].operands, dst, src) ||
        trim(dst) != "a") {
        return false;
    }
    src = trim(src);
    if (!is_pure_register_load_source(src) ||
        operand_has_token(src, "l") || operand_has_token(src, "hl")) {
        return false;
    }

    const std::string xor_ops = trim(lines_[i + 2].operands);
    if (lines_[i + 2].mnemonic != "xor" ||
        (xor_ops != "l" && xor_ops != "a,l" && xor_ops != "a, l")) {
        return false;
    }
    if (lines_[i + 3].mnemonic != "ld" ||
        !split_ld(lines_[i + 3].operands, dst, src) ||
        trim(dst) != "l" || trim(src) != "a") {
        return false;
    }

    lines_[i + 2].operands = "c";
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i));
    return true;
}

bool z80_peep::rule_superopt_dead_bc_xor_hl_forward(size_t i) {
    if (i + 7 >= lines_.size())
        return false;

    for (size_t j = i; j <= i + 7; ++j) {
        if (!lines_[j].label.empty())
            return false;
    }

    std::string dst, src;
    if (lines_[i].mnemonic != "ld" ||
        !split_ld(lines_[i].operands, dst, src) ||
        trim(dst) != "a" || trim(src) != "c") {
        return false;
    }
    if (!is_accumulator_reg_alu(lines_[i + 1], "xor", "l"))
        return false;
    if (lines_[i + 2].mnemonic != "ld" ||
        !split_ld(lines_[i + 2].operands, dst, src) ||
        trim(dst) != "c" || trim(src) != "a") {
        return false;
    }
    if (lines_[i + 3].mnemonic != "ld" ||
        !split_ld(lines_[i + 3].operands, dst, src) ||
        trim(dst) != "a" || trim(src) != "b") {
        return false;
    }
    if (!is_accumulator_reg_alu(lines_[i + 4], "xor", "h"))
        return false;
    if (lines_[i + 5].mnemonic != "ld" ||
        !split_ld(lines_[i + 5].operands, dst, src) ||
        trim(dst) != "b" || trim(src) != "a") {
        return false;
    }
    if (lines_[i + 6].mnemonic != "ld" ||
        !split_ld(lines_[i + 6].operands, dst, src) ||
        trim(dst) != "l" || trim(src) != "c") {
        return false;
    }
    if (lines_[i + 7].mnemonic != "ld" ||
        !split_ld(lines_[i + 7].operands, dst, src) ||
        trim(dst) != "h" || trim(src) != "b") {
        return false;
    }

    if (!path_overwrites_pair_before_read(lines_, i + 8, "bc", 'c', 'b'))
        return false;

    lines_[i] = asm_line::parse("\tld\ta, l");
    lines_[i + 1] = asm_line::parse("\txor\tc");
    lines_[i + 2] = asm_line::parse("\tld\tl, a");
    lines_[i + 3] = asm_line::parse("\tld\ta, h");
    lines_[i + 4] = asm_line::parse("\txor\tb");
    lines_[i + 5] = asm_line::parse("\tld\th, a");
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 6),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 8));
    return true;
}

bool z80_peep::rule_superopt_dead_c_xor_hl_forward(size_t i) {
    if (i + 7 >= lines_.size())
        return false;

    for (size_t j = i; j <= i + 7; ++j) {
        if (!lines_[j].label.empty())
            return false;
    }

    std::string dst, src;
    if (lines_[i].mnemonic != "ld" ||
        !split_ld(lines_[i].operands, dst, src) ||
        trim(dst) != "a" || trim(src) != "c") {
        return false;
    }
    if (!is_accumulator_reg_alu(lines_[i + 1], "xor", "l"))
        return false;
    if (lines_[i + 2].mnemonic != "ld" ||
        !split_ld(lines_[i + 2].operands, dst, src) ||
        trim(dst) != "c" || trim(src) != "a") {
        return false;
    }
    if (lines_[i + 3].mnemonic != "ld" ||
        !split_ld(lines_[i + 3].operands, dst, src) ||
        trim(dst) != "a" || trim(src) != "b") {
        return false;
    }
    if (!is_accumulator_reg_alu(lines_[i + 4], "xor", "h"))
        return false;
    if (lines_[i + 5].mnemonic != "ld" ||
        !split_ld(lines_[i + 5].operands, dst, src) ||
        trim(dst) != "b" || trim(src) != "a") {
        return false;
    }
    if (lines_[i + 6].mnemonic != "ld" ||
        !split_ld(lines_[i + 6].operands, dst, src) ||
        trim(dst) != "l" || trim(src) != "c") {
        return false;
    }
    if (lines_[i + 7].mnemonic != "ld" ||
        !split_ld(lines_[i + 7].operands, dst, src) ||
        trim(dst) != "h" || trim(src) != "b") {
        return false;
    }

    if (!c_overwritten_before_read(lines_, i + 8))
        return false;

    lines_[i] = asm_line::parse("\tld\ta, c");
    lines_[i + 1] = asm_line::parse("\txor\tl");
    lines_[i + 2] = asm_line::parse("\tld\tl, a");
    lines_[i + 3] = asm_line::parse("\tld\ta, b");
    lines_[i + 4] = asm_line::parse("\txor\th");
    lines_[i + 5] = asm_line::parse("\tld\tb, a");
    lines_[i + 6] = asm_line::parse("\tld\th, a");
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 7));
    return true;
}

bool z80_peep::rule_superopt_de_xor_right5(size_t i) {
    if (i + 16 >= lines_.size())
        return false;

    for (size_t j = i; j <= i + 16; ++j) {
        if (!lines_[j].label.empty())
            return false;
    }

    std::string dst, src;
    if (lines_[i].mnemonic != "ld" ||
        !split_ld(lines_[i].operands, dst, src) ||
        trim(dst) != "a" || trim(src) != "e") {
        return false;
    }
    if (lines_[i + 1].mnemonic != "ld" ||
        !split_ld(lines_[i + 1].operands, dst, src) ||
        trim(dst) != "l" || trim(src) != "d") {
        return false;
    }
    for (size_t k = 0; k < 5; ++k) {
        const size_t srl_idx = i + 2 + 2 * k;
        const size_t rr_idx = srl_idx + 1;
        if (lines_[srl_idx].mnemonic != "srl" ||
            trim(lines_[srl_idx].operands) != "l" ||
            lines_[rr_idx].mnemonic != "rr" ||
            trim(lines_[rr_idx].operands) != "a") {
            return false;
        }
    }
    if (!is_accumulator_reg_alu(lines_[i + 12], "xor", "e"))
        return false;
    if (lines_[i + 13].mnemonic != "ld" ||
        !split_ld(lines_[i + 13].operands, dst, src) ||
        trim(dst) != "e" || trim(src) != "a") {
        return false;
    }
    if (lines_[i + 14].mnemonic != "ld" ||
        !split_ld(lines_[i + 14].operands, dst, src) ||
        trim(dst) != "a" || trim(src) != "l") {
        return false;
    }
    if (!is_accumulator_reg_alu(lines_[i + 15], "xor", "d"))
        return false;
    if (lines_[i + 16].mnemonic != "ld" ||
        !split_ld(lines_[i + 16].operands, dst, src) ||
        trim(dst) != "d" || trim(src) != "a") {
        return false;
    }

    static const char *const replacement[] = {
        "\tld\ta, d",
        "\trlca",
        "\trlca",
        "\trlca",
        "\tand\t#248",
        "\tld\tl, a",
        "\tld\ta, e",
        "\trlca",
        "\trlca",
        "\trlca",
        "\tand\t#7",
        "\tor\tl",
        "\txor\te",
        "\tld\te, a",
        "\tld\ta, d",
        "\trlca",
        "\trlca",
        "\trlca",
        "\tand\t#7",
        "\tld\tl, a",
        "\txor\td",
        "\tld\td, a",
    };

    std::vector<asm_line> out;
    out.reserve(sizeof(replacement) / sizeof(replacement[0]));
    for (const char *line : replacement)
        out.push_back(asm_line::parse(line));

    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 17));
    lines_.insert(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                  out.begin(), out.end());
    return true;
}

bool z80_peep::rule_superopt_hl_xor_right5_stack(size_t i) {
    if (i + 21 >= lines_.size())
        return false;

    for (size_t j = i; j <= i + 21; ++j) {
        if (!lines_[j].label.empty())
            return false;
    }

    std::string dst, src;
    int store_low_offset = 0;
    int store_high_offset = 0;
    int reload_low_offset = 0;
    int reload_high_offset = 0;

    if (lines_[i].mnemonic != "ld" ||
        !split_ld(lines_[i].operands, dst, src) ||
        !parse_ix_ref(trim(dst), store_low_offset) ||
        trim(src) != "l") {
        return false;
    }
    if (lines_[i + 1].mnemonic != "ld" ||
        !split_ld(lines_[i + 1].operands, dst, src) ||
        !parse_ix_ref(trim(dst), store_high_offset) ||
        store_high_offset != store_low_offset + 1 ||
        trim(src) != "h") {
        return false;
    }

    for (size_t k = 0; k < 5; ++k) {
        const size_t srl_idx = i + 2 + 2 * k;
        const size_t rr_idx = srl_idx + 1;
        if (lines_[srl_idx].mnemonic != "srl" ||
            trim(lines_[srl_idx].operands) != "h" ||
            lines_[rr_idx].mnemonic != "rr" ||
            trim(lines_[rr_idx].operands) != "l") {
            return false;
        }
    }

    if (lines_[i + 12].mnemonic != "ld" ||
        !split_ld(lines_[i + 12].operands, dst, src) ||
        trim(dst) != "b" || trim(src) != "h") {
        return false;
    }
    if (lines_[i + 13].mnemonic != "ld" ||
        !split_ld(lines_[i + 13].operands, dst, src) ||
        trim(dst) != "c" || trim(src) != "l") {
        return false;
    }
    if (lines_[i + 14].mnemonic != "ld" ||
        !split_ld(lines_[i + 14].operands, dst, src) ||
        trim(dst) != "l" ||
        !parse_ix_ref(trim(src), reload_low_offset) ||
        reload_low_offset != store_low_offset) {
        return false;
    }
    if (lines_[i + 15].mnemonic != "ld" ||
        !split_ld(lines_[i + 15].operands, dst, src) ||
        trim(dst) != "h" ||
        !parse_ix_ref(trim(src), reload_high_offset) ||
        reload_high_offset != store_high_offset) {
        return false;
    }
    if (lines_[i + 16].mnemonic != "ld" ||
        !split_ld(lines_[i + 16].operands, dst, src) ||
        trim(dst) != "a" || trim(src) != "l") {
        return false;
    }
    if (!is_accumulator_reg_alu(lines_[i + 17], "xor", "c"))
        return false;
    if (lines_[i + 18].mnemonic != "ld" ||
        !split_ld(lines_[i + 18].operands, dst, src) ||
        trim(dst) != "l" || trim(src) != "a") {
        return false;
    }
    if (lines_[i + 19].mnemonic != "ld" ||
        !split_ld(lines_[i + 19].operands, dst, src) ||
        trim(dst) != "a" || trim(src) != "h") {
        return false;
    }
    if (!is_accumulator_reg_alu(lines_[i + 20], "xor", "b"))
        return false;
    if (lines_[i + 21].mnemonic != "ld" ||
        !split_ld(lines_[i + 21].operands, dst, src) ||
        trim(dst) != "h" || trim(src) != "a") {
        return false;
    }

    static const char *const synth[] = {
        "\tld\ta, h",
        "\trlca",
        "\trlca",
        "\trlca",
        "\tand\t#248",
        "\tld\tc, a",
        "\tld\ta, l",
        "\trlca",
        "\trlca",
        "\trlca",
        "\tand\t#7",
        "\tor\tc",
        "\tld\tc, a",
        "\tld\ta, h",
        "\trlca",
        "\trlca",
        "\trlca",
        "\tand\t#7",
        "\tld\tb, a",
        "\tld\ta, l",
        "\txor\tc",
        "\tld\tl, a",
        "\tld\ta, h",
        "\txor\tb",
        "\tld\th, a",
    };

    std::vector<asm_line> replacement;
    replacement.reserve(2 + sizeof(synth) / sizeof(synth[0]));
    replacement.push_back(lines_[i]);
    replacement.push_back(lines_[i + 1]);
    for (const char *line : synth)
        replacement.push_back(asm_line::parse(line));

    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 22));
    lines_.insert(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                  replacement.begin(), replacement.end());
    return true;
}

bool z80_peep::rule_superopt_zero_extend_a_to_bc(size_t i) {
    if (i + 4 >= lines_.size())
        return false;

    for (size_t j = i; j < i + 4; ++j) {
        if (!lines_[j].label.empty())
            return false;
    }

    std::string dst, src;
    if (lines_[i].mnemonic != "ld" ||
        !split_ld(lines_[i].operands, dst, src) ||
        trim(dst) != "l" || trim(src) != "a") {
        return false;
    }
    if (lines_[i + 1].mnemonic != "ld" ||
        !split_ld(lines_[i + 1].operands, dst, src) ||
        trim(dst) != "h" || !immediate_is(src, 0)) {
        return false;
    }
    if (lines_[i + 2].mnemonic != "ld" ||
        !split_ld(lines_[i + 2].operands, dst, src) ||
        trim(dst) != "b" || trim(src) != "h") {
        return false;
    }
    if (lines_[i + 3].mnemonic != "ld" ||
        !split_ld(lines_[i + 3].operands, dst, src) ||
        trim(dst) != "c" || trim(src) != "l") {
        return false;
    }

    if (!overwrites_hl_after_short_preserving_span(lines_, i + 4))
        return false;

    lines_[i] = asm_line::parse("\tld\tc, a");
    lines_[i + 1] = asm_line::parse("\tld\tb, #0");
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 2),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 4));
    return true;
}

bool z80_peep::rule_superopt_zero_extend_src_to_bc(size_t i) {
    if (i + 3 >= lines_.size())
        return false;

    for (size_t j = i; j <= i + 3; ++j) {
        if (!lines_[j].label.empty())
            return false;
    }

    std::string dst, src;
    if (lines_[i].mnemonic != "ld" ||
        !split_ld(lines_[i].operands, dst, src) ||
        trim(dst) != "l") {
        return false;
    }
    src = trim(src);
    if (src.empty())
        return false;

    const bool can_load_c =
        is_plain_8bit_reg(src) ||
        is_immediate_operand(src) ||
        is_numeric_literal(src) ||
        uses_hl_indirect(src) ||
        uses_ixiy_disp(src);
    if (!can_load_c)
        return false;

    if (lines_[i + 1].mnemonic != "ld" ||
        !split_ld(lines_[i + 1].operands, dst, src) ||
        trim(dst) != "h" || !immediate_is(src, 0)) {
        return false;
    }
    if (lines_[i + 2].mnemonic != "ld" ||
        !split_ld(lines_[i + 2].operands, dst, src) ||
        trim(dst) != "b" || trim(src) != "h") {
        return false;
    }
    if (lines_[i + 3].mnemonic != "ld" ||
        !split_ld(lines_[i + 3].operands, dst, src) ||
        trim(dst) != "c" || trim(src) != "l") {
        return false;
    }

    if (!path_overwrites_hl_before_read(lines_, i + 4))
        return false;

    const std::string source = trim(lines_[i].operands.substr(
        lines_[i].operands.find(',') + 1));
    std::vector<asm_line> replacement;
    if (source == "c") {
        replacement.push_back(asm_line::parse("\tld\tb, #0"));
    } else {
        replacement.push_back(asm_line::parse("\tld\tc, " + source));
        replacement.push_back(asm_line::parse("\tld\tb, #0"));
    }

    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 4));
    lines_.insert(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                  replacement.begin(), replacement.end());
    return true;
}

bool z80_peep::rule_superopt_zero_extend_truth_test(size_t i) {
    if (i + 5 >= lines_.size())
        return false;

    for (size_t j = i; j <= i + 5; ++j) {
        if (!lines_[j].label.empty())
            return false;
    }

    std::string dst, src;
    if (lines_[i].mnemonic != "ld" ||
        !split_ld(lines_[i].operands, dst, src) ||
        trim(dst) != "c" || trim(src) != "a") {
        return false;
    }
    if (lines_[i + 1].mnemonic != "ld" ||
        !split_ld(lines_[i + 1].operands, dst, src) ||
        trim(dst) != "b" || !immediate_is(src, 0)) {
        return false;
    }
    if (lines_[i + 2].mnemonic != "ld" ||
        !split_ld(lines_[i + 2].operands, dst, src) ||
        trim(dst) != "h" || trim(src) != "b") {
        return false;
    }
    if (lines_[i + 3].mnemonic != "ld" ||
        !split_ld(lines_[i + 3].operands, dst, src) ||
        trim(dst) != "l" || trim(src) != "c") {
        return false;
    }
    if (lines_[i + 4].mnemonic != "ld" ||
        !split_ld(lines_[i + 4].operands, dst, src) ||
        trim(dst) != "a" || trim(src) != "h") {
        return false;
    }

    const std::string ops = trim(lines_[i + 5].operands);
    if (lines_[i + 5].mnemonic != "or" ||
        (ops != "l" && ops != "a,l" && ops != "a, l")) {
        return false;
    }

    lines_[i + 4] = asm_line::parse("\tor\ta, a");
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 5));
    return true;
}

bool z80_peep::rule_superopt_zero_extend_dead_hl_truth_test(size_t i) {
    if (i + 5 >= lines_.size())
        return false;

    for (size_t j = i; j <= i + 5; ++j) {
        if (!lines_[j].label.empty())
            return false;
    }

    std::string dst, src;
    if (lines_[i].mnemonic != "ld" ||
        !split_ld(lines_[i].operands, dst, src) ||
        trim(dst) != "c" || trim(src) != "a") {
        return false;
    }
    if (lines_[i + 1].mnemonic != "ld" ||
        !split_ld(lines_[i + 1].operands, dst, src) ||
        trim(dst) != "b" || !immediate_is(src, 0)) {
        return false;
    }
    if (lines_[i + 2].mnemonic != "ld" ||
        !split_ld(lines_[i + 2].operands, dst, src) ||
        trim(dst) != "h" || trim(src) != "b") {
        return false;
    }
    if (lines_[i + 3].mnemonic != "ld" ||
        !split_ld(lines_[i + 3].operands, dst, src) ||
        trim(dst) != "l" || trim(src) != "c") {
        return false;
    }

    const auto &test = lines_[i + 4];
    const std::string test_ops = trim(test.operands);
    if (test.mnemonic != "or" ||
        (test_ops != "a" && test_ops != "a,a" && test_ops != "a, a")) {
        return false;
    }

    std::string cc, target;
    if (!split_conditional_branch_target(lines_[i + 5], cc, target))
        return false;

    size_t target_idx = find_label_index(lines_, target);
    if (target_idx == lines_.size())
        return false;

    if (!path_overwrites_hl_before_read(lines_, i + 6))
        return false;
    if (!path_overwrites_hl_before_read(lines_, target_idx))
        return false;

    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 2),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 4));
    return true;
}

bool z80_peep::rule_superopt_zero_extend_dead_bc_truth_test(size_t i) {
    if (i + 3 >= lines_.size())
        return false;

    for (size_t j = i; j <= i + 3; ++j) {
        if (!lines_[j].label.empty())
            return false;
    }

    std::string dst, src;
    if (lines_[i].mnemonic != "ld" ||
        !split_ld(lines_[i].operands, dst, src) ||
        trim(dst) != "c" || trim(src) != "a") {
        return false;
    }
    if (lines_[i + 1].mnemonic != "ld" ||
        !split_ld(lines_[i + 1].operands, dst, src) ||
        trim(dst) != "b" || !immediate_is(src, 0)) {
        return false;
    }

    const auto &test = lines_[i + 2];
    const std::string test_ops = trim(test.operands);
    if (test.mnemonic != "or" ||
        (test_ops != "a" && test_ops != "a,a" && test_ops != "a, a")) {
        return false;
    }

    std::string cc, target;
    if (!split_conditional_branch_target(lines_[i + 3], cc, target))
        return false;

    size_t target_idx = find_label_index(lines_, target);
    if (target_idx == lines_.size())
        return false;

    if (!path_overwrites_pair_before_read(lines_, i + 4, "bc", 'c', 'b'))
        return false;
    if (!path_overwrites_pair_before_read(lines_, target_idx, "bc", 'c', 'b'))
        return false;

    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 2));
    return true;
}

bool z80_peep::rule_superopt_de_to_hl_dead_de_copy(size_t i) {
    if (i + 1 >= lines_.size())
        return false;

    auto &hi = lines_[i];
    auto &lo = lines_[i + 1];
    if (!hi.label.empty() || !lo.label.empty())
        return false;
    if (hi.mnemonic != "ld" || lo.mnemonic != "ld")
        return false;

    std::string dst, src;
    if (!split_ld(hi.operands, dst, src) ||
        trim(dst) != "h" || trim(src) != "d") {
        return false;
    }
    if (!split_ld(lo.operands, dst, src) ||
        trim(dst) != "l" || trim(src) != "e") {
        return false;
    }

    if (!path_overwrites_pair_before_read(lines_, i + 2, "de", 'e', 'd'))
        return false;

    hi = asm_line::parse("\tex\tde, hl");
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 1));
    return true;
}

bool z80_peep::rule_superopt_dead_bc_return_copy(size_t i) {
    if (i + 2 >= lines_.size())
        return false;

    for (size_t scan = i; scan > 0;) {
        const asm_line &prev = lines_[--scan];
        if (is_spaghetti_helper_label(prev))
            return false;
        if (!prev.label.empty() || prev.mnemonic == "ret")
            break;
    }

    auto is_epilogue_from = [&](size_t pos) {
        auto next_mnemonic = [&](size_t &scan) -> const asm_line * {
            while (scan < lines_.size()) {
                const auto &line = lines_[scan++];
                if (!line.mnemonic.empty())
                    return &line;
            }
            return nullptr;
        };

        const asm_line *first = next_mnemonic(pos);
        if (!first)
            return false;
        if (first->mnemonic == "ret" && trim(first->operands).empty())
            return true;

        if (first->mnemonic == "pop" && trim(first->operands) == "ix") {
            const asm_line *ret = next_mnemonic(pos);
            return ret && ret->mnemonic == "ret" && trim(ret->operands).empty();
        }

        if (first->mnemonic == "ld") {
            std::string dst, src;
            if (!split_ld(first->operands, dst, src) ||
                trim(dst) != "sp" || trim(src) != "ix") {
                return false;
            }
            const asm_line *pop = next_mnemonic(pos);
            const asm_line *ret = next_mnemonic(pos);
            return pop && ret &&
                   pop->mnemonic == "pop" && trim(pop->operands) == "ix" &&
                   ret->mnemonic == "ret" && trim(ret->operands).empty();
        }

        return false;
    };

    for (size_t j = i; j <= i + 2; ++j) {
        if (!lines_[j].label.empty())
            return false;
    }

    std::string dst, src;
    if (lines_[i].mnemonic != "ld" ||
        !split_ld(lines_[i].operands, dst, src) ||
        trim(dst) != "b" || trim(src) != "h") {
        return false;
    }
    if (lines_[i + 1].mnemonic != "ld" ||
        !split_ld(lines_[i + 1].operands, dst, src) ||
        trim(dst) != "c" || trim(src) != "l") {
        return false;
    }
    if (lines_[i + 2].mnemonic != "ex" ||
        trim(lines_[i + 2].operands) != "de, hl") {
        return false;
    }
    if (!is_epilogue_from(i + 3))
        return false;

    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 2));
    return true;
}

static bool is_de_direct_load_source(const std::string &raw) {
    const std::string src = trim(raw);
    if (is_immediate_operand(src) || is_numeric_literal(src))
        return true;

    if (src.size() < 3 || src.front() != '(' || src.back() != ')')
        return false;

    const std::string inner = lower_copy(trim(src.substr(1, src.size() - 2)));
    if (inner.empty() || inner.find('(') != std::string::npos ||
        inner.find(')') != std::string::npos ||
        inner.find(',') != std::string::npos) {
        return false;
    }

    return inner != "a" && inner != "af" && inner != "b" &&
           inner != "bc" && inner != "c" && inner != "d" &&
           inner != "de" && inner != "e" && inner != "h" &&
           inner != "hl" && inner != "l" && inner != "ix" &&
           inner != "iy" && inner != "sp";
}

bool z80_peep::rule_superopt_hl_via_bc_return_de_copy(size_t i) {
    if (i + 3 >= lines_.size())
        return false;

    for (size_t j = i; j <= i + 3; ++j) {
        if (!lines_[j].label.empty() || lines_[j].mnemonic != "ld")
            return false;
    }

    std::string dst, src;
    if (!split_ld(lines_[i].operands, dst, src) ||
        trim(dst) != "b" || trim(src) != "h") {
        return false;
    }
    if (!split_ld(lines_[i + 1].operands, dst, src) ||
        trim(dst) != "c" || trim(src) != "l") {
        return false;
    }
    if (!split_ld(lines_[i + 2].operands, dst, src) ||
        trim(dst) != "d" || trim(src) != "b") {
        return false;
    }
    if (!split_ld(lines_[i + 3].operands, dst, src) ||
        trim(dst) != "e" || trim(src) != "c") {
        return false;
    }
    if (!is_modern_return_tail(lines_, i + 4))
        return false;

    lines_[i] = asm_line::parse("\tld\td, h");
    lines_[i + 1] = asm_line::parse("\tld\te, l");
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 2),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 4));
    return true;
}

bool z80_peep::rule_superopt_hl_load_return_de_direct(size_t i) {
    if (i + 2 >= lines_.size())
        return false;

    for (size_t j = i; j <= i + 2; ++j) {
        if (!lines_[j].label.empty())
            return false;
    }

    std::string dst, src;
    if (lines_[i].mnemonic != "ld" ||
        !split_ld(lines_[i].operands, dst, src) ||
        trim(dst) != "hl") {
        return false;
    }
    src = trim(src);
    if (!is_de_direct_load_source(src))
        return false;
    if (lines_[i + 1].mnemonic != "ld" ||
        !split_ld(lines_[i + 1].operands, dst, src) ||
        trim(dst) != "d" || trim(src) != "h") {
        return false;
    }
    if (lines_[i + 2].mnemonic != "ld" ||
        !split_ld(lines_[i + 2].operands, dst, src) ||
        trim(dst) != "e" || trim(src) != "l") {
        return false;
    }
    if (!is_modern_return_tail(lines_, i + 3))
        return false;

    std::string hl_dst, hl_src;
    split_ld(lines_[i].operands, hl_dst, hl_src);
    lines_[i].operands = "de, " + trim(hl_src);
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 1),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 3));
    return true;
}

bool z80_peep::rule_superopt_ix_word_return_de_direct(size_t i) {
    if (i + 3 >= lines_.size())
        return false;

    for (size_t j = i; j <= i + 3; ++j) {
        if (!lines_[j].label.empty() || lines_[j].mnemonic != "ld")
            return false;
    }

    std::string l_dst, l_src, h_dst, h_src, d_dst, d_src, e_dst, e_src;
    if (!split_ld(lines_[i].operands, l_dst, l_src) ||
        trim(l_dst) != "l") {
        return false;
    }
    if (!split_ld(lines_[i + 1].operands, h_dst, h_src) ||
        trim(h_dst) != "h") {
        return false;
    }
    int l_off = 0;
    int h_off = 0;
    if (!parse_ix_ref(trim(l_src), l_off) ||
        !parse_ix_ref(trim(h_src), h_off)) {
        return false;
    }
    if (!split_ld(lines_[i + 2].operands, d_dst, d_src) ||
        trim(d_dst) != "d" || trim(d_src) != "h") {
        return false;
    }
    if (!split_ld(lines_[i + 3].operands, e_dst, e_src) ||
        trim(e_dst) != "e" || trim(e_src) != "l") {
        return false;
    }
    if (!is_modern_return_tail(lines_, i + 4))
        return false;

    lines_[i] = asm_line::parse("\tld\te, " + std::to_string(l_off) + "(ix)");
    lines_[i + 1] = asm_line::parse("\tld\td, " + std::to_string(h_off) + "(ix)");
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 2),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 4));
    return true;
}

bool z80_peep::rule_superopt_hl_return_de_exchange(size_t i) {
    if (i + 1 >= lines_.size() ||
        !is_inside_sdcccall1_function(lines_, i)) {
        return false;
    }
    if (!lines_[i].label.empty() || !lines_[i + 1].label.empty() ||
        lines_[i].mnemonic != "ld" || lines_[i + 1].mnemonic != "ld") {
        return false;
    }

    std::string dst;
    std::string src;
    if (!split_ld(lines_[i].operands, dst, src) ||
        trim(dst) != "d" || trim(src) != "h") {
        return false;
    }
    if (!split_ld(lines_[i + 1].operands, dst, src) ||
        trim(dst) != "e" || trim(src) != "l" ||
        !is_modern_return_tail(lines_, i + 2)) {
        return false;
    }

    lines_[i] = asm_line::parse("\tex\tde, hl");
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 1));
    return true;
}

bool z80_peep::rule_superopt_lowbyte_sum_return_direct(size_t i) {
    if (i + 5 >= lines_.size())
        return false;

    for (size_t j = i; j <= i + 5; ++j) {
        if (!lines_[j].label.empty())
            return false;
    }

    std::string dst, src;
    if (lines_[i].mnemonic != "ld" ||
        !split_ld(lines_[i].operands, dst, src) ||
        trim(dst) != "b" || !immediate_is(src, 0)) {
        return false;
    }

    int addend = 0;
    if (lines_[i + 1].mnemonic != "ld" ||
        !split_ld(lines_[i + 1].operands, dst, src) ||
        trim(dst) != "hl" || !parse_immediate_value(src, addend)) {
        return false;
    }

    if (lines_[i + 2].mnemonic != "add" ||
        !split_ld(lines_[i + 2].operands, dst, src) ||
        trim(dst) != "hl" || trim(src) != "bc") {
        return false;
    }
    if (lines_[i + 3].mnemonic != "add" ||
        !split_ld(lines_[i + 3].operands, dst, src) ||
        trim(dst) != "hl" || trim(src) != "de") {
        return false;
    }
    if (lines_[i + 4].mnemonic != "ld" ||
        !split_ld(lines_[i + 4].operands, dst, src) ||
        trim(dst) != "a" || trim(src) != "l") {
        return false;
    }
    if (!is_plain_ret(lines_[i + 5]))
        return false;

    lines_[i] = asm_line::parse("\tld\ta, e");
    lines_[i + 1] = asm_line::parse("\tadd\ta, c");
    lines_[i + 2] = asm_line::parse("\tadd\ta, " + imm8_text(addend));
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 3),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 5));
    return true;
}

bool z80_peep::rule_superopt_bc_to_de_alu_forward(size_t i) {
    if (i + 2 >= lines_.size())
        return false;

    if (!lines_[i].label.empty() || !lines_[i + 1].label.empty())
        return false;
    if (lines_[i].mnemonic != "ld" || lines_[i + 1].mnemonic != "ld")
        return false;

    std::string dst, src;
    if (!split_ld(lines_[i].operands, dst, src) ||
        trim(dst) != "d" || trim(src) != "b") {
        return false;
    }
    if (!split_ld(lines_[i + 1].operands, dst, src) ||
        trim(dst) != "e" || trim(src) != "c") {
        return false;
    }

    size_t alu_idx = i + 2;
    if (alu_idx < lines_.size() && is_or_a_self(lines_[alu_idx]))
        ++alu_idx;
    if (alu_idx >= lines_.size() || !lines_[alu_idx].label.empty())
        return false;

    if (lines_[alu_idx].mnemonic == "sbc") {
        if (!split_ld(lines_[alu_idx].operands, dst, src) ||
            trim(dst) != "hl" || trim(src) != "de") {
            return false;
        }
        if (!path_overwrites_pair_before_read(lines_, alu_idx + 1,
                                              "de", 'e', 'd')) {
            return false;
        }
        lines_[alu_idx].operands = "hl, bc";
        lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                     lines_.begin() + static_cast<std::ptrdiff_t>(i + 2));
        return true;
    }

    if (lines_[alu_idx].mnemonic == "add") {
        if (!split_ld(lines_[alu_idx].operands, dst, src) ||
            trim(dst) != "hl" || trim(src) != "de") {
            return false;
        }
        if (!path_overwrites_pair_before_read(lines_, alu_idx + 1,
                                              "de", 'e', 'd')) {
            return false;
        }
        lines_[alu_idx].operands = "hl, bc";
        lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                     lines_.begin() + static_cast<std::ptrdiff_t>(i + 2));
        return true;
    }

    return false;
}

bool z80_peep::rule_superopt_dead_hl_de_return_store_forward(size_t i) {
    if (i + 5 >= lines_.size())
        return false;

    for (size_t j = i; j <= i + 5; ++j) {
        if (!lines_[j].label.empty())
            return false;
    }

    std::string dst, src;
    if (lines_[i].mnemonic != "ld" ||
        !split_ld(lines_[i].operands, dst, src) ||
        trim(dst) != "h" || trim(src) != "d") {
        return false;
    }
    if (lines_[i + 1].mnemonic != "ld" ||
        !split_ld(lines_[i + 1].operands, dst, src) ||
        trim(dst) != "l" || trim(src) != "e") {
        return false;
    }
    if (lines_[i + 2].mnemonic != "ld" ||
        !split_ld(lines_[i + 2].operands, dst, src) ||
        trim(dst) != "b" || trim(src) != "h") {
        return false;
    }
    if (lines_[i + 3].mnemonic != "ld" ||
        !split_ld(lines_[i + 3].operands, dst, src) ||
        trim(dst) != "c" || trim(src) != "l") {
        return false;
    }

    std::string lo_dst, lo_src, hi_dst, hi_src;
    if (lines_[i + 4].mnemonic != "ld" ||
        !split_ld(lines_[i + 4].operands, lo_dst, lo_src) ||
        trim(lo_src) != "l") {
        return false;
    }
    if (lines_[i + 5].mnemonic != "ld" ||
        !split_ld(lines_[i + 5].operands, hi_dst, hi_src) ||
        trim(hi_src) != "h") {
        return false;
    }

    lo_dst = trim(lo_dst);
    hi_dst = trim(hi_dst);
    if (is_plain_8bit_reg(lo_dst) || is_plain_8bit_reg(hi_dst))
        return false;
    if (operand_mentions_pair_or_bytes(lo_dst, "hl", 'l', 'h') ||
        operand_mentions_pair_or_bytes(hi_dst, "hl", 'l', 'h')) {
        return false;
    }
    if (!path_overwrites_hl_before_read(lines_, i + 6))
        return false;

    lines_[i] = asm_line::parse("\tld\tb, d");
    lines_[i + 1] = asm_line::parse("\tld\tc, e");
    lines_[i + 2] = asm_line::parse("\tld\t" + lo_dst + ", e");
    lines_[i + 3] = asm_line::parse("\tld\t" + hi_dst + ", d");
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 4),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 6));
    return true;
}

bool z80_peep::rule_superopt_modern_const_return_direct(size_t i) {
    if (i + 1 >= lines_.size())
        return false;

    auto &load = lines_[i];
    const auto &exchange = lines_[i + 1];
    if (!exchange.label.empty() || !is_ex_de_hl(exchange))
        return false;
    if (!in_sdcccall1_function(lines_, i))
        return false;

    std::string dst, src;
    if (load.mnemonic != "ld" ||
        !split_ld(load.operands, dst, src) ||
        trim(dst) != "hl") {
        return false;
    }
    src = trim(src);
    if (!is_immediate_operand(src) && !is_numeric_literal(src))
        return false;
    if (!is_modern_return_tail(lines_, i + 2))
        return false;

    load.operands = "de, " + src;
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 1));
    return true;
}

bool z80_peep::rule_superopt_cancel_exx_pair(size_t i) {
    if (i + 1 >= lines_.size())
        return false;

    const auto &first = lines_[i];
    const auto &second = lines_[i + 1];
    if (!first.label.empty() || !second.label.empty())
        return false;
    if (first.mnemonic != "exx" || second.mnemonic != "exx")
        return false;

    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 2));
    return true;
}

bool z80_peep::rule_superopt_call_arg_de_direct(size_t i) {
    if (i + 4 >= lines_.size())
        return false;

    for (size_t j = i; j <= i + 4; ++j) {
        if (!lines_[j].label.empty())
            return false;
    }

    std::string first_src, de_src, next_src;
    if (!is_ld_hl(lines_[i], first_src))
        return false;
    if (lines_[i + 1].mnemonic != "push" ||
        trim(lines_[i + 1].operands) != "hl") {
        return false;
    }
    if (!is_ld_hl(lines_[i + 2], de_src))
        return false;
    if (!is_ex_de_hl(lines_[i + 3]))
        return false;
    if (!is_ld_hl(lines_[i + 4], next_src))
        return false;
    if (!is_immediate_operand(de_src) && !is_numeric_literal(de_src))
        return false;

    if (trim(first_src) == de_src &&
        (is_immediate_operand(first_src) || is_numeric_literal(first_src))) {
        lines_[i] = asm_line::parse("\tld\tde, " + first_src);
        lines_[i + 1] = asm_line::parse("\tpush\tde");
        lines_[i + 2] = lines_[i + 4];
        lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 3),
                     lines_.begin() + static_cast<std::ptrdiff_t>(i + 5));
        return true;
    }

    lines_[i + 2].operands = "de, " + de_src;
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 3));
    return true;
}

bool z80_peep::rule_superopt_dead_hl_exchange_to_de_load(size_t i) {
    if (i + 1 >= lines_.size())
        return false;

    if (!lines_[i].label.empty() || !lines_[i + 1].label.empty())
        return false;

    std::string src;
    if (!is_ld_hl(lines_[i], src))
        return false;
    if (!is_immediate_operand(src) && !is_numeric_literal(src))
        return false;
    if (!is_ex_de_hl(lines_[i + 1]))
        return false;
    if (!path_overwrites_hl_before_read(lines_, i + 2))
        return false;

    lines_[i].operands = "de, " + src;
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 1));
    return true;
}

bool z80_peep::rule_superopt_equal_de_hl_exchange(size_t i) {
    auto no_labels = [&](size_t first, size_t count) {
        if (first + count > lines_.size())
            return false;
        for (size_t k = first; k < first + count; ++k) {
            if (!lines_[k].label.empty())
                return false;
        }
        return true;
    };

    auto is_ld_exact = [](const asm_line &line,
                          const std::string &want_dst,
                          const std::string &want_src) {
        if (line.mnemonic != "ld")
            return false;
        std::string dst;
        std::string src;
        if (!split_ld(line.operands, dst, src))
            return false;
        return trim(dst) == want_dst && trim(src) == want_src;
    };

    if (i + 2 < lines_.size() && no_labels(i, 3) &&
        is_ld_exact(lines_[i], "h", "d") &&
        is_ld_exact(lines_[i + 1], "l", "e") &&
        is_ex_de_hl(lines_[i + 2])) {
        lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 2));
        return true;
    }

    if (i + 4 < lines_.size() && no_labels(i, 5) &&
        is_ld_exact(lines_[i], "b", "d") &&
        is_ld_exact(lines_[i + 1], "c", "e") &&
        is_ld_exact(lines_[i + 2], "h", "b") &&
        is_ld_exact(lines_[i + 3], "l", "c") &&
        is_ex_de_hl(lines_[i + 4])) {
        lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 4));
        return true;
    }

    if (i + 4 < lines_.size() && no_labels(i, 5) &&
        is_ld_exact(lines_[i], "h", "d") &&
        is_ld_exact(lines_[i + 1], "l", "e") &&
        is_ld_exact(lines_[i + 2], "b", "h") &&
        is_ld_exact(lines_[i + 3], "c", "l") &&
        is_ex_de_hl(lines_[i + 4])) {
        lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 4));
        return true;
    }

    if (i + 4 < lines_.size() && no_labels(i, 5) &&
        lines_[i].mnemonic == "ld" &&
        lines_[i + 1].mnemonic == "ld" &&
        lines_[i + 2].mnemonic == "ld" &&
        lines_[i + 3].mnemonic == "ld" &&
        is_ex_de_hl(lines_[i + 4])) {
        std::string dst0, src0;
        std::string dst1, src1;
        std::string dst2, src2;
        std::string dst3, src3;
        int off0 = 0;
        int off1 = 0;
        int off2 = 0;
        int off3 = 0;
        if (split_ld(lines_[i].operands, dst0, src0) &&
            split_ld(lines_[i + 1].operands, dst1, src1) &&
            split_ld(lines_[i + 2].operands, dst2, src2) &&
            split_ld(lines_[i + 3].operands, dst3, src3) &&
            parse_ix_ref(trim(dst0), off0) &&
            parse_ix_ref(trim(dst1), off1) &&
            parse_ix_ref(trim(src2), off2) &&
            parse_ix_ref(trim(src3), off3) &&
            trim(src0) == "e" &&
            trim(src1) == "d" &&
            trim(dst2) == "l" &&
            trim(dst3) == "h" &&
            off1 == off0 + 1 &&
            off2 == off0 &&
            off3 == off1) {
            lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 4));
            return true;
        }
    }

    return false;
}

bool z80_peep::rule_superopt_exchange_sandwich_de_load(size_t i) {
    if (i + 2 >= lines_.size())
        return false;

    for (size_t k = i; k <= i + 2; ++k) {
        if (!lines_[k].label.empty())
            return false;
    }

    std::string src;
    if (!is_ex_de_hl(lines_[i]) ||
        !is_ld_hl(lines_[i + 1], src) ||
        !is_ex_de_hl(lines_[i + 2])) {
        return false;
    }
    if (!is_immediate_operand(src) && !is_numeric_literal(src))
        return false;

    lines_[i] = asm_line::parse("\tld\tde, " + src);
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 1),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 3));
    return true;
}

bool z80_peep::rule_superopt_dead_pair_stack_discard_pop(size_t i) {
    if (i + 1 >= lines_.size())
        return false;

    const auto &first = lines_[i];
    const auto &second = lines_[i + 1];
    if (!first.label.empty() || !second.label.empty())
        return false;
    if (first.mnemonic != "inc" || trim(first.operands) != "sp")
        return false;
    if (second.mnemonic != "inc" || trim(second.operands) != "sp")
        return false;

    struct pair_desc {
        const char *name;
        char lo;
        char hi;
    };
    static const pair_desc pairs[] = {
        {"bc", 'c', 'b'},
        {"de", 'e', 'd'},
        {"hl", 'l', 'h'},
    };

    for (const auto &pair : pairs) {
        if (!path_overwrites_pair_before_read(lines_, i + 2,
                                              pair.name, pair.lo, pair.hi)) {
            continue;
        }
        lines_[i] = asm_line::parse("\tpop\t" + std::string(pair.name));
        lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 1));
        return true;
    }

    return false;
}

bool z80_peep::rule_superopt_dead_pair_pop_push(size_t i) {
    if (i + 1 >= lines_.size())
        return false;

    const auto &pop = lines_[i];
    const auto &push = lines_[i + 1];
    if (!pop.label.empty() || !push.label.empty())
        return false;
    if (pop.mnemonic != "pop" || push.mnemonic != "push")
        return false;

    struct pair_desc {
        const char *name;
        char lo;
        char hi;
    };
    static const pair_desc pairs[] = {
        {"hl", 'l', 'h'},
        {"de", 'e', 'd'},
        {"bc", 'c', 'b'},
    };

    const std::string popped = trim(pop.operands);
    if (popped != trim(push.operands))
        return false;

    for (const auto &pair : pairs) {
        if (popped != pair.name)
            continue;
        if (!path_overwrites_pair_before_read(lines_, i + 2,
                                              pair.name, pair.lo, pair.hi)) {
            return false;
        }
        lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                     lines_.begin() + static_cast<std::ptrdiff_t>(i + 2));
        return true;
    }

    return false;
}

bool z80_peep::rule_superopt_long_inc_sp_run(size_t i) {
    if (i >= lines_.size())
        return false;

    size_t run = 0;
    while (i + run < lines_.size()) {
        const auto &line = lines_[i + run];
        if (!line.label.empty() ||
            line.mnemonic != "inc" ||
            trim(line.operands) != "sp") {
            break;
        }
        ++run;
    }

    if (run < 6)
        return false;

    const size_t after = i + run;
    if (!path_overwrites_hl_before_read(lines_, after))
        return false;
    if (!flags_overwritten_before_read_or_escape(lines_, after))
        return false;

    std::vector<asm_line> repl;
    repl.push_back(asm_line::parse("\tld\thl,#" + std::to_string(run)));
    repl.push_back(asm_line::parse("\tadd\thl,sp"));
    repl.push_back(asm_line::parse("\tld\tsp,hl"));

    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                 lines_.begin() + static_cast<std::ptrdiff_t>(after));
    lines_.insert(lines_.begin() + static_cast<std::ptrdiff_t>(i),
                  repl.begin(), repl.end());
    return true;
}

bool z80_peep::rule_superopt_spaghetti_load_helper_preserve_af_bc(size_t i) {
    if (i + 7 >= lines_.size())
        return false;

    const std::string helper = lines_[i].label;
    if (helper.rfind("__xopt_spaghetti_", 0) != 0 ||
        !lines_[i].mnemonic.empty()) {
        return false;
    }

    auto is_ld_exact = [](const asm_line &line,
                          const std::string &want_dst,
                          const std::string &want_src) {
        if (line.mnemonic != "ld")
            return false;
        std::string dst;
        std::string src;
        if (!split_ld(line.operands, dst, src))
            return false;
        return trim(dst) == want_dst && trim(src) == want_src;
    };

    if (lines_[i + 1].mnemonic != "add" ||
        trim(lines_[i + 1].operands) != "hl, bc") {
        return false;
    }
    if (!is_ld_exact(lines_[i + 2], "c", "(hl)") ||
        lines_[i + 3].mnemonic != "inc" ||
        trim(lines_[i + 3].operands) != "hl" ||
        !is_ld_exact(lines_[i + 4], "b", "(hl)") ||
        !is_ld_exact(lines_[i + 5], "l", "c") ||
        !is_ld_exact(lines_[i + 6], "h", "b") ||
        lines_[i + 7].mnemonic != "ret" ||
        !trim(lines_[i + 7].operands).empty()) {
        return false;
    }
    for (size_t k = i + 1; k <= i + 7; ++k) {
        if (!lines_[k].label.empty())
            return false;
    }

    std::vector<size_t> calls;
    for (size_t k = 0; k < lines_.size(); ++k) {
        if (k >= i && k <= i + 7)
            continue;

        const auto &line = lines_[k];
        if (trim(line.operands) != helper)
            continue;

        if (line.mnemonic != "call")
            return false;
        if (k < 5 || k + 2 >= lines_.size())
            return false;

        for (size_t w = k - 5; w <= k + 2; ++w) {
            if (!lines_[w].label.empty())
                return false;
        }
        if (lines_[k - 5].mnemonic != "push" ||
            trim(lines_[k - 5].operands) != "af" ||
            lines_[k - 4].mnemonic != "push" ||
            trim(lines_[k - 4].operands) != "bc" ||
            lines_[k - 3].mnemonic != "push" ||
            trim(lines_[k - 3].operands) != "ix" ||
            lines_[k - 2].mnemonic != "pop" ||
            trim(lines_[k - 2].operands) != "hl" ||
            lines_[k - 1].mnemonic != "ld" ||
            lines_[k + 1].mnemonic != "pop" ||
            trim(lines_[k + 1].operands) != "bc" ||
            lines_[k + 2].mnemonic != "pop" ||
            trim(lines_[k + 2].operands) != "af") {
            return false;
        }
        std::string dst;
        std::string src;
        if (!split_ld(lines_[k - 1].operands, dst, src) ||
            trim(dst) != "bc") {
            return false;
        }
        calls.push_back(k);
    }

    if (calls.size() < 2)
        return false;

    std::sort(calls.begin(), calls.end(), std::greater<size_t>());
    for (size_t call_idx : calls) {
        lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(call_idx + 1),
                     lines_.begin() + static_cast<std::ptrdiff_t>(call_idx + 3));
        lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(call_idx - 5),
                     lines_.begin() + static_cast<std::ptrdiff_t>(call_idx - 3));
    }

    const size_t helper_idx = find_label_index(lines_, helper);
    if (helper_idx == lines_.size() || helper_idx + 7 >= lines_.size())
        return false;

    lines_.insert(lines_.begin() + static_cast<std::ptrdiff_t>(helper_idx + 1),
                  asm_line::parse("\tpush\taf"));
    lines_.insert(lines_.begin() + static_cast<std::ptrdiff_t>(helper_idx + 2),
                  asm_line::parse("\tpush\tbc"));
    lines_.insert(lines_.begin() + static_cast<std::ptrdiff_t>(helper_idx + 9),
                  asm_line::parse("\tpop\tbc"));
    lines_.insert(lines_.begin() + static_cast<std::ptrdiff_t>(helper_idx + 10),
                  asm_line::parse("\tpop\taf"));
    return true;
}

bool z80_peep::rule_superopt_spaghetti_store_helper_preserve_regs(size_t i) {
    if (i + 6 >= lines_.size())
        return false;

    const std::string helper = lines_[i].label;
    if (helper.rfind("__xopt_spaghetti_", 0) != 0 ||
        !lines_[i].mnemonic.empty()) {
        return false;
    }

    auto is_ld_exact = [](const asm_line &line,
                          const std::string &want_dst,
                          const std::string &want_src) {
        if (line.mnemonic != "ld")
            return false;
        std::string dst;
        std::string src;
        if (!split_ld(line.operands, dst, src))
            return false;
        return trim(dst) == want_dst && trim(src) == want_src;
    };

    std::string dst;
    std::string src;
    if (lines_[i + 1].mnemonic != "ld" ||
        !split_ld(lines_[i + 1].operands, dst, src) ||
        trim(dst) != "bc") {
        return false;
    }
    const std::string offset = trim(src);
    if (lines_[i + 2].mnemonic != "add" ||
        trim(lines_[i + 2].operands) != "hl, bc" ||
        !is_ld_exact(lines_[i + 3], "(hl)", "e") ||
        lines_[i + 4].mnemonic != "inc" ||
        trim(lines_[i + 4].operands) != "hl" ||
        !is_ld_exact(lines_[i + 5], "(hl)", "d") ||
        lines_[i + 6].mnemonic != "ret" ||
        !trim(lines_[i + 6].operands).empty()) {
        return false;
    }
    for (size_t k = i + 1; k <= i + 6; ++k) {
        if (!lines_[k].label.empty())
            return false;
    }

    std::vector<size_t> calls;
    for (size_t k = 0; k < lines_.size(); ++k) {
        if (k >= i && k <= i + 6)
            continue;

        const auto &line = lines_[k];
        if (trim(line.operands) != helper)
            continue;

        if (line.mnemonic != "call")
            return false;
        if (k < 7 || k + 3 >= lines_.size())
            return false;

        for (size_t w = k - 7; w <= k + 3; ++w) {
            if (!lines_[w].label.empty())
                return false;
        }
        if (lines_[k - 7].mnemonic != "push" ||
            trim(lines_[k - 7].operands) != "af" ||
            lines_[k - 6].mnemonic != "push" ||
            trim(lines_[k - 6].operands) != "bc" ||
            lines_[k - 5].mnemonic != "push" ||
            trim(lines_[k - 5].operands) != "de" ||
            !is_ld_exact(lines_[k - 4], "d", "h") ||
            !is_ld_exact(lines_[k - 3], "e", "l") ||
            lines_[k - 2].mnemonic != "push" ||
            trim(lines_[k - 2].operands) != "ix" ||
            lines_[k - 1].mnemonic != "pop" ||
            trim(lines_[k - 1].operands) != "hl" ||
            lines_[k + 1].mnemonic != "pop" ||
            trim(lines_[k + 1].operands) != "de" ||
            lines_[k + 2].mnemonic != "pop" ||
            trim(lines_[k + 2].operands) != "bc" ||
            lines_[k + 3].mnemonic != "pop" ||
            trim(lines_[k + 3].operands) != "af") {
            return false;
        }
        calls.push_back(k);
    }

    if (calls.size() < 2)
        return false;

    std::sort(calls.begin(), calls.end(), std::greater<size_t>());
    for (size_t call_idx : calls) {
        lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(call_idx + 1),
                     lines_.begin() + static_cast<std::ptrdiff_t>(call_idx + 4));
        lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(call_idx - 7),
                     lines_.begin() + static_cast<std::ptrdiff_t>(call_idx));
    }

    const size_t helper_idx = find_label_index(lines_, helper);
    if (helper_idx == lines_.size() || helper_idx + 6 >= lines_.size())
        return false;

    std::vector<asm_line> prologue;
    prologue.push_back(asm_line::parse("\tpush\taf"));
    prologue.push_back(asm_line::parse("\tpush\tbc"));
    prologue.push_back(asm_line::parse("\tpush\tde"));
    prologue.push_back(asm_line::parse("\tld\td, h"));
    prologue.push_back(asm_line::parse("\tld\te, l"));
    prologue.push_back(asm_line::parse("\tpush\tix"));
    prologue.push_back(asm_line::parse("\tpop\thl"));
    lines_.insert(lines_.begin() + static_cast<std::ptrdiff_t>(helper_idx + 1),
                  prologue.begin(), prologue.end());

    const size_t ret_idx = helper_idx + 13;
    lines_.insert(lines_.begin() + static_cast<std::ptrdiff_t>(ret_idx),
                  asm_line::parse("\tpop\tde"));
    lines_.insert(lines_.begin() + static_cast<std::ptrdiff_t>(ret_idx + 1),
                  asm_line::parse("\tpop\tbc"));
    lines_.insert(lines_.begin() + static_cast<std::ptrdiff_t>(ret_idx + 2),
                  asm_line::parse("\tpop\taf"));

    (void)offset;
    return true;
}

bool z80_peep::rule_superopt_spaghetti_flag_helper_inline(size_t i) {
    const std::string helper = lines_[i].label;
    if (helper.rfind("__xopt_spaghetti_", 0) != 0 ||
        !lines_[i].mnemonic.empty()) {
        return false;
    }

    static const char *const word_from_hl_body[] = {
        "ld\te, (hl)",
        "inc\thl",
        "ld\td, (hl)",
        "ld\tb, d",
        "ld\tc, e",
        "ld\th, b",
        "ld\tl, c",
        "ld\ta, h",
        "or\ta, l",
        "ret",
    };
    static const char *const de_flag_body[] = {
        "ld\th, d",
        "ld\tl, e",
        "ld\tb, h",
        "ld\tc, l",
        "ld\ta, h",
        "or\ta, l",
        "ret",
    };

    auto body_matches = [&](const char *const *body, size_t count) {
        if (i + count >= lines_.size())
            return false;
        for (size_t k = 0; k < count; ++k) {
            const auto &line = lines_[i + 1 + k];
            if (!line.label.empty())
                return false;
            std::string actual = line.mnemonic;
            const std::string ops = trim(line.operands);
            if (!ops.empty())
                actual += "\t" + ops;
            if (actual != body[k])
                return false;
        }
        return true;
    };

    enum class flag_helper_kind {
        word_from_hl,
        de_value,
    };

    flag_helper_kind kind;
    size_t body_count = 0;
    std::vector<asm_line> replacement;
    if (body_matches(word_from_hl_body,
                     sizeof(word_from_hl_body) / sizeof(word_from_hl_body[0]))) {
        kind = flag_helper_kind::word_from_hl;
        body_count = sizeof(word_from_hl_body) / sizeof(word_from_hl_body[0]);
        replacement.push_back(asm_line::parse("\tld\ta, (hl)"));
        replacement.push_back(asm_line::parse("\tinc\thl"));
        replacement.push_back(asm_line::parse("\tor\ta, (hl)"));
    } else if (body_matches(de_flag_body,
                            sizeof(de_flag_body) / sizeof(de_flag_body[0]))) {
        kind = flag_helper_kind::de_value;
        body_count = sizeof(de_flag_body) / sizeof(de_flag_body[0]);
        replacement.push_back(asm_line::parse("\tld\ta, d"));
        replacement.push_back(asm_line::parse("\tor\ta, e"));
    } else {
        return false;
    }

    std::vector<size_t> calls;
    for (size_t k = 0; k < lines_.size(); ++k) {
        if (k >= i && k <= i + body_count)
            continue;

        const auto &line = lines_[k];
        if (trim(line.operands) != helper)
            continue;
        if (line.mnemonic != "call")
            return false;
        if (k + 1 >= lines_.size())
            return false;

        std::string cc;
        std::string target;
        if (!split_conditional_branch_target(lines_[k + 1], cc, target))
            return false;

        const bool modern_de_return = in_sdcccall1_function(lines_, k);
        const bool hl_dead =
            path_overwrites_hl_before_read(lines_, k + 1) ||
            (modern_de_return &&
             (hl_dead_before_read_or_modern_return(lines_, k + 1) ||
              pair_dead_before_read_allowing_spaghetti(
                  lines_, k + 1, "hl", 'l', 'h', true)));
        if (!hl_dead)
            return false;
        const bool bc_dead =
            path_overwrites_pair_before_read(lines_, k + 1, "bc", 'c', 'b') ||
            (modern_de_return &&
             pair_dead_before_read_allowing_spaghetti(
                 lines_, k + 1, "bc", 'c', 'b', true));
        if (!bc_dead)
            return false;
        if (kind == flag_helper_kind::word_from_hl &&
            !path_overwrites_pair_before_read(lines_, k + 1, "de", 'e', 'd') &&
            !(modern_de_return &&
              pair_dead_before_read_allowing_spaghetti(
                  lines_, k + 1, "de", 'e', 'd', false))) {
            return false;
        }

        calls.push_back(k);
    }

    if (calls.empty())
        return false;

    std::sort(calls.begin(), calls.end(), std::greater<size_t>());
    for (size_t call_idx : calls) {
        std::vector<asm_line> repl = replacement;
        repl.front().label = lines_[call_idx].label;
        repl.front().is_label = !repl.front().label.empty();
        repl.front().is_global_label = lines_[call_idx].is_global_label;
        repl.front().comment = lines_[call_idx].comment;
        lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(call_idx));
        lines_.insert(lines_.begin() + static_cast<std::ptrdiff_t>(call_idx),
                      repl.begin(), repl.end());
    }

    const size_t helper_idx = find_label_index(lines_, helper);
    if (helper_idx == lines_.size() || helper_idx + body_count >= lines_.size())
        return false;
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(helper_idx),
                 lines_.begin() + static_cast<std::ptrdiff_t>(
                     helper_idx + body_count + 1));
    return true;
}

bool z80_peep::rule_push_pop_same_reg_span(size_t i) {
    if (i + 2 >= lines_.size())
        return false;

    struct pair_desc {
        const char *name;
        char lo;
        char hi;
    };
    static const pair_desc pairs[] = {
        {"hl", 'l', 'h'},
        {"de", 'e', 'd'},
        {"bc", 'c', 'b'},
    };

    const asm_line &push = lines_[i];
    if (push.mnemonic != "push")
        return false;
    const std::string pushed = trim(push.operands);

    for (const auto &pair : pairs) {
        if (pushed != pair.name)
            continue;
        for (size_t pop_idx = i + 2;
             pop_idx < lines_.size() && pop_idx <= i + 4;
             ++pop_idx) {
            const asm_line &pop = lines_[pop_idx];
            if (pop.mnemonic != "pop" || trim(pop.operands) != pair.name)
                continue;
            bool safe = true;
            for (size_t k = i + 1; k < pop_idx; ++k) {
                if (!line_preserves_pair_and_sp(lines_[k], pair.name,
                                                pair.lo, pair.hi)) {
                    safe = false;
                    break;
                }
            }
            if (!safe)
                continue;
            lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(pop_idx));
            lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i));
            return true;
        }
    }
    return false;
}

bool z80_peep::rule_push_pair_copy_span(size_t i) {
    if (i + 2 >= lines_.size())
        return false;

    struct copy_desc {
        const char *src;
        char src_lo;
        char src_hi;
        const char *dst;
        char dst_lo;
        char dst_hi;
    };
    static const copy_desc copies[] = {
        {"hl", 'l', 'h', "bc", 'c', 'b'},
        {"bc", 'c', 'b', "hl", 'l', 'h'},
        {"bc", 'c', 'b', "de", 'e', 'd'},
        {"de", 'e', 'd', "bc", 'c', 'b'},
    };

    const asm_line &push = lines_[i];
    if (push.mnemonic != "push")
        return false;
    const std::string pushed = trim(push.operands);

    for (const auto &copy : copies) {
        if (pushed != copy.src)
            continue;
        for (size_t pop_idx = i + 2;
             pop_idx < lines_.size() && pop_idx <= i + 4;
             ++pop_idx) {
            const asm_line &pop = lines_[pop_idx];
            if (pop.mnemonic != "pop" || trim(pop.operands) != copy.dst)
                continue;
            bool safe = true;
            for (size_t k = i + 1; k < pop_idx; ++k) {
                if (!line_preserves_pair_and_sp(lines_[k], copy.src, copy.src_lo, copy.src_hi) ||
                    !line_preserves_pair_and_sp(lines_[k], copy.dst, copy.dst_lo, copy.dst_hi)) {
                    safe = false;
                    break;
                }
            }
            if (!safe)
                continue;

            lines_[i] = asm_line::parse(
                "\tld\t" + std::string(1, copy.dst_hi) + ", " +
                std::string(1, copy.src_hi));
            lines_.insert(lines_.begin() + static_cast<std::ptrdiff_t>(i + 1),
                          asm_line::parse(
                              "\tld\t" + std::string(1, copy.dst_lo) + ", " +
                              std::string(1, copy.src_lo)));
            ++pop_idx;
            lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(pop_idx));
            return true;
        }
    }
    return false;
}

// ld l,N(ix); ld h,N+1(ix); ld l,M(ix); ld h,M+1(ix)  →  last 2 lines only.
// The first 16-bit IX load is immediately overwritten by the second — dead load.
bool z80_peep::rule_dead_hl_ix_load(size_t i) {
    if (i + 3 >= lines_.size()) return false;
    auto &l0 = lines_[i];
    auto &l1 = lines_[i + 1];
    auto &l2 = lines_[i + 2];
    auto &l3 = lines_[i + 3];

    if (!l1.label.empty() || !l2.label.empty() || !l3.label.empty()) return false;

    if (l0.mnemonic != "ld" || l1.mnemonic != "ld" ||
        l2.mnemonic != "ld" || l3.mnemonic != "ld") return false;

    std::string d0, s0, d1, s1, d2, s2, d3, s3;
    if (!split_ld(l0.operands, d0, s0) || d0 != "l") return false;
    int off0_lo; if (!parse_ix_ref(s0, off0_lo)) return false;

    if (!split_ld(l1.operands, d1, s1) || d1 != "h") return false;
    int off0_hi; if (!parse_ix_ref(s1, off0_hi) || off0_hi != off0_lo + 1) return false;

    if (!split_ld(l2.operands, d2, s2) || d2 != "l") return false;
    int off1_lo; if (!parse_ix_ref(s2, off1_lo) || off1_lo == off0_lo) return false;

    if (!split_ld(l3.operands, d3, s3) || d3 != "h") return false;
    int off1_hi; if (!parse_ix_ref(s3, off1_hi) || off1_hi != off1_lo + 1) return false;

    lines_.erase(lines_.begin() + i, lines_.begin() + i + 2);
    return true;
}

// jr/jp cc1,L_true; ld hl,#A; jr/jp L_end; L_true: ld hl,#B; L_end: ld a,h; or a,l; jr/jp cc2,tgt
// →  jr/jp combined_cc, tgt
//
// The boolean generation pattern (branch to 0/1, then IFX test) is collapsed
// into a single direct branch using the original comparison condition.
// Combined condition logic:
//   true_val=1 means cc1 leads to HL=1; cc2=nz means "branch when true"
//   combined = cc1 if branch_when_true == cc1_leads_to_true, else !cc1
bool z80_peep::rule_bool_ifx_shortcircuit(size_t i) {
    if (i + 8 >= lines_.size()) return false;

    // Reject if any non-label position carries a label (branch target)
    for (int k = 0; k <= 8; ++k) {
        if (k == 3 || k == 5) continue;  // expected labels at these slots
        if (!lines_[i + k].label.empty()) return false;
    }

    // [i]: jr/jp cc1, L_true  (conditional jump — condition means "true")
    auto &jcc = lines_[i];
    if (jcc.mnemonic != "jr" && jcc.mnemonic != "jp") return false;
    std::string jcc_ops = trim(jcc.operands);
    size_t c0 = jcc_ops.find(',');
    if (c0 == std::string::npos) return false;
    std::string cc1    = trim(jcc_ops.substr(0, c0));
    std::string L_true = trim(jcc_ops.substr(c0 + 1));
    if (L_true.empty() || L_true[0] == '(') return false;

    // [i+1]: ld hl, #0 or #1  (false value)
    auto &false_load = lines_[i + 1];
    if (false_load.mnemonic != "ld") return false;
    int false_val;
    { std::string d, s; if (!split_ld(false_load.operands, d, s)) return false;
      if (d != "hl") return false;
      if (s == "#0" || s == "0") false_val = 0;
      else if (s == "#1" || s == "1") false_val = 1;
      else return false; }

    // [i+2]: jr/jp L_end  (unconditional skip)
    auto &jend = lines_[i + 2];
    if (jend.mnemonic != "jr" && jend.mnemonic != "jp") return false;
    { std::string ops = trim(jend.operands);
      if (ops.find(',') != std::string::npos) return false; }
    std::string L_end = trim(jend.operands);

    // [i+3]: L_true: (label-only line)
    if (lines_[i + 3].label != L_true || lines_[i + 3].mnemonic != "") return false;

    // [i+4]: ld hl, #1 or #0  (true value — complement of false_val)
    auto &true_load = lines_[i + 4];
    if (true_load.mnemonic != "ld") return false;
    int true_val;
    { std::string d, s; if (!split_ld(true_load.operands, d, s)) return false;
      if (d != "hl") return false;
      if (s == "#0" || s == "0") true_val = 0;
      else if (s == "#1" || s == "1") true_val = 1;
      else return false; }
    if (true_val == false_val) return false;

    // [i+5]: L_end: (label-only line)
    if (lines_[i + 5].label != L_end || lines_[i + 5].mnemonic != "") return false;

    // [i+6]: ld a, h
    { auto &l = lines_[i + 6];
      if (l.mnemonic != "ld") return false;
      std::string d, s; if (!split_ld(l.operands, d, s)) return false;
      if (d != "a" || s != "h") return false; }

    // [i+7]: or a, l
    { auto &l = lines_[i + 7];
      if (l.mnemonic != "or") return false;
      std::string ops = trim(l.operands);
      if (ops != "a, l" && ops != "a,l") return false; }

    // [i+8]: jr/jp cc2, target
    auto &jifx = lines_[i + 8];
    if (jifx.mnemonic != "jr" && jifx.mnemonic != "jp") return false;
    std::string jifx_ops = trim(jifx.operands);
    size_t c2 = jifx_ops.find(',');
    if (c2 == std::string::npos) return false;
    std::string cc2    = trim(jifx_ops.substr(0, c2));
    std::string target = trim(jifx_ops.substr(c2 + 1));
    if (cc2 != "z" && cc2 != "nz") return false;  // IFX only uses z/nz

    // Ensure L_true and L_end are not referenced outside this pattern
    for (size_t j = 0; j < lines_.size(); ++j) {
        if (j >= i && j <= i + 8) continue;
        const std::string &ops = lines_[j].operands;
        if (ops.find(L_true) != std::string::npos) return false;
        if (ops.find(L_end)  != std::string::npos) return false;
    }

    // Determine combined condition.
    // cc1 leads to true path (true_val=1).
    // cc2=nz branches when IFX result is nonzero (true).
    auto invert_cc = [](const std::string &cc) -> std::string {
        if (cc == "z")  return "nz";
        if (cc == "nz") return "z";
        if (cc == "c")  return "nc";
        if (cc == "nc") return "c";
        if (cc == "m")  return "p";
        if (cc == "p")  return "m";
        return "";
    };

    bool branch_when_true = (cc2 == "nz");
    bool cc1_true_path    = (true_val == 1);
    std::string combined_cc = (branch_when_true == cc1_true_path) ? cc1 : invert_cc(cc1);
    if (combined_cc.empty()) return false;

    // Always use jp: jr has ±127 byte range which may not hold for large functions.
    std::string new_mnem = "jp";

    lines_[i].mnemonic = new_mnem;
    lines_[i].operands = combined_cc + ", " + target;
    lines_.erase(lines_.begin() + i + 1, lines_.begin() + i + 9);
    return true;
}

// ld l,A(ix); ld h,A+1(ix); ex de,hl; ld l,B(ix); ld h,B+1(ix); ex de,hl
// →  ld e,B(ix); ld d,B+1(ix); ld l,A(ix); ld h,A+1(ix)
//
// Two consecutive IX-relative 16-bit loads with ex de,hl sandwiched produce
// HL=A-value, DE=B-value.  Direct loading achieves the same without touching
// the stack or swapping registers.  Safe because IX frame loads are side-effect-free.
bool z80_peep::rule_ex_de_hl_load_double(size_t i) {
    if (i + 5 >= lines_.size()) return false;
    for (int k = 1; k <= 5; ++k)
        if (!lines_[i + k].label.empty()) return false;

    auto &l0 = lines_[i];
    auto &l1 = lines_[i + 1];
    auto &l2 = lines_[i + 2];
    auto &l3 = lines_[i + 3];
    auto &l4 = lines_[i + 4];
    auto &l5 = lines_[i + 5];

    if (l0.mnemonic != "ld" || l1.mnemonic != "ld") return false;
    if (l2.mnemonic != "ex") return false;
    if (l3.mnemonic != "ld" || l4.mnemonic != "ld") return false;
    if (l5.mnemonic != "ex") return false;

    // l0: ld l, A(ix)
    std::string d0, s0; if (!split_ld(l0.operands, d0, s0) || d0 != "l") return false;
    int off_a_lo; if (!parse_ix_ref(s0, off_a_lo)) return false;

    // l1: ld h, A+1(ix)
    std::string d1, s1; if (!split_ld(l1.operands, d1, s1) || d1 != "h") return false;
    int off_a_hi; if (!parse_ix_ref(s1, off_a_hi) || off_a_hi != off_a_lo + 1) return false;

    // l2 and l5: both must be "ex de, hl"
    auto is_ex_de_hl = [](const asm_line &l) {
        std::string ops = trim(l.operands);
        return ops == "de, hl" || ops == "de,hl";
    };
    if (!is_ex_de_hl(l2) || !is_ex_de_hl(l5)) return false;

    // l3: ld l, B(ix)
    std::string d3, s3; if (!split_ld(l3.operands, d3, s3) || d3 != "l") return false;
    int off_b_lo; if (!parse_ix_ref(s3, off_b_lo)) return false;

    // l4: ld h, B+1(ix)
    std::string d4, s4; if (!split_ld(l4.operands, d4, s4) || d4 != "h") return false;
    int off_b_hi; if (!parse_ix_ref(s4, off_b_hi) || off_b_hi != off_b_lo + 1) return false;

    // Replace 6 lines with 4: ld e,B(ix); ld d,B+1(ix); ld l,A(ix); ld h,A+1(ix)
    std::string blo = std::to_string(off_b_lo) + " (ix)";
    std::string bhi = std::to_string(off_b_hi) + " (ix)";
    std::string alo = std::to_string(off_a_lo) + " (ix)";
    std::string ahi = std::to_string(off_a_hi) + " (ix)";

    lines_[i + 0] = asm_line::parse("\tld\te, " + blo);
    lines_[i + 1] = asm_line::parse("\tld\td, " + bhi);
    lines_[i + 2] = asm_line::parse("\tld\tl, " + alo);
    lines_[i + 3] = asm_line::parse("\tld\th, " + ahi);
    lines_.erase(lines_.begin() + i + 4, lines_.begin() + i + 6);
    return true;
}

// ld N(ix),a; ld a,N(ix)  →  ld N(ix),a
// Store byte to IX slot then immediately reload it — A is unchanged.
bool z80_peep::rule_ix_byte_store_reload(size_t i) {
    if (i + 1 >= lines_.size()) return false;
    auto &a = lines_[i];
    auto &b = lines_[i + 1];
    if (!b.label.empty()) return false;
    if (a.mnemonic != "ld" || b.mnemonic != "ld") return false;
    std::string a_dst, a_src, b_dst, b_src;
    if (!split_ld(a.operands, a_dst, a_src)) return false;
    if (!split_ld(b.operands, b_dst, b_src)) return false;
    if (a_src != "a" || b_dst != "a") return false;
    int off_a, off_b;
    if (!parse_ix_ref(a_dst, off_a)) return false;
    if (!parse_ix_ref(b_src, off_b)) return false;
    if (off_a != off_b) return false;
    lines_.erase(lines_.begin() + i + 1);
    return true;
}

// ld N(ix),a; ld r,N(ix)  →  ld N(ix),a; ld r,a
// Store byte to IX slot then immediately reload into another byte register.
bool z80_peep::rule_ix_byte_store_forward(size_t i) {
    if (i + 1 >= lines_.size()) return false;
    auto &a = lines_[i];
    auto &b = lines_[i + 1];
    if (!b.label.empty()) return false;
    if (a.mnemonic != "ld" || b.mnemonic != "ld") return false;

    std::string a_dst, a_src, b_dst, b_src;
    if (!split_ld(a.operands, a_dst, a_src)) return false;
    if (!split_ld(b.operands, b_dst, b_src)) return false;
    if (a_src != "a" || !is_reg8(b_dst) || b_dst == "a") return false;

    int off_a, off_b;
    if (!parse_ix_ref(a_dst, off_a)) return false;
    if (!parse_ix_ref(b_src, off_b)) return false;
    if (off_a != off_b) return false;

    b.operands = b_dst + ", a";
    return true;
}

bool z80_peep::rule_dead_temp_ix_store(size_t i) {
    if (i >= lines_.size()) return false;
    auto &line = lines_[i];
    if (!line.label.empty() || line.mnemonic != "ld")
        return false;

    std::string dst, src;
    if (!split_ld(line.operands, dst, src))
        return false;

    int offset = 0;
    if (!parse_ix_ref(dst, offset))
        return false;

    int locals = 0;
    int temp_frame = 0;
    size_t prologue_index = 0;
    if (!current_function_frame(lines_, i, locals, temp_frame, prologue_index))
        return false;
    if (!ix_offset_in_temp_frame(offset, locals, temp_frame))
        return false;

    src = trim(src);
    if (!is_reg8(src) && !is_immediate_operand(src))
        return false;

    const size_t function_end = function_end_after_prologue(lines_,
                                                            prologue_index);

    // A compiler temp is frequently reused for the value loaded through an
    // address that was just spilled to that same slot.  Prove the first store
    // dead within the basic block before falling back to the more conservative
    // whole-function check below.  Stopping at every possible control-flow
    // boundary keeps this valid even when the slot is live around a loop.
    for (size_t k = i + 1; k < function_end; ++k) {
        const asm_line &next = lines_[k];
        if (!next.label.empty() || is_section_directive(next))
            break;
        if (line_reads_ix_offset(next, offset))
            break;
        if (line_writes_ix_offset(next, offset)) {
            lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i));
            return true;
        }
        if (next.mnemonic == "ret" || next.mnemonic == "reti" ||
            next.mnemonic == "retn" || next.mnemonic == "rst" ||
            next.mnemonic == "jp" || next.mnemonic == "jr" ||
            next.mnemonic == "djnz") {
            break;
        }
    }

    if (ix_value_may_be_read_before_rewrite(lines_, prologue_index,
                                            function_end, i + 1, offset))
        return false;

    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i));
    return true;
}

// jr/jp cc,L; jr/jp L_end; L:  →  jr/jp !cc,L_end; L:
// Only fires when L is referenced by exactly one instruction (line i).
bool z80_peep::rule_invert_branch_skip(size_t i) {
    if (i + 2 >= lines_.size()) return false;

    auto &l0 = lines_[i];
    auto &l1 = lines_[i + 1];
    auto &l2 = lines_[i + 2];

    // l0: conditional branch  jp/jr cc, L  (no label on this line)
    if (!l0.label.empty()) return false;
    if (l0.mnemonic != "jp" && l0.mnemonic != "jr") return false;
    std::string ops0 = trim(l0.operands);
    // Split "cc, L" into condition and target.
    static const char *const ccs[] = {
        "nz,", "z,", "nc,", "c,", "m,", "p,", "pe,", "po,", nullptr
    };
    std::string cc0, tgt_l;
    for (int k = 0; ccs[k]; ++k) {
        size_t len = std::strlen(ccs[k]);
        if (ops0.size() > len && ops0.substr(0, len) == ccs[k]) {
            cc0   = ops0.substr(0, len - 1); // strip trailing comma
            tgt_l = trim(ops0.substr(len));
            break;
        }
    }
    if (cc0.empty()) return false; // not a conditional branch

    // l1: unconditional jump  jp/jr L_end  (no label, no condition)
    if (!l1.label.empty()) return false;
    if (l1.mnemonic != "jp" && l1.mnemonic != "jr") return false;
    std::string tgt_end = trim(l1.operands);
    // tgt_end must not contain a comma (would mean it's conditional)
    if (tgt_end.find(',') != std::string::npos) return false;
    if (tgt_end.empty()) return false;

    // l2: label-only line whose label == tgt_l
    if (l2.label != tgt_l) return false;
    if (!l2.mnemonic.empty()) {
        // Accept label-only lines that may also carry a mnemonic (e.g. real
        // label followed by an instruction on the same line), but only fire
        // if the line is purely a label.
        return false;
    }

    // Verify tgt_l is not referenced anywhere outside line i.
    for (size_t j = 0; j < lines_.size(); ++j) {
        if (j == i) continue;
        const auto &lj = lines_[j];
        // Check operands for tgt_l as a jump target.
        if (lj.mnemonic == "jp" || lj.mnemonic == "jr" ||
            lj.mnemonic == "call" || lj.mnemonic == "djnz") {
            std::string ops = trim(lj.operands);
            // Strip condition prefix if present.
            size_t comma = ops.find(',');
            std::string jdst = (comma != std::string::npos)
                                ? trim(ops.substr(comma + 1))
                                : ops;
            if (jdst == tgt_l) return false;
        }
    }

    // Invert the condition.
    static const std::pair<const char *, const char *> inv_table[] = {
        {"z",  "nz"}, {"nz", "z"},
        {"c",  "nc"}, {"nc", "c"},
        {"m",  "p"},  {"p",  "m"},
        {"pe", "po"}, {"po", "pe"},
    };
    std::string inv_cc;
    for (auto &[from, to] : inv_table) {
        if (cc0 == from) { inv_cc = to; break; }
    }
    if (inv_cc.empty()) return false;

    // Always use jp: jr has ±127 byte range which may not hold for large functions.
    std::string new_mnem = "jp";

    // Rewrite l0 → inverted branch to L_end; erase l1; keep l2.
    lines_[i] = asm_line::parse("\t" + new_mnem + "\t" + inv_cc + ", " + tgt_end);
    lines_.erase(lines_.begin() + i + 1);
    return true;
}

bool z80_peep::is_conditional_branch(const asm_line &l) {
    if (l.mnemonic != "jp"   && l.mnemonic != "jr" &&
        l.mnemonic != "call" && l.mnemonic != "ret") return false;
    static const char *const ccs[] = {
        "z,", "nz,", "c,", "nc,", "m,", "p,", "pe,", "po,",
        "z", "nz", "c", "nc", "m", "p", "pe", "po", nullptr
    };
    std::string ops = trim(l.operands);
    for (int k = 0; ccs[k]; ++k) {
        size_t len = std::strlen(ccs[k]);
        if (ops.size() >= len && ops.substr(0, len) == ccs[k]) return true;
    }
    return false;
}

} // namespace xopt
