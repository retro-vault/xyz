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
#include "backend/z80/z80peep.h"
#include <algorithm>
#include <cctype>
#include <cstring>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace xcc {

// ----- asm_line ------------------------------------------------------

static std::string trim(const std::string &s) {
    size_t b = s.find_first_not_of(" \t");
    if (b == std::string::npos) return "";
    size_t e = s.find_last_not_of(" \t");
    return s.substr(b, e - b + 1);
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
        if (s.empty()) return al;
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
    std::transform(al.mnemonic.begin(), al.mnemonic.end(),
                   al.mnemonic.begin(), ::tolower);

    return al;
}

std::string asm_line::to_string() const {
    std::ostringstream os;
    if (!label.empty())    os << label << ":\n";
    if (!mnemonic.empty()) {
        os << "\t" << mnemonic;
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
        if (rule_push_hl_de_load(i))     { changed = true; continue; }
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

std::string z80_peep::optimize(const std::string &asm_text) {
    z80_peep p;
    p.load(asm_text);
    p.apply_passes(10);
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
// call target; label: ret  →  jp target; label: ret
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
    if (j < lines_.size() && lines_[j].mnemonic == "ret" && lines_[j].label.empty()) {
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
    if (i + 1 >= lines_.size()) return false;
    auto &a = lines_[i];
    auto &b = lines_[i + 1];
    if (a.mnemonic != "push" || trim(a.operands) != "hl") return false;
    if (b.mnemonic != "pop"  || trim(b.operands) != "de") return false;
    a.mnemonic = "ex";
    a.operands = "de, hl";
    lines_.erase(lines_.begin() + i + 1);
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

    // Don't fire if the next non-empty line is a conditional branch.
    for (size_t j = i + 1; j < lines_.size(); ++j) {
        if (lines_[j].mnemonic.empty()) continue;
        if (is_conditional_branch(lines_[j])) return false;
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

} // namespace xcc
