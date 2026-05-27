//
// z80gen_regalloc.cpp — register allocator pre-pass for the Z80 backend.
//
// Analyses each ir_function's temp live intervals and assigns short-lived
// temps to real registers (BC or A') rather than IX-frame spill slots.
//
//   BC  : 16-bit temps whose live range contains no BC-clobbering op
//   A'  : 8-bit temps whose live range contains no CALL, IFX, or ex-af op
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//
#include "backend/z80/z80gen.h"
#include <algorithm>

namespace xcc {

bool z80_gen::clobbers_bc(const icode &ic) {
    switch (ic.op) {
    case icode_op::CALL:
    case icode_op::MUL:  case icode_op::DIV:  case icode_op::MOD:
    case icode_op::SHL:  case icode_op::SHR:
    case icode_op::FADD: case icode_op::FSUB:
    case icode_op::FMUL: case icode_op::FDIV:
    case icode_op::FITOSF: case icode_op::FSTOI:
        return true;
    default:
        return false;
    }
}

void z80_gen::regalloc_prepass(const ir_function &fn) {
    struct interval {
        int  first_def   = -1;
        int  last_use    = -1;
        int  size        = 2;
        bool has_addr_of = false;
    };
    std::unordered_map<int, interval> ivs;

    // Step 1: compute live intervals.
    for (int idx = 0; idx < (int)fn.icodes.size(); ++idx) {
        const icode &ic = fn.icodes[idx];
        if (ic.result.is_temp()) {
            auto &iv = ivs[ic.result.temp_id];
            if (iv.first_def == -1) iv.first_def = idx;
            if (ic.result.type) iv.size = ic.result.type->size();
        }
        auto mark_use = [&](const operand &op, bool addr_of = false) {
            if (!op.is_temp()) return;
            auto &iv      = ivs[op.temp_id];
            iv.last_use   = idx;
            if (addr_of) iv.has_addr_of = true;
        };
        mark_use(ic.left,  ic.op == icode_op::ADDRESS_OF);
        mark_use(ic.right);
    }

    // Step 2: per-instruction clobber masks.
    int n = (int)fn.icodes.size();
    std::vector<bool> bc_clob(n, false);
    for (int idx = 0; idx < n; ++idx)
        bc_clob[idx] = clobbers_bc(fn.icodes[idx]);

    // Step 3: sort by first_def.
    std::vector<std::pair<int,int>> order; // {first_def, temp_id}
    for (auto &[tid, iv] : ivs)
        if (iv.first_def >= 0 && iv.last_use > iv.first_def)
            order.push_back({iv.first_def, tid});
    std::sort(order.begin(), order.end());

    auto interior_safe = [&](const interval &iv,
                              const std::vector<bool> &mask) -> bool {
        for (int k = iv.first_def + 1; k <= iv.last_use - 1; ++k)
            if (mask[k]) return false;
        return true;
    };

    // Step 4a: BC (main) — one 16-bit slot.
    // Skip immediately-adjacent temps (peephole rule_temp_store_reload handles those).
    int bc_busy_until = -1;
    for (auto &[fd, tid] : order) {
        const interval &iv = ivs[tid];
        if (iv.size != 2)                         continue;
        if (iv.has_addr_of)                       continue;
        if (iv.last_use - iv.first_def < 2)       continue;
        if (iv.first_def <= bc_busy_until)        continue;
        if (!interior_safe(iv, bc_clob))          continue;
        temp_regs_[tid] = temp_home::main_bc;
        bc_busy_until   = iv.last_use;
    }

    // Step 4b: A' (alt-A) — one 8-bit slot via ex af,af'.
    // Rejected if the range contains a CALL or IFX (stale F' flags).
    {
        std::vector<bool> afex_clob(n, false);
        for (int idx = 0; idx < n; ++idx) {
            const icode &ic = fn.icodes[idx];
            afex_clob[idx] = clobbers_bc(ic) || ic.op == icode_op::IFX;
        }
        int a_alt_busy = -1;
        for (auto &[fd, tid] : order) {
            const interval &iv = ivs[tid];
            if (iv.size != 1)          continue;
            if (iv.has_addr_of)        continue;
            if (temp_regs_.count(tid)) continue;
            if (iv.first_def <= a_alt_busy)       continue;
            if (!interior_safe(iv, afex_clob))    continue;
            temp_regs_[tid] = temp_home::alt_a;
            a_alt_busy      = iv.last_use;
        }
    }
    // NOTE: BC'/DE'/HL' via EXX are not used — EXX swaps all three pairs
    // atomically and would corrupt DE/BC that the generator uses for operand
    // loads in the same window.  An EXX-block optimizer is a planned future pass.
}

} // namespace xcc
