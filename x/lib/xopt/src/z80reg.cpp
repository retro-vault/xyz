//
// z80reg.cpp -- register coverage analysis for Z80 assembly.
//
// This is intentionally a coverage/pressure estimator, not a full liveness
// engine.  It answers "which register families are busy here?" so optimization
// passes can use the result as a guide for later, more exact transformations.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//

#include "xopt/xopt.h"
#include "xopt/z80peep.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <sstream>
#include <string>
#include <unordered_set>

namespace xopt {
namespace {

enum reg_family {
    reg_a,
    reg_bc,
    reg_de,
    reg_hl,
    reg_ix,
    reg_iy,
    reg_sp,
    reg_f,
    reg_a_alt,
    reg_bc_alt,
    reg_de_alt,
    reg_hl_alt,
    reg_f_alt,
    reg_count,
};

static const char *const k_reg_names[] = {
    "A", "BC", "DE", "HL", "IX", "IY", "SP", "F",
    "A'", "BC'", "DE'", "HL'", "F'",
};

struct instruction_event {
    std::string scope;
    long line = 0;
    long alternate_switches = 0;
    std::array<long, reg_count> reads{};
    std::array<long, reg_count> writes{};
    std::array<long, reg_count> touches{};
};

struct scope_builder {
    register_coverage_scope scope;
    std::array<long, reg_count> reads{};
    std::array<long, reg_count> writes{};
    std::array<long, reg_count> touches{};
    double pressure_sum = 0.0;
};

static std::string trim(const std::string &s) {
    size_t b = s.find_first_not_of(" \t");
    if (b == std::string::npos)
        return "";
    size_t e = s.find_last_not_of(" \t");
    return s.substr(b, e - b + 1);
}

static std::string lower_copy(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char ch) {
                       return static_cast<char>(std::tolower(ch));
                   });
    return s;
}

static bool starts_with(const std::string &text, const std::string &prefix) {
    return text.size() >= prefix.size() &&
           text.compare(0, prefix.size(), prefix) == 0;
}

static bool split_operands(const std::string &ops,
                           std::string &left,
                           std::string &right) {
    size_t comma = ops.find(',');
    if (comma == std::string::npos)
        return false;
    left = trim(ops.substr(0, comma));
    right = trim(ops.substr(comma + 1));
    return true;
}

static std::vector<std::string> operand_tokens(const std::string &operand) {
    std::vector<std::string> out;
    const std::string s = lower_copy(operand);
    for (size_t i = 0; i < s.size();) {
        if (!std::isalnum(static_cast<unsigned char>(s[i])) &&
            s[i] != '_' && s[i] != '\'') {
            ++i;
            continue;
        }
        size_t j = i + 1;
        while (j < s.size() &&
               (std::isalnum(static_cast<unsigned char>(s[j])) ||
                s[j] == '_' || s[j] == '\'')) {
            ++j;
        }
        out.push_back(s.substr(i, j - i));
        i = j;
    }
    return out;
}

static bool has_memory_operand(const std::string &operand) {
    return operand.find('(') != std::string::npos ||
           operand.find(')') != std::string::npos;
}

static bool is_immediate_or_number(const std::string &operand) {
    const std::string s = trim(operand);
    if (s.empty())
        return true;
    if (s.front() == '#' || s.front() == '$')
        return true;
    if (std::isdigit(static_cast<unsigned char>(s.front())))
        return true;
    if ((s.front() == '-' || s.front() == '+') && s.size() > 1 &&
        std::isdigit(static_cast<unsigned char>(s[1]))) {
        return true;
    }
    return false;
}

static void add_family(std::unordered_set<int> &set, reg_family family) {
    set.insert(static_cast<int>(family));
}

static bool is_alternate_family(size_t r) {
    return r == reg_a_alt || r == reg_bc_alt || r == reg_de_alt ||
           r == reg_hl_alt || r == reg_f_alt;
}

static void add_token_family(std::unordered_set<int> &set,
                             const std::string &token) {
    if (token == "a")
        add_family(set, reg_a);
    else if (token == "a'")
        add_family(set, reg_a_alt);
    else if (token == "f")
        add_family(set, reg_f);
    else if (token == "f'")
        add_family(set, reg_f_alt);
    else if (token == "af") {
        add_family(set, reg_a);
        add_family(set, reg_f);
    } else if (token == "af'") {
        add_family(set, reg_a_alt);
        add_family(set, reg_f_alt);
    } else if (token == "b" || token == "c" || token == "bc")
        add_family(set, reg_bc);
    else if (token == "b'" || token == "c'" || token == "bc'")
        add_family(set, reg_bc_alt);
    else if (token == "d" || token == "e" || token == "de")
        add_family(set, reg_de);
    else if (token == "d'" || token == "e'" || token == "de'")
        add_family(set, reg_de_alt);
    else if (token == "h" || token == "l" || token == "hl")
        add_family(set, reg_hl);
    else if (token == "h'" || token == "l'" || token == "hl'")
        add_family(set, reg_hl_alt);
    else if (token == "ix" || token == "ixh" || token == "ixl")
        add_family(set, reg_ix);
    else if (token == "iy" || token == "iyh" || token == "iyl")
        add_family(set, reg_iy);
    else if (token == "sp")
        add_family(set, reg_sp);
}

static void add_operand_families(const std::string &operand,
                                 std::unordered_set<int> &set) {
    if (!has_memory_operand(operand) && is_immediate_or_number(operand))
        return;
    for (const auto &token : operand_tokens(operand))
        add_token_family(set, token);
}

static void mark_operand(const std::string &operand,
                         bool read_value,
                         bool write_value,
                         instruction_event &event,
                         long &memory_touches) {
    std::unordered_set<int> regs;
    add_operand_families(operand, regs);

    const bool memory = has_memory_operand(operand);
    if (memory)
        ++memory_touches;

    // Memory operands read their address registers even when the memory cell is
    // written.  The value itself is represented by memory_touches.
    if (memory) {
        for (int r : regs)
            event.reads[r] = 1;
        return;
    }

    if (read_value) {
        for (int r : regs)
            event.reads[r] = 1;
    }
    if (write_value) {
        for (int r : regs)
            event.writes[r] = 1;
    }
}

static bool is_conditional_control(const asm_line &line) {
    const std::string m = line.mnemonic;
    const std::string ops = lower_copy(trim(line.operands));
    if ((m == "jp" || m == "jr" || m == "call" || m == "ret") &&
        ops.find(',') != std::string::npos) {
        return true;
    }
    if (m == "ret" && !ops.empty())
        return true;
    return false;
}

static bool is_directive(const asm_line &line) {
    return !line.mnemonic.empty() && line.mnemonic[0] == '.';
}

static bool is_code_area(const asm_line &line, bool current) {
    if (line.mnemonic == ".text")
        return true;
    if (line.mnemonic == ".data" || line.mnemonic == ".bss")
        return false;
    if (line.mnemonic == ".area" || line.mnemonic == ".section") {
        const std::string ops = lower_copy(line.operands);
        if (ops.find("code") != std::string::npos ||
            ops.find("text") != std::string::npos) {
            return true;
        }
        if (ops.find("data") != std::string::npos ||
            ops.find("bss") != std::string::npos) {
            return false;
        }
    }
    return current;
}

static bool is_routine_label(const std::string &label) {
    if (label.empty())
        return false;
    if (label[0] == '.')
        return false;
    if (starts_with(label, "__"))
        return false;
    return true;
}

static bool is_8bit_target(const std::string &operand) {
    const std::string op = lower_copy(trim(operand));
    return op == "a" || op == "b" || op == "c" || op == "d" ||
           op == "e" || op == "h" || op == "l" ||
           op == "ixh" || op == "ixl" || op == "iyh" || op == "iyl" ||
           has_memory_operand(op);
}

static void classify_instruction(const asm_line &line,
                                 instruction_event &event,
                                 long &memory_touches,
                                 long &branches,
                                 long &calls) {
    const std::string m = line.mnemonic;
    std::string dst, src;

    auto read_flags = [&]() { event.reads[reg_f] = 1; };
    auto write_flags = [&]() { event.writes[reg_f] = 1; };
    auto touch_alt_pair = [&](reg_family main, reg_family alt) {
        event.reads[main] = event.writes[main] = 1;
        event.reads[alt] = event.writes[alt] = 1;
    };

    if (m == "ld" && split_operands(line.operands, dst, src)) {
        mark_operand(dst, has_memory_operand(dst), true, event, memory_touches);
        mark_operand(src, true, false, event, memory_touches);
    } else if ((m == "add" || m == "adc" || m == "sbc") &&
               split_operands(line.operands, dst, src)) {
        mark_operand(dst, true, true, event, memory_touches);
        mark_operand(src, true, false, event, memory_touches);
        if (m == "adc" || m == "sbc")
            read_flags();
        write_flags();
    } else if (m == "sub" || m == "and" || m == "or" ||
               m == "xor" || m == "cp") {
        event.reads[reg_a] = 1;
        if (split_operands(line.operands, dst, src)) {
            mark_operand(src, true, false, event, memory_touches);
        } else {
            mark_operand(line.operands, true, false, event, memory_touches);
        }
        if (m != "cp")
            event.writes[reg_a] = 1;
        write_flags();
    } else if (m == "inc" || m == "dec") {
        const std::string op = trim(line.operands);
        mark_operand(op, true, true, event, memory_touches);
        if (is_8bit_target(op))
            write_flags();
    } else if (m == "push") {
        mark_operand(line.operands, true, false, event, memory_touches);
        event.reads[reg_sp] = 1;
        event.writes[reg_sp] = 1;
    } else if (m == "pop") {
        mark_operand(line.operands, false, true, event, memory_touches);
        event.reads[reg_sp] = 1;
        event.writes[reg_sp] = 1;
    } else if (m == "ex") {
        const std::string ops = lower_copy(trim(line.operands));
        if (ops == "de, hl" || ops == "de,hl" ||
            ops == "hl, de" || ops == "hl,de") {
            event.reads[reg_de] = event.writes[reg_de] = 1;
            event.reads[reg_hl] = event.writes[reg_hl] = 1;
        } else if (split_operands(ops, dst, src) &&
                   ((dst == "af" && src == "af'") ||
                    (dst == "af'" && src == "af"))) {
            touch_alt_pair(reg_a, reg_a_alt);
            touch_alt_pair(reg_f, reg_f_alt);
            ++event.alternate_switches;
        } else {
            mark_operand(line.operands, true, true, event, memory_touches);
        }
    } else if (m == "exx") {
        touch_alt_pair(reg_bc, reg_bc_alt);
        touch_alt_pair(reg_de, reg_de_alt);
        touch_alt_pair(reg_hl, reg_hl_alt);
        ++event.alternate_switches;
    } else if (m == "rl" || m == "rr" || m == "rla" || m == "rra") {
        mark_operand(line.operands.empty() ? "a" : line.operands,
                     true, true, event, memory_touches);
        read_flags();
        write_flags();
    } else if (m == "sla" || m == "sra" || m == "srl" ||
               m == "rlc" || m == "rrc" ||
               m == "rlca" || m == "rrca") {
        mark_operand(line.operands.empty() ? "a" : line.operands,
                     true, true, event, memory_touches);
        write_flags();
    } else if (m == "bit") {
        if (split_operands(line.operands, dst, src))
            mark_operand(src, true, false, event, memory_touches);
        write_flags();
    } else if (m == "set" || m == "res") {
        if (split_operands(line.operands, dst, src))
            mark_operand(src, true, true, event, memory_touches);
    } else if (m == "jp" || m == "jr") {
        ++branches;
        if (is_conditional_control(line))
            read_flags();
    } else if (m == "djnz") {
        ++branches;
        event.reads[reg_bc] = 1;
        event.writes[reg_bc] = 1;
    } else if (m == "call" || m == "rst") {
        ++branches;
        ++calls;
        if (is_conditional_control(line))
            read_flags();
        event.reads[reg_sp] = 1;
        event.writes[reg_sp] = 1;
    } else if (m == "ret" || m == "reti" || m == "retn") {
        ++branches;
        if (is_conditional_control(line))
            read_flags();
        event.reads[reg_sp] = 1;
        event.writes[reg_sp] = 1;
    } else if (m == "scf") {
        write_flags();
    } else if (m == "ccf" || m == "daa") {
        read_flags();
        write_flags();
    } else if (m == "cpl" || m == "neg") {
        event.reads[reg_a] = 1;
        event.writes[reg_a] = 1;
        write_flags();
    } else if (m == "ldi" || m == "ldir" || m == "ldd" || m == "lddr") {
        event.reads[reg_hl] = event.writes[reg_hl] = 1;
        event.reads[reg_de] = event.writes[reg_de] = 1;
        event.reads[reg_bc] = event.writes[reg_bc] = 1;
        write_flags();
        memory_touches += 2;
    } else if (m == "cpi" || m == "cpir" || m == "cpd" || m == "cpdr") {
        event.reads[reg_a] = 1;
        event.reads[reg_hl] = event.writes[reg_hl] = 1;
        event.reads[reg_bc] = event.writes[reg_bc] = 1;
        write_flags();
        ++memory_touches;
    } else {
        // Unknown instructions still contribute operand coverage if they use
        // recognizable registers.
        mark_operand(line.operands, true, true, event, memory_touches);
    }
}

static long event_pressure(const instruction_event &event) {
    long count = 0;
    for (long touch : event.touches)
        if (touch)
            ++count;
    return count;
}

static void finish_event(instruction_event &event) {
    for (size_t r = 0; r < reg_count; ++r)
        event.touches[r] = event.reads[r] || event.writes[r] ? 1 : 0;
}

static bool event_touches_alternate(const instruction_event &event) {
    for (size_t r = 0; r < reg_count; ++r) {
        if (is_alternate_family(r) && event.touches[r])
            return true;
    }
    return false;
}

static void add_event(scope_builder &scope, const instruction_event &event,
                      long branches, long calls, long memory_touches) {
    ++scope.scope.instruction_count;
    scope.scope.branch_count += branches;
    scope.scope.call_count += calls;
    scope.scope.memory_touch_count += memory_touches;
    scope.scope.alternate_switch_count += event.alternate_switches;
    if (event_touches_alternate(event))
        ++scope.scope.alternate_touch_count;
    scope.pressure_sum += event_pressure(event);
    for (size_t r = 0; r < reg_count; ++r) {
        scope.reads[r] += event.reads[r];
        scope.writes[r] += event.writes[r];
        scope.touches[r] += event.touches[r];
    }
}

static register_coverage_scope finish_scope(scope_builder scope) {
    if (scope.scope.instruction_count > 0) {
        scope.scope.pressure =
            scope.pressure_sum / static_cast<double>(scope.scope.instruction_count);
    }
    scope.scope.registers.clear();
    for (size_t r = 0; r < reg_count; ++r) {
        register_touch touch;
        touch.reg = k_reg_names[r];
        touch.reads = scope.reads[r];
        touch.writes = scope.writes[r];
        touch.touches = scope.touches[r];
        scope.scope.registers.push_back(touch);
    }
    return scope.scope;
}

static register_coverage_window make_window(
        const std::vector<instruction_event> &events,
        size_t first,
        size_t count) {
    register_coverage_window window;
    window.scope = events[first].scope;
    window.first_line = events[first].line;
    window.instruction_count = static_cast<long>(count);

    std::array<long, reg_count> reads{};
    std::array<long, reg_count> writes{};
    std::array<long, reg_count> touches{};
    double pressure_sum = 0.0;
    for (size_t i = first; i < first + count; ++i) {
        pressure_sum += event_pressure(events[i]);
        window.alternate_switch_count += events[i].alternate_switches;
        if (event_touches_alternate(events[i]))
            ++window.alternate_touch_count;
        for (size_t r = 0; r < reg_count; ++r) {
            reads[r] += events[i].reads[r];
            writes[r] += events[i].writes[r];
            touches[r] += events[i].touches[r];
        }
    }
    window.pressure = pressure_sum / static_cast<double>(count);
    for (size_t r = 0; r < reg_count; ++r) {
        register_touch touch;
        touch.reg = k_reg_names[r];
        touch.reads = reads[r];
        touch.writes = writes[r];
        touch.touches = touches[r];
        window.registers.push_back(touch);
    }
    return window;
}

} // namespace

register_coverage_report analyze_z80_register_coverage(
        const std::string &asm_text,
        size_t window_size,
        size_t max_windows) {
    register_coverage_report report;
    report.total.name = "total";
    report.total.first_line = 1;

    scope_builder total;
    total.scope = report.total;

    scope_builder current;
    current.scope.name = "<file>";
    current.scope.first_line = 1;

    std::vector<instruction_event> events;
    bool in_code = true;
    bool current_started = false;

    std::istringstream in(asm_text);
    std::string raw;
    long line_no = 0;
    while (std::getline(in, raw)) {
        ++line_no;
        asm_line line = asm_line::parse(raw);
        in_code = is_code_area(line, in_code);

        if (!in_code)
            continue;
        if (!line.label.empty() && is_routine_label(line.label)) {
            if (current_started && current.scope.instruction_count > 0)
                report.routines.push_back(finish_scope(current));
            current = scope_builder{};
            current.scope.name = line.label;
            current.scope.first_line = line_no;
            current_started = true;
        }

        if (line.mnemonic.empty() || is_directive(line))
            continue;

        if (!current_started) {
            current_started = true;
            current.scope.name = "<file>";
            current.scope.first_line = line_no;
        }

        instruction_event event;
        event.scope = current.scope.name;
        event.line = line_no;
        long memory_touches = 0;
        long branches = 0;
        long calls = 0;
        classify_instruction(line, event, memory_touches, branches, calls);
        finish_event(event);

        add_event(current, event, branches, calls, memory_touches);
        add_event(total, event, branches, calls, memory_touches);
        events.push_back(event);
    }

    if (current_started && current.scope.instruction_count > 0)
        report.routines.push_back(finish_scope(current));
    report.total = finish_scope(total);

    if (window_size > 0 && events.size() >= window_size) {
        std::vector<register_coverage_window> windows;
        for (size_t i = 0; i + window_size <= events.size(); ++i) {
            bool same_scope = true;
            for (size_t j = i + 1; j < i + window_size; ++j) {
                if (events[j].scope != events[i].scope) {
                    same_scope = false;
                    break;
                }
            }
            if (same_scope)
                windows.push_back(make_window(events, i, window_size));
        }
        std::sort(windows.begin(), windows.end(),
                  [](const auto &a, const auto &b) {
                      if (a.pressure != b.pressure)
                          return a.pressure > b.pressure;
                      return a.first_line < b.first_line;
                  });
        if (windows.size() > max_windows)
            windows.resize(max_windows);
        report.hot_windows = std::move(windows);
    }

    return report;
}

} // namespace xopt
