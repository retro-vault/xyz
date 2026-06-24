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

static bool is_code_section_directive(const asm_line &line,
                                      std::string &section_name) {
    if (line.mnemonic == ".text") {
        section_name = "";
        return true;
    }
    if (line.mnemonic != ".area" && line.mnemonic != ".section")
        return false;

    const std::string ops = trim(line.operands);
    const std::string lower = lower_copy(ops);
    if (lower.find("code") == std::string::npos &&
        lower.find("text") == std::string::npos) {
        return false;
    }
    section_name = ops.empty() ? "_CODE" : ops;
    return true;
}

static bool is_data_section_directive(const asm_line &line) {
    if (line.mnemonic == ".data")
        return true;
    if (line.mnemonic != ".area" && line.mnemonic != ".section")
        return false;
    const std::string lower = lower_copy(trim(line.operands));
    return lower.find("data") != std::string::npos ||
           lower.find("bss") != std::string::npos;
}

static bool is_spaghetti_helper_label(const asm_line &line) {
    return line.label.rfind("__xopt_spaghetti_", 0) == 0;
}

static bool spaghetti_candidate_instruction(const asm_line &line) {
    if (!line.label.empty() || line.mnemonic.empty() || line.is_comment)
        return false;
    if (is_section_directive(line) || line.mnemonic[0] == '.')
        return false;

    const std::string &m = line.mnemonic;
    if (m == "call" || m == "ret" || m == "reti" || m == "retn" ||
        m == "rst" || m == "jp" || m == "jr" || m == "djnz" ||
        m == "push" || m == "pop" || m == "halt" || m == "di" ||
        m == "ei" || m == "exx" || m == "ldi" || m == "ldir" ||
        m == "ldd" || m == "lddr" || m == "cpi" || m == "cpir" ||
        m == "cpd" || m == "cpdr") {
        return false;
    }
    if (m == "ex" && lower_copy(trim(line.operands)).find("(sp)") != std::string::npos)
        return false;
    return !operand_has_token(lower_copy(line.operands), "sp");
}

static bool spaghetti_shift_train_instruction(const asm_line &line) {
    const std::string &m = line.mnemonic;
    if (m == "rl" || m == "rr" || m == "sla" || m == "sra" ||
        m == "srl" || m == "rlc" || m == "rrc" || m == "rla" ||
        m == "rra" || m == "rlca" || m == "rrca") {
        return true;
    }
    return m == "add" && lower_copy(trim(line.operands)) == "hl,hl";
}

static std::string spaghetti_key_line(const asm_line &line) {
    return line.mnemonic + "\t" + trim(line.operands);
}

static long spaghetti_estimated_bytes(const asm_line &line) {
    const std::string m = line.mnemonic;
    const std::string ops = lower_copy(trim(line.operands));
    if (m.empty())
        return 0;
    if (m == "nop" || m == "daa" || m == "cpl" || m == "scf" ||
        m == "ccf" || m == "rla" || m == "rra" || m == "rlca" ||
        m == "rrca" || m == "exx") {
        return 1;
    }
    if (m == "ld") {
        std::string dst, src;
        if (!split_ld(ops, dst, src))
            return 3;
        dst = trim(dst);
        src = trim(src);
        if (is_reg8(dst) && is_reg8(src))
            return 1;
        if (is_reg8(dst) && is_immediate_operand(src))
            return 2;
        if (is_reg16(dst) && is_immediate_operand(src))
            return 3;
        if ((is_reg8(dst) && uses_hl_indirect(src)) ||
            (uses_hl_indirect(dst) && is_reg8(src)))
            return 1;
        if (uses_hl_indirect(dst) && is_immediate_operand(src))
            return 2;
        if (uses_ixiy_disp(dst) || uses_ixiy_disp(src))
            return 3;
        if (uses_abs_indirect(dst) || uses_abs_indirect(src))
            return 3;
        return 2;
    }
    if (m == "inc" || m == "dec") {
        if (uses_ixiy_disp(ops))
            return 3;
        return 1;
    }
    if (m == "add" || m == "adc" || m == "sbc") {
        std::string dst, src;
        if (split_ld(ops, dst, src)) {
            dst = trim(dst);
            src = trim(src);
            if ((dst == "hl" || dst == "ix" || dst == "iy") && is_reg16(src))
                return 1;
            if (is_immediate_operand(src))
                return 2;
            if (uses_ixiy_disp(src))
                return 3;
        }
        return 1;
    }
    if (m == "sub" || m == "and" || m == "or" || m == "xor" || m == "cp") {
        if (is_immediate_operand(ops))
            return 2;
        if (uses_ixiy_disp(ops))
            return 3;
        return 1;
    }
    if (m == "rl" || m == "rr" || m == "sla" || m == "sra" ||
        m == "srl" || m == "rlc" || m == "rrc" || m == "bit" ||
        m == "set" || m == "res") {
        return uses_ixiy_disp(ops) ? 4 : 2;
    }
    return 3;
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
        std::unordered_set<size_t> &active) {
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
                                                 budget, active);
            const bool fallthrough =
                path_overwrites_pair_before_read(lines, k + 1, pair, lo, hi,
                                                 budget, active);
            return finish(taken && fallthrough);
        }

        if (parse_unconditional_jump(line, target)) {
            const size_t target_idx = find_label_index(lines, target);
            if (target_idx == lines.size())
                return finish(false);
            return finish(path_overwrites_pair_before_read(
                lines, target_idx, pair, lo, hi, budget, active));
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
        const asm_line *ret = next_instruction(lines, pos);
        return finish(pop_ix && ret &&
                      pop_ix->mnemonic == "pop" &&
                      trim(pop_ix->operands) == "ix" &&
                      is_plain_ret(*ret));
    }

    return finish(false);
}

static bool is_modern_return_tail(const std::vector<asm_line> &lines,
                                  size_t start) {
    std::unordered_set<size_t> active;
    return is_modern_return_tail(lines, start, 8, active);
}

static bool hl_dead_before_read_or_modern_return(
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
        if (overwrites_hl_without_reading_it(lines, k))
            return finish(true);
        if (is_modern_return_tail(lines, k))
            return finish(true);

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

        if (!line_preserves_pair_and_sp(line, "hl", 'l', 'h'))
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
    for (size_t i = 0; i + 1 < lines_.size(); ++i) {
        if (apply_structural_rules(lines_, i)) { changed = true; continue; }
        if (rule_bool_ifx_shortcircuit(i)) { changed = true; continue; }
        if (rule_call_ret_to_jp(i))      { changed = true; continue; }
        if (rule_signed_le_fallthrough(lines_, i)) { changed = true; continue; }
        if (rule_invert_branch_skip(i))  { changed = true; continue; }
        if (rule_zero_cmp_optimize(i))   { changed = true; continue; }
        if (rule_hl_nonzero_materialize(i)) { changed = true; continue; }
        if (rule_ex_de_hl_load_double(i)) { changed = true; continue; }
        if (rule_ix_store_reload(i))     { changed = true; continue; }
        if (rule_dead_hl_ix_load(i))     { changed = true; continue; }
        if (rule_temp_store_reload(i))   { changed = true; continue; }
        if (rule_push_hl_ix_pop_de(i))   { changed = true; continue; }
        if (rule_push_hl_load_pop_de(i)) { changed = true; continue; }
        if (rule_push_hl_pop_de(i))      { changed = true; continue; }
        if (rule_push_hl_pop_bc(i))      { changed = true; continue; }
        if (speed_bias_ && rule_push_pair_copy_adjacent_speed(i)) {
            changed = true;
            continue;
        }
        if (speed_bias_ && rule_superopt_accumulator_sequences(i)) {
            changed = true;
            continue;
        }
        if (speed_bias_ && rule_superopt_lowbit_pair_sequences(i)) {
            changed = true;
            continue;
        }
        if (speed_bias_ && rule_superopt_srl_a_const_shift(i)) {
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
        if (speed_bias_ && rule_superopt_dead_flag_setter(i)) {
            changed = true;
            continue;
        }
        if (speed_bias_ && rule_superopt_register_move_sequences(i)) {
            changed = true;
            continue;
        }
        if (speed_bias_ && rule_superopt_separated_pair_immediate_load(i)) {
            changed = true;
            continue;
        }
        if (speed_bias_ && rule_superopt_ix_word_inc_direct(i)) {
            changed = true;
            continue;
        }
        if (speed_bias_ && rule_superopt_ix_byte_inc_direct(i)) {
            changed = true;
            continue;
        }
        if (speed_bias_ && rule_superopt_ix_byte_load_forward(i)) {
            changed = true;
            continue;
        }
        if (speed_bias_ && rule_superopt_compare_fallthrough_reload(i)) {
            changed = true;
            continue;
        }
        if (speed_bias_ && rule_superopt_xor_compare_fallthrough_reload(i)) {
            changed = true;
            continue;
        }
        if (speed_bias_ && rule_superopt_redundant_zero_store_chain(i)) {
            changed = true;
            continue;
        }
        if (speed_bias_ && rule_superopt_zero_extend_pair_test_shortcut(i)) {
            changed = true;
            continue;
        }
        if (speed_bias_ && rule_superopt_low_byte_xor_forward(i)) {
            changed = true;
            continue;
        }
        if (speed_bias_ && rule_superopt_dead_bc_xor_hl_forward(i)) {
            changed = true;
            continue;
        }
        if (speed_bias_ && rule_superopt_dead_c_xor_hl_forward(i)) {
            changed = true;
            continue;
        }
        if (speed_bias_ && rule_superopt_de_xor_right5(i)) {
            changed = true;
            continue;
        }
        if (speed_bias_ && rule_superopt_hl_xor_right5_stack(i)) {
            changed = true;
            continue;
        }
        if (speed_bias_ && rule_superopt_zero_extend_src_to_bc(i)) {
            changed = true;
            continue;
        }
        if (speed_bias_ && rule_superopt_zero_extend_a_to_bc(i)) {
            changed = true;
            continue;
        }
        if (speed_bias_ && rule_superopt_zero_extend_truth_test(i)) {
            changed = true;
            continue;
        }
        if (speed_bias_ && rule_superopt_zero_extend_dead_hl_truth_test(i)) {
            changed = true;
            continue;
        }
        if (speed_bias_ && rule_superopt_zero_extend_dead_bc_truth_test(i)) {
            changed = true;
            continue;
        }
        if (speed_bias_ && rule_superopt_hl_byte_load_forward(i)) {
            changed = true;
            continue;
        }
        if (speed_bias_ && rule_superopt_de_to_hl_dead_de_copy(i)) {
            changed = true;
            continue;
        }
        if (speed_bias_ && rule_superopt_dead_bc_return_copy(i)) {
            changed = true;
            continue;
        }
        if (speed_bias_ && rule_superopt_bc_to_de_alu_forward(i)) {
            changed = true;
            continue;
        }
        if (speed_bias_ && rule_superopt_dead_hl_de_return_store_forward(i)) {
            changed = true;
            continue;
        }
        if (speed_bias_ && rule_superopt_modern_const_return_direct(i)) {
            changed = true;
            continue;
        }
        if (speed_bias_ && rule_superopt_cancel_exx_pair(i)) {
            changed = true;
            continue;
        }
        if (speed_bias_ && rule_superopt_call_arg_de_direct(i)) {
            changed = true;
            continue;
        }
        if (speed_bias_ && rule_superopt_dead_hl_exchange_to_de_load(i)) {
            changed = true;
            continue;
        }
        if (speed_bias_ && rule_superopt_equal_de_hl_exchange(i)) {
            changed = true;
            continue;
        }
        if (speed_bias_ && rule_superopt_exchange_sandwich_de_load(i)) {
            changed = true;
            continue;
        }
        if (speed_bias_ && rule_superopt_dead_pair_stack_discard_pop(i)) {
            changed = true;
            continue;
        }
        if (speed_bias_ && rule_superopt_dead_pair_pop_push(i)) {
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
        if (rule_push_pair_exchange_span(i)) { changed = true; continue; }
        if (rule_push_pair_copy_span(i)) { changed = true; continue; }
        if (rule_push_pop_same_reg_span(i)) { changed = true; continue; }
        if (rule_ix_byte_store_reload(i)) { changed = true; continue; }
        if (rule_redundant_ld(i))        { changed = true; continue; }
        if (rule_self_store(i))          { changed = true; continue; }
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

void z80_peep::apply_spaghetti_passes(int passes) {
    for (int p = 0; p < passes; ++p) {
        if (!apply_spaghetti_once())
            break;
    }
}

void z80_peep::apply_spaghetti_tail_passes(int passes) {
    for (int p = 0; p < passes; ++p) {
        if (!apply_spaghetti_tail_once())
            break;
    }
}

bool z80_peep::apply_spaghetti_once() {
    struct candidate {
        bool valid = false;
        std::string key;
        std::string section;
        size_t length = 0;
        long sequence_bytes = 0;
        long saving = 0;
        std::vector<size_t> starts;
    };

    if (lines_.empty())
        return false;

    std::vector<bool> is_code(lines_.size(), true);
    std::vector<std::string> section(lines_.size());
    bool in_code = true;
    bool in_helper = false;
    bool has_section_directives = false;
    std::string current_section;
    for (size_t i = 0; i < lines_.size(); ++i) {
        auto &line = lines_[i];
        if (is_spaghetti_helper_label(line))
            in_helper = true;

        std::string section_name;
        if (is_code_section_directive(line, section_name)) {
            has_section_directives = true;
            in_code = true;
            current_section = section_name;
        } else if (is_data_section_directive(line)) {
            has_section_directives = true;
            in_code = false;
            current_section = trim(line.operands);
        } else if (is_section_directive(line)) {
            has_section_directives = true;
        }

        is_code[i] = in_code && !in_helper;
        section[i] = current_section;
        if (in_helper && line.mnemonic == "ret")
            in_helper = false;
    }

    auto valid_sequence = [&](size_t start, size_t length) {
        if (start + length > lines_.size())
            return false;
        const std::string &seq_section = section[start];
        if (seq_section.empty() && has_section_directives)
            return false;
        bool pure_shift_train = true;
        for (size_t k = 0; k < length; ++k) {
            const size_t idx = start + k;
            if (!is_code[idx] || section[idx] != seq_section ||
                !spaghetti_candidate_instruction(lines_[idx])) {
                return false;
            }
            pure_shift_train =
                pure_shift_train && spaghetti_shift_train_instruction(lines_[idx]);
        }
        return !pure_shift_train;
    };

    auto sequence_key = [&](size_t start, size_t length) {
        std::string key = section[start] + "\n";
        for (size_t k = 0; k < length; ++k) {
            key += spaghetti_key_line(lines_[start + k]);
            key += "\n";
        }
        return key;
    };

    auto sequence_bytes = [&](size_t start, size_t length) {
        long bytes = 0;
        for (size_t k = 0; k < length; ++k)
            bytes += spaghetti_estimated_bytes(lines_[start + k]);
        return bytes;
    };

    candidate best;
    constexpr size_t min_length = 3;
    constexpr size_t max_length = 12;
    constexpr long call_bytes = 3;
    constexpr long ret_bytes = 1;
    constexpr long min_saving = 2;

    for (size_t length = max_length; length >= min_length; --length) {
        std::unordered_map<std::string, std::vector<size_t>> occurrences;
        for (size_t start = 0; start + length <= lines_.size(); ++start) {
            if (!valid_sequence(start, length))
                continue;
            occurrences[sequence_key(start, length)].push_back(start);
        }

        for (const auto &entry : occurrences) {
            if (entry.second.size() < 2)
                continue;

            std::vector<size_t> starts;
            size_t covered_until = 0;
            bool have_covered = false;
            for (size_t start : entry.second) {
                if (!have_covered || start >= covered_until) {
                    starts.push_back(start);
                    covered_until = start + length;
                    have_covered = true;
                }
            }
            if (starts.size() < 2)
                continue;

            const long bytes = sequence_bytes(starts.front(), length);
            const long saving =
                (static_cast<long>(starts.size()) - 1) * bytes -
                static_cast<long>(starts.size()) * call_bytes -
                ret_bytes;
            if (saving < min_saving)
                continue;

            if (!best.valid || saving > best.saving ||
                (saving == best.saving && length > best.length)) {
                best.valid = true;
                best.key = entry.first;
                best.section = section[starts.front()];
                best.length = length;
                best.sequence_bytes = bytes;
                best.saving = saving;
                best.starts = starts;
            }
        }

        if (length == min_length)
            break;
    }

    if (!best.valid)
        return false;

    std::unordered_set<std::string> labels;
    for (const auto &line : lines_) {
        if (!line.label.empty())
            labels.insert(line.label);
    }

    std::string label;
    for (int n = 0;; ++n) {
        label = "__xopt_spaghetti_" + std::to_string(n);
        if (labels.find(label) == labels.end())
            break;
    }

    std::vector<asm_line> body;
    body.reserve(best.length);
    for (size_t k = 0; k < best.length; ++k)
        body.push_back(lines_[best.starts.front() + k]);

    std::sort(best.starts.begin(), best.starts.end(), std::greater<size_t>());
    for (size_t start : best.starts) {
        lines_[start] = asm_line::parse("\tcall\t" + label);
        lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(start + 1),
                     lines_.begin() + static_cast<std::ptrdiff_t>(
                         start + best.length));
    }

    std::vector<asm_line> helper;
    helper.push_back(asm_line::parse("; xopt: spaghetti outlined duplicate kernel"));
    if (!best.section.empty())
        helper.push_back(asm_line::parse("\t.area\t" + best.section));
    helper.push_back(asm_line::parse(label + ":"));
    for (const auto &line : body)
        helper.push_back(line);
    helper.push_back(asm_line::parse("\tret"));
    lines_.insert(lines_.end(), helper.begin(), helper.end());
    return true;
}

bool z80_peep::apply_spaghetti_tail_once() {
    struct helper_info {
        size_t label_index = 0;
        size_t ret_index = 0;
    };
    struct tail_use {
        size_t call_index = 0;
        size_t jump_index = 0;
    };
    struct use_summary {
        bool invalid = false;
        std::string target;
        std::vector<tail_use> uses;
    };

    std::unordered_map<std::string, helper_info> helpers;
    for (size_t i = 0; i < lines_.size(); ++i) {
        const auto &line = lines_[i];
        if (!is_spaghetti_helper_label(line))
            continue;

        size_t ret_idx = lines_.size();
        for (size_t k = i + 1; k < lines_.size(); ++k) {
            if (k != i + 1 && is_spaghetti_helper_label(lines_[k]))
                break;
            if (!lines_[k].label.empty())
                break;
            if (lines_[k].mnemonic == "ret") {
                ret_idx = k;
                break;
            }
        }
        if (ret_idx != lines_.size())
            helpers[line.label] = helper_info{i, ret_idx};
    }
    if (helpers.empty())
        return false;

    std::unordered_map<std::string, use_summary> uses;
    for (const auto &entry : helpers)
        uses[entry.first] = use_summary{};

    for (size_t i = 0; i < lines_.size(); ++i) {
        const auto &line = lines_[i];
        if (is_spaghetti_helper_label(line))
            continue;

        std::string ref;
        bool references_helper = false;
        if (line.mnemonic == "call") {
            ref = trim(line.operands);
            references_helper = helpers.find(ref) != helpers.end();
        } else if (line.mnemonic == "jp" || line.mnemonic == "jr" ||
                   line.mnemonic == "djnz") {
            std::string cc;
            std::string target;
            if (parse_unconditional_jump(line, target)) {
                ref = target;
            } else if (split_conditional_branch_target(line, cc, target)) {
                ref = target;
            } else {
                ref = trim(line.operands);
            }
            references_helper = helpers.find(ref) != helpers.end();
        }

        if (!references_helper)
            continue;

        auto &summary = uses[ref];
        if (line.mnemonic != "call" || i + 1 >= lines_.size() ||
            !line.label.empty() || !lines_[i + 1].label.empty()) {
            summary.invalid = true;
            continue;
        }

        std::string jump_target;
        if (!parse_unconditional_jump(lines_[i + 1], jump_target)) {
            summary.invalid = true;
            continue;
        }

        if (jump_target == ref || helpers.find(jump_target) != helpers.end()) {
            summary.invalid = true;
            continue;
        }

        if (summary.target.empty())
            summary.target = jump_target;
        else if (summary.target != jump_target)
            summary.invalid = true;

        summary.uses.push_back(tail_use{i, i + 1});
    }

    std::string best_helper;
    size_t best_uses = 0;
    for (const auto &entry : uses) {
        const auto &summary = entry.second;
        if (summary.invalid || summary.target.empty() || summary.uses.size() < 2)
            continue;
        if (summary.uses.size() > best_uses) {
            best_helper = entry.first;
            best_uses = summary.uses.size();
        }
    }
    if (best_helper.empty())
        return false;

    const auto &summary = uses[best_helper];
    const auto &helper = helpers[best_helper];
    lines_[helper.ret_index] = asm_line::parse("\tjp\t" + summary.target);

    std::vector<tail_use> replacements = summary.uses;
    std::sort(replacements.begin(), replacements.end(),
              [](const tail_use &a, const tail_use &b) {
                  return a.call_index > b.call_index;
              });
    for (const auto &use : replacements) {
        asm_line replacement = asm_line::parse("\tjp\t" + best_helper);
        replacement.label = lines_[use.call_index].label;
        replacement.is_label = lines_[use.call_index].is_label;
        replacement.is_global_label = lines_[use.call_index].is_global_label;
        lines_[use.call_index] = replacement;
        lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(use.jump_index));
    }
    return true;
}

std::string z80_peep::optimize(const std::string &asm_text,
	                               bool speed_bias,
	                               bool enable_spaghetti,
                                   int normal_passes) {
    z80_peep p;
    p.speed_bias_ = speed_bias;
    p.load(asm_text);
    if (normal_passes > 0)
        p.apply_passes(normal_passes);
    if (enable_spaghetti) {
        p.apply_spaghetti_passes(4);
        p.apply_spaghetti_tail_passes(3);
        p.apply_passes(4);
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

// call target; ret  →  jp target
bool z80_peep::rule_call_ret_to_jp(size_t i) {
    if (i + 1 >= lines_.size()) return false;

    auto &call = lines_[i];
    if (call.mnemonic != "call") return false;
    if (call.operands.find(',') != std::string::npos) return false;

    auto is_noncode = [](const asm_line &l) {
        return l.mnemonic.empty();
    };
    size_t j = i + 1;
    while (j < lines_.size() && is_noncode(lines_[j]))
        ++j;
    if (j < lines_.size() && lines_[j].mnemonic == "ret" &&
        trim(lines_[j].operands).empty() && lines_[j].label.empty()) {
        call.mnemonic = "jp";
        lines_.erase(lines_.begin() + j);
        return true;
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

// push hl; pop de  →  ex de,hl  (nothing between)
bool z80_peep::rule_push_hl_pop_de(size_t i) {
    (void)i;
    // Not safe: push/pop copies HL into DE while preserving HL; ex swaps them.
    return false;
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
    if (!split_ld(l0.operands, d0, s0) || s0 != "l") return false;
    int off_lo; if (!parse_ix_ref(d0, off_lo)) return false;

    if (!split_ld(l1.operands, d1, s1) || s1 != "h") return false;
    int off_hi; if (!parse_ix_ref(d1, off_hi) || off_hi != off_lo + 1) return false;

    if (!split_ld(l2.operands, d2, s2) || d2 != "l") return false;
    int off_lo2; if (!parse_ix_ref(s2, off_lo2) || off_lo2 != off_lo) return false;

    if (!split_ld(l3.operands, d3, s3) || d3 != "h") return false;
    int off_hi2; if (!parse_ix_ref(s3, off_hi2) || off_hi2 != off_hi) return false;

    lines_.erase(lines_.begin() + i + 2, lines_.begin() + i + 4);
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

    // Keep a little headroom because this pass still relies on an internal
    // size estimator rather than the assembler's final encoded addresses.
    if (disp < -120 || disp > 120)
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

bool z80_peep::rule_superopt_ix_word_inc_direct(size_t i) {
    if (i + 4 >= lines_.size())
        return false;
    for (size_t k = 0; k < 5; ++k) {
        if (!lines_[i + k].label.empty())
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
    if (lines_[i + 2].mnemonic != "inc" ||
        trim(lines_[i + 2].operands) != "hl") {
        return false;
    }
    if (lines_[i + 3].mnemonic != "ld" ||
        !split_ld(lines_[i + 3].operands, dst, src) ||
        !parse_ix_ref(trim(dst), lo_store) ||
        trim(src) != "l") {
        return false;
    }
    if (lines_[i + 4].mnemonic != "ld" ||
        !split_ld(lines_[i + 4].operands, dst, src) ||
        !parse_ix_ref(trim(dst), hi_store) ||
        trim(src) != "h") {
        return false;
    }
    if (hi_load != lo_load + 1 ||
        lo_store != lo_load ||
        hi_store != hi_load) {
        return false;
    }
    if (!flags_overwritten_before_read_or_escape(lines_, i + 5))
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
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 5));
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

    if (!a_overwritten_before_read(lines_, i + 3) ||
        !flags_overwritten_before_read_or_escape(lines_, i + 3)) {
        return false;
    }

    lines_[i] = asm_line::parse("\tinc\t" + std::to_string(load_off) + "(ix)");
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(i + 1),
                 lines_.begin() + static_cast<std::ptrdiff_t>(i + 3));
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

    if (reg == "l" && i + 2 < lines_.size() && lines_[i + 2].mnemonic == "ld") {
        std::string next_dst, next_src;
        if (split_ld(lines_[i + 2].operands, next_dst, next_src) &&
            trim(next_dst) == "h" && immediate_is(next_src, 0)) {
            return false;
        }
    }
    if (reg == "c" && is_ld_zero(i + 2, "b") &&
        ((is_ld_reg(i + 3, "h", "b") && is_ld_reg(i + 4, "l", "c")) ||
         (i + 3 < lines_.size() && is_or_a_self(lines_[i + 3])))) {
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

bool z80_peep::rule_push_pair_exchange_span(size_t i) {
    (void)i;
    // Not safe in general for the same reason as rule_push_hl_pop_de(): the
    // stack sequence preserves the source pair while ex de,hl swaps it away.
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
