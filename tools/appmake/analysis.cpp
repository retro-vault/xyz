#include "appmake/analysis.h"

#include <algorithm>
#include <array>
#include <deque>
#include <format>
#include <iostream>
#include <span>
#include <tuple>
#include <unordered_set>

#include "appmake/basic.h"
#include "appmake/tape.h"
#include "appmake/util.h"
#include "appmake/zx_reference.h"
#include "vendor/z80.h"

namespace appmake {

namespace {

class analysis_disasm : public z80::z80_disasm<analysis_disasm> {
public:
    const std::array<uint8_t, 65536>* memory = nullptr;
    uint16_t pc = 0;
    std::string output;

    z80::fast_u16 on_get_pc() const {
        return pc;
    }

    void on_set_pc(z80::fast_u16 value) {
        pc = static_cast<uint16_t>(value);
    }

    z80::fast_u8 on_read_next_byte() {
        const uint8_t value = (*memory)[pc];
        pc = static_cast<uint16_t>(pc + 1);
        return value;
    }

    z80::fast_u8 on_read(z80::fast_u16 addr) {
        return (*memory)[addr];
    }

    void on_emit(const char* text) {
        output = text;
    }
};

struct decoded_instruction {
    std::string text;
    uint16_t length = 0;
};

enum class branch_condition {
    none,
    nz,
    z,
    nc,
    c,
    po,
    pe,
    p,
    m,
    djnz,
};

enum class flow_kind {
    normal,
    halt,
    jump,
    call,
    ret,
    rst,
};

struct flow_info {
    flow_kind kind = flow_kind::normal;
    branch_condition cond = branch_condition::none;
    uint16_t pc = 0;
    uint16_t length = 0;
    uint16_t fallthrough = 0;
    uint16_t target = 0;
    bool has_target = false;
};

class analysis_emulator : public z80::z80_cpu<analysis_emulator> {
public:
    std::array<uint8_t, 65536> memory {};
    analysis_report* report = nullptr;
    uint16_t current_pc = 0;
    std::string current_instruction;

    z80::fast_u8 on_read(z80::fast_u16 addr) {
        if (report && addr >= k_sysvar_begin && addr < k_sysvar_end) {
            report->sysvar_dependencies.push_back({
                .pc = current_pc,
                .addr = static_cast<uint16_t>(addr),
                .instruction = current_instruction,
            });
        }
        return memory[addr];
    }

    void on_write(z80::fast_u16 addr, z80::fast_u8 value) {
        if (addr >= k_rom_top) {
            memory[addr] = static_cast<uint8_t>(value);
        }
    }

    z80::fast_u8 on_input(z80::fast_u16) {
        return 0xff;
    }

    void on_output(z80::fast_u16, z80::fast_u8) {}
};

struct analysis_state_key {
    uint16_t pc = 0;
    uint16_t sp = 0;
    uint16_t af = 0;
    uint16_t bc = 0;
    uint16_t de = 0;
    uint16_t hl = 0;
    uint16_t ix = 0;
    uint16_t iy = 0;

    bool operator==(const analysis_state_key& other) const = default;
};

struct analysis_state_hash {
    std::size_t operator()(const analysis_state_key& key) const {
        std::size_t h = key.pc;
        h = (h * 1315423911u) ^ key.sp;
        h = (h * 1315423911u) ^ key.af;
        h = (h * 1315423911u) ^ key.bc;
        h = (h * 1315423911u) ^ key.de;
        h = (h * 1315423911u) ^ key.hl;
        h = (h * 1315423911u) ^ key.ix;
        h = (h * 1315423911u) ^ key.iy;
        return h;
    }
};

struct grouped_sysvar_dependency {
    uint16_t pc = 0;
    uint16_t begin = 0;
    uint16_t end = 0;
    std::string instruction;
    std::string description;
};

struct known_u16 {
    bool known = false;
    uint16_t value = 0;

    bool operator==(const known_u16& other) const = default;
};

struct static_state {
    uint16_t pc = 0;
    known_u16 hl;
    known_u16 de;
    known_u16 ix;
    known_u16 iy;
};

struct static_state_key {
    uint16_t pc = 0;
    known_u16 hl;
    known_u16 de;
    known_u16 ix;
    known_u16 iy;

    bool operator==(const static_state_key& other) const = default;
};

struct static_state_hash {
    std::size_t operator()(const static_state_key& key) const {
        auto fold = [](std::size_t h, const known_u16& reg) {
            h = (h * 1315423911u) ^ static_cast<std::size_t>(reg.known);
            h = (h * 1315423911u) ^ reg.value;
            return h;
        };

        std::size_t h = key.pc;
        h = fold(h, key.hl);
        h = fold(h, key.de);
        h = fold(h, key.ix);
        h = fold(h, key.iy);
        return h;
    }
};

std::vector<address_range> make_loaded_data_ranges(
    const std::vector<tap_file>& files,
    const std::set<uint16_t>& code_bytes
) {
    std::vector<address_range> ranges;

    for (const auto& file : files) {
        if (file.header.type != 0x03 || file.data.empty()) {
            continue;
        }

        const uint32_t begin = file.header.param1;
        const uint32_t end = begin + static_cast<uint32_t>(file.data.size());
        auto file_ranges = invert_ranges(code_bytes, begin, end);
        ranges.insert(ranges.end(), file_ranges.begin(), file_ranges.end());
    }

    std::sort(ranges.begin(), ranges.end(), [](const address_range& a, const address_range& b) {
        return std::tie(a.begin, a.end) < std::tie(b.begin, b.end);
    });
    return ranges;
}

decoded_instruction disassemble_instruction(const std::array<uint8_t, 65536>& memory, uint16_t pc) {
    analysis_disasm disasm;
    disasm.memory = &memory;
    disasm.pc = pc;
    disasm.on_disassemble();
    return {
        .text = disasm.output,
        .length = static_cast<uint16_t>(disasm.pc - pc),
    };
}

branch_condition decode_condition(uint8_t opcode) {
    switch ((opcode >> 3) & 0x07) {
    case 0: return branch_condition::nz;
    case 1: return branch_condition::z;
    case 2: return branch_condition::nc;
    case 3: return branch_condition::c;
    case 4: return branch_condition::po;
    case 5: return branch_condition::pe;
    case 6: return branch_condition::p;
    case 7: return branch_condition::m;
    default: return branch_condition::none;
    }
}

flow_info inspect_flow(const std::array<uint8_t, 65536>& memory,
                       known_u16 hl,
                       known_u16 ix,
                       known_u16 iy,
                       uint16_t pc,
                       uint16_t length) {
    flow_info info;
    info.pc = pc;
    info.length = length;
    info.fallthrough = static_cast<uint16_t>(pc + length);

    const uint8_t op = memory[pc];
    switch (op) {
    case 0x10:
        info.kind = flow_kind::jump;
        info.cond = branch_condition::djnz;
        info.target = static_cast<uint16_t>(info.fallthrough + static_cast<int8_t>(memory[pc + 1]));
        info.has_target = true;
        return info;
    case 0x18:
        info.kind = flow_kind::jump;
        info.target = static_cast<uint16_t>(info.fallthrough + static_cast<int8_t>(memory[pc + 1]));
        info.has_target = true;
        return info;
    case 0x20:
    case 0x28:
    case 0x30:
    case 0x38:
        info.kind = flow_kind::jump;
        info.cond = decode_condition(op);
        info.target = static_cast<uint16_t>(info.fallthrough + static_cast<int8_t>(memory[pc + 1]));
        info.has_target = true;
        return info;
    case 0xc3:
        info.kind = flow_kind::jump;
        info.target = rd16(memory.data() + pc + 1);
        info.has_target = true;
        return info;
    case 0xc2:
    case 0xca:
    case 0xd2:
    case 0xda:
    case 0xe2:
    case 0xea:
    case 0xf2:
    case 0xfa:
        info.kind = flow_kind::jump;
        info.cond = decode_condition(op);
        info.target = rd16(memory.data() + pc + 1);
        info.has_target = true;
        return info;
    case 0xcd:
        info.kind = flow_kind::call;
        info.target = rd16(memory.data() + pc + 1);
        info.has_target = true;
        return info;
    case 0xc4:
    case 0xcc:
    case 0xd4:
    case 0xdc:
    case 0xe4:
    case 0xec:
    case 0xf4:
    case 0xfc:
        info.kind = flow_kind::call;
        info.cond = decode_condition(op);
        info.target = rd16(memory.data() + pc + 1);
        info.has_target = true;
        return info;
    case 0xc0:
    case 0xc8:
    case 0xd0:
    case 0xd8:
    case 0xe0:
    case 0xe8:
    case 0xf0:
    case 0xf8:
        info.kind = flow_kind::ret;
        info.cond = decode_condition(op);
        return info;
    case 0xc9:
        info.kind = flow_kind::ret;
        return info;
    case 0x76:
        info.kind = flow_kind::halt;
        return info;
    case 0xc7:
    case 0xcf:
    case 0xd7:
    case 0xdf:
    case 0xe7:
    case 0xef:
    case 0xf7:
    case 0xff:
        info.kind = flow_kind::rst;
        info.target = static_cast<uint16_t>(op & 0x38);
        info.has_target = true;
        return info;
    case 0xe9:
        info.kind = flow_kind::jump;
        if (hl.known) {
            info.target = hl.value;
            info.has_target = true;
        }
        return info;
    case 0xdd:
        if (memory[pc + 1] == 0xe9) {
            info.kind = flow_kind::jump;
            if (ix.known) {
                info.target = ix.value;
                info.has_target = true;
            }
        }
        return info;
    case 0xfd:
        if (memory[pc + 1] == 0xe9) {
            info.kind = flow_kind::jump;
            if (iy.known) {
                info.target = iy.value;
                info.has_target = true;
            }
        }
        return info;
    case 0xed:
        if (memory[pc + 1] == 0x45 || memory[pc + 1] == 0x4d) {
            info.kind = flow_kind::ret;
        }
        return info;
    default:
        return info;
    }
}

bool is_rom_addr(uint16_t addr) {
    return addr < k_rom_top;
}

bool is_loaded_code_addr(const std::vector<tap_file>& files, uint16_t addr) {
    for (const auto& file : files) {
        if (file.header.type != 0x03) {
            continue;
        }

        const uint32_t begin = file.header.param1;
        const uint32_t end = begin + static_cast<uint32_t>(file.data.size());
        if (addr >= begin && addr < end) {
            return true;
        }
    }

    return false;
}

bool condition_holds(uint8_t f, branch_condition cond) {
    switch (cond) {
    case branch_condition::nz: return (f & 0x40) == 0;
    case branch_condition::z:  return (f & 0x40) != 0;
    case branch_condition::nc: return (f & 0x01) == 0;
    case branch_condition::c:  return (f & 0x01) != 0;
    case branch_condition::po: return (f & 0x04) == 0;
    case branch_condition::pe: return (f & 0x04) != 0;
    case branch_condition::p:  return (f & 0x80) == 0;
    case branch_condition::m:  return (f & 0x80) != 0;
    default: return false;
    }
}

analysis_state_key make_state_key(const analysis_emulator& cpu) {
    return {
        .pc = static_cast<uint16_t>(cpu.get_pc()),
        .sp = static_cast<uint16_t>(cpu.get_sp()),
        .af = static_cast<uint16_t>(cpu.get_af()),
        .bc = static_cast<uint16_t>(cpu.get_bc()),
        .de = static_cast<uint16_t>(cpu.get_de()),
        .hl = static_cast<uint16_t>(cpu.get_hl()),
        .ix = static_cast<uint16_t>(cpu.get_ix()),
        .iy = static_cast<uint16_t>(cpu.get_iy()),
    };
}

void force_condition(analysis_emulator& cpu, branch_condition cond, bool taken) {
    if (cond == branch_condition::djnz) {
        cpu.set_b(taken ? 2 : 1);
        return;
    }

    uint8_t f = cpu.get_f();
    auto set_bit = [&](uint8_t mask, bool value) {
        if (value) {
            f |= mask;
        } else {
            f &= static_cast<uint8_t>(~mask);
        }
    };

    switch (cond) {
    case branch_condition::nz: set_bit(0x40, !taken); break;
    case branch_condition::z:  set_bit(0x40, taken); break;
    case branch_condition::nc: set_bit(0x01, !taken); break;
    case branch_condition::c:  set_bit(0x01, taken); break;
    case branch_condition::po: set_bit(0x04, !taken); break;
    case branch_condition::pe: set_bit(0x04, taken); break;
    case branch_condition::p:  set_bit(0x80, !taken); break;
    case branch_condition::m:  set_bit(0x80, taken); break;
    default: break;
    }

    cpu.set_f(f);
}

bool emulate_block_transfer(analysis_emulator& cpu, int direction) {
    uint32_t count = cpu.get_bc();
    uint16_t hl = static_cast<uint16_t>(cpu.get_hl());
    uint16_t de = static_cast<uint16_t>(cpu.get_de());

    if (count == 0) {
        cpu.set_pc(static_cast<uint16_t>(cpu.get_pc() + 2));
        cpu.set_bc(0);
        return true;
    }

    for (uint32_t i = 0; i < count; ++i) {
        const uint8_t value = cpu.on_read(hl);
        cpu.on_write(de, value);
        hl = static_cast<uint16_t>(hl + direction);
        de = static_cast<uint16_t>(de + direction);
    }

    cpu.set_hl(hl);
    cpu.set_de(de);
    cpu.set_bc(0);
    cpu.set_pc(static_cast<uint16_t>(cpu.get_pc() + 2));
    return true;
}

bool emulate_block_instruction(analysis_emulator& cpu) {
    const uint16_t pc = static_cast<uint16_t>(cpu.get_pc());
    if (pc == 0xffff || cpu.memory[pc] != 0xed) {
        return false;
    }

    switch (cpu.memory[static_cast<uint16_t>(pc + 1)]) {
    case 0xb0:
        return emulate_block_transfer(cpu, +1);
    case 0xb8:
        return emulate_block_transfer(cpu, -1);
    default:
        return false;
    }
}

known_u16 make_known_u16(uint16_t value) {
    return {.known = true, .value = value};
}

void advance_known_u16(known_u16& reg, int delta) {
    if (reg.known) {
        reg.value = static_cast<uint16_t>(reg.value + delta);
    }
}

void apply_static_register_effects(static_state& state,
                                   const std::array<uint8_t, 65536>& memory,
                                   uint16_t pc) {
    const uint8_t op = memory[pc];

    switch (op) {
    case 0x21:
        state.hl = make_known_u16(rd16(memory.data() + pc + 1));
        return;
    case 0x11:
        state.de = make_known_u16(rd16(memory.data() + pc + 1));
        return;
    case 0x23:
        advance_known_u16(state.hl, +1);
        return;
    case 0x2b:
        advance_known_u16(state.hl, -1);
        return;
    case 0x13:
        advance_known_u16(state.de, +1);
        return;
    case 0x1b:
        advance_known_u16(state.de, -1);
        return;
    case 0x2a:
    case 0xe1:
    case 0xe3:
    case 0x09:
    case 0x19:
    case 0x29:
    case 0x39:
        state.hl = {};
        return;
    case 0xeb:
        std::swap(state.de, state.hl);
        return;
    case 0x26:
    case 0x2e:
    case 0x44:
    case 0x45:
    case 0x4c:
    case 0x4d:
    case 0x54:
    case 0x55:
    case 0x5c:
    case 0x5d:
    case 0x60:
    case 0x61:
    case 0x62:
    case 0x63:
    case 0x64:
    case 0x65:
    case 0x66:
    case 0x67:
    case 0x68:
    case 0x69:
    case 0x6a:
    case 0x6b:
    case 0x6c:
    case 0x6d:
    case 0x6e:
    case 0x6f:
        state.hl = {};
        return;
    case 0xdd: {
        const uint8_t op2 = memory[static_cast<uint16_t>(pc + 1)];
        switch (op2) {
        case 0x21:
            state.ix = make_known_u16(rd16(memory.data() + pc + 2));
            return;
        case 0x23:
            advance_known_u16(state.ix, +1);
            return;
        case 0x2b:
            advance_known_u16(state.ix, -1);
            return;
        case 0x2a:
        case 0xe1:
        case 0xe3:
        case 0x09:
        case 0x19:
        case 0x29:
        case 0x39:
        case 0x66:
        case 0x6e:
            state.ix = {};
            return;
        default:
            return;
        }
    }
    case 0xfd: {
        const uint8_t op2 = memory[static_cast<uint16_t>(pc + 1)];
        switch (op2) {
        case 0x21:
            state.iy = make_known_u16(rd16(memory.data() + pc + 2));
            return;
        case 0x23:
            advance_known_u16(state.iy, +1);
            return;
        case 0x2b:
            advance_known_u16(state.iy, -1);
            return;
        case 0x2a:
        case 0xe1:
        case 0xe3:
        case 0x09:
        case 0x19:
        case 0x29:
        case 0x39:
        case 0x66:
        case 0x6e:
            state.iy = {};
            return;
        default:
            return;
        }
    }
    default:
        return;
    }
}

void queue_code_target(std::deque<static_state>& pending,
                       const std::vector<tap_file>& files,
                       const static_state& state,
                       uint16_t addr) {
    if (!is_rom_addr(addr) && is_loaded_code_addr(files, addr)) {
        static_state next = state;
        next.pc = addr;
        pending.push_back(next);
    }
}

void expand_static_code_paths(const std::vector<tap_file>& files,
                              const std::array<uint8_t, 65536>& memory,
                              const std::vector<uint16_t>& seed_addrs,
                              std::set<uint16_t>& code_bytes) {
    std::deque<static_state> pending;
    std::unordered_set<static_state_key, static_state_hash> seen_states;
    for (uint16_t addr : seed_addrs) {
        pending.push_back({
            .pc = addr,
            .hl = {},
            .de = {},
            .ix = {},
            .iy = {},
        });
    }

    while (!pending.empty()) {
        static_state state = pending.front();
        pending.pop_front();
        const uint16_t pc = state.pc;

        const static_state_key key {
            .pc = state.pc,
            .hl = state.hl,
            .de = state.de,
            .ix = state.ix,
            .iy = state.iy,
        };
        if (!is_loaded_code_addr(files, pc) || seen_states.contains(key)) {
            continue;
        }
        seen_states.insert(key);

        const decoded_instruction decoded = disassemble_instruction(memory, pc);
        if (decoded.length == 0) {
            continue;
        }

        for (uint16_t addr = pc; addr < static_cast<uint16_t>(pc + decoded.length); ++addr) {
            code_bytes.insert(addr);
        }

        const flow_info flow = inspect_flow(memory, state.hl, state.ix, state.iy, pc, decoded.length);
        static_state next_state = state;
        apply_static_register_effects(next_state, memory, pc);

        switch (flow.kind) {
        case flow_kind::normal:
            queue_code_target(pending, files, next_state, flow.fallthrough);
            break;
        case flow_kind::halt:
            break;
        case flow_kind::jump:
            if (flow.cond != branch_condition::none) {
                queue_code_target(pending, files, next_state, flow.fallthrough);
            }
            if (flow.has_target) {
                queue_code_target(pending, files, next_state, flow.target);
            }
            break;
        case flow_kind::call:
        case flow_kind::rst:
            queue_code_target(pending, files, next_state, flow.fallthrough);
            if (flow.has_target) {
                queue_code_target(pending, files, next_state, flow.target);
            }
            break;
        case flow_kind::ret:
            if (flow.cond != branch_condition::none) {
                queue_code_target(pending, files, next_state, flow.fallthrough);
            }
            break;
        }
    }
}

void dedupe_dependencies(analysis_report& report) {
    auto rom_cmp = [](const rom_dependency& a, const rom_dependency& b) {
        return std::tie(a.from, a.target, a.instruction) < std::tie(b.from, b.target, b.instruction);
    };
    std::sort(report.rom_dependencies.begin(), report.rom_dependencies.end(), rom_cmp);
    report.rom_dependencies.erase(
        std::unique(report.rom_dependencies.begin(), report.rom_dependencies.end(),
                    [](const rom_dependency& a, const rom_dependency& b) {
                        return a.from == b.from && a.target == b.target && a.instruction == b.instruction;
                    }),
        report.rom_dependencies.end());

    auto sys_cmp = [](const sysvar_dependency& a, const sysvar_dependency& b) {
        return std::tie(a.pc, a.addr, a.instruction) < std::tie(b.pc, b.addr, b.instruction);
    };
    std::sort(report.sysvar_dependencies.begin(), report.sysvar_dependencies.end(), sys_cmp);
    report.sysvar_dependencies.erase(
        std::unique(report.sysvar_dependencies.begin(), report.sysvar_dependencies.end(),
                    [](const sysvar_dependency& a, const sysvar_dependency& b) {
                        return a.pc == b.pc && a.addr == b.addr && a.instruction == b.instruction;
                    }),
        report.sysvar_dependencies.end());
}

std::vector<grouped_sysvar_dependency> group_sysvar_dependencies(
    const std::vector<sysvar_dependency>& deps
) {
    std::vector<grouped_sysvar_dependency> grouped;

    for (const auto& dep : deps) {
        const sysvar_info* info = lookup_sysvar(dep.addr);
        const std::string description = info
            ? (info->description.empty()
                ? std::string(info->name)
                : std::format("{} - {}", info->name, info->description))
            : format_sysvar(dep.addr);
        if (!grouped.empty()) {
            auto& last = grouped.back();
            if (last.pc == dep.pc &&
                last.instruction == dep.instruction &&
                last.description == description &&
                dep.addr == static_cast<uint16_t>(last.end + 1)) {
                last.end = dep.addr;
                continue;
            }
        }

        grouped.push_back({
            .pc = dep.pc,
            .begin = dep.addr,
            .end = dep.addr,
            .instruction = dep.instruction,
            .description = description,
        });
    }

    return grouped;
}

}  // namespace

std::vector<address_range> make_ranges_from_bytes(const std::set<uint16_t>& bytes) {
    std::vector<address_range> ranges;
    if (bytes.empty()) {
        return ranges;
    }

    auto it = bytes.begin();
    uint32_t begin = *it;
    uint32_t prev = *it;
    ++it;

    for (; it != bytes.end(); ++it) {
        if (*it != prev + 1) {
            ranges.push_back({begin, prev + 1});
            begin = *it;
        }
        prev = *it;
    }

    ranges.push_back({begin, prev + 1});
    return ranges;
}

std::vector<address_range> invert_ranges(const std::set<uint16_t>& code_bytes, uint32_t begin, uint32_t end) {
    std::vector<address_range> ranges;
    uint32_t pos = begin;

    while (pos < end) {
        while (pos < end && code_bytes.contains(static_cast<uint16_t>(pos))) {
            ++pos;
        }
        const uint32_t hole_begin = pos;
        while (pos < end && !code_bytes.contains(static_cast<uint16_t>(pos))) {
            ++pos;
        }
        if (hole_begin < pos) {
            ranges.push_back({hole_begin, pos});
        }
    }

    return ranges;
}

void analyze_program(analysis_report& report) {
    analysis_emulator seed;
    seed.report = &report;
    seed.set_pc(report.entry_addr);
    seed.set_sp(report.stack_ptr);
    seed.set_af(0);
    seed.set_bc(0);
    seed.set_de(0);
    seed.set_hl(0);
    seed.set_ix(0);
    seed.set_iy(0);
    if (report.stack_ptr + 1 < 0xffff) {
        seed.memory[report.stack_ptr] = 0x00;
        seed.memory[static_cast<uint16_t>(report.stack_ptr + 1)] = 0x00;
    }

    for (const auto& file : report.files) {
        if (file.header.type != 0x03) {
            continue;
        }

        const uint16_t load_addr = file.header.param1;
        const std::size_t max_copy = std::min<std::size_t>(file.data.size(), 0x10000 - load_addr);
        std::copy_n(file.data.begin(), max_copy, seed.memory.begin() + load_addr);
    }

    std::deque<analysis_emulator> pending;
    pending.push_back(seed);

    std::unordered_set<analysis_state_key, analysis_state_hash> seen;

    while (!pending.empty() && seen.size() < k_analysis_state_limit) {
        analysis_emulator cpu = pending.front();
        pending.pop_front();

        std::size_t steps = 0;
        while (steps++ < k_analysis_steps_per_state) {
            const uint16_t pc = cpu.get_pc();
            if (is_rom_addr(pc)) {
                break;
            }

            const analysis_state_key key = make_state_key(cpu);
            if (seen.contains(key)) {
                break;
            }
            seen.insert(key);

            const decoded_instruction decoded = disassemble_instruction(cpu.memory, pc);
            if (decoded.length == 0) {
                break;
            }

            const flow_info flow = inspect_flow(cpu.memory,
                                               make_known_u16(static_cast<uint16_t>(cpu.get_hl())),
                                               make_known_u16(static_cast<uint16_t>(cpu.get_ix())),
                                               make_known_u16(static_cast<uint16_t>(cpu.get_iy())),
                                               pc,
                                               decoded.length);
            for (uint16_t addr = pc; addr < static_cast<uint16_t>(pc + decoded.length); ++addr) {
                report.code_bytes.insert(addr);
            }
            report.instructions.push_back({pc, decoded.text});

            cpu.current_pc = pc;
            cpu.current_instruction = decoded.text;

            if (emulate_block_instruction(cpu)) {
                continue;
            }

            auto queue_state = [&](analysis_emulator next) {
                if (!is_rom_addr(next.get_pc())) {
                    pending.push_back(std::move(next));
                }
            };

            auto record_rom = [&](uint16_t target) {
                report.rom_dependencies.push_back({
                    .from = pc,
                    .target = target,
                    .instruction = decoded.text,
                });
            };

            if (flow.cond != branch_condition::none) {
                const bool currently_taken = flow.cond == branch_condition::djnz
                    ? cpu.get_b() != 1
                    : condition_holds(cpu.get_f(), flow.cond);

                for (bool taken : {false, true}) {
                    analysis_emulator branch_cpu = cpu;
                    force_condition(branch_cpu, flow.cond, taken);

                    if (taken && flow.has_target && is_rom_addr(flow.target)) {
                        record_rom(flow.target);
                        if (flow.kind == flow_kind::call || flow.kind == flow_kind::rst) {
                            branch_cpu.set_pc(flow.fallthrough);
                            queue_state(std::move(branch_cpu));
                        }
                        continue;
                    }

                    branch_cpu.current_pc = pc;
                    branch_cpu.current_instruction = decoded.text + (taken != currently_taken ? " [forced]" : "");
                    branch_cpu.on_step();

                    if (is_rom_addr(branch_cpu.get_pc())) {
                        record_rom(branch_cpu.get_pc());
                        if (flow.kind == flow_kind::call || flow.kind == flow_kind::rst) {
                            branch_cpu.set_pc(flow.fallthrough);
                            queue_state(std::move(branch_cpu));
                        }
                        continue;
                    }

                    queue_state(std::move(branch_cpu));
                }
                break;
            }

            if (flow.has_target && is_rom_addr(flow.target)) {
                record_rom(flow.target);
                if (flow.kind == flow_kind::call || flow.kind == flow_kind::rst) {
                    cpu.set_pc(flow.fallthrough);
                    continue;
                }
                break;
            }

            cpu.on_step();

            if (is_rom_addr(cpu.get_pc())) {
                record_rom(cpu.get_pc());
                break;
            }

            if (flow.kind == flow_kind::halt) {
                break;
            }
        }
    }

    dedupe_dependencies(report);
    std::sort(report.instructions.begin(), report.instructions.end());
    report.instructions.erase(
        std::unique(report.instructions.begin(), report.instructions.end(),
                    [](const auto& a, const auto& b) { return a.first == b.first; }),
        report.instructions.end());
    std::vector<uint16_t> static_seeds;
    static_seeds.push_back(report.entry_addr);
    for (const auto& [addr, _] : report.instructions) {
        static_seeds.push_back(addr);
    }
    expand_static_code_paths(report.files, seed.memory, static_seeds, report.code_bytes);
}

analysis_report analyze_tap(const fs::path& path) {
    analysis_report report;
    report.listing = parse_tap_list(path);
    report.files = parse_tap_files(path);
    report.basic = parse_basic_from_tap(path);

    if (!report.basic.usr_addr) {
        throw std::runtime_error("BASIC loader does not contain USR");
    }

    report.entry_addr = *report.basic.usr_addr;
    report.stack_ptr = report.basic.clear_addr.value_or(0x8000);

    bool found_program = false;
    for (const auto& file : report.files) {
        if (file.header.type != 0x03) {
            continue;
        }

        const uint32_t begin = file.header.param1;
        const uint32_t end = begin + file.data.size();
        if (report.entry_addr >= begin && report.entry_addr < end) {
            report.program = file;
            found_program = true;
            break;
        }
    }

    if (!found_program) {
        throw std::runtime_error("could not find CODE block containing BASIC USR entry");
    }

    analyze_program(report);
    return report;
}

analysis_report analyze_tzx(const fs::path& path) {
    analysis_report report;
    report.listing = parse_tzx_list(path);
    report.files = parse_tzx_files(path);

    std::optional<basic_program> basic;
    try {
        basic = parse_basic_from_files(report.files);
    } catch (const std::runtime_error&) {
    }

    report.basic = basic.value_or(basic_program{});

    if (!basic || !basic->usr_addr) {
        throw std::runtime_error("BASIC loader does not contain USR");
    }

    report.entry_addr = *basic->usr_addr;
    report.stack_ptr = basic->clear_addr.value_or(0x8000);

    bool found_program = false;
    for (const auto& file : report.files) {
        if (file.header.type != 0x03) {
            continue;
        }

        const uint32_t begin = file.header.param1;
        const uint32_t end = begin + file.data.size();
        if (report.entry_addr >= begin && report.entry_addr < end) {
            report.program = file;
            found_program = true;
            break;
        }
    }

    if (!found_program) {
        throw std::runtime_error("could not find CODE block containing BASIC USR entry");
    }

    analyze_program(report);
    return report;
}

void print_ranges(std::string_view title, const std::vector<address_range>& ranges) {
    std::cout << std::format("{}:\n", title);
    if (ranges.empty()) {
        std::cout << "  none\n";
        return;
    }

    for (const auto& range : ranges) {
        std::cout << std::format("  0x{:04x}-0x{:04x} ({} bytes)\n",
                                 static_cast<unsigned>(range.begin),
                                 static_cast<unsigned>(range.end - 1),
                                 range.end - range.begin);
    }
}

void print_analysis_report(const fs::path& path, const analysis_report& report) {
    std::cout << std::format("analysis: {}\n\n", path.string());
    std::cout << "basic:\n";
    print_basic_listing(report.basic);
    std::cout << "\n";

    std::cout << std::format("entry: 0x{:04x}\n", report.entry_addr);
    std::cout << std::format("stack: 0x{:04x}\n", report.stack_ptr);
    std::cout << std::format("program block: {} load=0x{:04x} size={}\n\n",
                             report.program.header.name,
                             report.program.header.param1,
                             report.program.data.size());

    std::cout << "loaded CODE blocks:\n";
    for (const auto& file : report.files) {
        if (file.header.type != 0x03) {
            continue;
        }

        const uint32_t begin = file.header.param1;
        const uint32_t end = begin + static_cast<uint32_t>(file.data.size()) - 1;
        std::cout << std::format("  {}  {:<18}  0x{:04x}-0x{:04x} ({} bytes)\n",
                                 file.header.name,
                                 zx_code_role(file.header.param1, file.header.data_len),
                                 static_cast<unsigned>(begin),
                                 static_cast<unsigned>(end),
                                 file.data.size());
    }
    std::cout << "\n";

    const auto code_ranges = make_ranges_from_bytes(report.code_bytes);
    const auto data_ranges = make_loaded_data_ranges(report.files, report.code_bytes);
    std::size_t loaded_code_bytes = 0;
    for (const auto& file : report.files) {
        if (file.header.type == 0x03) {
            loaded_code_bytes += file.data.size();
        }
    }

    std::size_t reachable_code_bytes = 0;
    for (const auto& range : code_ranges) {
        reachable_code_bytes += range.end - range.begin;
    }

    std::cout << std::format("reachable code bytes: {} / {}\n\n",
                             reachable_code_bytes,
                             loaded_code_bytes);

    print_ranges("reachable code blocks", code_ranges);
    std::cout << "\n";
    print_ranges("unreached loaded blocks", data_ranges);
    std::cout << "\n";

    std::cout << "rom dependencies:\n";
    if (report.rom_dependencies.empty()) {
        std::cout << "  none\n";
    } else {
        for (const auto& dep : report.rom_dependencies) {
            const std::string description = format_rom_routine(dep.target);
            std::cout << std::format("  0x{:04x} -> 0x{:04x}  {}{}\n",
                                     dep.from,
                                     dep.target,
                                     dep.instruction,
                                     description.empty() ? "" : std::format("  [{}]", description));
        }
    }
    std::cout << "\n";

    std::cout << "sysvar reads:\n";
    if (report.sysvar_dependencies.empty()) {
        std::cout << "  none\n";
    } else {
        for (const auto& dep : group_sysvar_dependencies(report.sysvar_dependencies)) {
            if (dep.begin == dep.end) {
                std::cout << std::format("  0x{:04x} read 0x{:04x}  {}  [{}]\n",
                                         dep.pc,
                                         dep.begin,
                                         dep.instruction,
                                         dep.description);
                continue;
            }

            std::cout << std::format("  0x{:04x} read 0x{:04x}-0x{:04x}  {}  [{}]\n",
                                     dep.pc,
                                     dep.begin,
                                     dep.end,
                                     dep.instruction,
                                     dep.description);
        }
    }
}

}  // namespace appmake
