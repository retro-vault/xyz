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
namespace {

bool mentions_temp(const icode &ic, int temp_id) {
    auto uses_temp = [&](const operand &op) {
        return op.is_temp() && op.temp_id == temp_id;
    };
    return uses_temp(ic.result) || uses_temp(ic.left) || uses_temp(ic.right);
}

bool is_cfg_barrier(const icode &ic) {
    switch (ic.op) {
    case icode_op::LABEL:
    case icode_op::GOTO:
    case icode_op::IFX:
    case icode_op::RETURN:
    case icode_op::CALL:
        return true;
    default:
        return false;
    }
}

bool is_compare_op(icode_op op) {
    switch (op) {
    case icode_op::EQ:
    case icode_op::NE:
    case icode_op::LT:
    case icode_op::LE:
    case icode_op::GT:
    case icode_op::GE:
        return true;
    default:
        return false;
    }
}

bool main_hl_def_safe(const icode &ic) {
    if (!ic.result.is_temp() || !ic.result.type || ic.result.type->size() != 2)
        return false;
    switch (ic.op) {
    case icode_op::RECEIVE:
    case icode_op::ASSIGN:
    case icode_op::CAST:
    case icode_op::NEG:
    case icode_op::BNOT:
    case icode_op::ADD:
    case icode_op::SUB:
    case icode_op::BAND:
    case icode_op::BOR:
    case icode_op::BXOR:
    case icode_op::SHL:
    case icode_op::SHR:
    case icode_op::ROL:
    case icode_op::ROR:
    case icode_op::ADDRESS_OF:
        return true;
    default:
        return false;
    }
}

bool temp_used_as_left(const icode &ic, int temp_id) {
    return ic.left.is_temp() && ic.left.temp_id == temp_id;
}

bool temp_used_as_result(const icode &ic, int temp_id) {
    return ic.result.is_temp() && ic.result.temp_id == temp_id;
}

bool immediate_use_safe_in_a(const icode &ic, int temp_id) {
    if (!temp_used_as_left(ic, temp_id))
        return false;
    switch (ic.op) {
    case icode_op::ASSIGN:
    case icode_op::CAST:
    case icode_op::ADD:
    case icode_op::SUB:
    case icode_op::BAND:
    case icode_op::BOR:
    case icode_op::BXOR:
    case icode_op::NEG:
    case icode_op::BNOT:
    case icode_op::SHL:
    case icode_op::SHR:
    case icode_op::ROL:
    case icode_op::ROR:
    case icode_op::RETURN:
        return true;
    default:
        return false;
    }
}

bool immediate_use_safe_in_hl(const icode &ic, int temp_id) {
    if (temp_used_as_left(ic, temp_id)) {
        switch (ic.op) {
        case icode_op::ASSIGN:
        case icode_op::CAST:
        case icode_op::ADD:
        case icode_op::SUB:
        case icode_op::BAND:
        case icode_op::BOR:
        case icode_op::BXOR:
        case icode_op::NEG:
        case icode_op::BNOT:
        case icode_op::SHL:
        case icode_op::SHR:
        case icode_op::ROL:
        case icode_op::ROR:
        case icode_op::GET_VALUE_AT:
        case icode_op::RETURN:
            return true;
        default:
            break;
        }
    }
    if (temp_used_as_result(ic, temp_id) && ic.op == icode_op::SET_VALUE_AT)
        return true;
    return false;
}

bool fits_ix_disp(int off) {
    return off >= -128 && off <= 127;
}

int frame_base_offset(const operand &op) {
    if (!op.is_symbol())
        return 0;
    return op.is_param ? op.stack_offset + 4 : op.stack_offset;
}

bool symbol_word_access_may_need_bc_scratch(const operand &op) {
    if (!op.is_symbol() || op.is_global)
        return false;

    int width = 2;
    if (op.type && op.type->size() == 1)
        width = 1;

    int off = frame_base_offset(op) + op.byte_offset;
    if (width == 1)
        return !fits_ix_disp(off);
    return !fits_ix_disp(off) || !fits_ix_disp(off + 1);
}

bool address_of_may_need_bc_scratch(const operand &op) {
    if (op.is_global)
        return false;

    int off = op.kind == operand_kind::TEMP ? 5 : frame_base_offset(op);
    off += op.byte_offset;
    return off < -4 || off > 4;
}

bool uses_tls_global(const operand &op) {
    return op.is_symbol() && op.is_global && op.is_tls;
}

bool is_small_safe_word_temp_result(const icode &ic, bool small_ix_frame) {
    if (!small_ix_frame)
        return false;
    if (!(ic.result.is_temp() && ic.result.type && ic.result.type->size() == 2))
        return false;

    switch (ic.op) {
    case icode_op::RECEIVE:
    case icode_op::ASSIGN:
    case icode_op::CAST:
    case icode_op::NEG:
    case icode_op::ADD:
    case icode_op::SUB:
    case icode_op::BAND:
    case icode_op::BOR:
    case icode_op::BXOR:
    case icode_op::BNOT:
        return true;
    default:
        return false;
    }
}

bool bc_backend_hazard(const icode &ic, bool small_ix_frame) {
    if (is_cfg_barrier(ic))
        return true;

    if (uses_tls_global(ic.result) || uses_tls_global(ic.left) || uses_tls_global(ic.right))
        return true;

    if (ic.op == icode_op::ADDRESS_OF && address_of_may_need_bc_scratch(ic.left))
        return true;

    if (symbol_word_access_may_need_bc_scratch(ic.result))
        return true;

    if (ic.result.is_temp() && ic.result.type && ic.result.type->size() >= 2 &&
        !is_small_safe_word_temp_result(ic, small_ix_frame))
        return true;

    return false;
}

} // namespace

bool z80_gen::clobbers_bc(const icode &ic) {
    switch (ic.op) {
    case icode_op::CALL:
    case icode_op::MUL:  case icode_op::DIV:  case icode_op::MOD:
    case icode_op::FADD: case icode_op::FSUB:
    case icode_op::FMUL: case icode_op::FDIV:
    case icode_op::FITOSF: case icode_op::FSTOI:
        return true;
    case icode_op::ROL:
    case icode_op::ROR:
        return false;
    case icode_op::SHL:
    case icode_op::SHR:
        if (ic.right.kind == operand_kind::INT_CONST) {
            const int count = static_cast<int>(ic.right.ival & 0xFF);
            return !(count <= 5 || count == 8);
        }
        return true;
    default:
        return false;
    }
}

int z80_gen::compute_temp_frame_bytes(const ir_function &fn) {
    struct temp_interval {
        int temp_id = -1;
        int first_idx = -1;
        int last_idx = -1;
        int size = 0;
        int first_region = -1;
        int last_region = -1;
    };
    struct active_slot {
        int end_idx = -1;
        int start = 0;
        int size = 0;
    };
    struct free_block {
        int start = 0;
        int size = 0;
    };

    std::unordered_map<int, temp_interval> intervals;
    std::vector<int> regions(fn.icodes.size(), 0);

    auto is_region_barrier = [](icode_op op) {
        switch (op) {
        case icode_op::GOTO:
        case icode_op::IFX:
        case icode_op::RETURN:
        case icode_op::CALL:
            return true;
        default:
            return false;
        }
    };

    int region_id = 0;
    for (size_t idx = 0; idx < fn.icodes.size(); ++idx) {
        if (idx > 0 && fn.icodes[idx].op == icode_op::LABEL)
            ++region_id;
        regions[idx] = region_id;
        if (is_region_barrier(fn.icodes[idx].op))
            ++region_id;
    }

    auto note_temp = [&](const operand &op, int idx) {
        if (!op.is_temp())
            return;

        auto home_it = temp_regs_.find(op.temp_id);
        if (home_it != temp_regs_.end() &&
            !temp_home_uses_spill_slot(home_it->second)) {
            return;
        }

        auto &iv = intervals[op.temp_id];
        iv.temp_id = op.temp_id;
        if (iv.first_idx == -1)
            iv.first_idx = idx;
        iv.last_idx = idx;
        if (iv.first_region == -1)
            iv.first_region = regions[idx];
        iv.last_region = regions[idx];

        int sz = 2;
        if (op.type && op.type->size() > 0)
            sz = op.type->size();
        if (sz < 1)
            sz = 1;
        iv.size = std::max(iv.size, sz);
    };

    auto temp_slot_reuse_safe = [&](const ir_function &fn) {
        auto touches_nonlocal_symbol = [](const operand &op) {
            return op.is_symbol() &&
                   (op.is_global || op.is_tls || op.is_sfr || op.is_func);
        };
        for (const auto &ic : fn.icodes) {
            if (touches_nonlocal_symbol(ic.result) ||
                touches_nonlocal_symbol(ic.left) ||
                touches_nonlocal_symbol(ic.right)) {
                return false;
            }
            switch (ic.op) {
            case icode_op::ADDRESS_OF:
            case icode_op::GET_VALUE_AT:
            case icode_op::SET_VALUE_AT:
            case icode_op::CALL:
            case icode_op::LABEL:
            case icode_op::GOTO:
            case icode_op::IFX:
                return false;
            default:
                break;
            }
        }
        return true;
    };

    auto assign_linear_scan_slots =
        [&](const std::vector<temp_interval> &ordered, int base_bytes) {
            std::vector<active_slot> active;
            std::vector<free_block> free_blocks;
            int high_water = 0;

            auto add_free_block = [&](int start, int size) {
                if (size <= 0)
                    return;
                free_blocks.push_back({start, size});
                std::sort(free_blocks.begin(), free_blocks.end(),
                          [](const free_block &a, const free_block &b) {
                              if (a.start != b.start)
                                  return a.start < b.start;
                              return a.size < b.size;
                          });
                std::vector<free_block> merged;
                merged.reserve(free_blocks.size());
                for (const auto &blk : free_blocks) {
                    if (!merged.empty() &&
                        merged.back().start + merged.back().size == blk.start) {
                        merged.back().size += blk.size;
                    } else {
                        merged.push_back(blk);
                    }
                }
                free_blocks.swap(merged);
            };

            for (const auto &iv : ordered) {
                for (size_t i = 0; i < active.size();) {
                    if (active[i].end_idx < iv.first_idx) {
                        add_free_block(active[i].start, active[i].size);
                        active.erase(active.begin() + static_cast<std::ptrdiff_t>(i));
                    } else {
                        ++i;
                    }
                }

                int slot_start = -1;
                for (size_t i = 0; i < free_blocks.size(); ++i) {
                    if (free_blocks[i].size < iv.size)
                        continue;
                    slot_start = free_blocks[i].start;
                    if (free_blocks[i].size == iv.size) {
                        free_blocks.erase(free_blocks.begin() +
                                          static_cast<std::ptrdiff_t>(i));
                    } else {
                        free_blocks[i].start += iv.size;
                        free_blocks[i].size -= iv.size;
                    }
                    break;
                }

                if (slot_start < 0) {
                    slot_start = high_water;
                    high_water += iv.size;
                }

                temp_slots_[iv.temp_id] =
                    -(local_bytes_ + base_bytes + slot_start + iv.size);
                active.push_back({iv.last_idx, slot_start, iv.size});
            }

            return high_water;
        };

    for (int idx = 0; idx < static_cast<int>(fn.icodes.size()); ++idx) {
        const auto &ic = fn.icodes[idx];
        note_temp(ic.result, idx);
        note_temp(ic.left, idx);
        note_temp(ic.right, idx);
    }

    if (!temp_slot_reuse_safe(fn)) {
        if (opt_settings_.level == opt_level::O3) {
            int dedicated_bytes = 0;
            std::unordered_map<int, std::vector<temp_interval>> region_locals;

            for (const auto &[tid, iv] : intervals) {
                if (iv.first_idx < 0)
                    continue;
                if (iv.first_region != -1 && iv.first_region == iv.last_region) {
                    region_locals[iv.first_region].push_back(iv);
                    continue;
                }
                temp_slots_[tid] = -(local_bytes_ + dedicated_bytes + iv.size);
                dedicated_bytes += iv.size;
            }

            int shared_region_pool = 0;
            for (auto &[rid, local_intervals] : region_locals) {
                std::sort(local_intervals.begin(), local_intervals.end(),
                          [](const temp_interval &a, const temp_interval &b) {
                              if (a.first_idx != b.first_idx)
                                  return a.first_idx < b.first_idx;
                              if (a.last_idx != b.last_idx)
                                  return a.last_idx < b.last_idx;
                              return a.temp_id < b.temp_id;
                          });
                shared_region_pool = std::max(
                    shared_region_pool,
                    assign_linear_scan_slots(local_intervals, dedicated_bytes));
            }

            next_temp_slot_ = -(dedicated_bytes + shared_region_pool);
            return dedicated_bytes + shared_region_pool;
        }

        int total = 0;
        for (const auto &[tid, iv] : intervals)
            total += iv.size;
        next_temp_slot_ = 0;
        return total;
    }

    std::vector<temp_interval> ordered;
    ordered.reserve(intervals.size());
    for (const auto &[tid, iv] : intervals) {
        if (iv.first_idx >= 0)
            ordered.push_back(iv);
    }

    std::sort(ordered.begin(), ordered.end(),
              [](const temp_interval &a, const temp_interval &b) {
                  if (a.first_idx != b.first_idx)
                      return a.first_idx < b.first_idx;
                  if (a.last_idx != b.last_idx)
                      return a.last_idx < b.last_idx;
                  return a.temp_id < b.temp_id;
              });

    int high_water = assign_linear_scan_slots(ordered, 0);
    next_temp_slot_ = -high_water;
    return high_water;
}

void z80_gen::regalloc_prepass(const ir_function &fn) {
    struct interval {
        int  first_def   = -1;
        int  last_use    = -1;
        int  size        = 0;
        int  mentions    = 0;
        bool has_addr_of = false;
        abi_arg_loc receive_loc = abi_arg_loc::STACK;
    };
    struct sym_interval {
        operand base;
        int first_idx = -1;
        int last_idx = -1;
        int mentions = 0;
        bool has_addr_of = false;
        bool unsupported = false;
        abi_arg_loc receive_loc = abi_arg_loc::STACK;
    };
    struct bc_candidate {
        int start = -1;
        int end = -1;
        int score = 0;
        bool is_symbol = false;
        int id = -1;
    };
    std::unordered_map<int, interval> ivs;
    std::unordered_map<int, sym_interval> syms;


    // Step 1: compute live intervals.
    for (int idx = 0; idx < (int)fn.icodes.size(); ++idx) {
        const icode &ic = fn.icodes[idx];
        if (ic.result.is_temp()) {
            auto &iv = ivs[ic.result.temp_id];
            if (iv.first_def == -1) iv.first_def = idx;
            if (ic.result.type) iv.size = ic.result.type->size();
            if (ic.op == icode_op::RECEIVE)
                iv.receive_loc = ic.arg_loc;
        }
        auto mark_use = [&](const operand &op, bool addr_of = false) {
            if (!op.is_temp()) return;
            auto &iv      = ivs[op.temp_id];
            iv.last_use   = idx;
            ++iv.mentions;
            if (addr_of) iv.has_addr_of = true;
        };
        auto mark_symbol = [&](const operand &op, bool addr_of = false) {
            if (!op.is_symbol() || op.is_global || op.is_func || op.is_tls || op.is_sfr)
                return;

            operand base = op;
            base.byte_offset = 0;
            int key = symbol_reg_key(base);
            auto &iv = syms[key];
            if (iv.first_idx == -1) {
                iv.base = base;
                iv.first_idx = idx;
            }
            iv.last_idx = idx;
            ++iv.mentions;
            if (addr_of)
                iv.has_addr_of = true;
            if (op.byte_offset < 0 || op.byte_offset > 1)
                iv.unsupported = true;
            if (ic.op == icode_op::RECEIVE &&
                ic.result.kind == operand_kind::SYMBOL &&
                !ic.result.is_global &&
                ic.result.stack_offset == op.stack_offset &&
                ic.result.is_param == op.is_param &&
                ic.result.name == op.name)
                iv.receive_loc = ic.arg_loc;
        };
        mark_use(ic.left,  ic.op == icode_op::ADDRESS_OF);
        mark_use(ic.right);
        if (ic.op == icode_op::SET_VALUE_AT)
            mark_use(ic.result);
        mark_symbol(ic.result);
        mark_symbol(ic.left, ic.op == icode_op::ADDRESS_OF);
        mark_symbol(ic.right);
    }

    int raw_temp_bytes = 0;
    for (const auto &[tid, iv] : ivs) {
        int sz = iv.size > 0 ? iv.size : 2;
        if (sz < 1)
            sz = 1;
        raw_temp_bytes += sz;
    }
    const bool small_ix_frame = (fn.local_bytes + raw_temp_bytes) <= 16;

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

    auto contiguous_live_window = [&](const interval &iv, int temp_id) -> bool {
        for (int k = iv.first_def; k <= iv.last_use; ++k) {
            if (!mentions_temp(fn.icodes[k], temp_id))
                return false;
        }
        return true;
    };
    std::function<bool(const operand &, int)> is_zero_u8ish;
    std::function<bool(int, int)> remat_pointer_temp_ok;
    is_zero_u8ish = [&](const operand &op, int depth) -> bool {
        if (depth > 4)
            return false;
        if (op.kind == operand_kind::INT_CONST)
            return op.ival >= 0 && op.ival <= 0xff;
        if (op.type && op.type->size() == 1 && op.type->is_unsigned())
            return true;
        if (!op.is_temp())
            return false;
        auto it = ivs.find(op.temp_id);
        if (it == ivs.end() || it->second.first_def < 0)
            return false;
        const icode &def_ic = fn.icodes[it->second.first_def];
        if (def_ic.op == icode_op::ASSIGN || def_ic.op == icode_op::CAST)
            return is_zero_u8ish(def_ic.left, depth + 1);
        return false;
    };
    remat_pointer_temp_ok = [&](int temp_id, int depth) -> bool {
        if (depth > 4)
            return false;
        auto iv_it = ivs.find(temp_id);
        if (iv_it == ivs.end())
            return false;
        const interval &iv = iv_it->second;
        if (iv.size != 2 || iv.first_def < 0 || iv.last_use <= iv.first_def)
            return false;
        const icode &def_ic = fn.icodes[iv.first_def];
        if (!def_ic.result.is_temp() || def_ic.result.temp_id != temp_id)
            return false;

        auto base_ok = [](const operand &cand) {
            return (cand.kind == operand_kind::SYMBOL &&
                    cand.is_global && !cand.is_tls &&
                    !cand.is_func && !cand.is_param) ||
                   cand.kind == operand_kind::LABEL_REF;
        };
        auto addr_temp_ok = [&](const operand &cand) {
            if (!cand.is_temp())
                return false;
            auto src_it = ivs.find(cand.temp_id);
            if (src_it == ivs.end() || src_it->second.first_def < 0)
                return false;
            const icode &src_def = fn.icodes[src_it->second.first_def];
            return src_def.op == icode_op::ADDRESS_OF;
        };

        switch (def_ic.op) {
        case icode_op::ADDRESS_OF:
            break;
        case icode_op::ADD:
            if (base_ok(def_ic.left) || addr_temp_ok(def_ic.left)) {
                if (!is_zero_u8ish(def_ic.right, depth + 1))
                    return false;
            } else if (base_ok(def_ic.right) || addr_temp_ok(def_ic.right)) {
                if (!is_zero_u8ish(def_ic.left, depth + 1))
                    return false;
            } else {
                return false;
            }
            break;
        default:
            return false;
        }

        for (int k = iv.first_def + 1; k <= iv.last_use; ++k) {
            const icode &use_ic = fn.icodes[k];
            if (use_ic.op == icode_op::GET_VALUE_AT &&
                use_ic.left.is_temp() && use_ic.left.temp_id == temp_id)
                continue;
            if (use_ic.op == icode_op::SET_VALUE_AT &&
                use_ic.result.is_temp() && use_ic.result.temp_id == temp_id)
                continue;
            return false;
        }
        return true;
    };
    auto same_local_symbol = [&](const operand &a, const operand &b) {
        return a.kind == operand_kind::SYMBOL &&
               b.kind == operand_kind::SYMBOL &&
               !a.is_global &&
               !b.is_global &&
               a.stack_offset == b.stack_offset &&
               a.is_param == b.is_param &&
               a.name == b.name;
    };
    auto mentions_symbol = [&](const icode &ic, const operand &sym) {
        return same_local_symbol(ic.result, sym) ||
               same_local_symbol(ic.left, sym) ||
               same_local_symbol(ic.right, sym);
    };
    auto contiguous_symbol_window = [&](const sym_interval &iv) -> bool {
        for (int k = iv.first_idx; k <= iv.last_idx; ++k) {
            if (!mentions_symbol(fn.icodes[k], iv.base))
                return false;
        }
        return true;
    };
    auto straight_line_helper_like = [&]() {
        if (fn.local_bytes != 0)
            return false;
        for (size_t i = 1; i + 1 < fn.icodes.size(); ++i) {
            const auto &ic = fn.icodes[i];
            switch (ic.op) {
            case icode_op::RECEIVE:
            case icode_op::ASSIGN:
            case icode_op::ADDRESS_OF:
            case icode_op::GET_VALUE_AT:
            case icode_op::SET_VALUE_AT:
            case icode_op::ADD:
            case icode_op::SUB:
            case icode_op::NEG:
            case icode_op::MUL:
            case icode_op::DIV:
            case icode_op::MOD:
            case icode_op::BAND:
            case icode_op::BOR:
            case icode_op::BXOR:
            case icode_op::BNOT:
            case icode_op::SHL:
            case icode_op::SHR:
            case icode_op::ROL:
            case icode_op::ROR:
            case icode_op::EQ:
            case icode_op::NE:
            case icode_op::LT:
            case icode_op::LE:
            case icode_op::GT:
            case icode_op::GE:
            case icode_op::CAST:
            case icode_op::RETURN:
            case icode_op::MAKE_COMPLEX:
            case icode_op::FADD:
            case icode_op::FSUB:
            case icode_op::FMUL:
            case icode_op::FDIV:
            case icode_op::FITOSF:
            case icode_op::FSTOI:
                break;
            default:
                return false;
            }
        }
        return true;
    };
    auto remat_u8_index_temp_ok = [&](int temp_id, int depth) -> bool {
        if (depth > 4)
            return false;
        auto iv_it = ivs.find(temp_id);
        if (iv_it == ivs.end())
            return false;
        const interval &iv = iv_it->second;
        if (iv.size != 2 || iv.first_def < 0 || iv.last_use <= iv.first_def)
            return false;
        const icode &def_ic = fn.icodes[iv.first_def];
        if (!def_ic.result.is_temp() || def_ic.result.temp_id != temp_id)
            return false;
        if (!(def_ic.op == icode_op::ASSIGN || def_ic.op == icode_op::CAST))
            return false;
        if (!is_zero_u8ish(def_ic.left, depth + 1))
            return false;

        auto addr_temp_ok = [&](const operand &cand) {
            if (!cand.is_temp())
                return false;
            auto src_it = ivs.find(cand.temp_id);
            if (src_it == ivs.end() || src_it->second.first_def < 0)
                return false;
            const icode &src_def = fn.icodes[src_it->second.first_def];
            if (src_def.op == icode_op::ADDRESS_OF)
                return true;
            if (src_def.op != icode_op::ADD)
                return false;
            auto base_ok = [](const operand &base) {
                return (base.kind == operand_kind::SYMBOL &&
                        base.is_global && !base.is_tls &&
                        !base.is_func && !base.is_param) ||
                       base.kind == operand_kind::LABEL_REF;
            };
            return base_ok(src_def.left) || base_ok(src_def.right);
        };

        for (int k = iv.first_def + 1; k <= iv.last_use; ++k) {
            const icode &use_ic = fn.icodes[k];
            if (use_ic.op != icode_op::ADD)
                return false;
            const operand *other = nullptr;
            if (use_ic.left.is_temp() && use_ic.left.temp_id == temp_id)
                other = &use_ic.right;
            else if (use_ic.right.is_temp() && use_ic.right.temp_id == temp_id)
                other = &use_ic.left;
            else
                return false;
            if (!(other->kind == operand_kind::SYMBOL ||
                  other->kind == operand_kind::LABEL_REF ||
                  addr_temp_ok(*other))) {
                return false;
            }
        }
        return true;
    };
    const bool helper_like_fn = straight_line_helper_like();

    struct c_candidate {
        int tid = -1;
        int start = -1;
        int end = -1;
        int score = 0;
    };
    std::vector<c_candidate> c_candidates;

    // Step 4a: gather BC candidates from both word temps and simple
    // 16-bit local / parameter symbols. The symbol path is intentionally
    // conservative: it only handles short contiguous windows where the
    // symbol can stay in BC for the whole interval without address-taking
    // or call/barrier hazards.
    std::vector<bc_candidate> bc_candidates;
    for (auto &[fd, tid] : order) {
        const interval &iv = ivs[tid];
        if (iv.size != 2)                         continue;
        if (iv.has_addr_of)                       continue;
        if (iv.last_use - iv.first_def > 6)       continue;
        if (!contiguous_live_window(iv, tid))     continue;
        if (!interior_safe(iv, bc_clob))          continue;
        bool backend_safe = true;
        for (int k = iv.first_def + 1; k <= iv.last_use - 1; ++k) {
            if (bc_backend_hazard(fn.icodes[k], small_ix_frame)) {
                backend_safe = false;
                break;
            }
        }
        if (!backend_safe)                        continue;
        bc_candidates.push_back(
            {iv.first_def, iv.last_use,
             8 + (iv.last_use - iv.first_def + 1), false, tid});
    }

    // Keep one incoming register-passed 16-bit TEMP in BC across a wider
    // straight-line helper window. This is more permissive than the generic
    // contiguous TEMP rule above, but still restricted to barrier-free
    // windows with no BC-clobbering backend hazards.
    for (auto &[fd, tid] : order) {
        const interval &iv = ivs[tid];
        if (iv.size != 2)                          continue;
        if (iv.has_addr_of)                        continue;
        if (iv.receive_loc == abi_arg_loc::STACK)  continue;
        if (iv.receive_loc == abi_arg_loc::REG_A ||
            iv.receive_loc == abi_arg_loc::REG_L ||
            iv.receive_loc == abi_arg_loc::REG_DEHL)
            continue;
        if (iv.mentions < 3)                       continue;
        if (iv.last_use <= iv.first_def)           continue;
        if (contiguous_live_window(iv, tid))       continue;
        if (!interior_safe(iv, bc_clob))           continue;
        bool backend_safe = true;
        for (int k = iv.first_def + 1; k <= iv.last_use - 1; ++k) {
            if (bc_backend_hazard(fn.icodes[k], small_ix_frame)) {
                backend_safe = false;
                break;
            }
        }
        if (!backend_safe)                         continue;
        bc_candidates.push_back(
            {iv.first_def, iv.last_use,
             24 + iv.mentions * 3 - (iv.last_use - iv.first_def),
             false, tid});
    }

    if (helper_like_fn) {
        for (auto &[fd, tid] : order) {
            const interval &iv = ivs[tid];
            if (iv.size != 2)                          continue;
            if (iv.has_addr_of)                        continue;
            if (iv.receive_loc != abi_arg_loc::REG_HL &&
                iv.receive_loc != abi_arg_loc::REG_DE)
                continue;
            if (iv.mentions < 2)                       continue;
            if (iv.last_use <= iv.first_def)           continue;
            if (!interior_safe(iv, bc_clob))           continue;
            bool backend_safe = true;
            for (int k = iv.first_def + 1; k <= iv.last_use - 1; ++k) {
                if (bc_backend_hazard(fn.icodes[k], small_ix_frame)) {
                    backend_safe = false;
                    break;
                }
            }
            if (!backend_safe)                         continue;
            bc_candidates.push_back(
                {iv.first_def, iv.last_use,
                 1100 + iv.mentions * 8 - (iv.last_use - iv.first_def),
                 false, tid});
        }
    }

    for (const auto &[key, iv] : syms) {
        if (!iv.base.type || iv.base.type->size() != 2) continue;
        if (iv.has_addr_of || iv.unsupported)            continue;
        if (iv.mentions < 3)                             continue;
        if (iv.last_idx <= iv.first_idx)                 continue;
        if (iv.base.is_param && iv.receive_loc == abi_arg_loc::STACK)
            continue;
        if (!contiguous_symbol_window(iv))               continue;
        if (!interior_safe(interval{iv.first_idx, iv.last_idx, 2, false}, bc_clob))
            continue;
        bool backend_safe = true;
        for (int k = iv.first_idx + 1; k <= iv.last_idx - 1; ++k) {
            if (bc_backend_hazard(fn.icodes[k], small_ix_frame)) {
                backend_safe = false;
                break;
            }
        }
        if (!backend_safe)                               continue;
        bc_candidates.push_back(
            {iv.first_idx, iv.last_idx,
             16 + iv.mentions * 2 - (iv.last_idx - iv.first_idx),
             true, key});
    }

    // Same idea for noncontiguous parameter-symbol windows: keep one incoming
    // 16-bit parameter resident in BC across a straight-line helper body even
    // when some interior instructions do not mention it.
    for (const auto &[key, iv] : syms) {
        if (!iv.base.type || iv.base.type->size() != 2) continue;
        if (iv.has_addr_of || iv.unsupported)            continue;
        if (!iv.base.is_param)                           continue;
        if (iv.receive_loc == abi_arg_loc::STACK)        continue;
        if (iv.receive_loc == abi_arg_loc::REG_A ||
            iv.receive_loc == abi_arg_loc::REG_L ||
            iv.receive_loc == abi_arg_loc::REG_DEHL)
            continue;
        if (iv.mentions < 2)                             continue;
        if (iv.last_idx <= iv.first_idx)                 continue;
        if (contiguous_symbol_window(iv))                continue;
        if (!interior_safe(interval{iv.first_idx, iv.last_idx, 2, false}, bc_clob))
            continue;
        bool backend_safe = true;
        for (int k = iv.first_idx + 1; k <= iv.last_idx - 1; ++k) {
            if (bc_backend_hazard(fn.icodes[k], small_ix_frame)) {
                backend_safe = false;
                break;
            }
        }
        if (!backend_safe)                               continue;
        bc_candidates.push_back(
            {iv.first_idx, iv.last_idx,
             24 + iv.mentions * 3 - (iv.last_idx - iv.first_idx),
             true, key});
    }

    if (helper_like_fn) {
        for (const auto &[key, iv] : syms) {
            if (!iv.base.type || iv.base.type->size() != 2) continue;
            if (iv.has_addr_of || iv.unsupported)            continue;
            if (!iv.base.is_param)                           continue;
            if (iv.receive_loc != abi_arg_loc::REG_HL &&
                iv.receive_loc != abi_arg_loc::REG_DE)
                continue;
            if (iv.mentions < 2)                             continue;
            if (iv.last_idx <= iv.first_idx)                 continue;
            if (!interior_safe(interval{iv.first_idx, iv.last_idx, 2, false}, bc_clob))
                continue;
            bool backend_safe = true;
            for (int k = iv.first_idx + 1; k <= iv.last_idx - 1; ++k) {
                if (bc_backend_hazard(fn.icodes[k], small_ix_frame)) {
                    backend_safe = false;
                    break;
                }
            }
            if (!backend_safe)                               continue;
            bc_candidates.push_back(
                {iv.first_idx, iv.last_idx,
                 1200 + iv.mentions * 8 - (iv.last_idx - iv.first_idx),
                 true, key});
        }
    }

    std::sort(bc_candidates.begin(), bc_candidates.end(),
              [](const bc_candidate &a, const bc_candidate &b) {
                  if (a.score != b.score) return a.score > b.score;
                  int aspan = a.end - a.start;
                  int bspan = b.end - b.start;
                  if (aspan != bspan) return aspan < bspan;
                  if (a.start != b.start) return a.start < b.start;
                  return a.is_symbol && !b.is_symbol;
              });

    std::vector<std::pair<int, int>> bc_windows;
    auto overlaps_bc = [&](int start, int end) {
        for (const auto &[s, e] : bc_windows) {
            if (!(end < s || start > e))
                return true;
        }
        return false;
    };

    if (helper_like_fn) {
        for (const auto &[key, iv] : syms) {
            if (!iv.base.type || iv.base.type->size() != 2) continue;
            if (iv.has_addr_of || iv.unsupported)            continue;
            if (!iv.base.is_param)                           continue;
            if (iv.receive_loc != abi_arg_loc::REG_HL &&
                iv.receive_loc != abi_arg_loc::REG_DE)
                continue;
            if (iv.mentions < 2)                             continue;
            if (iv.last_idx <= iv.first_idx)                 continue;
            if (!interior_safe(interval{iv.first_idx, iv.last_idx, 2, false}, bc_clob))
                continue;
            bool backend_safe = true;
            for (int k = iv.first_idx + 1; k <= iv.last_idx - 1; ++k) {
                if (bc_backend_hazard(fn.icodes[k], small_ix_frame)) {
                    backend_safe = false;
                    break;
                }
            }
            if (!backend_safe)                               continue;
            symbol_regs_[key] = temp_home::main_bc;
            bc_windows.push_back({iv.first_idx, iv.last_idx});
            break;
        }

        if (bc_windows.empty()) {
            for (auto &[fd, tid] : order) {
                const interval &iv = ivs[tid];
                if (iv.size != 2)                          continue;
                if (iv.has_addr_of)                        continue;
                if (iv.receive_loc != abi_arg_loc::REG_HL &&
                    iv.receive_loc != abi_arg_loc::REG_DE)
                    continue;
                if (iv.mentions < 2)                       continue;
                if (iv.last_use <= iv.first_def)           continue;
                if (!interior_safe(iv, bc_clob))           continue;
                bool backend_safe = true;
                for (int k = iv.first_def + 1; k <= iv.last_use - 1; ++k) {
                    if (bc_backend_hazard(fn.icodes[k], small_ix_frame)) {
                        backend_safe = false;
                        break;
                    }
                }
                if (!backend_safe)                         continue;
                temp_regs_[tid] = temp_home::main_bc;
                bc_windows.push_back({iv.first_def, iv.last_use});
                break;
            }
        }
    }

    for (const auto &cand : bc_candidates) {
        if (overlaps_bc(cand.start, cand.end))
            continue;
        if (cand.is_symbol)
            symbol_regs_[cand.id] = temp_home::main_bc;
        else
            temp_regs_[cand.id] = temp_home::main_bc;
        bc_windows.push_back({cand.start, cand.end});
    }

    if (opt_settings_.level == opt_level::O3) {
        for (auto &[fd, tid] : order) {
            const interval &iv = ivs[tid];
            if (iv.size != 1)                         continue;
            if (iv.has_addr_of)                       continue;
            if (iv.first_def < 0 || iv.last_use <= iv.first_def)
                continue;
            if (iv.mentions < 4)                      continue;
            if (temp_regs_.find(tid) != temp_regs_.end())
                continue;

            const icode &def_ic = fn.icodes[iv.first_def];
            bool def_ok = false;
            if (def_ic.op == icode_op::GET_VALUE_AT) {
                def_ok = true;
            } else if ((def_ic.op == icode_op::ASSIGN || def_ic.op == icode_op::CAST) &&
                       def_ic.left.is_temp()) {
                auto src_it = ivs.find(def_ic.left.temp_id);
                def_ok = src_it != ivs.end() &&
                         src_it->second.first_def >= 0 &&
                         fn.icodes[src_it->second.first_def].op == icode_op::GET_VALUE_AT;
            }
            if (!def_ok)                              continue;

            bool backend_safe = true;
            for (int k = iv.first_def + 1; k <= iv.last_use - 1; ++k) {
                if (bc_clob[k]) {
                    backend_safe = false;
                    break;
                }
            }
            if (!backend_safe)                        continue;

            c_candidates.push_back(
                {tid, iv.first_def, iv.last_use,
                 40 + iv.mentions * 8 - (iv.last_use - iv.first_def)});
        }

        std::sort(c_candidates.begin(), c_candidates.end(),
                  [](const c_candidate &a, const c_candidate &b) {
                      if (a.score != b.score) return a.score > b.score;
                      int aspan = a.end - a.start;
                      int bspan = b.end - b.start;
                      if (aspan != bspan) return aspan < bspan;
                      if (a.start != b.start) return a.start < b.start;
                      return a.tid < b.tid;
                  });

        for (const auto &cand : c_candidates) {
            if (overlaps_bc(cand.start, cand.end))
                continue;
            temp_regs_[cand.tid] = temp_home::main_c;
            bc_windows.push_back({cand.start, cand.end});
            break;
        }
    }

    // Step 4b: allocate a very narrow stable subset of one-step temps to
    // registers that already hold the just-computed value.
    //
    // General byte or pair allocation is still too hard to trust blindly.
    // The stable subset here only covers single-use temps whose only use is
    // in the immediately following instruction. That is enough to trim a lot
    // of store/reload traffic from byte-heavy arithmetic chains and pointer
    // address formation without opening wider liveness hazards.
    auto alt_a_use_hazard = [&](const icode &ic) {
        return is_cfg_barrier(ic) || is_compare_op(ic.op);
    };
    auto alt_a_def_safe = [&](const icode &ic) {
        if (!ic.result.is_temp() || !ic.result.type || ic.result.type->size() != 1)
            return false;
        if (is_cfg_barrier(ic) || is_compare_op(ic.op))
            return false;
        return true;
    };

    for (auto &[tid, iv] : ivs) {
        if (iv.has_addr_of)                    continue;
        if (iv.first_def < 0)                  continue;
        if (iv.last_use != iv.first_def + 1)   continue;
        if (!contiguous_live_window(iv, tid))  continue;
        const icode &def_ic = fn.icodes[iv.first_def];
        const icode &use_ic = fn.icodes[iv.last_use];
        if (alt_a_use_hazard(use_ic))          continue;

        if (iv.size == 1 && alt_a_def_safe(def_ic) &&
            immediate_use_safe_in_a(use_ic, tid)) {
            temp_regs_[tid] = temp_home::main_a;
            continue;
        }
        if (iv.size == 2 && main_hl_def_safe(def_ic) &&
            immediate_use_safe_in_hl(use_ic, tid)) {
            temp_regs_[tid] = temp_home::main_hl;
            continue;
        }
    }

    // Step 4c: rematerialize cheap pointer temporaries instead of giving
    // them IX spill slots. This follows the same general direction as SDCC:
    // addresses built from &obj or base+u8-index are often cheaper to rebuild
    // on demand than to spill and reload around every dereference.
    for (auto &[fd, tid] : order) {
        const interval &iv = ivs[tid];
        if (iv.has_addr_of)                     continue;
        if (!remat_pointer_temp_ok(tid, 0))    continue;
        temp_regs_[tid] = temp_home::remat_hl;
    }

    for (auto &[fd, tid] : order) {
        const interval &iv = ivs[tid];
        if (iv.has_addr_of)                    continue;
        if (temp_regs_.find(tid) != temp_regs_.end())
            continue;
        if (!remat_u8_index_temp_ok(tid, 0))  continue;
        temp_regs_[tid] = temp_home::remat_hl;
    }

    // NOTE: BC'/DE'/HL' via EXX are not used — EXX swaps all three pairs
    // atomically and would corrupt DE/BC that the generator uses for operand
    // loads in the same window.  An EXX-block optimizer is a planned future pass.
}

} // namespace xcc
