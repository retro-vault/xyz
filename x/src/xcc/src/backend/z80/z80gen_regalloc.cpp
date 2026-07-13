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
                !temp_used_after(idx + 2, ic.result.temp_id)) {
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

        if (size_opt_enabled() ||
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

    struct interval {
        int  first_def   = -1;
        int  last_use    = -1;
        int  size        = 0;
        int  mentions    = 0;
        int  definitions = 0;
        bool has_addr_of = false;
        bool loop_extended = false;
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

    int raw_temp_bytes = 0;
    for (const auto &[tid, iv] : ivs) {
        int sz = iv.size > 0 ? iv.size : 2;
        if (sz < 1)
            sz = 1;
        raw_temp_bytes += sz;
    }
    int frame_upper_bound = raw_temp_bytes;
    if (size_opt_enabled()) {
        // CFG coloring without register homes includes every temporary, so it
        // is a safe upper bound for the final colored frame and much tighter
        // than summing mutually exclusive SSA-like values.
        frame_upper_bound = compute_temp_frame_bytes(fn);
        temp_slots_.clear();
        next_temp_slot_ = 0;
    }
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

    struct iy_candidate {
        int tid = -1;
        int start = -1;
        int end = -1;
        int score = 0;
    };
    std::vector<iy_candidate> iy_candidates;
    for (const auto &[tid, iv] : ivs) {
        if (iv.size != 2 || iv.has_addr_of || iv.first_def < 0)
            continue;

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
        for (int k = iv.first_def + 1; k <= loop_end; ++k) {
            const icode &ic = fn.icodes[k];
            if (ic.op == icode_op::CALL || ic.op == icode_op::ALLOCA ||
                ic.op == icode_op::INLINE_ASM) {
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

            if (ic.op == icode_op::GET_VALUE_AT && ic.left.is_temp() &&
                ic.left.temp_id == tid && ic.right.is_none() &&
                ic.result.type &&
                (ic.result.type->size() == 1 || ic.result.type->size() == 2)) {
                saw_mem = true;
                continue;
            }
            if (ic.op == icode_op::SET_VALUE_AT && ic.result.is_temp() &&
                ic.result.temp_id == tid && ic.right.is_none() &&
                ic.left.type &&
                (ic.left.type->size() == 1 || ic.left.type->size() == 2)) {
                saw_mem = true;
                continue;
            }
            if ((ic.op == icode_op::ADD || ic.op == icode_op::SUB) &&
                ic.left.is_temp() && ic.left.temp_id == tid &&
                ic.right.kind == operand_kind::INT_CONST &&
                ic.right.ival != 0 && ic.right.ival >= -32767 &&
                ic.right.ival <= 32767 && ic.result.is_temp()) {
                step_temps.insert(ic.result.temp_id);
                saw_update = true;
                continue;
            }
            if (ic.op == icode_op::ASSIGN && ic.result.is_temp() &&
                ic.result.temp_id == tid && ic.left.is_temp() &&
                step_temps.count(ic.left.temp_id)) {
                saw_commit = true;
                continue;
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
             2400 + iv.mentions * 20 - (loop_end - iv.first_def)});
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
             2200 + iv.mentions * 20 - (iv.last_use - iv.first_def)});
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

    // IY is also profitable for a call-free loop invariant that is only read
    // by comparisons.  Such values otherwise occupy an IX slot for the whole
    // loop even though IY can hold them without competing with the arithmetic
    // pairs.  Keep this deliberately narrower than general IY allocation:
    // one definition, integer word, compare-only uses, and no calls or opaque
    // code anywhere in the live range.
    std::vector<iy_candidate> invariant_iy_candidates;
    for (const auto &[tid, iv] : ivs) {
        if (!size_opt_enabled() ||
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
             2000 + compare_uses * 100 + iv.mentions * 8 -
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
        if (iv.size != 2 || iv.first_def < 0 || iv.last_use <= iv.first_def)
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

        score_out = 240 + iv.mentions * 10 - (iv.last_use - iv.first_def);
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
        if (old_temp < 0 || step_temp < 0 || commit_idx < 0)
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

        auto is_cursor = [&](const operand &op) {
            return op.is_temp() && op.temp_id == temp_id;
        };
        auto is_old_cursor = [&](const operand &op) {
            return op.is_temp() && op.temp_id == old_temp;
        };

        for (int k = iv.first_def + 1; k <= loop_end; ++k) {
            const icode &ic = fn.icodes[k];
            if (clobbers_bc(ic) || uses_tls_global(ic.result) ||
                uses_tls_global(ic.left) || uses_tls_global(ic.right) ||
                symbol_word_access_may_need_bc_scratch(ic.result) ||
                symbol_word_access_may_need_bc_scratch(ic.left) ||
                symbol_word_access_may_need_bc_scratch(ic.right)) {
                return false;
            }

            if (ic.op == icode_op::LABEL || ic.op == icode_op::GOTO ||
                ic.op == icode_op::IFX) {
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
            if (mentions_temp(ic, temp_id) || mentions_temp(ic, old_temp))
                return false;
            const bool byte_immediate_compare =
                is_compare_op(ic.op) && ic.result.is_temp() &&
                ((ic.left.kind == operand_kind::INT_CONST &&
                  ic.right.type && ic.right.type->size() == 1) ||
                 (ic.right.kind == operand_kind::INT_CONST &&
                  ic.left.type && ic.left.type->size() == 1));
            if (byte_immediate_compare && k + 1 <= loop_end &&
                fn.icodes[k + 1].op == icode_op::IFX &&
                fn.icodes[k + 1].left.is_temp() &&
                fn.icodes[k + 1].left.temp_id == ic.result.temp_id) {
                continue;
            }
            if (bc_backend_hazard(ic, direct_ix_frame))
                return false;
        }

        if (!saw_mem_use)
            return false;

        end_out = loop_end;
        score_out = 1800 + iv.mentions * 16 - (loop_end - iv.first_def);
        return true;
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

    struct loop_accumulator_candidate {
        int tid = -1;
        int start = -1;
        int end = -1;
        int score = 0;
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
            init.left.kind != operand_kind::INT_CONST || init.left.ival != 0) {
            continue;
        }

        bool safe = true;
        bool saw_update = false;
        bool saw_backedge = false;
        int update_count = 0;
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

            const bool direct_add_update =
                ic.op == icode_op::ADD && ic.result.is_temp() &&
                ic.result.temp_id == tid && ic.left.is_temp() &&
                ic.left.temp_id == tid && !ic.right.is_none() &&
                ic.right.kind != operand_kind::INT_CONST &&
                !(ic.right.is_temp() && ic.right.temp_id == tid) &&
                op_size(ic.right) <= 2 &&
                !uses_tls_global(ic.right) &&
                !symbol_word_access_may_need_bc_scratch(ic.right);
            if (direct_add_update) {
                saw_update = true;
                ++update_count;
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
                     (!ic.result.is_temp() || ic.result.temp_id != tid));
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

            const bool safe_global_address =
                ic.op == icode_op::ADDRESS_OF && direct_ix_frame &&
                ic.result.is_temp() && ic.result.type &&
                ic.result.type->size() == 2 &&
                ((ic.left.kind == operand_kind::SYMBOL && ic.left.is_global &&
                  !ic.left.is_tls && !ic.left.is_sfr && !ic.left.is_func) ||
                 ic.left.kind == operand_kind::LABEL_REF);
            if (safe_global_address)
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
             3000 + update_count * 200 + iv.mentions * 16 -
                 (iv.last_use - iv.first_def)});
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
        pair_windows.push_back({cand.start, cand.end});
        break;
    }

    struct loop_induction_candidate {
        int tid = -1;
        int start = -1;
        int end = -1;
        int score = 0;
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
            if (ic.op == icode_op::CALL || ic.op == icode_op::ALLOCA ||
                ic.op == icode_op::INLINE_ASM || clobbers_bc(ic) ||
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
                safe = false;
                break;
            }

            switch (ic.op) {
            case icode_op::LABEL:
            case icode_op::GOTO:
            case icode_op::IFX:
            case icode_op::EQ:
            case icode_op::NE:
            case icode_op::LT:
            case icode_op::LE:
            case icode_op::GT:
            case icode_op::GE:
                continue;
            case icode_op::SET_VALUE_AT:
                if (ic.left.type && ic.left.type->size() == 1 &&
                    direct_ix_frame) {
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
             2200 + compare_uses * 160 + iv.mentions * 12 -
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
    for (auto &[fd, tid] : order) {
        const interval &iv = ivs[tid];
        int score = 0;
        if (!indirect_callee_bc_candidate(tid, iv, score))
            continue;
        bc_candidates.push_back(
            {iv.first_def, iv.last_use, score, false, tid});
    }

    // A loaded word used only by comparisons can remain in BC across branch
    // labels when all executable instructions in its live range preserve BC.
    // This is common in small ordering helpers and avoids materializing the
    // same scalar in an IX slot for each relational test.
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
                if (!is_compare_op(ic.op)) {
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
             1300 + iv.mentions * 12 - (iv.last_use - iv.first_def),
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
             14 + iv.mentions * 3 - (iv.last_use - iv.first_def),
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
             24 + iv.mentions * 3 - (iv.last_use - iv.first_def),
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
                     220 + iv.mentions * 10 + mask_uses * 8 -
                         (iv.last_use - iv.first_def)});
            }

            b_candidates.push_back(
                {tid, iv.first_def, iv.last_use,
                 180 + iv.mentions * 8 + compare_uses * 10 +
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
                     170 + iv.mentions * 10 + mask_uses * 6 -
                         (iv.last_use - iv.first_def)});
            }
            b_candidates.push_back(
                {tid, iv.first_def, iv.last_use,
                 150 + iv.mentions * 8 + compare_uses * 8 +
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
                overlaps_windows(b_windows, cand.start, cand.end) ||
                overlaps_windows(c_windows, cand.start, cand.end))
                continue;
            temp_regs_[cand.tid] = temp_home::main_c;
            c_windows.push_back({cand.start, cand.end});
        }
        for (const auto &cand : sym_c_candidates) {
            if (symbol_regs_.find(cand.key) != symbol_regs_.end())
                continue;
            if (overlaps_windows(pair_windows, cand.start, cand.end) ||
                overlaps_windows(b_windows, cand.start, cand.end) ||
                overlaps_windows(c_windows, cand.start, cand.end))
                continue;
            symbol_regs_[cand.key] = temp_home::main_c;
            c_windows.push_back({cand.start, cand.end});
        }
        for (const auto &cand : b_candidates) {
            if (overlaps_windows(pair_windows, cand.start, cand.end) ||
                overlaps_windows(c_windows, cand.start, cand.end) ||
                overlaps_windows(b_windows, cand.start, cand.end))
                continue;
            temp_regs_[cand.tid] = temp_home::main_b;
            b_windows.push_back({cand.start, cand.end});
        }
        for (const auto &cand : sym_b_candidates) {
            if (symbol_regs_.find(cand.key) != symbol_regs_.end())
                continue;
            if (overlaps_windows(pair_windows, cand.start, cand.end) ||
                overlaps_windows(c_windows, cand.start, cand.end) ||
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

    struct d_candidate {
        int tid = -1;
        int start = -1;
        int end = -1;
        int score = 0;
    };
    std::vector<d_candidate> d_candidates;
    if (size_opt_enabled() || tuned_profile_enabled()) {
        auto byte_or_immediate = [](const operand &op) {
            return op.is_none() || op.kind == operand_kind::INT_CONST ||
                   (op.type && op.type->size() == 1);
        };
        auto byte_op_preserves_de = [&](const icode &ic) {
            switch (ic.op) {
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
                return ic.result.type && ic.result.type->size() == 1 &&
                       byte_or_immediate(ic.left) &&
                       byte_or_immediate(ic.right);
            case icode_op::LABEL:
            case icode_op::GOTO:
            case icode_op::IFX:
                return true;
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
            case icode_op::LT:
            case icode_op::LE:
            case icode_op::GT:
            case icode_op::GE:
                return (use_left && !use_right &&
                        ic.right.kind == operand_kind::INT_CONST) ||
                       (use_right && !use_left &&
                        ic.left.kind == operand_kind::INT_CONST);
            default:
                return false;
            }
        };

        for (auto &[fd, tid] : order) {
            const interval &iv = ivs[tid];
            if (iv.size != 1 || iv.has_addr_of ||
                iv.first_def < 0 || iv.last_use <= iv.first_def ||
                iv.mentions < 3 ||
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
                    ic.op == icode_op::GET_VALUE_AT ||
                    ic.op == icode_op::SET_VALUE_AT ||
                    !d_temp_use_safe(ic, tid)) {
                    safe = false;
                    break;
                }
            }
            if (!safe)
                continue;
            d_candidates.push_back(
                {tid, iv.first_def, iv.last_use,
                 300 + iv.mentions * 12 - (iv.last_use - iv.first_def)});
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
        if (overlaps_windows(d_windows, cand.start, cand.end))
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
            case icode_op::LT:
            case icode_op::LE:
            case icode_op::GT:
            case icode_op::GE:
                return !use_result &&
                       ((use_left && ic.right.kind == operand_kind::INT_CONST) ||
                        (use_right && ic.left.kind == operand_kind::INT_CONST));
            case icode_op::RETURN:
                return use_left && !use_result && !use_right;
            default:
                return false;
            }
        };
        auto byte_op_preserves_e = [&](const icode &ic) {
            switch (ic.op) {
            case icode_op::ASSIGN:
            case icode_op::CAST:
                if (ic.result.type && ic.result.type->size() == 2 &&
                    ic.left.type && ic.left.type->size() == 1 &&
                    ic.right.is_none()) {
                    return true;
                }
                [[fallthrough]];
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
                return ic.result.type && ic.result.type->size() == 1 &&
                       byte_or_immediate(ic.left) &&
                       byte_or_immediate(ic.right);
            case icode_op::EQ:
            case icode_op::NE:
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
                iv.mentions < 4 ||
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
                 360 + iv.mentions * 14 - (iv.last_use - iv.first_def)});
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
        if (overlaps_windows(e_windows, cand.start, cand.end))
            continue;
        temp_regs_[cand.tid] = temp_home::main_e;
        e_windows.push_back({cand.start, cand.end});
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
    // One-step word temps in HL can hide backend address/scratch clobbers in
    // pointer-heavy heap/free-list code. Keep byte A homes, but spill word
    // temps until the HL proof models those hazards more precisely.
    const bool one_step_hl_enabled = false;
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
            if (alt_a_use_hazard(use_ic))          continue;
            if ((use_ic.op == icode_op::ASSIGN || use_ic.op == icode_op::CAST) &&
                use_ic.result.is_temp()) {
                continue;
            }
            if (use_ic.op == icode_op::ASSIGN &&
                use_ic.result.kind == operand_kind::SYMBOL) {
                continue;
            }

            if (one_step_a_enabled &&
                iv.size == 1 && alt_a_def_safe(def_ic) &&
                immediate_use_safe_in_a(use_ic, tid)) {
                temp_regs_[tid] = temp_home::main_a;
                continue;
            }
            if (one_step_hl_enabled &&
                iv.size == 2 && main_hl_def_safe(def_ic) &&
                immediate_use_safe_in_hl(use_ic, tid)) {
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

    // Keep arbitrary pointer rematerialization disabled until the alias and
    // liveness proof can distinguish loop-carried pointer values from safe
    // invariant address expressions. The narrower u8-index remat below is
    // still enabled and tested.
    const bool general_pointer_remat_enabled = false;

    if (general_pointer_remat_enabled) {
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

    // Word-load rematerialization needs a stronger memory-version proof before
    // it can be enabled across pointer-heavy kernels.
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

    // NOTE: BC'/DE'/HL' via EXX are not used — EXX swaps all three pairs
    // atomically and would corrupt DE/BC that the generator uses for operand
    // loads in the same window.  An EXX-block optimizer is a planned future pass.
}

} // namespace xcc
