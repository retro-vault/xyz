//
// iromod.cpp — module-level IR optimizer for xcc.
//
// This file implements a small translation-unit optimizer that can:
//
//   - remove internal functions that are never referenced
//   - inline tiny size-profitable static helpers in -Os
//
// The implementation is intentionally conservative. Helpers are only
// inlined when they are direct-call-only, address-not-taken, acyclic,
// and use only simple callee-local storage that can be remapped to caller
// temporaries. Multi-use helpers are only inlined when a simple byte-minded
// profitability rule says the removed SEND/CALL overhead beats the replicated
// body size.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//
#include "opt/iromod.h"
#include "opt/iropt.h"

#include "backend/z80/convention.h"
#include "frontend/types.h"
#include <algorithm>
#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace xcc {
namespace {

struct module_use_info {
    std::unordered_set<std::string> referenced_funcs;
    std::unordered_set<std::string> address_taken_funcs;
    std::unordered_map<std::string, int> direct_call_counts;
};

struct inline_candidate {
    std::unordered_map<std::string, int> param_argregs;
    std::unordered_map<int, int> temp_param_argregs;
    std::unordered_set<std::string> param_keys;
    std::unordered_set<std::string> mutable_param_keys;
    std::unordered_map<std::string, operand> param_symbols;
    std::unordered_set<int> mutable_param_temp_ids;
    std::unordered_map<int, operand> param_temps;
    std::unordered_map<std::string, operand> local_symbols;
    std::unordered_set<std::string> stack_local_keys;
    std::vector<icode> body;
    int op_count = 0;
    int approx_inline_cost = 0;
    int helper_definition_bonus = 0;
    int local_frame_bytes = 0;
    bool pure_leaf_arith = true;
    bool contains_call = false;
};

struct constant_arg_candidate {
    std::vector<operand> params;
    std::unordered_map<std::string, operand> replacements;
    std::unordered_map<int, operand> temp_replacements;
};

struct dead_param_candidate {
    struct kept_param {
        operand old_op;
        operand new_op;
        int old_index = -1;
        int new_index = -1;
        abi_arg_loc new_loc = abi_arg_loc::STACK;
    };

    std::vector<operand> old_params;
    std::vector<bool> keep_mask;
    std::vector<kept_param> kept;
    int new_num_params = 0;
    int new_local_bytes = 0;
    int new_stack_param_bytes = 0;
};

struct merge_candidate {
    std::string key;
    int op_count = 0;
};

static int next_temp_id(const ir_function &fn) {
    int next = 0;
    for (auto &ic : fn.icodes) {
        auto scan = [&](const operand &op) {
            if (op.is_temp()) next = std::max(next, op.temp_id + 1);
        };
        scan(ic.result);
        scan(ic.left);
        scan(ic.right);
    }
    return next;
}

static std::string symbol_key(const operand &op) {
    if (!op.is_symbol()) return {};
    return op.name + "|" +
           (op.is_global ? "g" : "l") + "|" +
           (op.is_param ? "p" : "v") + "|" +
           std::to_string(op.stack_offset) + "|" +
           std::to_string(op.byte_offset) + "|" +
           (op.is_tls ? "t" : "n") + "|" +
           (op.is_sfr ? "s" : "n") + "|" +
           (op.is_func ? "f" : "o");
}

static std::string base_symbol_key(const operand &op) {
    if (!op.is_symbol()) return {};
    operand base = op;
    base.byte_offset = 0;
    return symbol_key(base);
}

static bool cast_can_fold_to_int_const(const type_ptr &target) {
    if (!target)
        return true;
    return target->is_integer() ||
           target->kind == type_kind::POINTER ||
           target->kind == type_kind::ENUM;
}

static bool has_implicit_void_fallthrough_return(const ir_function &fn) {
    return fn.ret_type && fn.ret_type->kind == type_kind::VOID;
}

static void note_function_operand(const operand &op, module_use_info &info) {
    if (!op.is_symbol() || !op.is_func) return;
    info.referenced_funcs.insert(op.name);
    info.address_taken_funcs.insert(op.name);
}

static module_use_info build_module_use_info(const ir_module &mod) {
    module_use_info info;
    for (auto &fn : mod.functions) {
        for (auto &ic : fn.icodes) {
            if (ic.op == icode_op::CALL && !ic.func_name.empty()) {
                info.referenced_funcs.insert(ic.func_name);
                ++info.direct_call_counts[ic.func_name];
            }
            note_function_operand(ic.result, info);
            note_function_operand(ic.left, info);
            note_function_operand(ic.right, info);
        }
    }
    return info;
}

static const ir_function *find_function(const ir_module &mod,
                                        const std::string &name) {
    for (auto &fn : mod.functions)
        if (fn.name == name) return &fn;
    return nullptr;
}

static bool is_inlineable_op(icode_op op) {
    switch (op) {
    case icode_op::ASSIGN:
    case icode_op::ADDRESS_OF:
    case icode_op::GET_VALUE_AT:
    case icode_op::SET_VALUE_AT:
    case icode_op::ADD:  case icode_op::SUB:  case icode_op::NEG:
    case icode_op::MUL:  case icode_op::DIV:  case icode_op::MOD:
    case icode_op::BAND: case icode_op::BOR:  case icode_op::BXOR:
    case icode_op::BNOT:
    case icode_op::SHL:  case icode_op::SHR:
    case icode_op::ROL:  case icode_op::ROR:
    case icode_op::EQ:   case icode_op::NE:
    case icode_op::LT:   case icode_op::LE:
    case icode_op::GT:   case icode_op::GE:
    case icode_op::CAST:
    case icode_op::FADD: case icode_op::FSUB:
    case icode_op::FMUL: case icode_op::FDIV:
    case icode_op::FITOSF: case icode_op::FSTOI:
    case icode_op::MAKE_COMPLEX:
        return true;
    default:
        return false;
    }
}

static bool is_inline_candidate_op(icode_op op) {
    return is_inlineable_op(op) ||
           op == icode_op::SEND ||
           op == icode_op::CALL;
}

static bool is_inline_control_op(icode_op op) {
    switch (op) {
    case icode_op::LABEL:
    case icode_op::GOTO:
    case icode_op::IFX:
    case icode_op::RETURN:
        return true;
    default:
        return false;
    }
}

static bool is_allowed_symbol_read(const operand &op,
                                   const std::unordered_set<std::string> &param_keys) {
    return op.is_symbol() &&
           (op.is_global || param_keys.find(symbol_key(op)) != param_keys.end());
}

static bool is_allowed_symbol_write(const operand &op,
                                    const std::unordered_set<std::string> &param_keys) {
    return op.is_symbol() &&
           op.is_global &&
           param_keys.find(symbol_key(op)) == param_keys.end();
}

static bool is_inline_local_symbol(const operand &op) {
    return op.is_symbol() &&
           !op.is_global &&
           !op.is_param &&
           !op.is_func &&
           !op.is_tls &&
           !op.is_sfr;
}

static bool operand_allowed_in_candidate(
    const operand &op, bool is_write_pos,
    const std::unordered_set<std::string> &param_keys,
    std::unordered_map<std::string, operand> *local_symbols,
    std::unordered_set<std::string> *stack_local_keys)
{
    if (op.is_none() || op.is_temp() || op.is_const() || op.is_label())
        return true;
    if (!op.is_symbol())
        return false;
    const std::string param_key = base_symbol_key(op);
    if (param_keys.find(param_key) != param_keys.end()) {
        return is_write_pos ? op.byte_offset == 0 : true;
    }
    if (is_inline_local_symbol(op)) {
        if (local_symbols) {
            operand base = op;
            base.byte_offset = 0;
            local_symbols->try_emplace(base_symbol_key(base), base);
        }
        if (stack_local_keys && op.byte_offset != 0)
            stack_local_keys->insert(base_symbol_key(op));
        return true;
    }
    return is_write_pos ? is_allowed_symbol_write(op, param_keys)
                        : is_allowed_symbol_read(op, param_keys);
}

static bool analyze_inline_candidate(const ir_function &fn,
                                     inline_candidate &out,
                                     int max_ops) {
    if (fn.is_global || fn.is_noreturn)
        return false;
    if (fn.num_params > 6)
        return false;
    if (fn.icodes.size() < 3)
        return false;
    if (fn.icodes.front().op != icode_op::FUNCTION ||
        fn.icodes.back().op != icode_op::ENDFUNCTION) {
        return false;
    }

    bool saw_return = false;
    std::unordered_map<std::string, size_t> label_index;
    struct pending_branch {
        size_t body_index = 0;
        std::string target;
    };
    std::vector<pending_branch> branches;
    out = {};
    out.local_frame_bytes = fn.local_bytes;

    for (size_t i = 1; i + 1 < fn.icodes.size(); ++i) {
        const auto &ic = fn.icodes[i];

        if (ic.op == icode_op::RECEIVE) {
            if (ic.result.is_symbol()) {
                const std::string key = base_symbol_key(ic.result);
                out.param_argregs[key] = ic.argreg;
                out.param_keys.insert(key);
                out.param_symbols.emplace(key, ic.result);
            } else if (ic.result.is_temp()) {
                out.temp_param_argregs[ic.result.temp_id] = ic.argreg;
                out.param_temps.emplace(ic.result.temp_id, ic.result);
            } else {
                return false;
            }
            continue;
        }

        if (ic.op == icode_op::LABEL) {
            out.pure_leaf_arith = false;
            if (label_index.find(ic.label_name) != label_index.end())
                return false;
            label_index.emplace(ic.label_name, i);
            out.body.push_back(ic);
            continue;
        }

        if (ic.op == icode_op::GOTO) {
            out.pure_leaf_arith = false;
            if (ic.label_name.empty())
                return false;
            branches.push_back({i, ic.label_name});
            out.body.push_back(ic);
            ++out.op_count;
            ++out.approx_inline_cost;
            continue;
        }

        if (ic.op == icode_op::IFX) {
            out.pure_leaf_arith = false;
            if (!operand_allowed_in_candidate(ic.left, false, out.param_keys,
                                              &out.local_symbols,
                                              &out.stack_local_keys))
                return false;
            if (!ic.true_lbl.empty())
                branches.push_back({i, ic.true_lbl});
            if (!ic.false_lbl.empty())
                branches.push_back({i, ic.false_lbl});
            out.body.push_back(ic);
            ++out.op_count;
            ++out.approx_inline_cost;
            continue;
        }

        if (ic.op == icode_op::RETURN) {
            saw_return = true;
            if (!operand_allowed_in_candidate(ic.left, false, out.param_keys,
                                              &out.local_symbols,
                                              &out.stack_local_keys))
                return false;
            out.body.push_back(ic);
            ++out.approx_inline_cost;
            continue;
        }

        if (!is_inline_candidate_op(ic.op) && !is_inline_control_op(ic.op))
            return false;
        if (!operand_allowed_in_candidate(ic.result, true, out.param_keys,
                                          &out.local_symbols,
                                          &out.stack_local_keys))
            return false;
        if (!operand_allowed_in_candidate(ic.left, false, out.param_keys,
                                          &out.local_symbols,
                                          &out.stack_local_keys))
            return false;
        if (!operand_allowed_in_candidate(ic.right, false, out.param_keys,
                                          &out.local_symbols,
                                          &out.stack_local_keys))
            return false;

        // Per-function optimization may already have converted an operation
        // to a Z80 backend form whose narrow operands are interpreted at the
        // wider result width (u16*u16->u32 and u8-u8->int are examples).
        // That form is intentionally terminal.  Inlining it would feed it
        // through another language-level propagation/folding round, where
        // the narrower operand types can be mistaken for the operation
        // width.  Keep such helpers out of the module inliner; their compact
        // backend lowering still applies out of line.
        if (ic.result.type && ic.left.type &&
            ic.result.type->is_integer() && ic.left.type->is_integer() &&
            ic.result.type->size() > ic.left.type->size()) {
            switch (ic.op) {
            case icode_op::ADD:
            case icode_op::SUB:
            case icode_op::MUL:
            case icode_op::DIV:
            case icode_op::MOD:
            case icode_op::BAND:
            case icode_op::BOR:
            case icode_op::BXOR:
            case icode_op::SHL:
            case icode_op::SHR:
            case icode_op::ROL:
            case icode_op::ROR:
                return false;
            default:
                break;
            }
        }

        // Parameter locals are pass-by-value and must not be assigned to.
        if (ic.result.is_symbol() &&
            out.param_keys.find(base_symbol_key(ic.result)) != out.param_keys.end()) {
            if (ic.result.byte_offset != 0)
                return false;
            out.mutable_param_keys.insert(base_symbol_key(ic.result));
        }
        if (ic.result.is_temp() &&
            out.temp_param_argregs.find(ic.result.temp_id) !=
                out.temp_param_argregs.end()) {
            out.mutable_param_temp_ids.insert(ic.result.temp_id);
        }

        // Taking the address of a parameter would expose a callee-local object.
        if (ic.op == icode_op::ADDRESS_OF &&
            ic.left.is_symbol()) {
            const std::string key = base_symbol_key(ic.left);
            if (out.param_keys.find(key) != out.param_keys.end()) {
                return false;
            }
            if (is_inline_local_symbol(ic.left)) {
                out.stack_local_keys.insert(key);
            }
        }

        if (out.pure_leaf_arith) {
            switch (ic.op) {
            case icode_op::ASSIGN:
            case icode_op::ADD:  case icode_op::SUB:  case icode_op::NEG:
            case icode_op::MUL:  case icode_op::DIV:  case icode_op::MOD:
            case icode_op::BAND: case icode_op::BOR:  case icode_op::BXOR:
            case icode_op::BNOT:
            case icode_op::SHL:  case icode_op::SHR:
            case icode_op::ROL:  case icode_op::ROR:
            case icode_op::EQ:   case icode_op::NE:
            case icode_op::LT:   case icode_op::LE:
            case icode_op::GT:   case icode_op::GE:
            case icode_op::CAST:
                break;
            default:
                out.pure_leaf_arith = false;
                break;
            }

            auto pure_ok_operand = [&](const operand &op) {
                return op.is_none() || op.is_temp() || op.is_const() ||
                       (op.is_symbol() &&
                        out.param_keys.find(base_symbol_key(op)) !=
                            out.param_keys.end());
            };
            if (!pure_ok_operand(ic.result) ||
                !pure_ok_operand(ic.left) ||
                !pure_ok_operand(ic.right)) {
                out.pure_leaf_arith = false;
            }
        }

        if (ic.op == icode_op::CALL) {
            out.contains_call = true;
        }

        out.body.push_back(ic);
        ++out.op_count;
        ++out.approx_inline_cost;
    }

    if (!saw_return && !has_implicit_void_fallthrough_return(fn))
        return false;
    for (const auto &branch : branches) {
        if (label_index.find(branch.target) == label_index.end())
            return false;
    }
    if (out.pure_leaf_arith) {
        // Removing a tiny out-of-line arithmetic helper saves more than just
        // SEND/CALL sites: the helper prologue/epilogue and receive/spill
        // scaffolding disappear too. Use a conservative fixed bonus so
        // repeated tiny helpers can inline when it is still size-profitable
        // overall.
        out.helper_definition_bonus =
            out.approx_inline_cost + fn.num_params * 3 + 8;
    }
    return out.op_count <= max_ops;
}

static std::string encode_type(const type_ptr &type) {
    return type ? type->to_string() : "?";
}

struct merge_signature_state {
    std::unordered_map<std::string, int> param_ids;
    std::unordered_map<std::string, int> local_ids;
    std::unordered_map<std::string, int> label_ids;
    std::unordered_map<int, int> temp_ids;
    int next_temp = 0;
    int next_local = 0;
    int next_label = 0;
};

static int normalize_named_id(std::unordered_map<std::string, int> &ids,
                              const std::string &key,
                              int &next_id) {
    auto it = ids.find(key);
    if (it == ids.end())
        it = ids.emplace(key, next_id++).first;
    return it->second;
}

static int normalize_temp_id(std::unordered_map<int, int> &temp_ids,
                             int temp_id,
                             int &next_temp) {
    auto it = temp_ids.find(temp_id);
    if (it == temp_ids.end())
        it = temp_ids.emplace(temp_id, next_temp++).first;
    return it->second;
}

static std::string encode_normalized_label(const std::string &label,
                                           merge_signature_state &state) {
    if (label.empty())
        return "_";
    return "L" + std::to_string(
        normalize_named_id(state.label_ids, label, state.next_label));
}

static std::string encode_normalized_operand(const operand &op,
                                             merge_signature_state &state) {
    switch (op.kind) {
    case operand_kind::NONE:
        return "_";
    case operand_kind::TEMP: {
        return "T" + std::to_string(
            normalize_temp_id(state.temp_ids, op.temp_id, state.next_temp)) +
            ":" + encode_type(op.type);
    }
    case operand_kind::SYMBOL:
        if (op.is_global || op.is_func || op.is_tls || op.is_sfr) {
            return "S:" + op.name + ":" +
                   (op.is_global ? "g" : "l") + ":" +
                   (op.is_func ? "f" : "o") + ":" +
                   std::to_string(op.byte_offset) + ":" +
                   (op.is_tls ? "t" : "n") + ":" +
                   (op.is_sfr ? "s" : "n") + ":" +
                   encode_type(op.type);
        }
        if (auto it = state.param_ids.find(base_symbol_key(op));
            it != state.param_ids.end()) {
            return "P" + std::to_string(it->second) + ":" +
                   std::to_string(op.byte_offset) + ":" + encode_type(op.type);
        }
        return "L" + std::to_string(normalize_named_id(
                   state.local_ids, base_symbol_key(op), state.next_local)) +
               ":" + std::to_string(op.byte_offset) + ":" +
               encode_type(op.type);
    case operand_kind::INT_CONST:
        return "I:" + std::to_string(op.ival) + ":" + encode_type(op.type);
    case operand_kind::FLOAT_CONST:
        return "F:" + std::to_string(op.fval) + ":" + encode_type(op.type);
    case operand_kind::LABEL_REF:
        return "LBL:" + encode_normalized_label(op.name, state);
    }
    return "?";
}

static bool analyze_merge_candidate(const ir_function &fn, merge_candidate &out) {
    if (fn.is_global || fn.is_noreturn)
        return false;
    if (fn.icodes.size() < 2)
        return false;
    if (fn.icodes.front().op != icode_op::FUNCTION ||
        fn.icodes.back().op != icode_op::ENDFUNCTION) {
        return false;
    }

    merge_signature_state state;
    int op_count = 0;
    bool saw_return = false;
    std::unordered_map<std::string, size_t> label_index;
    struct pending_branch {
        size_t source_index = 0;
        std::string target;
    };
    std::vector<pending_branch> branches;

    std::string key;
    key.reserve(256);
    key += "abi=" + std::to_string(static_cast<int>(effective_call_abi(fn.abi)));
    key += ";ret=" + encode_type(fn.ret_type);
    key += ";params=" + std::to_string(fn.num_params);

    for (size_t i = 1; i + 1 < fn.icodes.size(); ++i) {
        const auto &ic = fn.icodes[i];

        if (ic.op == icode_op::RECEIVE) {
            if (!ic.result.is_symbol())
                return false;
            state.param_ids.emplace(base_symbol_key(ic.result), ic.argreg);
            key += ";recv" + std::to_string(ic.argreg) + "=" +
                   encode_type(ic.result.type) + "@" +
                   std::to_string(static_cast<int>(ic.arg_loc));
            continue;
        }

        if (ic.op == icode_op::LABEL) {
            if (label_index.find(ic.label_name) != label_index.end())
                return false;
            label_index.emplace(ic.label_name, i);
            key += ";label=" + encode_normalized_label(ic.label_name, state);
            continue;
        }

        if (ic.op == icode_op::GOTO) {
            if (ic.label_name.empty())
                return false;
            branches.push_back({i, ic.label_name});
            key += ";goto=" + encode_normalized_label(ic.label_name, state);
            ++op_count;
            continue;
        }

        if (ic.op == icode_op::IFX) {
            if (!ic.true_lbl.empty())
                branches.push_back({i, ic.true_lbl});
            if (!ic.false_lbl.empty())
                branches.push_back({i, ic.false_lbl});
            key += ";ifx|l=" + encode_normalized_operand(ic.left, state);
            key += "|x=" + encode_normalized_operand(ic.right, state);
            key += "|t=" + encode_normalized_label(ic.true_lbl, state);
            key += "|f=" + encode_normalized_label(ic.false_lbl, state);
            ++op_count;
            continue;
        }

        if (ic.op == icode_op::RETURN) {
            saw_return = true;
            key += ";retv=" + encode_normalized_operand(ic.left, state);
            continue;
        }

        if (!is_inlineable_op(ic.op))
            return false;

        key += ";op=";
        key += icode_op_name(ic.op);
        key += "|r=" + encode_normalized_operand(ic.result, state);
        key += "|l=" + encode_normalized_operand(ic.left, state);
        key += "|x=" + encode_normalized_operand(ic.right, state);
        ++op_count;
    }

    if (!saw_return && !has_implicit_void_fallthrough_return(fn))
        return false;
    for (const auto &branch : branches) {
        auto it = label_index.find(branch.target);
        if (it == label_index.end())
            return false;
        if (it->second < branch.source_index)
            return false;
    }
    if (op_count > 12)
        return false;

    out.key = std::move(key);
    out.op_count = op_count;
    return true;
}

static bool collect_call_args(const std::vector<icode> &icodes, size_t call_idx,
                              const icode &call_ic,
                              size_t &send_begin,
                              std::vector<operand> &args) {
    // Hidden aggregate-result pointers are deliberately outside the source
    // parameter numbering. Module transforms that rebuild SEND sequences
    // must leave such calls alone until they explicitly model that ABI slot.
    if (call_ic.result_via_sret)
        return false;

    if (call_ic.num_params == 0) {
        send_begin = call_idx;
        args.clear();
        return true;
    }
    if (call_idx < static_cast<size_t>(call_ic.num_params))
        return false;

    send_begin = call_idx;
    args.assign(static_cast<size_t>(call_ic.num_params), operand::make_none());

    for (int need = 0; need < call_ic.num_params; ++need) {
        size_t idx = call_idx - static_cast<size_t>(need) - 1;
        const auto &send_ic = icodes[idx];
        if (send_ic.op != icode_op::SEND)
            return false;
        if (send_ic.argreg < 0 || send_ic.argreg >= call_ic.num_params)
            return false;
        args[static_cast<size_t>(send_ic.argreg)] = send_ic.left;
        send_begin = idx;
    }

    for (auto &arg : args)
        if (arg.is_none())
            return false;
    return true;
}

static bool collect_param_operands(const ir_function &fn, std::vector<operand> &params) {
    params.assign(static_cast<size_t>(fn.num_params), operand::make_none());
    if (fn.num_params == 0)
        return true;

    for (auto &ic : fn.icodes) {
        if (ic.op != icode_op::RECEIVE)
            continue;
        if (ic.argreg < 0 || ic.argreg >= fn.num_params)
            return false;
        params[static_cast<size_t>(ic.argreg)] = ic.result;
    }

    for (auto &param : params)
        if (param.is_none())
            return false;
    return true;
}

static bool same_constant_value(const operand &lhs, const operand &rhs) {
    if (lhs.kind != rhs.kind)
        return false;
    if (lhs.kind == operand_kind::INT_CONST)
        return lhs.ival == rhs.ival;
    if (lhs.kind == operand_kind::FLOAT_CONST)
        return lhs.fval == rhs.fval;
    if (lhs.kind == operand_kind::LABEL_REF)
        return lhs.name == rhs.name;
    if (lhs.kind == operand_kind::SYMBOL && lhs.is_func && rhs.is_func)
        return lhs.name == rhs.name && lhs.byte_offset == rhs.byte_offset;
    return false;
}

static bool is_constant_argument_value(const operand &op) {
    return op.is_const() || op.kind == operand_kind::LABEL_REF ||
           (op.kind == operand_kind::SYMBOL && op.is_func &&
            op.byte_offset == 0);
}

static bool same_parameter_value(const operand &arg, const operand &param) {
    if (arg.kind != param.kind || arg.byte_offset != 0 ||
        param.byte_offset != 0) {
        return false;
    }
    if (arg.is_temp())
        return arg.temp_id == param.temp_id;
    if (arg.is_symbol())
        return base_symbol_key(arg) == base_symbol_key(param);
    return false;
}

static std::string const_eval_key(const operand &op) {
    if (op.is_temp())
        return "T:" + std::to_string(op.temp_id);
    return {};
}

enum class const_eval_value_kind : uint8_t {
    INT,
    ADDRESS,
};

struct const_eval_value {
    const_eval_value_kind kind = const_eval_value_kind::INT;
    int64_t int_value = 0;
    std::string address_key;
    int64_t address_offset = 0;
    type_ptr address_object_type;

    static const_eval_value make_int(int64_t value) {
        const_eval_value out;
        out.kind = const_eval_value_kind::INT;
        out.int_value = value;
        return out;
    }

    static const_eval_value make_address(std::string key,
                                         int64_t offset = 0,
                                         type_ptr object_type = {}) {
        const_eval_value out;
        out.kind = const_eval_value_kind::ADDRESS;
        out.address_key = std::move(key);
        out.address_offset = offset;
        out.address_object_type = std::move(object_type);
        return out;
    }

    bool is_int() const { return kind == const_eval_value_kind::INT; }
    bool is_address() const { return kind == const_eval_value_kind::ADDRESS; }
};

using const_eval_store = std::unordered_map<std::string, const_eval_value>;
using const_eval_memory = std::unordered_map<std::string, const_eval_value>;

static bool is_const_eval_memory_symbol(const operand &op) {
    return op.is_symbol() &&
           !op.is_global &&
           !op.is_func &&
           !op.is_tls &&
           !op.is_sfr;
}

static std::string memory_key_for_symbol(const operand &op,
                                         const std::string &frame_key) {
    if (!is_const_eval_memory_symbol(op))
        return {};
    operand base = op;
    base.byte_offset = 0;
    return frame_key + "|M:" + symbol_key(base);
}

static uint64_t integer_mask_for_type(const type_ptr &type);
static uint64_t unsigned_value_for_type(int64_t value, const type_ptr &type);
static int64_t normalize_integer_value(int64_t value, const type_ptr &type);

static uint64_t extract_integer_slice(uint64_t raw, int byte_offset,
                                      const type_ptr &type) {
    if (!type)
        return raw;
    const int shift = byte_offset * 8;
    if (shift > 0)
        raw >>= static_cast<unsigned>(shift);
    return raw & integer_mask_for_type(type);
}

static uint64_t insert_integer_slice(uint64_t base_raw,
                                     uint64_t slice_raw,
                                     int byte_offset,
                                     const type_ptr &type) {
    if (!type)
        return slice_raw;
    const uint64_t slice_mask = integer_mask_for_type(type);
    const int shift = byte_offset * 8;
    const uint64_t shifted_mask =
        shift >= 64 ? 0ULL : (slice_mask << static_cast<unsigned>(shift));
    const uint64_t shifted_value =
        shift >= 64 ? 0ULL : ((slice_raw & slice_mask) << static_cast<unsigned>(shift));
    return (base_raw & ~shifted_mask) | shifted_value;
}

static bool read_const_eval_memory_integer(const std::string &base_key,
                                           int byte_offset,
                                           const type_ptr &type,
                                           const const_eval_memory &memory,
                                           int64_t &out) {
    auto it = memory.find(base_key);
    if (it == memory.end() || !it->second.is_int())
        return false;
    uint64_t raw = static_cast<uint64_t>(it->second.int_value);
    raw = extract_integer_slice(raw, byte_offset, type);
    out = normalize_integer_value(static_cast<int64_t>(raw), type);
    return true;
}

static bool write_const_eval_memory_integer(const std::string &base_key,
                                            int byte_offset,
                                            const type_ptr &type,
                                            int64_t value,
                                            const const_eval_memory &memory,
                                            const_eval_memory &out_memory) {
    uint64_t base_raw = 0;
    auto it = memory.find(base_key);
    if (it != memory.end() && it->second.is_int())
        base_raw = static_cast<uint64_t>(it->second.int_value);
    const uint64_t slice_raw = unsigned_value_for_type(value, type);
    const uint64_t merged = insert_integer_slice(base_raw, slice_raw, byte_offset, type);
    out_memory[base_key] = const_eval_value::make_int(
        normalize_integer_value(static_cast<int64_t>(merged), type::make_ulong()));
    return true;
}

static uint64_t integer_mask_for_type(const type_ptr &type) {
    if (!type)
        return ~0ULL;
    const int bits = type->size() * 8;
    if (bits <= 0 || bits >= 64)
        return ~0ULL;
    return (1ULL << bits) - 1ULL;
}

static uint64_t unsigned_value_for_type(int64_t value, const type_ptr &type) {
    return static_cast<uint64_t>(value) & integer_mask_for_type(type);
}

static int64_t normalize_integer_value(int64_t value, const type_ptr &type) {
    if (!type)
        return value;
    if (type->kind == type_kind::BOOL)
        return value != 0 ? 1 : 0;

    const int bits = type->size() * 8;
    if (bits <= 0 || bits >= 64)
        return value;

    const uint64_t mask = integer_mask_for_type(type);
    uint64_t raw = static_cast<uint64_t>(value) & mask;
    if (type->is_unsigned())
        return static_cast<int64_t>(raw);

    const uint64_t sign_bit = 1ULL << (bits - 1);
    if ((raw & sign_bit) != 0)
        raw |= ~mask;
    return static_cast<int64_t>(raw);
}

static bool read_const_eval_value(const operand &op,
                                  const const_eval_store &values,
                                  const const_eval_memory &memory,
                                  const std::string &frame_key,
                                  const_eval_value &out) {
    if (op.is_none()) {
        out = const_eval_value::make_int(0);
        return true;
    }
    if (op.kind == operand_kind::INT_CONST) {
        out = const_eval_value::make_int(normalize_integer_value(op.ival, op.type));
        return true;
    }

    if (op.is_temp()) {
        auto it = values.find(const_eval_key(op));
        if (it == values.end())
            return false;
        out = it->second;
        return true;
    }

    if (!is_const_eval_memory_symbol(op))
        return false;

    const std::string key = memory_key_for_symbol(op, frame_key);
    auto it = memory.find(key);
    if (it == memory.end())
        return false;
    if (it->second.is_int()) {
        int64_t value = 0;
        if (!read_const_eval_memory_integer(key, op.byte_offset, op.type, memory, value))
            return false;
        out = const_eval_value::make_int(value);
        return true;
    }
    out = it->second;
    return true;
}

static bool read_const_eval_integer(const operand &op,
                                    const const_eval_store &values,
                                    const const_eval_memory &memory,
                                    const std::string &frame_key,
                                    int64_t &out) {
    const_eval_value value;
    if (!read_const_eval_value(op, values, memory, frame_key, value) ||
        !value.is_int()) {
        return false;
    }
    out = value.int_value;
    return true;
}

static bool store_const_eval_value(const operand &op,
                                   const const_eval_value &value,
                                   const std::string &frame_key,
                                   const_eval_store &values,
                                   const_eval_memory &memory) {
    if (op.is_none())
        return true;

    if (op.is_temp()) {
        values[const_eval_key(op)] = value;
        return true;
    }

    const std::string key = memory_key_for_symbol(op, frame_key);
    if (key.empty())
        return false;

    if (value.is_int()) {
        return write_const_eval_memory_integer(key, op.byte_offset, op.type,
                                               value.int_value, memory, memory);
    }

    if (op.byte_offset != 0)
        return false;
    memory[key] = value;
    return true;
}

static bool evaluate_const_binary(const icode &ic, int64_t lhs, int64_t rhs,
                                  int64_t &out) {
    const type_ptr value_type = ic.result.type ? ic.result.type :
                                (ic.left.type ? ic.left.type : ic.right.type);
    type_ptr eval_type;
    switch (ic.op) {
    case icode_op::SHL:
    case icode_op::SHR:
    case icode_op::ROL:
    case icode_op::ROR:
        eval_type = integer_promote(ic.left.type ? ic.left.type : value_type);
        break;
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
        if (ic.left.type && ic.right.type &&
            ic.left.type->is_integer() && ic.right.type->is_integer()) {
            eval_type = usual_arith_conv(ic.left.type, ic.right.type);
        }
        break;
    default:
        break;
    }
    if (!eval_type)
        eval_type = value_type;
    // Backend-form lowering may retain narrow unsigned source operands while
    // recording the actual arithmetic width in the result (notably
    // u16*u16->u32).  Honour that width when interpreting an optimized helper;
    // comparisons are the exception because their result is Boolean.
    const bool comparison =
        ic.op == icode_op::EQ || ic.op == icode_op::NE ||
        ic.op == icode_op::LT || ic.op == icode_op::LE ||
        ic.op == icode_op::GT || ic.op == icode_op::GE;
    if (!comparison && value_type &&
        (!eval_type || value_type->size() > eval_type->size())) {
        eval_type = value_type;
    }

    const bool use_unsigned = eval_type && eval_type->is_unsigned();
    lhs = normalize_integer_value(lhs, eval_type);
    rhs = normalize_integer_value(rhs, eval_type);
    const uint64_t ul = unsigned_value_for_type(lhs, eval_type);
    const uint64_t ur = unsigned_value_for_type(rhs, eval_type);

    switch (ic.op) {
    case icode_op::ADD:
        out = normalize_integer_value(static_cast<int64_t>(ul + ur), value_type);
        return true;
    case icode_op::SUB:
        out = normalize_integer_value(static_cast<int64_t>(ul - ur), value_type);
        return true;
    case icode_op::MUL:
        out = normalize_integer_value(static_cast<int64_t>(ul * ur), value_type);
        return true;
    case icode_op::DIV:
        if (rhs == 0) {
            out = 0;
            return true;
        }
        if (!use_unsigned &&
            lhs == std::numeric_limits<int64_t>::min() && rhs == -1) {
            out = normalize_integer_value(lhs, value_type);
            return true;
        }
        out = use_unsigned
            ? normalize_integer_value(static_cast<int64_t>(ul / ur), value_type)
            : normalize_integer_value(lhs / rhs, value_type);
        return true;
    case icode_op::MOD:
        if (rhs == 0) {
            out = 0;
            return true;
        }
        if (!use_unsigned &&
            lhs == std::numeric_limits<int64_t>::min() && rhs == -1) {
            out = 0;
            return true;
        }
        out = use_unsigned
            ? normalize_integer_value(static_cast<int64_t>(ul % ur), value_type)
            : normalize_integer_value(lhs % rhs, value_type);
        return true;
    case icode_op::BAND:
        out = normalize_integer_value(static_cast<int64_t>(ul & ur), value_type);
        return true;
    case icode_op::BOR:
        out = normalize_integer_value(static_cast<int64_t>(ul | ur), value_type);
        return true;
    case icode_op::BXOR:
        out = normalize_integer_value(static_cast<int64_t>(ul ^ ur), value_type);
        return true;
    case icode_op::SHL:
        out = normalize_integer_value(
            static_cast<int64_t>(ul << (static_cast<unsigned>(ur) & 63U)), value_type);
        return true;
    case icode_op::SHR:
        if (use_unsigned) {
            out = normalize_integer_value(
                static_cast<int64_t>(ul >> (static_cast<unsigned>(ur) & 63U)),
                value_type);
        } else {
            out = normalize_integer_value(lhs >> (static_cast<unsigned>(ur) & 63U),
                                          value_type);
        }
        return true;
    case icode_op::ROL: {
        const unsigned bits = value_type ? static_cast<unsigned>(value_type->size() * 8) : 16U;
        const unsigned sh = static_cast<unsigned>(ur) & (bits - 1U);
        const uint64_t mask = bits >= 64 ? ~0ULL : ((1ULL << bits) - 1ULL);
        out = normalize_integer_value(
            static_cast<int64_t>(((ul << sh) | (ul >> ((bits - sh) & (bits - 1U)))) & mask),
            value_type);
        return true;
    }
    case icode_op::ROR: {
        const unsigned bits = value_type ? static_cast<unsigned>(value_type->size() * 8) : 16U;
        const unsigned sh = static_cast<unsigned>(ur) & (bits - 1U);
        const uint64_t mask = bits >= 64 ? ~0ULL : ((1ULL << bits) - 1ULL);
        out = normalize_integer_value(
            static_cast<int64_t>(((ul >> sh) | (ul << ((bits - sh) & (bits - 1U)))) & mask),
            value_type);
        return true;
    }
    case icode_op::EQ:
        out = use_unsigned ? (ul == ur ? 1 : 0) : (lhs == rhs ? 1 : 0);
        return true;
    case icode_op::NE:
        out = use_unsigned ? (ul != ur ? 1 : 0) : (lhs != rhs ? 1 : 0);
        return true;
    case icode_op::LT:
        out = use_unsigned ? (ul < ur ? 1 : 0) : (lhs < rhs ? 1 : 0);
        return true;
    case icode_op::LE:
        out = use_unsigned ? (ul <= ur ? 1 : 0) : (lhs <= rhs ? 1 : 0);
        return true;
    case icode_op::GT:
        out = use_unsigned ? (ul > ur ? 1 : 0) : (lhs > rhs ? 1 : 0);
        return true;
    case icode_op::GE:
        out = use_unsigned ? (ul >= ur ? 1 : 0) : (lhs >= rhs ? 1 : 0);
        return true;
    default:
        return false;
    }
}

static bool is_supported_const_runtime_helper(const std::string &name) {
    return name == "_mul16" || name == "_div16" || name == "_mod16" ||
           name == "_mul32" || name == "_div32" || name == "_mod32" ||
           name.rfind("fixed8_8_", 0) == 0 ||
           name.rfind("fixed16_16_", 0) == 0 ||
           name.rfind("fixed24_8_", 0) == 0 ||
           name.rfind("_fixed8_8_", 0) == 0 ||
           name.rfind("_fixed16_16_", 0) == 0 ||
           name.rfind("_fixed24_8_", 0) == 0;
}

struct fixed_helper_info {
    enum class format {
        NONE,
        F8_8,
        F16_16,
        F24_8,
    };

    format fmt = format::NONE;
    std::string op;
};

static fixed_helper_info parse_fixed_helper(const std::string &name) {
    struct prefix_info {
        const char *prefix;
        fixed_helper_info::format fmt;
    };
    static const prefix_info prefixes[] = {
        {"fixed8_8_", fixed_helper_info::format::F8_8},
        {"fixed16_16_", fixed_helper_info::format::F16_16},
        {"fixed24_8_", fixed_helper_info::format::F24_8},
        {"_fixed8_8_", fixed_helper_info::format::F8_8},
        {"_fixed16_16_", fixed_helper_info::format::F16_16},
        {"_fixed24_8_", fixed_helper_info::format::F24_8},
    };

    for (const auto &entry : prefixes) {
        const std::string prefix(entry.prefix);
        if (name.rfind(prefix, 0) == 0) {
            fixed_helper_info info;
            info.fmt = entry.fmt;
            info.op = name.substr(prefix.size());
            return info;
        }
    }
    return {};
}

static int fixed_fraction_bits(fixed_helper_info::format fmt) {
    switch (fmt) {
    case fixed_helper_info::format::F8_8:
    case fixed_helper_info::format::F24_8:
        return 8;
    case fixed_helper_info::format::F16_16:
        return 16;
    default:
        return 0;
    }
}

static int fixed_raw_bits(fixed_helper_info::format fmt) {
    return fmt == fixed_helper_info::format::F8_8 ? 16 : 32;
}

static uint64_t fixed_mask_for_bits(int bits) {
    if (bits <= 0 || bits >= 64)
        return ~0ULL;
    return (1ULL << bits) - 1ULL;
}

static int64_t normalize_fixed_raw(int64_t value, int bits) {
    const uint64_t mask = fixed_mask_for_bits(bits);
    uint64_t raw = static_cast<uint64_t>(value) & mask;
    if (bits > 0 && bits < 64) {
        const uint64_t sign_bit = 1ULL << (bits - 1);
        if ((raw & sign_bit) != 0)
            raw |= ~mask;
    }
    return static_cast<int64_t>(raw);
}

static int64_t abs_fixed_raw(int64_t value, int bits) {
    value = normalize_fixed_raw(value, bits);
    return value < 0 ? -value : value;
}

static int64_t arithmetic_shift_right_value(int64_t value, int shift) {
    if (shift <= 0)
        return value;
    if (value >= 0)
        return value >> shift;
    const int64_t bias = (1LL << shift) - 1;
    return -(((-value) + bias) >> shift);
}

static int fixed_compare(int64_t lhs, int64_t rhs) {
    if (lhs < rhs)
        return -1;
    if (lhs > rhs)
        return 1;
    return 0;
}

static int64_t fixed_mul_result_raw(fixed_helper_info::format fmt,
                                    int64_t lhs,
                                    int64_t rhs) {
    const int raw_bits = fixed_raw_bits(fmt);
    const int frac_bits = fixed_fraction_bits(fmt);
    lhs = normalize_fixed_raw(lhs, raw_bits);
    rhs = normalize_fixed_raw(rhs, raw_bits);
    if (fmt == fixed_helper_info::format::F8_8) {
        const bool neg = (lhs < 0) != (rhs < 0);
        int64_t product = (abs_fixed_raw(lhs, raw_bits) *
                           abs_fixed_raw(rhs, raw_bits)) >> frac_bits;
        return neg ? -product : product;
    }
    return arithmetic_shift_right_value(lhs * rhs, frac_bits);
}

static int64_t fixed_div_result_raw(fixed_helper_info::format fmt,
                                    int64_t lhs,
                                    int64_t rhs) {
    const int raw_bits = fixed_raw_bits(fmt);
    const int frac_bits = fixed_fraction_bits(fmt);
    lhs = normalize_fixed_raw(lhs, raw_bits);
    rhs = normalize_fixed_raw(rhs, raw_bits);
    if (rhs == 0)
        return 0;
    const bool neg = (lhs < 0) != (rhs < 0);
    int64_t quotient =
        (abs_fixed_raw(lhs, raw_bits) << frac_bits) /
        abs_fixed_raw(rhs, raw_bits);
    return neg ? -quotient : quotient;
}

static bool evaluate_const_fixed_helper_call(const icode &call_ic,
                                             const std::vector<const_eval_value> &args,
                                             int64_t &ret_value,
                                             bool &has_ret_value) {
    const fixed_helper_info helper = parse_fixed_helper(call_ic.func_name);
    if (helper.fmt == fixed_helper_info::format::NONE)
        return false;
    for (const auto &arg : args)
        if (!arg.is_int())
            return false;

    const int raw_bits = fixed_raw_bits(helper.fmt);
    const int frac_bits = fixed_fraction_bits(helper.fmt);
    auto raw = [&](size_t index) {
        return normalize_fixed_raw(args[index].int_value, raw_bits);
    };
    auto finish_raw = [&](int64_t value) {
        ret_value = normalize_fixed_raw(value, raw_bits);
        has_ret_value = true;
        return true;
    };
    auto integer_divisor = [&](int divisor) {
        return finish_raw(fixed_div_result_raw(
            helper.fmt, raw(0), static_cast<int64_t>(divisor) << frac_bits));
    };
    auto raw_multiplier = [&](int64_t multiplier) {
        return finish_raw(fixed_mul_result_raw(helper.fmt, raw(0), multiplier));
    };

    if (helper.op == "from_int") {
        if (args.size() != 1)
            return false;
        return finish_raw(args[0].int_value << frac_bits);
    }
    if (helper.op == "to_int") {
        if (args.size() != 1)
            return false;
        ret_value = normalize_integer_value(
            arithmetic_shift_right_value(raw(0), frac_bits), type::make_int());
        has_ret_value = true;
        return true;
    }
    if (helper.op == "neg") {
        if (args.size() != 1)
            return false;
        return finish_raw(-raw(0));
    }
    if (helper.op == "abs") {
        if (args.size() != 1)
            return false;
        const int64_t value = raw(0);
        return finish_raw(value < 0 ? -value : value);
    }

    if (helper.op == "to_8_8") {
        if (args.size() != 1)
            return false;
        int64_t converted = 0;
        if (helper.fmt == fixed_helper_info::format::F16_16) {
            converted = arithmetic_shift_right_value(raw(0), 8);
        } else if (helper.fmt == fixed_helper_info::format::F24_8) {
            converted = raw(0);
        } else {
            return false;
        }
        ret_value = normalize_fixed_raw(converted, 16);
        has_ret_value = true;
        return true;
    }
    if (helper.op == "to_16_16") {
        if (args.size() != 1)
            return false;
        int64_t converted = 0;
        if (helper.fmt == fixed_helper_info::format::F8_8 ||
            helper.fmt == fixed_helper_info::format::F24_8) {
            converted = raw(0) << 8;
        } else {
            return false;
        }
        ret_value = normalize_fixed_raw(converted, 32);
        has_ret_value = true;
        return true;
    }
    if (helper.op == "to_24_8") {
        if (args.size() != 1)
            return false;
        int64_t converted = 0;
        if (helper.fmt == fixed_helper_info::format::F8_8) {
            converted = raw(0);
        } else if (helper.fmt == fixed_helper_info::format::F16_16) {
            converted = arithmetic_shift_right_value(raw(0), 8);
        } else {
            return false;
        }
        ret_value = normalize_fixed_raw(converted, 32);
        has_ret_value = true;
        return true;
    }

    if (args.size() == 1) {
        if (helper.op == "div2")
            return integer_divisor(2);
        if (helper.op == "div3")
            return integer_divisor(3);
        if (helper.op == "div4")
            return integer_divisor(4);
        if (helper.op == "div8")
            return integer_divisor(8);
        if (helper.op == "mul1_2")
            return raw_multiplier(1LL << (frac_bits - 1));
        if (helper.op == "mul1_4")
            return raw_multiplier(1LL << (frac_bits - 2));
        if (helper.op == "mul1_8")
            return raw_multiplier(1LL << (frac_bits - 3));
        if (helper.op == "mul3_2")
            return raw_multiplier(3LL << (frac_bits - 1));
        if (helper.op == "mul5_4")
            return raw_multiplier(5LL << (frac_bits - 2));
    }

    if (args.size() != 2)
        return false;

    const int64_t lhs = raw(0);
    const int64_t rhs = raw(1);
    if (helper.op == "add")
        return finish_raw(lhs + rhs);
    if (helper.op == "sub")
        return finish_raw(lhs - rhs);
    if (helper.op == "cmp") {
        ret_value = fixed_compare(lhs, rhs);
        has_ret_value = true;
        return true;
    }
    if (helper.op == "mul")
        return finish_raw(fixed_mul_result_raw(helper.fmt, lhs, rhs));
    if (helper.op == "div")
        return finish_raw(fixed_div_result_raw(helper.fmt, lhs, rhs));

    return false;
}

static bool fixed_div_specialized_target(const std::string &func_name,
                                         int64_t denominator,
                                         std::string &target) {
    const fixed_helper_info helper = parse_fixed_helper(func_name);
    if (helper.fmt == fixed_helper_info::format::NONE || helper.op != "div")
        return false;

    const int raw_bits = fixed_raw_bits(helper.fmt);
    const int frac_bits = fixed_fraction_bits(helper.fmt);
    const int64_t raw_denominator = normalize_fixed_raw(denominator, raw_bits);

    int divisor = 0;
    for (int candidate : {2, 3, 4, 8}) {
        if (raw_denominator == (static_cast<int64_t>(candidate) << frac_bits)) {
            divisor = candidate;
            break;
        }
    }
    if (divisor == 0)
        return false;

    switch (helper.fmt) {
    case fixed_helper_info::format::F8_8:
        target = "fixed8_8_div" + std::to_string(divisor);
        return true;
    case fixed_helper_info::format::F16_16:
        target = "fixed16_16_div" + std::to_string(divisor);
        return true;
    case fixed_helper_info::format::F24_8:
        target = "fixed24_8_div" + std::to_string(divisor);
        return true;
    default:
        return false;
    }
}

static bool fixed_mul_specialized_target(const std::string &func_name,
                                         int64_t multiplier,
                                         std::string &target) {
    const fixed_helper_info helper = parse_fixed_helper(func_name);
    if (helper.fmt == fixed_helper_info::format::NONE || helper.op != "mul")
        return false;

    const int raw_bits = fixed_raw_bits(helper.fmt);
    const int frac_bits = fixed_fraction_bits(helper.fmt);
    const int64_t raw_multiplier = normalize_fixed_raw(multiplier, raw_bits);

    auto make_target = [&](const std::string &suffix) {
        switch (helper.fmt) {
        case fixed_helper_info::format::F8_8:
            target = "fixed8_8_" + suffix;
            return true;
        case fixed_helper_info::format::F16_16:
            target = "fixed16_16_" + suffix;
            return true;
        case fixed_helper_info::format::F24_8:
            target = "fixed24_8_" + suffix;
            return true;
        default:
            return false;
        }
    };

    auto make_fraction_target = [&](const std::string &div_suffix,
                                    const std::string &mul_suffix) {
        if (helper.fmt == fixed_helper_info::format::F8_8)
            return make_target(div_suffix);
        return make_target(mul_suffix);
    };

    if (raw_multiplier == (1LL << (frac_bits - 1)))
        return make_fraction_target("div2", "mul1_2");
    if (raw_multiplier == (1LL << (frac_bits - 2)))
        return make_fraction_target("div4", "mul1_4");
    if (raw_multiplier == (1LL << (frac_bits - 3)))
        return make_fraction_target("div8", "mul1_8");
    if (raw_multiplier == (3LL << (frac_bits - 1)))
        return make_target("mul3_2");
    if (raw_multiplier == (5LL << (frac_bits - 2)))
        return make_target("mul5_4");
    return false;
}

static bool rewrite_fixed_unary_specialized_call(ir_function &caller,
                                                 size_t call_idx,
                                                 size_t send_begin,
                                                 const operand &arg,
                                                 const std::string &target) {
    const icode &call_ic = caller.icodes[call_idx];
    if (effective_call_abi(call_ic.callee_abi) != call_abi::SDCCCALL1)
        return false;

    type_ptr arg_type = arg.type ? arg.type :
                        (call_ic.result.type ? call_ic.result.type : type::make_int());
    const auto &conv = get_abi_convention(call_ic.callee_abi);
    std::vector<type_ptr> arg_types = {arg_type};
    auto arg_locs = conv.classify_args(arg_types);
    if (arg_locs.empty())
        return false;

    const int total_arg_bytes = conv.stack_arg_bytes(arg_type, arg_locs[0]);

    std::vector<icode> replacement;
    replacement.reserve(2);

    icode send;
    send.op = icode_op::SEND;
    send.left = arg;
    send.argreg = 0;
    send.arg_loc = arg_locs[0];
    send.send_bytes = total_arg_bytes;
    send.callee_abi = call_ic.callee_abi;
    send.line = call_ic.line;
    replacement.push_back(std::move(send));

    icode new_call = call_ic;
    new_call.func_name = target;
    new_call.num_params = 1;
    new_call.arg_bytes = total_arg_bytes;
    new_call.callee_cleans_stack =
        abi_callee_cleans_stack(new_call.callee_abi,
                                new_call.result.type,
                                arg_types,
                                false);
    replacement.push_back(std::move(new_call));

    auto erase_begin =
        caller.icodes.begin() + static_cast<std::ptrdiff_t>(send_begin);
    auto erase_end =
        caller.icodes.begin() + static_cast<std::ptrdiff_t>(call_idx + 1);
    caller.icodes.erase(erase_begin, erase_end);
    caller.icodes.insert(
        caller.icodes.begin() + static_cast<std::ptrdiff_t>(send_begin),
        replacement.begin(), replacement.end());
    return true;
}

static bool fixed_raw_binary_opcode(const std::string &func_name,
                                    icode_op &op) {
    const fixed_helper_info helper = parse_fixed_helper(func_name);
    if (helper.fmt == fixed_helper_info::format::NONE)
        return false;
    if (helper.op == "add") {
        op = icode_op::ADD;
        return true;
    }
    if (helper.op == "sub") {
        op = icode_op::SUB;
        return true;
    }
    return false;
}

static bool rewrite_fixed_raw_binary_call(ir_function &caller,
                                          size_t call_idx,
                                          size_t send_begin,
                                          const std::vector<operand> &args,
                                          icode_op op) {
    if (args.size() != 2)
        return false;

    const icode &call_ic = caller.icodes[call_idx];
    std::vector<icode> replacement;
    replacement.reserve(1);

    if (!call_ic.result.is_none()) {
        icode repl;
        repl.op = op;
        repl.result = call_ic.result;
        repl.left = args[0];
        repl.right = args[1];
        repl.line = call_ic.line;
        replacement.push_back(std::move(repl));
    }

    auto erase_begin =
        caller.icodes.begin() + static_cast<std::ptrdiff_t>(send_begin);
    auto erase_end =
        caller.icodes.begin() + static_cast<std::ptrdiff_t>(call_idx + 1);
    caller.icodes.erase(erase_begin, erase_end);
    caller.icodes.insert(
        caller.icodes.begin() + static_cast<std::ptrdiff_t>(send_begin),
        replacement.begin(), replacement.end());
    return true;
}

static bool evaluate_const_runtime_helper_call(const icode &call_ic,
                                               const std::vector<const_eval_value> &args,
                                               int64_t &ret_value,
                                               bool &has_ret_value) {
    if (evaluate_const_fixed_helper_call(call_ic, args, ret_value, has_ret_value))
        return true;

    if (!is_supported_const_runtime_helper(call_ic.func_name) || args.size() != 2)
        return false;
    if (!args[0].is_int() || !args[1].is_int())
        return false;

    const type_ptr value_type = call_ic.result.type ? call_ic.result.type :
                                (call_ic.left.type ? call_ic.left.type : call_ic.right.type);
    const bool use_unsigned =
        (value_type && value_type->is_unsigned());

    const int64_t lhs = normalize_integer_value(args[0].int_value, value_type);
    const int64_t rhs = normalize_integer_value(args[1].int_value, value_type);
    const uint64_t ul = unsigned_value_for_type(lhs, value_type);
    const uint64_t ur = unsigned_value_for_type(rhs, value_type);

    if (call_ic.func_name == "_mul16" || call_ic.func_name == "_mul32") {
        ret_value = normalize_integer_value(static_cast<int64_t>(ul * ur), value_type);
    } else if (call_ic.func_name == "_div16" || call_ic.func_name == "_div32") {
        if (rhs == 0) {
            ret_value = 0;
        } else if (use_unsigned) {
            ret_value = normalize_integer_value(static_cast<int64_t>(ul / ur), value_type);
        } else if (lhs == std::numeric_limits<int64_t>::min() && rhs == -1) {
            ret_value = normalize_integer_value(lhs, value_type);
        } else {
            ret_value = normalize_integer_value(lhs / rhs, value_type);
        }
    } else if (call_ic.func_name == "_mod16" || call_ic.func_name == "_mod32") {
        if (rhs == 0) {
            ret_value = 0;
        } else if (use_unsigned) {
            ret_value = normalize_integer_value(static_cast<int64_t>(ul % ur), value_type);
        } else if (lhs == std::numeric_limits<int64_t>::min() && rhs == -1) {
            ret_value = 0;
        } else {
            ret_value = normalize_integer_value(lhs % rhs, value_type);
        }
    } else {
        return false;
    }

    has_ret_value = true;
    return true;
}

static bool is_const_eval_integer_type(const type_ptr &type) {
    return !type || (type->is_integer() && type->size() <= 4);
}

static bool is_const_eval_supported_type(const type_ptr &type) {
    return !type || is_const_eval_integer_type(type) || type->is_ptr();
}

static bool is_const_eval_reinterpretable_object_type(const type_ptr &type) {
    if (!type)
        return false;
    type_ptr base = type->unqual();
    return base &&
           is_const_eval_supported_type(base) &&
           base->size() > 0 &&
           base->size() <= 4;
}

static bool same_const_eval_object_type(const type_ptr &lhs,
                                        const type_ptr &rhs) {
    if (!lhs || !rhs)
        return false;
    type_ptr a = lhs->unqual();
    type_ptr b = rhs->unqual();
    if (!a || !b)
        return false;
    if (a->kind != b->kind)
        return false;
    if (a->kind == type_kind::POINTER)
        return same_const_eval_object_type(a->base, b->base);
    return a->size() == b->size() && a->is_unsigned() == b->is_unsigned();
}

static bool is_safe_const_eval_pointer_cast(const icode &ic) {
    if (ic.op != icode_op::CAST || !ic.left.type || !ic.result.type)
        return true;
    if (!ic.left.type->is_ptr() || !ic.result.type->is_ptr())
        return true;

    type_ptr from = ic.left.type->base ? ic.left.type->base->unqual() : nullptr;
    type_ptr to   = ic.result.type->base ? ic.result.type->base->unqual() : nullptr;
    if (!from || !to)
        return true;
    if (from->kind == type_kind::VOID || to->kind == type_kind::VOID)
        return true;
    return same_const_eval_object_type(from, to) ||
           (is_const_eval_reinterpretable_object_type(from) &&
            is_const_eval_reinterpretable_object_type(to));
}

static bool is_const_eval_address_access_compatible(const const_eval_value &addr,
                                                    const type_ptr &access_type) {
    if (!addr.is_address())
        return false;
    if (!access_type)
        return true;
    if (!is_const_eval_reinterpretable_object_type(access_type))
        return false;
    if (!addr.address_object_type)
        return false;

    type_ptr object_type = addr.address_object_type->unqual();
    type_ptr value_type = access_type->unqual();
    if (!object_type || !value_type)
        return false;

    if (addr.address_offset < 0)
        return false;

    const int64_t offset = addr.address_offset;
    const int64_t access_size = value_type->size();
    const int64_t object_size = object_type->size();
    if (access_size < 0 || object_size < 0)
        return false;
    return offset + access_size <= object_size;
}

static bool is_const_eval_nonlocal_storage_symbol(const operand &op) {
    return op.is_symbol() &&
           !op.is_func &&
           (op.is_global || op.is_tls || op.is_sfr);
}

static bool can_const_eval_whole_function(const ir_function &fn) {
    if (fn.ret_type &&
        fn.ret_type->kind != type_kind::VOID &&
        !is_const_eval_integer_type(fn.ret_type)) {
        return false;
    }

    for (const auto &ic : fn.icodes) {
        if (!is_const_eval_supported_type(ic.result.type) ||
            !is_const_eval_supported_type(ic.left.type) ||
            !is_const_eval_supported_type(ic.right.type)) {
            return false;
        }
        if (!is_safe_const_eval_pointer_cast(ic))
            return false;
        if (is_const_eval_nonlocal_storage_symbol(ic.result) ||
            is_const_eval_nonlocal_storage_symbol(ic.left) ||
            is_const_eval_nonlocal_storage_symbol(ic.right)) {
            return false;
        }
        switch (ic.op) {
        case icode_op::ADDRESS_OF:
        case icode_op::GET_VALUE_AT:
        case icode_op::SET_VALUE_AT:
            // The current evaluator is scalar-memory oriented. It is safe and
            // useful for pure integer control/data flow, but not yet robust
            // enough for general object-backed memory simulation.
            return false;
        default:
            break;
        }
    }

    return true;
}

static bool op_writes_result_symbol(icode_op op) {
    switch (op) {
    case icode_op::RECEIVE:
    case icode_op::CALL:
    case icode_op::ASSIGN:
    case icode_op::ADDRESS_OF:
    case icode_op::GET_VALUE_AT:
    case icode_op::ADD:  case icode_op::SUB:  case icode_op::NEG:
    case icode_op::MUL:  case icode_op::DIV:  case icode_op::MOD:
    case icode_op::BAND: case icode_op::BOR:  case icode_op::BXOR: case icode_op::BNOT:
    case icode_op::SHL:  case icode_op::SHR:
    case icode_op::ROL:  case icode_op::ROR:
    case icode_op::EQ:   case icode_op::NE:
    case icode_op::LT:   case icode_op::LE:
    case icode_op::GT:   case icode_op::GE:
    case icode_op::CAST:
    case icode_op::FADD: case icode_op::FSUB:
    case icode_op::FMUL: case icode_op::FDIV:
    case icode_op::FITOSF: case icode_op::FSTOI:
    case icode_op::ALLOCA:
    case icode_op::MAKE_COMPLEX:
        return true;
    default:
        return false;
    }
}

static bool replace_symbol_read_with_constant(
    operand &op, const std::unordered_map<std::string, operand> &replacements)
{
    if (!op.is_symbol())
        return false;
    auto it = replacements.find(base_symbol_key(op));
    if (it == replacements.end())
        return false;

    operand repl = it->second;
    repl.type = op.type ? op.type : repl.type;
    op = std::move(repl);
    return true;
}

static bool replace_temp_read_with_constant(
    operand &op, const std::unordered_map<int, operand> &replacements)
{
    if (!op.is_temp())
        return false;
    auto it = replacements.find(op.temp_id);
    if (it == replacements.end())
        return false;

    operand repl = it->second;
    repl.type = op.type ? op.type : repl.type;
    op = std::move(repl);
    return true;
}

static bool apply_constant_arg_replacements(
    ir_function &fn,
    const std::unordered_map<std::string, operand> &replacements,
    const std::unordered_map<int, operand> &temp_replacements = {})
{
    bool changed = false;
    auto replace_read = [&](operand &op) {
        return replace_symbol_read_with_constant(op, replacements) ||
               replace_temp_read_with_constant(op, temp_replacements);
    };

    for (auto &ic : fn.icodes) {
        switch (ic.op) {
        case icode_op::IFX:
        case icode_op::RETURN:
        case icode_op::SEND:
            changed = replace_read(ic.left) || changed;
            break;

        case icode_op::CALL:
            if (ic.func_name.empty()) {
                changed = replace_read(ic.left) || changed;
                if (ic.left.kind == operand_kind::SYMBOL && ic.left.is_func &&
                    ic.left.byte_offset == 0) {
                    ic.func_name = ic.left.name;
                    ic.left = operand::make_none();
                    changed = true;
                }
            }
            break;

        case icode_op::SET_VALUE_AT:
            changed = replace_read(ic.result) || changed;
            changed = replace_read(ic.left) || changed;
            break;

        case icode_op::FUNCTION:
        case icode_op::ENDFUNCTION:
        case icode_op::LABEL:
        case icode_op::GOTO:
        case icode_op::RECEIVE:
        case icode_op::INLINE_ASM:
            break;

        default:
            changed = replace_read(ic.left) || changed;
            changed = replace_read(ic.right) || changed;
            break;
        }
    }

    return changed;
}

static bool uses_param_symbol(const icode &ic, const std::string &param_key) {
    auto matches = [&](const operand &op) {
        return op.is_symbol() && base_symbol_key(op) == param_key;
    };

    switch (ic.op) {
    case icode_op::FUNCTION:
    case icode_op::ENDFUNCTION:
    case icode_op::LABEL:
    case icode_op::GOTO:
        return false;

    case icode_op::RECEIVE:
        return false;

    case icode_op::RETURN:
    case icode_op::IFX:
    case icode_op::SEND:
        return matches(ic.left);

    case icode_op::CALL:
        return ic.func_name.empty() && matches(ic.left);

    case icode_op::SET_VALUE_AT:
        return matches(ic.result) || matches(ic.left);

    default:
        return matches(ic.result) || matches(ic.left) || matches(ic.right);
    }
}

static bool remap_param_symbol(
    operand &op,
    const std::unordered_map<std::string, operand> &param_map)
{
    if (!op.is_symbol())
        return false;
    auto it = param_map.find(base_symbol_key(op));
    if (it == param_map.end())
        return false;

    operand remapped = it->second;
    remapped.type = op.type ? op.type : remapped.type;
    remapped.byte_offset += op.byte_offset;
    op = std::move(remapped);
    return true;
}

static bool analyze_dead_param_candidate(
    const ir_function &fn, dead_param_candidate &out)
{
    if (fn.is_global || fn.num_params == 0)
        return false;
    if (!collect_param_operands(fn, out.old_params))
        return false;

    out.keep_mask.assign(out.old_params.size(), true);
    bool any_removed = false;

    for (size_t i = 0; i < out.old_params.size(); ++i) {
        const std::string key = base_symbol_key(out.old_params[i]);
        bool used = false;
        for (auto &ic : fn.icodes) {
            if (uses_param_symbol(ic, key)) {
                used = true;
                break;
            }
        }
        if (!used) {
            out.keep_mask[i] = false;
            any_removed = true;
        }
    }

    if (!any_removed)
        return false;

    const auto &conv = get_abi_convention(fn.abi);
    std::vector<type_ptr> kept_types;
    kept_types.reserve(out.old_params.size());
    for (size_t i = 0; i < out.old_params.size(); ++i) {
        if (out.keep_mask[i])
            kept_types.push_back(out.old_params[i].type ? out.old_params[i].type
                                                        : type::make_int());
    }

    auto kept_locs = conv.classify_args(kept_types);
    int spill_bytes = 0;
    int stack_bytes = 0;
    int new_index = 0;

    for (size_t i = 0; i < out.old_params.size(); ++i) {
        if (!out.keep_mask[i])
            continue;

        const auto &old_op = out.old_params[i];
        operand new_op = old_op;
        abi_arg_loc loc = kept_locs[static_cast<size_t>(new_index)];

        if (loc == abi_arg_loc::STACK) {
            new_op.stack_offset = stack_bytes;
            new_op.is_param = true;
            stack_bytes += conv.stack_arg_bytes(new_op.type, loc);
        } else {
            spill_bytes += conv.spill_bytes(new_op.type, loc);
            new_op.stack_offset = -(fn.orig_local_bytes + spill_bytes);
            new_op.is_param = false;
        }

        out.kept.push_back(
            {old_op, new_op, static_cast<int>(i), new_index, loc});
        ++new_index;
    }

    out.new_num_params = static_cast<int>(out.kept.size());
    out.new_local_bytes = fn.orig_local_bytes + spill_bytes;
    out.new_stack_param_bytes = stack_bytes;
    return true;
}

static bool rewrite_dead_params_in_function(
    ir_function &fn, const dead_param_candidate &candidate)
{
    std::unordered_map<std::string, operand> param_map;
    std::unordered_set<std::string> dropped_keys;

    for (size_t i = 0; i < candidate.old_params.size(); ++i) {
        const std::string key = base_symbol_key(candidate.old_params[i]);
        if (!candidate.keep_mask[i])
            dropped_keys.insert(key);
    }
    for (auto &kept : candidate.kept)
        param_map.emplace(base_symbol_key(kept.old_op), kept.new_op);

    std::vector<icode> rewritten;
    rewritten.reserve(fn.icodes.size());

    for (auto ic : fn.icodes) {
        if (ic.op == icode_op::FUNCTION) {
            ic.num_params = candidate.new_num_params;
            ic.local_bytes = candidate.new_local_bytes;
            rewritten.push_back(std::move(ic));
            continue;
        }

        if (ic.op == icode_op::RECEIVE) {
            const std::string key = base_symbol_key(ic.result);
            if (dropped_keys.find(key) != dropped_keys.end())
                continue;

            auto kept_it = std::find_if(candidate.kept.begin(), candidate.kept.end(),
                [&](const dead_param_candidate::kept_param &kept) {
                    return base_symbol_key(kept.old_op) == key;
                });
            if (kept_it == candidate.kept.end())
                return false;

            ic.result = kept_it->new_op;
            ic.argreg = kept_it->new_index;
            ic.arg_loc = kept_it->new_loc;
            rewritten.push_back(std::move(ic));
            continue;
        }

        remap_param_symbol(ic.result, param_map);
        remap_param_symbol(ic.left, param_map);
        remap_param_symbol(ic.right, param_map);
        rewritten.push_back(std::move(ic));
    }

    fn.icodes = std::move(rewritten);
    fn.num_params = candidate.new_num_params;
    fn.local_bytes = candidate.new_local_bytes;
    fn.stack_param_bytes = candidate.new_stack_param_bytes;
    std::vector<type_ptr> kept_types;
    kept_types.reserve(candidate.kept.size());
    for (const auto &kept : candidate.kept)
        kept_types.push_back(kept.new_op.type ? kept.new_op.type : type::make_int());
    fn.callee_cleans_stack =
        abi_callee_cleans_stack(fn.abi, fn.ret_type, kept_types, fn.is_variadic);
    return true;
}

static bool rewrite_dead_params_in_calls(
    ir_module &mod, const ir_function &callee, const dead_param_candidate &candidate)
{
    const auto &conv = get_abi_convention(callee.abi);

    std::vector<type_ptr> kept_types;
    kept_types.reserve(candidate.kept.size());
    for (auto &kept : candidate.kept)
        kept_types.push_back(kept.new_op.type ? kept.new_op.type : type::make_int());
    auto kept_locs = conv.classify_args(kept_types);

    for (auto &caller : mod.functions) {
        for (size_t i = 0; i < caller.icodes.size(); ++i) {
            const auto &ic = caller.icodes[i];
            if (ic.op != icode_op::CALL || ic.func_name != callee.name)
                continue;

            size_t send_begin = i;
            std::vector<operand> args;
            if (!collect_call_args(caller.icodes, i, ic, send_begin, args))
                return false;

            std::vector<operand> kept_args;
            kept_args.reserve(candidate.kept.size());
            for (auto &kept : candidate.kept)
                kept_args.push_back(args[static_cast<size_t>(kept.old_index)]);

            int total_arg_bytes = 0;
            std::vector<icode> replacement;
            replacement.reserve(kept_args.size() + 1);

            for (int arg_i = static_cast<int>(kept_args.size()) - 1; arg_i >= 0; --arg_i) {
                const int stack_bytes = conv.stack_arg_bytes(
                    kept_types[static_cast<size_t>(arg_i)],
                    kept_locs[static_cast<size_t>(arg_i)]);
                total_arg_bytes += stack_bytes;
                icode send;
                send.op = icode_op::SEND;
                send.left = kept_args[static_cast<size_t>(arg_i)];
                send.argreg = arg_i;
                send.arg_loc = kept_locs[static_cast<size_t>(arg_i)];
                send.send_bytes = stack_bytes;
                send.callee_abi = callee.abi;
                send.line = ic.line;
                replacement.push_back(std::move(send));
            }

            icode new_call = ic;
            new_call.num_params = static_cast<int>(kept_args.size());
            new_call.arg_bytes = total_arg_bytes;
            new_call.callee_cleans_stack =
                abi_callee_cleans_stack(callee.abi, callee.ret_type, kept_types,
                                        callee.is_variadic);
            replacement.push_back(std::move(new_call));

            auto erase_begin =
                caller.icodes.begin() + static_cast<std::ptrdiff_t>(send_begin);
            auto erase_end =
                caller.icodes.begin() + static_cast<std::ptrdiff_t>(i + 1);
            caller.icodes.erase(erase_begin, erase_end);
            caller.icodes.insert(
                caller.icodes.begin() + static_cast<std::ptrdiff_t>(send_begin),
                replacement.begin(), replacement.end());
            i = send_begin + replacement.size() - 1;
        }
    }

    return true;
}

struct internal_abi_call_site {
    size_t caller_idx = 0;
    size_t call_idx = 0;
    size_t send_begin = 0;
    std::vector<operand> args;
};

static bool collect_internal_abi_call_sites(
    const ir_module &mod, const ir_function &callee,
    std::vector<internal_abi_call_site> &sites)
{
    for (size_t caller_idx = 0; caller_idx < mod.functions.size();
         ++caller_idx) {
        const auto &caller = mod.functions[caller_idx];
        for (size_t call_idx = 0; call_idx < caller.icodes.size();
             ++call_idx) {
            const auto &call = caller.icodes[call_idx];
            if (call.op != icode_op::CALL ||
                call.func_name != callee.name) {
                continue;
            }
            if (effective_call_abi(call.callee_abi) !=
                call_abi::SDCCCALL0) {
                return false;
            }

            internal_abi_call_site site;
            site.caller_idx = caller_idx;
            site.call_idx = call_idx;
            if (!collect_call_args(caller.icodes, call_idx, call,
                                   site.send_begin, site.args) ||
                static_cast<int>(site.args.size()) != callee.num_params) {
                return false;
            }
            sites.push_back(std::move(site));
        }
    }
    return !sites.empty();
}

static std::vector<int> abi_send_order(const abi_convention &conv,
                                       int num_params)
{
    std::vector<int> order;
    order.reserve(static_cast<size_t>(num_params));
    if (conv.caller_sends_right_to_left()) {
        for (int i = num_params - 1; i >= 0; --i)
            order.push_back(i);
    } else {
        for (int i = 0; i < num_params; ++i)
            order.push_back(i);
    }
    return order;
}

static bool promote_internal_calls_to_sdcccall1(
    ir_module &mod, const ir_function &callee,
    const std::vector<type_ptr> &param_types,
    const std::vector<abi_arg_loc> &param_locs,
    std::vector<internal_abi_call_site> sites)
{
    auto &conv = get_abi_convention(call_abi::SDCCCALL1);
    const auto order = abi_send_order(conv, callee.num_params);

    std::sort(sites.begin(), sites.end(),
              [](const internal_abi_call_site &lhs,
                 const internal_abi_call_site &rhs) {
                  if (lhs.caller_idx != rhs.caller_idx)
                      return lhs.caller_idx > rhs.caller_idx;
                  return lhs.call_idx > rhs.call_idx;
              });

    for (const auto &site : sites) {
        auto &caller = mod.functions[site.caller_idx];
        const icode old_call = caller.icodes[site.call_idx];
        std::vector<icode> replacement;
        replacement.reserve(site.args.size() + 1);
        int total_arg_bytes = 0;

        for (int arg_i : order) {
            const int stack_bytes = conv.stack_arg_bytes(
                param_types[static_cast<size_t>(arg_i)],
                param_locs[static_cast<size_t>(arg_i)]);
            total_arg_bytes += stack_bytes;

            icode send;
            send.op = icode_op::SEND;
            send.left = site.args[static_cast<size_t>(arg_i)];
            send.argreg = arg_i;
            send.arg_loc = param_locs[static_cast<size_t>(arg_i)];
            send.send_bytes = stack_bytes;
            send.callee_abi = call_abi::SDCCCALL1;
            send.line = old_call.line;
            replacement.push_back(std::move(send));
        }

        icode new_call = old_call;
        new_call.arg_bytes = total_arg_bytes;
        new_call.callee_abi = call_abi::SDCCCALL1;
        new_call.callee_cleans_stack =
            abi_callee_cleans_stack(call_abi::SDCCCALL1, callee.ret_type,
                                    param_types, false);
        replacement.push_back(std::move(new_call));

        auto erase_begin =
            caller.icodes.begin() +
            static_cast<std::ptrdiff_t>(site.send_begin);
        auto erase_end =
            caller.icodes.begin() +
            static_cast<std::ptrdiff_t>(site.call_idx + 1);
        caller.icodes.erase(erase_begin, erase_end);
        caller.icodes.insert(
            caller.icodes.begin() +
                static_cast<std::ptrdiff_t>(site.send_begin),
            replacement.begin(), replacement.end());
    }
    return true;
}

static bool promote_internal_function_to_sdcccall1(
    ir_function &fn, const std::vector<operand> &old_params,
    const std::vector<type_ptr> &param_types,
    const std::vector<abi_arg_loc> &param_locs)
{
    auto &conv = get_abi_convention(call_abi::SDCCCALL1);
    std::vector<operand> new_params = old_params;
    int spill_bytes = 0;
    int stack_bytes = 0;

    for (size_t i = 0; i < old_params.size(); ++i) {
        if (!old_params[i].is_symbol())
            return false;
        if (param_locs[i] == abi_arg_loc::STACK)
            continue;

        spill_bytes += conv.spill_bytes(param_types[i], param_locs[i]);
        new_params[i].stack_offset =
            -(fn.orig_local_bytes + spill_bytes);
        new_params[i].is_param = false;
    }

    const auto send_order = abi_send_order(conv, fn.num_params);
    for (auto it = send_order.rbegin(); it != send_order.rend(); ++it) {
        const int arg_i = *it;
        if (param_locs[static_cast<size_t>(arg_i)] !=
            abi_arg_loc::STACK) {
            continue;
        }
        new_params[static_cast<size_t>(arg_i)].stack_offset = stack_bytes;
        new_params[static_cast<size_t>(arg_i)].is_param = true;
        stack_bytes += conv.stack_arg_bytes(
            param_types[static_cast<size_t>(arg_i)],
            param_locs[static_cast<size_t>(arg_i)]);
    }

    std::unordered_map<std::string, operand> param_map;
    for (size_t i = 0; i < old_params.size(); ++i)
        param_map.emplace(base_symbol_key(old_params[i]), new_params[i]);

    for (auto &ic : fn.icodes) {
        if (ic.op == icode_op::FUNCTION) {
            ic.local_bytes = fn.orig_local_bytes + spill_bytes;
            continue;
        }
        if (ic.op == icode_op::RECEIVE) {
            if (ic.argreg < 0 || ic.argreg >= fn.num_params)
                return false;
            ic.result = new_params[static_cast<size_t>(ic.argreg)];
            ic.arg_loc = param_locs[static_cast<size_t>(ic.argreg)];
            continue;
        }
        remap_param_symbol(ic.result, param_map);
        remap_param_symbol(ic.left, param_map);
        remap_param_symbol(ic.right, param_map);
    }

    fn.abi = call_abi::SDCCCALL1;
    fn.local_bytes = fn.orig_local_bytes + spill_bytes;
    fn.stack_param_bytes = stack_bytes;
    fn.callee_cleans_stack =
        abi_callee_cleans_stack(fn.abi, fn.ret_type, param_types, false);
    fn.can_internalize_abi = false;
    return true;
}

static bool analyze_constant_arg_candidate(
    const ir_module &mod, const ir_function &fn, const module_use_info &info,
    constant_arg_candidate &out)
{
    if (fn.is_global || fn.num_params == 0)
        return false;
    if (info.address_taken_funcs.find(fn.name) != info.address_taken_funcs.end())
        return false;

    auto call_count_it = info.direct_call_counts.find(fn.name);
    if (call_count_it == info.direct_call_counts.end() || call_count_it->second == 0)
        return false;

    out = {};
    if (!collect_param_operands(fn, out.params))
        return false;

    std::vector<operand> common_args(static_cast<size_t>(fn.num_params), operand::make_none());
    std::vector<bool> have_value(static_cast<size_t>(fn.num_params), false);
    std::vector<bool> still_constant(static_cast<size_t>(fn.num_params), true);
    int matched_calls = 0;

    for (auto &caller : mod.functions) {
        for (size_t i = 0; i < caller.icodes.size(); ++i) {
            const auto &ic = caller.icodes[i];
            if (ic.op != icode_op::CALL || ic.func_name != fn.name)
                continue;

            ++matched_calls;
            size_t send_begin = i;
            std::vector<operand> args;
            if (!collect_call_args(caller.icodes, i, ic, send_begin, args))
                return false;
            if (static_cast<int>(args.size()) != fn.num_params)
                return false;

            for (size_t arg_i = 0; arg_i < args.size(); ++arg_i) {
                if (!still_constant[arg_i])
                    continue;
                if (caller.name == fn.name &&
                    same_parameter_value(args[arg_i], out.params[arg_i])) {
                    // A recursive call that forwards its own argument does
                    // not introduce a new value into the fixed point.
                    continue;
                }
                if (!is_constant_argument_value(args[arg_i])) {
                    still_constant[arg_i] = false;
                    continue;
                }
                if (!have_value[arg_i]) {
                    common_args[arg_i] = args[arg_i];
                    have_value[arg_i] = true;
                    continue;
                }
                if (!same_constant_value(common_args[arg_i], args[arg_i]))
                    still_constant[arg_i] = false;
            }
        }
    }

    if (matched_calls != call_count_it->second)
        return false;

    std::vector<bool> eligible(out.params.size(), false);
    for (size_t i = 0; i < out.params.size(); ++i)
        eligible[i] = have_value[i] && still_constant[i];

    for (auto &ic : fn.icodes) {
        auto matches_param_base = [&](const operand &op, const operand &param) {
            if (op.kind != param.kind)
                return false;
            if (op.is_temp())
                return op.temp_id == param.temp_id;
            if (op.is_symbol())
                return base_symbol_key(op) == base_symbol_key(param);
            return false;
        };

        for (size_t i = 0; i < out.params.size(); ++i) {
            if (!eligible[i])
                continue;
            const operand &param = out.params[i];
            auto reject_offset_use = [&](const operand &op) {
                if (matches_param_base(op, param) && op.byte_offset != 0)
                    eligible[i] = false;
            };
            reject_offset_use(ic.result);
            reject_offset_use(ic.left);
            reject_offset_use(ic.right);

            if (ic.op == icode_op::ADDRESS_OF &&
                matches_param_base(ic.left, param)) {
                eligible[i] = false;
            }

            if (ic.op != icode_op::RECEIVE &&
                op_writes_result_symbol(ic.op) &&
                matches_param_base(ic.result, param)) {
                eligible[i] = false;
            }
        }
    }

    for (size_t i = 0; i < out.params.size(); ++i) {
        if (!eligible[i])
            continue;
        if (out.params[i].is_symbol()) {
            out.replacements.emplace(base_symbol_key(out.params[i]),
                                     common_args[i]);
        } else if (out.params[i].is_temp()) {
            out.temp_replacements.emplace(out.params[i].temp_id,
                                          common_args[i]);
        }
    }

    return !out.replacements.empty() || !out.temp_replacements.empty();
}

static bool evaluate_const_call_impl(const ir_module *mod,
                                     const ir_function &callee,
                                     const std::vector<const_eval_value> &args,
                                     int64_t &ret_value,
                                     bool &has_ret_value,
                                     bool require_private,
                                     std::unordered_set<std::string> &active,
                                     const_eval_memory &memory,
                                     int &next_frame_id);

static bool evaluate_const_direct_call(
    const ir_module *mod, const icode &call_ic, const std::vector<const_eval_value> &args,
    int64_t &ret_value, bool &has_ret_value,
    std::unordered_set<std::string> &active,
    const_eval_memory &memory,
    int &next_frame_id)
{
    if (call_ic.func_name.empty())
        return false;
    if (evaluate_const_runtime_helper_call(call_ic, args, ret_value, has_ret_value))
        return true;
    if (!mod)
        return false;

    const ir_function *callee = find_function(*mod, call_ic.func_name);
    if (!callee)
        return false;

    return evaluate_const_call_impl(mod, *callee, args, ret_value, has_ret_value,
                                    true, active, memory, next_frame_id);
}

static bool evaluate_const_call_impl(const ir_module *mod,
                                     const ir_function &callee,
                                     const std::vector<const_eval_value> &args,
                                     int64_t &ret_value,
                                     bool &has_ret_value,
                                     bool require_private,
                                     std::unordered_set<std::string> &active,
                                     const_eval_memory &memory,
                                     int &next_frame_id)
{
    if (require_private && callee.is_global)
        return false;
    if (callee.ret_type &&
        !callee.ret_type->is_integer() &&
        callee.ret_type->kind != type_kind::VOID) {
        return false;
    }
    if (!can_const_eval_whole_function(callee))
        return false;

    std::vector<operand> params;
    if (!collect_param_operands(callee, params))
        return false;
    if (params.size() != args.size())
        return false;

    const std::string frame_key =
        (callee.name.empty() ? "__anon" : callee.name) + "#" +
        std::to_string(next_frame_id++);

    const_eval_store values;
    values.reserve(params.size() * 2);
    for (size_t i = 0; i < params.size(); ++i) {
        if (!store_const_eval_value(params[i], args[i], frame_key, values, memory))
            return false;
    }

    std::unordered_map<std::string, size_t> label_index;
    label_index.reserve(callee.icodes.size());
    for (size_t i = 0; i < callee.icodes.size(); ++i) {
        if (callee.icodes[i].op == icode_op::LABEL)
            label_index[callee.icodes[i].label_name] = i;
    }

    bool inserted = false;
    if (!callee.name.empty()) {
        auto [_, was_inserted] = active.insert(callee.name);
        if (!was_inserted)
            return false;
        inserted = true;
    }

    auto run = [&]() -> bool {
        has_ret_value = false;
        ret_value = 0;

        size_t pc = 0;
        int steps = 0;
        while (pc < callee.icodes.size() && steps++ < 10000) {
            const auto &ic = callee.icodes[pc];

            switch (ic.op) {
            case icode_op::FUNCTION:
            case icode_op::ENDFUNCTION:
            case icode_op::LABEL:
            case icode_op::RECEIVE:
            case icode_op::SEND:
                ++pc;
                break;

            case icode_op::GOTO: {
                auto it = label_index.find(ic.label_name);
                if (it == label_index.end())
                    return false;
                pc = it->second;
                break;
            }

            case icode_op::IFX: {
                int64_t cond = 0;
                if (!read_const_eval_integer(ic.left, values, memory, frame_key, cond))
                    return false;
                const std::string &target =
                    cond != 0 ? ic.true_lbl : ic.false_lbl;
                if (target.empty()) {
                    ++pc;
                    break;
                }
                auto it = label_index.find(target);
                if (it == label_index.end())
                    return false;
                pc = it->second;
                break;
            }

            case icode_op::RETURN:
                if (ic.left.is_none()) {
                    has_ret_value = false;
                    return true;
                }
                if (!read_const_eval_integer(ic.left, values, memory, frame_key,
                                             ret_value)) {
                    return false;
                }
                ret_value = normalize_integer_value(
                    ret_value, callee.ret_type ? callee.ret_type : ic.left.type);
                has_ret_value = true;
                return true;

            case icode_op::ASSIGN: {
                const_eval_value value;
                if (!read_const_eval_value(ic.left, values, memory, frame_key, value) ||
                    !store_const_eval_value(ic.result, value, frame_key, values, memory)) {
                    return false;
                }
                ++pc;
                break;
            }

            case icode_op::NEG:
            case icode_op::BNOT: {
                int64_t value = 0;
                if (!read_const_eval_integer(ic.left, values, memory, frame_key, value))
                    return false;
                value = (ic.op == icode_op::NEG) ? -value : ~value;
                if (!store_const_eval_value(
                        ic.result,
                        const_eval_value::make_int(normalize_integer_value(
                            value, ic.result.type ? ic.result.type : ic.left.type)),
                        frame_key, values, memory)) {
                    return false;
                }
                ++pc;
                break;
            }

            case icode_op::ADDRESS_OF: {
                const std::string addr_key = memory_key_for_symbol(ic.left, frame_key);
                if (addr_key.empty() ||
                    !store_const_eval_value(ic.result,
                                            const_eval_value::make_address(addr_key,
                                                                          ic.left.byte_offset,
                                                                          ic.left.type),
                                            frame_key, values, memory)) {
                    return false;
                }
                ++pc;
                break;
            }

            case icode_op::GET_VALUE_AT: {
                const_eval_value ptr;
                if (!read_const_eval_value(ic.left, values, memory, frame_key, ptr) ||
                    !is_const_eval_address_access_compatible(ptr, ic.result.type)) {
                    return false;
                }
                int64_t loaded = 0;
                if (!read_const_eval_memory_integer(ptr.address_key,
                                                    static_cast<int>(ptr.address_offset),
                                                    ic.result.type,
                                                    memory, loaded) ||
                    !store_const_eval_value(ic.result,
                                            const_eval_value::make_int(loaded),
                                            frame_key, values, memory)) {
                    return false;
                }
                ++pc;
                break;
            }

            case icode_op::SET_VALUE_AT: {
                const_eval_value ptr;
                const_eval_value value;
                if (!read_const_eval_value(ic.result, values, memory, frame_key, ptr) ||
                    !is_const_eval_address_access_compatible(ptr, ic.left.type) ||
                    !read_const_eval_value(ic.left, values, memory, frame_key, value)) {
                    return false;
                }
                if (!value.is_int() ||
                    !write_const_eval_memory_integer(ptr.address_key,
                                                     static_cast<int>(ptr.address_offset),
                                                     ic.left.type,
                                                     value.int_value,
                                                     memory, memory)) {
                    return false;
                }
                ++pc;
                break;
            }

            case icode_op::ADD: case icode_op::SUB:
            case icode_op::MUL: case icode_op::DIV: case icode_op::MOD:
            case icode_op::BAND: case icode_op::BOR: case icode_op::BXOR:
            case icode_op::SHL: case icode_op::SHR:
            case icode_op::ROL: case icode_op::ROR:
            case icode_op::EQ:  case icode_op::NE:
            case icode_op::LT:  case icode_op::LE:
            case icode_op::GT:  case icode_op::GE: {
                const_eval_value lhs_val;
                const_eval_value rhs_val;
                if (!read_const_eval_value(ic.left, values, memory, frame_key, lhs_val) ||
                    !read_const_eval_value(ic.right, values, memory, frame_key, rhs_val)) {
                    return false;
                }

                if (lhs_val.is_address() || rhs_val.is_address()) {
                    if (ic.op == icode_op::ADD || ic.op == icode_op::SUB) {
                        const const_eval_value &addr =
                            lhs_val.is_address() ? lhs_val : rhs_val;
                        const const_eval_value &offset =
                            lhs_val.is_address() ? rhs_val : lhs_val;
                        if (!addr.is_address() || !offset.is_int())
                            return false;
                        if (ic.op == icode_op::SUB && !lhs_val.is_address())
                            return false;
                        const int64_t delta =
                            ic.op == icode_op::SUB
                                ? -offset.int_value
                                : offset.int_value;
                        if (!store_const_eval_value(
                                ic.result,
                                const_eval_value::make_address(
                                    addr.address_key,
                                    addr.address_offset + delta,
                                    addr.address_object_type),
                                frame_key, values, memory)) {
                            return false;
                        }
                        ++pc;
                        break;
                    }
                    if (ic.op == icode_op::EQ || ic.op == icode_op::NE) {
                        if (!lhs_val.is_address() || !rhs_val.is_address())
                            return false;
                        const int64_t eq =
                            lhs_val.address_key == rhs_val.address_key &&
                            lhs_val.address_offset == rhs_val.address_offset
                                ? 1 : 0;
                        if (!store_const_eval_value(
                                ic.result,
                                const_eval_value::make_int(
                                    ic.op == icode_op::EQ ? eq : (eq == 0 ? 1 : 0)),
                                frame_key, values, memory)) {
                            return false;
                        }
                        ++pc;
                        break;
                    }
                    return false;
                }

                int64_t value = 0;
                if (!evaluate_const_binary(ic, lhs_val.int_value, rhs_val.int_value,
                                           value) ||
                    !store_const_eval_value(ic.result,
                                            const_eval_value::make_int(value),
                                            frame_key, values, memory)) {
                    return false;
                }
                ++pc;
                break;
            }

            case icode_op::CAST: {
                const_eval_value value;
                if (!read_const_eval_value(ic.left, values, memory, frame_key, value))
                    return false;
                if (value.is_address()) {
                    if ((!ic.result.type || !ic.result.type->is_ptr()) &&
                        (!ic.left.type || !ic.left.type->is_ptr())) {
                        return false;
                    }
                    if (!store_const_eval_value(ic.result, value, frame_key,
                                                values, memory)) {
                        return false;
                    }
                    ++pc;
                    break;
                }
                if (!store_const_eval_value(
                        ic.result,
                        const_eval_value::make_int(
                            normalize_integer_value(value.int_value, ic.result.type)),
                        frame_key, values, memory)) {
                    return false;
                }
                ++pc;
                break;
            }

            case icode_op::CALL: {
                size_t send_begin = pc;
                std::vector<operand> call_args;
                if (!collect_call_args(callee.icodes, pc, ic, send_begin, call_args))
                    return false;

                std::vector<const_eval_value> const_args;
                const_args.reserve(call_args.size());
                for (const auto &arg : call_args) {
                    const_eval_value value;
                    if (!read_const_eval_value(arg, values, memory, frame_key, value))
                        return false;
                    const_args.push_back(std::move(value));
                }

                int64_t nested_ret = 0;
                bool nested_has_ret = false;
                if (!evaluate_const_direct_call(mod, ic, const_args, nested_ret,
                                                nested_has_ret, active,
                                                memory, next_frame_id)) {
                    return false;
                }

                if (!ic.result.is_none()) {
                    if (!nested_has_ret ||
                        !store_const_eval_value(
                            ic.result,
                            const_eval_value::make_int(
                                ic.result.type
                                    ? normalize_integer_value(nested_ret, ic.result.type)
                                    : nested_ret),
                            frame_key, values, memory)) {
                        return false;
                    }
                }
                ++pc;
                break;
            }

            default:
                return false;
            }
        }

        if (!callee.ret_type || callee.ret_type->kind == type_kind::VOID) {
            has_ret_value = false;
            ret_value = 0;
            return true;
        }
        return false;
    };

    const bool ok = run();
    if (inserted)
        active.erase(callee.name);
    return ok;
}

static bool evaluate_const_call(const ir_module *mod,
                                const ir_function &callee,
                                const std::vector<operand> &args,
                                int64_t &ret_value,
                                bool &has_ret_value,
                                bool require_private = true)
{
    if (!can_const_eval_whole_function(callee))
        return false;
    std::unordered_set<std::string> active;
    const_eval_memory memory;
    std::vector<const_eval_value> arg_values;
    arg_values.reserve(args.size());
    for (const auto &arg : args) {
        if (arg.kind != operand_kind::INT_CONST)
            return false;
        arg_values.push_back(
            const_eval_value::make_int(normalize_integer_value(arg.ival, arg.type)));
    }
    int next_frame_id = 0;
    return evaluate_const_call_impl(mod, callee, arg_values, ret_value,
                                    has_ret_value, require_private, active,
                                    memory, next_frame_id);
}

static bool rewrite_const_call_site(ir_function &caller, size_t call_idx,
                                    const type_ptr &ret_type,
                                    int64_t ret_value,
                                    bool has_ret_value) {
    const auto &call_ic = caller.icodes[call_idx];
    size_t send_begin = call_idx;
    std::vector<operand> args;
    if (!collect_call_args(caller.icodes, call_idx, call_ic, send_begin, args))
        return false;

    std::vector<icode> replacement;
    if (!call_ic.result.is_none()) {
        if (!has_ret_value)
            return false;
        icode assign;
        assign.op = icode_op::ASSIGN;
        assign.result = call_ic.result;
        assign.left = operand::make_int(
            normalize_integer_value(ret_value,
                                    call_ic.result.type ? call_ic.result.type
                                                        : ret_type),
            call_ic.result.type ? call_ic.result.type : ret_type);
        assign.line = call_ic.line;
        replacement.push_back(std::move(assign));
    }

    auto erase_begin =
        caller.icodes.begin() + static_cast<std::ptrdiff_t>(send_begin);
    auto erase_end =
        caller.icodes.begin() + static_cast<std::ptrdiff_t>(call_idx + 1);
    caller.icodes.erase(erase_begin, erase_end);
    caller.icodes.insert(
        caller.icodes.begin() + static_cast<std::ptrdiff_t>(send_begin),
        replacement.begin(), replacement.end());
    return true;
}

using straight_const_env = std::unordered_map<std::string, operand>;

static std::string straight_const_key(const operand &op) {
    if (op.is_temp())
        return "T:" + std::to_string(op.temp_id);
    if (!op.is_symbol() || op.is_global || op.is_func ||
        op.is_tls || op.is_sfr || op.byte_offset != 0) {
        return {};
    }
    return "S:" + symbol_key(op);
}

static void erase_straight_const(const operand &op, straight_const_env &env) {
    const std::string key = straight_const_key(op);
    if (!key.empty())
        env.erase(key);
}

static bool resolve_straight_const(const operand &op,
                                   const straight_const_env &env,
                                   operand &out) {
    if (op.kind == operand_kind::INT_CONST) {
        out = operand::make_int(normalize_integer_value(op.ival, op.type), op.type);
        return true;
    }

    const std::string key = straight_const_key(op);
    if (key.empty())
        return false;

    auto it = env.find(key);
    if (it == env.end())
        return false;

    type_ptr out_type = op.type ? op.type : it->second.type;
    out = operand::make_int(normalize_integer_value(it->second.ival, out_type),
                            out_type);
    return true;
}

static void set_straight_const(const operand &dst,
                               int64_t value,
                               const type_ptr &value_type,
                               straight_const_env &env) {
    const std::string key = straight_const_key(dst);
    if (key.empty())
        return;
    type_ptr out_type = dst.type ? dst.type : value_type;
    env[key] = operand::make_int(normalize_integer_value(value, out_type), out_type);
}

static void update_straight_const_env(const icode &ic, straight_const_env &env) {
    switch (ic.op) {
    case icode_op::FUNCTION:
    case icode_op::ENDFUNCTION:
    case icode_op::LABEL:
    case icode_op::GOTO:
    case icode_op::IFX:
    case icode_op::INLINE_ASM:
    case icode_op::ALLOCA:
    case icode_op::SET_VALUE_AT:
        env.clear();
        return;

    case icode_op::RETURN:
    case icode_op::SEND:
        return;

    case icode_op::RECEIVE:
        erase_straight_const(ic.result, env);
        return;

    case icode_op::CALL:
        env.clear();
        erase_straight_const(ic.result, env);
        return;

    case icode_op::ASSIGN: {
        operand value;
        if (resolve_straight_const(ic.left, env, value)) {
            set_straight_const(ic.result, value.ival,
                               ic.result.type ? ic.result.type : value.type, env);
        } else {
            erase_straight_const(ic.result, env);
        }
        return;
    }

    case icode_op::NEG:
    case icode_op::BNOT: {
        operand value;
        if (!resolve_straight_const(ic.left, env, value)) {
            erase_straight_const(ic.result, env);
            return;
        }
        int64_t out_value = (ic.op == icode_op::NEG) ? -value.ival : ~value.ival;
        set_straight_const(ic.result, out_value,
                           ic.result.type ? ic.result.type : value.type, env);
        return;
    }

    case icode_op::CAST: {
        operand value;
        if (!resolve_straight_const(ic.left, env, value)) {
            erase_straight_const(ic.result, env);
            return;
        }
        if (!cast_can_fold_to_int_const(ic.result.type)) {
            erase_straight_const(ic.result, env);
            return;
        }
        set_straight_const(ic.result, value.ival, ic.result.type, env);
        return;
    }

    case icode_op::ADD: case icode_op::SUB:
    case icode_op::MUL: case icode_op::DIV: case icode_op::MOD:
    case icode_op::BAND: case icode_op::BOR: case icode_op::BXOR:
    case icode_op::SHL: case icode_op::SHR:
    case icode_op::ROL: case icode_op::ROR:
    case icode_op::EQ:  case icode_op::NE:
    case icode_op::LT:  case icode_op::LE:
    case icode_op::GT:  case icode_op::GE: {
        operand lhs;
        operand rhs;
        int64_t out_value = 0;
        if (!resolve_straight_const(ic.left, env, lhs) ||
            !resolve_straight_const(ic.right, env, rhs) ||
            !evaluate_const_binary(ic, lhs.ival, rhs.ival, out_value)) {
            erase_straight_const(ic.result, env);
            return;
        }
        set_straight_const(ic.result, out_value, ic.result.type, env);
        return;
    }

    default:
        erase_straight_const(ic.result, env);
        return;
    }
}

struct single_const_symbol {
    operand value = operand::make_none();
    bool have_value = false;
    bool invalid = false;
};

static bool is_single_const_local_symbol(const operand &op) {
    if (!op.is_symbol() || op.is_global || op.is_func ||
        op.is_tls || op.is_sfr || op.is_param) {
        return false;
    }
    if (op.byte_offset != 0)
        return false;
    if (op.type && op.type->is_volatile)
        return false;
    return is_const_eval_integer_type(op.type);
}

static bool is_local_symbol_base(const operand &op) {
    return op.is_symbol() &&
           !op.is_global &&
           !op.is_func &&
           !op.is_tls &&
           !op.is_sfr &&
           !op.is_param;
}

static void invalidate_const_symbol(
    const operand &op,
    std::unordered_map<std::string, single_const_symbol> &symbols)
{
    if (!is_local_symbol_base(op))
        return;
    symbols[base_symbol_key(op)].invalid = true;
}

static void invalidate_partial_symbol_use(
    const operand &op,
    std::unordered_map<std::string, single_const_symbol> &symbols)
{
    if (!is_local_symbol_base(op) || op.byte_offset == 0)
        return;
    symbols[base_symbol_key(op)].invalid = true;
}

static bool collect_single_const_local_replacements(
    const ir_function &fn,
    std::unordered_map<std::string, operand> &replacements)
{
    std::unordered_map<std::string, single_const_symbol> symbols;
    straight_const_env env;

    for (const auto &ic : fn.icodes) {
        invalidate_partial_symbol_use(ic.result, symbols);
        invalidate_partial_symbol_use(ic.left, symbols);
        invalidate_partial_symbol_use(ic.right, symbols);

        if (ic.op == icode_op::SET_VALUE_AT)
            return false;

        if (ic.op == icode_op::ADDRESS_OF) {
            invalidate_const_symbol(ic.left, symbols);
            update_straight_const_env(ic, env);
            continue;
        }

        if (!op_writes_result_symbol(ic.op) || !ic.result.is_symbol()) {
            update_straight_const_env(ic, env);
            continue;
        }

        const std::string key = base_symbol_key(ic.result);
        if (!is_single_const_local_symbol(ic.result)) {
            invalidate_const_symbol(ic.result, symbols);
            update_straight_const_env(ic, env);
            continue;
        }

        auto &entry = symbols[key];
        operand resolved;
        if (ic.op != icode_op::ASSIGN ||
            !resolve_straight_const(ic.left, env, resolved) ||
            resolved.kind != operand_kind::INT_CONST) {
            entry.invalid = true;
            update_straight_const_env(ic, env);
            continue;
        }
        if (entry.have_value) {
            entry.invalid = true;
            update_straight_const_env(ic, env);
            continue;
        }

        entry.value = operand::make_int(
            normalize_integer_value(resolved.ival, ic.result.type),
            ic.result.type ? ic.result.type : resolved.type);
        entry.have_value = true;
        update_straight_const_env(ic, env);
    }

    for (const auto &[key, entry] : symbols) {
        if (!entry.invalid && entry.have_value)
            replacements.emplace(key, entry.value);
    }
    return !replacements.empty();
}

static bool rewrite_function_as_const_return(ir_function &fn,
                                             int64_t ret_value,
                                             bool has_ret_value) {
    if (fn.icodes.empty())
        return false;

    std::vector<icode> rewritten;
    rewritten.reserve(3);

    icode function_ic = fn.icodes.front();
    function_ic.local_bytes = 0;
    rewritten.push_back(std::move(function_ic));

    icode ret_ic;
    ret_ic.op = icode_op::RETURN;
    if (has_ret_value) {
        ret_ic.left = operand::make_int(
            normalize_integer_value(ret_value, fn.ret_type), fn.ret_type);
    } else {
        ret_ic.left = operand::make_none();
    }
    rewritten.push_back(std::move(ret_ic));

    rewritten.push_back(fn.icodes.back());
    fn.icodes = std::move(rewritten);
    fn.local_bytes = 0;
    fn.orig_local_bytes = 0;
    fn.stack_param_bytes = 0;
    return true;
}

static operand remap_operand(
    const operand &op,
    std::unordered_map<int, int> &temp_map,
    const std::unordered_map<std::string, operand> &param_map,
    const std::unordered_map<int, operand> &param_temp_map,
    const std::unordered_map<std::string, operand> &local_map,
    int &next_temp)
{
    if (op.is_temp()) {
        auto pit = param_temp_map.find(op.temp_id);
        if (pit != param_temp_map.end()) {
            operand remapped = pit->second;
            remapped.type = op.type ? op.type : remapped.type;
            return remapped;
        }
        auto it = temp_map.find(op.temp_id);
        if (it == temp_map.end()) {
            it = temp_map.emplace(op.temp_id, next_temp++).first;
        }
        operand remapped = op;
        remapped.temp_id = it->second;
        return remapped;
    }

    if (op.is_symbol()) {
        auto it = param_map.find(base_symbol_key(op));
        if (it != param_map.end()) {
            operand remapped = it->second;
            remapped.type = op.type ? op.type : remapped.type;
            remapped.byte_offset += op.byte_offset;
            return remapped;
        }

        auto local_it = local_map.find(base_symbol_key(op));
        if (local_it != local_map.end()) {
            operand remapped = local_it->second;
            remapped.type = op.type ? op.type : remapped.type;
            remapped.byte_offset += op.byte_offset;
            return remapped;
        }
    }

    return op;
}

static std::string make_unique_inline_label(const ir_function &fn,
                                            const std::string &prefix) {
    for (int id = 0;; ++id) {
        std::string candidate = prefix + std::to_string(id);
        bool taken = false;
        for (const auto &ic : fn.icodes) {
            if (ic.op == icode_op::LABEL && ic.label_name == candidate) {
                taken = true;
                break;
            }
        }
        if (!taken)
            return candidate;
    }
}

static bool inline_call_site(ir_function &caller, size_t call_idx,
                             const inline_candidate &candidate) {
    const auto &call_ic = caller.icodes[call_idx];
    size_t send_begin = call_idx;
    std::vector<operand> args;
    if (!collect_call_args(caller.icodes, call_idx, call_ic, send_begin, args))
        return false;

    std::unordered_map<int, int> temp_map;
    int next_temp = next_temp_id(caller);
    std::unordered_map<std::string, operand> local_map;
    const int caller_local_bytes = caller.local_bytes;
    const bool needs_stack_locals = !candidate.stack_local_keys.empty();
    if (needs_stack_locals) {
        caller.local_bytes += candidate.local_frame_bytes;
        caller.orig_local_bytes += candidate.local_frame_bytes;
        if (!caller.icodes.empty() && caller.icodes.front().op == icode_op::FUNCTION)
            caller.icodes.front().local_bytes = caller.local_bytes;
    }
    for (auto &[key, op] : candidate.local_symbols) {
        if (candidate.stack_local_keys.find(key) != candidate.stack_local_keys.end()) {
            operand storage = op;
            storage.byte_offset = 0;
            storage.stack_offset = op.stack_offset - caller_local_bytes;
            local_map.emplace(key, std::move(storage));
            continue;
        }
        local_map.emplace(key, operand::make_temp(next_temp++, op.type));
    }
    std::unordered_map<std::string, operand> param_map;
    std::unordered_map<int, operand> param_temp_map;
    std::vector<icode> init_assigns;
    for (auto &entry : candidate.param_argregs) {
        if (entry.second < 0 || entry.second >= static_cast<int>(args.size()))
            return false;
        if (candidate.mutable_param_keys.find(entry.first) !=
            candidate.mutable_param_keys.end()) {
            auto sym_it = candidate.param_symbols.find(entry.first);
            if (sym_it == candidate.param_symbols.end())
                return false;
            operand storage = operand::make_temp(next_temp++, sym_it->second.type);
            local_map[entry.first] = storage;

            icode init_ic;
            init_ic.op = icode_op::ASSIGN;
            init_ic.result = storage;
            init_ic.left = args[static_cast<size_t>(entry.second)];
            init_ic.line = call_ic.line;
            init_assigns.push_back(std::move(init_ic));
        } else {
            param_map[entry.first] = args[static_cast<size_t>(entry.second)];
        }
    }
    for (auto &entry : candidate.temp_param_argregs) {
        if (entry.second < 0 || entry.second >= static_cast<int>(args.size()))
            return false;
        auto temp_it = candidate.param_temps.find(entry.first);
        if (temp_it == candidate.param_temps.end())
            return false;
        operand storage = operand::make_temp(next_temp++, temp_it->second.type);
        param_temp_map[entry.first] = storage;

        icode init_ic;
        init_ic.op = icode_op::ASSIGN;
        init_ic.result = storage;
        init_ic.left = args[static_cast<size_t>(entry.second)];
        init_ic.line = call_ic.line;
        init_assigns.push_back(std::move(init_ic));
    }

    std::unordered_map<std::string, std::string> label_map;
    for (const auto &body_ic : candidate.body) {
        if (body_ic.op != icode_op::LABEL)
            continue;
        label_map.emplace(
            body_ic.label_name,
            make_unique_inline_label(caller, "__xcc_inl_" + caller.name + "_" +
                                                 body_ic.label_name + "_"));
    }
    std::vector<icode> replacement;
    replacement.reserve(init_assigns.size() + candidate.body.size() + 4);
    replacement.insert(replacement.end(), init_assigns.begin(), init_assigns.end());
    std::string exit_label;

    for (size_t idx = 0; idx < candidate.body.size(); ++idx) {
        auto body_ic = candidate.body[idx];

        if (body_ic.op == icode_op::LABEL) {
            auto it = label_map.find(body_ic.label_name);
            if (it == label_map.end())
                return false;
            body_ic.label_name = it->second;
            replacement.push_back(std::move(body_ic));
            continue;
        }

        if (body_ic.op == icode_op::GOTO) {
            auto it = label_map.find(body_ic.label_name);
            if (it == label_map.end())
                return false;
            body_ic.label_name = it->second;
            replacement.push_back(std::move(body_ic));
            continue;
        }

        if (body_ic.op == icode_op::IFX) {
            body_ic.left   = remap_operand(body_ic.left, temp_map, param_map,
                                           param_temp_map,
                                           local_map, next_temp);
            body_ic.right  = remap_operand(body_ic.right, temp_map, param_map,
                                           param_temp_map,
                                           local_map, next_temp);
            if (!body_ic.true_lbl.empty()) {
                auto it = label_map.find(body_ic.true_lbl);
                if (it == label_map.end())
                    return false;
                body_ic.true_lbl = it->second;
            }
            if (!body_ic.false_lbl.empty()) {
                auto it = label_map.find(body_ic.false_lbl);
                if (it == label_map.end())
                    return false;
                body_ic.false_lbl = it->second;
            }
            replacement.push_back(std::move(body_ic));
            continue;
        }

        if (body_ic.op == icode_op::RETURN) {
            if (!call_ic.result.is_none() && !body_ic.left.is_none()) {
                operand ret = remap_operand(body_ic.left, temp_map, param_map,
                                            param_temp_map,
                                            local_map, next_temp);
                bool forwarded = false;
                if (ret.is_temp() && !replacement.empty()) {
                    auto &producer = replacement.back();
                    if (producer.result.is_temp() &&
                        producer.result.temp_id == ret.temp_id &&
                        producer.op != icode_op::LABEL &&
                        producer.op != icode_op::GOTO &&
                        producer.op != icode_op::IFX &&
                        producer.op != icode_op::RETURN) {
                        producer.result = call_ic.result;
                        forwarded = true;
                    }
                }
                if (!forwarded) {
                    icode assign;
                    assign.op     = icode_op::ASSIGN;
                    assign.result = call_ic.result;
                    assign.left   = ret;
                    assign.line   = call_ic.line;
                    replacement.push_back(std::move(assign));
                }
            }

            if (idx + 1 < candidate.body.size()) {
                if (exit_label.empty())
                    exit_label = make_unique_inline_label(
                        caller, "__xcc_inl_" + caller.name + "_exit_");
                icode goto_ic;
                goto_ic.op = icode_op::GOTO;
                goto_ic.label_name = exit_label;
                goto_ic.line = call_ic.line;
                replacement.push_back(std::move(goto_ic));
            }
            continue;
        }

        body_ic.result = remap_operand(body_ic.result, temp_map, param_map,
                                       param_temp_map,
                                       local_map, next_temp);
        body_ic.left   = remap_operand(body_ic.left,   temp_map, param_map,
                                       param_temp_map,
                                       local_map, next_temp);
        body_ic.right  = remap_operand(body_ic.right,  temp_map, param_map,
                                       param_temp_map,
                                       local_map, next_temp);
        replacement.push_back(std::move(body_ic));
    }

    if (!exit_label.empty()) {
        icode exit_ic;
        exit_ic.op = icode_op::LABEL;
        exit_ic.label_name = exit_label;
        replacement.push_back(std::move(exit_ic));
    }

    auto begin = caller.icodes.begin() + static_cast<std::ptrdiff_t>(send_begin);
    auto end   = caller.icodes.begin() + static_cast<std::ptrdiff_t>(call_idx + 1);
    caller.icodes.erase(begin, end);
    caller.icodes.insert(caller.icodes.begin() + static_cast<std::ptrdiff_t>(send_begin),
                         replacement.begin(), replacement.end());
    return true;
}

class dead_internal_function_pass final : public ir_module_pass {
public:
    const char *name() const override { return "dead_internal_function"; }

    bool run(ir_module &mod) override {
        const auto info = build_module_use_info(mod);
        auto old_size = mod.functions.size();
        mod.functions.erase(
            std::remove_if(mod.functions.begin(), mod.functions.end(),
                [&](const ir_function &fn) {
                    return (!fn.is_global || fn.discard_if_unused) &&
                           info.referenced_funcs.find(fn.name) ==
                               info.referenced_funcs.end();
                }),
            mod.functions.end());
        return mod.functions.size() != old_size;
    }
};

class constant_arg_propagation_pass final : public ir_module_pass {
public:
    const char *name() const override { return "constant_arg_propagation"; }

    bool run(ir_module &mod) override {
        const auto info = build_module_use_info(mod);
        bool changed = false;

        for (auto &fn : mod.functions) {
            constant_arg_candidate candidate;
            if (!analyze_constant_arg_candidate(mod, fn, info, candidate))
                continue;
            changed = apply_constant_arg_replacements(
                fn, candidate.replacements, candidate.temp_replacements) ||
                changed;
        }

        return changed;
    }
};

class const_call_eval_pass final : public ir_module_pass {
public:
    explicit const_call_eval_pass(bool fixed_specialization)
        : fixed_specialization_(fixed_specialization) {}

    const char *name() const override { return "const_call_eval"; }

    bool run(ir_module &mod) override {
        const auto info = build_module_use_info(mod);

        for (auto &caller : mod.functions) {
            straight_const_env env;
            for (size_t i = 0; i < caller.icodes.size(); ++i) {
                const auto &ic = caller.icodes[i];
                if (ic.op != icode_op::CALL || ic.func_name.empty()) {
                    update_straight_const_env(ic, env);
                    continue;
                }

                if (info.address_taken_funcs.find(ic.func_name) !=
                    info.address_taken_funcs.end()) {
                    update_straight_const_env(ic, env);
                    continue;
                }

                size_t send_begin = i;
                std::vector<operand> args;
                if (!collect_call_args(caller.icodes, i, ic, send_begin, args)) {
                    update_straight_const_env(ic, env);
                    continue;
                }

                icode_op raw_binary_op = icode_op::ASSIGN;
                if (fixed_specialization_ &&
                    fixed_raw_binary_opcode(ic.func_name, raw_binary_op) &&
                    rewrite_fixed_raw_binary_call(caller, i, send_begin,
                                                  args, raw_binary_op)) {
                    return true;
                }

                operand resolved_num;
                operand resolved_den;
                std::string div_target;
                const bool numerator_const =
                    args.size() == 2 &&
                    resolve_straight_const(args[0], env, resolved_num) &&
                    resolved_num.kind == operand_kind::INT_CONST;
                if (fixed_specialization_ &&
                    !numerator_const &&
                    args.size() == 2 &&
                    resolve_straight_const(args[1], env, resolved_den) &&
                    resolved_den.kind == operand_kind::INT_CONST &&
                    fixed_div_specialized_target(ic.func_name, resolved_den.ival,
                                                 div_target)) {
                    if (rewrite_fixed_unary_specialized_call(caller, i, send_begin,
                                                             args[0], div_target)) {
                        return true;
                    }
                }

                operand resolved_left;
                operand resolved_right;
                std::string mul_target;
                const bool left_const =
                    args.size() == 2 &&
                    resolve_straight_const(args[0], env, resolved_left) &&
                    resolved_left.kind == operand_kind::INT_CONST;
                const bool right_const =
                    args.size() == 2 &&
                    resolve_straight_const(args[1], env, resolved_right) &&
                    resolved_right.kind == operand_kind::INT_CONST;
                if (fixed_specialization_ && left_const != right_const) {
                    const operand &dynamic_arg = left_const ? args[1] : args[0];
                    const int64_t multiplier =
                        left_const ? resolved_left.ival : resolved_right.ival;
                    if (fixed_mul_specialized_target(ic.func_name, multiplier,
                                                     mul_target) &&
                        rewrite_fixed_unary_specialized_call(caller, i, send_begin,
                                                             dynamic_arg,
                                                             mul_target)) {
                        return true;
                    }
                }

                bool all_int_consts = true;
                std::vector<operand> resolved_args;
                resolved_args.reserve(args.size());
                for (const auto &arg : args) {
                    operand resolved;
                    if (!resolve_straight_const(arg, env, resolved) ||
                        resolved.kind != operand_kind::INT_CONST) {
                        all_int_consts = false;
                        break;
                    }
                    resolved_args.push_back(std::move(resolved));
                }
                if (!all_int_consts) {
                    update_straight_const_env(ic, env);
                    continue;
                }

                int64_t ret_value = 0;
                bool has_ret_value = false;
                const ir_function *callee = find_function(mod, ic.func_name);
                if (callee) {
                    if (!evaluate_const_call(&mod, *callee, resolved_args, ret_value,
                                             has_ret_value)) {
                        update_straight_const_env(ic, env);
                        continue;
                    }
                    if (rewrite_const_call_site(caller, i, callee->ret_type, ret_value,
                                                has_ret_value)) {
                        return true;
                    }
                    update_straight_const_env(ic, env);
                    continue;
                }

                std::vector<const_eval_value> helper_args;
                helper_args.reserve(resolved_args.size());
                for (const auto &arg : resolved_args) {
                    helper_args.push_back(const_eval_value::make_int(
                        normalize_integer_value(arg.ival, arg.type)));
                }

                if (!evaluate_const_runtime_helper_call(ic, helper_args, ret_value,
                                                        has_ret_value)) {
                    update_straight_const_env(ic, env);
                    continue;
                }

                if (rewrite_const_call_site(caller, i,
                                            ic.result.type ? ic.result.type : type_ptr{},
                                            ret_value,
                                            has_ret_value)) {
                    return true;
                }
                update_straight_const_env(ic, env);
            }
        }

        return false;
    }

private:
    bool fixed_specialization_ = false;
};

class single_const_local_propagation_pass final : public ir_module_pass {
public:
    const char *name() const override { return "single_const_local_propagation"; }

    bool run(ir_module &mod) override {
        bool changed = false;
        for (auto &fn : mod.functions) {
            std::unordered_map<std::string, operand> replacements;
            if (!collect_single_const_local_replacements(fn, replacements))
                continue;
            changed = apply_constant_arg_replacements(fn, replacements) || changed;
        }
        return changed;
    }
};

class function_const_eval_pass final : public ir_module_pass {
public:
    const char *name() const override { return "function_const_eval"; }

    bool run(ir_module &mod) override {
        for (auto &fn : mod.functions) {
            if (fn.is_global || fn.num_params != 0 || fn.is_noreturn)
                continue;
            if (!can_const_eval_whole_function(fn))
                continue;

            int64_t ret_value = 0;
            bool has_ret_value = false;
            if (!evaluate_const_call(&mod, fn, {}, ret_value, has_ret_value, false))
                continue;
            if (rewrite_function_as_const_return(fn, ret_value, has_ret_value))
                return true;
        }
        return false;
    }
};

static bool same_parameter_storage(const operand &op, const operand &param) {
    if (op.kind != param.kind)
        return false;
    if (op.is_temp())
        return op.temp_id == param.temp_id && op.byte_offset == 0;
    if (op.is_symbol())
        return base_symbol_key(op) == base_symbol_key(param) &&
               op.byte_offset == 0;
    return false;
}

static bool pointer_parameter_is_dereference_only(const ir_function &callee,
                                                  int argreg) {
    std::vector<operand> params;
    if (!collect_param_operands(callee, params) || argreg < 0 ||
        argreg >= static_cast<int>(params.size())) {
        return false;
    }
    const operand &param = params[static_cast<size_t>(argreg)];
    if (!param.type ||
        !(param.type->is_ptr() || param.type->is_array())) {
        return false;
    }

    std::unordered_set<int> derived_temps;
    bool expanded;
    do {
        expanded = false;
        for (const auto &ic : callee.icodes) {
            const bool source_tracked =
                same_parameter_storage(ic.left, param) ||
                (ic.left.is_temp() &&
                 derived_temps.count(ic.left.temp_id) != 0);
            if (!source_tracked || !ic.result.is_temp() ||
                (ic.op != icode_op::ASSIGN && ic.op != icode_op::CAST)) {
                continue;
            }
            if (derived_temps.insert(ic.result.temp_id).second)
                expanded = true;
        }
    } while (expanded);

    auto tracked = [&](const operand &op) {
        return same_parameter_storage(op, param) ||
               (op.is_temp() && derived_temps.count(op.temp_id) != 0);
    };

    bool saw_dereference = false;
    for (const auto &ic : callee.icodes) {
        if (ic.op == icode_op::RECEIVE)
            continue;
        const bool in_result = tracked(ic.result);
        const bool in_left = tracked(ic.left);
        const bool in_right = tracked(ic.right);
        if (!in_result && !in_left && !in_right)
            continue;

        if ((ic.op == icode_op::ASSIGN || ic.op == icode_op::CAST) &&
            in_left && ic.result.is_temp() &&
            derived_temps.count(ic.result.temp_id) != 0 && !in_right) {
            continue;
        }

        if (ic.op == icode_op::GET_VALUE_AT && in_left &&
            !in_result && !in_right) {
            saw_dereference = true;
            continue;
        }
        if (ic.op == icode_op::SET_VALUE_AT && in_result &&
            !in_left && !in_right) {
            saw_dereference = true;
            continue;
        }
        return false;
    }
    return saw_dereference;
}

class internal_call_abi_promotion_pass final : public ir_module_pass {
public:
    const char *name() const override {
        return "internal_call_abi_promotion";
    }

    bool run(ir_module &mod) override {
        const auto info = build_module_use_info(mod);
        bool changed = false;

        for (size_t fn_idx = 0; fn_idx < mod.functions.size(); ++fn_idx) {
            auto &fn = mod.functions[fn_idx];
            if (fn.is_global || !fn.can_internalize_abi ||
                fn.is_variadic ||
                effective_call_abi(fn.abi) != call_abi::SDCCCALL0 ||
                info.address_taken_funcs.find(fn.name) !=
                    info.address_taken_funcs.end()) {
                continue;
            }

            const bool has_opaque_code =
                std::any_of(fn.icodes.begin(), fn.icodes.end(),
                            [](const icode &ic) {
                                return ic.op == icode_op::INLINE_ASM;
                            });
            if (has_opaque_code)
                continue;

            auto count_it = info.direct_call_counts.find(fn.name);
            if (count_it == info.direct_call_counts.end() ||
                count_it->second == 0) {
                continue;
            }

            std::vector<operand> params;
            if (!collect_param_operands(fn, params))
                continue;
            bool all_symbol_params = true;
            std::vector<type_ptr> param_types;
            param_types.reserve(params.size());
            for (const auto &param : params) {
                if (!param.is_symbol()) {
                    all_symbol_params = false;
                    break;
                }
                param_types.push_back(param.type ? param.type :
                                                   type::make_int());
            }
            if (!all_symbol_params)
                continue;

            auto &conv = get_abi_convention(call_abi::SDCCCALL1);
            const auto param_locs = conv.classify_args(param_types);
            const bool has_register_arg =
                std::any_of(param_locs.begin(), param_locs.end(),
                            [](abi_arg_loc loc) {
                                return loc != abi_arg_loc::STACK;
                            });
            const bool has_return_value =
                fn.ret_type && fn.ret_type->kind != type_kind::VOID;
            if (!has_register_arg && !has_return_value) {
                continue;
            }

            std::vector<internal_abi_call_site> sites;
            if (!collect_internal_abi_call_sites(mod, fn, sites) ||
                static_cast<int>(sites.size()) != count_it->second) {
                continue;
            }

            if (!promote_internal_function_to_sdcccall1(
                    fn, params, param_types, param_locs)) {
                continue;
            }

            // Function remapping changes parameter operands in recursive
            // SENDs.  Recollect every call site so a self-call cannot retain
            // the old ABI0 stack locations captured during validation.
            sites.clear();
            if (!collect_internal_abi_call_sites(mod, fn, sites) ||
                static_cast<int>(sites.size()) != count_it->second) {
                return changed;
            }
            if (!promote_internal_calls_to_sdcccall1(
                    mod, fn, param_types, param_locs, std::move(sites))) {
                return changed;
            }
            changed = true;
        }
        return changed;
    }
};

class tail_address_noescape_pass final : public ir_module_pass {
public:
    const char *name() const override { return "tail_address_noescape"; }

    bool run(ir_module &mod) override {
        bool changed = false;
        for (auto &fn : mod.functions) {
            bool saw_local_address = false;
            bool proven = true;

            for (size_t def_idx = 0; def_idx < fn.icodes.size(); ++def_idx) {
                const icode &def = fn.icodes[def_idx];
                if (def.op != icode_op::ADDRESS_OF ||
                    !def.left.is_symbol() || def.left.is_global) {
                    continue;
                }
                saw_local_address = true;
                if (!def.result.is_temp()) {
                    proven = false;
                    break;
                }

                std::unordered_set<int> address_temps{def.result.temp_id};
                bool expanded;
                do {
                    expanded = false;
                    for (const auto &copy : fn.icodes) {
                        if ((copy.op != icode_op::ASSIGN &&
                             copy.op != icode_op::CAST) ||
                            !copy.left.is_temp() || !copy.result.is_temp() ||
                            address_temps.count(copy.left.temp_id) == 0) {
                            continue;
                        }
                        if (address_temps.insert(copy.result.temp_id).second)
                            expanded = true;
                    }
                } while (expanded);

                bool saw_use = false;
                for (size_t use_idx = def_idx + 1;
                     use_idx < fn.icodes.size(); ++use_idx) {
                    const icode &use = fn.icodes[use_idx];
                    const bool in_left = use.left.is_temp() &&
                        address_temps.count(use.left.temp_id) != 0;
                    const bool in_right = use.right.is_temp() &&
                        address_temps.count(use.right.temp_id) != 0;
                    const bool in_result = use.op == icode_op::SET_VALUE_AT &&
                        use.result.is_temp() &&
                        address_temps.count(use.result.temp_id) != 0;
                    if (!in_left && !in_right && !in_result) {
                        if (use.result.is_temp() &&
                            address_temps.count(use.result.temp_id) != 0 &&
                            use_idx != def_idx) {
                            proven = false;
                            break;
                        }
                        continue;
                    }

                    saw_use = true;
                    if ((use.op == icode_op::ASSIGN ||
                         use.op == icode_op::CAST) && in_left && !in_right &&
                        use.result.is_temp() &&
                        address_temps.count(use.result.temp_id) != 0) {
                        continue;
                    }
                    if (use.op == icode_op::GET_VALUE_AT && in_left &&
                        !in_right && !in_result) {
                        continue;
                    }
                    if (use.op == icode_op::SET_VALUE_AT && in_result &&
                        !in_left && !in_right) {
                        continue;
                    }
                    if (use.op != icode_op::SEND || !in_left ||
                        in_right || in_result) {
                        proven = false;
                        break;
                    }

                    size_t call_idx = use_idx + 1;
                    while (call_idx < fn.icodes.size() &&
                           fn.icodes[call_idx].op == icode_op::SEND) {
                        ++call_idx;
                    }
                    if (call_idx >= fn.icodes.size() ||
                        fn.icodes[call_idx].op != icode_op::CALL ||
                        fn.icodes[call_idx].func_name.empty()) {
                        proven = false;
                        break;
                    }
                    const ir_function *callee = find_function(
                        mod, fn.icodes[call_idx].func_name);
                    if (!callee || !pointer_parameter_is_dereference_only(
                                       *callee, use.argreg)) {
                        proven = false;
                        break;
                    }
                }
                if (!proven)
                    break;
                if (!saw_use)
                    continue;
            }

            const bool safe = saw_local_address && proven;
            if (fn.tail_local_addresses_noescape != safe) {
                fn.tail_local_addresses_noescape = safe;
                changed = true;
            }
        }
        return changed;
    }
};

class dead_param_elimination_pass final : public ir_module_pass {
public:
    const char *name() const override { return "dead_param_elimination"; }

    bool run(ir_module &mod) override {
        const auto info = build_module_use_info(mod);

        for (auto &fn : mod.functions) {
            if (info.address_taken_funcs.find(fn.name) != info.address_taken_funcs.end())
                continue;
            auto count_it = info.direct_call_counts.find(fn.name);
            if (count_it == info.direct_call_counts.end() || count_it->second == 0)
                continue;

            dead_param_candidate candidate;
            if (!analyze_dead_param_candidate(fn, candidate))
                continue;
            if (!rewrite_dead_params_in_function(fn, candidate))
                continue;
            if (!rewrite_dead_params_in_calls(mod, fn, candidate))
                continue;
            return true;
        }

        return false;
    }
};

class merge_identical_helpers_pass final : public ir_module_pass {
public:
    const char *name() const override { return "merge_identical_helpers"; }

    bool run(ir_module &mod) override {
        const auto info = build_module_use_info(mod);
        std::unordered_map<std::string, std::string> canonical_by_key;

        for (auto &fn : mod.functions) {
            if (info.address_taken_funcs.find(fn.name) != info.address_taken_funcs.end())
                continue;
            if (!info.direct_call_counts.count(fn.name))
                continue;

            merge_candidate candidate;
            if (!analyze_merge_candidate(fn, candidate))
                continue;

            auto it = canonical_by_key.find(candidate.key);
            if (it == canonical_by_key.end()) {
                canonical_by_key.emplace(candidate.key, fn.name);
                continue;
            }

            if (it->second == fn.name)
                continue;

            for (auto &caller : mod.functions) {
                for (auto &ic : caller.icodes) {
                    if (ic.op == icode_op::CALL && ic.func_name == fn.name)
                        ic.func_name = it->second;
                }
            }
            return true;
        }

        return false;
    }
};

static bool collect_inline_sites(const ir_module &mod,
                                 const std::string &callee_name,
                                 std::vector<std::pair<size_t, size_t>> &sites,
                                 int &total_overhead)
{
    sites.clear();
    total_overhead = 0;

    for (size_t caller_idx = 0; caller_idx < mod.functions.size(); ++caller_idx) {
        const auto &caller = mod.functions[caller_idx];
        for (size_t i = 0; i < caller.icodes.size(); ++i) {
            const auto &ic = caller.icodes[i];
            if (ic.op != icode_op::CALL || ic.func_name != callee_name)
                continue;

            if (caller.name == callee_name)
                return false;

            size_t send_begin = i;
            std::vector<operand> args;
            if (!collect_call_args(caller.icodes, i, ic, send_begin, args))
                return false;

            total_overhead += static_cast<int>(i - send_begin + 1);
            sites.emplace_back(caller_idx, i);
        }
    }

    return true;
}

static bool candidate_has_internal_control_flow(const inline_candidate &candidate) {
    for (const auto &ic : candidate.body) {
        if (ic.op == icode_op::LABEL ||
            ic.op == icode_op::GOTO ||
            ic.op == icode_op::IFX) {
            return true;
        }
    }
    return false;
}

class trivial_internal_leaf_inline_pass final : public ir_module_pass {
public:
    explicit trivial_internal_leaf_inline_pass(optimization_settings analysis_settings)
        : analysis_settings_(std::move(analysis_settings)) {}

    const char *name() const override { return "trivial_internal_leaf_inline"; }

    bool run(ir_module &mod) override {
        const auto info = build_module_use_info(mod);

        for (size_t callee_idx = 0; callee_idx < mod.functions.size(); ++callee_idx) {
            const auto &callee = mod.functions[callee_idx];
            if (callee.is_global || callee.num_params != 0 ||
                callee.local_bytes != 0 || callee.stack_param_bytes != 0) {
                continue;
            }
            if (info.address_taken_funcs.find(callee.name) !=
                info.address_taken_funcs.end()) {
                continue;
            }
            auto count_it = info.direct_call_counts.find(callee.name);
            if (count_it == info.direct_call_counts.end() || count_it->second == 0)
                continue;

            ir_function analysis_fn = callee;
            if (analysis_settings_.has_function_ir_passes())
                ir_optimizer::optimize(analysis_fn, analysis_settings_);

            inline_candidate candidate;
            if (!analyze_inline_candidate(analysis_fn, candidate, 1))
                continue;
            if (candidate.op_count > 1 || candidate.contains_call ||
                candidate.local_frame_bytes != 0 ||
                candidate_has_internal_control_flow(candidate)) {
                continue;
            }

            std::vector<std::pair<size_t, size_t>> sites;
            int total_overhead = 0;
            if (!collect_inline_sites(mod, callee.name, sites, total_overhead))
                continue;

            for (const auto &[caller_idx, call_idx] : sites) {
                if (caller_idx == callee_idx)
                    continue;
                if (mod.functions[caller_idx].is_global)
                    continue;
                if (inline_call_site(mod.functions[caller_idx], call_idx, candidate))
                    return true;
            }
        }

        return false;
    }

private:
    optimization_settings analysis_settings_;
};

class size_profitable_static_inline_pass final : public ir_module_pass {
public:
    size_profitable_static_inline_pass(int max_inline_ops,
                                       int single_use_max_inline_ops,
                                       int few_use_max_inline_ops,
                                       bool widen_few_use_analysis,
                                       bool speed_inline_small_helpers,
                                       bool broad_speed_inline,
                                       optimization_settings analysis_settings)
        : max_inline_ops_(max_inline_ops),
          single_use_max_inline_ops_(single_use_max_inline_ops),
          few_use_max_inline_ops_(few_use_max_inline_ops),
          widen_few_use_analysis_(widen_few_use_analysis),
          speed_inline_small_helpers_(speed_inline_small_helpers),
          broad_speed_inline_(broad_speed_inline),
          analysis_settings_(std::move(analysis_settings)) {}

    const char *name() const override { return "size_profitable_static_inline"; }

    bool run(ir_module &mod) override {
        const auto info = build_module_use_info(mod);
        struct choice {
            size_t caller_idx = 0;
            size_t call_idx = 0;
            int profitability_margin = 0;
            int site_overhead = 0;
            inline_candidate candidate;
        };

        std::optional<choice> best;
        for (size_t callee_idx = 0; callee_idx < mod.functions.size(); ++callee_idx) {
            const auto &callee = mod.functions[callee_idx];
            if (info.address_taken_funcs.find(callee.name) !=
                info.address_taken_funcs.end()) {
                continue;
            }

            auto count_it = info.direct_call_counts.find(callee.name);
            if (count_it == info.direct_call_counts.end() || count_it->second == 0)
                continue;

            ir_function analysis_fn = callee;
            if (analysis_settings_.has_function_ir_passes())
                ir_optimizer::optimize(analysis_fn, analysis_settings_);

            inline_candidate candidate;
            const bool few_use_candidate =
                count_it->second > 1 && count_it->second <= 3;
            const int allowed_ops =
                count_it->second == 1
                    ? single_use_max_inline_ops_
                    : (widen_few_use_analysis_ && few_use_candidate
                           ? std::max(max_inline_ops_, few_use_max_inline_ops_)
                           : max_inline_ops_);
            if (!analyze_inline_candidate(analysis_fn, candidate, allowed_ops)) {
                continue;
            }

            std::vector<std::pair<size_t, size_t>> sites;
            int total_overhead = 0;
            if (!collect_inline_sites(mod, callee.name, sites, total_overhead)) {
                continue;
            }
            if (sites.empty() || static_cast<int>(sites.size()) != count_it->second) {
                continue;
            }

            const int use_count = static_cast<int>(sites.size());
            const bool speed_profile =
                speed_inline_small_helpers_ || broad_speed_inline_;
            const bool speed_safe_leaf_helper =
                !candidate.contains_call &&
                candidate.local_frame_bytes == 0 &&
                !candidate_has_internal_control_flow(candidate) &&
                candidate.op_count <= few_use_max_inline_ops_;
            if (speed_profile && !speed_safe_leaf_helper) {
                continue;
            }

            const bool caller_already_has_locals =
                use_count == 1 &&
                mod.functions[sites.front().first].local_bytes >= 8;
            // Promotion can remove the analyzed frame without removing the
            // register pressure that made a stack-local CFG helper costly to
            // inline into an already-large caller.
            const bool callee_had_stack_locals = callee.local_bytes > 0;
            const bool stack_local_cfg_candidate =
                use_count == 1 &&
                (candidate.local_frame_bytes > 0 || callee_had_stack_locals) &&
                caller_already_has_locals &&
                candidate_has_internal_control_flow(candidate);
            if (stack_local_cfg_candidate) {
                continue;
            }

            if (use_count > 1) {
                // Repeatedly inlining larger pure arithmetic helpers often
                // looks profitable in raw IR, but it can bloat final Z80
                // code by increasing frame pressure and duplicating rotate /
                // mix chains at every site. Keep that path very narrow and
                // allow a broader tuned-profile repeated-inline budget for
                // non-pure-leaf helpers that can remove larger call/receive
                // scaffolding.
                const bool allow_repeated_pure_leaf_inline =
                    candidate.pure_leaf_arith &&
                    candidate.op_count <= 4 &&
                    use_count <= 2;
                const bool allow_broader_tuned_inline =
                    widen_few_use_analysis_ &&
                    !candidate.pure_leaf_arith &&
                    !candidate.contains_call &&
                    use_count <= 6 &&
                    candidate.op_count <= 20;
                const bool allow_small_repeated_helper_inline =
                    !candidate.pure_leaf_arith &&
                    !candidate.contains_call &&
                    use_count <= 3 &&
                    candidate.op_count <= few_use_max_inline_ops_;
                const bool allow_speed_profile_inline =
                    speed_inline_small_helpers_ &&
                    !candidate.contains_call &&
                    candidate.local_frame_bytes == 0 &&
                    !candidate_has_internal_control_flow(candidate) &&
                    use_count <= 2 &&
                    candidate.op_count <= 18;
                const bool allow_broad_speed_inline =
                    broad_speed_inline_ &&
                    !candidate.contains_call &&
                    candidate.local_frame_bytes == 0 &&
                    !candidate_has_internal_control_flow(candidate) &&
                    use_count <= 4 &&
                    candidate.op_count <= few_use_max_inline_ops_;
                const bool allow_few_use_inline =
                    allow_repeated_pure_leaf_inline ||
                    allow_broader_tuned_inline ||
                    allow_small_repeated_helper_inline;
                const bool bypass_size_profit_gate =
                    allow_speed_profile_inline ||
                    allow_broad_speed_inline;
                if (!allow_few_use_inline && !bypass_size_profit_gate) {
                    continue;
                }

                if (!bypass_size_profit_gate) {
                    const int replicated_body_cost =
                        (use_count - 1) * candidate.approx_inline_cost;
                    const int total_savings =
                        total_overhead +
                        (candidate.pure_leaf_arith ? candidate.helper_definition_bonus : 0);
                    if (total_savings <= replicated_body_cost) {
                        continue;
                    }
                }
            }

            auto best_site = sites.front();
            int best_site_overhead = std::numeric_limits<int>::max();
            for (const auto &[caller_idx, call_idx] : sites) {
                const auto &caller = mod.functions[caller_idx];
                const auto &call_ic = caller.icodes[call_idx];
                size_t send_begin = call_idx;
                std::vector<operand> args;
                if (!collect_call_args(caller.icodes, call_idx, call_ic, send_begin, args))
                    continue;
                int site_overhead = static_cast<int>(call_idx - send_begin + 1);
                if (site_overhead < best_site_overhead) {
                    best_site_overhead = site_overhead;
                    best_site = {caller_idx, call_idx};
                }
            }

            if (best_site_overhead == std::numeric_limits<int>::max()) {
                continue;
            }

            choice current;
            current.caller_idx = best_site.first;
            current.call_idx = best_site.second;
            current.profitability_margin =
                total_overhead +
                (candidate.pure_leaf_arith ? candidate.helper_definition_bonus : 0) -
                (use_count - 1) * candidate.approx_inline_cost;
            current.site_overhead = best_site_overhead;
            current.candidate = candidate;

            if (!best ||
                current.profitability_margin > best->profitability_margin ||
                (current.profitability_margin == best->profitability_margin &&
                 current.site_overhead < best->site_overhead)) {
                best = std::move(current);
            }
        }

        if (!best)
            return false;
        return inline_call_site(mod.functions[best->caller_idx],
                                best->call_idx,
                                best->candidate);
    }

private:
    int max_inline_ops_ = 12;
    int single_use_max_inline_ops_ = 24;
    int few_use_max_inline_ops_ = 16;
    bool widen_few_use_analysis_ = false;
    bool speed_inline_small_helpers_ = false;
    bool broad_speed_inline_ = false;
    optimization_settings analysis_settings_;
};

} // namespace

std::vector<std::unique_ptr<ir_module_pass>>
ir_module_optimizer::build_pipeline(const optimization_settings &settings) {
    std::vector<std::unique_ptr<ir_module_pass>> passes;
    if (settings.dead_static_functions)
        passes.push_back(std::make_unique<dead_internal_function_pass>());
    if (settings.const_arg_propagation)
        passes.push_back(std::make_unique<constant_arg_propagation_pass>());
    if (settings.const_call_eval) {
        passes.push_back(
            std::make_unique<const_call_eval_pass>(true));
        passes.push_back(std::make_unique<single_const_local_propagation_pass>());
    }
    if (settings.function_const_eval)
        passes.push_back(std::make_unique<function_const_eval_pass>());
    if (settings.dead_params)
        passes.push_back(std::make_unique<dead_param_elimination_pass>());
    if (settings.internal_call_abi_promotion)
        passes.push_back(
            std::make_unique<internal_call_abi_promotion_pass>());
    if (settings.tail_recursion_elim)
        passes.push_back(std::make_unique<tail_address_noescape_pass>());
    if (settings.inline_trivial_internal_functions)
        passes.push_back(std::make_unique<trivial_internal_leaf_inline_pass>(
            settings));
    if (settings.inline_static_functions) {
        passes.push_back(std::make_unique<size_profitable_static_inline_pass>(
            12,
            24,
            16,
            false,
            false,
            false,
            settings));
    }
    if (settings.merge_identical_functions)
        passes.push_back(std::make_unique<merge_identical_helpers_pass>());
    if (settings.dead_static_functions)
        passes.push_back(std::make_unique<dead_internal_function_pass>());
    return passes;
}

void ir_module_optimizer::optimize(ir_module &mod,
                                   const optimization_settings &settings) {
    auto passes = build_pipeline(settings);
    size_t total_icodes = 0;
    for (const auto &fn : mod.functions)
        total_icodes += fn.icodes.size();

    // Some module passes, especially the size-oriented inliner, intentionally
    // rewrite one profitable site at a time. Small fixed limits can stop the
    // pipeline before it reaches later call sites in helper-heavy translation
    // units, so scale the fixed-point budget with module size.
    const size_t max_rounds =
        std::max<size_t>(64, std::min<size_t>(512, total_icodes + passes.size()));
    bool changed = true;
    size_t guard = 0;
    while (changed && guard++ < max_rounds) {
        changed = false;
        for (auto &pass : passes)
            changed = pass->run(mod) || changed;
    }
}

} // namespace xcc
