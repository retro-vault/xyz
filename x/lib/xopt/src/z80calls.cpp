//
// Prove register preservation over the final Z80 instruction graph, including
// local calls, shared tails, branches, and recursive call cycles. Only saves
// explicitly emitted by XCC for register-only calls are eligible: ordinary
// PUSH/CALL/POP sequences can instead describe arguments or stack cleanup.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//

#include "xopt/z80calls.h"
#include "xopt/z80peep.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace xopt {

namespace {

constexpr unsigned bc_mask = 1;
constexpr unsigned iy_mask = 2;
constexpr unsigned all_mask = bc_mask | iy_mask;

std::string compact(std::string text) {
    text.erase(std::remove_if(text.begin(), text.end(),
                             [](unsigned char c) { return std::isspace(c); }),
               text.end());
    std::transform(text.begin(), text.end(), text.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return text;
}

unsigned register_mask(const std::string &operand) {
    if (operand == "bc" || operand == "b" || operand == "c")
        return bc_mask;
    if (operand == "iy" || operand == "iyh" || operand == "iyl")
        return iy_mask;
    return 0;
}

// Include every destination of the undocumented indexed CB forms as well.
unsigned destination_masks(const std::string &operands) {
    unsigned mask = 0;
    size_t start = 0;
    while (start < operands.size()) {
        const size_t comma = operands.find(',', start);
        mask |= register_mask(operands.substr(start, comma - start));
        if (comma == std::string::npos)
            break;
        start = comma + 1;
    }
    return mask;
}

unsigned instruction_clobbers(const asm_line &line) {
    const std::string &mnemonic = line.mnemonic;
    const std::string operands = compact(line.operands);
    if (mnemonic == "ld" || mnemonic == "in" || mnemonic == "add" ||
        mnemonic == "adc" || mnemonic == "sbc") {
        return register_mask(operands.substr(0, operands.find(',')));
    }
    if (mnemonic == "inc" || mnemonic == "dec" || mnemonic == "pop")
        return register_mask(operands);
    if (mnemonic == "rl" || mnemonic == "rr" || mnemonic == "rlc" ||
        mnemonic == "rrc" || mnemonic == "sla" || mnemonic == "sra" ||
        mnemonic == "srl" || mnemonic == "sll" || mnemonic == "set" ||
        mnemonic == "res" || mnemonic == "ex") {
        return destination_masks(operands);
    }
    if (mnemonic == "exx" || mnemonic == "djnz" || mnemonic == "ldi" ||
        mnemonic == "ldir" || mnemonic == "ldd" || mnemonic == "lddr" ||
        mnemonic == "cpi" || mnemonic == "cpir" || mnemonic == "cpd" ||
        mnemonic == "cpdr" || mnemonic == "ini" || mnemonic == "inir" ||
        mnemonic == "ind" || mnemonic == "indr" || mnemonic == "outi" ||
        mnemonic == "otir" || mnemonic == "outd" || mnemonic == "otdr") {
        return bc_mask;
    }
    static const std::unordered_set<std::string> preserving = {
        "push", "cp", "bit", "sub", "and", "or", "xor", "neg",
        "cpl", "daa", "scf", "ccf", "rlca", "rrca", "rla", "rra",
        "nop", "out", "di", "ei", "im", "halt",
        "ret", "reti", "retn", "call", "jp", "jr"
    };
    return preserving.count(mnemonic) != 0 ? 0 : all_mask;
}

bool metadata_directive(const std::string &mnemonic) {
    return mnemonic == ".globl" || mnemonic == ".global" ||
           mnemonic == ".type" || mnemonic == ".size" ||
           mnemonic == ".optsdcc";
}

// Preserve symbol case; only the mnemonic and register spelling are folded.
std::string branch_target(const asm_line &line) {
    std::string target = line.operands;
    const size_t comma = target.rfind(',');
    if (comma != std::string::npos)
        target.erase(0, comma + 1);
    target.erase(std::remove_if(target.begin(), target.end(),
                               [](unsigned char c) { return std::isspace(c); }),
                 target.end());
    return target;
}

} // namespace

std::string remove_preserved_z80_caller_saves(const std::string &assembly) {
    if (assembly.find("xcc-caller-save:") == std::string::npos)
        return assembly;

    std::vector<asm_line> lines;
    std::istringstream input(assembly);
    std::string text;
    while (std::getline(input, text))
        lines.push_back(asm_line::parse(text));
    const size_t count = lines.size();
    std::unordered_map<std::string, size_t> labels;
    std::unordered_set<std::string> ambiguous;
    for (size_t i = 0; i < count; ++i) {
        if (lines[i].label.empty())
            continue;
        if (!labels.emplace(lines[i].label, i).second)
            ambiguous.insert(lines[i].label);
    }
    for (const std::string &label : ambiguous)
        labels.erase(label);

    // Two-bit backwards dataflow reaches a fixed point in linear time.
    // A call contributes both its continuation and its callee; a tail jump
    // contributes only its target. Clean recursive SCCs preserve the pairs,
    // while a clobber on any reachable arm propagates to every caller.
    std::vector<unsigned> effects(count, 0);
    std::vector<std::vector<size_t>> predecessors(count);
    auto add_next = [&](size_t i) {
        if (i + 1 < count)
            predecessors[i + 1].push_back(i);
        else
            effects[i] |= all_mask;
    };
    auto add_target = [&](size_t i) {
        const auto target = labels.find(branch_target(lines[i]));
        if (target == labels.end())
            effects[i] |= all_mask;
        else
            predecessors[target->second].push_back(i);
    };
    for (size_t i = 0; i < count; ++i) {
        const asm_line &line = lines[i];
        const std::string &mnemonic = line.mnemonic;
        if (line.comment == "xcc-opaque-asm")
            effects[i] = all_mask;
        if (mnemonic.empty() || metadata_directive(mnemonic)) {
            add_next(i);
            continue;
        }
        effects[i] |= instruction_clobbers(line);
        if (mnemonic == "ret" || mnemonic == "reti" || mnemonic == "retn") {
            if (!line.operands.empty())
                add_next(i);
        } else if (mnemonic == "jp" || mnemonic == "jr") {
            add_target(i);
            if (line.operands.find(',') != std::string::npos)
                add_next(i);
        } else if (mnemonic == "call" || mnemonic == "djnz") {
            add_target(i);
            add_next(i);
        } else {
            add_next(i);
        }
    }
    std::vector<size_t> work;
    for (size_t i = 0; i < count; ++i) {
        if (effects[i] != 0)
            work.push_back(i);
    }
    for (size_t next = 0; next < work.size(); ++next) {
        const size_t changed = work[next];
        for (size_t pred : predecessors[changed]) {
            const unsigned joined = effects[pred] | effects[changed];
            if (joined != effects[pred]) {
                effects[pred] = joined;
                work.push_back(pred);
            }
        }
    }

    std::vector<bool> removed(count, false);
    auto annotation = [&](size_t index) {
        const asm_line &line = lines[index];
        return line.label.empty() && line.comment != "xcc-opaque-asm" &&
               (line.mnemonic.empty() || metadata_directive(line.mnemonic));
    };
    for (size_t call = 0; call < count; ++call) {
        if (lines[call].mnemonic != "call" ||
            lines[call].operands.find(',') != std::string::npos)
            continue;
        const auto callee = labels.find(branch_target(lines[call]));
        if (callee == labels.end())
            continue;
        // Nested BC/IY saves are paired from the call outwards. Only symbol
        // declarations/comments can intervene: XCC may emit .globl at CALL.
        // A label, argument setup, or any real instruction ends the match.
        size_t before = call;
        size_t after = call;
        for (size_t distance = 1; distance <= 2; ++distance) {
            if (before == 0)
                break;
            do {
                if (before == 0)
                    break;
                --before;
            } while (annotation(before));
            do {
                ++after;
            } while (after < count && annotation(after));
            if (before == call || after >= count)
                break;
            const asm_line &push = lines[before];
            const asm_line &pop = lines[after];
            if (!push.label.empty() || !pop.label.empty() ||
                push.mnemonic != "push" || pop.mnemonic != "pop" ||
                push.operands != pop.operands ||
                push.comment != "xcc-caller-save:" + push.operands ||
                pop.comment != push.comment)
                break;
            const unsigned mask = register_mask(compact(push.operands));
            if (mask != 0 && (effects[callee->second] & mask) == 0) {
                removed[before] = true;
                removed[after] = true;
            }
        }
    }
    std::string output;
    for (size_t i = 0; i < count; ++i) {
        if (!removed[i])
            output += lines[i].to_string();
    }
    return output;
}

} // namespace xopt
