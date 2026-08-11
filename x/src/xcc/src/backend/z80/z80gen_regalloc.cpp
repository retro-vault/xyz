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
#include <set>
#include <unordered_map>
#include <unordered_set>

namespace xcc {
namespace {

bool mentions_temp(const icode &ic, int temp_id) {
    auto uses_temp = [&](const operand &op) {
        return op.is_temp() && op.temp_id == temp_id;
    };
    return uses_temp(ic.result) || uses_temp(ic.left) || uses_temp(ic.right);
}

bool same_local_symbol_base(const operand &a, const operand &b) {
    return a.kind == operand_kind::SYMBOL &&
           b.kind == operand_kind::SYMBOL &&
           !a.is_global &&
           !b.is_global &&
           !a.is_tls &&
           !b.is_tls &&
           !a.is_func &&
           !b.is_func &&
           a.is_param == b.is_param &&
           a.stack_offset == b.stack_offset &&
           a.name == b.name;
}

bool is_cfg_barrier(const icode &ic) {
    switch (ic.op) {
    case icode_op::LABEL:
    case icode_op::GOTO:
    case icode_op::IFX:
    case icode_op::RETURN:
    case icode_op::CALL:
    case icode_op::BLOCK_FILL:
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
    case icode_op::GET_VALUE_AT:
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
    if (!temp_used_as_left(ic, temp_id)) {
        const bool used_as_rhs =
            ic.right.is_temp() && ic.right.temp_id == temp_id &&
            !(ic.left.is_temp() && ic.left.temp_id == temp_id);
        return used_as_rhs && ic.op == icode_op::BXOR;
    }
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
    case icode_op::SET_VALUE_AT:
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

bool is_safe_word_temp_result_with_direct_frame(const icode &ic,
                                                bool direct_ix_frame) {
    if (!direct_ix_frame)
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
    case icode_op::ROL:
    case icode_op::ROR:
    case icode_op::SHL:
    case icode_op::SHR:
    case icode_op::GET_VALUE_AT:
        return true;
    default:
        return false;
    }
}

bool bc_backend_hazard(const icode &ic, bool direct_ix_frame) {
    if (is_cfg_barrier(ic))
        return true;

    if (uses_tls_global(ic.result) || uses_tls_global(ic.left) || uses_tls_global(ic.right))
        return true;

    if (ic.op == icode_op::ADDRESS_OF && address_of_may_need_bc_scratch(ic.left))
        return true;

    if (symbol_word_access_may_need_bc_scratch(ic.result))
        return true;

    if (ic.result.is_temp() && ic.result.type && ic.result.type->size() >= 2 &&
        !is_safe_word_temp_result_with_direct_frame(ic, direct_ix_frame))
        return true;

    return false;
}

} // namespace

bool z80_gen::clobbers_bc(const icode &ic) {
    switch (ic.op) {
    case icode_op::CALL:
    case icode_op::BLOCK_FILL:
        return true;
    case icode_op::MUL:
        if (ic.result.type && ic.result.type->size() == 1 &&
            ic.left.type && ic.left.type->size() == 1 &&
            ic.right.type && ic.right.type->size() == 1 &&
            (ic.left.kind == operand_kind::INT_CONST ||
             ic.right.kind == operand_kind::INT_CONST)) {
            return false;
        }
        return true;
    case icode_op::DIV:  case icode_op::MOD:
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
    std::unordered_set<int> no_spill_temps;
    std::unordered_map<int, abi_arg_loc> incoming_arg_locs;

    auto same_call_result_operand = [](const operand &a, const operand &b) {
        if (a.kind != b.kind)
            return false;
        switch (a.kind) {
        case operand_kind::TEMP:
            return a.temp_id == b.temp_id;
        case operand_kind::SYMBOL:
            return a.name == b.name &&
                   a.stack_offset == b.stack_offset &&
                   a.byte_offset == b.byte_offset &&
                   a.is_global == b.is_global;
        default:
            return false;
        }
    };

    auto supports_direct_call_ifx = [](call_abi abi, int size) {
        return effective_call_abi(abi) == call_abi::SDCCCALL1 &&
               (size == 1 || size == 2);
    };

    auto word_return_family = [](call_abi abi) {
        switch (effective_call_abi(abi)) {
        case call_abi::SDCCCALL0:
        case call_abi::Z88DK_CALLEE:
        case call_abi::Z88DK_SMALLC:
        case call_abi::Z88DK_FASTCALL:
            return 1; // Legacy word results use HL.
        case call_abi::SDCCCALL1:
            return 2; // Modern word results use DE.
        default:
            return 0;
        }
    };

    auto compatible_direct_return_abis =
        [&](call_abi caller, call_abi callee) {
            caller = effective_call_abi(caller);
            callee = effective_call_abi(callee);
            if (caller == callee)
                return true;
            const int caller_family = word_return_family(caller);
            return caller_family != 0 &&
                   caller_family == word_return_family(callee);
        };

    auto supports_direct_compare_return = [&](const operand &op) {
        if (!op.type)
            return false;
        const int sz = op_size(op);
        if (sz != 1 && sz != 2)
            return false;
        switch (effective_call_abi(fn.abi)) {
        case call_abi::SDCCCALL1:
        case call_abi::SDCCCALL0:
        case call_abi::Z88DK_SMALLC:
        case call_abi::Z88DK_FASTCALL:
        case call_abi::Z88DK_CALLEE:
        case call_abi::NAKED:
        case call_abi::INTERRUPT:
        case call_abi::CRITICAL:
            return true;
        default:
            return false;
        }
    };

    auto is_truth_test_preserving_integer_cast = [](const icode &ic) {
        if (ic.op != icode_op::CAST || !ic.left.type || !ic.result.type)
            return false;
        if (ic.left.type->is_far_ptr() || ic.result.type->is_far_ptr())
            return false;
        const bool src_ok = ic.left.type->is_integer() || ic.left.type->is_ptr();
        const bool dst_ok = ic.result.type->is_integer() || ic.result.type->is_ptr();
        return src_ok && dst_ok;
    };

    auto temp_used_after = [&](size_t start_idx, int temp_id) {
        for (size_t i = start_idx; i < fn.icodes.size(); ++i) {
            const auto &ic = fn.icodes[i];
            auto uses = [&](const operand &op) {
                return op.is_temp() && op.temp_id == temp_id;
            };
            if (uses(ic.left) || uses(ic.right))
                return true;
            if (uses(ic.result)) {
                if (ic.op == icode_op::SET_VALUE_AT)
                    return true;
                return false;
            }
        }
        return false;
    };

    auto has_direct_call_ifx_fallthrough_consumer =
        [&](size_t ifx_idx, const operand &value) {
            if (!value.is_temp() || ifx_idx + 2 >= fn.icodes.size())
                return false;

            const auto &ifx = fn.icodes[ifx_idx];
            if (ifx.op != icode_op::IFX ||
                !same_call_result_operand(ifx.left, value) ||
                ifx.true_lbl.empty()) {
                return false;
            }

            const auto &true_label = fn.icodes[ifx_idx + 1];
            if (true_label.op != icode_op::LABEL ||
                true_label.label_name != ifx.true_lbl) {
                return false;
            }

            size_t consumer_idx = ifx_idx + 2;
            while (consumer_idx < fn.icodes.size() &&
                   fn.icodes[consumer_idx].op == icode_op::LABEL) {
                ++consumer_idx;
            }
            if (consumer_idx >= fn.icodes.size())
                return false;

            const auto &consumer = fn.icodes[consumer_idx];
            const bool direct_return =
                consumer.op == icode_op::RETURN &&
                same_call_result_operand(consumer.left, value);
            const bool direct_send =
                consumer.op == icode_op::SEND &&
                same_call_result_operand(consumer.left, value) &&
                (consumer.arg_loc == abi_arg_loc::REG_HL ||
                 consumer.arg_loc == abi_arg_loc::REG_DE);
            if (!direct_return && !direct_send) {
                return false;
            }

            return !temp_used_after(consumer_idx + 1, value.temp_id);
        };

    auto maybe_mark_dead_incoming_arg_temp = [&](const operand &op,
                                                 size_t use_idx,
                                                 size_t after_idx) {
        if (!op.is_temp())
            return;
        if (incoming_arg_locs.find(op.temp_id) == incoming_arg_locs.end())
            return;
        for (size_t i = 0; i < use_idx; ++i) {
            const auto &earlier = fn.icodes[i];
            const auto uses = [&](const operand &candidate) {
                return candidate.is_temp() &&
                       candidate.temp_id == op.temp_id;
            };
            if (uses(earlier.left) || uses(earlier.right) ||
                (earlier.op == icode_op::SET_VALUE_AT &&
                 uses(earlier.result))) {
                return;
            }
        }
        if (!temp_used_after(after_idx, op.temp_id))
            no_spill_temps.insert(op.temp_id);
    };

    auto is_region_barrier = [](icode_op op) {
        switch (op) {
        case icode_op::GOTO:
        case icode_op::IFX:
        case icode_op::RETURN:
        case icode_op::CALL:
        case icode_op::BLOCK_FILL:
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

    for (size_t idx = 1; idx < fn.icodes.size(); ++idx) {
        const auto &ic = fn.icodes[idx];
        if (ic.op != icode_op::RECEIVE)
            break;
        if (ic.result.is_temp() && ic.arg_loc != abi_arg_loc::STACK)
            incoming_arg_locs[ic.result.temp_id] = ic.arg_loc;
    }

    for (size_t idx = 0; idx < fn.icodes.size(); ++idx) {
        const auto &ic = fn.icodes[idx];
        if (ic.op == icode_op::CALL &&
            ic.result.is_temp() &&
            !temp_used_after(idx + 1, ic.result.temp_id)) {
            no_spill_temps.insert(ic.result.temp_id);
            continue;
        }

        if (ic.op == icode_op::BAND &&
            ic.result.is_temp() &&
            idx + 1 < fn.icodes.size()) {
            const auto &next = fn.icodes[idx + 1];
            if (next.op == icode_op::RETURN &&
                same_call_result_operand(next.left, ic.result) &&
                !temp_used_after(idx + 2, ic.result.temp_id) &&
                supports_direct_compare_return(next.left)) {
                no_spill_temps.insert(ic.result.temp_id);
                continue;
            }
        }

        if (ic.op == icode_op::CALL && ic.result.is_temp() &&
            idx + 1 < fn.icodes.size()) {
            const auto &next = fn.icodes[idx + 1];

            if (next.op == icode_op::RETURN &&
                same_call_result_operand(next.left, ic.result) &&
                !temp_used_after(idx + 2, ic.result.temp_id) &&
                compatible_direct_return_abis(fn.abi, ic.callee_abi)) {
                no_spill_temps.insert(ic.result.temp_id);
                continue;
            }

            const int result_size =
                (ic.result.type && ic.result.type->size() > 0)
                    ? ic.result.type->size()
                    : 2;
            if (!supports_direct_call_ifx(ic.callee_abi, result_size))
                continue;

            if (next.op == icode_op::IFX &&
                same_call_result_operand(next.left, ic.result) &&
                (!temp_used_after(idx + 2, ic.result.temp_id) ||
                 (result_size == 2 &&
                  has_direct_call_ifx_fallthrough_consumer(idx + 1,
                                                           ic.result)))) {
                no_spill_temps.insert(ic.result.temp_id);
                continue;
            }

            if (next.op != icode_op::CAST ||
                !same_call_result_operand(next.left, ic.result) ||
                !next.result.is_temp() ||
                !is_truth_test_preserving_integer_cast(next) ||
                idx + 2 >= fn.icodes.size()) {
                continue;
            }

            const auto &ifx = fn.icodes[idx + 2];
            if (ifx.op != icode_op::IFX ||
                !same_call_result_operand(ifx.left, next.result) ||
                temp_used_after(idx + 2, ic.result.temp_id) ||
                temp_used_after(idx + 3, next.result.temp_id)) {
                continue;
            }

            no_spill_temps.insert(ic.result.temp_id);
            no_spill_temps.insert(next.result.temp_id);
            continue;
        }

        if (!is_compare_op(ic.op) || !ic.result.is_temp() ||
            idx + 1 >= fn.icodes.size()) {
            continue;
        }

        const auto &next = fn.icodes[idx + 1];
        if (compare_ifx_fusion_enabled() &&
            next.op == icode_op::IFX &&
            same_call_result_operand(next.left, ic.result) &&
            !temp_used_after(idx + 2, ic.result.temp_id)) {
            no_spill_temps.insert(ic.result.temp_id);
            maybe_mark_dead_incoming_arg_temp(ic.left, idx, idx + 2);
            maybe_mark_dead_incoming_arg_temp(ic.right, idx, idx + 2);
            continue;
        }

        if (next.op == icode_op::RETURN &&
            same_call_result_operand(next.left, ic.result) &&
            !temp_used_after(idx + 2, ic.result.temp_id) &&
            supports_direct_compare_return(next.left)) {
            no_spill_temps.insert(ic.result.temp_id);
            maybe_mark_dead_incoming_arg_temp(ic.left, idx, idx + 2);
            maybe_mark_dead_incoming_arg_temp(ic.right, idx, idx + 2);
            continue;
        }

        if (next.op == icode_op::CAST &&
            same_call_result_operand(next.left, ic.result) &&
            next.result.is_temp() &&
            is_truth_test_preserving_integer_cast(next) &&
            idx + 2 < fn.icodes.size()) {
            const auto &ret_ic = fn.icodes[idx + 2];
            if (ret_ic.op == icode_op::RETURN &&
                same_call_result_operand(ret_ic.left, next.result) &&
                !temp_used_after(idx + 2, ic.result.temp_id) &&
                !temp_used_after(idx + 3, next.result.temp_id) &&
                supports_direct_compare_return(ret_ic.left)) {
                no_spill_temps.insert(ic.result.temp_id);
                no_spill_temps.insert(next.result.temp_id);
                maybe_mark_dead_incoming_arg_temp(ic.left, idx, idx + 3);
                maybe_mark_dead_incoming_arg_temp(ic.right, idx, idx + 3);
                continue;
            }
        }
    }

    auto note_temp = [&](const operand &op, int idx) {
        if (!op.is_temp())
            return;
        if (no_spill_temps.count(op.temp_id))
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
        const int extent = sz + std::max(op.byte_offset, 0);
        iv.size = std::max(iv.size, extent);
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
            case icode_op::BLOCK_FILL:
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

    auto assign_cfg_liveness_slots =
        [&](const std::vector<temp_interval> &ordered) -> int {
            struct frame_block {
                size_t id = 0;
                size_t begin = 0;
                size_t end = 0;
                std::vector<size_t> succs;
                std::vector<size_t> preds;
            };

            auto is_terminator = [](icode_op op) {
                return op == icode_op::GOTO ||
                       op == icode_op::IFX ||
                       op == icode_op::RETURN ||
                       op == icode_op::ENDFUNCTION;
            };

            std::unordered_set<int> frame_temps;
            std::unordered_map<int, int> temp_sizes;
            frame_temps.reserve(ordered.size());
            for (const auto &iv : ordered) {
                frame_temps.insert(iv.temp_id);
                temp_sizes[iv.temp_id] = iv.size;
            }

            std::set<size_t> starts;
            starts.insert(0);
            for (size_t i = 0; i < fn.icodes.size(); ++i) {
                if (fn.icodes[i].op == icode_op::LABEL)
                    starts.insert(i);
                if (i + 1 < fn.icodes.size() && is_terminator(fn.icodes[i].op))
                    starts.insert(i + 1);
            }

            std::vector<size_t> ordered_starts(starts.begin(), starts.end());
            std::vector<frame_block> blocks;
            blocks.reserve(ordered_starts.size());
            for (size_t i = 0; i < ordered_starts.size(); ++i) {
                frame_block block;
                block.id = i;
                block.begin = ordered_starts[i];
                block.end = (i + 1 < ordered_starts.size())
                                ? ordered_starts[i + 1]
                                : fn.icodes.size();
                blocks.push_back(std::move(block));
            }

            std::unordered_map<std::string, size_t> label_to_block;
            for (const auto &block : blocks) {
                for (size_t i = block.begin; i < block.end; ++i) {
                    if (fn.icodes[i].op == icode_op::LABEL)
                        label_to_block[fn.icodes[i].label_name] = block.id;
                }
            }

            auto add_edge = [&](frame_block &block, const std::string &label) {
                auto it = label_to_block.find(label);
                if (it != label_to_block.end())
                    block.succs.push_back(it->second);
            };

            for (auto &block : blocks) {
                if (block.begin >= block.end)
                    continue;
                const icode &term = fn.icodes[block.end - 1];
                switch (term.op) {
                case icode_op::GOTO:
                    add_edge(block, term.label_name);
                    break;
                case icode_op::IFX:
                    add_edge(block, term.true_lbl);
                    if (!term.false_lbl.empty())
                        add_edge(block, term.false_lbl);
                    else if (block.id + 1 < blocks.size())
                        block.succs.push_back(block.id + 1);
                    break;
                case icode_op::RETURN:
                case icode_op::ENDFUNCTION:
                    break;
                default:
                    if (block.id + 1 < blocks.size())
                        block.succs.push_back(block.id + 1);
                    break;
                }
                std::sort(block.succs.begin(), block.succs.end());
                block.succs.erase(std::unique(block.succs.begin(),
                                              block.succs.end()),
                                  block.succs.end());
            }

            for (auto &block : blocks) {
                for (size_t succ : block.succs)
                    blocks[succ].preds.push_back(block.id);
            }

            auto add_temp = [&](std::unordered_set<int> &set,
                                const operand &op) {
                if (op.is_temp() && frame_temps.count(op.temp_id))
                    set.insert(op.temp_id);
            };

            auto add_uses_defs = [&](const icode &ic,
                                      std::unordered_set<int> &uses,
                                      std::unordered_set<int> &defs) {
                add_temp(uses, ic.left);
                add_temp(uses, ic.right);
                if (ic.op == icode_op::SET_VALUE_AT)
                    add_temp(uses, ic.result);
                else
                    add_temp(defs, ic.result);
            };

            const size_t nb = blocks.size();
            std::vector<std::unordered_set<int>> block_use(nb), block_def(nb);
            for (const auto &block : blocks) {
                auto &uses = block_use[block.id];
                auto &defs = block_def[block.id];
                for (size_t i = block.begin; i < block.end; ++i) {
                    std::unordered_set<int> ic_uses;
                    std::unordered_set<int> ic_defs;
                    add_uses_defs(fn.icodes[i], ic_uses, ic_defs);
                    for (int tid : ic_uses) {
                        if (!defs.count(tid))
                            uses.insert(tid);
                    }
                    for (int tid : ic_defs)
                        defs.insert(tid);
                }
            }

            std::vector<std::unordered_set<int>> live_in(nb), live_out(nb);
            bool changed;
            do {
                changed = false;
                for (size_t bi = nb; bi-- > 0;) {
                    std::unordered_set<int> out;
                    for (size_t succ : blocks[bi].succs) {
                        out.insert(live_in[succ].begin(), live_in[succ].end());
                    }

                    std::unordered_set<int> in = block_use[bi];
                    for (int tid : out) {
                        if (!block_def[bi].count(tid))
                            in.insert(tid);
                    }

                    if (out != live_out[bi]) {
                        live_out[bi] = std::move(out);
                        changed = true;
                    }
                    if (in != live_in[bi]) {
                        live_in[bi] = std::move(in);
                        changed = true;
                    }
                }
            } while (changed);

            std::unordered_map<int, std::unordered_set<int>> interference;
            for (int tid : frame_temps)
                interference.emplace(tid, std::unordered_set<int>{});

            auto add_interference = [&](int a, int b) {
                if (a == b)
                    return;
                interference[a].insert(b);
                interference[b].insert(a);
            };

            for (const auto &block : blocks) {
                std::unordered_set<int> live = live_out[block.id];
                for (size_t i = block.end; i-- > block.begin;) {
                    std::unordered_set<int> ic_uses;
                    std::unordered_set<int> ic_defs;
                    add_uses_defs(fn.icodes[i], ic_uses, ic_defs);

                    for (int def : ic_defs) {
                        for (int live_tid : live)
                            add_interference(def, live_tid);
                        // Z80 lowering is not uniformly destructive: a
                        // multi-byte result may be stored before every source
                        // byte has been consumed. Keep defs separate from
                        // operands even when an operand dies at this icode.
                        for (int use : ic_uses) {
                            if (temp_sizes[def] > 1 || temp_sizes[use] > 1)
                                add_interference(def, use);
                        }
                        live.erase(def);
                    }
                    for (int use : ic_uses)
                        live.insert(use);
                }
            }

            std::vector<int> color_order;
            color_order.reserve(ordered.size());
            for (const auto &iv : ordered)
                color_order.push_back(iv.temp_id);
            std::sort(color_order.begin(), color_order.end(),
                      [&](int a, int b) {
                          const size_t da = interference[a].size();
                          const size_t db = interference[b].size();
                          if (da != db) return da > db;
                          if (temp_sizes[a] != temp_sizes[b])
                              return temp_sizes[a] > temp_sizes[b];
                          return a < b;
                      });

            struct placed_slot {
                int start = 0;
                int size = 0;
            };
            std::unordered_map<int, placed_slot> placed;
            int high_water = 0;

            auto overlaps = [](int a_start, int a_size,
                               int b_start, int b_size) {
                return a_start < b_start + b_size &&
                       b_start < a_start + a_size;
            };

            for (int tid : color_order) {
                const int size = temp_sizes[tid];
                int start = 0;
                for (;;) {
                    bool conflict = false;
                    for (int other : interference[tid]) {
                        auto it = placed.find(other);
                        if (it == placed.end())
                            continue;
                        if (overlaps(start, size, it->second.start,
                                     it->second.size)) {
                            start = it->second.start + it->second.size;
                            conflict = true;
                            break;
                        }
                    }
                    if (!conflict)
                        break;
                }

                placed[tid] = {start, size};
                temp_slots_[tid] = -(local_bytes_ + start + size);
                high_water = std::max(high_water, start + size);
            }

            return high_water;
        };

    for (int idx = 0; idx < static_cast<int>(fn.icodes.size()); ++idx) {
        const auto &ic = fn.icodes[idx];
        note_temp(ic.result, idx);
        note_temp(ic.left, idx);
        note_temp(ic.right, idx);
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

    auto assign_dedicated_slots = [&]() {
        int dedicated_bytes = 0;
        for (const auto &iv : ordered) {
            temp_slots_[iv.temp_id] =
                -(local_bytes_ + dedicated_bytes + iv.size);
            dedicated_bytes += iv.size;
        }
        return dedicated_bytes;
    };

    auto has_large_eq_ifx_dispatch = [&]() {
        int run = 0;
        int best = 0;
        for (size_t i = 0; i + 1 < fn.icodes.size();) {
            const icode &cmp = fn.icodes[i];
            const icode &ifx = fn.icodes[i + 1];
            const bool cmp_const =
                cmp.op == icode_op::EQ &&
                cmp.result.is_temp() &&
                ((cmp.left.is_temp() &&
                  cmp.right.kind == operand_kind::INT_CONST) ||
                 (cmp.right.is_temp() &&
                  cmp.left.kind == operand_kind::INT_CONST));
            const bool ifx_uses_cmp =
                ifx.op == icode_op::IFX &&
                ifx.left.is_temp() &&
                ifx.left.temp_id == cmp.result.temp_id &&
                !ifx.true_lbl.empty() &&
                ifx.false_lbl.empty();
            if (cmp_const && ifx_uses_cmp) {
                ++run;
                best = std::max(best, run);
                i += 2;
                continue;
            }

            if (cmp.op == icode_op::LABEL || cmp.op == icode_op::GOTO ||
                cmp.op == icode_op::RETURN) {
                run = 0;
            }
            ++i;
        }
        return best >= 8;
    };

    if (!temp_slot_reuse_safe(fn)) {
        const int dedicated_bytes = [&]() {
            int total = 0;
            for (const auto &iv : ordered)
                total += iv.size;
            return total;
        }();

        // Dedicated slots become actively harmful in ordinary large
        // functions: frame offsets grow beyond the compact IX range and every
        // short-lived expression gets a distinct home.  The CFG allocator
        // already computes exact temp liveness and interference, so use it
        // for substantial frames in every profile, not only -Os.  Keep tiny
        // frames on the simpler path to avoid needless analysis and slot
        // churn.
        //
        // -Os previously forced every frame through the CFG allocator
        // unconditionally (via `size_opt_enabled() ||` here), regardless of
        // dedicated_bytes.  That let a placement bug in the CFG allocator's
        // interference-graph coloring corrupt an unrelated local array in
        // small-to-medium -Os frames that O2/O3/Of never routed here at all
        // (see t127_sha256_stress_regression's -Os-only "Long message" case,
        // where a spilled temp aliased a named local's storage). Until that
        // placement bug is root-caused, apply the same size threshold to
        // every optimization level so -Os only takes the CFG allocator where
        // O2/O3/Of already exercise it.
        if (dedicated_bytes >= 32 ||
            (dedicated_bytes >= 200 && has_large_eq_ifx_dispatch())) {
            int high_water = assign_cfg_liveness_slots(ordered);
            next_temp_slot_ = -high_water;
            return high_water;
        }

        int dedicated = assign_dedicated_slots();
        next_temp_slot_ = -dedicated;
        return dedicated;
    }

    int high_water = assign_linear_scan_slots(ordered, 0);
    next_temp_slot_ = -high_water;
    return high_water;
}

void z80_gen::regalloc_prepass(const ir_function &fn) {
    temp_regs_.clear();
    symbol_regs_.clear();
    incoming_symbol_homes_.clear();
    iy_preserved_call_indices_.clear();
    bc_preserved_call_indices_.clear();

    // The physical-home allocator currently reasons about byte, word, and
    // 32-bit values.  A function containing a wider value can call helpers
    // whose 64-bit register convention uses both the main and alternate
    // register sets; assigning an otherwise unrelated narrow temporary to
    // one of those homes can corrupt a value across that helper sequence.
    // Keep the normal frame allocator for the complete function until wide
    // helper clobbers are represented explicitly in allocator liveness.
    const auto is_wide = [](const operand &op) {
        return op.type && op.type->size() > 4;
    };
    for (const auto &ic : fn.icodes) {
        // ADDRESS_OF consumes an object but produces only a target pointer.
        // The operand retains the object's declared type, so an array or
        // structure can be much larger than the machine value manipulated by
        // this instruction.  Do not mistake that object size for a live wide
        // scalar and disable allocation for the whole function.
        const bool wide_left_value =
            ic.op != icode_op::ADDRESS_OF && is_wide(ic.left);
        if (is_wide(ic.result) || wide_left_value || is_wide(ic.right))
            return;
    }

    struct interval {
        int  first_def   = -1;
        int  last_use    = -1;
        int  size        = 0;
        int  mentions    = 0;
        int  definitions = 0;
        bool has_addr_of = false;
        bool loop_extended = false;
        abi_arg_loc receive_loc = abi_arg_loc::STACK;
        int weighted_mentions = 0;
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
            ++iv.definitions;
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

    // CFG coloring without register homes includes every temporary, so it is
    // a safe upper bound for the final colored frame and much tighter than
    // summing mutually exclusive SSA-like values.  Use it for every tuned
    // profile: speed code generation also uses the preallocated coloured
    // frame, and treating mutually exclusive switch-arm temps as simultaneous
    // needlessly disables otherwise safe BC live ranges.
    int frame_upper_bound = compute_temp_frame_bytes(fn);
    temp_slots_.clear();
    next_temp_slot_ = 0;
    const bool direct_ix_frame =
        (fn.local_bytes + frame_upper_bound) <= 120;

    // Step 2: per-instruction clobber masks.
    int n = (int)fn.icodes.size();
    std::vector<bool> bc_clob(n, false);
    for (int idx = 0; idx < n; ++idx)
        bc_clob[idx] = clobbers_bc(fn.icodes[idx]);

    std::unordered_map<std::string, int> label_indices;
    for (int idx = 0; idx < n; ++idx) {
        const icode &ic = fn.icodes[idx];
        if (ic.op == icode_op::LABEL)
            label_indices[ic.label_name] = idx;
    }

    // Estimate dynamic spill cost from loop nesting.  A textual mention in a
    // loop is more expensive than a cold straight-line mention; nested loops
    // receive progressively more weight, capped to keep heuristic scores
    // stable on deeply nested or irreducible control flow.
    std::vector<int> loop_depth(n, 0);
    auto mark_loop_depth = [&](const std::string &label, int backedge_idx) {
        auto it = label_indices.find(label);
        if (it == label_indices.end() || it->second >= backedge_idx)
            return;
        for (int k = it->second; k <= backedge_idx; ++k)
            loop_depth[k] = std::min(loop_depth[k] + 1, 3);
    };
    for (int idx = 0; idx < n; ++idx) {
        const icode &ic = fn.icodes[idx];
        if (ic.op == icode_op::GOTO) {
            mark_loop_depth(ic.label_name, idx);
        } else if (ic.op == icode_op::IFX) {
            mark_loop_depth(ic.true_lbl, idx);
            mark_loop_depth(ic.false_lbl, idx);
        }
    }
    for (auto &[tid, iv] : ivs)
        iv.weighted_mentions = iv.mentions;
    for (int idx = 0; idx < n; ++idx) {
        const int bonus = (1 << loop_depth[idx]) - 1;
        if (bonus == 0)
            continue;
        auto add_use_bonus = [&](const operand &op) {
            if (op.is_temp())
                ivs[op.temp_id].weighted_mentions += bonus;
        };
        add_use_bonus(fn.icodes[idx].left);
        add_use_bonus(fn.icodes[idx].right);
        if (fn.icodes[idx].op == icode_op::SET_VALUE_AT)
            add_use_bonus(fn.icodes[idx].result);
    }
    auto hot_mentions = [&](const interval &iv) {
        // -Os values compactness over dynamic spill frequency; retain its
        // established textual-use ordering and apply loop weighting to the
        // speed/balanced profiles.
        return size_opt_enabled()
                   ? iv.mentions
                   : std::max(iv.mentions, iv.weighted_mentions);
    };

    auto extend_loop_carried_symbols = [&]() {
        auto extend_for_backedge = [&](const std::string &label,
                                       int backedge_idx) {
            auto lit = label_indices.find(label);
            if (lit == label_indices.end() || lit->second >= backedge_idx)
                return;

            const int loop_begin = lit->second;
            std::unordered_set<int> mentioned;
            auto note = [&](const operand &op) {
                if (!op.is_symbol() || op.is_global || op.is_func ||
                    op.is_tls || op.is_sfr) {
                    return;
                }
                operand base = op;
                base.byte_offset = 0;
                mentioned.insert(symbol_reg_key(base));
            };

            for (int k = loop_begin; k <= backedge_idx; ++k) {
                const icode &cur = fn.icodes[k];
                note(cur.result);
                note(cur.left);
                note(cur.right);
            }

            for (int key : mentioned) {
                auto it = syms.find(key);
                if (it == syms.end() || it->second.first_idx > backedge_idx)
                    continue;
                it->second.last_idx =
                    std::max(it->second.last_idx, backedge_idx);
            }
        };

        for (int idx = 0; idx < n; ++idx) {
            const icode &ic = fn.icodes[idx];
            if (ic.op == icode_op::GOTO) {
                extend_for_backedge(ic.label_name, idx);
            } else if (ic.op == icode_op::IFX) {
                extend_for_backedge(ic.true_lbl, idx);
                extend_for_backedge(ic.false_lbl, idx);
            }
        }
    };
    extend_loop_carried_symbols();

    auto extend_loop_invariant_temps = [&]() {
        auto extend_for_backedge = [&](const std::string &label, int backedge_idx) {
            auto lit = label_indices.find(label);
            if (lit == label_indices.end())
                return;
            const int loop_begin = lit->second;
            if (loop_begin >= backedge_idx)
                return;

            std::unordered_set<int> defined_in_loop;
            std::unordered_set<int> used_in_loop;
            for (int k = loop_begin; k <= backedge_idx; ++k) {
                const icode &cur = fn.icodes[k];
                if (cur.result.is_temp())
                    defined_in_loop.insert(cur.result.temp_id);
                if (cur.op == icode_op::SET_VALUE_AT && cur.result.is_temp())
                    used_in_loop.insert(cur.result.temp_id);
                if (cur.left.is_temp())
                    used_in_loop.insert(cur.left.temp_id);
                if (cur.right.is_temp())
                    used_in_loop.insert(cur.right.temp_id);
            }

            for (int tid : used_in_loop) {
                auto it = ivs.find(tid);
                if (it == ivs.end())
                    continue;
                interval &iv = it->second;
                if (iv.first_def < 0 || iv.first_def >= loop_begin)
                    continue;
                if (defined_in_loop.count(tid))
                    continue;
                if (iv.last_use < backedge_idx) {
                    iv.last_use = backedge_idx;
                    iv.loop_extended = true;
                }
            }
        };

        for (int idx = 0; idx < n; ++idx) {
            const icode &ic = fn.icodes[idx];
            if (ic.op == icode_op::GOTO) {
                extend_for_backedge(ic.label_name, idx);
            } else if (ic.op == icode_op::IFX) {
                extend_for_backedge(ic.true_lbl, idx);
                extend_for_backedge(ic.false_lbl, idx);
            }
        }
    };
    extend_loop_invariant_temps();

    // Pair a reusable word index in DE with the word loaded through its
    // scaled global address in HL.  This is the natural Z80 register layout
    // for search/table probes: DE survives address formation, while HL can
    // carry the loaded value through an equality branch and a following
    // ordered comparison.  Treat the pair as one allocation decision so the
    // independent IY and loaded-value passes cannot choose conflicting homes.
    for (const auto &[tid, iv] : ivs) {
        if (iv.size != 2 || iv.has_addr_of || iv.definitions != 1 ||
            iv.first_def < 0 || iv.last_use <= iv.first_def ||
            iv.mentions < 3 || temp_regs_.find(tid) != temp_regs_.end()) {
            continue;
        }
        const icode &def = fn.icodes[iv.first_def];
        if (!def.result.is_temp() || def.result.temp_id != tid ||
            (def.op != icode_op::SHR && def.op != icode_op::SHL &&
             def.op != icode_op::ADD && def.op != icode_op::SUB)) {
            continue;
        }

        int scale_idx = -1;
        bool index_uses_safe = true;
        for (int k = iv.first_def + 1; k <= iv.last_use; ++k) {
            const icode &ic = fn.icodes[k];
            const bool left = ic.left.is_temp() && ic.left.temp_id == tid;
            const bool right = ic.right.is_temp() && ic.right.temp_id == tid;
            const bool result = ic.result.is_temp() && ic.result.temp_id == tid;
            if (!left && !right && !result)
                continue;
            const bool scale_use =
                ic.op == icode_op::SHL && left && !right && !result &&
                ic.right.kind == operand_kind::INT_CONST &&
                ic.right.ival >= 1 && ic.right.ival <= 7 &&
                ic.result.is_temp();
            const bool small_update =
                (ic.op == icode_op::ADD || ic.op == icode_op::SUB) &&
                left && !right && !result &&
                ic.right.kind == operand_kind::INT_CONST;
            const bool direct_return =
                ic.op == icode_op::RETURN && left && !right && !result;
            if (scale_use && scale_idx < 0)
                scale_idx = k;
            else if (!small_update && !direct_return) {
                index_uses_safe = false;
                break;
            }
        }
        if (!index_uses_safe || scale_idx < 0 || scale_idx + 2 >= n)
            continue;

        const icode &scale = fn.icodes[scale_idx];
        const icode &address = fn.icodes[scale_idx + 1];
        const icode &load = fn.icodes[scale_idx + 2];
        if (address.op != icode_op::ADD || !address.result.is_temp() ||
            load.op != icode_op::GET_VALUE_AT || !load.result.is_temp() ||
            !load.left.is_temp() ||
            load.left.temp_id != address.result.temp_id ||
            !load.right.is_none() || op_size(load.result) != 2) {
            continue;
        }
        const operand *base = nullptr;
        if (address.left.is_temp() &&
            address.left.temp_id == scale.result.temp_id)
            base = &address.right;
        else if (address.right.is_temp() &&
                 address.right.temp_id == scale.result.temp_id)
            base = &address.left;
        if (!base)
            continue;

        bool global_base = base->kind == operand_kind::SYMBOL &&
                           base->is_global && !base->is_tls && !base->is_func;
        if (!global_base && base->is_temp()) {
            auto bit = ivs.find(base->temp_id);
            if (bit != ivs.end() && bit->second.first_def >= 0) {
                const icode &base_def = fn.icodes[bit->second.first_def];
                global_base = base_def.op == icode_op::ADDRESS_OF &&
                              base_def.left.kind == operand_kind::SYMBOL &&
                              base_def.left.is_global &&
                              !base_def.left.is_tls &&
                              !base_def.left.is_func;
            }
        }
        if (!global_base)
            continue;

        auto loaded_iv = ivs.find(load.result.temp_id);
        if (loaded_iv == ivs.end() || loaded_iv->second.size != 2 ||
            loaded_iv->second.has_addr_of ||
            loaded_iv->second.first_def != scale_idx + 2 ||
            loaded_iv->second.last_use <= scale_idx + 2 ||
            temp_regs_.find(load.result.temp_id) != temp_regs_.end()) {
            continue;
        }
        int compared_value_tid = load.result.temp_id;
        int compare_scan_begin = scale_idx + 3;
        bool coalesced_transform = false;
        if (scale_idx + 3 < n) {
            const icode &transform = fn.icodes[scale_idx + 3];
            const bool load_on_left = transform.left.is_temp() &&
                transform.left.temp_id == load.result.temp_id;
            const bool load_on_right = transform.right.is_temp() &&
                transform.right.temp_id == load.result.temp_id;
            if (transform.op == icode_op::BAND &&
                load_on_left != load_on_right && transform.result.is_temp() &&
                ((load_on_left &&
                  transform.right.kind == operand_kind::INT_CONST) ||
                 (load_on_right &&
                  transform.left.kind == operand_kind::INT_CONST))) {
                auto transformed_iv = ivs.find(transform.result.temp_id);
                if (loaded_iv->second.last_use == scale_idx + 3 &&
                    transformed_iv != ivs.end() &&
                    transformed_iv->second.size == 2 &&
                    transformed_iv->second.first_def == scale_idx + 3 &&
                    temp_regs_.find(transform.result.temp_id) ==
                        temp_regs_.end()) {
                    compared_value_tid = transform.result.temp_id;
                    compare_scan_begin = scale_idx + 4;
                    coalesced_transform = true;
                }
            }
        }

        auto compared_iv = ivs.find(compared_value_tid);
        if (compared_iv == ivs.end() ||
            compared_iv->second.last_use < compare_scan_begin)
            continue;
        bool loaded_uses_safe = true;
        int comparisons = 0;
        std::unordered_set<int> compared_temp_ids;
        for (int k = compare_scan_begin;
             k <= compared_iv->second.last_use; ++k) {
            const icode &ic = fn.icodes[k];
            const bool left = ic.left.is_temp() &&
                              ic.left.temp_id == compared_value_tid;
            const bool right = ic.right.is_temp() &&
                               ic.right.temp_id == compared_value_tid;
            const bool result = ic.result.is_temp() &&
                                ic.result.temp_id == compared_value_tid;
            if (left || right || result) {
                if (result || left == right || !is_compare_op(ic.op)) {
                    loaded_uses_safe = false;
                    break;
                }
                const operand &other = left ? ic.right : ic.left;
                if (other.is_temp())
                    compared_temp_ids.insert(other.temp_id);
                ++comparisons;
                continue;
            }
            if (ic.op != icode_op::LABEL && ic.op != icode_op::IFX &&
                ic.op != icode_op::GOTO && ic.op != icode_op::RETURN) {
                loaded_uses_safe = false;
                break;
            }
        }
        if (!loaded_uses_safe || comparisons < 2)
            continue;

        temp_regs_[tid] = temp_home::main_de;
        temp_regs_[load.result.temp_id] = temp_home::main_hl;
        if (coalesced_transform)
            temp_regs_[compared_value_tid] = temp_home::main_hl;
        // Ordinary comparison operands use a compact frame slot so the paired
        // index in DE survives.  Leave a register-passed loop invariant
        // unassigned: the direct HL-vs-IY compare path can preserve DE and
        // keep that value resident for the complete loop.
        for (int other_tid : compared_temp_ids) {
            auto other = ivs.find(other_tid);
            if (other != ivs.end() && other->second.size == 2 &&
                temp_regs_.find(other_tid) == temp_regs_.end()) {
                const interval &other_iv = other->second;
                const bool incoming_loop_invariant =
                    other_iv.loop_extended && other_iv.first_def >= 0 &&
                    fn.icodes[other_iv.first_def].op == icode_op::RECEIVE &&
                    (other_iv.receive_loc == abi_arg_loc::REG_HL ||
                     other_iv.receive_loc == abi_arg_loc::REG_DE);
                if (!incoming_loop_invariant)
                    temp_regs_[other_tid] = temp_home::stack;
            }
        }
    }

    struct iy_candidate {
        int tid = -1;
        int start = -1;
        int end = -1;
        int score = 0;
    };
    std::vector<iy_candidate> iy_candidates;
    std::vector<iy_candidate> spare_iy_cursor_candidates;
    for (const auto &[tid, iv] : ivs) {
        if (iv.size != 2 || iv.has_addr_of || iv.first_def < 0)
            continue;

        const icode &init = fn.icodes[iv.first_def];
        const auto near_pointer = [](const operand &op) {
            return op.type && op.type->is_ptr() &&
                   !op.type->is_far_ptr();
        };
        const auto narrow_integer = [](const operand &op) {
            return op.kind == operand_kind::INT_CONST ||
                   (op.type && op.type->is_integer() &&
                    op.type->size() > 0 && op.type->size() <= 2);
        };
        const bool pointer_arithmetic_init =
            (init.op == icode_op::ADD || init.op == icode_op::SUB) &&
            ((near_pointer(init.left) && narrow_integer(init.right)) ||
             (init.op == icode_op::ADD && near_pointer(init.right) &&
              narrow_integer(init.left)));
        if (!init.result.is_temp() || init.result.temp_id != tid ||
            !init.result.type || !init.result.type->is_ptr() ||
            init.result.type->is_far_ptr() || !init.result.type->base ||
            init.result.type->base->is_volatile ||
            (init.op != icode_op::ASSIGN &&
             init.op != icode_op::ADDRESS_OF &&
             init.op != icode_op::CAST &&
             !pointer_arithmetic_init &&
             !(init.op == icode_op::RECEIVE &&
               (init.arg_loc == abi_arg_loc::REG_HL ||
                init.arg_loc == abi_arg_loc::REG_DE)))) {
            continue;
        }

        int last_cursor_touch = iv.first_def;
        int loop_end = -1;
        for (int k = iv.first_def + 1; k < n; ++k) {
            if (mentions_temp(fn.icodes[k], tid))
                last_cursor_touch = k;

            auto note_backedge = [&](const std::string &label) {
                auto it = label_indices.find(label);
                if (it == label_indices.end())
                    return;
                if (it->second > iv.first_def && it->second <= last_cursor_touch &&
                    it->second < k) {
                    loop_end = std::max(loop_end, k);
                }
            };
            if (fn.icodes[k].op == icode_op::GOTO) {
                note_backedge(fn.icodes[k].label_name);
            } else if (fn.icodes[k].op == icode_op::IFX) {
                note_backedge(fn.icodes[k].true_lbl);
                note_backedge(fn.icodes[k].false_lbl);
            }
        }
        if (loop_end < 0)
            continue;

        bool saw_mem = false;
        bool saw_update = false;
        bool saw_commit = false;
        bool safe = true;
        std::unordered_set<int> step_temps;
        std::unordered_set<int> chase_temps;
        for (int k = iv.first_def + 1; k <= loop_end; ++k) {
            const icode &ic = fn.icodes[k];
            if (ic.op == icode_op::CALL) {
                // IY is caller-clobbered, but a direct call with no stack
                // arguments can preserve it locally without changing the
                // callee ABI or argument layout.  The exact save sites are
                // recorded after register assignment below.
                if (ic.func_name.empty() || ic.arg_bytes != 0) {
                    safe = false;
                    break;
                }
                continue;
            }
            if (ic.op == icode_op::ALLOCA || ic.op == icode_op::INLINE_ASM) {
                safe = false;
                break;
            }

            auto far_pointer = [](const operand &op) {
                return op.type && op.type->is_ptr() && op.type->is_far_ptr();
            };
            if (far_pointer(ic.result) || far_pointer(ic.left) ||
                far_pointer(ic.right)) {
                safe = false;
                break;
            }

            // Null-terminated walks test the cursor itself at the loop head.
            // The backend can branch directly on an IY-resident pointer, and
            // the test neither changes nor aliases the cursor.
            if (ic.op == icode_op::IFX && ic.left.is_temp() &&
                ic.left.temp_id == tid) {
                continue;
            }

            if (ic.op == icode_op::SEND && ic.left.is_temp() &&
                ic.left.temp_id == tid &&
                (ic.arg_loc == abi_arg_loc::REG_HL ||
                 ic.arg_loc == abi_arg_loc::REG_DE)) {
                continue;
            }

            if (is_compare_op(ic.op) &&
                ((ic.left.is_temp() && ic.left.temp_id == tid) ||
                 (ic.right.is_temp() && ic.right.temp_id == tid)) &&
                !(ic.result.is_temp() && ic.result.temp_id == tid)) {
                continue;
            }

            if (ic.op == icode_op::GET_VALUE_AT && ic.left.is_temp() &&
                ic.left.temp_id == tid && ic.right.is_none() &&
                ic.result.type &&
                (ic.result.type->size() == 1 || ic.result.type->size() == 2)) {
                saw_mem = true;
                // A loop cursor may advance by following a link instead of by
                // adding a constant: `next = *cursor; cursor = next`.  This is
                // still a genuine loop-carried pointer and benefits from the
                // same IY residency as a strided walk.  Record only near
                // pointer loads so an unrelated scalar reload cannot become a
                // cursor update.
                if (ic.result.is_temp() && ic.result.type->is_ptr() &&
                    !ic.result.type->is_far_ptr()) {
                    chase_temps.insert(ic.result.temp_id);
                    saw_update = true;
                }
                continue;
            }
            if (ic.op == icode_op::SET_VALUE_AT && ic.result.is_temp() &&
                ic.result.temp_id == tid && ic.right.is_none() &&
                ic.left.type &&
                (ic.left.type->size() == 1 || ic.left.type->size() == 2)) {
                saw_mem = true;
                continue;
            }
            const bool scalar_step =
                ic.right.kind == operand_kind::INT_CONST ||
                (ic.right.type && ic.right.type->is_integer() &&
                 ic.right.type->size() > 0 && ic.right.type->size() <= 2);
            if ((ic.op == icode_op::ADD || ic.op == icode_op::SUB) &&
                ic.left.is_temp() && ic.left.temp_id == tid &&
                scalar_step && ic.result.is_temp()) {
                step_temps.insert(ic.result.temp_id);
                saw_update = true;
                if (ic.result.temp_id == tid)
                    saw_commit = true;
                continue;
            }
            if (ic.op == icode_op::ASSIGN && ic.left.is_temp() &&
                ic.left.temp_id == tid && ic.result.is_temp() &&
                ic.result.temp_id != tid && ic.result.type &&
                ic.result.type->is_ptr() &&
                !ic.result.type->is_far_ptr()) {
                // Read-only snapshots such as `previous = cursor` do not end
                // cursor residency.  The copy is materialized normally.
                continue;
            }
            if (ic.op == icode_op::ASSIGN && ic.result.is_temp() &&
                ic.result.temp_id == tid && ic.left.is_temp() &&
                (step_temps.count(ic.left.temp_id) ||
                 chase_temps.count(ic.left.temp_id))) {
                saw_commit = true;
                continue;
            }
            if (ic.op == icode_op::ASSIGN && ic.result.is_temp() &&
                ic.result.temp_id == tid && ic.left.is_temp() &&
                ic.left.type && ic.left.type->is_ptr() &&
                !ic.left.type->is_far_ptr()) {
                auto source = ivs.find(ic.left.temp_id);
                if (source != ivs.end() && source->second.first_def >= 0 &&
                    source->second.first_def < iv.first_def) {
                    // A cursor can be reset to the same loop-invariant base
                    // for a second pass over a chain.  The base predates the
                    // cursor live range, so this is not an unproven update.
                    continue;
                }
            }
            if (mentions_temp(ic, tid)) {
                safe = false;
                break;
            }
        }
        if (!safe || !saw_mem || !saw_update || !saw_commit)
            continue;

        iy_candidates.push_back(
            {tid, iv.first_def, loop_end,
             2400 + hot_mentions(iv) * 20 - (loop_end - iv.first_def)});
    }

    // Keep a register-passed near pointer in IY across a straight-line leaf
    // window when every use is a direct dereference. This is the non-loop
    // counterpart of the cursor allocation above: it avoids repeatedly
    // spilling and reloading callback/state pointers around arithmetic that
    // needs HL/DE/BC, while rejecting calls, branches and pointer arithmetic.
    for (const auto &[tid, iv] : ivs) {
        if (iv.size != 2 || iv.has_addr_of || iv.first_def < 0 ||
            iv.last_use <= iv.first_def || iv.mentions < 2) {
            continue;
        }

        const icode &init = fn.icodes[iv.first_def];
        if (init.op != icode_op::RECEIVE ||
            !init.result.is_temp() || init.result.temp_id != tid ||
            (init.arg_loc != abi_arg_loc::REG_HL &&
             init.arg_loc != abi_arg_loc::REG_DE) ||
            !init.result.type || !init.result.type->is_ptr() ||
            init.result.type->is_far_ptr() || !init.result.type->base ||
            init.result.type->base->is_volatile) {
            continue;
        }

        bool saw_load = false;
        bool saw_store = false;
        bool safe = true;
        for (int k = iv.first_def + 1; k <= iv.last_use; ++k) {
            const icode &ic = fn.icodes[k];
            if (ic.op == icode_op::CALL || ic.op == icode_op::ALLOCA ||
                ic.op == icode_op::INLINE_ASM || ic.op == icode_op::LABEL ||
                ic.op == icode_op::GOTO || ic.op == icode_op::IFX ||
                ic.op == icode_op::RETURN) {
                safe = false;
                break;
            }
            if (ic.op == icode_op::GET_VALUE_AT &&
                ic.left.is_temp() && ic.left.temp_id == tid &&
                ic.right.is_none() && ic.result.type &&
                (ic.result.type->size() == 1 || ic.result.type->size() == 2)) {
                saw_load = true;
                continue;
            }
            if (ic.op == icode_op::SET_VALUE_AT &&
                ic.result.is_temp() && ic.result.temp_id == tid &&
                ic.right.is_none() && ic.left.type &&
                (ic.left.type->size() == 1 || ic.left.type->size() == 2)) {
                saw_store = true;
                continue;
            }
            if (mentions_temp(ic, tid)) {
                safe = false;
                break;
            }
        }
        if (!safe || !saw_load || !saw_store)
            continue;

        iy_candidates.push_back(
            {tid, iv.first_def, iv.last_use,
             2200 + hot_mentions(iv) * 20 - (iv.last_use - iv.first_def)});
    }

    // Keep a stationary near-pointer base in IY when a hot loop repeatedly
    // accesses the base and constant-offset pointers derived from it.  C
    // front ends commonly hoist `p + field_offset` before the loop; treating
    // those derived temporaries as IY displacements avoids reloading five or
    // six separate frame-resident pointers on every aggregate update.  Every
    // derived pointer must be single-definition and dereference-only, and the
    // complete window must be free of calls or opaque code.
    for (const auto &[tid, iv] : ivs) {
        if (opt_settings_.level != opt_level::Of &&
            opt_settings_.level != opt_level::O3 &&
            opt_settings_.level != opt_level::Os)
            break;
        if (iv.size != 2 || iv.has_addr_of || iv.first_def < 0 ||
            iv.last_use <= iv.first_def) {
            continue;
        }
        auto existing_home = temp_regs_.find(tid);
        if (existing_home != temp_regs_.end() &&
            existing_home->second != temp_home::arg_hl &&
            existing_home->second != temp_home::arg_de) {
            continue;
        }

        const icode &init = fn.icodes[iv.first_def];
        if (!init.result.is_temp() || init.result.temp_id != tid ||
            !init.result.type || !init.result.type->is_ptr() ||
            init.result.type->is_far_ptr() || !init.result.type->base ||
            init.result.type->base->is_volatile ||
            (init.op != icode_op::ASSIGN &&
             init.op != icode_op::ADDRESS_OF &&
             init.op != icode_op::CAST &&
             !(init.op == icode_op::RECEIVE &&
               (init.arg_loc == abi_arg_loc::REG_HL ||
                init.arg_loc == abi_arg_loc::REG_DE)))) {
            continue;
        }

        bool safe = true;
        int hot_accesses = 0;
        int window_end = iv.last_use;
        std::unordered_set<int> derived_tids;
        for (int k = iv.first_def + 1; k <= iv.last_use; ++k) {
            const icode &ic = fn.icodes[k];
            if (!mentions_temp(ic, tid))
                continue;

            const bool direct_load =
                ic.op == icode_op::GET_VALUE_AT && ic.left.is_temp() &&
                ic.left.temp_id == tid && ic.right.is_none() &&
                (op_size(ic.result) == 1 || op_size(ic.result) == 2 ||
                 op_size(ic.result) == 4);
            const bool direct_store =
                ic.op == icode_op::SET_VALUE_AT && ic.result.is_temp() &&
                ic.result.temp_id == tid && ic.right.is_none() &&
                (op_size(ic.left) == 1 || op_size(ic.left) == 2 ||
                 op_size(ic.left) == 4);
            if (direct_load || direct_store) {
                ++hot_accesses;
                continue;
            }

            if (ic.op != icode_op::ADD || !ic.result.is_temp() ||
                !ic.result.type || !ic.result.type->is_ptr() ||
                ic.result.type->is_far_ptr()) {
                safe = false;
                break;
            }
            const operand *base = &ic.left;
            const operand *offset = &ic.right;
            if (base->kind == operand_kind::INT_CONST)
                std::swap(base, offset);
            if (!base->is_temp() || base->temp_id != tid ||
                offset->kind != operand_kind::INT_CONST ||
                offset->ival < -128 || offset->ival > 126) {
                safe = false;
                break;
            }

            auto derived_iv = ivs.find(ic.result.temp_id);
            if (derived_iv == ivs.end() ||
                derived_iv->second.first_def != k ||
                derived_iv->second.last_use <= k) {
                safe = false;
                break;
            }
            derived_tids.insert(ic.result.temp_id);
            window_end = std::max(window_end, derived_iv->second.last_use);
        }
        if (!safe)
            continue;

        for (int derived_tid : derived_tids) {
            const interval &derived_iv = ivs.at(derived_tid);
            for (int k = derived_iv.first_def + 1;
                 k <= derived_iv.last_use; ++k) {
                const icode &ic = fn.icodes[k];
                if (!mentions_temp(ic, derived_tid))
                    continue;
                const bool load =
                    ic.op == icode_op::GET_VALUE_AT && ic.left.is_temp() &&
                    ic.left.temp_id == derived_tid && ic.right.is_none() &&
                    (op_size(ic.result) == 1 || op_size(ic.result) == 2 ||
                     op_size(ic.result) == 4);
                const bool store =
                    ic.op == icode_op::SET_VALUE_AT &&
                    ic.result.is_temp() &&
                    ic.result.temp_id == derived_tid && ic.right.is_none() &&
                    (op_size(ic.left) == 1 || op_size(ic.left) == 2 ||
                     op_size(ic.left) == 4);
                if (!load && !store) {
                    safe = false;
                    break;
                }
                ++hot_accesses;
            }
            if (!safe)
                break;
        }
        if (!safe || hot_accesses < 4)
            continue;

        for (int k = iv.first_def + 1; k <= window_end; ++k) {
            const icode &ic = fn.icodes[k];
            if (ic.op == icode_op::CALL || ic.op == icode_op::ALLOCA ||
                ic.op == icode_op::INLINE_ASM) {
                safe = false;
                break;
            }
        }
        if (!safe)
            continue;

        iy_candidates.push_back(
            {tid, iv.first_def, window_end,
             3000 + hot_accesses * 140 - (window_end - iv.first_def)});
    }

    // A branch-hoisted computed pointer is another natural IY value.  Switch
    // and parser arms frequently dereference the same table address, while
    // the ordinary arithmetic pairs are needed to form local addresses and
    // values inside each arm.  Require a single definition, three direct
    // loads in distinct blocks, and no other use or opaque barrier throughout
    // the live range.  This is independent of how the expression was exposed
    // (PRE, source factoring, or hand-written code).
    for (const auto &[tid, iv] : ivs) {
        if (iv.size != 2 || iv.has_addr_of || iv.definitions != 1 ||
            iv.first_def < 0 || iv.last_use <= iv.first_def ||
            temp_regs_.find(tid) != temp_regs_.end()) {
            continue;
        }
        const icode &def = fn.icodes[iv.first_def];
        if (def.op != icode_op::ADD || !def.result.is_temp() ||
            def.result.temp_id != tid || !def.result.type ||
            !def.result.type->is_ptr() || def.result.type->is_far_ptr() ||
            !def.result.type->base || def.result.type->base->is_volatile) {
            continue;
        }

        bool safe = true;
        int direct_loads = 0;
        std::unordered_set<int> load_regions;
        int region = 0;
        for (int k = iv.first_def + 1; k <= iv.last_use; ++k) {
            const icode &ic = fn.icodes[k];
            if (ic.op == icode_op::LABEL)
                ++region;
            if (ic.op == icode_op::CALL || ic.op == icode_op::ALLOCA ||
                ic.op == icode_op::INLINE_ASM) {
                safe = false;
                break;
            }
            if (!mentions_temp(ic, tid))
                continue;
            const bool direct_load =
                ic.op == icode_op::GET_VALUE_AT && ic.left.is_temp() &&
                ic.left.temp_id == tid && ic.right.is_none() &&
                ic.result.type &&
                (ic.result.type->size() == 1 ||
                 ic.result.type->size() == 2);
            if (!direct_load) {
                safe = false;
                break;
            }
            ++direct_loads;
            load_regions.insert(region);
        }
        if (!safe || direct_loads < 3 || load_regions.size() < 3)
            continue;

        iy_candidates.push_back(
            {tid, iv.first_def, iv.last_use,
             2600 + direct_loads * 120 + hot_mentions(iv) * 16 -
                 (iv.last_use - iv.first_def)});
    }

    // Keep a computed byte address in IY when a conditional update reads and
    // then writes through the same pointer.  Without this home the address is
    // normally spilled before the branch and rebuilt or reloaded for the
    // store.  Calls and opaque operations remain barriers, and every use of
    // the pointer must be one of the two direct memory accesses.
    for (const auto &[tid, iv] : ivs) {
        if (iv.size != 2 || iv.has_addr_of || iv.first_def < 0 ||
            iv.last_use <= iv.first_def ||
            loop_depth[iv.first_def] == 0 ||
            temp_regs_.find(tid) != temp_regs_.end()) {
            continue;
        }
        const icode &def = fn.icodes[iv.first_def];
        if (def.op != icode_op::ADD || !def.result.is_temp() ||
            def.result.temp_id != tid || !def.result.type ||
            !def.result.type->is_ptr() || def.result.type->is_far_ptr() ||
            !def.result.type->base || def.result.type->base->is_volatile) {
            continue;
        }

        bool safe = true;
        int load_idx = -1;
        int store_idx = -1;
        for (int k = iv.first_def + 1; k <= iv.last_use; ++k) {
            const icode &ic = fn.icodes[k];
            if (ic.op == icode_op::CALL || ic.op == icode_op::ALLOCA ||
                ic.op == icode_op::INLINE_ASM) {
                safe = false;
                break;
            }
            if (!mentions_temp(ic, tid))
                continue;
            const bool direct_load =
                ic.op == icode_op::GET_VALUE_AT && ic.left.is_temp() &&
                ic.left.temp_id == tid && ic.right.is_none() &&
                op_size(ic.result) == 1;
            const bool direct_store =
                ic.op == icode_op::SET_VALUE_AT && ic.result.is_temp() &&
                ic.result.temp_id == tid && ic.right.is_none() &&
                op_size(ic.left) == 1;
            if (direct_load && load_idx < 0) {
                load_idx = k;
            } else if (direct_store && store_idx < 0) {
                store_idx = k;
            } else {
                safe = false;
                break;
            }
        }
        if (!safe || load_idx < 0 || store_idx <= load_idx)
            continue;

        bool control_split = false;
        for (int k = load_idx + 1; k < store_idx; ++k) {
            if (fn.icodes[k].op == icode_op::IFX ||
                fn.icodes[k].op == icode_op::GOTO) {
                control_split = true;
                break;
            }
        }
        if (!control_split)
            continue;

        iy_candidates.push_back(
            {tid, iv.first_def, iv.last_use,
             2850 + hot_mentions(iv) * 20 -
                 (iv.last_use - iv.first_def)});
    }

    // IY can also carry a short-lived computed integer across a call-free
    // section of a loop.  Z80 has only three ordinary arithmetic pairs, so a
    // value reused for index formation and two alternative updates otherwise
    // tends to be spilled at every use.  Restrict this to single-definition
    // word values whose uses are read-only arithmetic/control consumers; IY
    // windows below prevent it from displacing a more valuable live cursor.
    if (size_opt_enabled() || tuned_profile_enabled()) {
        for (const auto &[tid, iv] : ivs) {
            if (iv.size != 2 || iv.has_addr_of || iv.definitions != 1 ||
                iv.first_def < 0 || iv.last_use <= iv.first_def ||
                hot_mentions(iv) < 3 || loop_depth[iv.first_def] == 0 ||
                temp_regs_.find(tid) != temp_regs_.end()) {
                continue;
            }

            const icode &def = fn.icodes[iv.first_def];
            if (!def.result.is_temp() || def.result.temp_id != tid ||
                !def.result.type || !def.result.type->is_integer() ||
                (def.op != icode_op::ADD && def.op != icode_op::SUB &&
                 def.op != icode_op::SHL && def.op != icode_op::SHR)) {
                continue;
            }

            bool safe = true;
            int useful_reads = 0;
            for (int k = iv.first_def + 1; k <= iv.last_use; ++k) {
                const icode &ic = fn.icodes[k];
                if (ic.op == icode_op::CALL || ic.op == icode_op::ALLOCA ||
                    ic.op == icode_op::INLINE_ASM) {
                    safe = false;
                    break;
                }
                if (!mentions_temp(ic, tid))
                    continue;
                const bool reads_left =
                    ic.left.is_temp() && ic.left.temp_id == tid;
                const bool reads_right =
                    ic.right.is_temp() && ic.right.temp_id == tid;
                const bool writes_value =
                    ic.result.is_temp() && ic.result.temp_id == tid;
                if (writes_value || (!reads_left && !reads_right)) {
                    safe = false;
                    break;
                }
                switch (ic.op) {
                case icode_op::ADD:
                case icode_op::SUB:
                case icode_op::SHL:
                case icode_op::SHR:
                case icode_op::EQ:
                case icode_op::NE:
                case icode_op::LT:
                case icode_op::LE:
                case icode_op::GT:
                case icode_op::GE:
                case icode_op::RETURN:
                    ++useful_reads;
                    break;
                default:
                    safe = false;
                    break;
                }
                if (!safe)
                    break;
            }
            if (!safe || useful_reads < 3)
                continue;

            iy_candidates.push_back(
                {tid, iv.first_def, iv.last_use,
                 2100 + hot_mentions(iv) * 24 -
                     (iv.last_use - iv.first_def)});
        }
    }

    // A loop-carried word offset can use IY just as profitably as a pointer.
    // This covers lockstep byte offsets used to address arrays: the value is
    // initialized before the loop, advances by a small constant, and is only
    // otherwise consumed to form a near pointer.  IY is not a backend scratch
    // register, so ordinary arithmetic is safe; reject real calls and opaque
    // code across the live window.
    for (const auto &[tid, iv] : ivs) {
        if (iv.size != 2 || iv.has_addr_of || iv.first_def < 0 ||
            !fn.icodes[iv.first_def].result.type ||
            !fn.icodes[iv.first_def].result.type->is_integer())
            continue;

        int last_touch = iv.first_def;
        int loop_end = -1;
        for (int k = iv.first_def + 1; k < n; ++k) {
            if (mentions_temp(fn.icodes[k], tid))
                last_touch = k;
            auto note_backedge = [&](const std::string &label) {
                auto found = label_indices.find(label);
                if (found != label_indices.end() &&
                    found->second > iv.first_def &&
                    found->second <= last_touch && found->second < k)
                    loop_end = std::max(loop_end, k);
            };
            if (fn.icodes[k].op == icode_op::GOTO)
                note_backedge(fn.icodes[k].label_name);
            else if (fn.icodes[k].op == icode_op::IFX) {
                note_backedge(fn.icodes[k].true_lbl);
                note_backedge(fn.icodes[k].false_lbl);
            }
        }
        if (loop_end < 0)
            continue;

        bool saw_update = false;
        bool saw_address_use = false;
        bool safe = true;
        for (int k = iv.first_def + 1; k <= loop_end; ++k) {
            const icode &ic = fn.icodes[k];
            if (ic.op == icode_op::CALL || ic.op == icode_op::ALLOCA ||
                ic.op == icode_op::INLINE_ASM) {
                safe = false;
                break;
            }
            if (!mentions_temp(ic, tid))
                continue;
            const bool update =
                ic.op == icode_op::ADD && ic.result.is_temp() &&
                ic.result.temp_id == tid && ic.left.is_temp() &&
                ic.left.temp_id == tid &&
                ic.right.kind == operand_kind::INT_CONST &&
                ic.right.ival >= 1 && ic.right.ival <= 4;
            if (update) {
                saw_update = true;
                continue;
            }
            const bool address_use =
                ic.op == icode_op::ADD && ic.result.is_temp() &&
                ic.result.temp_id != tid && ic.result.type &&
                ic.result.type->is_ptr() &&
                ((ic.left.is_temp() && ic.left.temp_id == tid) ||
                 (ic.right.is_temp() && ic.right.temp_id == tid));
            if (address_use) {
                saw_address_use = true;
                continue;
            }
            safe = false;
            break;
        }
        if (!safe || !saw_update || !saw_address_use)
            continue;
        iy_candidates.push_back(
            {tid, iv.first_def, loop_end,
             2500 + hot_mentions(iv) * 20 - (loop_end - iv.first_def)});
    }

    std::sort(iy_candidates.begin(), iy_candidates.end(),
              [](const iy_candidate &lhs, const iy_candidate &rhs) {
                  if (lhs.score != rhs.score)
                      return lhs.score > rhs.score;
                  if (lhs.start != rhs.start)
                      return lhs.start < rhs.start;
                  return lhs.tid < rhs.tid;
              });
    std::vector<std::pair<int, int>> iy_windows;
    for (const auto &cand : iy_candidates) {
        bool overlaps = false;
        for (const auto &[start, end] : iy_windows) {
            if (!(cand.end < start || cand.start > end)) {
                overlaps = true;
                break;
            }
        }
        if (overlaps)
            continue;
        temp_regs_[cand.tid] = temp_home::main_iy;
        iy_windows.push_back({cand.start, cand.end});
    }
    for (const auto &cand : iy_candidates) {
        if (temp_regs_.find(cand.tid) == temp_regs_.end())
            spare_iy_cursor_candidates.push_back(cand);
    }

    // IY is also profitable for a call-free loop invariant that is only read
    // by comparisons.  Such values otherwise occupy an IX slot for the whole
    // loop even though IY can hold them without competing with the arithmetic
    // pairs.  Keep this deliberately narrower than general IY allocation:
    // one definition, integer word, compare-only uses, and no calls or opaque
    // code anywhere in the live range.
    std::vector<iy_candidate> invariant_iy_candidates;
    for (const auto &[tid, iv] : ivs) {
        if ((!size_opt_enabled() && !tuned_profile_enabled()) ||
            iv.size != 2 || iv.has_addr_of || iv.definitions != 1 ||
            !iv.loop_extended || iv.first_def < 0 ||
            iv.last_use <= iv.first_def || iv.mentions < 2 ||
            temp_regs_.find(tid) != temp_regs_.end()) {
            continue;
        }

        const icode &init = fn.icodes[iv.first_def];
        if (!init.result.type || !init.result.type->is_integer() ||
            (init.op != icode_op::RECEIVE &&
             init.op != icode_op::ASSIGN &&
             init.op != icode_op::CAST)) {
            continue;
        }

        bool safe = true;
        int compare_uses = 0;
        for (int k = iv.first_def + 1; k <= iv.last_use; ++k) {
            const icode &ic = fn.icodes[k];
            if (ic.op == icode_op::CALL || ic.op == icode_op::ALLOCA ||
                ic.op == icode_op::INLINE_ASM) {
                safe = false;
                break;
            }
            if (!mentions_temp(ic, tid))
                continue;
            const bool compare_use =
                is_compare_op(ic.op) &&
                ((ic.left.is_temp() && ic.left.temp_id == tid) ||
                 (ic.right.is_temp() && ic.right.temp_id == tid)) &&
                !(ic.result.is_temp() && ic.result.temp_id == tid);
            if (!compare_use) {
                safe = false;
                break;
            }
            ++compare_uses;
        }
        if (!safe || compare_uses == 0)
            continue;

        invariant_iy_candidates.push_back(
            {tid, iv.first_def, iv.last_use,
             2000 + compare_uses * 100 + hot_mentions(iv) * 8 -
                 (iv.last_use - iv.first_def)});
    }
    std::sort(invariant_iy_candidates.begin(),
              invariant_iy_candidates.end(),
              [](const iy_candidate &lhs, const iy_candidate &rhs) {
                  if (lhs.score != rhs.score)
                      return lhs.score > rhs.score;
                  if (lhs.start != rhs.start)
                      return lhs.start < rhs.start;
                  return lhs.tid < rhs.tid;
              });
    for (const iy_candidate &cand : invariant_iy_candidates) {
        bool overlaps = false;
        for (const auto &[start, end] : iy_windows) {
            if (!(cand.end < start || cand.start > end)) {
                overlaps = true;
                break;
            }
        }
        if (overlaps)
            continue;
        temp_regs_[cand.tid] = temp_home::main_iy;
        iy_windows.push_back({cand.start, cand.end});
    }

    // Stack linkage can profit from the same invariant IY compare path as a
    // register argument.  Promote only an immutable word parameter whose
    // complete live range is call-free and whose uses are comparisons inside
    // a proven loop.  The RECEIVE then loads IY once after IX is established.
    std::vector<iy_candidate> stack_param_iy_candidates;
    for (const auto &[key, iv] : syms) {
        if ((!size_opt_enabled() && !tuned_profile_enabled()) ||
            !iv.base.is_param || !iv.base.type ||
            iv.base.type->size() != 2 ||
            iv.receive_loc != abi_arg_loc::STACK ||
            iv.has_addr_of || iv.unsupported ||
            iv.first_idx < 0 || iv.last_idx <= iv.first_idx) {
            continue;
        }

        const icode &receive = fn.icodes[iv.first_idx];
        if (receive.op != icode_op::RECEIVE ||
            !receive.result.is_symbol() ||
            symbol_reg_key(receive.result) != key ||
            receive.result.byte_offset != 0) {
            continue;
        }

        bool safe = true;
        int compare_uses = 0;
        std::vector<int> compare_indices;
        for (int k = iv.first_idx + 1; k <= iv.last_idx; ++k) {
            const icode &ic = fn.icodes[k];
            if (ic.op == icode_op::CALL || ic.op == icode_op::ALLOCA ||
                ic.op == icode_op::INLINE_ASM) {
                safe = false;
                break;
            }
            auto same_symbol = [&](const operand &op) {
                return op.is_symbol() && !op.is_global &&
                       symbol_reg_key(op) == key;
            };
            if ((same_symbol(ic.left) && ic.left.byte_offset != 0) ||
                (same_symbol(ic.right) && ic.right.byte_offset != 0) ||
                (same_symbol(ic.result) && ic.result.byte_offset != 0)) {
                safe = false;
                break;
            }
            const bool used_left = same_symbol(ic.left);
            const bool used_right = same_symbol(ic.right);
            const bool defined = same_symbol(ic.result);
            if (defined || (used_left && used_right) ||
                ((used_left || used_right) && !is_compare_op(ic.op))) {
                safe = false;
                break;
            }
            if (used_left || used_right) {
                ++compare_uses;
                compare_indices.push_back(k);
            }
        }
        if (!safe || compare_uses == 0)
            continue;

        bool compared_in_loop = false;
        for (int k = iv.first_idx + 1;
             k <= iv.last_idx && !compared_in_loop; ++k) {
            const icode &edge = fn.icodes[k];
            auto note_backedge = [&](const std::string &label) {
                auto target = label_indices.find(label);
                if (target == label_indices.end() ||
                    target->second >= k) {
                    return;
                }
                for (int compare_idx : compare_indices) {
                    if (compare_idx >= target->second && compare_idx <= k) {
                        compared_in_loop = true;
                        return;
                    }
                }
            };
            if (edge.op == icode_op::GOTO) {
                note_backedge(edge.label_name);
            } else if (edge.op == icode_op::IFX) {
                note_backedge(edge.true_lbl);
                note_backedge(edge.false_lbl);
            }
        }
        if (!compared_in_loop)
            continue;

        stack_param_iy_candidates.push_back(
            {key, iv.first_idx, iv.last_idx,
             1900 + compare_uses * 100 -
                 (iv.last_idx - iv.first_idx)});
    }
    std::sort(stack_param_iy_candidates.begin(),
              stack_param_iy_candidates.end(),
              [](const iy_candidate &lhs, const iy_candidate &rhs) {
                  if (lhs.score != rhs.score)
                      return lhs.score > rhs.score;
                  if (lhs.start != rhs.start)
                      return lhs.start < rhs.start;
                  return lhs.tid < rhs.tid;
              });
    for (const iy_candidate &cand : stack_param_iy_candidates) {
        bool overlaps = false;
        for (const auto &[start, end] : iy_windows) {
            if (!(cand.end < start || cand.start > end)) {
                overlaps = true;
                break;
            }
        }
        if (overlaps)
            continue;
        symbol_regs_[cand.tid] = temp_home::main_iy;
        iy_windows.push_back({cand.start, cand.end});
    }

    // A TEMP whose value has no later IR use must still allow the defining
    // instruction to execute, but it does not need an IX-frame spill slot.
    // Give those dead results a register home that makes store_a/store_hl a
    // no-op (or at worst a register move for DE-origin word results).
    for (auto &[tid, iv] : ivs) {
        if (iv.first_def < 0 || iv.has_addr_of)
            continue;
        if (iv.last_use > iv.first_def)
            continue;
        if (temp_regs_.find(tid) != temp_regs_.end())
            continue;
        if (iv.size == 1) {
            temp_regs_[tid] = temp_home::main_a;
        } else if (iv.size == 2) {
            temp_regs_[tid] = temp_home::main_hl;
        }
    }

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
        if (iv.size != 2 || iv.definitions != 1 ||
            iv.first_def < 0 || iv.last_use <= iv.first_def)
            return false;
        const icode &def_ic = fn.icodes[iv.first_def];
        if (!def_ic.result.is_temp() || def_ic.result.temp_id != temp_id)
            return false;
        if (!def_ic.result.type || !def_ic.result.type->is_ptr() ||
            def_ic.result.type->is_far_ptr())
            return false;

        auto direct_base_ok = [](const operand &base) {
            return (base.kind == operand_kind::SYMBOL &&
                    base.is_global &&
                    !base.is_tls &&
                    !base.is_func &&
                    !base.is_param) ||
                   base.kind == operand_kind::LABEL_REF;
        };
        auto remat_base_ok = [&](const operand &base, int next_depth) {
            return direct_base_ok(base) ||
                   (base.is_temp() &&
                    remat_pointer_temp_ok(base.temp_id, next_depth));
        };
        auto offset_ok = [&](const operand &index, bool subtract_index,
                             int next_depth) {
            if (index.kind == operand_kind::INT_CONST)
                return index.ival >= -32768 && index.ival <= 32767;
            return !subtract_index && is_zero_u8ish(index, next_depth);
        };

        bool def_rematerializable = false;
        if (def_ic.op == icode_op::ADDRESS_OF) {
            // Direct object addresses are pure rematerializations: globals use
            // an immediate label and locals are IX-relative.  The emitter
            // invalidates the pair cache while rebuilding HL, so repeated
            // address temps do not need spill slots.
            def_rematerializable =
                def_ic.left.kind == operand_kind::SYMBOL &&
                !def_ic.left.is_tls &&
                !def_ic.left.is_sfr &&
                !def_ic.left.is_func &&
                !def_ic.left.is_param;
        } else if (def_ic.op == icode_op::ADD ||
                   def_ic.op == icode_op::SUB) {
            const operand *base = nullptr;
            const operand *index = nullptr;
            bool subtract_index = false;
            if (remat_base_ok(def_ic.left, depth + 1)) {
                base = &def_ic.left;
                index = &def_ic.right;
                subtract_index = def_ic.op == icode_op::SUB;
            } else if (def_ic.op == icode_op::ADD &&
                       remat_base_ok(def_ic.right, depth + 1)) {
                base = &def_ic.right;
                index = &def_ic.left;
            }
            def_rematerializable =
                base && index && offset_ok(*index, subtract_index, depth + 1);
        }
        if (!def_rematerializable)
            return false;

        for (int k = iv.first_def + 1; k <= iv.last_use; ++k) {
            const icode &use_ic = fn.icodes[k];
            const bool used_as_left =
                use_ic.left.is_temp() && use_ic.left.temp_id == temp_id;
            const bool used_as_right =
                use_ic.right.is_temp() && use_ic.right.temp_id == temp_id;
            const bool used_as_result =
                use_ic.result.is_temp() && use_ic.result.temp_id == temp_id;
            if (!used_as_left && !used_as_right && !used_as_result)
                continue;

            if (use_ic.op == icode_op::GET_VALUE_AT &&
                used_as_left)
                continue;
            if (use_ic.op == icode_op::SET_VALUE_AT &&
                used_as_result)
                continue;

            if (!used_as_result && (used_as_left || used_as_right)) {
                switch (use_ic.op) {
                case icode_op::ASSIGN:
                case icode_op::CAST:
                case icode_op::SEND:
                case icode_op::RETURN:
                case icode_op::IFX:
                case icode_op::ADD:
                case icode_op::SUB:
                case icode_op::MUL:
                case icode_op::DIV:
                case icode_op::MOD:
                case icode_op::BAND:
                case icode_op::BOR:
                case icode_op::BXOR:
                case icode_op::EQ:
                case icode_op::NE:
                case icode_op::LT:
                case icode_op::LE:
                case icode_op::GT:
                case icode_op::GE:
                case icode_op::SHL:
                case icode_op::SHR:
                case icode_op::ROL:
                case icode_op::ROR:
                case icode_op::CALL:
                    continue;
                default:
                    break;
                }
            }
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
        if (def_ic.op == icode_op::CAST) {
            if (!def_ic.left.type || !def_ic.result.type)
                return false;
            if (def_ic.left.type->is_far_ptr() || def_ic.result.type->is_far_ptr())
                return false;
            const bool src_ok =
                def_ic.left.type->is_integer() || def_ic.left.type->is_ptr();
            const bool dst_ok =
                def_ic.result.type->is_integer() || def_ic.result.type->is_ptr();
            if (!src_ok || !dst_ok)
                return false;
        }
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
    auto remat_word_load_temp_ok = [&](int temp_id, int depth) -> bool {
        if (depth > 4)
            return false;
        auto iv_it = ivs.find(temp_id);
        if (iv_it == ivs.end())
            return false;
        const interval &iv = iv_it->second;
        if (iv.size != 2 || iv.definitions != 1 ||
            iv.first_def < 0 || iv.last_use <= iv.first_def)
            return false;
        if (iv.has_addr_of)
            return false;

        const icode &def_ic = fn.icodes[iv.first_def];
        if (def_ic.op != icode_op::GET_VALUE_AT ||
            !def_ic.result.is_temp() ||
            def_ic.result.temp_id != temp_id ||
            !def_ic.right.is_none()) {
            return false;
        }
        if (!def_ic.result.type || def_ic.result.type->size() != 2 ||
            def_ic.result.type->is_volatile) {
            return false;
        }
        if (!def_ic.left.type || !def_ic.left.type->is_ptr() ||
            def_ic.left.type->is_far_ptr())
            return false;
        if (def_ic.left.type->base && def_ic.left.type->base->is_volatile)
            return false;

        auto pointer_rematerializable = [&](const operand &ptr) {
            if (ptr.kind == operand_kind::INT_CONST ||
                ptr.kind == operand_kind::LABEL_REF) {
                return true;
            }
            return ptr.is_temp() &&
                   remat_pointer_temp_ok(ptr.temp_id, depth + 1);
        };
        if (!pointer_rematerializable(def_ic.left))
            return false;

        auto is_global_or_special_store = [](const operand &op) {
            return op.kind == operand_kind::SYMBOL &&
                   (op.is_global || op.is_tls || op.is_sfr || op.is_func);
        };
        auto safe_use = [&](const icode &use_ic) {
            const bool use_left =
                use_ic.left.is_temp() && use_ic.left.temp_id == temp_id;
            const bool use_right =
                use_ic.right.is_temp() && use_ic.right.temp_id == temp_id;
            const bool use_result =
                use_ic.result.is_temp() && use_ic.result.temp_id == temp_id;
            if (!use_left && !use_right && !use_result)
                return true;
            if (use_result)
                return false;

            switch (use_ic.op) {
            case icode_op::ASSIGN:
            case icode_op::CAST:
            case icode_op::SEND:
            case icode_op::RETURN:
            case icode_op::ADD:
            case icode_op::SUB:
            case icode_op::BAND:
            case icode_op::BOR:
            case icode_op::BXOR:
            case icode_op::EQ:
            case icode_op::NE:
            case icode_op::LT:
            case icode_op::LE:
            case icode_op::GT:
            case icode_op::GE:
            case icode_op::SHL:
            case icode_op::SHR:
            case icode_op::ROL:
            case icode_op::ROR:
            case icode_op::GET_VALUE_AT:
                return true;
            default:
                return false;
            }
        };

        for (int k = iv.first_def + 1; k <= iv.last_use; ++k) {
            const icode &cur = fn.icodes[k];
            if (cur.op == icode_op::CALL ||
                cur.op == icode_op::SET_VALUE_AT ||
                cur.op == icode_op::ALLOCA ||
                cur.op == icode_op::INLINE_ASM ||
                cur.op == icode_op::LABEL ||
                cur.op == icode_op::GOTO ||
                cur.op == icode_op::IFX) {
                return false;
            }
            if (uses_tls_global(cur.result) ||
                uses_tls_global(cur.left) ||
                uses_tls_global(cur.right) ||
                is_global_or_special_store(cur.result)) {
                return false;
            }
            if (!safe_use(cur))
                return false;
        }
        return true;
    };
    auto loop_pointer_hl_candidate = [&](int temp_id, const interval &iv,
                                         int &score_out) -> bool {
        if (iv.size != 2 || iv.has_addr_of)
            return false;
        if (iv.first_def < 0 || iv.last_use <= iv.first_def)
            return false;
        if (iv.mentions < 2)
            return false;

        const icode &def_ic = fn.icodes[iv.first_def];
        if (!def_ic.result.is_temp() || def_ic.result.temp_id != temp_id)
            return false;

        auto is_byte_data_base = [](const operand &op) {
            if (op.kind == operand_kind::LABEL_REF)
                return true;
            return op.kind == operand_kind::SYMBOL &&
                   op.is_global &&
                   !op.is_tls &&
                   !op.is_sfr &&
                   !op.is_func &&
                   op.type &&
                   ((op.type->is_array() && op.type->base &&
                     op.type->base->size() == 1) ||
                    (op.type->is_ptr() && op.type->base &&
                     op.type->base->size() == 1));
        };

        bool init_ok = false;
        switch (def_ic.op) {
        case icode_op::ASSIGN:
        case icode_op::CAST:
        case icode_op::ADDRESS_OF:
            init_ok = is_byte_data_base(def_ic.left);
            break;
        default:
            break;
        }
        if (!init_ok)
            return false;

        bool saw_mem_use = false;
        bool saw_update = false;
        int inc_temp = -1;

        auto byteish_operand = [&](const operand &op) {
            if (op.is_none())
                return true;
            if (op.kind == operand_kind::INT_CONST ||
                op.kind == operand_kind::FLOAT_CONST)
                return true;
            if (op.kind == operand_kind::LABEL_REF)
                return true;
            if (op.is_temp() && op.temp_id == temp_id)
                return true;
            return op.type && op.type->size() <= 1;
        };

        auto safe_byteish_ic = [&](const icode &cur) {
            switch (cur.op) {
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
                return byteish_operand(cur.left) && byteish_operand(cur.right);
            default:
                return false;
            }
        };

        for (int k = iv.first_def + 1; k <= iv.last_use; ++k) {
            const icode &ic = fn.icodes[k];

            if (ic.op == icode_op::LABEL ||
                ic.op == icode_op::GOTO ||
                ic.op == icode_op::IFX) {
                continue;
            }
            if (ic.op == icode_op::CALL ||
                ic.op == icode_op::ALLOCA ||
                ic.op == icode_op::INLINE_ASM ||
                ic.op == icode_op::ADDRESS_OF) {
                return false;
            }
            if (uses_tls_global(ic.result) || uses_tls_global(ic.left) ||
                uses_tls_global(ic.right)) {
                return false;
            }
            if (symbol_word_access_may_need_bc_scratch(ic.result) ||
                symbol_word_access_may_need_bc_scratch(ic.left) ||
                symbol_word_access_may_need_bc_scratch(ic.right)) {
                return false;
            }
            if (ic.op == icode_op::GET_VALUE_AT &&
                ic.left.is_temp() &&
                ic.left.temp_id == temp_id &&
                ic.result.type &&
                ic.result.type->size() == 1) {
                saw_mem_use = true;
                continue;
            }
            if (ic.op == icode_op::SET_VALUE_AT &&
                ic.result.is_temp() &&
                ic.result.temp_id == temp_id &&
                ic.left.type &&
                ic.left.type->size() == 1) {
                saw_mem_use = true;
                continue;
            }
            if (ic.op == icode_op::ADD &&
                ic.left.is_temp() &&
                ic.left.temp_id == temp_id &&
                ic.right.kind == operand_kind::INT_CONST &&
                ic.right.ival == 1 &&
                ic.result.is_temp()) {
                inc_temp = ic.result.temp_id;
                saw_update = true;
                continue;
            }
            if (ic.op == icode_op::ASSIGN &&
                ic.result.is_temp() &&
                ic.result.temp_id == temp_id &&
                ic.left.is_temp() &&
                ic.left.temp_id == inc_temp) {
                continue;
            }
            if (is_compare_op(ic.op))
                continue;
            if (!safe_byteish_ic(ic))
                return false;
            if (mentions_temp(ic, temp_id))
                return false;
        }

        if (!saw_mem_use || !saw_update)
            return false;

        score_out = 240 + hot_mentions(iv) * 10 -
                    (iv.last_use - iv.first_def);
        return true;
    };
    auto loop_pointer_bc_candidate = [&](int temp_id, const interval &iv,
                                         int &end_out,
                                         int &score_out) -> bool {
        if (iv.size != 2 || iv.has_addr_of || iv.first_def < 0 ||
            iv.last_use <= iv.first_def) {
            return false;
        }

        const icode &def_ic = fn.icodes[iv.first_def];
        if (!def_ic.result.is_temp() || def_ic.result.temp_id != temp_id ||
            !def_ic.result.type || !def_ic.result.type->is_ptr() ||
            def_ic.result.type->is_far_ptr() ||
            !def_ic.result.type->base ||
            def_ic.result.type->base->size() != 1 ||
            def_ic.result.type->base->is_volatile) {
            return false;
        }

        auto direct_byte_base = [](const operand &op) {
            if (op.kind == operand_kind::LABEL_REF)
                return true;
            return op.kind == operand_kind::SYMBOL && op.is_global &&
                   !op.is_tls && !op.is_sfr && !op.is_func && op.type &&
                   ((op.type->is_array() && op.type->base &&
                     op.type->base->size() == 1) ||
                    (op.type->is_ptr() && !op.type->is_far_ptr() &&
                     op.type->base && op.type->base->size() == 1));
        };

        bool init_ok =
            def_ic.op == icode_op::RECEIVE &&
            (def_ic.arg_loc == abi_arg_loc::REG_HL ||
             def_ic.arg_loc == abi_arg_loc::REG_DE);
        if (!init_ok && def_ic.op == icode_op::ASSIGN &&
            def_ic.left.is_symbol() && def_ic.left.is_param &&
            !def_ic.left.is_global && def_ic.left.byte_offset == 0 &&
            effective_call_abi(fn.abi) == call_abi::SDCCCALL0) {
            init_ok = true;
        }
        if (!init_ok && def_ic.op == icode_op::ASSIGN &&
            def_ic.left.is_temp()) {
            auto incoming = ivs.find(def_ic.left.temp_id);
            if (incoming != ivs.end() && incoming->second.first_def >= 0) {
                const icode &receive =
                    fn.icodes[incoming->second.first_def];
                init_ok = receive.op == icode_op::RECEIVE &&
                          receive.result.is_temp() &&
                          receive.result.temp_id == def_ic.left.temp_id &&
                          (receive.arg_loc == abi_arg_loc::REG_HL ||
                           receive.arg_loc == abi_arg_loc::REG_DE);
            }
        }
        if (!init_ok &&
            (def_ic.op == icode_op::ASSIGN ||
             def_ic.op == icode_op::CAST ||
             def_ic.op == icode_op::ADDRESS_OF)) {
            init_ok = direct_byte_base(def_ic.left) ||
                      (def_ic.left.is_temp() &&
                       remat_pointer_temp_ok(def_ic.left.temp_id, 0));
        }
        if (!init_ok)
            return false;

        int old_temp = -1;
        int step_temp = -1;
        int commit_idx = -1;
        bool saw_mem_use = false;

        for (int k = iv.first_def + 1; k < n; ++k) {
            const icode &ic = fn.icodes[k];
            if (ic.op == icode_op::ASSIGN && ic.result.is_temp() &&
                ic.result.temp_id != temp_id && ic.left.is_temp() &&
                ic.left.temp_id == temp_id) {
                old_temp = ic.result.temp_id;
                continue;
            }
            if (ic.op == icode_op::ADD && ic.result.is_temp() &&
                ic.left.is_temp() && ic.left.temp_id == temp_id &&
                ic.right.kind == operand_kind::INT_CONST &&
                ic.right.ival == 1) {
                if (ic.result.temp_id == temp_id) {
                    step_temp = temp_id;
                    commit_idx = k;
                    break;
                }
                step_temp = ic.result.temp_id;
                continue;
            }
            if (step_temp >= 0 && ic.op == icode_op::ASSIGN &&
                ic.result.is_temp() && ic.result.temp_id == temp_id &&
                ic.left.is_temp() && ic.left.temp_id == step_temp) {
                commit_idx = k;
                break;
            }
        }
        // A post-increment expression introduces an explicit copy of the old
        // cursor, but the more common `value = *cursor; ++cursor` form does
        // not.  Both have the same register-safety requirements below: the
        // dereference may use the live cursor directly, while the optional
        // old value is only needed when it actually appears in the IR.
        if (step_temp < 0 || commit_idx < 0)
            return false;

        int loop_end = -1;
        for (int k = commit_idx + 1; k < n; ++k) {
            const icode &ic = fn.icodes[k];
            auto backward_target_after_def = [&](const std::string &label) {
                auto it = label_indices.find(label);
                return it != label_indices.end() &&
                       it->second > iv.first_def && it->second < k;
            };
            if ((ic.op == icode_op::GOTO &&
                 backward_target_after_def(ic.label_name)) ||
                (ic.op == icode_op::IFX &&
                 (backward_target_after_def(ic.true_lbl) ||
                  backward_target_after_def(ic.false_lbl)))) {
                loop_end = k;
                break;
            }
        }
        if (loop_end < 0)
            return false;
        const int allocation_end = std::max(loop_end, iv.last_use);

        auto is_cursor = [&](const operand &op) {
            return op.is_temp() && op.temp_id == temp_id;
        };
        auto is_old_cursor = [&](const operand &op) {
            return old_temp >= 0 && op.is_temp() && op.temp_id == old_temp;
        };

        for (int k = iv.first_def + 1; k <= allocation_end; ++k) {
            const icode &ic = fn.icodes[k];
            if (clobbers_bc(ic) || uses_tls_global(ic.result) ||
                uses_tls_global(ic.left) || uses_tls_global(ic.right) ||
                symbol_word_access_may_need_bc_scratch(ic.result) ||
                symbol_word_access_may_need_bc_scratch(ic.left) ||
                symbol_word_access_may_need_bc_scratch(ic.right)) {
                return false;
            }

            if (ic.op == icode_op::LABEL || ic.op == icode_op::GOTO ||
                ic.op == icode_op::IFX || ic.op == icode_op::RETURN) {
                continue;
            }
            if ((opt_settings_.level == opt_level::Of ||
                 opt_settings_.level == opt_level::O3) &&
                ic.op == icode_op::ADDRESS_OF &&
                !address_of_may_need_bc_scratch(ic.left)) {
                continue;
            }
            if (ic.op == icode_op::CALL || ic.op == icode_op::ALLOCA ||
                ic.op == icode_op::INLINE_ASM ||
                ic.op == icode_op::ADDRESS_OF) {
                return false;
            }

            if (ic.op == icode_op::ASSIGN && ic.result.is_temp() &&
                ic.result.temp_id == old_temp && is_cursor(ic.left)) {
                continue;
            }
            if (ic.op == icode_op::ADD && ic.result.is_temp() &&
                is_cursor(ic.left) &&
                ic.right.kind == operand_kind::INT_CONST) {
                // The +1 form advances the cursor. Other constants form
                // stable derived values such as an end pointer; ADD lowers
                // through HL while preserving a BC-resident source.
                continue;
            }
            if (ic.op == icode_op::ASSIGN && is_cursor(ic.result) &&
                ic.left.is_temp() && ic.left.temp_id == step_temp) {
                continue;
            }
            if (is_compare_op(ic.op) &&
                (is_cursor(ic.left) || is_cursor(ic.right))) {
                continue;
            }
            if (ic.op == icode_op::GET_VALUE_AT &&
                (is_cursor(ic.left) || is_old_cursor(ic.left)) &&
                ic.result.type && ic.result.type->size() == 1) {
                saw_mem_use = true;
                continue;
            }
            if (ic.op == icode_op::SET_VALUE_AT &&
                (is_cursor(ic.result) || is_old_cursor(ic.result)) &&
                ic.left.type && ic.left.type->size() == 1) {
                saw_mem_use = true;
                continue;
            }
            if (mentions_temp(ic, temp_id) ||
                (old_temp >= 0 && mentions_temp(ic, old_temp)))
                return false;
            // Byte comparisons and immediate byte/word comparisons lower
            // through A/HL/DE and an optional direct IX result slot; they do
            // not consume BC.  Do not accept a general word/word comparison:
            // a rematerialized address on either side could need BC itself.
            const bool bc_preserving_compare =
                is_compare_op(ic.op) &&
                ((op_size(ic.left) <= 1 && op_size(ic.right) <= 1) ||
                 (ic.left.kind == operand_kind::INT_CONST &&
                  op_size(ic.right) <= 2) ||
                 (ic.right.kind == operand_kind::INT_CONST &&
                  op_size(ic.left) <= 2));
            if (bc_preserving_compare)
                continue;
            const bool byte_immediate_compare =
                is_compare_op(ic.op) && ic.result.is_temp() &&
                ((ic.left.kind == operand_kind::INT_CONST &&
                  ic.right.type && ic.right.type->size() == 1) ||
                 (ic.right.kind == operand_kind::INT_CONST &&
                  ic.left.type && ic.left.type->size() == 1));
            if (byte_immediate_compare && k + 1 <= allocation_end &&
                fn.icodes[k + 1].op == icode_op::IFX &&
                fn.icodes[k + 1].left.is_temp() &&
                fn.icodes[k + 1].left.temp_id == ic.result.temp_id) {
                continue;
            }
            auto is_iy_home = [&](const operand &op) {
                if (!op.is_temp())
                    return false;
                auto home = temp_regs_.find(op.temp_id);
                return home != temp_regs_.end() &&
                       home->second == temp_home::main_iy;
            };
            auto is_iy_constant_offset = [&](const operand &op) {
                if (!op.is_temp())
                    return false;
                auto derived_iv = ivs.find(op.temp_id);
                if (derived_iv == ivs.end() ||
                    derived_iv->second.first_def < 0)
                    return false;
                const icode &def = fn.icodes[derived_iv->second.first_def];
                return (def.op == icode_op::ADD ||
                        def.op == icode_op::SUB) &&
                       def.result.is_temp() &&
                       def.result.temp_id == op.temp_id &&
                       is_iy_home(def.left) &&
                       def.right.kind == operand_kind::INT_CONST &&
                       def.right.ival >= -128 && def.right.ival <= 127;
            };
            const bool safe_iy_operation =
                (ic.op == icode_op::GET_VALUE_AT &&
                 (is_iy_home(ic.left) ||
                  is_iy_constant_offset(ic.left)) &&
                 ic.right.is_none() &&
                 op_size(ic.result) >= 1 && op_size(ic.result) <= 2) ||
                (ic.op == icode_op::SET_VALUE_AT &&
                 (is_iy_home(ic.result) ||
                  is_iy_constant_offset(ic.result)) &&
                 ic.right.is_none() &&
                 op_size(ic.left) >= 1 && op_size(ic.left) <= 2) ||
                ((ic.op == icode_op::ADD || ic.op == icode_op::SUB) &&
                 is_iy_home(ic.left) &&
                 ic.right.kind == operand_kind::INT_CONST) ||
                (ic.op == icode_op::ASSIGN && is_iy_home(ic.result));
            if (safe_iy_operation)
                continue;
            if (bc_backend_hazard(ic, direct_ix_frame))
                return false;
        }

        if (!saw_mem_use)
            return false;

        end_out = allocation_end;
        score_out = 1800 + hot_mentions(iv) * 16 -
                    (allocation_end - iv.first_def);
        return true;
    };
    std::function<bool(const operand &, int)> hl_load_may_clobber_bc;
    hl_load_may_clobber_bc = [&](const operand &op, int depth) -> bool {
        if (depth > 4)
            return true;
        if (uses_tls_global(op) || symbol_word_access_may_need_bc_scratch(op))
            return true;
        if (!op.is_temp())
            return false;

        auto iv_it = ivs.find(op.temp_id);
        if (iv_it == ivs.end() || iv_it->second.first_def < 0)
            return false;
        const icode &def_ic = fn.icodes[iv_it->second.first_def];
        if (def_ic.op == icode_op::ADDRESS_OF)
            return address_of_may_need_bc_scratch(def_ic.left);
        if (def_ic.op == icode_op::ASSIGN || def_ic.op == icode_op::CAST)
            return hl_load_may_clobber_bc(def_ic.left, depth + 1);
        if ((def_ic.op == icode_op::ADD || def_ic.op == icode_op::SUB) &&
            remat_pointer_temp_ok(op.temp_id, 0)) {
            return hl_load_may_clobber_bc(def_ic.left, depth + 1) ||
                   hl_load_may_clobber_bc(def_ic.right, depth + 1);
        }
        return false;
    };
    auto symbol_bc_backend_hazard = [&](const icode &ic,
                                        const operand &sym_base) {
        if (is_cfg_barrier(ic))
            return true;

        if (uses_tls_global(ic.result) || uses_tls_global(ic.left) ||
            uses_tls_global(ic.right)) {
            return true;
        }

        if (ic.op == icode_op::ADDRESS_OF && address_of_may_need_bc_scratch(ic.left))
            return true;
        if (hl_load_may_clobber_bc(ic.left, 0) ||
            hl_load_may_clobber_bc(ic.right, 0)) {
            return true;
        }

        auto needs_bc_scratch = [&](const operand &op) {
            if (same_local_symbol_base(op, sym_base) &&
                op.byte_offset >= 0 && op.byte_offset <= 1) {
                return false;
            }
            return symbol_word_access_may_need_bc_scratch(op);
        };

        return needs_bc_scratch(ic.result) ||
               needs_bc_scratch(ic.left) ||
               needs_bc_scratch(ic.right);
    };
    const bool helper_like_fn = straight_line_helper_like();
    auto simple_send_operand_for_bc_live_range = [&](const operand &op) {
        switch (op.kind) {
        case operand_kind::INT_CONST:
        case operand_kind::FLOAT_CONST:
        case operand_kind::LABEL_REF:
            return true;
        case operand_kind::SYMBOL:
            return op.is_global &&
                   !op.is_tls &&
                   !op.is_sfr &&
                   !op.is_func;
        default:
            return false;
        }
    };
    auto indirect_callee_bc_candidate = [&](int temp_id, const interval &iv,
                                            int &score_out) -> bool {
        if (iv.size != 2 || iv.has_addr_of)
            return false;
        if (iv.first_def < 0 || iv.last_use <= iv.first_def)
            return false;

        const icode &call_ic = fn.icodes[iv.last_use];
        if (call_ic.op != icode_op::CALL || !call_ic.func_name.empty())
            return false;
        if (!(call_ic.left.is_temp() && call_ic.left.temp_id == temp_id))
            return false;

        for (int k = iv.first_def + 1; k < iv.last_use; ++k) {
            const icode &mid = fn.icodes[k];
            if (mentions_temp(mid, temp_id))
                return false;
            if (mid.op == icode_op::LABEL)
                continue;
            if (mid.op != icode_op::SEND)
                return false;
            if (!simple_send_operand_for_bc_live_range(mid.left))
                return false;
        }

        score_out = 600 - (iv.last_use - iv.first_def);
        return true;
    };

    auto main_byte_window_use_safe = [&](const icode &ic, int temp_id) {
        const bool use_left =
            ic.left.is_temp() && ic.left.temp_id == temp_id;
        const bool use_right =
            ic.right.is_temp() && ic.right.temp_id == temp_id;
        if (!use_left && !use_right)
            return true;

        auto other_is_u8_const = [&](const operand &op) {
            return op.kind == operand_kind::INT_CONST &&
                   op.ival >= 0 && op.ival <= 0xff;
        };

        switch (ic.op) {
        case icode_op::ASSIGN:
        case icode_op::RETURN:
            return use_left && !use_right;
        case icode_op::CAST:
            return use_left && !use_right &&
                   ic.left.type && ic.left.type->size() <= 1 &&
                   ic.result.type && ic.result.type->size() <= 2;
        case icode_op::BAND:
        case icode_op::BOR:
        case icode_op::BXOR:
        case icode_op::ADD:
        case icode_op::SUB:
            if (use_left && !use_right)
                return other_is_u8_const(ic.right);
            if (use_right && !use_left &&
                (ic.op == icode_op::BAND || ic.op == icode_op::BOR ||
                 ic.op == icode_op::BXOR || ic.op == icode_op::ADD))
                return other_is_u8_const(ic.left);
            return false;
        case icode_op::SHL:
        case icode_op::SHR:
        case icode_op::ROL:
        case icode_op::ROR:
            return use_left && !use_right && other_is_u8_const(ic.right);
        case icode_op::EQ:
        case icode_op::NE:
        case icode_op::LT:
        case icode_op::LE:
        case icode_op::GT:
        case icode_op::GE:
            if (use_left && !use_right)
                return other_is_u8_const(ic.right);
            if (use_right && !use_left)
                return other_is_u8_const(ic.left);
            return false;
        default:
            return false;
        }
    };
    auto loop_byte_temp_window_safe = [&](const icode &ic, int temp_id) {
        if (!mentions_temp(ic, temp_id))
            return true;

        auto byteish_operand = [&](const operand &op) {
            if (op.is_none())
                return true;
            if (op.kind == operand_kind::INT_CONST ||
                op.kind == operand_kind::FLOAT_CONST ||
                op.kind == operand_kind::LABEL_REF)
                return true;
            if (op.is_temp() && op.temp_id == temp_id)
                return true;
            return op.type && op.type->size() <= 1;
        };

        switch (ic.op) {
        case icode_op::ASSIGN:
        case icode_op::RETURN:
            return byteish_operand(ic.left) && byteish_operand(ic.right);
        case icode_op::CAST:
            return byteish_operand(ic.left) && byteish_operand(ic.right) &&
                   ic.left.type && ic.left.type->size() <= 1 &&
                   ic.result.type && ic.result.type->size() <= 2;
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
        case icode_op::EQ:
        case icode_op::NE:
        case icode_op::LT:
        case icode_op::LE:
        case icode_op::GT:
        case icode_op::GE:
            return byteish_operand(ic.left) && byteish_operand(ic.right);
        default:
            return false;
        }
    };
    auto main_byte_window_use_safe_symbol = [&](const icode &ic,
                                                const operand &sym) {
        const bool use_result = same_local_symbol(ic.result, sym);
        const bool use_left = same_local_symbol(ic.left, sym);
        const bool use_right = same_local_symbol(ic.right, sym);
        if (!use_result && !use_left && !use_right)
            return true;

        auto byteish_operand = [&](const operand &op) {
            if (op.is_none())
                return true;
            if (op.kind == operand_kind::INT_CONST ||
                op.kind == operand_kind::FLOAT_CONST ||
                op.kind == operand_kind::LABEL_REF)
                return true;
            if (same_local_symbol(op, sym))
                return true;
            return op.type && op.type->size() <= 1;
        };

        switch (ic.op) {
        case icode_op::ASSIGN:
        case icode_op::RETURN:
            return byteish_operand(ic.left) && byteish_operand(ic.right);
        case icode_op::CAST:
            return byteish_operand(ic.left) && byteish_operand(ic.right) &&
                   ic.left.type && ic.left.type->size() <= 1 &&
                   ic.result.type && ic.result.type->size() <= 2;
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
        case icode_op::EQ:
        case icode_op::NE:
        case icode_op::LT:
        case icode_op::LE:
        case icode_op::GT:
        case icode_op::GE:
            return byteish_operand(ic.left) && byteish_operand(ic.right);
        default:
            return false;
        }
    };
    auto byte_bc_scratch_hazard = [&](const icode &ic) {
        if (clobbers_bc(ic))
            return true;
        switch (ic.op) {
        case icode_op::ADD:
        case icode_op::SUB:
            if (op_size(ic.result) < 2 &&
                op_size(ic.left) < 2 &&
                op_size(ic.right) < 2) {
                return false;
            }
            if (uses_tls_global(ic.result) ||
                uses_tls_global(ic.left) ||
                uses_tls_global(ic.right)) {
                return true;
            }
            if (symbol_word_access_may_need_bc_scratch(ic.result) ||
                symbol_word_access_may_need_bc_scratch(ic.left) ||
                symbol_word_access_may_need_bc_scratch(ic.right)) {
                return true;
            }
            // Plain 16-bit add/sub lowering uses HL/DE (and A for SBC carry)
            // but not BC when all frame operands have direct IX displacements.
            // Let byte B/C homes survive those pointer/index updates.
            return false;
        case icode_op::EQ:
        case icode_op::NE:
        case icode_op::LT:
        case icode_op::LE:
        case icode_op::GT:
        case icode_op::GE:
            if (op_size(ic.left) < 2 && op_size(ic.right) < 2)
                return true;
            if (uses_tls_global(ic.result) ||
                uses_tls_global(ic.left) ||
                uses_tls_global(ic.right)) {
                return true;
            }
            return symbol_word_access_may_need_bc_scratch(ic.result) ||
                   symbol_word_access_may_need_bc_scratch(ic.left) ||
                   symbol_word_access_may_need_bc_scratch(ic.right);
        default:
            return bc_backend_hazard(ic, direct_ix_frame);
        }
    };
    auto loop_bc_scratch_hazard = [&](const icode &ic) {
        switch (ic.op) {
        case icode_op::LABEL:
        case icode_op::GOTO:
        case icode_op::IFX:
            return false;
        default:
            break;
        }
        return byte_bc_scratch_hazard(ic);
    };
    auto byte_symbol_reg_hazard = [&](const icode &ic,
                                      const operand &sym) {
        switch (ic.op) {
        case icode_op::LABEL:
        case icode_op::GOTO:
        case icode_op::IFX:
        case icode_op::RETURN:
            return false;
        default:
            break;
        }

        if (byte_bc_scratch_hazard(ic))
            return true;
        if (uses_tls_global(ic.result) || uses_tls_global(ic.left) ||
            uses_tls_global(ic.right)) {
            return true;
        }
        if (ic.op == icode_op::ADDRESS_OF &&
            address_of_may_need_bc_scratch(ic.left)) {
            return true;
        }

        auto needs_bc_scratch = [&](const operand &op) {
            if (same_local_symbol(op, sym))
                return false;
            return symbol_word_access_may_need_bc_scratch(op);
        };
        return needs_bc_scratch(ic.result) ||
               needs_bc_scratch(ic.left) ||
               needs_bc_scratch(ic.right);
    };
    auto byte_symbol_first_occurrence_is_definition =
        [&](const sym_interval &iv) {
        if (iv.first_idx < 0 || iv.first_idx >= n)
            return false;
        const icode &first = fn.icodes[iv.first_idx];
        if (!same_local_symbol(first.result, iv.base))
            return false;
        switch (first.op) {
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
            return true;
        default:
            return false;
        }
    };
    auto symbol_feeds_branchy_bit_test = [&](const sym_interval &iv) {
        for (int k = iv.first_idx; k + 1 <= iv.last_idx && k + 1 < n; ++k) {
            const icode &test_ic = fn.icodes[k];
            const icode &ifx_ic = fn.icodes[k + 1];
            if (test_ic.op != icode_op::BAND ||
                !test_ic.result.is_temp() ||
                !mentions_symbol(test_ic, iv.base) ||
                ifx_ic.op != icode_op::IFX ||
                !ifx_ic.left.is_temp() ||
                ifx_ic.left.temp_id != test_ic.result.temp_id) {
                continue;
            }
            return true;
        }
        return false;
    };

    struct c_candidate {
        int tid = -1;
        int start = -1;
        int end = -1;
        int score = 0;
    };
    struct sym_byte_candidate {
        int key = -1;
        int start = -1;
        int end = -1;
        int score = 0;
    };
    std::vector<c_candidate> c_candidates;
    std::vector<c_candidate> b_candidates;
    std::vector<sym_byte_candidate> sym_c_candidates;
    std::vector<sym_byte_candidate> sym_b_candidates;
    std::vector<bc_candidate> hl_candidates;
    std::vector<std::pair<int, int>> pair_windows;
    std::vector<std::pair<int, int>> b_windows;
    std::vector<std::pair<int, int>> c_windows;
    std::vector<std::pair<int, int>> hl_windows;

    // A canonical dead induction variable may already have been rewritten
    // by countdown_dead_loops to:
    //
    //     count = trip_count;
    //   body:
    //     ...
    //     count = count - 1;
    //     if (count) goto body;
    //
    // Keep that counter in BC when the body has BC-preserving lowerings.  The
    // older induction candidate below only recognizes ascending zero-based
    // counters that still feed a comparison, so it cannot see this cheaper
    // post-canonicalization form.
    struct loop_countdown_candidate {
        int tid = -1;
        int start = -1;
        int end = -1;
        int score = 0;
    };
    std::vector<loop_countdown_candidate> loop_countdown_candidates;

    auto inline_word_const_mul_preserves_bc = [&](const icode &ic) {
        if (ic.op != icode_op::MUL || op_size(ic.result) != 2 ||
            op_size(ic.left) != 2 || op_size(ic.right) != 2)
            return false;

        const operand *value = nullptr;
        const operand *constant = nullptr;
        if (ic.left.kind == operand_kind::INT_CONST &&
            ic.right.kind != operand_kind::INT_CONST) {
            value = &ic.right;
            constant = &ic.left;
        } else if (ic.right.kind == operand_kind::INT_CONST &&
                   ic.left.kind != operand_kind::INT_CONST) {
            value = &ic.left;
            constant = &ic.right;
        }
        if (!value || !constant)
            return false;

        // The inline multiplier keeps the source in DE and accumulates in HL.
        // Restrict the source to a compact frame value so loading it cannot
        // require BC as an address scratch pair.
        int off = 0;
        if (value->kind == operand_kind::TEMP) {
            if (temp_regs_.count(value->temp_id))
                return false;
            off = ix_offset_of(*value);
        } else if (value->kind == operand_kind::SYMBOL &&
                   !value->is_global && !value->is_tls &&
                   !value->is_sfr && !value->is_func) {
            off = ix_offset_of(*value);
        } else {
            return false;
        }
        if (!fits_ix_disp(off) || !fits_ix_disp(off + 1))
            return false;

        const uint16_t k = static_cast<uint16_t>(constant->ival);
        int msb = -1;
        for (int bit = 15; bit >= 0; --bit) {
            if ((k >> bit) & 1u) {
                msb = bit;
                break;
            }
        }
        int operation_count = 0;
        if (msb >= 0) {
            operation_count = msb;
            for (int bit = msb - 1; bit >= 0; --bit)
                if ((k >> bit) & 1u)
                    ++operation_count;
        }
        const int max_inline_operations = size_opt_enabled() ? 20 : 30;
        return msb < 0 || operation_count <= max_inline_operations;
    };

    for (const auto &[tid, iv] : ivs) {
        if (iv.size != 2 || iv.has_addr_of || iv.first_def < 0 ||
            iv.last_use <= iv.first_def || temp_regs_.count(tid))
            continue;

        const icode &init = fn.icodes[iv.first_def];
        if (init.op != icode_op::ASSIGN || !init.result.is_temp() ||
            init.result.temp_id != tid ||
            init.left.kind != operand_kind::INT_CONST ||
            init.left.ival <= 1 || init.left.ival > 65535)
            continue;

        bool safe = true;
        bool saw_step = false;
        bool saw_backedge = false;
        int end = -1;
        for (int k = iv.first_def + 1; k <= iv.last_use; ++k) {
            const icode &ic = fn.icodes[k];
            if (mentions_temp(ic, tid)) {
                const bool step =
                    ic.op == icode_op::SUB && ic.result.is_temp() &&
                    ic.result.temp_id == tid && ic.left.is_temp() &&
                    ic.left.temp_id == tid &&
                    ic.right.kind == operand_kind::INT_CONST &&
                    ic.right.ival == 1;
                if (step) {
                    saw_step = true;
                    continue;
                }
                const bool branch =
                    ic.op == icode_op::IFX && ic.left.is_temp() &&
                    ic.left.temp_id == tid;
                if (branch && saw_step) {
                    auto note_target = [&](const std::string &label) {
                        auto it = label_indices.find(label);
                        if (it != label_indices.end() &&
                            it->second > iv.first_def && it->second < k) {
                            saw_backedge = true;
                            end = k;
                        }
                    };
                    note_target(ic.true_lbl);
                    note_target(ic.false_lbl);
                    if (saw_backedge)
                        continue;
                }
                safe = false;
                break;
            }

            if (inline_word_const_mul_preserves_bc(ic))
                continue;
            // A direct call with register-only arguments can cheaply save a
            // loop-carried BC value at the call site.  All preceding SENDs
            // are still checked normally, so this admits the call only when
            // argument lowering itself preserves BC.
            if (ic.op == icode_op::CALL && !ic.func_name.empty() &&
                ic.arg_bytes == 0) {
                continue;
            }
            const bool simple_direct_operation =
                direct_ix_frame &&
                (ic.op == icode_op::LABEL || ic.op == icode_op::GOTO ||
                 ic.op == icode_op::IFX || ic.op == icode_op::ASSIGN ||
                 ic.op == icode_op::CAST || ic.op == icode_op::ADD ||
                 ic.op == icode_op::SUB || ic.op == icode_op::BAND ||
                 ic.op == icode_op::BOR || ic.op == icode_op::BXOR ||
                 ic.op == icode_op::BNOT || ic.op == icode_op::SHL ||
                 ic.op == icode_op::SHR || ic.op == icode_op::ADDRESS_OF ||
                 ic.op == icode_op::GET_VALUE_AT ||
                 ic.op == icode_op::SET_VALUE_AT) &&
                !clobbers_bc(ic) &&
                !uses_tls_global(ic.result) &&
                !uses_tls_global(ic.left) &&
                !uses_tls_global(ic.right) &&
                !(ic.op == icode_op::ADDRESS_OF &&
                  address_of_may_need_bc_scratch(ic.left)) &&
                !symbol_word_access_may_need_bc_scratch(ic.result) &&
                !symbol_word_access_may_need_bc_scratch(ic.left) &&
                !symbol_word_access_may_need_bc_scratch(ic.right);
            if (!simple_direct_operation && loop_bc_scratch_hazard(ic)) {
                safe = false;
                break;
            }
        }
        if (!safe || !saw_step || !saw_backedge || end < 0)
            continue;
        loop_countdown_candidates.push_back(
            {tid, iv.first_def, end,
             3600 + hot_mentions(iv) * 20 - (end - iv.first_def)});
    }
    std::sort(loop_countdown_candidates.begin(),
              loop_countdown_candidates.end(),
              [](const loop_countdown_candidate &lhs,
                 const loop_countdown_candidate &rhs) {
                  if (lhs.score != rhs.score)
                      return lhs.score > rhs.score;
                  if (lhs.start != rhs.start)
                      return lhs.start < rhs.start;
                  return lhs.tid < rhs.tid;
              });
    for (const auto &cand : loop_countdown_candidates) {
        bool overlaps = false;
        for (const auto &[start, end] : pair_windows) {
            if (!(cand.end < start || cand.start > end)) {
                overlaps = true;
                break;
            }
        }
        if (overlaps)
            continue;
        temp_regs_[cand.tid] = temp_home::main_bc;
        pair_windows.push_back({cand.start, cand.end});
        for (int k = cand.start + 1; k < cand.end; ++k) {
            const icode &ic = fn.icodes[k];
            if (ic.op == icode_op::CALL && !ic.func_name.empty() &&
                ic.arg_bytes == 0) {
                bc_preserved_call_indices_.insert(static_cast<size_t>(k));
            }
        }
        break;
    }

    // IY can hold only one cursor at a time.  In a call-free dual-cursor loop
    // (string comparison is the archetypal case), retain the other cursor in
    // BC when every instruction in the shared window has a BC-preserving
    // lowering.  The candidates come from the same structural pointer-walk
    // proof used for IY; this is ordinary live-range allocation, not a helper
    // or source-pattern substitution.
    for (const auto &cand : spare_iy_cursor_candidates) {
        auto iv_it = ivs.find(cand.tid);
        if (iv_it == ivs.end() || temp_regs_.count(cand.tid))
            continue;

        bool safe = true;
        for (int k = cand.start + 1; k <= cand.end; ++k) {
            const icode &ic = fn.icodes[k];
            auto is_cursor = [&](const operand &op) {
                return op.is_temp() && op.temp_id == cand.tid;
            };
            auto is_iy_home = [&](const operand &op) {
                if (!op.is_temp())
                    return false;
                auto home = temp_regs_.find(op.temp_id);
                return home != temp_regs_.end() &&
                       home->second == temp_home::main_iy;
            };

            if (mentions_temp(ic, cand.tid)) {
                const bool cursor_use =
                    (ic.op == icode_op::GET_VALUE_AT &&
                     is_cursor(ic.left) && ic.right.is_none() &&
                     op_size(ic.result) >= 1 && op_size(ic.result) <= 2) ||
                    (ic.op == icode_op::SET_VALUE_AT &&
                     is_cursor(ic.result) && ic.right.is_none() &&
                     op_size(ic.left) >= 1 && op_size(ic.left) <= 2) ||
                    ((ic.op == icode_op::ADD || ic.op == icode_op::SUB) &&
                     is_cursor(ic.left) &&
                     ic.right.kind == operand_kind::INT_CONST) ||
                    (ic.op == icode_op::ASSIGN && is_cursor(ic.result)) ||
                    (ic.op == icode_op::IFX && is_cursor(ic.left)) ||
                    (is_compare_op(ic.op) &&
                     (is_cursor(ic.left) || is_cursor(ic.right)));
                if (!cursor_use) {
                    safe = false;
                    break;
                }
                continue;
            }

            if (ic.op == icode_op::LABEL || ic.op == icode_op::GOTO ||
                ic.op == icode_op::IFX)
                continue;
            const bool safe_iy_operation =
                (ic.op == icode_op::GET_VALUE_AT &&
                 is_iy_home(ic.left) && ic.right.is_none() &&
                 op_size(ic.result) >= 1 && op_size(ic.result) <= 2) ||
                (ic.op == icode_op::SET_VALUE_AT &&
                 is_iy_home(ic.result) && ic.right.is_none() &&
                 op_size(ic.left) >= 1 && op_size(ic.left) <= 2) ||
                ((ic.op == icode_op::ADD || ic.op == icode_op::SUB) &&
                 is_iy_home(ic.left) &&
                 ic.right.kind == operand_kind::INT_CONST) ||
                (ic.op == icode_op::ASSIGN && is_iy_home(ic.result));
            if (safe_iy_operation)
                continue;
            const bool fused_byte_compare =
                is_compare_op(ic.op) && ic.result.is_temp() &&
                op_size(ic.left) == 1 && op_size(ic.right) == 1 &&
                k + 1 <= cand.end &&
                fn.icodes[k + 1].op == icode_op::IFX &&
                fn.icodes[k + 1].left.is_temp() &&
                fn.icodes[k + 1].left.temp_id == ic.result.temp_id;
            if (fused_byte_compare)
                continue;
            if (clobbers_bc(ic) ||
                bc_backend_hazard(ic, direct_ix_frame)) {
                safe = false;
                break;
            }
        }
        if (!safe)
            continue;
        temp_regs_[cand.tid] = temp_home::main_bc;
        pair_windows.push_back({cand.start, cand.end});
        break;
    }

    struct loop_accumulator_candidate {
        int tid = -1;
        int start = -1;
        int end = -1;
        int score = 0;
        std::vector<int> aliases;
    };
    std::vector<loop_accumulator_candidate> loop_accumulator_candidates;
    for (const auto &[tid, iv] : ivs) {
        if (iv.size != 2 || iv.has_addr_of || iv.first_def < 0 ||
            iv.last_use <= iv.first_def ||
            temp_regs_.find(tid) != temp_regs_.end()) {
            continue;
        }

        const icode &init = fn.icodes[iv.first_def];
        if (init.op != icode_op::ASSIGN || !init.result.is_temp() ||
            init.result.temp_id != tid ||
            init.left.kind != operand_kind::INT_CONST) {
            continue;
        }

        bool safe = true;
        bool saw_update = false;
        bool saw_backedge = false;
        int update_count = 0;
        int current_accumulator = tid;
        std::vector<int> accumulator_aliases;
        for (int k = iv.first_def + 1; k <= iv.last_use; ++k) {
            const icode &ic = fn.icodes[k];

            auto note_backedge = [&](const std::string &label) {
                auto it = label_indices.find(label);
                if (it != label_indices.end() &&
                    it->second > iv.first_def && it->second < k) {
                    saw_backedge = true;
                }
            };
            if (ic.op == icode_op::GOTO)
                note_backedge(ic.label_name);
            else if (ic.op == icode_op::IFX) {
                note_backedge(ic.true_lbl);
                note_backedge(ic.false_lbl);
            }
            if (saw_backedge && current_accumulator != tid) {
                safe = false;
                break;
            }

            const bool reduction_update =
                (ic.op == icode_op::ADD || ic.op == icode_op::SUB) &&
                ic.result.is_temp() &&
                ic.left.is_temp() &&
                ic.left.temp_id == current_accumulator &&
                !ic.right.is_none() &&
                !(ic.right.is_temp() &&
                  (ic.right.temp_id == tid ||
                   ic.right.temp_id == current_accumulator)) &&
                op_size(ic.right) <= 2 &&
                loop_depth[k] > 0 &&
                !uses_tls_global(ic.right) &&
                !symbol_word_access_may_need_bc_scratch(ic.right);
            if (reduction_update) {
                if (current_accumulator != tid) {
                    auto current_iv = ivs.find(current_accumulator);
                    if (current_iv == ivs.end() ||
                        current_iv->second.last_use != k) {
                        safe = false;
                        break;
                    }
                }
                const int result_tid = ic.result.temp_id;
                if (result_tid != tid) {
                    auto result_iv = ivs.find(result_tid);
                    if (result_iv == ivs.end() ||
                        result_iv->second.first_def != k ||
                        result_iv->second.last_use <= k ||
                        result_iv->second.has_addr_of ||
                        temp_regs_.find(result_tid) != temp_regs_.end()) {
                        safe = false;
                        break;
                    }
                    accumulator_aliases.push_back(result_tid);
                }
                current_accumulator = result_tid;
                saw_update = true;
                ++update_count;
                continue;
            }

            const bool commits_accumulator_alias =
                current_accumulator != tid &&
                ic.op == icode_op::ASSIGN &&
                ic.result.is_temp() && ic.result.temp_id == tid &&
                ic.left.is_temp() &&
                ic.left.temp_id == current_accumulator;
            if (commits_accumulator_alias) {
                auto current_iv = ivs.find(current_accumulator);
                if (current_iv == ivs.end() ||
                    current_iv->second.last_use != k) {
                    safe = false;
                    break;
                }
                current_accumulator = tid;
                continue;
            }

            if (mentions_temp(ic, tid)) {
                const bool final_read =
                    (ic.op == icode_op::RETURN && ic.left.is_temp() &&
                     ic.left.temp_id == tid) ||
                    (ic.op == icode_op::ADD && ic.right.is_temp() &&
                     ic.right.temp_id == tid &&
                     (!ic.result.is_temp() || ic.result.temp_id != tid)) ||
                    ((ic.op == icode_op::ASSIGN || ic.op == icode_op::CAST) &&
                     ic.left.is_temp() && ic.left.temp_id == tid &&
                     (!ic.result.is_temp() || ic.result.temp_id != tid)) ||
                    ((ic.op == icode_op::BAND || ic.op == icode_op::BOR ||
                      ic.op == icode_op::BXOR) &&
                     ic.left.is_temp() && ic.left.temp_id == tid &&
                     (!ic.result.is_temp() || ic.result.temp_id != tid) &&
                     op_size(ic.right) <= 2);
                if (!final_read) {
                    safe = false;
                    break;
                }
                continue;
            }

            auto is_iy_home = [&](const operand &op) {
                if (!op.is_temp())
                    return false;
                auto home = temp_regs_.find(op.temp_id);
                return home != temp_regs_.end() &&
                       home->second == temp_home::main_iy;
            };
            auto is_iy_offset = [&](const operand &op, int size) {
                if (!op.is_temp())
                    return false;
                auto q = ivs.find(op.temp_id);
                if (q == ivs.end() || q->second.first_def < 0 ||
                    q->second.first_def >= k)
                    return false;
                const icode &def = fn.icodes[q->second.first_def];
                if (def.op != icode_op::ADD || !def.result.is_temp() ||
                    def.result.temp_id != op.temp_id)
                    return false;
                const operand *base = &def.left;
                const operand *offset = &def.right;
                if (base->kind == operand_kind::INT_CONST)
                    std::swap(base, offset);
                return is_iy_home(*base) &&
                       offset->kind == operand_kind::INT_CONST &&
                       offset->ival >= -128 &&
                       offset->ival + size - 1 <= 127;
            };
            const bool safe_iy_operation =
                (ic.op == icode_op::GET_VALUE_AT &&
                 is_iy_home(ic.left) &&
                 ic.right.is_none() &&
                 op_size(ic.result) >= 1 && op_size(ic.result) <= 2) ||
                (ic.op == icode_op::SET_VALUE_AT &&
                 (is_iy_home(ic.result) ||
                  is_iy_offset(ic.result, op_size(ic.left))) &&
                 ic.right.is_none() &&
                 op_size(ic.left) >= 1 && op_size(ic.left) <= 2) ||
                ((ic.op == icode_op::ADD || ic.op == icode_op::SUB) &&
                 is_iy_home(ic.left) &&
                 ic.right.kind == operand_kind::INT_CONST) ||
                (ic.op == icode_op::ASSIGN && is_iy_home(ic.result));
            if (safe_iy_operation)
                continue;

            const bool safe_global_address =
                ic.op == icode_op::ADDRESS_OF && direct_ix_frame &&
                ic.result.is_temp() && ic.result.type &&
                ic.result.type->size() == 2 &&
                ((ic.left.kind == operand_kind::SYMBOL && ic.left.is_global &&
                  !ic.left.is_tls && !ic.left.is_sfr && !ic.left.is_func) ||
                 ic.left.kind == operand_kind::LABEL_REF);
            if (safe_global_address)
                continue;

            // Preserve a loop reduction in BC across a direct call whose
            // arguments are register-only. SEND lowering is still examined
            // independently above/below, so only BC-safe argument setup is
            // admitted. This lets checksums and counters consume call results
            // without a spill/reload on every iteration.
            if (ic.op == icode_op::CALL && !ic.func_name.empty() &&
                ic.arg_bytes == 0) {
                continue;
            }

            const bool fused_byte_compare =
                is_compare_op(ic.op) && ic.result.is_temp() &&
                op_size(ic.left) == 1 && op_size(ic.right) == 1 &&
                k + 1 <= iv.last_use &&
                fn.icodes[k + 1].op == icode_op::IFX &&
                fn.icodes[k + 1].left.is_temp() &&
                fn.icodes[k + 1].left.temp_id == ic.result.temp_id;
            if (fused_byte_compare)
                continue;

            if (loop_bc_scratch_hazard(ic)) {
                safe = false;
                break;
            }
        }
        if (!safe || !saw_update || !saw_backedge)
            continue;
        loop_accumulator_candidates.push_back(
            {tid, iv.first_def, iv.last_use,
             3000 + update_count * 200 + hot_mentions(iv) * 16 -
                 (iv.last_use - iv.first_def),
             std::move(accumulator_aliases)});
    }
    std::sort(loop_accumulator_candidates.begin(),
              loop_accumulator_candidates.end(),
              [](const loop_accumulator_candidate &lhs,
                 const loop_accumulator_candidate &rhs) {
                  if (lhs.score != rhs.score)
                      return lhs.score > rhs.score;
                  if (lhs.start != rhs.start)
                      return lhs.start < rhs.start;
                  return lhs.tid < rhs.tid;
              });
    for (const auto &cand : loop_accumulator_candidates) {
        bool overlaps = false;
        for (const auto &[start, end] : pair_windows) {
            if (!(cand.end < start || cand.start > end)) {
                overlaps = true;
                break;
            }
        }
        if (overlaps)
            continue;
        temp_regs_[cand.tid] = temp_home::main_bc;
        for (int alias_tid : cand.aliases)
            temp_regs_[alias_tid] = temp_home::main_bc;
        pair_windows.push_back({cand.start, cand.end});
        for (int k = cand.start + 1; k < cand.end; ++k) {
            const icode &ic = fn.icodes[k];
            if (ic.op == icode_op::CALL && !ic.func_name.empty() &&
                ic.arg_bytes == 0) {
                bc_preserved_call_indices_.insert(static_cast<size_t>(k));
            }
        }
    }

    // Prove the range of canonical zero-based induction values independently
    // of whether BC is available for them.  For `i = 0; i < K; ++i`, K <=
    // 255 guarantees a zero high byte and permits comparing only the low
    // byte; K == 256 permits testing only the high byte.  Require a top-test,
    // one +1 update, and a backedge to that test, with no payload use of the
    // induction value.  This is a range proof, not a source-level loop-name
    // or benchmark pattern.
    for (const auto &[tid, iv] : ivs) {
        if (iv.size != 2 || iv.has_addr_of || iv.first_def < 0)
            continue;
        const icode &init = fn.icodes[iv.first_def];
        if (init.op != icode_op::ASSIGN || !init.result.is_temp() ||
            init.result.temp_id != tid ||
            init.left.kind != operand_kind::INT_CONST || init.left.ival != 0)
            continue;

        for (int cmp_idx = iv.first_def + 1; cmp_idx + 1 < n; ++cmp_idx) {
            const icode &cmp = fn.icodes[cmp_idx];
            const icode &ifx = fn.icodes[cmp_idx + 1];
            if (cmp.op != icode_op::LT || !cmp.result.is_temp() ||
                !cmp.left.is_temp() || cmp.left.temp_id != tid ||
                cmp.left.byte_offset != 0 ||
                cmp.right.kind != operand_kind::INT_CONST ||
                cmp.right.ival <= 1 || cmp.right.ival > 256 ||
                ifx.op != icode_op::IFX || !ifx.left.is_temp() ||
                ifx.left.temp_id != cmp.result.temp_id)
                continue;

            int backedge_idx = -1;
            int header_idx = -1;
            for (int k = cmp_idx + 2; k < n; ++k) {
                auto note_backedge = [&](const std::string &label) {
                    auto found = label_indices.find(label);
                    if (found != label_indices.end() &&
                        found->second > iv.first_def &&
                        found->second <= cmp_idx) {
                        backedge_idx = k;
                        header_idx = found->second;
                    }
                };
                if (fn.icodes[k].op == icode_op::GOTO)
                    note_backedge(fn.icodes[k].label_name);
                else if (fn.icodes[k].op == icode_op::IFX) {
                    note_backedge(fn.icodes[k].true_lbl);
                    note_backedge(fn.icodes[k].false_lbl);
                }
                if (backedge_idx >= 0)
                    break;
            }
            if (backedge_idx < 0 || header_idx < 0)
                continue;

            auto target_index = [&](const std::string &label) {
                auto found = label_indices.find(label);
                return found == label_indices.end() ? -1 : found->second;
            };
            const int true_idx = target_index(ifx.true_lbl);
            const int false_idx = target_index(ifx.false_lbl);
            const bool true_is_body =
                true_idx > cmp_idx && true_idx <= backedge_idx;
            const bool false_is_body =
                false_idx > cmp_idx && false_idx <= backedge_idx;
            const bool true_is_exit = true_idx > backedge_idx;
            const bool false_is_exit = false_idx > backedge_idx;
            if (!((true_is_body && false_is_exit) ||
                  (false_is_body && true_is_exit)))
                continue;

            int step_tid = -1;
            int step_count = 0;
            bool safe = true;
            for (int k = header_idx; k <= backedge_idx; ++k) {
                const icode &ic = fn.icodes[k];
                if (k == cmp_idx)
                    continue;
                if (!mentions_temp(ic, tid))
                    continue;
                const bool step =
                    ic.op == icode_op::ADD && ic.left.is_temp() &&
                    ic.left.temp_id == tid &&
                    ic.right.kind == operand_kind::INT_CONST &&
                    ic.right.ival == 1 && ic.result.is_temp();
                if (step) {
                    step_tid = ic.result.temp_id;
                    ++step_count;
                    continue;
                }
                const bool commit =
                    step_tid >= 0 && ic.op == icode_op::ASSIGN &&
                    ic.result.is_temp() && ic.result.temp_id == tid &&
                    ic.left.is_temp() && ic.left.temp_id == step_tid;
                if (!commit) {
                    safe = false;
                    break;
                }
            }
            if (safe && step_count == 1) {
                bounded_induction_limits_[tid] =
                    static_cast<int>(cmp.right.ival);
                break;
            }
        }
    }

    // Apply the same range proof to direct local variables.  Unlike SSA-like
    // temps, one source variable can be reused by several loops, so record the
    // exact comparison and increment instructions rather than attaching a
    // function-wide fact to the symbol.  Payload reads are harmless: only
    // definitions can invalidate the proven [0, K] range.
    for (const auto &[key, siv] : syms) {
        (void)key;
        if (!siv.base.type || siv.base.type->size() != 2 ||
            !siv.base.type->is_integer() || siv.has_addr_of)
            continue;

        auto is_induction_symbol = [&](const operand &op) {
            return op.is_symbol() && !op.is_global && !op.is_tls &&
                   !op.is_sfr && !op.is_func && op.byte_offset == 0 &&
                   same_local_symbol_base(op, siv.base);
        };
        auto target_index = [&](const std::string &label) {
            auto found = label_indices.find(label);
            return found == label_indices.end() ? -1 : found->second;
        };

        for (int cmp_idx = std::max(0, siv.first_idx);
             cmp_idx + 1 < n; ++cmp_idx) {
            const icode &cmp = fn.icodes[cmp_idx];
            const icode &ifx = fn.icodes[cmp_idx + 1];
            if (cmp.op != icode_op::LT || !cmp.result.is_temp() ||
                !is_induction_symbol(cmp.left) ||
                cmp.right.kind != operand_kind::INT_CONST ||
                cmp.right.ival <= 1 || cmp.right.ival > 256 ||
                ifx.op != icode_op::IFX || !ifx.left.is_temp() ||
                ifx.left.temp_id != cmp.result.temp_id)
                continue;

            int init_idx = -1;
            bool init_is_zero = false;
            for (int k = siv.first_idx; k < cmp_idx; ++k) {
                const icode &ic = fn.icodes[k];
                if (!is_induction_symbol(ic.result))
                    continue;
                init_idx = k;
                init_is_zero =
                    ic.op == icode_op::ASSIGN &&
                    ic.left.kind == operand_kind::INT_CONST &&
                    ic.left.ival == 0;
            }
            if (init_idx < 0 || !init_is_zero)
                continue;

            int backedge_idx = -1;
            int header_idx = -1;
            for (int k = cmp_idx + 2; k < n; ++k) {
                auto note_backedge = [&](const std::string &label) {
                    auto found = label_indices.find(label);
                    if (found != label_indices.end() &&
                        found->second > init_idx &&
                        found->second <= cmp_idx) {
                        backedge_idx = k;
                        header_idx = found->second;
                    }
                };
                if (fn.icodes[k].op == icode_op::GOTO)
                    note_backedge(fn.icodes[k].label_name);
                else if (fn.icodes[k].op == icode_op::IFX) {
                    note_backedge(fn.icodes[k].true_lbl);
                    note_backedge(fn.icodes[k].false_lbl);
                }
                if (backedge_idx >= 0)
                    break;
            }
            if (backedge_idx < 0 || header_idx < 0)
                continue;

            const int true_idx = target_index(ifx.true_lbl);
            const int false_idx = target_index(ifx.false_lbl);
            const bool true_is_body =
                true_idx > cmp_idx && true_idx <= backedge_idx;
            const bool false_is_body =
                false_idx > cmp_idx && false_idx <= backedge_idx;
            const bool true_is_exit = true_idx > backedge_idx;
            const bool false_is_exit = false_idx > backedge_idx;
            if (!((true_is_body && false_is_exit) ||
                  (false_is_body && true_is_exit)))
                continue;

            int step_idx = -1;
            int step_count = 0;
            bool safe = true;
            for (int k = header_idx; k <= backedge_idx; ++k) {
                const icode &ic = fn.icodes[k];
                if (!is_induction_symbol(ic.result))
                    continue;
                const bool direct_step =
                    ic.op == icode_op::ADD &&
                    is_induction_symbol(ic.left) &&
                    ic.right.kind == operand_kind::INT_CONST &&
                    ic.right.ival == 1;
                if (!direct_step) {
                    safe = false;
                    break;
                }
                step_idx = k;
                ++step_count;
            }
            if (!safe || step_count != 1)
                continue;

            bounded_symbol_induction_comparisons_[
                static_cast<size_t>(cmp_idx)] =
                static_cast<int>(cmp.right.ival);
            if (cmp.right.ival <= 255)
                bounded_symbol_induction_increments_.insert(
                    static_cast<size_t>(step_idx));
        }
    }

    struct loop_induction_candidate {
        int tid = -1;
        int start = -1;
        int end = -1;
        int score = 0;
    };
    auto may_inline_ctype_call = [&](const icode &ic) {
        if (!opt_settings_.ctype_builtins || ic.func_name.empty() ||
            defined_function_names_.count(ic.func_name) != 0 ||
            effective_call_abi(ic.callee_abi) != call_abi::SDCCCALL1 ||
            ic.num_params != 1 || ic.arg_bytes != 0) {
            return false;
        }
        static const std::unordered_set<std::string> names = {
            "isalnum", "isalpha", "isblank", "iscntrl", "isdigit",
            "isgraph", "islower", "isprint", "ispunct", "isspace",
            "isupper", "isxdigit", "tolower", "toupper",
        };
        return names.count(ic.func_name) != 0;
    };
    std::vector<loop_induction_candidate> loop_induction_candidates;
    for (const auto &[tid, iv] : ivs) {
        if (iv.size != 2 || iv.has_addr_of || iv.first_def < 0)
            continue;

        const icode &init = fn.icodes[iv.first_def];
        if (init.op != icode_op::ASSIGN ||
            !init.result.is_temp() || init.result.temp_id != tid ||
            !init.result.type || !init.result.type->is_integer() ||
            init.left.kind != operand_kind::INT_CONST || init.left.ival != 0) {
            continue;
        }

        int last_touch = iv.first_def;
        int loop_end = -1;
        for (int k = iv.first_def + 1; k < n; ++k) {
            if (mentions_temp(fn.icodes[k], tid))
                last_touch = k;

            auto note_backedge = [&](const std::string &label) {
                auto it = label_indices.find(label);
                if (it == label_indices.end())
                    return;
                if (it->second > iv.first_def && it->second <= last_touch &&
                    it->second < k) {
                    loop_end = std::max(loop_end, k);
                }
            };
            if (fn.icodes[k].op == icode_op::GOTO) {
                note_backedge(fn.icodes[k].label_name);
            } else if (fn.icodes[k].op == icode_op::IFX) {
                note_backedge(fn.icodes[k].true_lbl);
                note_backedge(fn.icodes[k].false_lbl);
            }
        }
        if (loop_end < 0)
            continue;
        const int allocation_end = std::max(loop_end, iv.last_use);

        bool safe = true;
        bool saw_update = false;
        int compare_uses = 0;
        std::unordered_set<int> step_temps;
        for (int k = iv.first_def + 1; k <= allocation_end; ++k) {
            const icode &ic = fn.icodes[k];
            const bool preservable_direct_call =
                ic.op == icode_op::CALL && !ic.func_name.empty() &&
                ic.arg_bytes == 0 && !ic.result_via_sret &&
                !may_inline_ctype_call(ic);
            const bool preserves_bc_despite_metadata =
                preservable_direct_call ||
                inline_word_const_mul_preserves_bc(ic);
            if ((!preserves_bc_despite_metadata &&
                 (ic.op == icode_op::CALL || clobbers_bc(ic))) ||
                ic.op == icode_op::ALLOCA ||
                ic.op == icode_op::INLINE_ASM ||
                uses_tls_global(ic.result) || uses_tls_global(ic.left) ||
                uses_tls_global(ic.right) ||
                symbol_word_access_may_need_bc_scratch(ic.result) ||
                symbol_word_access_may_need_bc_scratch(ic.left) ||
                symbol_word_access_may_need_bc_scratch(ic.right) ||
                (ic.op == icode_op::ADDRESS_OF &&
                 address_of_may_need_bc_scratch(ic.left))) {
                safe = false;
                break;
            }

            const bool uses_candidate = mentions_temp(ic, tid);
            if (uses_candidate) {
                if ((ic.op == icode_op::ADD || ic.op == icode_op::SUB) &&
                    ic.left.is_temp() && ic.left.temp_id == tid &&
                    ic.right.kind == operand_kind::INT_CONST &&
                    ic.right.ival == 1 && ic.result.is_temp()) {
                    step_temps.insert(ic.result.temp_id);
                    saw_update = true;
                    continue;
                }
                if (ic.op == icode_op::ASSIGN && ic.result.is_temp() &&
                    ic.result.temp_id == tid && ic.left.is_temp() &&
                    step_temps.count(ic.left.temp_id)) {
                    continue;
                }
                if (is_compare_op(ic.op) &&
                    ((ic.left.is_temp() && ic.left.temp_id == tid) ||
                     (ic.right.is_temp() && ic.right.temp_id == tid))) {
                    ++compare_uses;
                    continue;
                }
                if ((ic.op == icode_op::ASSIGN || ic.op == icode_op::CAST) &&
                    ic.result.is_temp() && ic.result.temp_id != tid &&
                    ic.left.is_temp() && ic.left.temp_id == tid) {
                    continue;
                }
                if ((ic.op == icode_op::ADD || ic.op == icode_op::SUB) &&
                    ic.result.is_temp() && ic.result.temp_id != tid &&
                    ic.result.type && ic.result.type->size() == 2 &&
                    temp_regs_.find(ic.result.temp_id) == temp_regs_.end()) {
                    const bool use_left =
                        ic.left.is_temp() && ic.left.temp_id == tid;
                    const bool use_right =
                        ic.right.is_temp() && ic.right.temp_id == tid;
                    const operand &other = use_left ? ic.right : ic.left;
                    const bool direct_other =
                        other.kind == operand_kind::INT_CONST ||
                        (direct_ix_frame && other.is_temp() &&
                         temp_regs_.find(other.temp_id) == temp_regs_.end()) ||
                        (direct_ix_frame && other.is_symbol() &&
                         !other.is_global && !other.is_tls);
                    if (use_left != use_right && direct_other) {
                        // Loading the BC value into HL/DE is a pair copy; a
                        // direct IX-slot counterpart and result also avoid BC.
                        // This lets induction values participate in address or
                        // distance arithmetic without forcing a spill.
                        continue;
                    }
                }
                safe = false;
                break;
            }

            if (preserves_bc_despite_metadata)
                continue;

            switch (ic.op) {
            case icode_op::LABEL:
            case icode_op::GOTO:
            case icode_op::IFX:
            case icode_op::ADDRESS_OF:
            case icode_op::EQ:
            case icode_op::NE:
            case icode_op::LT:
            case icode_op::LE:
            case icode_op::GT:
            case icode_op::GE:
                continue;
            case icode_op::SET_VALUE_AT:
                if (direct_ix_frame && ic.left.type &&
                    (ic.left.type->size() == 1 ||
                     ic.left.type->size() == 2) &&
                    ic.result.is_temp() &&
                    temp_regs_.find(ic.result.temp_id) == temp_regs_.end() &&
                    (!ic.left.is_temp() ||
                     temp_regs_.find(ic.left.temp_id) == temp_regs_.end())) {
                    // A plain IX-slot value stored through a plain IX-slot
                    // near pointer uses HL for the address and A/DE for the
                    // payload.  It therefore preserves a loop induction
                    // value in BC even for a word store.  The direct-frame
                    // requirement guarantees both slots fit indexed loads;
                    // allocated/rematerialized operands stay excluded because
                    // their lowering may need BC as scratch.
                    continue;
                }
                safe = false;
                break;
            default:
                if (bc_backend_hazard(ic, direct_ix_frame))
                    safe = false;
                break;
            }
            if (!safe)
                break;
        }
        if (!safe || !saw_update || compare_uses == 0)
            continue;

        loop_induction_candidates.push_back(
            {tid, iv.first_def, allocation_end,
             2200 + compare_uses * 160 + hot_mentions(iv) * 12 -
                 (allocation_end - iv.first_def)});
    }
    std::sort(loop_induction_candidates.begin(),
              loop_induction_candidates.end(),
              [](const loop_induction_candidate &lhs,
                 const loop_induction_candidate &rhs) {
                  if (lhs.score != rhs.score)
                      return lhs.score > rhs.score;
                  if (lhs.start != rhs.start)
                      return lhs.start < rhs.start;
                  return lhs.tid < rhs.tid;
              });
    for (const auto &cand : loop_induction_candidates) {
        bool overlaps = false;
        for (const auto &[start, end] : pair_windows) {
            if (!(cand.end < start || cand.start > end)) {
                overlaps = true;
                break;
            }
        }
        if (overlaps || temp_regs_.find(cand.tid) != temp_regs_.end())
            continue;
        temp_regs_[cand.tid] = temp_home::main_bc;
        pair_windows.push_back({cand.start, cand.end});
        for (int k = cand.start + 1; k < cand.end; ++k) {
            const icode &ic = fn.icodes[k];
            if (ic.op == icode_op::CALL && !ic.func_name.empty() &&
                ic.arg_bytes == 0 && !ic.result_via_sret &&
                !may_inline_ctype_call(ic)) {
                bc_preserved_call_indices_.insert(static_cast<size_t>(k));
            }
        }
    }

    // Step 4a: gather BC candidates from both word temps and simple
    // 16-bit local / parameter symbols. The symbol path is intentionally
    // conservative: it only handles short contiguous windows where the
    // symbol can stay in BC for the whole interval without address-taking
    // or call/barrier hazards.
    auto first_use_is_temp_copy = [&](int temp_id, const interval &iv) {
        for (int k = iv.first_def + 1; k <= iv.last_use; ++k) {
            const icode &use_ic = fn.icodes[k];
            const bool use_left =
                use_ic.left.is_temp() && use_ic.left.temp_id == temp_id;
            const bool use_right =
                use_ic.right.is_temp() && use_ic.right.temp_id == temp_id;
            if (!use_left && !use_right)
                continue;
            return use_left && !use_right &&
                   (use_ic.op == icode_op::ASSIGN ||
                    use_ic.op == icode_op::CAST) &&
                   use_ic.result.is_temp();
        }
        return false;
    };
    auto bc_temp_uses_are_backend_safe = [&](int temp_id, int start, int end) {
        for (int k = start + 1; k <= end; ++k) {
            const icode &use_ic = fn.icodes[k];
            const bool use_right =
                use_ic.right.is_temp() && use_ic.right.temp_id == temp_id;
            const bool use_left =
                use_ic.left.is_temp() && use_ic.left.temp_id == temp_id;

            // Binary emitters prepare HL from the left operand before copying
            // a BC-resident right operand to DE. Rematerializing a local frame
            // address uses BC itself, so that operand order cannot retain the
            // right value in BC safely.
            if (use_right && !use_left &&
                hl_load_may_clobber_bc(use_ic.left, 0)) {
                return false;
            }
        }
        return true;
    };
    std::vector<bc_candidate> bc_candidates;

    // A scaled byte offset reused for two or more accesses to global tables is
    // especially valuable in BC: each address then becomes `ld hl,table` /
    // `add hl,bc`, and the intervening load can use DE.  This is a short,
    // straight-line live range (parsers, bytecode engines and lookup kernels
    // commonly produce it), so admit it only when every interior lowering is
    // known to preserve BC.
    for (auto &[fd, tid] : order) {
        const interval &iv = ivs[tid];
        auto existing_home = temp_regs_.find(tid);
        if (iv.size != 2 || iv.has_addr_of || iv.first_def < 0 ||
            iv.last_use <= iv.first_def || iv.mentions < 2 ||
            (existing_home != temp_regs_.end() &&
             existing_home->second != temp_home::stack)) {
            continue;
        }
        const icode &def = fn.icodes[iv.first_def];
        if (def.op != icode_op::SHL || !def.result.is_temp() ||
            def.result.temp_id != tid ||
            def.right.kind != operand_kind::INT_CONST ||
            def.right.ival < 1 || def.right.ival > 7) {
            continue;
        }

        auto is_global_table_base = [&](const operand &op) {
            if (op.kind == operand_kind::LABEL_REF)
                return true;
            if (op.kind == operand_kind::SYMBOL && op.is_global &&
                !op.is_tls && !op.is_func)
                return true;
            if (!op.is_temp())
                return false;
            auto base_iv = ivs.find(op.temp_id);
            if (base_iv == ivs.end() || base_iv->second.first_def < 0 ||
                base_iv->second.first_def >= iv.first_def)
                return false;
            const icode &base_def = fn.icodes[base_iv->second.first_def];
            return base_def.op == icode_op::ADDRESS_OF &&
                   base_def.left.kind == operand_kind::SYMBOL &&
                   base_def.left.is_global && !base_def.left.is_tls &&
                   !base_def.left.is_func;
        };

        bool safe = true;
        int table_uses = 0;
        for (int k = iv.first_def + 1; k <= iv.last_use; ++k) {
            const icode &use = fn.icodes[k];
            const bool left = use.left.is_temp() && use.left.temp_id == tid;
            const bool right = use.right.is_temp() && use.right.temp_id == tid;
            const bool result = use.result.is_temp() &&
                                use.result.temp_id == tid;
            if (left || right || result) {
                if (result || left == right || use.op != icode_op::ADD ||
                    !use.result.is_temp() ||
                    !is_global_table_base(left ? use.right : use.left)) {
                    safe = false;
                    break;
                }
                ++table_uses;
                continue;
            }
            if (clobbers_bc(use) || bc_backend_hazard(use, direct_ix_frame)) {
                safe = false;
                break;
            }
        }
        if (!safe || table_uses < 2)
            continue;
        // Earlier paired-probe planning may have reserved an ordinary stack
        // home for a comparison operand.  This independently proven BC-safe
        // table offset has a stronger, non-overlapping local use case.
        if (existing_home != temp_regs_.end())
            temp_regs_.erase(existing_home);
        auto overlaps = [&](const auto &windows) {
            for (const auto &[start, end] : windows) {
                if (!(iv.last_use < start || iv.first_def > end))
                    return true;
            }
            return false;
        };
        if (overlaps(pair_windows) || overlaps(b_windows) ||
            overlaps(c_windows)) {
            continue;
        }
        temp_regs_[tid] = temp_home::main_bc;
        pair_windows.push_back({iv.first_def, iv.last_use});
    }

    // When an incoming pointer is copied into an IY-resident loop cursor and
    // the original value is needed again after the copy, retain that immutable
    // base in BC.  This is the common shape of pointer-distance loops such as
    // `cursor = base; while (...) ++cursor; return cursor - base`.  Treat the
    // two values independently: IY advances, while BC survives the loop.
    for (auto &[fd, tid] : order) {
        if (size_opt_enabled())
            continue;
        const interval &iv = ivs[tid];
        if (iv.size != 2 || iv.has_addr_of || iv.first_def < 0 ||
            iv.last_use <= iv.first_def + 1 ||
            temp_regs_.find(tid) != temp_regs_.end()) {
            continue;
        }
        const icode &def = fn.icodes[iv.first_def];
        if (def.op != icode_op::RECEIVE ||
            (def.arg_loc != abi_arg_loc::REG_HL &&
             def.arg_loc != abi_arg_loc::REG_DE) ||
            !def.result.type || !def.result.type->is_ptr() ||
            def.result.type->is_far_ptr()) {
            continue;
        }

        int copy_idx = -1;
        int cursor_tid = -1;
        for (int k = iv.first_def + 1; k <= iv.last_use; ++k) {
            const icode &use = fn.icodes[k];
            const bool use_left =
                use.left.is_temp() && use.left.temp_id == tid;
            const bool use_right =
                use.right.is_temp() && use.right.temp_id == tid;
            if (!use_left && !use_right)
                continue;
            if (use_left && !use_right && use.op == icode_op::ASSIGN &&
                use.result.is_temp()) {
                copy_idx = k;
                cursor_tid = use.result.temp_id;
            }
            break;
        }
        auto cursor_home = temp_regs_.find(cursor_tid);
        if (copy_idx < 0 || cursor_home == temp_regs_.end() ||
            cursor_home->second != temp_home::main_iy ||
            iv.last_use <= copy_idx ||
            !bc_temp_uses_are_backend_safe(tid, iv.first_def, iv.last_use)) {
            continue;
        }

        bool backend_safe = true;
        for (int k = iv.first_def + 1; k < iv.last_use; ++k) {
            const icode &inside = fn.icodes[k];
            if (inside.op == icode_op::LABEL ||
                inside.op == icode_op::GOTO ||
                inside.op == icode_op::IFX) {
                continue;
            }
            if (bc_backend_hazard(inside, direct_ix_frame) ||
                clobbers_bc(inside)) {
                backend_safe = false;
                break;
            }
        }
        if (!backend_safe)
            continue;
        auto overlaps_existing = [&](const auto &windows) {
            for (const auto &[start, end] : windows) {
                if (!(iv.last_use < start || iv.first_def > end))
                    return true;
            }
            return false;
        };
        if (overlaps_existing(pair_windows) ||
            overlaps_existing(b_windows) ||
            overlaps_existing(c_windows)) {
            continue;
        }
        temp_regs_[tid] = temp_home::main_bc;
        pair_windows.push_back({iv.first_def, iv.last_use});
    }

    for (auto &[fd, tid] : order) {
        const interval &iv = ivs[tid];
        int score = 0;
        if (!indirect_callee_bc_candidate(tid, iv, score))
            continue;
        bc_candidates.push_back(
            {iv.first_def, iv.last_use, score, false, tid});
    }

    // A word-sized predicate passed after other stack arguments can remain in
    // BC instead of being spilled to the frame.  Admit only a single-use
    // result and argument setup made entirely from constants, global
    // addresses, or stack SENDs whose operands have the same properties.
    // Those lowerings use HL but preserve BC, and the final SEND can push BC
    // directly.
    for (auto &[fd, tid] : order) {
        const interval &iv = ivs[tid];
        if (iv.size != 2 || iv.has_addr_of || iv.definitions != 1 ||
            iv.mentions != 1 || iv.first_def < 0 ||
            iv.last_use <= iv.first_def ||
            temp_regs_.find(tid) != temp_regs_.end()) {
            continue;
        }

        const icode &def = fn.icodes[iv.first_def];
        const icode &use = fn.icodes[iv.last_use];
        if (!is_compare_op(def.op) || !def.result.is_temp() ||
            def.result.temp_id != tid ||
            use.op != icode_op::SEND ||
            use.arg_loc != abi_arg_loc::STACK ||
            !use.left.is_temp() || use.left.temp_id != tid) {
            continue;
        }

        auto simple_stack_send = [&](const operand &op) {
            if (simple_send_operand_for_bc_live_range(op))
                return true;
            if (!op.is_temp())
                return false;
            auto source_iv = ivs.find(op.temp_id);
            if (source_iv == ivs.end() || source_iv->second.first_def < 0 ||
                source_iv->second.first_def >= iv.last_use) {
                return false;
            }
            const icode &source =
                fn.icodes[source_iv->second.first_def];
            return source.op == icode_op::ADDRESS_OF &&
                   source.result.is_temp() &&
                   source.result.temp_id == op.temp_id &&
                   !address_of_may_need_bc_scratch(source.left);
        };

        bool safe = true;
        for (int k = iv.first_def + 1; k < iv.last_use; ++k) {
            const icode &mid = fn.icodes[k];
            if (mentions_temp(mid, tid)) {
                safe = false;
                break;
            }
            if (mid.op == icode_op::ADDRESS_OF &&
                mid.result.is_temp() &&
                !address_of_may_need_bc_scratch(mid.left)) {
                continue;
            }
            if (mid.op == icode_op::SEND &&
                mid.arg_loc == abi_arg_loc::STACK &&
                simple_stack_send(mid.left)) {
                continue;
            }
            safe = false;
            break;
        }
        if (!safe)
            continue;

        bc_candidates.push_back(
            {iv.first_def, iv.last_use,
             900 - (iv.last_use - iv.first_def), false, tid});
    }

    // Keep one ordinary loop-carried word local in BC when every operation in
    // its lifetime has a BC-preserving lowering.  The generic symbol windows
    // below reject labels and backedges; that is unnecessarily conservative
    // for scalar loops such as binary searches, where the local is explicitly
    // updated on either branch and then reused at the header.  This remains a
    // normal live-range allocation: calls, TLS/deep-frame accesses and real
    // BC clobbers all reject the candidate.
    for (const auto &[key, iv] : syms) {
        if (!iv.base.type || iv.base.type->size() != 2 ||
            iv.base.is_param || iv.has_addr_of || iv.unsupported ||
            iv.mentions < 4 || iv.first_idx < 0 ||
            iv.last_idx <= iv.first_idx) {
            continue;
        }

        bool crosses_backedge = false;
        bool backend_safe = true;
        for (int k = iv.first_idx; k <= iv.last_idx; ++k) {
            const icode &inside = fn.icodes[k];
            if (inside.op == icode_op::GOTO) {
                auto target = label_indices.find(inside.label_name);
                if (target != label_indices.end() && target->second <= k)
                    crosses_backedge = true;
            } else if (inside.op == icode_op::IFX) {
                for (const std::string *label :
                     {&inside.true_lbl, &inside.false_lbl}) {
                    auto target = label_indices.find(*label);
                    if (target != label_indices.end() && target->second <= k)
                        crosses_backedge = true;
                }
            }
            if (inside.op == icode_op::LABEL ||
                inside.op == icode_op::GOTO ||
                inside.op == icode_op::IFX ||
                inside.op == icode_op::RETURN) {
                continue;
            }
            if (symbol_bc_backend_hazard(inside, iv.base) ||
                clobbers_bc(inside)) {
                backend_safe = false;
                break;
            }
        }
        if (!crosses_backedge || !backend_safe)
            continue;

        bc_candidates.push_back(
            {iv.first_idx, iv.last_idx,
             1700 + iv.mentions * 12 - (iv.last_idx - iv.first_idx),
             true, key});
    }

    // A loaded word used by comparisons and simple word ALU consumers can
    // remain in BC across branch labels when all executable instructions in
    // its live range preserve BC. This is common in ordering/conflict helpers
    // and avoids materializing the same scalar in an IX slot before both a
    // predicate and its follow-up difference calculation.
    for (auto &[fd, tid] : order) {
        const interval &iv = ivs[tid];
        if (iv.size != 2 || iv.has_addr_of || iv.definitions != 1 ||
            iv.first_def < 0 || iv.last_use <= iv.first_def ||
            iv.mentions < 2 || temp_regs_.find(tid) != temp_regs_.end()) {
            continue;
        }
        const icode &def = fn.icodes[iv.first_def];
        if (def.op != icode_op::GET_VALUE_AT || !def.result.is_temp() ||
            def.result.temp_id != tid) {
            continue;
        }

        bool safe = true;
        for (int k = iv.first_def + 1; k <= iv.last_use; ++k) {
            const icode &ic = fn.icodes[k];
            if (mentions_temp(ic, tid)) {
                const bool use_left =
                    ic.left.is_temp() && ic.left.temp_id == tid;
                const bool use_right =
                    ic.right.is_temp() && ic.right.temp_id == tid;
                const bool simple_alu =
                    (ic.op == icode_op::ADD || ic.op == icode_op::SUB ||
                     ic.op == icode_op::BAND || ic.op == icode_op::BOR ||
                     ic.op == icode_op::BXOR) &&
                    use_left != use_right && ic.result.is_temp() &&
                    ic.result.type && ic.result.type->size() == 2 &&
                    !hl_load_may_clobber_bc(use_left ? ic.right : ic.left, 0);
                if (!is_compare_op(ic.op) && !simple_alu) {
                    safe = false;
                    break;
                }
                continue;
            }
            if (ic.op == icode_op::LABEL || ic.op == icode_op::GOTO ||
                ic.op == icode_op::IFX || ic.op == icode_op::RETURN) {
                continue;
            }
            if (bc_backend_hazard(ic, direct_ix_frame)) {
                safe = false;
                break;
            }
        }
        if (!safe)
            continue;
        bc_candidates.push_back(
            {iv.first_def, iv.last_use,
             1300 + hot_mentions(iv) * 12 - (iv.last_use - iv.first_def),
             false, tid});
    }

    for (auto &[fd, tid] : order) {
        const interval &iv = ivs[tid];
        if (iv.size != 2)                         continue;
        if (iv.has_addr_of)                       continue;
        if (first_use_is_temp_copy(tid, iv))       continue;
        if (iv.last_use - iv.first_def > 6)       continue;
        if (!contiguous_live_window(iv, tid))     continue;
        if (!interior_safe(iv, bc_clob))          continue;
        bool backend_safe = true;
        for (int k = iv.first_def + 1; k <= iv.last_use - 1; ++k) {
            if (bc_backend_hazard(fn.icodes[k], direct_ix_frame)) {
                backend_safe = false;
                break;
            }
        }
        if (!backend_safe)                        continue;
        bc_candidates.push_back(
            {iv.first_def, iv.last_use,
             8 + (iv.last_use - iv.first_def + 1), false, tid});
    }

    // Broaden the stable BC path one notch beyond the fully contiguous case:
    // if a 16-bit temp stays inside one straight-line window with no BC
    // backend hazards, keep it resident even when a few interior instructions
    // do not mention it. This is still far from a global allocator, but it
    // captures more ordinary scalar state than the old “every instruction in
    // the window must mention the temp” rule.
    for (auto &[fd, tid] : order) {
        const interval &iv = ivs[tid];
        if (iv.size != 2)                          continue;
        if (iv.has_addr_of)                        continue;
        if (first_use_is_temp_copy(tid, iv))        continue;
        if (iv.mentions < 3)                       continue;
        if (iv.last_use <= iv.first_def)           continue;
        if (contiguous_live_window(iv, tid))       continue;
        if (!interior_safe(iv, bc_clob))           continue;
        bool backend_safe = true;
        for (int k = iv.first_def + 1; k <= iv.last_use - 1; ++k) {
            if (bc_backend_hazard(fn.icodes[k], direct_ix_frame)) {
                backend_safe = false;
                break;
            }
        }
        if (!backend_safe)                         continue;
        bc_candidates.push_back(
            {iv.first_def, iv.last_use,
             14 + hot_mentions(iv) * 3 - (iv.last_use - iv.first_def),
             false, tid});
    }

    // Keep one incoming register-passed 16-bit TEMP in BC across a wider
    // straight-line helper window. This is more permissive than the generic
    // contiguous TEMP rule above, but still restricted to barrier-free
    // windows with no BC-clobbering backend hazards.
    for (auto &[fd, tid] : order) {
        const interval &iv = ivs[tid];
        if (iv.size != 2)                          continue;
        if (iv.has_addr_of)                        continue;
        if (first_use_is_temp_copy(tid, iv))        continue;
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
            if (bc_backend_hazard(fn.icodes[k], direct_ix_frame)) {
                backend_safe = false;
                break;
            }
        }
        if (!backend_safe)                         continue;
        bc_candidates.push_back(
            {iv.first_def, iv.last_use,
             24 + hot_mentions(iv) * 3 - (iv.last_use - iv.first_def),
             false, tid});
    }

    const bool pair_bc_allocation_enabled = true;
    const bool loop_bound_bc_allocation_enabled = true;

    if (pair_bc_allocation_enabled && helper_like_fn) {
        for (auto &[fd, tid] : order) {
            const interval &iv = ivs[tid];
            if (iv.size != 2)                          continue;
            if (iv.has_addr_of)                        continue;
            if (first_use_is_temp_copy(tid, iv))        continue;
            if (iv.receive_loc != abi_arg_loc::REG_HL &&
                iv.receive_loc != abi_arg_loc::REG_DE)
                continue;
            if (iv.mentions < 2)                       continue;
            if (iv.last_use <= iv.first_def)           continue;
            if (!interior_safe(iv, bc_clob))           continue;
            bool backend_safe = true;
            for (int k = iv.first_def + 1; k <= iv.last_use - 1; ++k) {
                if (bc_backend_hazard(fn.icodes[k], direct_ix_frame)) {
                    backend_safe = false;
                    break;
                }
            }
            if (!backend_safe)                         continue;
            bc_candidates.push_back(
                {iv.first_def, iv.last_use,
                 1100 + hot_mentions(iv) * 8 - (iv.last_use - iv.first_def),
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
            if (symbol_bc_backend_hazard(fn.icodes[k], iv.base)) {
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

    // Same widening for ordinary 16-bit locals/parameters whose mentions are
    // not fully contiguous but still stay inside one barrier-free BC-safe
    // window.
    for (const auto &[key, iv] : syms) {
        if (!iv.base.type || iv.base.type->size() != 2) continue;
        if (iv.has_addr_of || iv.unsupported)            continue;
        if (iv.mentions < 4)                             continue;
        if (iv.last_idx <= iv.first_idx)                 continue;
        if (iv.base.is_param && iv.receive_loc == abi_arg_loc::STACK)
            continue;
        if (contiguous_symbol_window(iv))                continue;
        if (!interior_safe(interval{iv.first_idx, iv.last_idx, 2, false}, bc_clob))
            continue;
        bool backend_safe = true;
        for (int k = iv.first_idx + 1; k <= iv.last_idx - 1; ++k) {
            if (symbol_bc_backend_hazard(fn.icodes[k], iv.base)) {
                backend_safe = false;
                break;
            }
        }
        if (!backend_safe)                               continue;
        bc_candidates.push_back(
            {iv.first_idx, iv.last_idx,
             18 + iv.mentions * 3 - (iv.last_idx - iv.first_idx),
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
            if (symbol_bc_backend_hazard(fn.icodes[k], iv.base)) {
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

    if (pair_bc_allocation_enabled && helper_like_fn) {
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
                if (symbol_bc_backend_hazard(fn.icodes[k], iv.base)) {
                    backend_safe = false;
                    break;
                }
            }
            if (!backend_safe)                               continue;
            symbol_regs_[key] = temp_home::main_bc;
            pair_windows.push_back({iv.first_idx, iv.last_idx});
            break;
        }

        if (pair_windows.empty()) {
            for (auto &[fd, tid] : order) {
                const interval &iv = ivs[tid];
                if (iv.size != 2)                          continue;
                if (iv.has_addr_of)                        continue;
                if (first_use_is_temp_copy(tid, iv))        continue;
                if (iv.receive_loc != abi_arg_loc::REG_HL &&
                    iv.receive_loc != abi_arg_loc::REG_DE)
                    continue;
                if (iv.mentions < 2)                       continue;
                if (iv.last_use <= iv.first_def)           continue;
                if (!interior_safe(iv, bc_clob))           continue;
                bool backend_safe = true;
                for (int k = iv.first_def + 1; k <= iv.last_use - 1; ++k) {
                    if (bc_backend_hazard(fn.icodes[k], direct_ix_frame)) {
                        backend_safe = false;
                        break;
                    }
                }
                if (!backend_safe)                         continue;
                temp_regs_[tid] = temp_home::main_bc;
                pair_windows.push_back({iv.first_def, iv.last_use});
                break;
            }
        }
    }

    const bool u8_index_remat_enabled = true;
    if (u8_index_remat_enabled &&
        (size_opt_enabled() || tuned_profile_enabled())) {
        for (auto &[fd, tid] : order) {
            const interval &iv = ivs[tid];
            int score = 0;
            if (!loop_pointer_hl_candidate(tid, iv, score))
                continue;
            hl_candidates.push_back(
                {iv.first_def, iv.last_use, score, false, tid});
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
    std::sort(hl_candidates.begin(), hl_candidates.end(),
              [](const bc_candidate &a, const bc_candidate &b) {
                  if (a.score != b.score) return a.score > b.score;
                  int aspan = a.end - a.start;
                  int bspan = b.end - b.start;
                  if (aspan != bspan) return aspan < bspan;
                  if (a.start != b.start) return a.start < b.start;
                  return a.is_symbol && !b.is_symbol;
              });

    auto overlaps_windows =
        [&](const std::vector<std::pair<int, int>> &windows,
            int start, int end) {
        for (const auto &[s, e] : windows) {
            if (!(end < s || start > e))
                return true;
        }
        return false;
    };

    // A byte-pointer cursor that survives a real loop backedge is more valuable
    // in BC than a rematerializable loop bound: it enables (bc), INC BC and
    // keeps address state out of the frame for every iteration.
    std::vector<bc_candidate> loop_pointer_bc_candidates;
    for (auto &[fd, tid] : order) {
        const interval &iv = ivs[tid];
        int end = -1;
        int score = 0;
        if (!loop_pointer_bc_candidate(tid, iv, end, score))
            continue;
        loop_pointer_bc_candidates.push_back(
            {iv.first_def, end, score, false, tid});
    }
    std::sort(loop_pointer_bc_candidates.begin(),
              loop_pointer_bc_candidates.end(),
              [](const bc_candidate &a, const bc_candidate &b) {
                  if (a.score != b.score) return a.score > b.score;
                  if (a.start != b.start) return a.start < b.start;
                  return a.id < b.id;
              });
    for (const auto &cand : loop_pointer_bc_candidates) {
        if (temp_regs_.find(cand.id) != temp_regs_.end())
            continue;
        if (overlaps_windows(pair_windows, cand.start, cand.end) ||
            overlaps_windows(b_windows, cand.start, cand.end) ||
            overlaps_windows(c_windows, cand.start, cand.end)) {
            continue;
        }
        temp_regs_[cand.id] = temp_home::main_bc;
        pair_windows.push_back({cand.start, cand.end});
    }

    if (pair_bc_allocation_enabled && helper_like_fn) {
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
                if (symbol_bc_backend_hazard(fn.icodes[k], iv.base)) {
                    backend_safe = false;
                    break;
                }
            }
            if (!backend_safe)                               continue;
            symbol_regs_[key] = temp_home::main_bc;
                pair_windows.push_back({iv.first_idx, iv.last_idx});
            break;
        }
    }

    // Loop-invariant word temps used in a loop header compare are SDCC's
    // classic "keep the bound in BC" win.  The normal linear interval only
    // sees the textual header use once, so accept a backedge-extended live
    // range when the whole loop body preserves BC.
    if (loop_bound_bc_allocation_enabled &&
        (size_opt_enabled() || tuned_profile_enabled())) {
        for (auto &[fd, tid] : order) {
            const interval &iv = ivs[tid];
            if (!iv.loop_extended)                  continue;
            if (iv.size != 2 || iv.has_addr_of)     continue;
            if (iv.first_def < 0 || iv.last_use <= iv.first_def)
                continue;
            if (temp_regs_.find(tid) != temp_regs_.end())
                continue;

            bool only_compare_uses = false;
            bool safe = true;
            for (int k = iv.first_def + 1; k <= iv.last_use; ++k) {
                const icode &use_ic = fn.icodes[k];
                const bool use_left =
                    use_ic.left.is_temp() && use_ic.left.temp_id == tid;
                const bool use_right =
                    use_ic.right.is_temp() && use_ic.right.temp_id == tid;
                const bool use_result =
                    use_ic.result.is_temp() && use_ic.result.temp_id == tid;
                if (!use_left && !use_right && !use_result)
                    continue;
                if (!is_compare_op(use_ic.op) || use_result) {
                    safe = false;
                    break;
                }
                only_compare_uses = true;
            }
            if (!safe || !only_compare_uses)
                continue;

            for (int k = iv.first_def + 1; k <= iv.last_use; ++k) {
                if (mentions_temp(fn.icodes[k], tid))
                    continue;
                if (loop_bc_scratch_hazard(fn.icodes[k])) {
                    safe = false;
                    break;
                }
            }
            if (!safe)
                continue;

            if (overlaps_windows(pair_windows, iv.first_def, iv.last_use) ||
                overlaps_windows(b_windows, iv.first_def, iv.last_use) ||
                overlaps_windows(c_windows, iv.first_def, iv.last_use)) {
                continue;
            }
            temp_regs_[tid] = temp_home::main_bc;
            pair_windows.push_back({iv.first_def, iv.last_use});
            break;
        }
    }

    // No-call byte pointer loops are safe to keep in HL: the recognizer rejects
    // calls, address-taking, deep frame accesses, and non-byte memory traffic.
    const bool loop_pointer_hl_enabled = true;
    if (loop_pointer_hl_enabled &&
        (size_opt_enabled() || tuned_profile_enabled())) {
        for (const auto &cand : hl_candidates) {
            if (temp_regs_.find(cand.id) != temp_regs_.end())
                continue;
            if (overlaps_windows(hl_windows, cand.start, cand.end))
                continue;
            temp_regs_[cand.id] = temp_home::main_hl;
            hl_windows.push_back({cand.start, cand.end});
        }
    }

    // Conservative byte B/C pinning: keep short byte temps/symbols in B or C
    // when the whole live window is free of calls, address-taking, memory
    // indirection, and overlapping pair-register allocations.
    //
    // Keep this disabled for the preset allocator path for now. The backend
    // still uses B/C as scratch in byte compare and address formation sequences,
    // so a long-lived byte home in C can be silently clobbered inside ordinary
    // counted loops.
    const bool byte_bc_pinning_enabled = true;
    if (byte_bc_pinning_enabled &&
        (size_opt_enabled() || tuned_profile_enabled())) {
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
                if (byte_bc_scratch_hazard(fn.icodes[k])) {
                    backend_safe = false;
                    break;
                }
            }
            if (!backend_safe)                        continue;

            int compare_uses = 0;
            int mask_uses = 0;
            for (int k = iv.first_def + 1; k <= iv.last_use; ++k) {
                const icode &use_ic = fn.icodes[k];
                bool uses_here =
                    (use_ic.left.is_temp() && use_ic.left.temp_id == tid) ||
                    (use_ic.right.is_temp() && use_ic.right.temp_id == tid);
                if (!uses_here)
                    continue;
                if (!main_byte_window_use_safe(use_ic, tid)) {
                    backend_safe = false;
                    break;
                }
                if (is_compare_op(use_ic.op))
                    ++compare_uses;
                if (use_ic.op == icode_op::BAND)
                    ++mask_uses;
            }
            if (!backend_safe)
                continue;

            if (compare_uses == 0) {
                c_candidates.push_back(
                    {tid, iv.first_def, iv.last_use,
                     220 + hot_mentions(iv) * 10 + mask_uses * 8 -
                         (iv.last_use - iv.first_def)});
            }

            b_candidates.push_back(
                {tid, iv.first_def, iv.last_use,
                 180 + hot_mentions(iv) * 8 + compare_uses * 10 +
                     mask_uses * 6 -
                     (iv.last_use - iv.first_def)});
        }

        for (auto &[fd, tid] : order) {
            const interval &iv = ivs[tid];
            if (iv.size != 1)                         continue;
            if (iv.has_addr_of)                       continue;
            if (iv.first_def < 0 || iv.last_use <= iv.first_def)
                continue;
            if (iv.mentions < 5)                      continue;
            if (temp_regs_.find(tid) != temp_regs_.end())
                continue;

            bool backend_safe = true;
            for (int k = iv.first_def + 1; k <= iv.last_use - 1; ++k) {
                if (byte_bc_scratch_hazard(fn.icodes[k])) {
                    backend_safe = false;
                    break;
                }
            }
            if (!backend_safe)
                continue;

            int compare_uses = 0;
            int mask_uses = 0;
            for (int k = iv.first_def; k <= iv.last_use; ++k) {
                const icode &use_ic = fn.icodes[k];
                if (!mentions_temp(use_ic, tid))
                    continue;
                if (use_ic.op == icode_op::CALL ||
                    use_ic.op == icode_op::ALLOCA ||
                    use_ic.op == icode_op::INLINE_ASM ||
                    use_ic.op == icode_op::ADDRESS_OF ||
                    use_ic.op == icode_op::GET_VALUE_AT ||
                    use_ic.op == icode_op::SET_VALUE_AT) {
                    backend_safe = false;
                    break;
                }
                if (!loop_byte_temp_window_safe(use_ic, tid)) {
                    backend_safe = false;
                    break;
                }
                if (is_compare_op(use_ic.op))
                    ++compare_uses;
                if (use_ic.op == icode_op::BAND)
                    ++mask_uses;
            }
            if (!backend_safe)
                continue;

            if (compare_uses == 0) {
                c_candidates.push_back(
                    {tid, iv.first_def, iv.last_use,
                     170 + hot_mentions(iv) * 10 + mask_uses * 6 -
                         (iv.last_use - iv.first_def)});
            }
            b_candidates.push_back(
                {tid, iv.first_def, iv.last_use,
                 150 + hot_mentions(iv) * 8 + compare_uses * 8 +
                     mask_uses * 6 - (iv.last_use - iv.first_def)});
        }

        for (auto &[key, iv] : syms) {
            if (!iv.base.type || iv.base.type->size() != 1)
                continue;
            if (iv.base.is_param)
                continue;
            if (iv.has_addr_of || iv.unsupported)
                continue;
            if (iv.first_idx < 0 || iv.last_idx <= iv.first_idx)
                continue;
            if (iv.mentions < 4)
                continue;
            if (!byte_symbol_first_occurrence_is_definition(iv))
                continue;
            if (symbol_feeds_branchy_bit_test(iv))
                continue;

            bool backend_safe = true;
            for (int k = iv.first_idx; k <= iv.last_idx; ++k) {
                if (byte_symbol_reg_hazard(fn.icodes[k], iv.base)) {
                    backend_safe = false;
                    break;
                }
            }
            if (!backend_safe)
                continue;

            int compare_uses = 0;
            int mask_uses = 0;
            for (int k = iv.first_idx; k <= iv.last_idx; ++k) {
                const icode &use_ic = fn.icodes[k];
                if (!mentions_symbol(use_ic, iv.base))
                    continue;
                if (use_ic.op == icode_op::CALL ||
                    use_ic.op == icode_op::ALLOCA ||
                    use_ic.op == icode_op::INLINE_ASM ||
                    use_ic.op == icode_op::ADDRESS_OF ||
                    use_ic.op == icode_op::GET_VALUE_AT ||
                    use_ic.op == icode_op::SET_VALUE_AT) {
                    backend_safe = false;
                    break;
                }
                if (!main_byte_window_use_safe_symbol(use_ic, iv.base)) {
                    backend_safe = false;
                    break;
                }
                if (is_compare_op(use_ic.op))
                    ++compare_uses;
                if (use_ic.op == icode_op::BAND)
                    ++mask_uses;
            }
            if (!backend_safe)
                continue;

            if (compare_uses == 0) {
                sym_c_candidates.push_back(
                    {key, iv.first_idx, iv.last_idx,
                     200 + iv.mentions * 10 + mask_uses * 8 -
                         (iv.last_idx - iv.first_idx)});
            }
            sym_b_candidates.push_back(
                {key, iv.first_idx, iv.last_idx,
                 160 + iv.mentions * 8 + compare_uses * 10 +
                     mask_uses * 6 - (iv.last_idx - iv.first_idx)});
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
        std::sort(sym_c_candidates.begin(), sym_c_candidates.end(),
                  [](const sym_byte_candidate &a,
                     const sym_byte_candidate &b) {
                      if (a.score != b.score) return a.score > b.score;
                      int aspan = a.end - a.start;
                      int bspan = b.end - b.start;
                      if (aspan != bspan) return aspan < bspan;
                      if (a.start != b.start) return a.start < b.start;
                      return a.key < b.key;
                  });
        std::sort(b_candidates.begin(), b_candidates.end(),
                  [](const c_candidate &a, const c_candidate &b) {
                      if (a.score != b.score) return a.score > b.score;
                      int aspan = a.end - a.start;
                      int bspan = b.end - b.start;
                      if (aspan != bspan) return aspan < bspan;
                      if (a.start != b.start) return a.start < b.start;
                      return a.tid < b.tid;
                  });
        std::sort(sym_b_candidates.begin(), sym_b_candidates.end(),
                  [](const sym_byte_candidate &a,
                     const sym_byte_candidate &b) {
                      if (a.score != b.score) return a.score > b.score;
                      int aspan = a.end - a.start;
                      int bspan = b.end - b.start;
                      if (aspan != bspan) return aspan < bspan;
                      if (a.start != b.start) return a.start < b.start;
                      return a.key < b.key;
                  });

        for (const auto &cand : c_candidates) {
            if (overlaps_windows(pair_windows, cand.start, cand.end) ||
                overlaps_windows(c_windows, cand.start, cand.end))
                continue;
            temp_regs_[cand.tid] = temp_home::main_c;
            c_windows.push_back({cand.start, cand.end});
        }
        for (const auto &cand : sym_c_candidates) {
            if (symbol_regs_.find(cand.key) != symbol_regs_.end())
                continue;
            if (overlaps_windows(pair_windows, cand.start, cand.end) ||
                overlaps_windows(c_windows, cand.start, cand.end))
                continue;
            symbol_regs_[cand.key] = temp_home::main_c;
            c_windows.push_back({cand.start, cand.end});
        }
        for (const auto &cand : b_candidates) {
            if (overlaps_windows(pair_windows, cand.start, cand.end) ||
                overlaps_windows(b_windows, cand.start, cand.end))
                continue;
            temp_regs_[cand.tid] = temp_home::main_b;
            b_windows.push_back({cand.start, cand.end});
        }
        for (const auto &cand : sym_b_candidates) {
            if (symbol_regs_.find(cand.key) != symbol_regs_.end())
                continue;
            if (overlaps_windows(pair_windows, cand.start, cand.end) ||
                overlaps_windows(b_windows, cand.start, cand.end))
                continue;
            symbol_regs_[cand.key] = temp_home::main_b;
            b_windows.push_back({cand.start, cand.end});
        }
    }

    if (pair_bc_allocation_enabled) {
        for (const auto &cand : bc_candidates) {
            if (!cand.is_symbol &&
                temp_regs_.find(cand.id) != temp_regs_.end())
                continue;
            if (!cand.is_symbol &&
                !bc_temp_uses_are_backend_safe(cand.id, cand.start, cand.end))
                continue;
            if (overlaps_windows(pair_windows, cand.start, cand.end) ||
                overlaps_windows(b_windows, cand.start, cand.end) ||
                overlaps_windows(c_windows, cand.start, cand.end))
                continue;
            if (cand.is_symbol)
                symbol_regs_[cand.id] = temp_home::main_bc;
            else
                temp_regs_[cand.id] = temp_home::main_bc;
            pair_windows.push_back({cand.start, cand.end});
        }
    }

    // A word freshly loaded from memory commonly feeds an equality test and,
    // on the unequal edge, an ordered comparison (binary search and tree/hash
    // probes are typical examples).  The load generator already produces the
    // value in DE, so retain it there when the intervening control-flow-only
    // instructions provably cannot need DE.  Earlier equality comparisons use
    // DE as the right operand without modifying it; the final use may consume
    // the pair normally.
    std::vector<std::pair<int, int>> de_windows;
    // DE homes selected by the paired table-probe pass above participate in
    // the same interference space as the later independent DE candidates.
    // Seed the window set so a second pass cannot silently overlap them.
    for (const auto &[tid, home] : temp_regs_) {
        if (home != temp_home::main_de)
            continue;
        auto it = ivs.find(tid);
        if (it != ivs.end() && it->second.first_def >= 0)
            de_windows.push_back(
                {it->second.first_def, it->second.last_use});
    }
    if (size_opt_enabled() || tuned_profile_enabled()) {
        for (auto &[fd, tid] : order) {
            const interval &iv = ivs[tid];
            if (iv.size != 2 || iv.has_addr_of || iv.first_def < 0 ||
                iv.last_use <= iv.first_def || iv.mentions < 2 ||
                temp_regs_.find(tid) != temp_regs_.end()) {
                continue;
            }
            const icode &def_ic = fn.icodes[iv.first_def];
            if (def_ic.op != icode_op::GET_VALUE_AT ||
                !def_ic.result.is_temp() || def_ic.result.temp_id != tid ||
                !def_ic.right.is_none()) {
                continue;
            }

            int compare_uses = 0;
            bool safe = true;
            for (int k = iv.first_def + 1; k <= iv.last_use; ++k) {
                const icode &ic = fn.icodes[k];
                const bool use_left =
                    ic.left.is_temp() && ic.left.temp_id == tid;
                const bool use_right =
                    ic.right.is_temp() && ic.right.temp_id == tid;
                const bool use_result =
                    ic.result.is_temp() && ic.result.temp_id == tid;
                if (use_result || (use_left && use_right)) {
                    safe = false;
                    break;
                }
                if (use_left || use_right) {
                    const bool final_simple_alu =
                        k == iv.last_use && use_left != use_right &&
                        (ic.op == icode_op::ADD || ic.op == icode_op::SUB ||
                         ic.op == icode_op::BAND || ic.op == icode_op::BOR ||
                         ic.op == icode_op::BXOR) &&
                        ic.result.is_temp() && ic.result.type &&
                        ic.result.type->size() == 2;
                    if (!is_compare_op(ic.op) && !final_simple_alu) {
                        safe = false;
                        break;
                    }
                    if (is_compare_op(ic.op))
                        ++compare_uses;
                    if (!final_simple_alu && k != iv.last_use &&
                        (ic.op != icode_op::EQ && ic.op != icode_op::NE)) {
                        safe = false;
                        break;
                    }
                    continue;
                }
                switch (ic.op) {
                case icode_op::LABEL:
                case icode_op::IFX:
                case icode_op::RETURN:
                    break;
                default:
                    safe = false;
                    break;
                }
                if (!safe)
                    break;
            }
            if (!safe || compare_uses < 1 ||
                overlaps_windows(de_windows, iv.first_def, iv.last_use)) {
                continue;
            }
            temp_regs_[tid] = temp_home::main_de;
            de_windows.push_back({iv.first_def, iv.last_use});
        }
    }

    // Coalesce a consumed DE input with a signed word result that is
    // conditionally negated before its final comparison.  Absolute-value and
    // distance calculations commonly have this SSA-like shape:
    //
    //     diff = lhs - rhs;
    //     if (diff < 0) diff = -diff;
    //     ... compare diff ...
    //
    // The input and result intervals touch at the defining instruction but do
    // not overlap dynamically.  Keeping the result in DE avoids stores on
    // both control-flow arms.  Limit the intervening arithmetic to direct
    // operations against a BC-resident value, whose lowering uses SBC HL,BC
    // and therefore preserves DE.
    if (size_opt_enabled() || tuned_profile_enabled()) {
        auto operand_home_is = [&](const operand &op, temp_home home) {
            if (!op.is_temp())
                return false;
            auto it = temp_regs_.find(op.temp_id);
            return it != temp_regs_.end() && it->second == home;
        };
        for (auto &[fd, tid] : order) {
            const interval &iv = ivs[tid];
            if (iv.size != 2 || iv.has_addr_of || iv.first_def < 0 ||
                iv.last_use <= iv.first_def || iv.definitions < 2 ||
                temp_regs_.find(tid) != temp_regs_.end()) {
                continue;
            }
            const icode &def = fn.icodes[iv.first_def];
            if ((def.op != icode_op::ADD && def.op != icode_op::SUB) ||
                !def.result.is_temp() || def.result.temp_id != tid ||
                !(operand_home_is(def.left, temp_home::main_de) ||
                  operand_home_is(def.right, temp_home::main_de))) {
                continue;
            }

            bool saw_sign_test = false;
            bool saw_self_negate = false;
            bool saw_final_compare = false;
            bool safe = true;
            for (int k = iv.first_def + 1; k <= iv.last_use; ++k) {
                const icode &ic = fn.icodes[k];
                const bool use_left =
                    ic.left.is_temp() && ic.left.temp_id == tid;
                const bool use_right =
                    ic.right.is_temp() && ic.right.temp_id == tid;
                const bool use_result =
                    ic.result.is_temp() && ic.result.temp_id == tid;
                if (use_left || use_right || use_result) {
                    const bool sign_test =
                        !use_result && use_left && !use_right &&
                        (ic.op == icode_op::LT || ic.op == icode_op::GE) &&
                        ic.right.kind == operand_kind::INT_CONST &&
                        ic.right.ival == 0;
                    const bool self_negate =
                        ic.op == icode_op::NEG && use_left && !use_right &&
                        use_result;
                    const bool final_compare =
                        k == iv.last_use && !use_result &&
                        use_left != use_right && is_compare_op(ic.op);
                    if (sign_test)
                        saw_sign_test = true;
                    else if (self_negate)
                        saw_self_negate = true;
                    else if (final_compare)
                        saw_final_compare = true;
                    else {
                        safe = false;
                        break;
                    }
                    continue;
                }

                if (ic.op == icode_op::LABEL || ic.op == icode_op::GOTO ||
                    ic.op == icode_op::IFX) {
                    continue;
                }
                const bool bc_rhs_arithmetic =
                    (ic.op == icode_op::ADD || ic.op == icode_op::SUB) &&
                    ic.result.is_temp() && ic.result.type &&
                    ic.result.type->size() == 2 &&
                    temp_regs_.find(ic.result.temp_id) == temp_regs_.end() &&
                    operand_home_is(ic.right, temp_home::main_bc);
                if (!bc_rhs_arithmetic) {
                    safe = false;
                    break;
                }
            }
            if (!safe || !saw_sign_test || !saw_self_negate ||
                !saw_final_compare) {
                continue;
            }

            bool overlaps = false;
            for (const auto &[start, end] : de_windows) {
                if (iv.first_def == end)
                    continue;
                if (!(iv.last_use < start || iv.first_def > end)) {
                    overlaps = true;
                    break;
                }
            }
            if (overlaps)
                continue;
            temp_regs_[tid] = temp_home::main_de;
            de_windows.push_back({iv.first_def, iv.last_use});
        }
    }

    // Direct global-array indexing normally borrows DE while forming the
    // address in HL.  The backend has a guarded path that saves/restores DE
    // when either byte half carries a live value, so byte homes may cross
    // these address calculations and their compact indirect stores.
    auto de_preserving_indexed_memory_op = [&](const icode &ic) {
        if (!direct_ix_frame)
            return false;

        auto direct_global_array = [](const operand &op) {
            return op.kind == operand_kind::LABEL_REF ||
                   (op.kind == operand_kind::SYMBOL && op.is_global &&
                    !op.is_tls && !op.is_func && !op.is_param && op.type &&
                    op.type->unqual() &&
                    op.type->unqual()->kind == type_kind::ARRAY);
        };
        auto compact_frame_temp = [&](const operand &op) {
            if (!op.is_temp())
                return false;
            auto home = temp_regs_.find(op.temp_id);
            if (home != temp_regs_.end() &&
                home->second != temp_home::stack)
                return false;
            const int off = ix_offset_of(op);
            return fits_ix_disp(off) && fits_ix_disp(off + 1);
        };
        auto compact_address_result = [&](const operand &op) {
            if (!op.is_temp())
                return false;
            auto home = temp_regs_.find(op.temp_id);
            if (home != temp_regs_.end() &&
                home->second != temp_home::stack &&
                home->second != temp_home::main_hl)
                return false;
            const int off = ix_offset_of(op);
            return home != temp_regs_.end() ||
                   (fits_ix_disp(off) && fits_ix_disp(off + 1));
        };

        if (ic.op == icode_op::ADD && op_size(ic.result) == 2 &&
            ic.result.type && ic.result.type->is_ptr() &&
            compact_address_result(ic.result)) {
            const operand *index = nullptr;
            if (direct_global_array(ic.left))
                index = &ic.right;
            else if (direct_global_array(ic.right))
                index = &ic.left;
            return index && op_size(*index) == 2 &&
                   compact_frame_temp(*index);
        }

        return ic.op == icode_op::SET_VALUE_AT &&
               ic.right.is_none() && op_size(ic.left) == 1 &&
               compact_address_result(ic.result);
    };
    auto direct_word_operand_preserves_de = [&](const operand &op) {
        if (!direct_ix_frame || op_size(op) != 2)
            return false;
        if (op.is_temp()) {
            auto home = temp_regs_.find(op.temp_id);
            if (home != temp_regs_.end() &&
                home->second != temp_home::stack &&
                home->second != temp_home::main_hl &&
                home->second != temp_home::main_bc &&
                home->second != temp_home::main_iy)
                return false;
            if (home != temp_regs_.end())
                return true;
            const int off = ix_offset_of(op);
            return fits_ix_disp(off) && fits_ix_disp(off + 1);
        }
        if (op.kind != operand_kind::SYMBOL || op.is_global ||
            op.is_tls || op.is_func || op.is_sfr)
            return false;
        auto home = symbol_regs_.find(symbol_reg_key(op));
        if (home != symbol_regs_.end() &&
            home->second != temp_home::main_bc &&
            home->second != temp_home::main_iy)
            return false;
        if (home != symbol_regs_.end())
            return true;
        const int off = ix_offset_of(op);
        return fits_ix_disp(off) && fits_ix_disp(off + 1);
    };
    auto direct_small_word_step_preserves_de = [&](const icode &ic) {
        return ic.op == icode_op::ADD && op_size(ic.result) == 2 &&
               ic.right.kind == operand_kind::INT_CONST &&
               ic.right.ival >= -3 && ic.right.ival <= 3 &&
               direct_word_operand_preserves_de(ic.result) &&
               direct_word_operand_preserves_de(ic.left);
    };

    // When BC and IY already carry a loop counter and cursor, keep one
    // independent word recurrence in DE.  This is the common remaining
    // register-pressure pattern in checksums, hashes, parsers and byte-stream
    // transforms.  Admit only a constant-initialized, no-call live range and
    // instruction forms whose current lowering is known to preserve DE.
    // Unlike a source-pattern rewrite, this is an ordinary live-range home:
    // it is selected from type, def/use, CFG and physical interference only.
    struct word_de_candidate {
        int tid = -1;
        int start = -1;
        int end = -1;
        int score = 0;
    };
    std::vector<word_de_candidate> word_de_candidates;
    if (size_opt_enabled() || tuned_profile_enabled()) {
        auto home_is = [&](const operand &op, temp_home home) {
            if (!op.is_temp())
                return false;
            auto it = temp_regs_.find(op.temp_id);
            return it != temp_regs_.end() && it->second == home;
        };
        auto compact_word_source_preserves_de = [&](const operand &op) {
            if (op.kind == operand_kind::INT_CONST ||
                op.kind == operand_kind::LABEL_REF)
                return true;
            if (op.kind == operand_kind::SYMBOL)
                return !op.is_tls && !op.is_sfr;
            if (!op.is_temp())
                return false;
            auto home = temp_regs_.find(op.temp_id);
            if (home == temp_regs_.end() || home->second == temp_home::stack ||
                home->second == temp_home::arg_hl ||
                home->second == temp_home::main_hl ||
                home->second == temp_home::main_bc ||
                home->second == temp_home::main_iy ||
                home->second == temp_home::remat_hl)
                return true;
            return false;
        };
        auto word_alu = [](icode_op op) {
            return op == icode_op::ADD || op == icode_op::SUB ||
                   op == icode_op::BAND || op == icode_op::BOR ||
                   op == icode_op::BXOR;
        };

        for (auto &[fd, tid] : order) {
            const interval &iv = ivs[tid];
            if (iv.size != 2 || iv.has_addr_of || iv.definitions < 2 ||
                iv.first_def < 0 || iv.last_use <= iv.first_def ||
                iv.mentions < 3 || temp_regs_.find(tid) != temp_regs_.end())
                continue;
            const icode &initial = fn.icodes[iv.first_def];
            if (initial.op != icode_op::ASSIGN ||
                !initial.result.is_temp() || initial.result.temp_id != tid ||
                initial.left.kind != operand_kind::INT_CONST)
                continue;

            bool crosses_backedge = false;
            bool saw_update = false;
            bool safe = true;
            for (int k = iv.first_def; k <= iv.last_use; ++k) {
                const icode &ic = fn.icodes[k];
                const bool use_left =
                    ic.left.is_temp() && ic.left.temp_id == tid;
                const bool use_right =
                    ic.right.is_temp() && ic.right.temp_id == tid;
                const bool use_result =
                    ic.result.is_temp() && ic.result.temp_id == tid;

                if (ic.op == icode_op::GOTO) {
                    auto target = label_indices.find(ic.label_name);
                    if (target != label_indices.end() && target->second <= k)
                        crosses_backedge = true;
                } else if (ic.op == icode_op::IFX) {
                    for (const std::string *label :
                         {&ic.true_lbl, &ic.false_lbl}) {
                        auto target = label_indices.find(*label);
                        if (target != label_indices.end() &&
                            target->second <= k)
                            crosses_backedge = true;
                    }
                }

                if (use_left || use_right || use_result) {
                    if (use_result) {
                        if (k == iv.first_def)
                            continue;
                        const bool update = word_alu(ic.op) &&
                            ic.result.type && ic.result.type->size() == 2;
                        if (!update) {
                            safe = false;
                            break;
                        }
                        saw_update = true;
                        continue;
                    }
                    if (ic.op == icode_op::RETURN && use_left && !use_right)
                        continue;
                    const bool constant_unary = use_left && !use_right &&
                        (ic.op == icode_op::SHL ||
                         ic.op == icode_op::SHR ||
                         ic.op == icode_op::ROL ||
                         ic.op == icode_op::ROR) &&
                        ic.right.kind == operand_kind::INT_CONST;
                    const bool safe_binary = word_alu(ic.op) &&
                        use_left != use_right &&
                        (use_right ||
                         (ic.right.kind == operand_kind::INT_CONST));
                    if (constant_unary || safe_binary)
                        continue;
                    safe = false;
                    break;
                }

                if (ic.op == icode_op::LABEL || ic.op == icode_op::GOTO ||
                    ic.op == icode_op::IFX)
                    continue;
                if (ic.op == icode_op::CALL || ic.op == icode_op::ALLOCA ||
                    ic.op == icode_op::INLINE_ASM ||
                    ic.op == icode_op::BLOCK_FILL ||
                    ic.op == icode_op::RETURN) {
                    safe = false;
                    break;
                }
                if (is_compare_op(ic.op)) {
                    const operand *value = nullptr;
                    if (ic.left.kind == operand_kind::INT_CONST)
                        value = &ic.right;
                    else if (ic.right.kind == operand_kind::INT_CONST)
                        value = &ic.left;
                    if (value &&
                        (op_size(*value) == 1 ||
                         home_is(*value, temp_home::main_bc)))
                        continue;
                    safe = false;
                    break;
                }
                if (ic.op == icode_op::GET_VALUE_AT &&
                    ic.result.type && ic.result.type->size() == 1 &&
                    (home_is(ic.left, temp_home::main_iy) ||
                     home_is(ic.left, temp_home::main_bc)))
                    continue;
                if (ic.op == icode_op::ASSIGN &&
                    (home_is(ic.result, temp_home::main_iy) ||
                     home_is(ic.result, temp_home::main_bc)) &&
                    compact_word_source_preserves_de(ic.left))
                    continue;
                if (ic.op == icode_op::ADDRESS_OF &&
                    (home_is(ic.result, temp_home::main_iy) ||
                     home_is(ic.result, temp_home::main_bc)))
                    continue;
                if (direct_small_word_step_preserves_de(ic))
                    continue;
                safe = false;
                break;
            }
            if (!safe || !crosses_backedge || !saw_update)
                continue;
            word_de_candidates.push_back(
                {tid, iv.first_def, iv.last_use,
                 1400 + hot_mentions(iv) * 16 -
                     (iv.last_use - iv.first_def)});
        }
    }
    std::sort(word_de_candidates.begin(), word_de_candidates.end(),
              [](const word_de_candidate &a,
                 const word_de_candidate &b) {
                  if (a.score != b.score) return a.score > b.score;
                  if (a.start != b.start) return a.start < b.start;
                  return a.tid < b.tid;
              });
    for (const auto &cand : word_de_candidates) {
        if (overlaps_windows(de_windows, cand.start, cand.end))
            continue;
        temp_regs_[cand.tid] = temp_home::main_de;
        de_windows.push_back({cand.start, cand.end});
    }

    struct d_candidate {
        int tid = -1;
        int start = -1;
        int end = -1;
        int score = 0;
    };
    std::vector<d_candidate> d_candidates;
    if (size_opt_enabled() || tuned_profile_enabled()) {
        auto operand_in_bc = [&](const operand &op) {
            if (!op.is_temp())
                return false;
            auto home = temp_regs_.find(op.temp_id);
            return home != temp_regs_.end() &&
                   home->second == temp_home::main_bc;
        };
        auto byte_or_immediate = [](const operand &op) {
            return op.is_none() || op.kind == operand_kind::INT_CONST ||
                   (op.type && op.type->size() == 1);
        };
        auto iy_byte_load_preserves_de = [&](const icode &ic) {
            if (ic.op != icode_op::GET_VALUE_AT || !ic.left.is_temp() ||
                !ic.right.is_none() || !ic.result.type ||
                ic.result.type->size() != 1) {
                return false;
            }
            auto ptr = temp_regs_.find(ic.left.temp_id);
            return ptr != temp_regs_.end() &&
                   (ptr->second == temp_home::main_iy ||
                    ptr->second == temp_home::main_bc);
        };
        auto byte_op_preserves_de = [&](const icode &ic) {
            switch (ic.op) {
            case icode_op::CAST:
            case icode_op::NEG:
            case icode_op::BNOT:
            case icode_op::SUB:
            case icode_op::BAND:
            case icode_op::BOR:
            case icode_op::BXOR:
            case icode_op::SHL:
            case icode_op::SHR:
            case icode_op::ROL:
            case icode_op::ROR:
                return ic.result.type && ic.result.type->size() == 1 &&
                       byte_or_immediate(ic.left) &&
                       byte_or_immediate(ic.right);
            case icode_op::ASSIGN:
                if (ic.result.type && ic.result.type->size() == 2 &&
                    ic.result.is_temp() && ic.left.is_temp()) {
                    auto result_home = temp_regs_.find(ic.result.temp_id);
                    if (result_home != temp_regs_.end() &&
                        (result_home->second == temp_home::main_iy ||
                         result_home->second == temp_home::main_bc))
                        return true;
                }
                if (direct_word_operand_preserves_de(ic.result) &&
                    direct_word_operand_preserves_de(ic.left))
                    return true;
                return ic.result.type && ic.result.type->size() == 1 &&
                       byte_or_immediate(ic.left) &&
                       byte_or_immediate(ic.right);
            case icode_op::ADD:
                if (de_preserving_indexed_memory_op(ic))
                    return true;
                if (direct_small_word_step_preserves_de(ic))
                    return true;
                if (ic.result.type && ic.result.type->size() == 2 &&
                    ic.right.kind == operand_kind::INT_CONST &&
                    ic.right.ival == 1 && ic.left.is_temp()) {
                    const bool direct_self_update =
                        ic.result.is_temp() &&
                        ic.result.temp_id == ic.left.temp_id;
                    auto left_home = temp_regs_.find(ic.left.temp_id);
                    const bool direct_pair_update =
                        left_home != temp_regs_.end() &&
                        (left_home->second == temp_home::main_iy ||
                         left_home->second == temp_home::main_bc);
                    if (direct_self_update || direct_pair_update)
                        return true;
                }
                return ic.result.type && ic.result.type->size() == 1 &&
                       byte_or_immediate(ic.left) &&
                       byte_or_immediate(ic.right);
            case icode_op::LABEL:
            case icode_op::GOTO:
            case icode_op::IFX:
                return true;
            case icode_op::GET_VALUE_AT:
                // A byte load through IY is just "ld a, 0(iy)" plus the
                // result store, so it does not consume the DE scratch pair.
                return iy_byte_load_preserves_de(ic);
            case icode_op::SET_VALUE_AT:
                return de_preserving_indexed_memory_op(ic);
            case icode_op::EQ:
            case icode_op::NE:
            case icode_op::LT:
            case icode_op::LE:
            case icode_op::GT:
            case icode_op::GE:
                return ((ic.left.kind == operand_kind::INT_CONST &&
                         ic.right.type && ic.right.type->size() == 1) ||
                        (ic.right.kind == operand_kind::INT_CONST &&
                         ic.left.type && ic.left.type->size() == 1) ||
                        (ic.left.kind == operand_kind::INT_CONST &&
                         operand_in_bc(ic.right)) ||
                        (ic.right.kind == operand_kind::INT_CONST &&
                         operand_in_bc(ic.left)));
            default:
                return false;
            }
        };
        auto d_temp_use_safe = [&](const icode &ic, int tid) {
            const bool use_left = ic.left.is_temp() && ic.left.temp_id == tid;
            const bool use_right = ic.right.is_temp() && ic.right.temp_id == tid;
            const bool use_result =
                ic.result.is_temp() && ic.result.temp_id == tid;
            if (!use_left && !use_right && !use_result)
                return byte_op_preserves_de(ic);
            if (use_result)
                return false;

            switch (ic.op) {
            case icode_op::ASSIGN:
            case icode_op::CAST:
            case icode_op::RETURN:
                return use_left && !use_right;
            case icode_op::ADD:
            case icode_op::SUB:
            case icode_op::BAND:
            case icode_op::BOR:
            case icode_op::BXOR:
                return ic.result.type && ic.result.type->size() == 1 &&
                       byte_or_immediate(ic.left) &&
                       byte_or_immediate(ic.right);
            case icode_op::EQ:
            case icode_op::NE:
                return !use_result && use_left != use_right &&
                       byte_or_immediate(use_left ? ic.right : ic.left);
            case icode_op::LT:
            case icode_op::LE:
            case icode_op::GT:
            case icode_op::GE:
                return (use_left && !use_right &&
                        ic.right.kind == operand_kind::INT_CONST) ||
                       (use_right && !use_left &&
                        ic.left.kind == operand_kind::INT_CONST);
            case icode_op::PACK_BYTES:
                return !use_result && use_left != use_right &&
                       ic.result.type && ic.result.type->size() == 2 &&
                       byte_or_immediate(ic.left) &&
                       byte_or_immediate(ic.right);
            case icode_op::SET_VALUE_AT:
                return use_left && !use_right && !use_result &&
                       de_preserving_indexed_memory_op(ic);
            default:
                return false;
            }
        };

        for (auto &[fd, tid] : order) {
            const interval &iv = ivs[tid];
            if (iv.size != 1 || iv.has_addr_of ||
                iv.first_def < 0 || iv.last_use <= iv.first_def ||
                iv.mentions < 2 ||
                temp_regs_.find(tid) != temp_regs_.end()) {
                continue;
            }
            const icode &def_ic = fn.icodes[iv.first_def];
            if (def_ic.op != icode_op::GET_VALUE_AT ||
                !def_ic.result.is_temp() ||
                def_ic.result.temp_id != tid) {
                continue;
            }

            bool safe = true;
            for (int k = iv.first_def + 1; k <= iv.last_use; ++k) {
                const icode &ic = fn.icodes[k];
                if (ic.op == icode_op::CALL || ic.op == icode_op::ALLOCA ||
                    ic.op == icode_op::INLINE_ASM ||
                    ic.op == icode_op::ADDRESS_OF ||
                    !d_temp_use_safe(ic, tid)) {
                    safe = false;
                    break;
                }
            }
            if (!safe)
                continue;
            d_candidates.push_back(
                {tid, iv.first_def, iv.last_use,
                 300 + hot_mentions(iv) * 12 -
                     (iv.last_use - iv.first_def)});
        }
    }
    std::sort(d_candidates.begin(), d_candidates.end(),
              [](const d_candidate &a, const d_candidate &b) {
                  if (a.score != b.score) return a.score > b.score;
                  if (a.start != b.start) return a.start < b.start;
                  return a.tid < b.tid;
              });
    std::vector<std::pair<int, int>> d_windows;
    for (const auto &cand : d_candidates) {
        if (overlaps_windows(de_windows, cand.start, cand.end) ||
            overlaps_windows(d_windows, cand.start, cand.end))
            continue;
        temp_regs_[cand.tid] = temp_home::main_d;
        d_windows.push_back({cand.start, cand.end});
    }

    struct e_candidate {
        int tid = -1;
        int start = -1;
        int end = -1;
        int score = 0;
    };
    std::vector<e_candidate> e_candidates;
    if (size_opt_enabled() || tuned_profile_enabled()) {
        auto home_is = [&](const operand &op, temp_home home) {
            if (!op.is_temp())
                return false;
            auto it = temp_regs_.find(op.temp_id);
            return it != temp_regs_.end() && it->second == home;
        };
        auto byte_or_immediate = [](const operand &op) {
            return op.is_none() || op.kind == operand_kind::INT_CONST ||
                   (op.type && op.type->size() == 1);
        };
        auto iy_byte_load_preserves_e = [&](const icode &ic) {
            if (ic.op != icode_op::GET_VALUE_AT || !ic.left.is_temp() ||
                !ic.right.is_none() || !ic.result.type ||
                ic.result.type->size() != 1) {
                return false;
            }
            auto ptr = temp_regs_.find(ic.left.temp_id);
            return ptr != temp_regs_.end() &&
                   ptr->second == temp_home::main_iy;
        };
        auto iy_word_load_to_bc_preserves_e = [&](const icode &ic) {
            if (ic.op != icode_op::GET_VALUE_AT || !ic.left.is_temp() ||
                !ic.right.is_none() || !ic.result.is_temp() ||
                !ic.result.type || ic.result.type->size() != 2) {
                return false;
            }
            auto ptr = temp_regs_.find(ic.left.temp_id);
            auto dst = temp_regs_.find(ic.result.temp_id);
            return ptr != temp_regs_.end() &&
                   ptr->second == temp_home::main_iy &&
                   dst != temp_regs_.end() &&
                   dst->second == temp_home::main_bc;
        };
        auto candidate_byte_use_safe = [&](const icode &ic, int tid) {
            const bool use_result =
                ic.result.is_temp() && ic.result.temp_id == tid;
            const bool use_left = ic.left.is_temp() && ic.left.temp_id == tid;
            const bool use_right = ic.right.is_temp() && ic.right.temp_id == tid;
            if (!use_result && !use_left && !use_right)
                return false;

            switch (ic.op) {
            case icode_op::ASSIGN:
            case icode_op::CAST:
                return byte_or_immediate(ic.result) &&
                       byte_or_immediate(ic.left) && !use_right;
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
                return ic.result.type && ic.result.type->size() == 1 &&
                       byte_or_immediate(ic.left) &&
                       byte_or_immediate(ic.right);
            case icode_op::EQ:
            case icode_op::NE:
                return !use_result && use_left != use_right &&
                       byte_or_immediate(use_left ? ic.right : ic.left);
            case icode_op::LT:
            case icode_op::LE:
            case icode_op::GT:
            case icode_op::GE:
                return !use_result &&
                       ((use_left && ic.right.kind == operand_kind::INT_CONST) ||
                        (use_right && ic.left.kind == operand_kind::INT_CONST));
            case icode_op::PACK_BYTES:
                return !use_result && use_left != use_right &&
                       ic.result.type && ic.result.type->size() == 2 &&
                       byte_or_immediate(ic.left) &&
                       byte_or_immediate(ic.right);
            case icode_op::RETURN:
                return use_left && !use_result && !use_right;
            case icode_op::SET_VALUE_AT:
                return use_left && !use_result && !use_right &&
                       de_preserving_indexed_memory_op(ic);
            default:
                return false;
            }
        };
        auto byte_op_preserves_e = [&](const icode &ic) {
            switch (ic.op) {
            case icode_op::ASSIGN:
                if (ic.result.type && ic.result.type->size() == 2 &&
                    ic.result.is_temp() && ic.left.is_temp()) {
                    auto result_home = temp_regs_.find(ic.result.temp_id);
                    if (result_home != temp_regs_.end() &&
                        (result_home->second == temp_home::main_iy ||
                         result_home->second == temp_home::main_bc))
                        return true;
                }
                if (direct_word_operand_preserves_de(ic.result) &&
                    direct_word_operand_preserves_de(ic.left))
                    return true;
                [[fallthrough]];
            case icode_op::CAST:
                if (ic.result.type && ic.result.type->size() == 2 &&
                    ic.left.type && ic.left.type->size() == 1 &&
                    ic.right.is_none()) {
                    return true;
                }
                [[fallthrough]];
            case icode_op::NEG:
            case icode_op::BNOT:
            case icode_op::SUB:
            case icode_op::BAND:
            case icode_op::BOR:
            case icode_op::BXOR:
            case icode_op::SHL:
            case icode_op::SHR:
            case icode_op::ROL:
            case icode_op::ROR:
                return ic.result.type && ic.result.type->size() == 1 &&
                       byte_or_immediate(ic.left) &&
                       byte_or_immediate(ic.right);
            case icode_op::ADD:
                if (de_preserving_indexed_memory_op(ic))
                    return true;
                if (direct_small_word_step_preserves_de(ic))
                    return true;
                if (ic.result.type && ic.result.type->size() == 2 &&
                    ic.right.kind == operand_kind::INT_CONST &&
                    ic.right.ival == 1 && ic.left.is_temp()) {
                    const bool direct_self_update =
                        ic.result.is_temp() &&
                        ic.result.temp_id == ic.left.temp_id;
                    auto left_home = temp_regs_.find(ic.left.temp_id);
                    const bool direct_iy_update =
                        left_home != temp_regs_.end() &&
                        left_home->second == temp_home::main_iy;
                    if (direct_self_update || direct_iy_update)
                        return true;
                }
                return ic.result.type && ic.result.type->size() == 1 &&
                       byte_or_immediate(ic.left) &&
                       byte_or_immediate(ic.right);
            case icode_op::EQ:
            case icode_op::NE:
                return ic.left.type && ic.left.type->size() == 1 &&
                       ic.right.type && ic.right.type->size() == 1;
            case icode_op::LT:
            case icode_op::LE:
            case icode_op::GT:
            case icode_op::GE:
                return ((ic.left.kind == operand_kind::INT_CONST &&
                         ic.right.type && ic.right.type->size() == 1) ||
                        (ic.right.kind == operand_kind::INT_CONST &&
                         ic.left.type && ic.left.type->size() == 1));
            case icode_op::LABEL:
            case icode_op::GOTO:
            case icode_op::IFX:
                return true;
            case icode_op::GET_VALUE_AT:
                return iy_byte_load_preserves_e(ic) ||
                       iy_word_load_to_bc_preserves_e(ic);
            case icode_op::SET_VALUE_AT:
                return de_preserving_indexed_memory_op(ic);
            default:
                return false;
            }
        };
        auto direct_bc_postinc = [&](int k, int end) {
            if (k + 3 > end)
                return false;
            const icode &old_ic = fn.icodes[k];
            const icode &step_ic = fn.icodes[k + 1];
            const icode &commit_ic = fn.icodes[k + 2];
            const icode &get_ic = fn.icodes[k + 3];
            const auto old_iv = old_ic.result.is_temp()
                                    ? ivs.find(old_ic.result.temp_id)
                                    : ivs.end();
            const auto step_iv = step_ic.result.is_temp()
                                     ? ivs.find(step_ic.result.temp_id)
                                     : ivs.end();
            return old_ic.op == icode_op::ASSIGN &&
                   old_ic.result.is_temp() && home_is(old_ic.left, temp_home::main_bc) &&
                   old_ic.right.is_none() && old_ic.left.type &&
                   old_ic.left.type->size() == 2 &&
                   step_ic.op == icode_op::ADD && step_ic.result.is_temp() &&
                   step_ic.left.is_temp() && old_ic.left.is_temp() &&
                   step_ic.left.temp_id == old_ic.left.temp_id &&
                   step_ic.right.kind == operand_kind::INT_CONST &&
                   step_ic.right.ival == 1 &&
                   commit_ic.op == icode_op::ASSIGN &&
                   commit_ic.result.is_temp() &&
                   commit_ic.result.temp_id == old_ic.left.temp_id &&
                   commit_ic.left.is_temp() &&
                   commit_ic.left.temp_id == step_ic.result.temp_id &&
                   get_ic.op == icode_op::GET_VALUE_AT &&
                   get_ic.left.is_temp() &&
                   get_ic.left.temp_id == old_ic.result.temp_id &&
                   get_ic.result.type && get_ic.result.type->size() == 1 &&
                   get_ic.right.is_none() &&
                   old_iv != ivs.end() && old_iv->second.last_use == k + 3 &&
                   step_iv != ivs.end() && step_iv->second.last_use == k + 2;
        };
        auto direct_byte_band_ifx = [&](int k, int end, int tid) {
            if (!compare_ifx_fusion_enabled() || k + 1 > end)
                return false;
            const icode &band = fn.icodes[k];
            const icode &ifx = fn.icodes[k + 1];
            if (band.op != icode_op::BAND || !band.result.is_temp() ||
                ifx.op != icode_op::IFX || !ifx.left.is_temp() ||
                ifx.left.temp_id != band.result.temp_id) {
                return false;
            }
            const operand *value = &band.left;
            const operand *mask = &band.right;
            if (mask->is_temp() && mask->temp_id == tid)
                std::swap(value, mask);
            if (!value->is_temp() || value->temp_id != tid ||
                !value->type || value->type->size() != 1 ||
                mask->kind != operand_kind::INT_CONST) {
                return false;
            }
            auto result_iv = ivs.find(band.result.temp_id);
            return result_iv != ivs.end() &&
                   result_iv->second.last_use == k + 1;
        };

        for (auto &[fd, tid] : order) {
            const interval &iv = ivs[tid];
            if (iv.size != 1 || iv.has_addr_of ||
                iv.first_def < 0 || iv.last_use <= iv.first_def ||
                iv.mentions < 2 ||
                temp_regs_.find(tid) != temp_regs_.end()) {
                continue;
            }
            const icode &def_ic = fn.icodes[iv.first_def];
            if ((def_ic.op != icode_op::ASSIGN &&
                 def_ic.op != icode_op::CAST) ||
                !def_ic.result.is_temp() || def_ic.result.temp_id != tid ||
                !byte_or_immediate(def_ic.left)) {
                continue;
            }

            bool saw_backedge = false;
            bool safe = true;
            for (int k = iv.first_def + 1; k <= iv.last_use; ++k) {
                const icode &ic = fn.icodes[k];
                if (candidate_byte_use_safe(ic, tid))
                    continue;
                if (direct_byte_band_ifx(k, iv.last_use, tid))
                    continue;
                if (direct_bc_postinc(k, iv.last_use)) {
                    k += 3;
                    continue;
                }
                if (ic.op == icode_op::ADD && ic.result.type &&
                    ic.result.type->size() == 2 &&
                    home_is(ic.left, temp_home::main_bc) &&
                    ic.right.kind == operand_kind::INT_CONST) {
                    continue;
                }
                if (is_compare_op(ic.op) &&
                    home_is(ic.left, temp_home::main_bc) &&
                    ic.left.type &&
                    (ic.left.type->is_unsigned() || ic.left.type->is_ptr()) &&
                    k + 1 <= iv.last_use &&
                    fn.icodes[k + 1].op == icode_op::IFX) {
                    continue;
                }
                if (ic.op == icode_op::GOTO) {
                    auto it = label_indices.find(ic.label_name);
                    if (it != label_indices.end() && it->second < k)
                        saw_backedge = true;
                }
                if (!byte_op_preserves_e(ic)) {
                    safe = false;
                    break;
                }
            }
            if (!safe || !saw_backedge)
                continue;
            e_candidates.push_back(
                {tid, iv.first_def, iv.last_use,
                 360 + hot_mentions(iv) * 14 -
                     (iv.last_use - iv.first_def)});
        }
    }
    std::sort(e_candidates.begin(), e_candidates.end(),
              [](const e_candidate &a, const e_candidate &b) {
                  if (a.score != b.score) return a.score > b.score;
                  if (a.start != b.start) return a.start < b.start;
                  return a.tid < b.tid;
              });
    std::vector<std::pair<int, int>> e_windows;
    for (const auto &cand : e_candidates) {
        if (overlaps_windows(de_windows, cand.start, cand.end) ||
            overlaps_windows(e_windows, cand.start, cand.end))
            continue;
        temp_regs_[cand.tid] = temp_home::main_e;
        e_windows.push_back({cand.start, cand.end});
    }

    // Keep a freshly loaded byte in A across a short, pair-only address
    // calculation until it is stored.  Ordinary word ASSIGN/ADD lowering does
    // not use A, but a word operand rematerialized from an unsigned byte does.
    // Reject those promoted-byte address components so the destination cannot
    // overwrite the source byte before the final SET_VALUE_AT.
    std::function<bool(const operand &, int, int)> pair_load_may_use_a;
    pair_load_may_use_a = [&](const operand &op, int before, int depth) {
        if (depth > 6 || op.kind == operand_kind::INT_CONST)
            return false;
        if (op.type && op.type->size() == 1)
            return true;
        if (op.is_symbol())
            return op.is_global && op.is_tls;
        if (!op.is_temp())
            return false;

        auto home = temp_regs_.find(op.temp_id);
        if (home != temp_regs_.end() &&
            (home->second == temp_home::main_a ||
             home->second == temp_home::alt_a ||
             home->second == temp_home::arg_a)) {
            return true;
        }

        const icode *def = find_temp_def_before(op.temp_id, before);
        if (!def)
            return false;
        if (def->op == icode_op::ASSIGN || def->op == icode_op::CAST)
            return pair_load_may_use_a(def->left, before, depth + 1);
        return false;
    };

    std::vector<std::pair<int, int>> main_a_windows;
    for (const auto &[tid, iv] : ivs) {
        if (iv.size != 1 || iv.has_addr_of || iv.definitions != 1 ||
            iv.first_def < 0 || iv.last_use <= iv.first_def + 1 ||
            iv.last_use - iv.first_def > 5 ||
            temp_regs_.find(tid) != temp_regs_.end()) {
            continue;
        }
        const icode &def = fn.icodes[iv.first_def];
        if (def.op != icode_op::GET_VALUE_AT || !def.result.is_temp() ||
            def.result.temp_id != tid ||
            (def.result.type && def.result.type->is_volatile) ||
            (def.left.type && def.left.type->base &&
             def.left.type->base->is_volatile)) {
            continue;
        }

        bool safe = true;
        for (int k = iv.first_def + 1; k <= iv.last_use; ++k) {
            const icode &ic = fn.icodes[k];
            if (mentions_temp(ic, tid)) {
                const bool final_store =
                    k == iv.last_use && ic.op == icode_op::SET_VALUE_AT &&
                    ic.left.is_temp() && ic.left.temp_id == tid &&
                    op_size(ic.left) == 1;
                if (!final_store) {
                    safe = false;
                    break;
                }
                continue;
            }
            const bool pair_assign =
                ic.op == icode_op::ASSIGN && op_size(ic.result) == 2 &&
                op_size(ic.left) == 2 &&
                !pair_load_may_use_a(ic.left, k, 0);
            const bool pair_add =
                ic.op == icode_op::ADD && op_size(ic.result) == 2 &&
                op_size(ic.left) <= 2 && op_size(ic.right) <= 2 &&
                !pair_load_may_use_a(ic.left, k, 0) &&
                !pair_load_may_use_a(ic.right, k, 0);
            if (!pair_assign && !pair_add) {
                safe = false;
                break;
            }
        }
        if (!safe)
            continue;
        temp_regs_[tid] = temp_home::main_a;
        main_a_windows.push_back({iv.first_def, iv.last_use});
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

    const bool one_step_temp_homes_enabled = true;
    const bool one_step_a_enabled = true;
    // Retaining a just-produced word in HL is safe for consumers that use HL
    // directly before preparing any other address or operand. Wider binary
    // cases remain on the separately proven commutative-RHS path below.
    const bool one_step_hl_enabled = !size_opt_enabled();
    if (one_step_temp_homes_enabled) {
        std::vector<std::pair<int, int>> one_step_hl_windows = hl_windows;
        for (const auto &[other_tid, home] : temp_regs_) {
            if (home != temp_home::main_hl)
                continue;
            auto other = ivs.find(other_tid);
            if (other != ivs.end())
                one_step_hl_windows.push_back(
                    {other->second.first_def, other->second.last_use});
        }

        for (auto &[tid, iv] : ivs) {
            if (iv.first_def < 0)                  continue;
            if (iv.last_use != iv.first_def + 1)   continue;
            if (!contiguous_live_window(iv, tid))  continue;
            const icode &def_ic = fn.icodes[iv.first_def];
            const icode &use_ic = fn.icodes[iv.last_use];
            if (iv.has_addr_of)                    continue;
            if ((use_ic.op == icode_op::ASSIGN || use_ic.op == icode_op::CAST) &&
                use_ic.result.is_temp()) {
                continue;
            }
            if (use_ic.op == icode_op::ASSIGN &&
                use_ic.result.kind == operand_kind::SYMBOL) {
                continue;
            }

            if (one_step_a_enabled &&
                !alt_a_use_hazard(use_ic) &&
                iv.size == 1 && alt_a_def_safe(def_ic) &&
                immediate_use_safe_in_a(use_ic, tid) &&
                !overlaps_windows(main_a_windows, iv.first_def, iv.last_use)) {
                temp_regs_[tid] = temp_home::main_a;
                main_a_windows.push_back({iv.first_def, iv.last_use});
                continue;
            }
            if (one_step_hl_enabled &&
                iv.size == 2 && main_hl_def_safe(def_ic) &&
                immediate_use_safe_in_hl(use_ic, tid) &&
                ((use_ic.op == icode_op::RETURN &&
                  use_ic.left.is_temp() && use_ic.left.temp_id == tid) ||
                 (use_ic.op == icode_op::GET_VALUE_AT &&
                  use_ic.left.is_temp() && use_ic.left.temp_id == tid))) {
                temp_regs_[tid] = temp_home::main_hl;
                one_step_hl_windows.push_back({iv.first_def, iv.last_use});
                continue;
            }

            const bool commutative_rhs_use =
                use_ic.right.is_temp() && use_ic.right.temp_id == tid &&
                !(use_ic.left.is_temp() && use_ic.left.temp_id == tid) &&
                (use_ic.op == icode_op::ADD ||
                 use_ic.op == icode_op::BAND ||
                 use_ic.op == icode_op::BOR ||
                 use_ic.op == icode_op::BXOR);
            if (iv.size == 2 && main_hl_def_safe(def_ic) &&
                commutative_rhs_use &&
                !overlaps_windows(one_step_hl_windows,
                                  iv.first_def, iv.last_use)) {
                temp_regs_[tid] = temp_home::main_hl;
                one_step_hl_windows.push_back({iv.first_def, iv.last_use});
                continue;
            }
        }
    }

    // Keep broad pointer rematerialization disabled. Expression-shape and
    // single-definition checks alone do not prove the pointed-to allocator
    // state stable; the realloc/free-list regression catches that distinction.
    // The narrower ADDRESS_OF and u8-index paths below remain proven.
    const bool general_pointer_remat_enabled = false;

    if (general_pointer_remat_enabled &&
        (size_opt_enabled() || tuned_profile_enabled())) {
        for (auto &[fd, tid] : order) {
            const interval &iv = ivs[tid];
            const bool ok = remat_pointer_temp_ok(tid, 0);
            if (iv.has_addr_of)                    continue;
            if (temp_regs_.find(tid) != temp_regs_.end())
                continue;
            if (!ok)                              continue;
            temp_regs_[tid] = temp_home::remat_hl;
        }
    }

    if (size_opt_enabled() || tuned_profile_enabled()) {
        for (auto &[fd, tid] : order) {
            const interval &iv = ivs[tid];
            if (iv.has_addr_of)
                continue;
            if (temp_regs_.find(tid) != temp_regs_.end())
                continue;
            if (iv.first_def < 0)
                continue;
            const icode &def_ic = fn.icodes[iv.first_def];
            if (def_ic.op != icode_op::ADDRESS_OF)
                continue;
            if (!remat_pointer_temp_ok(tid, 0))
                continue;
            temp_regs_[tid] = temp_home::remat_hl;
        }
    }

    if (u8_index_remat_enabled &&
        (size_opt_enabled() || tuned_profile_enabled())) {
        for (auto &[fd, tid] : order) {
            const interval &iv = ivs[tid];
            if (iv.has_addr_of)                    continue;
            if (temp_regs_.find(tid) != temp_regs_.end())
                continue;
            if (!remat_u8_index_temp_ok(tid, 0))  continue;
            temp_regs_[tid] = temp_home::remat_hl;
        }
    }

    // A single global word read used as a later argument can be delayed past
    // other argument SENDs.  With no call, store, or control-flow instruction
    // in between, the memory version cannot change; rematerializing at the
    // final SEND removes an otherwise mandatory spill/reload pair.
    if (size_opt_enabled() || tuned_profile_enabled()) {
        for (auto &[fd, tid] : order) {
            const interval &iv = ivs[tid];
            if (iv.size != 2 || iv.has_addr_of || iv.definitions != 1 ||
                iv.mentions != 1 || iv.first_def < 0 ||
                iv.last_use <= iv.first_def ||
                temp_regs_.find(tid) != temp_regs_.end()) {
                continue;
            }

            const icode &def_ic = fn.icodes[iv.first_def];
            const icode &use_ic = fn.icodes[iv.last_use];
            if (def_ic.op != icode_op::ASSIGN ||
                !def_ic.result.is_temp() ||
                def_ic.result.temp_id != tid ||
                !def_ic.right.is_none() ||
                def_ic.left.kind != operand_kind::SYMBOL ||
                !def_ic.left.is_global || def_ic.left.is_param ||
                def_ic.left.is_func || def_ic.left.is_tls ||
                def_ic.left.is_sfr || !def_ic.left.type ||
                def_ic.left.type->is_volatile ||
                op_size(def_ic.left) != 2 ||
                use_ic.op != icode_op::SEND ||
                !use_ic.left.is_temp() ||
                use_ic.left.temp_id != tid) {
                continue;
            }

            bool only_argument_sends = true;
            for (int k = iv.first_def + 1; k < iv.last_use; ++k) {
                if (fn.icodes[k].op != icode_op::SEND) {
                    only_argument_sends = false;
                    break;
                }
            }
            if (!only_argument_sends)
                continue;

            temp_regs_[tid] = temp_home::remat_hl;
        }
    }

    // Keep word-load rematerialization disabled as well. Even the branch-free,
    // call-free, store-free, single-definition filter is insufficient for the
    // heap free-list case; enabling this requires real memory SSA/versioning.
    const bool word_load_remat_enabled = false;
    if (word_load_remat_enabled &&
        (size_opt_enabled() || tuned_profile_enabled())) {
        for (auto &[fd, tid] : order) {
            if (temp_regs_.find(tid) != temp_regs_.end())
                continue;
            if (!remat_word_load_temp_ok(tid, 0))
                continue;
            temp_regs_[tid] = temp_home::remat_hl;
        }
    }

    // Prefer BC's seven-cycle indirect byte access for a walking cursor and
    // leave IY to an overlapping invariant pointer that is only copied or
    // used arithmetically.  Initial allocation cannot always see this swap:
    // IY cursors are selected before the general BC candidates.  Re-homing
    // the proven pair here changes no live ranges and avoids the much slower
    // indexed-IY load on every loop iteration.
    for (const auto &[cursor_tid, cursor_home] : temp_regs_) {
        if (opt_settings_.level != opt_level::Of &&
            opt_settings_.level != opt_level::O3 &&
            opt_settings_.level != opt_level::Os)
            break;
        if (cursor_home != temp_home::main_iy)
            continue;
        auto cursor_iv = ivs.find(cursor_tid);
        if (cursor_iv == ivs.end() || cursor_iv->second.first_def < 0)
            continue;

        bool byte_walk = false;
        bool cursor_safe = true;
        for (int k = cursor_iv->second.first_def + 1;
             k <= cursor_iv->second.last_use; ++k) {
            const icode &ic = fn.icodes[k];
            if (!mentions_temp(ic, cursor_tid))
                continue;
            const bool byte_load =
                ic.op == icode_op::GET_VALUE_AT && ic.left.is_temp() &&
                ic.left.temp_id == cursor_tid && ic.right.is_none() &&
                op_size(ic.result) == 1;
            const bool constant_step =
                (ic.op == icode_op::ADD || ic.op == icode_op::SUB) &&
                ic.left.is_temp() && ic.left.temp_id == cursor_tid &&
                ic.right.kind == operand_kind::INT_CONST;
            const bool update_commit =
                ic.op == icode_op::ASSIGN && ic.result.is_temp() &&
                ic.result.temp_id == cursor_tid;
            const bool arithmetic_read =
                (ic.op == icode_op::ADD || ic.op == icode_op::SUB ||
                 is_compare_op(ic.op) || ic.op == icode_op::RETURN) &&
                !(ic.result.is_temp() && ic.result.temp_id == cursor_tid);
            if (byte_load) {
                byte_walk = true;
                continue;
            }
            if (constant_step || update_commit || arithmetic_read)
                continue;
            cursor_safe = false;
            break;
        }
        if (!cursor_safe || !byte_walk)
            continue;

        // Symbol homes participate in the same physical BC pair but are kept
        // in a separate map.  A post-allocation temp swap must therefore
        // reject any overlapping BC-resident symbol explicitly; otherwise a
        // promoted loop counter and the cursor silently alias one another.
        bool bc_symbol_conflict = false;
        for (const auto &[key, home] : symbol_regs_) {
            if (home != temp_home::main_bc)
                continue;
            auto sym = syms.find(key);
            if (sym == syms.end())
                continue;
            if (!(sym->second.last_idx < cursor_iv->second.first_def ||
                  sym->second.first_idx > cursor_iv->second.last_use)) {
                bc_symbol_conflict = true;
                break;
            }
        }
        if (bc_symbol_conflict)
            continue;

        // A later, independent BC allocation can overlap this cursor even
        // when the pointer chosen as the swap partner is safe.  In
        // particular, a reduction confined to a second loop may occupy BC
        // while the cursor spans both loops.  Reject the swap when any
        // overlapping non-pointer temp already owns BC; otherwise replacing
        // the cursor's IY home would silently make the two values alias.
        bool bc_value_conflict = false;
        for (const auto &[other_tid, other_home] : temp_regs_) {
            if (other_home != temp_home::main_bc ||
                other_tid == cursor_tid)
                continue;
            auto other_iv = ivs.find(other_tid);
            if (other_iv == ivs.end() ||
                other_iv->second.last_use < cursor_iv->second.first_def ||
                other_iv->second.first_def > cursor_iv->second.last_use)
                continue;
            const icode &other_def =
                fn.icodes[other_iv->second.first_def];
            if (!other_def.result.type ||
                !other_def.result.type->is_ptr()) {
                bc_value_conflict = true;
                break;
            }
        }
        if (bc_value_conflict)
            continue;

        bool placed_in_bc = false;
        for (const auto &[base_tid, base_home] : temp_regs_) {
            if (base_home != temp_home::main_bc || base_tid == cursor_tid)
                continue;
            auto base_iv = ivs.find(base_tid);
            if (base_iv == ivs.end() || base_iv->second.size != 2 ||
                base_iv->second.first_def < 0 ||
                base_iv->second.last_use < cursor_iv->second.first_def ||
                base_iv->second.first_def > cursor_iv->second.last_use)
                continue;
            const icode &base_def = fn.icodes[base_iv->second.first_def];
            if (!base_def.result.type || !base_def.result.type->is_ptr() ||
                base_def.result.type->is_far_ptr())
                continue;

            bool invariant_safe = true;
            for (int k = base_iv->second.first_def + 1;
                 k <= base_iv->second.last_use; ++k) {
                const icode &ic = fn.icodes[k];
                if (!mentions_temp(ic, base_tid))
                    continue;
                const bool copied_to_cursor =
                    ic.op == icode_op::ASSIGN && ic.left.is_temp() &&
                    ic.left.temp_id == base_tid && ic.result.is_temp() &&
                    ic.result.temp_id == cursor_tid;
                const bool arithmetic_read =
                    (ic.op == icode_op::ADD || ic.op == icode_op::SUB ||
                     is_compare_op(ic.op)) &&
                    !(ic.result.is_temp() && ic.result.temp_id == base_tid);
                if (!copied_to_cursor && !arithmetic_read) {
                    invariant_safe = false;
                    break;
                }
            }
            if (!invariant_safe)
                continue;

            temp_regs_[cursor_tid] = temp_home::main_bc;
            temp_regs_[base_tid] = temp_home::main_iy;
            placed_in_bc = true;
            break;
        }
        if (placed_in_bc)
            continue;
    }

    // Record the precise direct calls at which a selected IY live range must
    // be caller-saved.  Candidate validation above admits only direct calls
    // with register-only arguments; assert the same properties here so later
    // allocator extensions cannot accidentally alter a stack-call layout.
    for (const auto &[tid, home] : temp_regs_) {
        if (home != temp_home::main_iy)
            continue;
        auto iv_it = ivs.find(tid);
        if (iv_it == ivs.end())
            continue;
        const interval &iv = iv_it->second;
        for (int k = iv.first_def + 1; k < iv.last_use; ++k) {
            const icode &ic = fn.icodes[k];
            if (ic.op == icode_op::CALL && !ic.func_name.empty() &&
                ic.arg_bytes == 0) {
                iy_preserved_call_indices_.insert(static_cast<size_t>(k));
            }
        }
    }

    // A constant-offset pointer derived from an IY-resident base is
    // rematerialized as (iy+d) by GET/SET lowering.  If direct dereferences
    // are its only uses, emitting the original ADD merely computes and spills
    // a value that no machine instruction reads.
    for (size_t def_index = 0; def_index < fn.icodes.size(); ++def_index) {
        const icode &def = fn.icodes[def_index];
        if (def.op != icode_op::ADD || !def.result.is_temp() ||
            !def.result.type || !def.result.type->is_ptr() ||
            def.result.type->is_far_ptr()) {
            continue;
        }

        const operand *base = &def.left;
        const operand *offset = &def.right;
        if (base->kind == operand_kind::INT_CONST)
            std::swap(base, offset);
        if (!base->is_temp() || offset->kind != operand_kind::INT_CONST)
            continue;

        auto base_home = temp_regs_.find(base->temp_id);
        if (base_home == temp_regs_.end() ||
            base_home->second != temp_home::main_iy ||
            offset->ival < -128 || offset->ival > 126) {
            continue;
        }

        const int derived_tid = def.result.temp_id;
        auto derived_iv = ivs.find(derived_tid);
        if (derived_iv == ivs.end() ||
            derived_iv->second.definitions != 1 ||
            derived_iv->second.first_def != static_cast<int>(def_index)) {
            continue;
        }
        bool saw_use = false;
        bool direct_dereferences_only = true;
        for (size_t use_index = def_index + 1;
             use_index < fn.icodes.size(); ++use_index) {
            const icode &use = fn.icodes[use_index];
            const bool in_result =
                use.result.is_temp() &&
                use.result.temp_id == derived_tid;
            const bool in_left =
                use.left.is_temp() && use.left.temp_id == derived_tid;
            const bool in_right =
                use.right.is_temp() && use.right.temp_id == derived_tid;
            if (!in_result && !in_left && !in_right)
                continue;

            saw_use = true;
            const bool direct_load =
                use.op == icode_op::GET_VALUE_AT && in_left &&
                !in_result && !in_right && use.right.is_none() &&
                (op_size(use.result) == 1 || op_size(use.result) == 2 ||
                 op_size(use.result) == 4) &&
                offset->ival + op_size(use.result) - 1 <= 127 &&
                !(use.result.type && use.result.type->is_volatile) &&
                !(use.left.type && use.left.type->base &&
                  use.left.type->base->is_volatile);
            const bool direct_store =
                use.op == icode_op::SET_VALUE_AT && in_result &&
                !in_left && !in_right && use.right.is_none() &&
                (op_size(use.left) == 1 || op_size(use.left) == 2) &&
                offset->ival + op_size(use.left) - 1 <= 127 &&
                !(use.left.type && use.left.type->is_volatile) &&
                !(use.result.type && use.result.type->base &&
                  use.result.type->base->is_volatile);
            if (!direct_load && !direct_store) {
                direct_dereferences_only = false;
                break;
            }
        }

        if (saw_use && direct_dereferences_only)
            skipped_icodes_.insert(def_index);
    }

    // NOTE: BC'/DE'/HL' via EXX are not used — EXX swaps all three pairs
    // atomically and would corrupt DE/BC that the generator uses for operand
    // loads in the same window.  An EXX-block optimizer is a planned future pass.
}

} // namespace xcc
