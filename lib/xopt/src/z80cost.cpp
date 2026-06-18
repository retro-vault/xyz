//
// z80cost.cpp -- static Z80 assembly byte/cycle estimator.
//
// This is not a full assembler. It recognizes the instruction/directive
// shapes emitted by xcc/xas well enough for xopt --stats to report how much
// the peephole pass shaved from a source file before it is assembled.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//

#include "xopt/xopt.h"
#include "xopt/z80peep.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>

namespace xopt {
namespace {

struct inst_cost {
    long bytes = 0;
    long cycles = 0;
    bool known = false;
};

static std::string trim(const std::string &s) {
    size_t b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos)
        return "";
    size_t e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

static std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return (char)std::tolower(c); });
    return s;
}

static std::string normalize_operand(std::string s) {
    s = lower(trim(s));
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        if (c != ' ' && c != '\t')
            out.push_back(c);
    }
    return out;
}

static std::vector<std::string> split_operands(const std::string &ops) {
    std::vector<std::string> out;
    std::string cur;
    bool in_quote = false;
    char quote = '\0';

    for (char c : ops) {
        if ((c == '"' || c == '\'') && (!in_quote || c == quote)) {
            in_quote = !in_quote;
            quote = in_quote ? c : '\0';
            cur.push_back(c);
        } else if (c == ',' && !in_quote) {
            out.push_back(normalize_operand(cur));
            cur.clear();
        } else {
            cur.push_back(c);
        }
    }
    if (!cur.empty() || !ops.empty())
        out.push_back(normalize_operand(cur));
    return out;
}

static std::string directive_name(const std::string &mnemonic) {
    if (!mnemonic.empty() && mnemonic[0] == '.')
        return mnemonic.substr(1);
    return mnemonic;
}

static bool parse_integer(const std::string &text, long &value) {
    std::string s = normalize_operand(text);
    if (s.empty())
        return false;
    if (s[0] == '#')
        s.erase(s.begin());
    if (!s.empty() && s[0] == '$')
        s = "0x" + s.substr(1);

    char *end = nullptr;
    value = std::strtol(s.c_str(), &end, 0);
    return end != s.c_str() && *end == '\0';
}

static long quoted_string_size(const std::string &item, bool nul_terminated) {
    std::string s = trim(item);
    if (s.size() < 2 || (s.front() != '"' && s.front() != '\''))
        return -1;
    const char quote = s.front();
    if (s.back() != quote)
        return -1;

    long count = 0;
    for (size_t i = 1; i + 1 < s.size(); ++i) {
        if (s[i] == '\\' && i + 1 < s.size() - 1)
            ++i;
        ++count;
    }
    return count + (nul_terminated ? 1 : 0);
}

static inst_cost directive_cost(const asm_line &line) {
    const std::string m = directive_name(line.mnemonic);
    const std::vector<std::string> ops = split_operands(line.operands);

    if (m == "db" || m == "byte") {
        long bytes = 0;
        for (const auto &op : ops) {
            const long q = quoted_string_size(op, false);
            bytes += q >= 0 ? q : 1;
        }
        return {bytes, 0, true};
    }
    if (m == "ascii" || m == "asciz" || m == "ascis") {
        long bytes = 0;
        for (const auto &op : ops) {
            const long q = quoted_string_size(op, m != "ascii");
            bytes += q >= 0 ? q : 0;
        }
        return {bytes, 0, true};
    }
    if (m == "dw" || m == "word") {
        return {2L * (long)ops.size(), 0, true};
    }
    if (m == "ds" || m == "space" || m == "skip" || m == "blkb") {
        long bytes = 0;
        if (!ops.empty() && parse_integer(ops[0], bytes))
            return {bytes, 0, true};
        return {0, 0, true};
    }
    if (m == "area" || m == "module" || m == "globl" || m == "global" ||
        m == "extern" || m == "org" || m == "equ" || m == "include" ||
        m == "list" || m == "nlist" || m == "optsdcc" || m == "title" ||
        m == "sbttl" || m == "end") {
        return {0, 0, true};
    }

    return {0, 0, false};
}

static bool is_reg8(const std::string &op) {
    return op == "a" || op == "b" || op == "c" || op == "d" ||
           op == "e" || op == "h" || op == "l";
}

static bool is_index_half(const std::string &op) {
    return op == "ixh" || op == "ixl" || op == "iyh" || op == "iyl";
}

static bool is_simple_reg8(const std::string &op) {
    return is_reg8(op) || is_index_half(op);
}

static bool is_reg16(const std::string &op) {
    return op == "bc" || op == "de" || op == "hl" || op == "sp";
}

static bool is_index_reg(const std::string &op) {
    return op == "ix" || op == "iy";
}

static bool is_stack_pair(const std::string &op) {
    return op == "bc" || op == "de" || op == "hl" || op == "af" ||
           op == "ix" || op == "iy";
}

static bool is_mem_hl(const std::string &op) {
    return op == "(hl)";
}

static bool is_mem_bcde(const std::string &op) {
    return op == "(bc)" || op == "(de)";
}

static bool is_mem_sp(const std::string &op) {
    return op == "(sp)";
}

static bool is_index_disp(const std::string &op) {
    return op.find("(ix)") != std::string::npos ||
           op.find("(iy)") != std::string::npos ||
           op.find("(ix+") != std::string::npos ||
           op.find("(iy+") != std::string::npos ||
           op.find("(ix-") != std::string::npos ||
           op.find("(iy-") != std::string::npos;
}

static bool is_mem_direct(const std::string &op) {
    return op.size() >= 2 && op.front() == '(' && op.back() == ')' &&
           !is_mem_hl(op) && !is_mem_bcde(op) && !is_mem_sp(op) &&
           !is_index_disp(op);
}

static bool is_cond(const std::string &op) {
    return op == "nz" || op == "z" || op == "nc" || op == "c" ||
           op == "po" || op == "pe" || op == "p" || op == "m";
}

static bool is_jr_cond(const std::string &op) {
    return op == "nz" || op == "z" || op == "nc" || op == "c";
}

static inst_cost ld_cost(const std::vector<std::string> &ops) {
    if (ops.size() != 2)
        return {0, 0, false};

    const std::string &dst = ops[0];
    const std::string &src = ops[1];

    if ((is_simple_reg8(dst) && is_simple_reg8(src)) ||
        (is_reg8(dst) && is_reg8(src))) {
        const bool indexed = is_index_half(dst) || is_index_half(src);
        return {indexed ? 2 : 1, indexed ? 8 : 4, true};
    }
    if ((is_reg8(dst) && is_mem_hl(src)) ||
        (is_mem_hl(dst) && is_reg8(src)))
        return {1, 7, true};
    if (is_reg8(dst) && is_index_disp(src))
        return {3, 19, true};
    if (is_index_disp(dst) && is_reg8(src))
        return {3, 19, true};
    if (is_index_disp(dst))
        return {4, 19, true};
    if (is_reg8(dst))
        return {2, 7, true};
    if (is_mem_hl(dst))
        return {2, 10, true};
    if ((dst == "a" && is_mem_bcde(src)) ||
        (is_mem_bcde(dst) && src == "a"))
        return {1, 7, true};
    if ((dst == "a" && is_mem_direct(src)) ||
        (is_mem_direct(dst) && src == "a"))
        return {3, 13, true};
    if ((dst == "sp" && src == "hl") || (dst == "sp" && is_index_reg(src)))
        return {src == "hl" ? 1 : 2, src == "hl" ? 6 : 10, true};
    if ((dst == "hl" && is_mem_direct(src)) ||
        (is_mem_direct(dst) && src == "hl"))
        return {3, 16, true};
    if ((is_index_reg(dst) && is_mem_direct(src)) ||
        (is_mem_direct(dst) && is_index_reg(src)))
        return {4, 20, true};
    if ((is_reg16(dst) && is_mem_direct(src)) ||
        (is_mem_direct(dst) && is_reg16(src)))
        return {4, 20, true};
    if (is_reg16(dst))
        return {3, 10, true};
    if (is_index_reg(dst))
        return {4, 14, true};

    return {0, 0, false};
}

static inst_cost inc_dec_cost(const std::string &op) {
    if (is_reg8(op))
        return {1, 4, true};
    if (is_index_half(op))
        return {2, 8, true};
    if (is_mem_hl(op))
        return {1, 11, true};
    if (is_index_disp(op))
        return {3, 23, true};
    if (is_reg16(op))
        return {1, 6, true};
    if (is_index_reg(op))
        return {2, 10, true};
    return {0, 0, false};
}

static inst_cost alu_operand_cost(const std::string &op) {
    if (is_reg8(op))
        return {1, 4, true};
    if (is_index_half(op))
        return {2, 8, true};
    if (is_mem_hl(op))
        return {1, 7, true};
    if (is_index_disp(op))
        return {3, 19, true};
    return {2, 7, true};
}

static inst_cost alu_cost(const std::string &mnemonic,
                          const std::vector<std::string> &ops) {
    if (mnemonic == "add" && ops.size() == 2) {
        if (ops[0] == "hl" && is_reg16(ops[1]))
            return {1, 11, true};
        if (is_index_reg(ops[0]) && (is_reg16(ops[1]) || is_index_reg(ops[1])))
            return {2, 15, true};
        if (ops[0] == "a")
            return alu_operand_cost(ops[1]);
    }
    if ((mnemonic == "adc" || mnemonic == "sbc") && ops.size() == 2) {
        if (ops[0] == "hl" && is_reg16(ops[1]))
            return {2, 15, true};
        if (ops[0] == "a")
            return alu_operand_cost(ops[1]);
    }
    if (mnemonic == "sub") {
        if (ops.size() == 1)
            return alu_operand_cost(ops[0]);
        if (ops.size() == 2 && ops[0] == "a")
            return alu_operand_cost(ops[1]);
    }
    if (mnemonic == "and" || mnemonic == "or" ||
        mnemonic == "xor" || mnemonic == "cp") {
        if (ops.size() == 1)
            return alu_operand_cost(ops[0]);
        if (ops.size() == 2 && ops[0] == "a")
            return alu_operand_cost(ops[1]);
    }
    return {0, 0, false};
}

static inst_cost rotate_shift_cost(const std::string &op) {
    if (is_reg8(op))
        return {2, 8, true};
    if (is_index_half(op))
        return {2, 8, true};
    if (is_mem_hl(op))
        return {2, 15, true};
    if (is_index_disp(op))
        return {4, 23, true};
    return {0, 0, false};
}

static inst_cost bit_cost(const std::string &mnemonic,
                          const std::vector<std::string> &ops) {
    if (ops.size() != 2)
        return {0, 0, false};
    const std::string &target = ops[1];
    if (is_reg8(target))
        return {2, 8, true};
    if (is_mem_hl(target))
        return {2, mnemonic == "bit" ? 12 : 15, true};
    if (is_index_disp(target))
        return {4, mnemonic == "bit" ? 20 : 23, true};
    return {0, 0, false};
}

static inst_cost instruction_cost(const asm_line &line) {
    const std::string m = line.mnemonic;
    const std::vector<std::string> ops = split_operands(line.operands);

    if (m.empty())
        return {0, 0, true};
    if (m[0] == '.' || m == "db" || m == "dw" || m == "ds")
        return directive_cost(line);

    if (m == "nop")
        return {1, 4, true};
    if (m == "halt")
        return {1, 4, true};
    if (m == "di" || m == "ei" || m == "scf" || m == "ccf" ||
        m == "cpl" || m == "daa" || m == "exx")
        return {1, 4, true};
    if (m == "neg" || m == "im")
        return {2, 8, true};
    if (m == "rld" || m == "rrd")
        return {2, 18, true};
    if (m == "ld")
        return ld_cost(ops);
    if ((m == "inc" || m == "dec") && ops.size() == 1)
        return inc_dec_cost(ops[0]);
    if (m == "add" || m == "adc" || m == "sbc" || m == "sub" ||
        m == "and" || m == "or" || m == "xor" || m == "cp")
        return alu_cost(m, ops);
    if (m == "rlca" || m == "rrca" || m == "rla" || m == "rra")
        return {1, 4, true};
    if (m == "rlc" || m == "rrc" || m == "rl" || m == "rr" ||
        m == "sla" || m == "sra" || m == "srl" || m == "sll") {
        if (ops.size() == 1)
            return rotate_shift_cost(ops[0]);
    }
    if (m == "bit" || m == "set" || m == "res")
        return bit_cost(m, ops);
    if (m == "jp") {
        if (ops.size() == 1) {
            if (ops[0] == "(hl)")
                return {1, 4, true};
            if (ops[0] == "(ix)" || ops[0] == "(iy)")
                return {2, 8, true};
            return {3, 10, true};
        }
        if (ops.size() == 2 && is_cond(ops[0]))
            return {3, 10, true};
    }
    if (m == "jr") {
        if (ops.size() == 1)
            return {2, 12, true};
        if (ops.size() == 2 && is_jr_cond(ops[0]))
            return {2, 12, true};
    }
    if (m == "djnz")
        return {2, 13, true};
    if (m == "call") {
        if (ops.size() == 1)
            return {3, 17, true};
        if (ops.size() == 2 && is_cond(ops[0]))
            return {3, 17, true};
    }
    if (m == "ret") {
        if (ops.empty())
            return {1, 10, true};
        if (ops.size() == 1 && is_cond(ops[0]))
            return {1, 11, true};
    }
    if (m == "reti" || m == "retn")
        return {2, 14, true};
    if (m == "rst")
        return {1, 11, true};
    if (m == "push" && ops.size() == 1 && is_stack_pair(ops[0]))
        return {is_index_reg(ops[0]) ? 2 : 1, is_index_reg(ops[0]) ? 15 : 11, true};
    if (m == "pop" && ops.size() == 1 && is_stack_pair(ops[0]))
        return {is_index_reg(ops[0]) ? 2 : 1, is_index_reg(ops[0]) ? 14 : 10, true};
    if (m == "ex" && ops.size() == 2) {
        if ((ops[0] == "de" && ops[1] == "hl") ||
            (ops[0] == "af" && ops[1] == "af'"))
            return {1, 4, true};
        if (is_mem_sp(ops[0]) && ops[1] == "hl")
            return {1, 19, true};
        if (is_mem_sp(ops[0]) && is_index_reg(ops[1]))
            return {2, 23, true};
    }
    if (m == "in" && ops.size() == 2) {
        if (ops[0] == "a" && ops[1].size() >= 2 && ops[1][0] == '(' &&
            ops[1] != "(c)")
            return {2, 11, true};
        if (ops[1] == "(c)")
            return {2, 12, true};
    }
    if (m == "out" && ops.size() == 2) {
        if (ops[0] == "(c)")
            return {2, 12, true};
        if (ops[0].size() >= 2 && ops[0][0] == '(')
            return {2, 11, true};
    }
    if (m == "ldi" || m == "ldd" || m == "cpi" || m == "cpd" ||
        m == "ini" || m == "ind" || m == "outi" || m == "outd")
        return {2, 16, true};
    if (m == "ldir" || m == "lddr" || m == "cpir" || m == "cpdr" ||
        m == "inir" || m == "indr" || m == "otir" || m == "otdr")
        return {2, 21, true};

    return {0, 0, false};
}

} // namespace

assembly_cost estimate_z80_assembly_cost(const std::string &asm_text) {
    assembly_cost total;
    std::istringstream in(asm_text);
    std::string raw;

    while (std::getline(in, raw)) {
        const asm_line line = asm_line::parse(raw);
        if (line.is_comment || line.mnemonic.empty())
            continue;

        const inst_cost cost = instruction_cost(line);
        if (!cost.known) {
            ++total.unknown_instructions;
            continue;
        }
        total.bytes += cost.bytes;
        total.cycles += cost.cycles;
    }

    return total;
}

optimization_stats analyze_assembly_optimization(
    const std::string &asm_text,
    const optimizer_options &options) {
    optimization_stats stats;
    stats.before = estimate_z80_assembly_cost(asm_text);
    stats.after = estimate_z80_assembly_cost(optimize_assembly(asm_text, options));
    return stats;
}

} // namespace xopt
