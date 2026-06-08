//
// iropt.cpp — IR-level optimizer for xcc.
//
// The -O2 pipeline is organized as a sequence of IR passes over each
// function. The passes build CFG and loop information as needed, track
// values in an SSA-style lattice keyed by reaching definitions, and
// keep all target-specific peephole work in the Z80 backend.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//
#include "opt/iropt.h"
#include "backend/z80/convention.h"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace xcc {
namespace {

struct alias_info {
    std::unordered_set<std::string> address_taken_symbols;
    std::unordered_set<std::string> address_taken_bases;
};

struct ssa_value {
    enum class kind : uint8_t {
        UNKNOWN,
        INT_CONST,
        VALUE,
    };

    kind    tag      = kind::UNKNOWN;
    int64_t ival     = 0;
    int     value_id = -1;

    static ssa_value unknown() { return {}; }

    static ssa_value int_const(int64_t v) {
        ssa_value value;
        value.tag  = kind::INT_CONST;
        value.ival = v;
        return value;
    }

    static ssa_value value_ref(int id) {
        ssa_value value;
        value.tag      = kind::VALUE;
        value.value_id = id;
        return value;
    }

    bool operator==(const ssa_value &other) const {
        if (tag != other.tag) return false;
        if (tag == kind::INT_CONST) return ival == other.ival;
        if (tag == kind::VALUE)     return value_id == other.value_id;
        return true;
    }

    bool operator!=(const ssa_value &other) const { return !(*this == other); }
};

using ssa_env = std::unordered_map<std::string, ssa_value>;

struct basic_block {
    size_t              id    = 0;
    size_t              begin = 0;
    size_t              end   = 0;
    std::vector<size_t> preds;
    std::vector<size_t> succs;
};

struct natural_loop {
    size_t                    header = 0;
    std::unordered_set<size_t> blocks;
    std::vector<size_t>       latches;
    std::vector<size_t>       outside_preds;
};

static bool is_terminator(icode_op op) {
    switch (op) {
    case icode_op::GOTO:
    case icode_op::IFX:
    case icode_op::RETURN:
        return true;
    default:
        return false;
    }
}

static bool is_binary_foldable(icode_op op) {
    switch (op) {
    case icode_op::ADD: case icode_op::SUB: case icode_op::MUL:
    case icode_op::DIV: case icode_op::MOD:
    case icode_op::BAND: case icode_op::BOR: case icode_op::BXOR:
    case icode_op::SHL: case icode_op::SHR:
    case icode_op::ROL: case icode_op::ROR:
    case icode_op::PACK_BYTES:
    case icode_op::EQ: case icode_op::NE:
    case icode_op::LT: case icode_op::LE: case icode_op::GT: case icode_op::GE:
        return true;
    default:
        return false;
    }
}

static bool is_compare_op(icode_op op) {
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

static bool is_removable_if_dead(icode_op op) {
    switch (op) {
    case icode_op::RECEIVE:
    case icode_op::ASSIGN:
    case icode_op::ADDRESS_OF:
    case icode_op::GET_VALUE_AT:
    case icode_op::ADD:  case icode_op::SUB:  case icode_op::NEG:
    case icode_op::MUL:  case icode_op::DIV:  case icode_op::MOD:
    case icode_op::BAND: case icode_op::BOR:  case icode_op::BXOR: case icode_op::BNOT:
    case icode_op::SHL:  case icode_op::SHR:
    case icode_op::ROL:  case icode_op::ROR:
    case icode_op::PACK_BYTES:
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

static bool is_licm_safe(icode_op op) {
    switch (op) {
    case icode_op::ASSIGN:
    case icode_op::ADDRESS_OF:
    case icode_op::ADD:  case icode_op::SUB:  case icode_op::NEG:
    case icode_op::BAND: case icode_op::BOR:  case icode_op::BXOR: case icode_op::BNOT:
    case icode_op::SHL:  case icode_op::SHR:
    case icode_op::ROL:  case icode_op::ROR:
    case icode_op::EQ:   case icode_op::NE:
    case icode_op::LT:   case icode_op::LE:
    case icode_op::GT:   case icode_op::GE:
    case icode_op::CAST:
        return true;
    default:
        return false;
    }
}

static int64_t fold_binary(icode_op op, int64_t l, int64_t r) {
    auto rotl = [](uint64_t v, unsigned sh, unsigned bits) -> uint64_t {
        const uint64_t mask = bits >= 64 ? ~0ULL : ((1ULL << bits) - 1ULL);
        sh &= (bits - 1U);
        v &= mask;
        return ((v << sh) | (v >> ((bits - sh) & (bits - 1U)))) & mask;
    };
    auto rotr = [&](uint64_t v, unsigned sh, unsigned bits) -> uint64_t {
        const uint64_t mask = bits >= 64 ? ~0ULL : ((1ULL << bits) - 1ULL);
        sh &= (bits - 1U);
        v &= mask;
        return ((v >> sh) | (v << ((bits - sh) & (bits - 1U)))) & mask;
    };
    switch (op) {
    case icode_op::ADD:  return l + r;
    case icode_op::SUB:  return l - r;
    case icode_op::MUL:  return l * r;
    case icode_op::DIV:  return r ? l / r : 0;
    case icode_op::MOD:  return r ? l % r : 0;
    case icode_op::BAND: return l & r;
    case icode_op::BOR:  return l | r;
    case icode_op::BXOR: return l ^ r;
    case icode_op::SHL:  return l << (r & 63);
    case icode_op::SHR:  return l >> (r & 63);
    case icode_op::ROL:  return static_cast<int64_t>(rotl(static_cast<uint64_t>(l), static_cast<unsigned>(r), 16));
    case icode_op::ROR:  return static_cast<int64_t>(rotr(static_cast<uint64_t>(l), static_cast<unsigned>(r), 16));
    case icode_op::PACK_BYTES: return (l & 0xff) | ((r & 0xff) << 8);
    case icode_op::EQ:   return l == r ? 1 : 0;
    case icode_op::NE:   return l != r ? 1 : 0;
    case icode_op::LT:   return l <  r ? 1 : 0;
    case icode_op::LE:   return l <= r ? 1 : 0;
    case icode_op::GT:   return l >  r ? 1 : 0;
    case icode_op::GE:   return l >= r ? 1 : 0;
    default:             return 0;
    }
}

static int64_t cast_int_value(int64_t v, const type_ptr &type) {
    if (!type) return v;
    const bool is_unsigned = type->is_unsigned() || type->is_ptr();
    switch (type->size()) {
    case 1:
        return is_unsigned
                   ? static_cast<int64_t>(static_cast<uint8_t>(v & 0xFF))
                   : static_cast<int64_t>(static_cast<int8_t>(v & 0xFF));
    case 2:
        return is_unsigned
                   ? static_cast<int64_t>(static_cast<uint16_t>(v & 0xFFFF))
                   : static_cast<int64_t>(static_cast<int16_t>(v & 0xFFFF));
    case 4:
        return is_unsigned
                   ? static_cast<int64_t>(static_cast<uint32_t>(v & 0xFFFFFFFFLL))
                   : static_cast<int64_t>(static_cast<int32_t>(v & 0xFFFFFFFFLL));
    default: return v;
    }
}

static bool same_value_operand(const operand &a, const operand &b) {
    if (a.kind != b.kind)
        return false;
    if (a.byte_offset != b.byte_offset)
        return false;
    switch (a.kind) {
    case operand_kind::TEMP:
        return a.temp_id == b.temp_id;
    case operand_kind::SYMBOL:
        return a.name == b.name &&
               a.is_global == b.is_global &&
               a.is_param == b.is_param &&
               a.stack_offset == b.stack_offset &&
               a.is_tls == b.is_tls;
    case operand_kind::INT_CONST:
        return a.ival == b.ival;
    case operand_kind::LABEL_REF:
        return a.name == b.name;
    case operand_kind::NONE:
        return true;
    default:
        return false;
    }
}

static bool is_noop_scalar_cast(const icode &ic) {
    if (ic.op != icode_op::CAST || !ic.left.type || !ic.result.type)
        return false;
    type_ptr src = ic.left.type->unqual();
    type_ptr dst = ic.result.type->unqual();
    if (!src || !dst)
        return false;
    if (src->size() != dst->size())
        return false;
    if (src->is_integer() && dst->is_integer())
        return true;
    if (src->is_ptr() && dst->is_ptr())
        return true;
    return false;
}

static bool same_type_shape(const type_ptr &lhs, const type_ptr &rhs) {
    if (!lhs && !rhs)
        return true;
    if (!lhs || !rhs)
        return false;
    return lhs->to_string() == rhs->to_string();
}

static bool is_local_cse_barrier(const icode &ic);

static bool env_equal(const ssa_env &lhs, const ssa_env &rhs) {
    if (lhs.size() != rhs.size()) return false;
    for (auto &it : lhs) {
        auto found = rhs.find(it.first);
        if (found == rhs.end()) return false;
        if (found->second != it.second) return false;
    }
    return true;
}

static int log2_exact(int64_t v) {
    if (v <= 0 || (v & (v - 1)) != 0) return -1;
    int n = 0;
    while (v > 1) {
        v >>= 1;
        ++n;
    }
    return n;
}

static int popcount_u64(uint64_t v) {
    int count = 0;
    while (v) {
        v &= (v - 1);
        ++count;
    }
    return count;
}

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

static operand make_fresh_temp(int &next_temp, type_ptr type) {
    return operand::make_temp(next_temp++, type ? type : type::make_int());
}

static std::string symbol_key(const operand &op) {
    if (!op.is_symbol()) return {};
    return op.name + "|" +
           (op.is_global ? "g" : "l") + "|" +
           (op.is_param ? "p" : "v") + "|" +
           std::to_string(op.stack_offset) + "|" +
           std::to_string(op.byte_offset) + "|" +
           (op.is_tls ? "t" : "n") + "|" +
           (op.is_sfr ? "s" : "n");
}

static std::string base_symbol_key(const operand &op) {
    if (!op.is_symbol()) return {};
    operand base = op;
    base.byte_offset = 0;
    return symbol_key(base);
}

static alias_info build_alias_info(const ir_function &fn) {
    alias_info info;
    for (auto &ic : fn.icodes) {
        if (ic.op == icode_op::ADDRESS_OF && ic.left.is_symbol()) {
            info.address_taken_symbols.insert(symbol_key(ic.left));
            info.address_taken_bases.insert(base_symbol_key(ic.left));
        }
    }
    return info;
}

static bool base_symbol_address_taken(const alias_info &alias, const operand &op) {
    if (!op.is_symbol()) return false;
    std::string base_key = base_symbol_key(op);
    if (base_key.empty()) return false;
    return alias.address_taken_bases.find(base_key) !=
           alias.address_taken_bases.end();
}

static bool fits_u8_like_value(const operand &op) {
    if (op.kind == operand_kind::INT_CONST)
        return op.ival >= 0 && op.ival <= 0xff;
    return op.type && op.type->size() == 1 && op.type->is_unsigned();
}

static bool is_compare_opcode(icode_op op) {
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

static bool is_trackable_symbol(const operand &op, const alias_info &alias) {
    if (!op.is_symbol()) return false;
    if (op.is_global || op.is_tls || op.is_sfr) return false;
    if (op.byte_offset != 0) return false;
    return !base_symbol_address_taken(alias, op);
}

static std::string trackable_key(const operand &op, const alias_info &alias) {
    if (op.is_temp()) return "t:" + std::to_string(op.temp_id);
    if (is_trackable_symbol(op, alias)) return "s:" + symbol_key(op);
    return {};
}

static ssa_value lookup_value(const operand &op, const ssa_env &env,
                              const alias_info &alias) {
    if (op.kind == operand_kind::INT_CONST)
        return ssa_value::int_const(op.ival);
    std::string key = trackable_key(op, alias);
    if (key.empty()) return ssa_value::unknown();
    auto it = env.find(key);
    return it != env.end() ? it->second : ssa_value::unknown();
}

static uint64_t integer_mask_for_identity_type(const type_ptr &type) {
    if (!type)
        return 0xFFFFu;

    const int bytes = type->size();
    if (bytes <= 0)
        return 0xFFFFu;
    if (bytes >= 8)
        return ~0ULL;

    return (1ULL << (bytes * 8)) - 1ULL;
}

enum class ir_const_side : uint8_t {
    left,
    right,
    either,
};

enum class ir_const_predicate : uint8_t {
    is_zero,
    is_one,
    is_all_ones,
};

enum class ir_identity_action : uint8_t {
    keep_other_operand,
    replace_with_zero,
};

struct ir_binary_identity_rule {
    const char *name = "";
    icode_op op = icode_op::ASSIGN;
    ir_const_side const_side = ir_const_side::either;
    ir_const_predicate predicate = ir_const_predicate::is_zero;
    ir_identity_action action = ir_identity_action::keep_other_operand;
};

static const ir_binary_identity_rule k_ir_binary_identity_rules[] = {
    {"add_zero",       icode_op::ADD,  ir_const_side::either, ir_const_predicate::is_zero,     ir_identity_action::keep_other_operand},
    {"sub_zero",       icode_op::SUB,  ir_const_side::right,  ir_const_predicate::is_zero,     ir_identity_action::keep_other_operand},
    {"mul_zero",       icode_op::MUL,  ir_const_side::either, ir_const_predicate::is_zero,     ir_identity_action::replace_with_zero},
    {"mul_one",        icode_op::MUL,  ir_const_side::either, ir_const_predicate::is_one,      ir_identity_action::keep_other_operand},
    {"div_one",        icode_op::DIV,  ir_const_side::right,  ir_const_predicate::is_one,      ir_identity_action::keep_other_operand},
    {"band_zero",      icode_op::BAND, ir_const_side::either, ir_const_predicate::is_zero,     ir_identity_action::replace_with_zero},
    {"band_all_ones",  icode_op::BAND, ir_const_side::either, ir_const_predicate::is_all_ones, ir_identity_action::keep_other_operand},
    {"bor_zero",       icode_op::BOR,  ir_const_side::either, ir_const_predicate::is_zero,     ir_identity_action::keep_other_operand},
    {"bxor_zero",      icode_op::BXOR, ir_const_side::either, ir_const_predicate::is_zero,     ir_identity_action::keep_other_operand},
    {"shl_zero",       icode_op::SHL,  ir_const_side::right,  ir_const_predicate::is_zero,     ir_identity_action::keep_other_operand},
    {"shr_zero",       icode_op::SHR,  ir_const_side::right,  ir_const_predicate::is_zero,     ir_identity_action::keep_other_operand},
};

static bool matches_ir_const_predicate(ir_const_predicate predicate,
                                       int64_t value,
                                       const type_ptr &type) {
    switch (predicate) {
    case ir_const_predicate::is_zero:
        return value == 0;
    case ir_const_predicate::is_one:
        return value == 1;
    case ir_const_predicate::is_all_ones:
        return (static_cast<uint64_t>(value) &
                integer_mask_for_identity_type(type)) ==
               integer_mask_for_identity_type(type);
    }
    return false;
}

template<typename Apply>
static bool apply_ir_binary_identity_rules(icode_op op,
                                           bool left_const, int64_t left_value,
                                           bool right_const, int64_t right_value,
                                           const type_ptr &result_type,
                                           Apply apply) {
    for (const auto &rule : k_ir_binary_identity_rules) {
        if (rule.op != op)
            continue;

        auto maybe_apply = [&](bool const_on_left, int64_t const_value) {
            if (!matches_ir_const_predicate(rule.predicate, const_value,
                                            result_type))
                return false;
            return apply(rule, const_on_left);
        };

        switch (rule.const_side) {
        case ir_const_side::left:
            if (left_const && maybe_apply(true, left_value))
                return true;
            break;
        case ir_const_side::right:
            if (right_const && maybe_apply(false, right_value))
                return true;
            break;
        case ir_const_side::either:
            if (left_const && maybe_apply(true, left_value))
                return true;
            if (right_const && maybe_apply(false, right_value))
                return true;
            break;
        }
    }
    return false;
}

static ssa_value simplify_binary_value(const icode &ic,
                                       const ssa_value &lhs,
                                       const ssa_value &rhs,
                                       int value_id) {
    if (lhs.tag == ssa_value::kind::INT_CONST &&
        rhs.tag == ssa_value::kind::INT_CONST &&
        is_binary_foldable(ic.op)) {
        return ssa_value::int_const(fold_binary(ic.op, lhs.ival, rhs.ival));
    }

    const bool left_const  = lhs.tag == ssa_value::kind::INT_CONST;
    const bool right_const = rhs.tag == ssa_value::kind::INT_CONST;
    if (left_const || right_const) {
        std::optional<ssa_value> simplified;
        apply_ir_binary_identity_rules(
            ic.op,
            left_const, lhs.ival,
            right_const, rhs.ival,
            ic.result.type,
            [&](const ir_binary_identity_rule &rule, bool const_on_left) {
                const ssa_value &other = const_on_left ? rhs : lhs;
                switch (rule.action) {
                case ir_identity_action::keep_other_operand:
                    simplified = other;
                    return true;
                case ir_identity_action::replace_with_zero:
                    simplified = ssa_value::int_const(0);
                    return true;
                }
                return false;
            });
        if (simplified.has_value())
            return *simplified;
    }

    if (lhs.tag == ssa_value::kind::VALUE &&
        rhs.tag == ssa_value::kind::VALUE &&
        lhs.value_id == rhs.value_id) {
        switch (ic.op) {
        case icode_op::EQ:
        case icode_op::LE:
        case icode_op::GE:
            return ssa_value::int_const(1);
        case icode_op::NE:
        case icode_op::LT:
        case icode_op::GT:
        case icode_op::SUB:
        case icode_op::BXOR:
            return ssa_value::int_const(0);
        default:
            break;
        }
    }

    return ssa_value::value_ref(value_id);
}

static ssa_value evaluate_defined_value(const icode &ic, size_t index,
                                        const ssa_env &env,
                                        const alias_info &alias) {
    const int value_id = static_cast<int>(index) + 1;
    ssa_value lhs = lookup_value(ic.left, env, alias);
    ssa_value rhs = lookup_value(ic.right, env, alias);

    switch (ic.op) {
    case icode_op::RECEIVE:
    case icode_op::CALL:
    case icode_op::GET_VALUE_AT:
    case icode_op::ALLOCA:
    case icode_op::MAKE_COMPLEX:
    case icode_op::ADDRESS_OF:
    case icode_op::FADD: case icode_op::FSUB:
    case icode_op::FMUL: case icode_op::FDIV:
    case icode_op::FITOSF: case icode_op::FSTOI:
        return ssa_value::value_ref(value_id);

    case icode_op::ASSIGN:
        if (ic.result.is_symbol()) {
            if (lhs.tag == ssa_value::kind::INT_CONST) return lhs;
            return ssa_value::value_ref(value_id);
        }
        if (lhs.tag != ssa_value::kind::UNKNOWN) return lhs;
        return ssa_value::value_ref(value_id);

    case icode_op::NEG:
        if (lhs.tag == ssa_value::kind::INT_CONST)
            return ssa_value::int_const(-lhs.ival);
        return ssa_value::value_ref(value_id);

    case icode_op::BNOT:
        if (lhs.tag == ssa_value::kind::INT_CONST)
            return ssa_value::int_const(~lhs.ival);
        return ssa_value::value_ref(value_id);

    case icode_op::CAST:
        if (lhs.tag == ssa_value::kind::INT_CONST)
            return ssa_value::int_const(cast_int_value(lhs.ival, ic.result.type));
        return ssa_value::value_ref(value_id);

    default:
        if (is_binary_foldable(ic.op))
            return simplify_binary_value(ic, lhs, rhs, value_id);
        return ssa_value::value_ref(value_id);
    }
}

template<typename Fn>
static void for_each_use_operand(icode &ic, Fn fn) {
    switch (ic.op) {
    case icode_op::LABEL:
    case icode_op::GOTO:
    case icode_op::FUNCTION:
    case icode_op::ENDFUNCTION:
    case icode_op::RECEIVE:
        break;
    case icode_op::IFX:
    case icode_op::RETURN:
    case icode_op::SEND:
    case icode_op::ALLOCA:
        if (!ic.left.is_none()) fn(ic.left);
        break;
    case icode_op::CALL:
        if (!ic.left.is_none()) fn(ic.left);
        break;
    case icode_op::ADDRESS_OF:
        break;
    case icode_op::SET_VALUE_AT:
        if (!ic.result.is_none()) fn(ic.result);
        if (!ic.left.is_none())   fn(ic.left);
        break;
    case icode_op::INLINE_ASM:
        break;
    default:
        if (!ic.left.is_none())  fn(ic.left);
        if (!ic.right.is_none()) fn(ic.right);
        break;
    }
}

template<typename Fn>
static void for_each_use_operand(const icode &ic, Fn fn) {
    switch (ic.op) {
    case icode_op::LABEL:
    case icode_op::GOTO:
    case icode_op::FUNCTION:
    case icode_op::ENDFUNCTION:
    case icode_op::RECEIVE:
        break;
    case icode_op::IFX:
    case icode_op::RETURN:
    case icode_op::SEND:
    case icode_op::ALLOCA:
        if (!ic.left.is_none()) fn(ic.left);
        break;
    case icode_op::CALL:
        if (!ic.left.is_none()) fn(ic.left);
        break;
    case icode_op::ADDRESS_OF:
        break;
    case icode_op::SET_VALUE_AT:
        if (!ic.result.is_none()) fn(ic.result);
        if (!ic.left.is_none())   fn(ic.left);
        break;
    case icode_op::INLINE_ASM:
        break;
    default:
        if (!ic.left.is_none())  fn(ic.left);
        if (!ic.right.is_none()) fn(ic.right);
        break;
    }
}

static bool defines_result(const icode &ic) {
    switch (ic.op) {
    case icode_op::LABEL:
    case icode_op::GOTO:
    case icode_op::IFX:
    case icode_op::FUNCTION:
    case icode_op::ENDFUNCTION:
    case icode_op::RETURN:
    case icode_op::SEND:
    case icode_op::SET_VALUE_AT:
    case icode_op::INLINE_ASM:
        return false;
    default:
        return !ic.result.is_none();
    }
}

static std::string def_key(const icode &ic, const alias_info &alias) {
    return defines_result(ic) ? trackable_key(ic.result, alias) : std::string();
}

class control_flow_graph {
public:
    explicit control_flow_graph(const ir_function &fn) : fn_(fn) { build(); }

    const std::vector<basic_block> &blocks() const { return blocks_; }

    const basic_block &block(size_t id) const { return blocks_[id]; }

    std::optional<size_t> block_for_label(const std::string &label) const {
        auto it = label_to_block_.find(label);
        if (it == label_to_block_.end())
            return std::nullopt;
        return it->second;
    }

    std::unordered_set<size_t> reachable_blocks() const {
        std::unordered_set<size_t> reachable;
        if (blocks_.empty()) return reachable;
        std::vector<size_t> stack{0};
        reachable.insert(0);
        while (!stack.empty()) {
            size_t cur = stack.back();
            stack.pop_back();
            for (size_t succ : blocks_[cur].succs) {
                if (reachable.insert(succ).second)
                    stack.push_back(succ);
            }
        }
        return reachable;
    }

    std::vector<size_t> reverse_postorder() const {
        std::vector<size_t> order;
        if (blocks_.empty()) return order;
        std::unordered_set<size_t> visited;
        dfs_rpo(0, visited, order);
        std::reverse(order.begin(), order.end());
        return order;
    }

    std::vector<std::unordered_set<size_t>> dominators() const {
        std::vector<std::unordered_set<size_t>> dom(blocks_.size());
        auto reachable = reachable_blocks();
        std::unordered_set<size_t> all = reachable;

        for (auto &block : blocks_) {
            if (!reachable.count(block.id)) continue;
            if (block.id == 0) dom[block.id] = {0};
            else               dom[block.id] = all;
        }

        bool changed;
        do {
            changed = false;
            for (auto &block : blocks_) {
                if (block.id == 0 || !reachable.count(block.id)) continue;
                bool first = true;
                std::unordered_set<size_t> new_dom;
                for (size_t pred : block.preds) {
                    if (!reachable.count(pred)) continue;
                    if (first) {
                        new_dom = dom[pred];
                        first = false;
                    } else {
                        std::unordered_set<size_t> merged;
                        for (size_t id : new_dom)
                            if (dom[pred].count(id)) merged.insert(id);
                        new_dom = std::move(merged);
                    }
                }
                new_dom.insert(block.id);
                if (new_dom != dom[block.id]) {
                    dom[block.id] = std::move(new_dom);
                    changed = true;
                }
            }
        } while (changed);
        return dom;
    }

    std::vector<natural_loop> natural_loops() const {
        std::map<size_t, natural_loop> by_header;
        auto reachable = reachable_blocks();
        auto dom = dominators();

        for (auto &block : blocks_) {
            if (!reachable.count(block.id)) continue;
            for (size_t succ : block.succs) {
                if (!reachable.count(succ)) continue;
                if (!dom[block.id].count(succ)) continue;

                natural_loop &loop = by_header[succ];
                loop.header = succ;
                loop.blocks.insert(succ);
                if (std::find(loop.latches.begin(), loop.latches.end(), block.id) ==
                    loop.latches.end())
                    loop.latches.push_back(block.id);

                std::vector<size_t> stack{block.id};
                while (!stack.empty()) {
                    size_t cur = stack.back();
                    stack.pop_back();
                    if (!loop.blocks.insert(cur).second) continue;
                    for (size_t pred : blocks_[cur].preds)
                        if (!loop.blocks.count(pred)) stack.push_back(pred);
                }
            }
        }

        std::vector<natural_loop> loops;
        for (auto &it : by_header) {
            natural_loop loop = std::move(it.second);
            for (size_t pred : blocks_[loop.header].preds) {
                if (!loop.blocks.count(pred))
                    loop.outside_preds.push_back(pred);
            }
            std::sort(loop.latches.begin(), loop.latches.end());
            std::sort(loop.outside_preds.begin(), loop.outside_preds.end());
            loops.push_back(std::move(loop));
        }

        std::sort(loops.begin(), loops.end(),
                  [](const natural_loop &lhs, const natural_loop &rhs) {
                      if (lhs.blocks.size() != rhs.blocks.size())
                          return lhs.blocks.size() < rhs.blocks.size();
                      return lhs.header < rhs.header;
                  });
        return loops;
    }

private:
    void build() {
        if (fn_.icodes.empty()) return;

        std::set<size_t> starts;
        starts.insert(0);
        for (size_t i = 0; i < fn_.icodes.size(); ++i) {
            if (fn_.icodes[i].op == icode_op::LABEL)
                starts.insert(i);
            if (i + 1 < fn_.icodes.size() && is_terminator(fn_.icodes[i].op))
                starts.insert(i + 1);
        }

        std::vector<size_t> ordered(starts.begin(), starts.end());
        for (size_t i = 0; i < ordered.size(); ++i) {
            basic_block block;
            block.id    = i;
            block.begin = ordered[i];
            block.end   = (i + 1 < ordered.size()) ? ordered[i + 1] : fn_.icodes.size();
            blocks_.push_back(block);
        }

        for (auto &block : blocks_) {
            for (size_t i = block.begin; i < block.end; ++i) {
                if (fn_.icodes[i].op == icode_op::LABEL)
                    label_to_block_[fn_.icodes[i].label_name] = block.id;
                if (fn_.icodes[i].op == icode_op::ENDFUNCTION)
                    end_block_ = block.id;
            }
        }

        for (auto &block : blocks_) {
            if (block.begin >= block.end) continue;
            const icode &term = fn_.icodes[block.end - 1];
            auto add_edge = [&](const std::string &label) {
                auto found = label_to_block_.find(label);
                if (found != label_to_block_.end())
                    block.succs.push_back(found->second);
            };

            switch (term.op) {
            case icode_op::GOTO:
                add_edge(term.label_name);
                break;
            case icode_op::IFX:
                add_edge(term.true_lbl);
                if (!term.false_lbl.empty())
                    add_edge(term.false_lbl);
                else if (block.id + 1 < blocks_.size())
                    block.succs.push_back(block.id + 1);
                break;
            case icode_op::RETURN:
                if (end_block_ != static_cast<size_t>(-1) && end_block_ != block.id)
                    block.succs.push_back(end_block_);
                break;
            default:
                if (block.id + 1 < blocks_.size())
                    block.succs.push_back(block.id + 1);
                break;
            }

            std::sort(block.succs.begin(), block.succs.end());
            block.succs.erase(std::unique(block.succs.begin(), block.succs.end()),
                              block.succs.end());
        }

        for (auto &block : blocks_) {
            for (size_t succ : block.succs)
                blocks_[succ].preds.push_back(block.id);
        }
    }

    void dfs_rpo(size_t block_id, std::unordered_set<size_t> &visited,
                 std::vector<size_t> &order) const {
        if (!visited.insert(block_id).second) return;
        for (size_t succ : blocks_[block_id].succs)
            dfs_rpo(succ, visited, order);
        order.push_back(block_id);
    }

    const ir_function &fn_;
    std::vector<basic_block> blocks_;
    std::unordered_map<std::string, size_t> label_to_block_;
    size_t end_block_ = static_cast<size_t>(-1);
};

static size_t insertion_index_before_terminator(const basic_block &block,
                                                const ir_function &fn) {
    if (block.begin < block.end && is_terminator(fn.icodes[block.end - 1].op))
        return block.end - 1;
    return block.end;
}

static std::vector<icode> rebuild_with_insertions(
        const std::vector<icode> &input,
        const std::unordered_set<size_t> &skip,
        const std::map<size_t, std::vector<icode>> &insert_before) {
    std::vector<icode> output;
    output.reserve(input.size() + 8);

    for (size_t i = 0; i <= input.size(); ++i) {
        auto add = insert_before.find(i);
        if (add != insert_before.end())
            output.insert(output.end(), add->second.begin(), add->second.end());
        if (i == input.size()) break;
        if (!skip.count(i))
            output.push_back(input[i]);
    }
    return output;
}

static std::vector<icode> rebuild_reachable_blocks(
        const ir_function &fn,
        const control_flow_graph &cfg,
        const std::unordered_set<size_t> &reachable) {
    std::vector<icode> output;
    for (auto &block : cfg.blocks()) {
        if (!reachable.count(block.id)) continue;
        for (size_t i = block.begin; i < block.end; ++i)
            output.push_back(fn.icodes[i]);
    }
    return output;
}

static std::string first_label_in_block(const ir_function &fn,
                                        const basic_block &block) {
    for (size_t i = block.begin; i < block.end; ++i) {
        if (fn.icodes[i].op != icode_op::LABEL)
            break;
        return fn.icodes[i].label_name;
    }
    return {};
}

static std::unordered_map<std::string, std::string>
build_label_redirects(const ir_function &fn, const control_flow_graph &cfg) {
    std::unordered_map<std::string, std::string> redirects;

    for (const auto &block : cfg.blocks()) {
        std::vector<std::string> labels;
        size_t first_non_label = block.begin;
        while (first_non_label < block.end &&
               fn.icodes[first_non_label].op == icode_op::LABEL) {
            labels.push_back(fn.icodes[first_non_label].label_name);
            ++first_non_label;
        }

        if (labels.empty())
            continue;

        std::string target;
        if (first_non_label == block.end) {
            if (block.id + 1 < cfg.blocks().size())
                target = first_label_in_block(fn, cfg.block(block.id + 1));
        } else if (first_non_label + 1 == block.end &&
                   fn.icodes[first_non_label].op == icode_op::GOTO) {
            target = fn.icodes[first_non_label].label_name;
        }

        if (target.empty())
            continue;

        for (const auto &label : labels) {
            if (label != target)
                redirects.emplace(label, target);
        }
    }

    return redirects;
}

static std::string resolve_label_redirect(
        const std::unordered_map<std::string, std::string> &redirects,
        const std::string &label) {
    std::unordered_set<std::string> seen;
    std::string cur = label;

    while (!cur.empty()) {
        auto it = redirects.find(cur);
        if (it == redirects.end())
            break;
        if (!seen.insert(cur).second)
            break;
        cur = it->second;
    }

    return cur;
}

static std::vector<std::string> labels_in_block(const ir_function &fn,
                                                const basic_block &block) {
    std::vector<std::string> labels;
    for (size_t i = block.begin; i < block.end; ++i) {
        if (fn.icodes[i].op != icode_op::LABEL)
            break;
        labels.push_back(fn.icodes[i].label_name);
    }
    return labels;
}

static size_t first_non_label_index(const ir_function &fn,
                                    const basic_block &block) {
    size_t i = block.begin;
    while (i < block.end && fn.icodes[i].op == icode_op::LABEL)
        ++i;
    return i;
}

static bool block_is_label_return_only(const ir_function &fn,
                                       const basic_block &block) {
    size_t first = first_non_label_index(fn, block);
    return first < block.end &&
           first + 1 == block.end &&
           fn.icodes[first].op == icode_op::RETURN;
}

static size_t logical_block_end_for_tail_compare(const ir_function &fn,
                                                 const control_flow_graph &cfg,
                                                 const basic_block &block) {
    size_t logical_end = block.end;
    if (logical_end <= block.begin)
        return logical_end;

    const auto &last = fn.icodes[logical_end - 1];
    if (last.op != icode_op::GOTO)
        return logical_end;

    if (block.id + 1 < cfg.blocks().size()) {
        std::string next_label = first_label_in_block(fn, cfg.block(block.id + 1));
        if (!next_label.empty() && last.label_name == next_label)
            return logical_end - 1;
    }

    auto target_block = cfg.block_for_label(last.label_name);
    if (target_block && block_is_label_return_only(fn, cfg.block(*target_block)))
        --logical_end;

    return logical_end;
}

static std::string operand_signature(
        const operand &op,
        std::unordered_map<int, int> *temp_ids = nullptr) {
    std::string sig = std::to_string(static_cast<int>(op.kind)) + "|";
    sig += op.type ? op.type->to_string() : "_";
    sig += "|";

    switch (op.kind) {
    case operand_kind::NONE:
        break;
    case operand_kind::TEMP:
        if (temp_ids) {
            auto [it, inserted] = temp_ids->emplace(op.temp_id,
                                                    static_cast<int>(temp_ids->size()));
            sig += "t" + std::to_string(it->second);
        } else {
            sig += std::to_string(op.temp_id);
        }
        break;
    case operand_kind::SYMBOL:
        sig += op.name + "|" +
               (op.is_global ? "g" : "l") + "|" +
               (op.is_param ? "p" : "v") + "|" +
               (op.is_func ? "f" : "d") + "|" +
               (op.is_tls ? "t" : "n") + "|" +
               (op.is_sfr ? "s" : "n") + "|" +
               std::to_string(op.sfr_port) + "|" +
               std::to_string(op.stack_offset) + "|" +
               std::to_string(op.byte_offset);
        break;
    case operand_kind::INT_CONST:
        sig += std::to_string(op.ival);
        break;
    case operand_kind::FLOAT_CONST:
        sig += std::to_string(op.fval);
        break;
    case operand_kind::LABEL_REF:
        sig += op.name;
        break;
    }

    return sig;
}

static std::string icode_signature(
        const icode &ic,
        std::unordered_map<int, int> *temp_ids = nullptr) {
    return std::to_string(static_cast<int>(ic.op)) + "|" +
           operand_signature(ic.result, temp_ids) + "|" +
           operand_signature(ic.left, temp_ids) + "|" +
           operand_signature(ic.right, temp_ids) + "|" +
           ic.label_name + "|" + ic.true_lbl + "|" + ic.false_lbl + "|" +
           ic.func_name + "|" + ic.asm_text + "|" +
           std::to_string(ic.num_params) + "|" +
           std::to_string(ic.local_bytes) + "|" +
           std::to_string(ic.argreg) + "|" +
           std::to_string(static_cast<int>(ic.arg_loc)) + "|" +
           std::to_string(ic.arg_bytes) + "|" +
           std::to_string(static_cast<int>(ic.callee_abi));
}

static std::string block_body_signature(const ir_function &fn,
                                        const basic_block &block,
                                        size_t body_end) {
    size_t first = first_non_label_index(fn, block);
    if (first >= body_end)
        return {};

    std::string sig;
    std::unordered_map<int, int> temp_ids;
    for (size_t i = first; i < body_end; ++i) {
        sig += icode_signature(fn.icodes[i], &temp_ids);
        sig.push_back('\n');
    }
    return sig;
}

static bool operands_equivalent_for_tail_merge(
        const operand &lhs, const operand &rhs,
        std::unordered_map<int, int> &lhs_to_rhs,
        std::unordered_map<int, int> &rhs_to_lhs) {
    if (lhs.kind != rhs.kind)
        return false;

    if ((lhs.type && !rhs.type) || (!lhs.type && rhs.type))
        return false;
    if (lhs.type && rhs.type && lhs.type->to_string() != rhs.type->to_string())
        return false;

    switch (lhs.kind) {
    case operand_kind::NONE:
        return true;
    case operand_kind::TEMP: {
        auto l_it = lhs_to_rhs.find(lhs.temp_id);
        auto r_it = rhs_to_lhs.find(rhs.temp_id);
        if (l_it != lhs_to_rhs.end() || r_it != rhs_to_lhs.end()) {
            return l_it != lhs_to_rhs.end() &&
                   r_it != rhs_to_lhs.end() &&
                   l_it->second == rhs.temp_id &&
                   r_it->second == lhs.temp_id;
        }
        lhs_to_rhs.emplace(lhs.temp_id, rhs.temp_id);
        rhs_to_lhs.emplace(rhs.temp_id, lhs.temp_id);
        return true;
    }
    case operand_kind::SYMBOL:
        return lhs.name == rhs.name &&
               lhs.is_global == rhs.is_global &&
               lhs.is_param == rhs.is_param &&
               lhs.is_func == rhs.is_func &&
               lhs.is_tls == rhs.is_tls &&
               lhs.is_sfr == rhs.is_sfr &&
               lhs.sfr_port == rhs.sfr_port &&
               lhs.stack_offset == rhs.stack_offset &&
               lhs.byte_offset == rhs.byte_offset;
    case operand_kind::INT_CONST:
        return lhs.ival == rhs.ival;
    case operand_kind::FLOAT_CONST:
        return lhs.fval == rhs.fval;
    case operand_kind::LABEL_REF:
        return lhs.name == rhs.name;
    }
    return false;
}

static bool icodes_equivalent_for_tail_merge(
        const icode &lhs, const icode &rhs,
        std::unordered_map<int, int> &lhs_to_rhs,
        std::unordered_map<int, int> &rhs_to_lhs) {
    if (lhs.op != rhs.op ||
        lhs.label_name != rhs.label_name ||
        lhs.true_lbl != rhs.true_lbl ||
        lhs.false_lbl != rhs.false_lbl ||
        lhs.func_name != rhs.func_name ||
        lhs.asm_text != rhs.asm_text ||
        lhs.num_params != rhs.num_params ||
        lhs.local_bytes != rhs.local_bytes ||
        lhs.argreg != rhs.argreg ||
        lhs.arg_loc != rhs.arg_loc ||
        lhs.arg_bytes != rhs.arg_bytes ||
        lhs.callee_abi != rhs.callee_abi) {
        return false;
    }

    return operands_equivalent_for_tail_merge(lhs.result, rhs.result,
                                              lhs_to_rhs, rhs_to_lhs) &&
           operands_equivalent_for_tail_merge(lhs.left, rhs.left,
                                              lhs_to_rhs, rhs_to_lhs) &&
           operands_equivalent_for_tail_merge(lhs.right, rhs.right,
                                              lhs_to_rhs, rhs_to_lhs);
}

static bool is_tail_mergeable_icode(const icode &ic) {
    switch (ic.op) {
    case icode_op::FUNCTION:
    case icode_op::ENDFUNCTION:
    case icode_op::RECEIVE:
    case icode_op::INLINE_ASM:
        return false;
    default:
        return true;
    }
}

static std::string make_unique_label(const ir_function &fn,
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

static bool rewrite_operand(operand &op, const ssa_env &env,
                            const alias_info &alias,
                            const std::unordered_map<std::string, operand> &operand_bank,
                            const std::vector<std::string> &ordered_keys) {
    std::string key = trackable_key(op, alias);
    if (key.empty()) return false;
    auto it = env.find(key);
    if (it == env.end()) return false;

    if (it->second.tag == ssa_value::kind::INT_CONST) {
        type_ptr orig_type = op.type;
        op = operand::make_int(it->second.ival, orig_type ? orig_type : op.type);
        return true;
    }

    if (it->second.tag != ssa_value::kind::VALUE) return false;

    for (const auto &candidate_key : ordered_keys) {
        if (candidate_key == key) continue;
        auto env_it = env.find(candidate_key);
        if (env_it == env.end()) continue;
        if (env_it->second != it->second) continue;
        auto bank_it = operand_bank.find(candidate_key);
        if (bank_it == operand_bank.end()) continue;
        operand repl = bank_it->second;
        // Pointer temps participate in loop-carried address updates. Replacing
        // them with a base symbol/label through generic SSA equivalence is too
        // aggressive: once a later back-edge redefinition bumps the pointer,
        // earlier "same value" reasoning is no longer a safe source-level
        // substitute. Keep pointer identities explicit unless we are merely
        // rewriting to another temp.
        if (op.type && op.type->is_ptr() && !repl.is_temp())
            continue;
        if (op.type) repl.type = op.type;
        op = repl;
        return true;
    }
    return false;
}

class cfg_cleanup_pass final : public ir_pass {
public:
    const char *name() const override { return "cfg_cleanup"; }

    bool run(ir_function &fn) override {
        bool changed = false;

        for (auto it = fn.icodes.begin(); it != fn.icodes.end(); ) {
            if (it->op == icode_op::IFX &&
                it->left.kind == operand_kind::INT_CONST) {
                std::string target = it->left.ival ? it->true_lbl : it->false_lbl;
                if (target.empty()) {
                    it = fn.icodes.erase(it);
                    changed = true;
                    continue;
                }
                it->op         = icode_op::GOTO;
                it->label_name = target;
                it->true_lbl.clear();
                it->false_lbl.clear();
                it->left  = operand::make_none();
                it->right = operand::make_none();
                changed = true;
            } else if (it->op == icode_op::IFX &&
                       !it->true_lbl.empty() &&
                       it->true_lbl == it->false_lbl) {
                it->op         = icode_op::GOTO;
                it->label_name = it->true_lbl;
                it->true_lbl.clear();
                it->false_lbl.clear();
                it->left  = operand::make_none();
                it->right = operand::make_none();
                changed = true;
            }
            ++it;
        }

        for (auto it = fn.icodes.begin(); it != fn.icodes.end(); ) {
            auto next = it;
            ++next;
            if (it->op == icode_op::GOTO &&
                next != fn.icodes.end() &&
                next->op == icode_op::LABEL &&
                next->label_name == it->label_name) {
                it = fn.icodes.erase(it);
                changed = true;
            } else {
                ++it;
            }
        }

        control_flow_graph cfg(fn);
        auto reachable = cfg.reachable_blocks();
        if (reachable.size() != cfg.blocks().size()) {
            fn.icodes = rebuild_reachable_blocks(fn, cfg, reachable);
            changed = true;
        }
        return changed;
    }
};

class address_deref_fold_pass final : public ir_pass {
public:
    const char *name() const override { return "address_deref_fold"; }

    bool run(ir_function &fn) override {
        struct addr_expr {
            operand base;
            int byte_offset = 0;
        };

        auto is_foldable_base = [](const operand &op) {
            return op.is_symbol() &&
                   !op.is_func &&
                   !op.is_tls &&
                   !op.is_sfr &&
                   op.type;
        };

        std::unordered_map<int, addr_expr> temp_addr;
        bool changed = false;
        for (auto &ic : fn.icodes) {
            switch (ic.op) {
            case icode_op::FUNCTION:
            case icode_op::ENDFUNCTION:
            case icode_op::LABEL:
            case icode_op::GOTO:
            case icode_op::IFX:
            case icode_op::CALL:
            case icode_op::RETURN:
            case icode_op::INLINE_ASM:
                temp_addr.clear();
                break;
            default:
                break;
            }

            if (ic.op == icode_op::GET_VALUE_AT &&
                ic.left.is_temp()) {
                auto it = temp_addr.find(ic.left.temp_id);
                if (it == temp_addr.end())
                    continue;
                if (!ic.result.type || !it->second.base.type)
                    continue;
                int access_size = ic.result.type->size();
                if (access_size <= 0)
                    continue;
                if (it->second.byte_offset < 0)
                    continue;
                if (it->second.byte_offset + access_size > it->second.base.type->size())
                    continue;

                operand direct = it->second.base;
                direct.byte_offset += it->second.byte_offset;
                direct.type = ic.result.type;
                ic.op = icode_op::ASSIGN;
                ic.left = std::move(direct);
                ic.right = operand::make_none();
                changed = true;
                continue;
            }

            if (ic.op == icode_op::SET_VALUE_AT &&
                ic.result.is_temp()) {
                auto it = temp_addr.find(ic.result.temp_id);
                if (it == temp_addr.end())
                    continue;
                if (!ic.left.type || !it->second.base.type)
                    continue;
                int access_size = ic.left.type->size();
                if (access_size <= 0)
                    continue;
                if (it->second.byte_offset < 0)
                    continue;
                if (it->second.byte_offset + access_size > it->second.base.type->size())
                    continue;

                operand direct = it->second.base;
                direct.byte_offset += it->second.byte_offset;
                direct.type = ic.left.type;
                ic.op = icode_op::ASSIGN;
                ic.result = std::move(direct);
                ic.right = operand::make_none();
                changed = true;
                continue;
            }

            if (ic.result.is_temp()) {
                temp_addr.erase(ic.result.temp_id);

                if (ic.op == icode_op::ADDRESS_OF &&
                    is_foldable_base(ic.left)) {
                    operand base = ic.left;
                    base.byte_offset = 0;
                    temp_addr[ic.result.temp_id] = addr_expr{base, ic.left.byte_offset};
                    continue;
                }

                if (ic.op == icode_op::ASSIGN &&
                    ic.left.is_temp()) {
                    auto it = temp_addr.find(ic.left.temp_id);
                    if (it != temp_addr.end()) {
                        temp_addr[ic.result.temp_id] = it->second;
                        continue;
                    }
                }

                auto propagate_add = [&](const operand &lhs, const operand &rhs,
                                         int sign) {
                    if (!lhs.is_temp() || rhs.kind != operand_kind::INT_CONST)
                        return false;
                    auto it = temp_addr.find(lhs.temp_id);
                    if (it == temp_addr.end())
                        return false;
                    int64_t delta64 = static_cast<int64_t>(it->second.byte_offset) +
                                      sign * rhs.ival;
                    if (delta64 < std::numeric_limits<int>::min() ||
                        delta64 > std::numeric_limits<int>::max()) {
                        return false;
                    }
                    temp_addr[ic.result.temp_id] =
                        addr_expr{it->second.base, static_cast<int>(delta64)};
                    return true;
                };

                if (ic.op == icode_op::ADD) {
                    if (propagate_add(ic.left, ic.right, +1) ||
                        propagate_add(ic.right, ic.left, +1)) {
                        continue;
                    }
                }

                if (ic.op == icode_op::SUB) {
                    if (propagate_add(ic.left, ic.right, -1)) {
                        continue;
                    }
                }
            }
        }

        return changed;
    }
};

class jump_threading_pass final : public ir_pass {
public:
    const char *name() const override { return "jump_threading"; }

    bool run(ir_function &fn) override {
        bool changed = false;

        control_flow_graph cfg(fn);
        auto redirects = build_label_redirects(fn, cfg);
        if (redirects.empty())
            return false;

        for (auto &ic : fn.icodes) {
            if (ic.op == icode_op::GOTO) {
                std::string target = resolve_label_redirect(redirects, ic.label_name);
                if (!target.empty() && target != ic.label_name) {
                    ic.label_name = std::move(target);
                    changed = true;
                }
            } else if (ic.op == icode_op::IFX) {
                std::string true_lbl = resolve_label_redirect(redirects, ic.true_lbl);
                std::string false_lbl = resolve_label_redirect(redirects, ic.false_lbl);
                if (!true_lbl.empty() && true_lbl != ic.true_lbl) {
                    ic.true_lbl = std::move(true_lbl);
                    changed = true;
                }
                if (!false_lbl.empty() && false_lbl != ic.false_lbl) {
                    ic.false_lbl = std::move(false_lbl);
                    changed = true;
                }
            }
        }

        for (auto &ic : fn.icodes) {
            if (ic.op == icode_op::IFX &&
                !ic.true_lbl.empty() &&
                ic.true_lbl == ic.false_lbl) {
                ic.op         = icode_op::GOTO;
                ic.label_name = ic.true_lbl;
                ic.true_lbl.clear();
                ic.false_lbl.clear();
                ic.left  = operand::make_none();
                ic.right = operand::make_none();
                changed = true;
            }
        }

        return changed;
    }
};

class global_address_const_pass final : public ir_pass {
public:
    const char *name() const override { return "global_address_const"; }

    bool run(ir_function &fn) override {
        std::unordered_map<int, operand> global_addr_defs;

        for (const auto &ic : fn.icodes) {
            if (ic.op != icode_op::ADDRESS_OF || !ic.result.is_temp())
                continue;
            if (!ic.left.is_global || ic.left.is_tls || ic.left.is_sfr)
                continue;

            operand ref = operand::make_label(ic.left.name);
            ref.type = ic.result.type;
            global_addr_defs[ic.result.temp_id] = ref;
        }

        if (global_addr_defs.empty())
            return false;

        bool changed = false;
        for (auto &ic : fn.icodes) {
            for_each_use_operand(ic, [&](operand &op) {
                if (!op.is_temp() || op.byte_offset != 0)
                    return;
                auto it = global_addr_defs.find(op.temp_id);
                if (it == global_addr_defs.end())
                    return;
                operand repl = it->second;
                if (!repl.type)
                    repl.type = op.type;
                op = repl;
                changed = true;
            });
        }

        return changed;
    }
};

class scalar_local_promotion_pass final : public ir_pass {
public:
    const char *name() const override { return "scalar_local_promotion"; }

    bool run(ir_function &fn) override {
        for (const auto &ic : fn.icodes) {
            if (ic.op == icode_op::INLINE_ASM ||
                ic.op == icode_op::ALLOCA) {
                return false;
            }
        }

        alias_info alias = build_alias_info(fn);
        std::unordered_set<std::string> blocked;

        auto promotable_local_type = [](const type_ptr &type) {
            if (!type)
                return false;
            if (type->is_array() || type->is_func() ||
                type->is_ptr() ||
                type->kind == type_kind::STRUCT ||
                type->kind == type_kind::UNION)
                return false;
            int sz = type->size();
            return sz == 1 || sz == 2;
        };

        auto block_if_needed = [&](const operand &op) {
            if (!op.is_symbol() || op.is_global || op.is_param ||
                op.is_tls || op.is_sfr || op.is_func) {
                return;
            }
            std::string key = base_symbol_key(op);
            if (key.empty())
                return;
            if (!promotable_local_type(op.type) || op.byte_offset != 0)
                blocked.insert(std::move(key));
        };

        for (const auto &ic : fn.icodes) {
            if (ic.op == icode_op::RECEIVE && ic.result.is_symbol()) {
                blocked.insert(base_symbol_key(ic.result));
            }
            block_if_needed(ic.result);
            block_if_needed(ic.left);
            block_if_needed(ic.right);
        }

        struct candidate {
            operand temp;
        };

        int next_temp = next_temp_id(fn);
        std::unordered_map<std::string, candidate> promoted;

        auto consider = [&](const operand &op) {
            if (!op.is_symbol() || op.is_global || op.is_param ||
                op.is_tls || op.is_sfr || op.is_func) {
                return;
            }
            if (!promotable_local_type(op.type) || op.byte_offset != 0)
                return;
            if (base_symbol_address_taken(alias, op))
                return;

            std::string key = base_symbol_key(op);
            if (key.empty() || blocked.count(key))
                return;

            promoted.try_emplace(key, candidate{operand::make_temp(next_temp++, op.type)});
        };

        for (const auto &ic : fn.icodes) {
            consider(ic.result);
            consider(ic.left);
            consider(ic.right);
        }

        if (promoted.empty())
            return false;

        auto remap = [&](operand &op) {
            if (!op.is_symbol())
                return false;
            auto it = promoted.find(base_symbol_key(op));
            if (it == promoted.end())
                return false;
            operand repl = it->second.temp;
            repl.type = op.type ? op.type : repl.type;
            repl.byte_offset = op.byte_offset;
            op = std::move(repl);
            return true;
        };

        bool changed = false;
        for (auto &ic : fn.icodes) {
            changed = remap(ic.result) || changed;
            changed = remap(ic.left) || changed;
            changed = remap(ic.right) || changed;
        }

        return changed;
    }
};

class reg_param_promotion_pass final : public ir_pass {
public:
    const char *name() const override { return "reg_param_promotion"; }

    bool run(ir_function &fn) override {
        if (effective_call_abi(fn.abi) != call_abi::SDCCCALL1)
            return false;

        for (const auto &ic : fn.icodes) {
            if (ic.op == icode_op::INLINE_ASM)
                return false;
        }

        alias_info alias = build_alias_info(fn);

        struct candidate {
            operand temp;
        };

        int next_temp = next_temp_id(fn);
        std::unordered_map<std::string, candidate> promoted;

        for (const auto &ic : fn.icodes) {
            if (ic.op != icode_op::RECEIVE)
                continue;
            if (ic.arg_loc == abi_arg_loc::STACK)
                continue;
            if (!ic.result.is_symbol() || ic.result.is_global || ic.result.is_param)
                continue;
            if (ic.result.byte_offset != 0 || !ic.result.type)
                continue;
            if (ic.result.type->is_ptr())
                continue;
            if (base_symbol_address_taken(alias, ic.result))
                continue;
            promoted.try_emplace(base_symbol_key(ic.result),
                                 candidate{operand::make_temp(next_temp++, ic.result.type)});
        }

        if (promoted.empty())
            return false;

        auto remap = [&](operand &op) {
            if (!op.is_symbol())
                return false;
            auto it = promoted.find(base_symbol_key(op));
            if (it == promoted.end())
                return false;
            operand repl = it->second.temp;
            repl.type = op.type ? op.type : repl.type;
            repl.byte_offset = op.byte_offset;
            op = std::move(repl);
            return true;
        };

        bool changed = false;
        for (auto &ic : fn.icodes) {
            if (ic.op == icode_op::RECEIVE) {
                changed = remap(ic.result) || changed;
                continue;
            }
            changed = remap(ic.result) || changed;
            changed = remap(ic.left) || changed;
            changed = remap(ic.right) || changed;
        }

        return changed;
    }
};

class compare_bool_normalize_pass final : public ir_pass {
public:
    const char *name() const override { return "compare_bool_normalize"; }

    bool run(ir_function &fn) override {
        std::unordered_map<int, const icode *> temp_defs;
        std::unordered_map<int, int> temp_def_count;
        std::unordered_map<int, std::vector<const icode *>> temp_uses;
        for (const auto &ic : fn.icodes) {
            if (ic.result.is_temp()) {
                temp_defs[ic.result.temp_id] = &ic;
                ++temp_def_count[ic.result.temp_id];
            }
            for_each_use_operand(ic, [&](const operand &op) {
                if (op.is_temp())
                    temp_uses[op.temp_id].push_back(&ic);
            });
        }

        std::unordered_map<int, operand> aliases;
        std::vector<bool> erase(fn.icodes.size(), false);
        bool changed = false;

        auto zero_const = [](const operand &op) {
            return op.kind == operand_kind::INT_CONST && op.ival == 0;
        };
        auto one_const = [](const operand &op) {
            return op.kind == operand_kind::INT_CONST && op.ival == 1;
        };

        std::function<std::optional<operand>(const operand &,
                                             std::unordered_set<int> &)>
            resolve_truth_preserving_source =
                [&](const operand &op,
                    std::unordered_set<int> &visiting) -> std::optional<operand> {
            if (!op.is_temp())
                return std::nullopt;
            if (temp_def_count[op.temp_id] != 1)
                return std::nullopt;
            if (!visiting.insert(op.temp_id).second)
                return std::nullopt;

            auto def_it = temp_defs.find(op.temp_id);
            if (def_it == temp_defs.end() || !def_it->second)
                return std::nullopt;
            const icode &def = *def_it->second;

            if (is_compare_op(def.op)) {
                operand repl = op;
                repl.type = def.result.type ? def.result.type : repl.type;
                return repl;
            }

            if (def.op == icode_op::ASSIGN && def.left.is_temp()) {
                auto nested = resolve_truth_preserving_source(def.left, visiting);
                if (nested) {
                    operand repl = *nested;
                    repl.type = def.result.type ? def.result.type : repl.type;
                    return repl;
                }
            }

            if (def.op == icode_op::NE || def.op == icode_op::EQ) {
                const operand *temp_side = nullptr;
                const operand *const_side = nullptr;
                if (def.left.is_temp() &&
                    (zero_const(def.right) || one_const(def.right))) {
                    temp_side = &def.left;
                    const_side = &def.right;
                } else if (def.right.is_temp() &&
                           (zero_const(def.left) || one_const(def.left))) {
                    temp_side = &def.right;
                    const_side = &def.left;
                }
                if (temp_side && const_side) {
                    bool preserves_truth =
                        (def.op == icode_op::NE && zero_const(*const_side)) ||
                        (def.op == icode_op::EQ && one_const(*const_side));
                    if (preserves_truth) {
                        auto nested =
                            resolve_truth_preserving_source(*temp_side, visiting);
                        if (nested) {
                            operand repl = *nested;
                            repl.type = def.result.type ? def.result.type : repl.type;
                            return repl;
                        }
                    }
                }
            }

            return std::nullopt;
        };

        auto identity_source = [&](const icode &ic) -> std::optional<operand> {
            const operand *source = nullptr;
            bool identity = false;
            if (ic.left.is_temp() &&
                ((ic.op == icode_op::NE && zero_const(ic.right)) ||
                 (ic.op == icode_op::EQ && one_const(ic.right)))) {
                source = &ic.left;
                identity = true;
            }
            if (!identity && ic.right.is_temp() &&
                ((ic.op == icode_op::NE && zero_const(ic.left)) ||
                 (ic.op == icode_op::EQ && one_const(ic.left)))) {
                source = &ic.right;
                identity = true;
            }
            if (!identity || !source)
                return std::nullopt;
            std::unordered_set<int> visiting;
            auto repl = resolve_truth_preserving_source(*source, visiting);
            if (!repl)
                return std::nullopt;
            repl->type = ic.result.type ? ic.result.type : repl->type;
            return repl;
        };

        auto used_by_one_sided_ifx = [&](int temp_id) {
            auto it = temp_uses.find(temp_id);
            if (it == temp_uses.end())
                return false;
            for (const icode *user : it->second) {
                if (!user || user->op != icode_op::IFX || !user->left.is_temp() ||
                    user->left.temp_id != temp_id) {
                    continue;
                }
                if (user->true_lbl.empty() || user->false_lbl.empty())
                    return true;
            }
            return false;
        };

        for (size_t i = 0; i < fn.icodes.size(); ++i) {
            const auto &ic = fn.icodes[i];
            if (!ic.result.is_temp())
                continue;
            if (temp_def_count[ic.result.temp_id] != 1)
                continue;
            std::optional<operand> repl;
            if (ic.op == icode_op::NE || ic.op == icode_op::EQ) {
                repl = identity_source(ic);
            } else if (ic.op == icode_op::ASSIGN && ic.left.is_temp()) {
                std::unordered_set<int> visiting;
                repl = resolve_truth_preserving_source(ic.left, visiting);
                if (repl)
                    repl->type = ic.result.type ? ic.result.type : repl->type;
            }
            if (!repl)
                continue;
            if (used_by_one_sided_ifx(ic.result.temp_id))
                continue;
            aliases[ic.result.temp_id] = *repl;
            erase[i] = true;
            changed = true;
        }

        if (!changed)
            return false;

        auto resolve_alias = [&](operand &op) {
            if (!op.is_temp())
                return false;
            bool local_changed = false;
            std::unordered_set<int> seen;
            while (op.is_temp()) {
                auto it = aliases.find(op.temp_id);
                if (it == aliases.end())
                    break;
                if (!seen.insert(op.temp_id).second)
                    break;
                operand repl = it->second;
                repl.type = op.type ? op.type : repl.type;
                repl.byte_offset = op.byte_offset;
                op = std::move(repl);
                local_changed = true;
            }
            return local_changed;
        };

        for (size_t i = 0; i < fn.icodes.size(); ++i) {
            if (erase[i])
                continue;
            auto &ic = fn.icodes[i];
            changed = resolve_alias(ic.left) || changed;
            changed = resolve_alias(ic.right) || changed;
            changed = resolve_alias(ic.result) || changed;
        }

        std::vector<icode> out;
        out.reserve(fn.icodes.size());
        for (size_t i = 0; i < fn.icodes.size(); ++i) {
            if (!erase[i])
                out.push_back(fn.icodes[i]);
        }
        fn.icodes = std::move(out);
        return true;
    }
};

class narrow_counted_byte_loops_pass final : public ir_pass {
public:
    const char *name() const override { return "narrow_counted_byte_loops"; }

    bool run(ir_function &fn) override {
        if (fn.icodes.empty())
            return false;

        control_flow_graph cfg(fn);
        auto loops = cfg.natural_loops();
        if (loops.empty())
            return false;

        bool changed = false;
        type_ptr byte_type = type::make_uchar();

        auto retag_temp = [&](int temp_id) {
            for (auto &ic : fn.icodes) {
                auto fix = [&](operand &op) {
                    if (op.is_temp() && op.temp_id == temp_id)
                        op.type = byte_type;
                };
                fix(ic.result);
                fix(ic.left);
                fix(ic.right);
            }
        };

        for (const auto &loop : loops) {
            if (loop.outside_preds.size() != 1 || loop.latches.size() != 1)
                continue;

            const auto &header = cfg.block(loop.header);
            const auto &preheader = cfg.block(loop.outside_preds.front());
            const auto &latch = cfg.block(loop.latches.front());

            int iv_temp = -1;
            int next_temp = -1;
            size_t compare_idx = fn.icodes.size();

            for (size_t i = header.begin; i < header.end; ++i) {
                const auto &ifx = fn.icodes[i];
                if (ifx.op != icode_op::IFX || !ifx.left.is_temp())
                    continue;
                for (size_t j = i; j-- > header.begin;) {
                    const auto &def = fn.icodes[j];
                    if (!def.result.is_temp() || def.result.temp_id != ifx.left.temp_id)
                        continue;
                    bool match =
                        def.op == icode_op::LT &&
                        def.left.is_temp() &&
                        def.right.kind == operand_kind::INT_CONST &&
                        def.right.ival > 0 &&
                        def.right.ival <= 255;
                    if (!match)
                        break;
                    iv_temp = def.left.temp_id;
                    compare_idx = j;
                    break;
                }
                if (iv_temp >= 0)
                    break;
            }

            if (iv_temp < 0 || compare_idx == fn.icodes.size())
                continue;

            bool found_init = false;
            for (size_t i = preheader.begin; i < preheader.end; ++i) {
                auto &ic = fn.icodes[i];
                if (ic.op == icode_op::ASSIGN &&
                    ic.result.is_temp() &&
                    ic.result.temp_id == iv_temp &&
                    ic.left.kind == operand_kind::INT_CONST &&
                    ic.left.ival == 0) {
                    ic.result.type = byte_type;
                    ic.left.type = byte_type;
                    found_init = true;
                }
            }
            if (!found_init)
                continue;

            int loop_assign_defs = 0;
            bool bad_defs = false;
            for (size_t block_id : loop.blocks) {
                const auto &block = cfg.block(block_id);
                for (size_t i = block.begin; i < block.end; ++i) {
                    const auto &ic = fn.icodes[i];
                    if (ic.result.is_temp() && ic.result.temp_id == iv_temp) {
                        ++loop_assign_defs;
                        if (ic.op != icode_op::ASSIGN)
                            bad_defs = true;
                    }
                }
            }
            if (bad_defs || loop_assign_defs != 1)
                continue;

            bool found_update = false;
            for (size_t i = latch.begin; i < latch.end; ++i) {
                const auto &inc = fn.icodes[i];
                if (inc.op != icode_op::ADD ||
                    !inc.result.is_temp() ||
                    !inc.left.is_temp() ||
                    inc.left.temp_id != iv_temp ||
                    inc.right.kind != operand_kind::INT_CONST ||
                    inc.right.ival != 1) {
                    continue;
                }
                for (size_t j = i + 1; j < latch.end; ++j) {
                    const auto &asn = fn.icodes[j];
                    if (asn.op == icode_op::ASSIGN &&
                        asn.result.is_temp() &&
                        asn.result.temp_id == iv_temp &&
                        asn.left.is_temp() &&
                        asn.left.temp_id == inc.result.temp_id) {
                        next_temp = inc.result.temp_id;
                        found_update = true;
                        break;
                    }
                }
                if (found_update)
                    break;
            }
            if (!found_update || next_temp < 0)
                continue;

            retag_temp(iv_temp);
            retag_temp(next_temp);

            auto &cmp = fn.icodes[compare_idx];
            if (cmp.left.is_temp() && cmp.left.temp_id == iv_temp)
                cmp.left.type = byte_type;
            if (cmp.right.kind == operand_kind::INT_CONST)
                cmp.right.type = byte_type;

            changed = true;
        }

        return changed;
    }
};

class loop_pointer_walk_pass final : public ir_pass {
public:
    const char *name() const override { return "loop_pointer_walk"; }

    bool run(ir_function &fn) override {
        if (fn.icodes.empty())
            return false;

        control_flow_graph cfg(fn);
        auto loops = cfg.natural_loops();
        if (loops.empty())
            return false;

        std::unordered_map<int, const icode *> temp_defs;
        for (const auto &ic : fn.icodes) {
            if (ic.result.is_temp())
                temp_defs[ic.result.temp_id] = &ic;
        }

        auto unwrap_loop_index = [&](const operand &op, int iv_temp) -> bool {
            std::function<bool(const operand &)> match = [&](const operand &cur) -> bool {
                if (cur.is_temp() && cur.temp_id == iv_temp)
                    return true;
                if (!cur.is_temp())
                    return false;
                auto it = temp_defs.find(cur.temp_id);
                if (it == temp_defs.end() || !it->second)
                    return false;
                const icode *def = it->second;
                if ((def->op == icode_op::ASSIGN || def->op == icode_op::CAST) &&
                    def->left.is_temp()) {
                    return match(def->left);
                }
                return false;
            };
            return match(op);
        };

        auto is_byte_mem_use = [](const icode &ic, int temp_id) {
            const int result_size =
                (ic.result.type && ic.result.type->size() > 0) ? ic.result.type->size() : 2;
            const int left_size =
                (ic.left.type && ic.left.type->size() > 0) ? ic.left.type->size() : 2;
            if (ic.op == icode_op::GET_VALUE_AT &&
                ic.left.is_temp() &&
                ic.left.temp_id == temp_id &&
                result_size == 1) {
                return true;
            }
            if (ic.op == icode_op::SET_VALUE_AT &&
                ic.result.is_temp() &&
                ic.result.temp_id == temp_id &&
                left_size == 1) {
                return true;
            }
            return false;
        };

        auto pointer_type_for_base = [](const operand &base) -> type_ptr {
            if (base.type) {
                if (base.type->is_array() && base.type->base)
                    return type::make_pointer(base.type->base);
                if (base.type->is_ptr())
                    return base.type;
            }
            return type::make_pointer(type::make_char());
        };

        int next_temp = next_temp_id(fn);
        std::map<size_t, std::vector<icode>> insert_before;
        std::unordered_map<int, operand> replace_temp;

        for (const auto &loop : loops) {
            if (loop.outside_preds.size() != 1 || loop.latches.size() != 1)
                continue;

            const auto &header = cfg.block(loop.header);
            const auto &preheader = cfg.block(loop.outside_preds.front());
            const auto &latch = cfg.block(loop.latches.front());

            int iv_temp = -1;
            size_t compare_idx = fn.icodes.size();

            for (size_t i = header.begin; i < header.end; ++i) {
                const auto &ifx = fn.icodes[i];
                if (ifx.op != icode_op::IFX || !ifx.left.is_temp())
                    continue;
                for (size_t j = i; j-- > header.begin;) {
                    const auto &def = fn.icodes[j];
                    if (!def.result.is_temp() || def.result.temp_id != ifx.left.temp_id)
                        continue;
                    bool match =
                        def.op == icode_op::LT &&
                        def.left.is_temp() &&
                        def.right.kind == operand_kind::INT_CONST &&
                        def.right.ival > 0 &&
                        def.right.ival <= 255;
                    if (!match)
                        break;
                    iv_temp = def.left.temp_id;
                    compare_idx = j;
                    break;
                }
                if (iv_temp >= 0)
                    break;
            }

            if (iv_temp < 0 || compare_idx == fn.icodes.size())
                continue;

            bool found_init = false;
            for (size_t i = preheader.begin; i < preheader.end; ++i) {
                const auto &ic = fn.icodes[i];
                if (ic.op == icode_op::ASSIGN &&
                    ic.result.is_temp() &&
                    ic.result.temp_id == iv_temp &&
                    ic.left.kind == operand_kind::INT_CONST &&
                    ic.left.ival == 0) {
                    found_init = true;
                    break;
                }
            }
            if (!found_init)
                continue;

            int loop_assign_defs = 0;
            bool bad_defs = false;
            for (size_t block_id : loop.blocks) {
                const auto &block = cfg.block(block_id);
                for (size_t i = block.begin; i < block.end; ++i) {
                    const auto &ic = fn.icodes[i];
                    if (ic.result.is_temp() && ic.result.temp_id == iv_temp) {
                        ++loop_assign_defs;
                        if (ic.op != icode_op::ASSIGN)
                            bad_defs = true;
                    }
                }
            }
            if (bad_defs || loop_assign_defs != 1)
                continue;

            bool found_update = false;
            for (size_t i = latch.begin; i < latch.end; ++i) {
                const auto &inc = fn.icodes[i];
                if (inc.op != icode_op::ADD ||
                    !inc.result.is_temp() ||
                    !inc.left.is_temp() ||
                    inc.left.temp_id != iv_temp ||
                    inc.right.kind != operand_kind::INT_CONST ||
                    inc.right.ival != 1) {
                    continue;
                }
                for (size_t j = i + 1; j < latch.end; ++j) {
                    const auto &asn = fn.icodes[j];
                    if (asn.op == icode_op::ASSIGN &&
                        asn.result.is_temp() &&
                        asn.result.temp_id == iv_temp &&
                        asn.left.is_temp() &&
                        asn.left.temp_id == inc.result.temp_id) {
                        found_update = true;
                        break;
                    }
                }
                if (found_update)
                    break;
            }
            if (!found_update)
                continue;

            std::vector<bool> in_loop_inst(fn.icodes.size(), false);
            for (size_t block_id : loop.blocks) {
                const auto &block = cfg.block(block_id);
                for (size_t i = block.begin; i < block.end; ++i)
                    in_loop_inst[i] = true;
            }

            struct candidate_group {
                operand base;
                type_ptr ptr_type;
                std::vector<int> add_temps;
            };
            std::unordered_map<std::string, candidate_group> groups;

            auto loop_only_byte_mem_temp = [&](int temp_id) {
                bool saw_use = false;
                for (size_t i = 0; i < fn.icodes.size(); ++i) {
                    bool used_here = false;
                    for_each_use_operand(fn.icodes[i], [&](const operand &op) {
                        if (op.is_temp() && op.temp_id == temp_id)
                            used_here = true;
                    });
                    if (!used_here)
                        continue;
                    saw_use = true;
                    if (!in_loop_inst[i] || !is_byte_mem_use(fn.icodes[i], temp_id))
                        return false;
                }
                return saw_use;
            };

            for (size_t block_id : loop.blocks) {
                const auto &block = cfg.block(block_id);
                for (size_t i = block.begin; i < block.end; ++i) {
                    const auto &ic = fn.icodes[i];
                    if (ic.op != icode_op::ADD || !ic.result.is_temp())
                        continue;

                    operand base;
                    auto is_byte_base = [](const operand &op) {
                        if (op.kind == operand_kind::LABEL_REF)
                            return true;
                        return op.is_symbol() &&
                               op.is_global &&
                               !op.is_tls &&
                               !op.is_sfr &&
                               !op.is_func &&
                               op.type &&
                               op.type->is_array() &&
                               op.type->base &&
                               op.type->base->size() == 1;
                    };

                    if (is_byte_base(ic.left) &&
                        unwrap_loop_index(ic.right, iv_temp)) {
                        base = ic.left;
                    } else if (is_byte_base(ic.right) &&
                               unwrap_loop_index(ic.left, iv_temp)) {
                        base = ic.right;
                    } else {
                        continue;
                    }

                    if (!loop_only_byte_mem_temp(ic.result.temp_id))
                        continue;

                    std::string key = base.kind == operand_kind::LABEL_REF
                                          ? "label|" + base.name
                                          : symbol_key(base);
                    auto &group = groups[key];
                    if (group.base.is_none()) {
                        group.base = base;
                        group.ptr_type = pointer_type_for_base(base);
                    }
                    group.add_temps.push_back(ic.result.temp_id);
                }
            }

            if (groups.empty())
                continue;

            size_t pre_insert = insertion_index_before_terminator(preheader, fn);
            size_t latch_insert = insertion_index_before_terminator(latch, fn);
            for (auto &[_, group] : groups) {
                operand ptr = make_fresh_temp(next_temp, group.ptr_type);
                icode init;
                init.op = group.base.kind == operand_kind::LABEL_REF
                              ? icode_op::ASSIGN
                              : icode_op::ADDRESS_OF;
                init.result = ptr;
                init.left = group.base;
                insert_before[pre_insert].push_back(init);

                operand advanced = make_fresh_temp(next_temp, group.ptr_type);
                icode bump;
                bump.op = icode_op::ADD;
                bump.result = advanced;
                bump.left = ptr;
                bump.right = operand::make_int(1, type::make_int());
                insert_before[latch_insert].push_back(bump);

                icode commit;
                commit.op = icode_op::ASSIGN;
                commit.result = ptr;
                commit.left = advanced;
                insert_before[latch_insert].push_back(commit);

                for (int temp_id : group.add_temps)
                    replace_temp[temp_id] = ptr;
            }
        }

        if (replace_temp.empty())
            return false;

        for (auto &ic : fn.icodes) {
            for_each_use_operand(ic, [&](operand &op) {
                if (!op.is_temp())
                    return;
                auto it = replace_temp.find(op.temp_id);
                if (it != replace_temp.end())
                    op = it->second;
            });
        }

        fn.icodes = rebuild_with_insertions(fn.icodes, {}, insert_before);
        return true;
    }
};

class ifx_bool_wrapper_elide_pass final : public ir_pass {
public:
    const char *name() const override { return "ifx_bool_wrapper_elide"; }

    bool run(ir_function &fn) override {
        std::unordered_map<int, int> use_counts;
        for (const auto &ic : fn.icodes) {
            for_each_use_operand(ic, [&](const operand &op) {
                if (op.is_temp())
                    ++use_counts[op.temp_id];
            });
        }

        std::vector<bool> erase(fn.icodes.size(), false);
        bool changed = false;

        auto parse_wrapper = [&](const icode &ic)
            -> std::optional<std::pair<operand, bool>> {
            if (!ic.result.is_temp())
                return std::nullopt;

            if (ic.op == icode_op::ASSIGN && ic.left.is_temp())
                return std::make_pair(ic.left, true);

            if (ic.op != icode_op::EQ && ic.op != icode_op::NE)
                return std::nullopt;

            const bool left_temp = ic.left.is_temp() &&
                                   ic.right.kind == operand_kind::INT_CONST;
            const bool right_temp = ic.right.is_temp() &&
                                    ic.left.kind == operand_kind::INT_CONST;
            if (!left_temp && !right_temp)
                return std::nullopt;

            operand source = left_temp ? ic.left : ic.right;
            const int64_t other = left_temp ? ic.right.ival : ic.left.ival;
            if ((ic.op == icode_op::NE && other == 0) ||
                (ic.op == icode_op::EQ && other == 1))
                return std::make_pair(source, true);
            if ((ic.op == icode_op::EQ && other == 0) ||
                (ic.op == icode_op::NE && other == 1))
                return std::make_pair(source, false);
            return std::nullopt;
        };

        for (size_t i = 0; i + 1 < fn.icodes.size(); ++i) {
            auto wrapper = parse_wrapper(fn.icodes[i]);
            if (!wrapper)
                continue;

            auto &ifx = fn.icodes[i + 1];
            if (ifx.op != icode_op::IFX || !ifx.left.is_temp() ||
                ifx.left.temp_id != fn.icodes[i].result.temp_id)
                continue;

            // One-sided IFX is used by switch lowering to mean
            // "branch on true, otherwise fall through".  Replacing
            // `eq/ne temp, const` with the raw temp changes equality
            // tests like `x == 1` into generic truthiness checks, and
            // inverting `x == 0` also cannot be represented safely in
            // that form.  Keep those wrappers intact unless both
            // destinations are explicit.
            if (ifx.true_lbl.empty() || ifx.false_lbl.empty())
                continue;

            operand repl = wrapper->first;
            repl.type = ifx.left.type ? ifx.left.type : repl.type;
            ifx.left = repl;
            if (!wrapper->second)
                std::swap(ifx.true_lbl, ifx.false_lbl);

            if (use_counts[fn.icodes[i].result.temp_id] == 1)
                erase[i] = true;
            changed = true;
        }

        if (!changed)
            return false;

        std::vector<icode> out;
        out.reserve(fn.icodes.size());
        for (size_t i = 0; i < fn.icodes.size(); ++i) {
            if (!erase[i])
                out.push_back(fn.icodes[i]);
        }
        fn.icodes = std::move(out);
        return true;
    }
};

class short_circuit_bool_ifx_pass final : public ir_pass {
public:
    const char *name() const override { return "short_circuit_bool_ifx"; }

    bool run(ir_function &fn) override {
        control_flow_graph cfg(fn);
        std::vector<bool> erase(fn.icodes.size(), false);
        bool changed = false;

        auto temp_used_after = [&](size_t start_idx, int temp_id) {
            for (size_t i = start_idx; i < fn.icodes.size(); ++i) {
                bool used = false;
                for_each_use_operand(fn.icodes[i], [&](const operand &op) {
                    if (op.is_temp() && op.temp_id == temp_id)
                        used = true;
                });
                if (used)
                    return true;
                if (fn.icodes[i].result.is_temp() &&
                    fn.icodes[i].result.temp_id == temp_id)
                    return false;
            }
            return false;
        };

        struct const_pred_info {
            size_t assign_idx = static_cast<size_t>(-1);
            int value = 0;
        };

        struct join_ifx_info {
            size_t bool_idx = static_cast<size_t>(-1);
            size_t ifx_idx = static_cast<size_t>(-1);
            int phi_temp = -1;
            bool same_truth = true;
        };

        auto parse_const_pred = [&](const basic_block &block,
                                    int phi_temp,
                                    const std::string &join_label)
            -> std::optional<const_pred_info> {
            size_t first = first_non_label_index(fn, block);
            size_t logical_end = block.end;
            bool has_join_goto = false;
            if (logical_end > first &&
                fn.icodes[logical_end - 1].op == icode_op::GOTO &&
                fn.icodes[logical_end - 1].label_name == join_label) {
                has_join_goto = true;
                --logical_end;
            }
            if (logical_end != first + 1)
                return std::nullopt;
            const auto &ic = fn.icodes[first];
            if (ic.op != icode_op::ASSIGN || !ic.result.is_temp() ||
                ic.result.temp_id != phi_temp ||
                ic.left.kind != operand_kind::INT_CONST ||
                (ic.left.ival != 0 && ic.left.ival != 1))
                return std::nullopt;
            (void)has_join_goto;
            return const_pred_info{first, static_cast<int>(ic.left.ival)};
        };

        struct eval_pred_info {
            size_t cmp_idx = static_cast<size_t>(-1);
            size_t bool_idx = static_cast<size_t>(-1);
            size_t goto_idx = static_cast<size_t>(-1);
            int cmp_temp = -1;
            bool same_truth = false;
        };

        auto parse_eval_pred = [&](const basic_block &block,
                                   int phi_temp,
                                   const std::string &join_label)
            -> std::optional<eval_pred_info> {
            size_t first = first_non_label_index(fn, block);
            size_t end = block.end;
            size_t goto_idx = static_cast<size_t>(-1);
            if (end > first &&
                fn.icodes[end - 1].op == icode_op::GOTO &&
                fn.icodes[end - 1].label_name == join_label) {
                goto_idx = end - 1;
                --end;
            }
            if (end < first + 2)
                return std::nullopt;

            size_t bool_idx = end - 1;
            const auto &bool_ic = fn.icodes[bool_idx];
            bool same_truth = false;
            int cmp_temp = -1;

            if (bool_ic.op == icode_op::ASSIGN &&
                bool_ic.result.is_temp() &&
                bool_ic.result.temp_id == phi_temp &&
                bool_ic.left.is_temp()) {
                cmp_temp = bool_ic.left.temp_id;
                same_truth = true;
            } else if ((bool_ic.op == icode_op::EQ || bool_ic.op == icode_op::NE) &&
                       bool_ic.result.is_temp() &&
                       bool_ic.result.temp_id == phi_temp) {
                bool left_cmp = bool_ic.left.is_temp() &&
                                bool_ic.right.kind == operand_kind::INT_CONST;
                bool right_cmp = bool_ic.right.is_temp() &&
                                 bool_ic.left.kind == operand_kind::INT_CONST;
                if (!left_cmp && !right_cmp)
                    return std::nullopt;
                cmp_temp = left_cmp ? bool_ic.left.temp_id : bool_ic.right.temp_id;
                const int64_t other = left_cmp ? bool_ic.right.ival : bool_ic.left.ival;
                if ((bool_ic.op == icode_op::NE && other == 0) ||
                    (bool_ic.op == icode_op::EQ && other == 1)) {
                    same_truth = true;
                } else if ((bool_ic.op == icode_op::EQ && other == 0) ||
                           (bool_ic.op == icode_op::NE && other == 1)) {
                    same_truth = false;
                } else {
                    return std::nullopt;
                }
            } else {
                return std::nullopt;
            }

            if (temp_used_after(bool_idx + 1, cmp_temp))
                return std::nullopt;

            size_t cmp_idx = bool_idx - 1;
            const auto &cmp_ic = fn.icodes[cmp_idx];
            if (!is_compare_op(cmp_ic.op) ||
                !cmp_ic.result.is_temp() ||
                cmp_ic.result.temp_id != cmp_temp) {
                return std::nullopt;
            }

            return eval_pred_info{cmp_idx, bool_idx, goto_idx, cmp_temp, same_truth};
        };

        auto parse_join_ifx = [&](const basic_block &block)
            -> std::optional<join_ifx_info> {
            size_t first = first_non_label_index(fn, block);
            if (first >= block.end)
                return std::nullopt;

            if (fn.icodes[first].op == icode_op::IFX &&
                fn.icodes[first].left.is_temp() &&
                !fn.icodes[first].true_lbl.empty() &&
                !fn.icodes[first].false_lbl.empty()) {
                return join_ifx_info{
                    static_cast<size_t>(-1),
                    first,
                    fn.icodes[first].left.temp_id,
                    true,
                };
            }

            if (first + 1 >= block.end)
                return std::nullopt;

            const auto &bool_ic = fn.icodes[first];
            const auto &ifx_ic = fn.icodes[first + 1];
            if (ifx_ic.op != icode_op::IFX || !ifx_ic.left.is_temp() ||
                ifx_ic.left.temp_id != bool_ic.result.temp_id ||
                ifx_ic.true_lbl.empty() || ifx_ic.false_lbl.empty()) {
                return std::nullopt;
            }

            if (bool_ic.result.kind != operand_kind::TEMP)
                return std::nullopt;

            if (bool_ic.op == icode_op::ASSIGN && bool_ic.left.is_temp()) {
                return join_ifx_info{
                    first,
                    first + 1,
                    bool_ic.left.temp_id,
                    true,
                };
            }

            if ((bool_ic.op != icode_op::EQ && bool_ic.op != icode_op::NE))
                return std::nullopt;

            const bool left_phi = bool_ic.left.is_temp() &&
                                  bool_ic.right.kind == operand_kind::INT_CONST;
            const bool right_phi = bool_ic.right.is_temp() &&
                                   bool_ic.left.kind == operand_kind::INT_CONST;
            if (!left_phi && !right_phi)
                return std::nullopt;

            const int phi_temp = left_phi ? bool_ic.left.temp_id
                                          : bool_ic.right.temp_id;
            const int64_t other = left_phi ? bool_ic.right.ival : bool_ic.left.ival;
            bool same_truth = false;
            if ((bool_ic.op == icode_op::NE && other == 0) ||
                (bool_ic.op == icode_op::EQ && other == 1)) {
                same_truth = true;
            } else if ((bool_ic.op == icode_op::EQ && other == 0) ||
                       (bool_ic.op == icode_op::NE && other == 1)) {
                same_truth = false;
            } else {
                return std::nullopt;
            }

            return join_ifx_info{
                first,
                first + 1,
                phi_temp,
                same_truth,
            };
        };

        auto block_first_label = [&](size_t block_id) {
            return first_label_in_block(fn, cfg.block(block_id));
        };

        for (const auto &join_block : cfg.blocks()) {
            auto join_info = parse_join_ifx(join_block);
            if (!join_info)
                continue;
            if (join_block.preds.size() != 2)
                continue;

            const auto &join_ifx = fn.icodes[join_info->ifx_idx];
            const int phi_temp = join_info->phi_temp;
            const std::string join_label = block_first_label(join_block.id);
            if (join_label.empty())
                continue;

            std::optional<size_t> const_pred_id;
            std::optional<size_t> eval_pred_id;
            std::optional<const_pred_info> const_info;
            std::optional<eval_pred_info> eval_info;

            for (size_t pred_id : join_block.preds) {
                const auto &pred_block = cfg.block(pred_id);
                auto maybe_const = parse_const_pred(pred_block, phi_temp, join_label);
                if (maybe_const) {
                    const_pred_id = pred_id;
                    const_info = *maybe_const;
                    continue;
                }
                auto maybe_eval = parse_eval_pred(pred_block, phi_temp, join_label);
                if (maybe_eval) {
                    eval_pred_id = pred_id;
                    eval_info = *maybe_eval;
                }
            }

            if (!const_pred_id || !eval_pred_id || !const_info || !eval_info)
                continue;

            std::optional<size_t> branch_pred_id;
            for (size_t pred_of_const : cfg.block(*const_pred_id).preds) {
                const auto &preds_of_eval = cfg.block(*eval_pred_id).preds;
                if (std::find(preds_of_eval.begin(), preds_of_eval.end(), pred_of_const) !=
                    preds_of_eval.end()) {
                    branch_pred_id = pred_of_const;
                    break;
                }
            }
            if (!branch_pred_id)
                continue;

            const auto &branch_block = cfg.block(*branch_pred_id);
            if (branch_block.end <= branch_block.begin)
                continue;
            size_t branch_term_idx = branch_block.end - 1;
            auto &branch_ifx = fn.icodes[branch_term_idx];
            if (branch_ifx.op != icode_op::IFX)
                continue;

            const std::string const_label = block_first_label(*const_pred_id);
            const std::string eval_label = block_first_label(*eval_pred_id);
            if (const_label.empty() || eval_label.empty())
                continue;

            const bool true_to_const = branch_ifx.true_lbl == const_label &&
                                       branch_ifx.false_lbl == eval_label;
            const bool false_to_const = branch_ifx.false_lbl == const_label &&
                                        branch_ifx.true_lbl == eval_label;
            if (!true_to_const && !false_to_const)
                continue;

            const bool const_phi_truth = const_info->value != 0;
            const bool const_join_truth = join_info->same_truth
                                              ? const_phi_truth
                                              : !const_phi_truth;
            const std::string short_circuit_target = const_join_truth
                                                         ? join_ifx.true_lbl
                                                         : join_ifx.false_lbl;

            if (true_to_const) {
                branch_ifx.true_lbl = short_circuit_target;
                branch_ifx.false_lbl = eval_label;
            } else {
                branch_ifx.false_lbl = short_circuit_target;
                branch_ifx.true_lbl = eval_label;
            }

            auto &bool_ic = fn.icodes[eval_info->bool_idx];
            bool_ic.op = icode_op::IFX;
            bool_ic.left = operand::make_temp(eval_info->cmp_temp, type::make_uchar());
            bool_ic.result = operand::make_none();
            bool_ic.right = operand::make_none();
            bool_ic.label_name.clear();
            bool_ic.func_name.clear();
            bool_ic.asm_text.clear();
            bool_ic.num_params = 0;
            bool_ic.local_bytes = 0;
            bool_ic.argreg = -1;
            const bool eval_join_truth = join_info->same_truth
                                             ? eval_info->same_truth
                                             : !eval_info->same_truth;
            if (eval_join_truth) {
                bool_ic.true_lbl = join_ifx.true_lbl;
                bool_ic.false_lbl = join_ifx.false_lbl;
            } else {
                bool_ic.true_lbl = join_ifx.false_lbl;
                bool_ic.false_lbl = join_ifx.true_lbl;
            }

            if (eval_info->goto_idx != static_cast<size_t>(-1))
                erase[eval_info->goto_idx] = true;

            changed = true;
        }

        if (!changed)
            return false;

        std::vector<icode> out;
        out.reserve(fn.icodes.size());
        for (size_t i = 0; i < fn.icodes.size(); ++i) {
            if (!erase[i])
                out.push_back(std::move(fn.icodes[i]));
        }
        fn.icodes = std::move(out);
        return true;
    }
};

class direct_byte_eq_ne_pass final : public ir_pass {
public:
    const char *name() const override { return "direct_byte_eq_ne"; }

    bool run(ir_function &fn) override {
        std::unordered_map<int, const icode *> temp_defs;
        for (const auto &ic : fn.icodes) {
            if (ic.op != icode_op::SET_VALUE_AT && ic.result.is_temp())
                temp_defs[ic.result.temp_id] = &ic;
        }

        auto is_byte_value_operand = [](const operand &op) {
            return op.kind != operand_kind::INT_CONST &&
                   op.type && op.type->size() == 1;
        };

        std::function<std::optional<operand>(const operand &,
                                             std::unordered_set<int> &)>
            resolve_byte_source;
        resolve_byte_source = [&](const operand &op,
                                  std::unordered_set<int> &visiting)
            -> std::optional<operand> {
            if (op.kind == operand_kind::INT_CONST)
                return op;
            if (is_byte_value_operand(op))
                return op;
            if (!op.is_temp())
                return std::nullopt;
            if (!visiting.insert(op.temp_id).second)
                return std::nullopt;
            auto it = temp_defs.find(op.temp_id);
            if (it == temp_defs.end() || !it->second)
                return std::nullopt;
            const icode *def = it->second;
            if (def->op == icode_op::ASSIGN)
                return resolve_byte_source(def->left, visiting);
            if (def->op == icode_op::CAST && def->result.type &&
                def->result.type->size() >= 2 &&
                is_byte_value_operand(def->left)) {
                return def->left;
            }
            return std::nullopt;
        };

        auto fits_byte_const_for_value = [](const operand &value_op,
                                            const operand &const_op) {
            if (const_op.kind != operand_kind::INT_CONST || !value_op.type ||
                value_op.type->size() != 1)
                return false;
            if (value_op.type->is_unsigned())
                return const_op.ival >= 0 && const_op.ival <= 0xff;
            return const_op.ival >= -128 && const_op.ival <= 127;
        };

        auto rewrite_safe = [&](const operand &lhs, const operand &rhs) {
            if (lhs.kind == operand_kind::INT_CONST &&
                rhs.kind == operand_kind::INT_CONST)
                return false;
            if (is_byte_value_operand(lhs) && is_byte_value_operand(rhs)) {
                return lhs.type && rhs.type &&
                       lhs.type->is_unsigned() == rhs.type->is_unsigned();
            }
            if (is_byte_value_operand(lhs) && rhs.kind == operand_kind::INT_CONST)
                return fits_byte_const_for_value(lhs, rhs);
            if (lhs.kind == operand_kind::INT_CONST && is_byte_value_operand(rhs))
                return fits_byte_const_for_value(rhs, lhs);
            return false;
        };

        bool changed = false;
        for (auto &ic : fn.icodes) {
            if (ic.op != icode_op::EQ && ic.op != icode_op::NE)
                continue;
            std::unordered_set<int> lhs_visiting;
            std::unordered_set<int> rhs_visiting;
            auto lhs = resolve_byte_source(ic.left, lhs_visiting);
            auto rhs = resolve_byte_source(ic.right, rhs_visiting);
            if (!lhs || !rhs)
                continue;
            if (!rewrite_safe(*lhs, *rhs))
                continue;
            if (!same_value_operand(ic.left, *lhs) ||
                !same_value_operand(ic.right, *rhs)) {
                ic.left = *lhs;
                ic.right = *rhs;
                changed = true;
            }
        }

        return changed;
    }
};

class promoted_byte_compare_pass final : public ir_pass {
public:
    const char *name() const override { return "promoted_byte_compare"; }

    bool run(ir_function &fn) override {
        std::unordered_map<int, const icode *> temp_defs;
        for (const auto &ic : fn.icodes) {
            if (ic.op != icode_op::SET_VALUE_AT &&
                ic.result.is_temp())
                temp_defs[ic.result.temp_id] = &ic;
        }

        struct narrowed_compare_operand {
            operand value;
            bool supports_u8 = false;
            bool supports_s8 = false;
        };

        auto analyze = [&](const operand &op)
            -> std::optional<narrowed_compare_operand> {
            if (op.kind == operand_kind::INT_CONST) {
                return narrowed_compare_operand{
                    op,
                    op.ival >= 0 && op.ival <= 0xff,
                    op.ival >= -128 && op.ival <= 127,
                };
            }
            if (op.type && op.type->size() == 1) {
                return narrowed_compare_operand{
                    op,
                    op.type->is_unsigned(),
                    !op.type->is_unsigned(),
                };
            }
            if (!op.is_temp())
                return std::nullopt;
            auto it = temp_defs.find(op.temp_id);
            if (it == temp_defs.end())
                return std::nullopt;
            const icode *def = it->second;
            if (!def || def->op != icode_op::CAST)
                return std::nullopt;
            if (!def->result.type || def->result.type->size() < 2)
                return std::nullopt;
            if (!def->left.type || def->left.type->size() != 1)
                return std::nullopt;
            operand narrowed = def->left;
            narrowed.byte_offset += op.byte_offset;
            return narrowed_compare_operand{
                narrowed,
                def->left.type->is_unsigned(),
                !def->left.type->is_unsigned(),
            };
        };

        bool changed = false;
        for (auto &ic : fn.icodes) {
            if (!is_compare_opcode(ic.op))
                continue;
            auto lhs = analyze(ic.left);
            auto rhs = analyze(ic.right);
            if (!lhs || !rhs)
                continue;
            const bool can_u8 = lhs->supports_u8 && rhs->supports_u8;
            const bool can_s8 = lhs->supports_s8 && rhs->supports_s8;
            if (!can_u8 && !can_s8)
                continue;
            ic.left = lhs->value;
            ic.right = rhs->value;
            changed = true;
        }
        return changed;
    }
};

class duplicate_block_merge_pass final : public ir_pass {
public:
    const char *name() const override { return "duplicate_block_merge"; }

    bool run(ir_function &fn) override {
        control_flow_graph cfg(fn);
        std::unordered_map<std::string, std::string> redirects;
        std::unordered_map<std::string, std::string> canonical_by_shape;

        for (const auto &block : cfg.blocks()) {
            if (block.id == 0)
                continue;

            std::vector<std::string> labels = labels_in_block(fn, block);
            if (labels.empty())
                continue;

            size_t body_end = block.end;
            std::string successor;
            if (block.begin < block.end &&
                fn.icodes[block.end - 1].op == icode_op::GOTO) {
                body_end = block.end - 1;
                successor = fn.icodes[block.end - 1].label_name;
            } else if (block.id + 1 < cfg.blocks().size()) {
                successor = first_label_in_block(fn, cfg.block(block.id + 1));
            }

            std::string sig = block_body_signature(fn, block, body_end);
            if (sig.empty())
                continue;

            std::string shape = sig + "=>"+ successor;
            auto [it, inserted] = canonical_by_shape.emplace(shape, labels.front());
            if (inserted)
                continue;

            for (const auto &label : labels) {
                if (label != it->second)
                    redirects.emplace(label, it->second);
            }
        }

        if (redirects.empty())
            return false;

        bool changed = false;
        for (auto &ic : fn.icodes) {
            if (ic.op == icode_op::GOTO) {
                std::string target = resolve_label_redirect(redirects, ic.label_name);
                if (!target.empty() && target != ic.label_name) {
                    ic.label_name = std::move(target);
                    changed = true;
                }
            } else if (ic.op == icode_op::IFX) {
                std::string true_lbl = resolve_label_redirect(redirects, ic.true_lbl);
                std::string false_lbl = resolve_label_redirect(redirects, ic.false_lbl);
                if (!true_lbl.empty() && true_lbl != ic.true_lbl) {
                    ic.true_lbl = std::move(true_lbl);
                    changed = true;
                }
                if (!false_lbl.empty() && false_lbl != ic.false_lbl) {
                    ic.false_lbl = std::move(false_lbl);
                    changed = true;
                }
            }
        }

        return changed;
    }
};

class tail_merge_pass final : public ir_pass {
public:
    const char *name() const override { return "merge_tails"; }

    bool run(ir_function &fn) override {
        control_flow_graph cfg(fn);

        struct candidate {
            size_t canon_split = 0;
            size_t canon_end = 0;
            size_t other_split = 0;
            size_t other_begin = 0;
            size_t other_end = 0;
            size_t suffix_len = 0;
            std::unordered_map<int, int> other_to_canon_temps;
        };

        std::optional<candidate> best;

        for (size_t bi = 1; bi < cfg.blocks().size(); ++bi) {
            const auto &block_i = cfg.block(bi);
            size_t first_i = first_non_label_index(fn, block_i);
            size_t end_i = logical_block_end_for_tail_compare(fn, cfg, block_i);
            if (first_i >= block_i.end ||
                first_i >= end_i ||
                fn.icodes[first_i].op == icode_op::FUNCTION) {
                continue;
            }

            for (size_t bj = bi + 1; bj < cfg.blocks().size(); ++bj) {
                const auto &block_j = cfg.block(bj);
                size_t first_j = first_non_label_index(fn, block_j);
                size_t end_j = logical_block_end_for_tail_compare(fn, cfg, block_j);
                if (first_j >= block_j.end ||
                    first_j >= end_j ||
                    fn.icodes[first_j].op == icode_op::FUNCTION) {
                    continue;
                }

                size_t suffix_len = 0;
                std::unordered_map<int, int> lhs_to_rhs_temps;
                std::unordered_map<int, int> rhs_to_lhs_temps;
                while (first_i + suffix_len < end_i &&
                       first_j + suffix_len < end_j) {
                    const auto &ic_i = fn.icodes[end_i - 1 - suffix_len];
                    const auto &ic_j = fn.icodes[end_j - 1 - suffix_len];
                    if (!is_tail_mergeable_icode(ic_i) ||
                        !is_tail_mergeable_icode(ic_j) ||
                        !icodes_equivalent_for_tail_merge(
                            ic_i, ic_j, lhs_to_rhs_temps, rhs_to_lhs_temps)) {
                        break;
                    }
                    ++suffix_len;
                }

                if (suffix_len < 2)
                    continue;

                size_t split_i = end_i - suffix_len;
                size_t split_j = end_j - suffix_len;
                if (split_i == first_i && split_j == first_j)
                    continue;

                candidate current{split_i, block_i.end, split_j, first_j,
                                  block_j.end, suffix_len, rhs_to_lhs_temps};

                if (!best || current.suffix_len > best->suffix_len)
                    best = current;
            }
        }

        if (!best)
            return false;

        std::string tail_label = make_unique_label(fn, "__xcc_tail_");
        std::vector<icode> shared_tail(
            fn.icodes.begin() + static_cast<std::ptrdiff_t>(best->canon_split),
            fn.icodes.begin() + static_cast<std::ptrdiff_t>(best->canon_end));

        auto rename_temp = [&](operand &op) {
            if (!op.is_temp())
                return;
            auto it = best->other_to_canon_temps.find(op.temp_id);
            if (it != best->other_to_canon_temps.end())
                op.temp_id = it->second;
        };

        for (size_t i = best->other_begin; i < best->other_split; ++i) {
            rename_temp(fn.icodes[i].result);
            rename_temp(fn.icodes[i].left);
            rename_temp(fn.icodes[i].right);
        }

        auto replace_with_goto = [&](size_t start, size_t end) {
            icode goto_ic;
            goto_ic.op = icode_op::GOTO;
            goto_ic.label_name = tail_label;
            if (start < fn.icodes.size())
                goto_ic.line = fn.icodes[start].line;

            auto erase_begin =
                fn.icodes.begin() + static_cast<std::ptrdiff_t>(start);
            auto erase_end =
                fn.icodes.begin() + static_cast<std::ptrdiff_t>(end);
            fn.icodes.erase(erase_begin, erase_end);
            fn.icodes.insert(
                fn.icodes.begin() + static_cast<std::ptrdiff_t>(start),
                std::move(goto_ic));
        };

        replace_with_goto(best->other_split, best->other_end);
        replace_with_goto(best->canon_split, best->canon_end);

        icode label_ic;
        label_ic.op = icode_op::LABEL;
        label_ic.label_name = tail_label;

        auto insert_pos = fn.icodes.end();
        if (!fn.icodes.empty() && fn.icodes.back().op == icode_op::ENDFUNCTION)
            insert_pos = fn.icodes.end() - 1;
        insert_pos = fn.icodes.insert(insert_pos, std::move(label_ic));
        ++insert_pos;
        fn.icodes.insert(insert_pos, shared_tail.begin(), shared_tail.end());
        return true;
    }
};

class value_propagation_pass final : public ir_pass {
public:
    const char *name() const override { return "value_propagation"; }

    bool run(ir_function &fn) override {
        if (fn.icodes.empty()) return false;

        alias_info alias = build_alias_info(fn);
        control_flow_graph cfg(fn);
        auto reachable = cfg.reachable_blocks();
        auto order = cfg.reverse_postorder();

        std::unordered_map<std::string, operand> operand_bank;
        for (auto &ic : fn.icodes) {
            auto remember = [&](const operand &op) {
                std::string key = trackable_key(op, alias);
                if (!key.empty() && !operand_bank.count(key))
                    operand_bank.emplace(key, op);
            };
            remember(ic.result);
            remember(ic.left);
            remember(ic.right);
        }

        std::vector<std::string> ordered_keys;
        ordered_keys.reserve(operand_bank.size());
        for (auto &it : operand_bank)
            ordered_keys.push_back(it.first);
        std::sort(ordered_keys.begin(), ordered_keys.end());

        std::vector<ssa_env> in(cfg.blocks().size());
        std::vector<ssa_env> out(cfg.blocks().size());

        bool dataflow_changed;
        do {
            dataflow_changed = false;
            for (size_t block_id : order) {
                if (!reachable.count(block_id)) continue;

                ssa_env new_in;
                const auto &block = cfg.block(block_id);
                if (block_id != 0 && !block.preds.empty()) {
                    bool first = true;
                    for (size_t pred : block.preds) {
                        if (!reachable.count(pred)) continue;
                        if (first) {
                            new_in = out[pred];
                            first = false;
                        } else {
                            ssa_env merged;
                            for (auto &it : new_in) {
                                auto found = out[pred].find(it.first);
                                if (found != out[pred].end() && found->second == it.second)
                                    merged.emplace(it.first, it.second);
                            }
                            new_in = std::move(merged);
                        }
                    }
                }

                ssa_env env = new_in;
                for (size_t i = block.begin; i < block.end; ++i) {
                    std::string key = def_key(fn.icodes[i], alias);
                    if (!key.empty())
                        env[key] = evaluate_defined_value(fn.icodes[i], i, env, alias);
                }

                if (!env_equal(in[block_id], new_in)) {
                    in[block_id] = std::move(new_in);
                    dataflow_changed = true;
                }
                if (!env_equal(out[block_id], env)) {
                    out[block_id] = std::move(env);
                    dataflow_changed = true;
                }
            }
        } while (dataflow_changed);

        bool changed = false;
        for (size_t block_id : order) {
            if (!reachable.count(block_id)) continue;
            ssa_env env = in[block_id];
            const auto &block = cfg.block(block_id);
            for (size_t i = block.begin; i < block.end; ++i) {
                for_each_use_operand(fn.icodes[i], [&](operand &op) {
                    changed |= rewrite_operand(op, env, alias, operand_bank, ordered_keys);
                });
                std::string key = def_key(fn.icodes[i], alias);
                if (!key.empty())
                    env[key] = evaluate_defined_value(fn.icodes[i], i, env, alias);
            }
        }
        return changed;
    }
};

class calc_temp_fusion_pass final : public ir_pass {
public:
    const char *name() const override { return "calc_temp_fusion"; }

    bool run(ir_function &fn) override {
        if (fn.icodes.empty())
            return false;

        control_flow_graph cfg(fn);
        if (cfg.blocks().size() != 1)
            return false;

        auto supported_ic = [&](const icode &ic) {
            switch (ic.op) {
            case icode_op::FUNCTION:
            case icode_op::ENDFUNCTION:
            case icode_op::RECEIVE:
            case icode_op::RETURN:
            case icode_op::ASSIGN:
            case icode_op::ADDRESS_OF:
            case icode_op::ADD:
            case icode_op::SUB:
            case icode_op::MUL:
            case icode_op::DIV:
            case icode_op::MOD:
            case icode_op::NEG:
            case icode_op::BAND:
            case icode_op::BOR:
            case icode_op::BXOR:
            case icode_op::BNOT:
            case icode_op::SHL:
            case icode_op::SHR:
            case icode_op::ROL:
            case icode_op::ROR:
            case icode_op::PACK_BYTES:
            case icode_op::EQ:
            case icode_op::NE:
            case icode_op::LT:
            case icode_op::LE:
            case icode_op::GT:
            case icode_op::GE:
            case icode_op::CAST:
                return true;
            default:
                return false;
            }
        };

        for (const auto &ic : fn.icodes) {
            if (!supported_ic(ic))
                return false;
        }

        auto consumer_is_copy_sink = [&](const icode &consumer,
                                         int produced_temp) {
            if (!(consumer.left.is_temp() &&
                  consumer.left.temp_id == produced_temp)) {
                return false;
            }
            if (consumer.op == icode_op::ASSIGN)
                return true;
            return consumer.op == icode_op::CAST &&
                   is_noop_scalar_cast(consumer);
        };

        std::unordered_map<int, int> temp_def_count;
        std::unordered_map<int, std::vector<size_t>> temp_use_indices;

        for (size_t i = 0; i < fn.icodes.size(); ++i) {
            const auto &ic = fn.icodes[i];
            if (ic.result.is_temp()) {
                ++temp_def_count[ic.result.temp_id];
            }
            for_each_use_operand(ic, [&](const operand &op) {
                if (op.is_temp())
                    temp_use_indices[op.temp_id].push_back(i);
            });
        }

        auto temp_used_before = [&](int temp_id, size_t index) {
            auto it = temp_use_indices.find(temp_id);
            if (it == temp_use_indices.end())
                return false;
            for (size_t use_idx : it->second)
                if (use_idx < index)
                    return true;
            return false;
        };

        auto barrier_between = [&](size_t begin, size_t end) {
            for (size_t i = begin; i < end; ++i) {
                if (is_local_cse_barrier(fn.icodes[i]))
                    return true;
            }
            return false;
        };

        std::vector<bool> erase(fn.icodes.size(), false);
        bool changed = false;

        for (const auto &block : cfg.blocks()) {
            for (size_t i = block.begin; i < block.end; ++i) {
                if (erase[i])
                    continue;

                auto &producer = fn.icodes[i];
                if (!supported_ic(producer))
                    continue;
                if (!producer.result.is_temp() ||
                    !is_removable_if_dead(producer.op))
                    continue;

                const int produced_temp = producer.result.temp_id;
                if (temp_def_count[produced_temp] != 1)
                    continue;

                auto use_it = temp_use_indices.find(produced_temp);
                if (use_it == temp_use_indices.end() || use_it->second.size() != 1)
                    continue;

                const size_t consumer_idx = use_it->second.front();
                if (consumer_idx <= i || consumer_idx >= block.end || erase[consumer_idx])
                    continue;
                if (barrier_between(i + 1, consumer_idx))
                    continue;

                auto &consumer = fn.icodes[consumer_idx];
                if (!supported_ic(consumer))
                    continue;

                if (!same_type_shape(producer.result.type, consumer.result.type))
                    continue;
                if ((consumer.left.is_temp() &&
                     consumer.left.temp_id == consumer.result.temp_id) ||
                    (consumer.right.is_temp() &&
                     consumer.right.temp_id == consumer.result.temp_id)) {
                    continue;
                }

                if (consumer_is_copy_sink(consumer, produced_temp)) {
                    producer.result = consumer.result;
                    erase[consumer_idx] = true;
                    changed = true;
                    continue;
                }

                if (!consumer.result.is_temp())
                    continue;
                const int consumer_temp = consumer.result.temp_id;
                if (consumer_temp == produced_temp)
                    continue;
                if (temp_def_count[consumer_temp] != 1)
                    continue;
                if (temp_used_before(consumer_temp, consumer_idx))
                    continue;

                operand fused_temp = consumer.result;
                auto replace_single_use = [&](operand &op) {
                    if (op.is_temp() && op.temp_id == produced_temp) {
                        fused_temp.type = op.type ? op.type : fused_temp.type;
                        op = fused_temp;
                    }
                };

                if (!defines_result(consumer) ||
                    !is_removable_if_dead(consumer.op))
                    continue;

                producer.result = consumer.result;
                replace_single_use(consumer.left);
                replace_single_use(consumer.right);
                changed = true;
            }
        }

        if (!changed)
            return false;

        std::vector<icode> out;
        out.reserve(fn.icodes.size());
        for (size_t i = 0; i < fn.icodes.size(); ++i) {
            if (!erase[i])
                out.push_back(fn.icodes[i]);
        }
        fn.icodes = std::move(out);
        return true;
    }
};

class noop_temp_assign_elide_pass final : public ir_pass {
public:
    const char *name() const override { return "noop_temp_assign_elide"; }

    bool run(ir_function &fn) override {
        control_flow_graph cfg(fn);
        if (cfg.blocks().size() != 1)
            return false;

        std::vector<bool> erase(fn.icodes.size(), false);
        bool changed = false;

        for (size_t i = 0; i < fn.icodes.size(); ++i) {
            const auto &ic = fn.icodes[i];
            if (ic.op != icode_op::ASSIGN)
                continue;
            if (!ic.result.is_temp() || !ic.left.is_temp())
                continue;
            if (ic.result.temp_id != ic.left.temp_id)
                continue;
            erase[i] = true;
            changed = true;
        }

        if (!changed)
            return false;

        std::vector<icode> out;
        out.reserve(fn.icodes.size());
        for (size_t i = 0; i < fn.icodes.size(); ++i) {
            if (!erase[i])
                out.push_back(fn.icodes[i]);
        }
        fn.icodes = std::move(out);
        return true;
    }
};

class constant_fold_pass final : public ir_pass {
public:
    const char *name() const override { return "constant_fold"; }

    bool run(ir_function &fn) override {
        bool changed = false;
        for (auto &ic : fn.icodes) {
            if (is_binary_foldable(ic.op) &&
                ic.left.kind == operand_kind::INT_CONST &&
                ic.right.kind == operand_kind::INT_CONST) {
                const int64_t folded = fold_binary(ic.op, ic.left.ival, ic.right.ival);
                ic.op    = icode_op::ASSIGN;
                ic.left  = operand::make_int(folded, ic.result.type);
                ic.right = operand::make_none();
                changed  = true;
            } else if (ic.op == icode_op::NEG &&
                       ic.left.kind == operand_kind::INT_CONST) {
                ic.op   = icode_op::ASSIGN;
                ic.left = operand::make_int(-ic.left.ival, ic.result.type);
                changed = true;
            } else if (ic.op == icode_op::BNOT &&
                       ic.left.kind == operand_kind::INT_CONST) {
                ic.op   = icode_op::ASSIGN;
                ic.left = operand::make_int(~ic.left.ival, ic.result.type);
                changed = true;
            } else if (ic.op == icode_op::CAST &&
                       ic.left.kind == operand_kind::INT_CONST &&
                       ic.result.type) {
                ic.op   = icode_op::ASSIGN;
                ic.left = operand::make_int(cast_int_value(ic.left.ival, ic.result.type),
                                            ic.result.type);
                changed = true;
            }
        }
        return changed;
    }
};

class algebraic_simplify_pass final : public ir_pass {
public:
    const char *name() const override { return "algebraic_simplify"; }

    bool run(ir_function &fn) override {
        bool changed = false;
        for (auto &ic : fn.icodes) {
            const bool left_const  = ic.left.kind == operand_kind::INT_CONST;
            const bool right_const = ic.right.kind == operand_kind::INT_CONST;
            if (is_noop_scalar_cast(ic)) {
                ic.op = icode_op::ASSIGN;
                ic.right = operand::make_none();
                changed = true;
                continue;
            }
            if (!left_const && !right_const)
                continue;

            apply_ir_binary_identity_rules(
                ic.op,
                left_const, ic.left.ival,
                right_const, ic.right.ival,
                ic.result.type,
                [&](const ir_binary_identity_rule &rule, bool const_on_left) {
                    operand replacement = operand::make_none();
                    switch (rule.action) {
                    case ir_identity_action::keep_other_operand:
                        replacement = const_on_left ? ic.right : ic.left;
                        break;
                    case ir_identity_action::replace_with_zero:
                        replacement = operand::make_int(0, ic.result.type);
                        break;
                    }

                    ic.op = icode_op::ASSIGN;
                    ic.left = std::move(replacement);
                    ic.right = operand::make_none();
                    changed = true;
                    return true;
                });
        }
        return changed;
    }
};

static bool is_local_cse_barrier(const icode &ic) {
    switch (ic.op) {
    case icode_op::FUNCTION:
    case icode_op::ENDFUNCTION:
    case icode_op::LABEL:
    case icode_op::GOTO:
    case icode_op::IFX:
    case icode_op::CALL:
    case icode_op::RETURN:
    case icode_op::SEND:
    case icode_op::RECEIVE:
    case icode_op::GET_VALUE_AT:
    case icode_op::SET_VALUE_AT:
    case icode_op::INLINE_ASM:
        return true;
    default:
        return false;
    }
}

static bool is_local_cse_commutative(icode_op op) {
    switch (op) {
    case icode_op::ADD:
    case icode_op::MUL:
    case icode_op::BAND:
    case icode_op::BOR:
    case icode_op::BXOR:
    case icode_op::EQ:
    case icode_op::NE:
        return true;
    default:
        return false;
    }
}

static bool is_local_cse_candidate(const icode &ic) {
    switch (ic.op) {
    case icode_op::ADD:
    case icode_op::SUB:
    case icode_op::MUL:
    case icode_op::DIV:
    case icode_op::MOD:
    case icode_op::SHL:
    case icode_op::SHR:
    case icode_op::ROL:
    case icode_op::ROR:
    case icode_op::BAND:
    case icode_op::BOR:
    case icode_op::BXOR:
    case icode_op::EQ:
    case icode_op::NE:
    case icode_op::LT:
    case icode_op::LE:
    case icode_op::GT:
    case icode_op::GE:
    case icode_op::NEG:
    case icode_op::BNOT:
    case icode_op::CAST:
        return ic.result.kind != operand_kind::NONE;
    default:
        return false;
    }
}

static std::string local_cse_operand_key(const operand &op) {
    switch (op.kind) {
    case operand_kind::TEMP:
        return "T:" + std::to_string(op.temp_id) + ":" +
               std::to_string(op.byte_offset);
    case operand_kind::SYMBOL:
        return "S:" + symbol_key(op) + ":" + std::to_string(op.byte_offset);
    case operand_kind::INT_CONST:
        return "I:" + std::to_string(op.ival) + ":" +
               std::to_string(op.type ? op.type->size() : 0) + ":" +
               std::to_string(op.type && op.type->is_unsigned());
    case operand_kind::FLOAT_CONST:
        return "F:" + std::to_string(op.fval);
    case operand_kind::LABEL_REF:
        return "L:" + op.name;
    default:
        return {};
    }
}

static std::string local_cse_expr_key(const icode &ic) {
    std::string left = local_cse_operand_key(ic.left);
    std::string right = local_cse_operand_key(ic.right);
    if (left.empty() || (ic.op != icode_op::NEG &&
                         ic.op != icode_op::BNOT &&
                         ic.op != icode_op::CAST &&
                         right.empty())) {
        return {};
    }

    if (is_local_cse_commutative(ic.op) && right < left)
        std::swap(left, right);

    std::string type_key =
        ic.result.type ? std::to_string(static_cast<int>(ic.result.type->kind)) +
                             ":" + std::to_string(ic.result.type->size()) +
                             ":" + std::to_string(ic.result.type->is_unsigned())
                       : "0:0:0";

    return std::to_string(static_cast<int>(ic.op)) + "|" + type_key + "|" +
           left + "|" + right;
}

static void local_cse_clear(
    std::unordered_map<std::string, operand> &expr_to_result,
    std::unordered_map<std::string, std::vector<std::string>> &dep_to_exprs) {
    expr_to_result.clear();
    dep_to_exprs.clear();
}

class local_cse_pass final : public ir_pass {
public:
    const char *name() const override { return "local_cse"; }

    bool run(ir_function &fn) override {
        std::unordered_map<std::string, operand> expr_to_result;
        std::unordered_map<std::string, std::vector<std::string>> dep_to_exprs;
        bool changed = false;

        auto invalidate_operand = [&](const operand &op) {
            std::string dep = local_cse_operand_key(op);
            if (dep.empty())
                return;
            auto it = dep_to_exprs.find(dep);
            if (it == dep_to_exprs.end())
                return;
            for (const auto &expr_key : it->second)
                expr_to_result.erase(expr_key);
            dep_to_exprs.erase(it);
        };

        auto note_dep = [&](const operand &op, const std::string &expr_key) {
            std::string dep = local_cse_operand_key(op);
            if (dep.empty())
                return;
            dep_to_exprs[dep].push_back(expr_key);
        };

        for (auto &ic : fn.icodes) {
            if (is_local_cse_barrier(ic)) {
                local_cse_clear(expr_to_result, dep_to_exprs);
                continue;
            }

            if (defines_result(ic))
                invalidate_operand(ic.result);

            if (!is_local_cse_candidate(ic))
                continue;

            std::string expr_key = local_cse_expr_key(ic);
            if (expr_key.empty())
                continue;

            auto it = expr_to_result.find(expr_key);
            if (it != expr_to_result.end()) {
                ic.op = icode_op::ASSIGN;
                ic.left = it->second;
                ic.right = operand{};
                changed = true;
                continue;
            }

            expr_to_result.emplace(expr_key, ic.result);
            note_dep(ic.left, expr_key);
            note_dep(ic.right, expr_key);
        }

        return changed;
    }
};

struct available_byte_load_entry {
    operand value;
    std::vector<std::string> deps;
};

static bool is_available_byte_load_barrier(const icode &ic) {
    switch (ic.op) {
    case icode_op::FUNCTION:
    case icode_op::ENDFUNCTION:
    case icode_op::CALL:
    case icode_op::INLINE_ASM:
    case icode_op::RECEIVE:
    case icode_op::SEND:
    case icode_op::ALLOCA:
    case icode_op::SET_VALUE_AT:
        return true;
    case icode_op::ASSIGN:
        return ic.result.is_symbol() &&
               (ic.result.is_global || ic.result.is_tls || ic.result.is_sfr);
    default:
        return false;
    }
}

static bool is_available_byte_load_candidate(const icode &ic) {
    if (ic.op != icode_op::GET_VALUE_AT ||
        ic.result.kind == operand_kind::NONE ||
        !ic.result.type ||
        ic.result.type->size() != 1)
        return false;

    if (ic.left.kind == operand_kind::LABEL_REF)
        return !ic.right.is_none();

    return ic.right.is_none() &&
           !local_cse_operand_key(ic.left).empty();
}

static std::string available_byte_load_key(const icode &ic) {
    if (!is_available_byte_load_candidate(ic))
        return {};

    if (ic.left.kind == operand_kind::LABEL_REF) {
        std::string idx = local_cse_operand_key(ic.right);
        if (idx.empty())
            return {};
        return "LOAD8|" + ic.left.name + "|" + idx;
    }

    std::string ptr = local_cse_operand_key(ic.left);
    if (ptr.empty())
        return {};
    return "LOAD8PTR|" + ptr;
}

static std::vector<std::string> available_byte_load_deps(const icode &ic) {
    std::vector<std::string> deps;
    std::string ptr = local_cse_operand_key(ic.left);
    if (!ptr.empty())
        deps.push_back(ptr);
    std::string idx = local_cse_operand_key(ic.right);
    if (!idx.empty())
        deps.push_back(idx);
    return deps;
}

static void available_byte_load_invalidate_operand(
    std::unordered_map<std::string, available_byte_load_entry> &avail,
    const operand &op) {
    std::string dep = local_cse_operand_key(op);
    if (dep.empty() || avail.empty())
        return;

    for (auto it = avail.begin(); it != avail.end();) {
        bool erase = false;
        for (const auto &entry_dep : it->second.deps) {
            if (entry_dep == dep) {
                erase = true;
                break;
            }
        }
        if (erase)
            it = avail.erase(it);
        else
            ++it;
    }
}

static bool available_byte_load_maps_equal(
    const std::unordered_map<std::string, available_byte_load_entry> &lhs,
    const std::unordered_map<std::string, available_byte_load_entry> &rhs) {
    if (lhs.size() != rhs.size())
        return false;
    for (const auto &it : lhs) {
        auto found = rhs.find(it.first);
        if (found == rhs.end())
            return false;
        if (!same_value_operand(it.second.value, found->second.value))
            return false;
    }
    return true;
}

class available_byte_load_pass final : public ir_pass {
public:
    const char *name() const override { return "available_byte_load"; }

    bool run(ir_function &fn) override {
        if (fn.icodes.empty())
            return false;

        std::unordered_map<int, const icode *> temp_defs;
        for (const auto &ic : fn.icodes) {
            if (ic.result.is_temp())
                temp_defs[ic.result.temp_id] = &ic;
        }

        control_flow_graph cfg(fn);
        auto reachable = cfg.reachable_blocks();
        auto order = cfg.reverse_postorder();
        const size_t nblocks = cfg.blocks().size();

        using avail_map = std::unordered_map<std::string, available_byte_load_entry>;
        std::vector<avail_map> in(nblocks), out(nblocks);

        auto transfer = [&](const basic_block &block, const avail_map &start) {
            avail_map current = start;
            for (size_t i = block.begin; i < block.end; ++i) {
                const icode &ic = fn.icodes[i];

                if (is_available_byte_load_barrier(ic)) {
                    current.clear();
                    continue;
                }

                if (defines_result(ic))
                    available_byte_load_invalidate_operand(current, ic.result);

                std::string key = available_byte_load_key(ic);
                if (!key.empty()) {
                    auto it = current.find(key);
                    if (it == current.end()) {
                        current.emplace(key, available_byte_load_entry{
                                                ic.result,
                                                available_byte_load_deps(ic)});
                    }
                }
            }
            return current;
        };

        bool changed = true;
        while (changed) {
            changed = false;
            for (size_t block_id : order) {
                if (!reachable.count(block_id))
                    continue;

                avail_map merged;
                bool have_pred = false;
                const auto &block = cfg.block(block_id);
                for (size_t pred_id : block.preds) {
                    if (!reachable.count(pred_id))
                        continue;
                    if (!have_pred) {
                        merged = out[pred_id];
                        have_pred = true;
                        continue;
                    }

                    for (auto it = merged.begin(); it != merged.end();) {
                        auto found = out[pred_id].find(it->first);
                        if (found == out[pred_id].end() ||
                            !same_value_operand(it->second.value,
                                                found->second.value)) {
                            it = merged.erase(it);
                        } else {
                            ++it;
                        }
                    }
                }

                if (!have_pred)
                    merged.clear();

                if (!available_byte_load_maps_equal(in[block_id], merged)) {
                    in[block_id] = merged;
                    changed = true;
                }

                avail_map new_out = transfer(block, in[block_id]);
                if (!available_byte_load_maps_equal(out[block_id], new_out)) {
                    out[block_id] = std::move(new_out);
                    changed = true;
                }
            }
        }

        bool rewritten = false;
        for (size_t block_id : order) {
            if (!reachable.count(block_id))
                continue;

            avail_map current = in[block_id];
            const auto &block = cfg.block(block_id);
            for (size_t i = block.begin; i < block.end; ++i) {
                icode &ic = fn.icodes[i];

                if (is_available_byte_load_barrier(ic)) {
                    current.clear();
                    continue;
                }

                if (defines_result(ic))
                    available_byte_load_invalidate_operand(current, ic.result);

                std::string key = available_byte_load_key(ic);
                if (key.empty())
                    continue;

                auto it = current.find(key);
                if (it != current.end()) {
                    ic.op = icode_op::ASSIGN;
                    ic.left = it->second.value;
                    ic.right = operand::make_none();
                    rewritten = true;
                }

                current[key] = available_byte_load_entry{
                    ic.result,
                    available_byte_load_deps(ic)
                };
            }
        }

        return rewritten;
    }
};

class loop_licm_pass final : public ir_pass {
public:
    const char *name() const override { return "loop_licm"; }

    bool run(ir_function &fn) override {
        if (fn.icodes.empty()) return false;

        alias_info alias = build_alias_info(fn);
        control_flow_graph cfg(fn);
        auto loops = cfg.natural_loops();
        if (loops.empty()) return false;

        std::unordered_set<size_t> hoisted;
        std::map<size_t, std::vector<icode>> insert_before;

        for (auto &loop : loops) {
            if (loop.outside_preds.size() != 1) continue;

            std::unordered_set<std::string> defs_in_loop;
            std::unordered_map<std::string, int> def_counts_in_loop;
            for (size_t block_id : loop.blocks) {
                const auto &block = cfg.block(block_id);
                for (size_t i = block.begin; i < block.end; ++i) {
                    std::string key = def_key(fn.icodes[i], alias);
                    if (!key.empty()) {
                        defs_in_loop.insert(key);
                        ++def_counts_in_loop[key];
                    }
                }
            }

            std::unordered_set<std::string> invariant = defs_in_loop;
            for (auto it = invariant.begin(); it != invariant.end(); ) {
                ++it;
            }
            invariant.clear();

            std::vector<size_t> selected;
            std::unordered_set<std::string> hoisted_defs;

            bool local_changed;
            do {
                local_changed = false;
                for (size_t block_id : loop.blocks) {
                    const auto &block = cfg.block(block_id);
                    for (size_t i = block.begin; i < block.end; ++i) {
                        if (hoisted.count(i)) continue;
                        if (std::find(selected.begin(), selected.end(), i) != selected.end())
                            continue;

                        icode &ic = fn.icodes[i];
                        if (!is_licm_safe(ic.op) || !ic.result.is_temp())
                            continue;

                        std::string result_key = trackable_key(ic.result, alias);
                        if (!result_key.empty() &&
                            def_counts_in_loop[result_key] != 1)
                            continue;

                        bool ok = true;
                        for_each_use_operand(ic, [&](const operand &op) {
                            if (op.kind == operand_kind::INT_CONST) return;
                            std::string key = trackable_key(op, alias);
                            if (key.empty()) {
                                ok = false;
                                return;
                            }
                            if (defs_in_loop.count(key) && !hoisted_defs.count(key))
                                ok = false;
                        });
                        if (!ok) continue;

                        selected.push_back(i);
                        if (!result_key.empty()) hoisted_defs.insert(result_key);
                        hoisted.insert(i);
                        local_changed = true;
                    }
                }
            } while (local_changed);

            if (selected.empty()) continue;

            std::sort(selected.begin(), selected.end());
            size_t preheader = loop.outside_preds.front();
            size_t insert_at = insertion_index_before_terminator(cfg.block(preheader), fn);
            auto &bucket = insert_before[insert_at];
            for (size_t index : selected)
                bucket.push_back(fn.icodes[index]);
        }

        if (hoisted.empty()) return false;
        fn.icodes = rebuild_with_insertions(fn.icodes, hoisted, insert_before);
        return true;
    }
};

class loop_induction_pass final : public ir_pass {
public:
    const char *name() const override { return "loop_induction"; }

    bool run(ir_function &fn) override {
        if (fn.icodes.empty()) return false;

        alias_info alias = build_alias_info(fn);
        control_flow_graph cfg(fn);
        auto loops = cfg.natural_loops();
        if (loops.empty()) return false;

        int next_temp = next_temp_id(fn);
        std::map<size_t, std::vector<icode>> insert_before;
        std::unordered_map<size_t, operand> replace_with;

        for (auto &loop : loops) {
            if (loop.outside_preds.size() != 1 || loop.latches.size() != 1)
                continue;

            const auto &preheader = cfg.block(loop.outside_preds.front());
            const auto &latch = cfg.block(loop.latches.front());

            std::unordered_map<std::string, int> def_counts;
            for (size_t block_id : loop.blocks) {
                const auto &block = cfg.block(block_id);
                for (size_t i = block.begin; i < block.end; ++i) {
                    std::string key = def_key(fn.icodes[i], alias);
                    if (!key.empty()) ++def_counts[key];
                }
            }

            struct induction_var {
                std::string key;
                operand     op;
                int64_t     step = 0;
            };
            std::vector<induction_var> ivs;

            for (size_t i = latch.begin; i < latch.end; ++i) {
                icode &ic = fn.icodes[i];
                if ((ic.op != icode_op::ADD && ic.op != icode_op::SUB) ||
                    ic.right.kind != operand_kind::INT_CONST)
                    continue;

                std::string result_key = trackable_key(ic.result, alias);
                std::string left_key   = trackable_key(ic.left, alias);
                if (result_key.empty() || result_key != left_key)
                    continue;
                if (def_counts[result_key] != 1)
                    continue;

                induction_var iv;
                iv.key  = result_key;
                iv.op   = ic.result;
                iv.step = (ic.op == icode_op::ADD) ? ic.right.ival : -ic.right.ival;
                ivs.push_back(iv);
            }

            if (ivs.empty()) continue;

            size_t pre_insert = insertion_index_before_terminator(preheader, fn);
            size_t latch_insert = insertion_index_before_terminator(latch, fn);

            for (auto &iv : ivs) {
                struct scaled_value {
                    int64_t factor = 0;
                    type_ptr type;
                    operand scaled;
                };
                std::vector<scaled_value> scaled_values;

                for (size_t block_id : loop.blocks) {
                    if (block_id == loop.latches.front()) continue;
                    const auto &block = cfg.block(block_id);
                    for (size_t i = block.begin; i < block.end; ++i) {
                        icode &ic = fn.icodes[i];
                        if (ic.op != icode_op::MUL) continue;

                        bool left_match = trackable_key(ic.left, alias) == iv.key &&
                                          ic.right.kind == operand_kind::INT_CONST;
                        bool right_match = trackable_key(ic.right, alias) == iv.key &&
                                           ic.left.kind == operand_kind::INT_CONST;
                        if (!left_match && !right_match) continue;

                        int64_t factor = left_match ? ic.right.ival : ic.left.ival;
                        if (factor == 0 || factor == 1 || factor == -1) continue;

                        scaled_value *found = nullptr;
                        for (auto &entry : scaled_values) {
                            if (entry.factor == factor && entry.type == ic.result.type) {
                                found = &entry;
                                break;
                            }
                        }
                        if (!found) {
                            scaled_value entry;
                            entry.factor = factor;
                            entry.type   = ic.result.type ? ic.result.type : iv.op.type;
                            entry.scaled = make_fresh_temp(next_temp, entry.type);

                            icode init;
                            init.op     = icode_op::MUL;
                            init.result = entry.scaled;
                            init.left   = iv.op;
                            init.right  = operand::make_int(factor, entry.type);
                            insert_before[pre_insert].push_back(init);

                            int64_t delta = iv.step * factor;
                            if (delta != 0) {
                                icode update;
                                update.op     = delta > 0 ? icode_op::ADD : icode_op::SUB;
                                update.result = entry.scaled;
                                update.left   = entry.scaled;
                                update.right  = operand::make_int(delta > 0 ? delta : -delta,
                                                                  entry.type);
                                insert_before[latch_insert].push_back(update);
                            }

                            scaled_values.push_back(entry);
                            found = &scaled_values.back();
                        }

                        replace_with[i] = found->scaled;
                    }
                }
            }
        }

        if (replace_with.empty()) return false;

        std::unordered_set<size_t> skip;
        std::vector<icode> rebuilt;
        rebuilt.reserve(fn.icodes.size() + replace_with.size() * 2);

        for (size_t i = 0; i <= fn.icodes.size(); ++i) {
            auto add = insert_before.find(i);
            if (add != insert_before.end())
                rebuilt.insert(rebuilt.end(), add->second.begin(), add->second.end());
            if (i == fn.icodes.size()) break;

            auto repl = replace_with.find(i);
            if (repl == replace_with.end()) {
                rebuilt.push_back(fn.icodes[i]);
                continue;
            }

            icode ic = fn.icodes[i];
            ic.op    = icode_op::ASSIGN;
            ic.left  = repl->second;
            ic.right = operand::make_none();
            rebuilt.push_back(std::move(ic));
        }

        fn.icodes = std::move(rebuilt);
        return true;
    }
};

class strength_reduction_pass final : public ir_pass {
public:
    const char *name() const override { return "strength_reduce"; }

    bool run(ir_function &fn) override {
        if (fn.icodes.empty()) return false;

        int next_temp = next_temp_id(fn);
        std::vector<icode> output;
        output.reserve(fn.icodes.size() + 8);
        bool changed = false;

        for (auto &ic : fn.icodes) {
            if (ic.right.kind == operand_kind::INT_CONST) {
                int64_t rval = ic.right.ival;
                int shift = log2_exact(rval);

                if (ic.op == icode_op::MUL && shift >= 0) {
                    icode reduced = ic;
                    reduced.op    = icode_op::SHL;
                    reduced.right = operand::make_int(shift, ic.right.type);
                    output.push_back(reduced);
                    changed = true;
                    continue;
                }

                if (ic.op == icode_op::DIV && shift >= 0 &&
                    ic.left.type && ic.left.type->is_unsigned()) {
                    icode reduced = ic;
                    reduced.op    = icode_op::SHR;
                    reduced.right = operand::make_int(shift, ic.right.type);
                    output.push_back(reduced);
                    changed = true;
                    continue;
                }

                if (ic.op == icode_op::MOD && shift >= 0 &&
                    ic.left.type && ic.left.type->is_unsigned()) {
                    icode reduced = ic;
                    reduced.op    = icode_op::BAND;
                    reduced.right = operand::make_int(rval - 1, ic.right.type);
                    output.push_back(reduced);
                    changed = true;
                    continue;
                }

                if (ic.op == icode_op::MUL && rval != 0 && rval != 1 && rval != -1 &&
                    std::abs(rval) <= 255 &&
                    popcount_u64(static_cast<uint64_t>(std::abs(rval))) <= 3) {
                    uint64_t mag = static_cast<uint64_t>(std::abs(rval));
                    std::vector<operand> terms;
                    type_ptr t = ic.result.type ? ic.result.type : ic.left.type;

                    for (int bit = 0; mag; ++bit, mag >>= 1) {
                        if ((mag & 1U) == 0) continue;
                        if (bit == 0) {
                            terms.push_back(ic.left);
                        } else {
                            operand shifted = make_fresh_temp(next_temp, t);
                            icode sh;
                            sh.op     = icode_op::SHL;
                            sh.result = shifted;
                            sh.left   = ic.left;
                            sh.right  = operand::make_int(bit, type::make_int());
                            output.push_back(sh);
                            terms.push_back(shifted);
                        }
                    }

                    if (!terms.empty()) {
                        operand accum = terms.front();
                        for (size_t j = 1; j < terms.size(); ++j) {
                            operand dst = (j + 1 == terms.size() && rval > 0)
                                          ? ic.result
                                          : make_fresh_temp(next_temp, t);
                            icode add;
                            add.op     = icode_op::ADD;
                            add.result = dst;
                            add.left   = accum;
                            add.right  = terms[j];
                            output.push_back(add);
                            accum = dst;
                        }

                        if (rval < 0) {
                            icode neg;
                            neg.op     = icode_op::NEG;
                            neg.result = ic.result;
                            neg.left   = accum;
                            output.push_back(neg);
                        } else if (terms.size() == 1) {
                            icode asn;
                            asn.op     = icode_op::ASSIGN;
                            asn.result = ic.result;
                            asn.left   = accum;
                            output.push_back(asn);
                        }
                        changed = true;
                        continue;
                    }
                }
            }

            output.push_back(ic);
        }

        if (changed) fn.icodes = std::move(output);
        return changed;
    }
};

class promoted_byte_ops_pass final : public ir_pass {
public:
    const char *name() const override { return "promoted_byte_ops"; }

    bool run(ir_function &fn) override {
        std::unordered_map<int, const icode *> temp_defs;
        std::unordered_map<int, int> temp_use_count;

        for (const auto &ic : fn.icodes) {
            if (ic.result.is_temp())
                temp_defs[ic.result.temp_id] = &ic;
            for_each_use_operand(ic, [&](const operand &op) {
                if (op.is_temp())
                    ++temp_use_count[op.temp_id];
            });
        }

        std::function<bool(int, std::unordered_set<int>&)> promotable_temp_expr =
            [&](int temp_id, std::unordered_set<int> &visiting) -> bool {
                if (!visiting.insert(temp_id).second)
                    return false;
                auto it = temp_defs.find(temp_id);
                if (it == temp_defs.end() || !it->second)
                    return false;
                const icode *def = it->second;
                if (!def->result.type || def->result.type->size() < 2)
                    return false;
                if (temp_use_count[temp_id] != 1)
                    return false;

                auto operand_promotable = [&](const operand &candidate) -> bool {
                    if (fits_u8_like_value(candidate))
                        return true;
                    if (!candidate.is_temp())
                        return false;
                    auto inner = temp_defs.find(candidate.temp_id);
                    if (inner == temp_defs.end() || !inner->second)
                        return false;
                    const icode *inner_def = inner->second;
                    if (inner_def->op == icode_op::CAST && inner_def->result.type &&
                        inner_def->result.type->size() >= 2 &&
                        fits_u8_like_value(inner_def->left)) {
                        return true;
                    }
                    return promotable_temp_expr(candidate.temp_id, visiting);
                };

                switch (def->op) {
                case icode_op::ADD:
                case icode_op::SUB:
                case icode_op::BAND:
                case icode_op::BOR:
                case icode_op::BXOR:
                    return operand_promotable(def->left) &&
                           operand_promotable(def->right);
                case icode_op::SHL:
                case icode_op::SHR:
                    return def->right.kind == operand_kind::INT_CONST &&
                           def->right.ival >= 0 && def->right.ival <= 7 &&
                           operand_promotable(def->left);
                default:
                    return false;
                }
            };

        auto unwrap_input = [&](const operand &op) -> std::optional<operand> {
            if (fits_u8_like_value(op))
                return op;
            if (!op.is_temp())
                return std::nullopt;
            auto it = temp_defs.find(op.temp_id);
            if (it == temp_defs.end())
                return std::nullopt;
            const icode *def = it->second;
            if (!def || def->op != icode_op::CAST || !def->result.type)
            {
                std::unordered_set<int> visiting;
                if (promotable_temp_expr(op.temp_id, visiting))
                    return op;
                return std::nullopt;
            }
            if (def->result.type->size() < 2 || !fits_u8_like_value(def->left))
                return std::nullopt;
            return def->left;
        };

        auto make_byte_const = [](const operand &op, const type_ptr &type) {
            if (op.kind == operand_kind::INT_CONST)
                return operand::make_int(op.ival & 0xff, type);
            return op;
        };

        auto stores_through_byte_ptr = [](const icode &ic, int temp_id) {
            if (ic.op != icode_op::SET_VALUE_AT ||
                !ic.left.is_temp() ||
                ic.left.temp_id != temp_id ||
                !ic.result.type ||
                !ic.result.type->is_ptr() ||
                !ic.result.type->base) {
                return false;
            }
            return ic.result.type->base->size() == 1;
        };

        std::function<bool(int, int)> flows_to_byte_sink;
        flows_to_byte_sink = [&](int temp_id, int depth) -> bool {
            if (depth > 8)
                return false;
            if (temp_use_count[temp_id] != 1)
                return false;

            size_t user_idx = fn.icodes.size();
            for (size_t j = 0; j < fn.icodes.size(); ++j) {
                bool uses = false;
                for_each_use_operand(fn.icodes[j], [&](const operand &op) {
                    if (op.is_temp() && op.temp_id == temp_id)
                        uses = true;
                });
                if (uses) {
                    user_idx = j;
                    break;
                }
            }
            if (user_idx == fn.icodes.size())
                return false;

            const auto &user = fn.icodes[user_idx];
            if (user.op == icode_op::CAST && user.result.type &&
                user.result.type->size() == 1)
                return true;
            if (user.op == icode_op::ASSIGN && user.result.type &&
                user.result.type->size() == 1)
                return true;
            if (stores_through_byte_ptr(user, temp_id))
                return true;

            switch (user.op) {
            case icode_op::ADD:
            case icode_op::SUB:
            case icode_op::BAND:
            case icode_op::BOR:
            case icode_op::BXOR:
                break;
            case icode_op::SHL:
            case icode_op::SHR:
                if (!(user.right.kind == operand_kind::INT_CONST &&
                      user.right.ival >= 0 && user.right.ival <= 7))
                    return false;
                break;
            default:
                return false;
            }

            if (!user.result.is_temp())
                return false;
            return flows_to_byte_sink(user.result.temp_id, depth + 1);
        };

        std::vector<bool> erase(fn.icodes.size(), false);
        bool changed = false;

        for (size_t i = 0; i < fn.icodes.size(); ++i) {
            auto &ic = fn.icodes[i];
            if (!ic.result.is_temp())
                continue;

            bool candidate = false;
            switch (ic.op) {
            case icode_op::ADD:
            case icode_op::SUB:
            case icode_op::BAND:
            case icode_op::BOR:
            case icode_op::BXOR:
                candidate = true;
                break;
            case icode_op::SHL:
            case icode_op::SHR:
                candidate = ic.right.kind == operand_kind::INT_CONST &&
                            ic.right.ival >= 0 && ic.right.ival <= 7;
                break;
            default:
                break;
            }
            if (!candidate)
                continue;

            if (temp_use_count[ic.result.temp_id] != 1)
                continue;

            size_t user_idx = fn.icodes.size();
            for (size_t j = i + 1; j < fn.icodes.size(); ++j) {
                bool uses = false;
                for_each_use_operand(fn.icodes[j], [&](const operand &op) {
                    if (op.is_temp() && op.temp_id == ic.result.temp_id)
                        uses = true;
                });
                if (uses) {
                    user_idx = j;
                    break;
                }
                if (fn.icodes[j].result.is_temp() &&
                    fn.icodes[j].result.temp_id == ic.result.temp_id) {
                    break;
                }
            }
            if (user_idx == fn.icodes.size())
                continue;

            auto &user = fn.icodes[user_idx];
            const int produced_temp_id = ic.result.temp_id;
            const bool cast_consumer =
                user.op == icode_op::CAST && user.result.type &&
                user.result.type->size() == 1 &&
                user.left.is_temp() && user.left.temp_id == ic.result.temp_id;
            const bool byte_store_consumer =
                stores_through_byte_ptr(user, ic.result.temp_id);

            const bool byte_chain_consumer =
                ((user.left.is_temp() && user.left.temp_id == ic.result.temp_id) ||
                 (user.right.is_temp() && user.right.temp_id == ic.result.temp_id)) &&
                (user.op == icode_op::ADD || user.op == icode_op::SUB ||
                 user.op == icode_op::BAND || user.op == icode_op::BOR ||
                 user.op == icode_op::BXOR || user.op == icode_op::SHL ||
                 user.op == icode_op::SHR) &&
                ((user.result.type && user.result.type->size() == 1 &&
                  user.result.type->is_unsigned()) ||
                 (user.result.is_temp() &&
                  flows_to_byte_sink(user.result.temp_id, 0)));

            const bool compare_consumer =
                is_compare_opcode(user.op) &&
                ((user.left.is_temp() && user.left.temp_id == produced_temp_id) ||
                 (user.right.is_temp() && user.right.temp_id == produced_temp_id));
            const bool ifx_consumer =
                user.op == icode_op::IFX &&
                user.left.is_temp() && user.left.temp_id == produced_temp_id;

            if (!cast_consumer && !byte_store_consumer && !byte_chain_consumer &&
                !compare_consumer && !ifx_consumer) {
                continue;
            }

            if (compare_consumer) {
                switch (ic.op) {
                case icode_op::BAND:
                case icode_op::BOR:
                case icode_op::BXOR:
                    break;
                case icode_op::SHL:
                case icode_op::SHR:
                    if (!(ic.right.kind == operand_kind::INT_CONST &&
                          ic.right.ival >= 0 && ic.right.ival <= 7)) {
                        continue;
                    }
                    break;
                default:
                    continue;
                }
            }

            auto lhs = unwrap_input(ic.left);
            if (!lhs)
                continue;

            std::optional<operand> rhs;
            if (ic.op == icode_op::SHL || ic.op == icode_op::SHR) {
                rhs = ic.right;
            } else {
                rhs = unwrap_input(ic.right);
                if (!rhs)
                    continue;
            }

            type_ptr byte_type = user.result.type;
            if (byte_chain_consumer &&
                (!byte_type || byte_type->size() != 1))
                byte_type = type::make_uchar();
            if (byte_store_consumer)
                byte_type = type::make_uchar();
            if (compare_consumer)
                byte_type = type::make_uchar();
            if (ifx_consumer)
                byte_type = type::make_uchar();

            std::optional<operand> compare_other_rewrite;
            if (compare_consumer) {
                const operand &other =
                    (user.left.is_temp() && user.left.temp_id == produced_temp_id)
                        ? user.right
                        : user.left;
                auto other_byte = unwrap_input(other);
                if (other_byte) {
                    compare_other_rewrite = make_byte_const(*other_byte, byte_type);
                } else if (other.kind == operand_kind::INT_CONST) {
                    if (other.ival < 0 || other.ival > 0xff)
                        continue;
                    compare_other_rewrite = make_byte_const(other, byte_type);
                } else {
                    continue;
                }
            }

            ic.left = make_byte_const(*lhs, byte_type);
            if (rhs)
                ic.right = make_byte_const(*rhs, byte_type);

            if (cast_consumer) {
                ic.result = user.result;
                ic.result.type = byte_type;
                erase[user_idx] = true;
            } else {
                ic.result.type = byte_type;
                if (user.left.is_temp() && user.left.temp_id == produced_temp_id)
                    user.left.type = byte_type;
                if (user.right.is_temp() && user.right.temp_id == produced_temp_id)
                    user.right.type = byte_type;

                if (compare_consumer) {
                    operand *other =
                        (user.left.is_temp() && user.left.temp_id == produced_temp_id)
                            ? &user.right
                            : &user.left;
                    *other = *compare_other_rewrite;
                }
                if (ifx_consumer)
                    user.left.type = byte_type;
                if (byte_store_consumer)
                    user.left.type = byte_type;
            }
            changed = true;
        }

        for (size_t i = 0; i < fn.icodes.size(); ++i) {
            auto &ic = fn.icodes[i];
            if (!ic.result.is_temp())
                continue;

            switch (ic.op) {
            case icode_op::BAND:
            case icode_op::BOR:
            case icode_op::BXOR:
                break;
            case icode_op::SHL:
            case icode_op::SHR:
                if (!(ic.right.kind == operand_kind::INT_CONST &&
                      ic.right.ival >= 0 && ic.right.ival <= 7))
                    continue;
                break;
            default:
                continue;
            }

            const int produced_temp_id = ic.result.temp_id;
            if (temp_use_count[produced_temp_id] <= 1)
                continue;

            auto lhs = unwrap_input(ic.left);
            if (!lhs)
                continue;

            std::optional<operand> rhs;
            if (ic.op == icode_op::SHL || ic.op == icode_op::SHR) {
                rhs = ic.right;
            } else {
                rhs = unwrap_input(ic.right);
                if (!rhs)
                    continue;
            }

            std::vector<std::pair<size_t, operand>> compare_other_rewrites;
            std::vector<size_t> ifx_users;
            bool ok = true;
            bool saw_compare = false;

            for (size_t j = i + 1; j < fn.icodes.size(); ++j) {
                auto &user = fn.icodes[j];
                bool uses_temp = false;
                for_each_use_operand(user, [&](const operand &op) {
                    if (op.is_temp() && op.temp_id == produced_temp_id)
                        uses_temp = true;
                });
                if (!uses_temp)
                    continue;

                if (is_compare_opcode(user.op)) {
                    operand other =
                        (user.left.is_temp() && user.left.temp_id == produced_temp_id)
                            ? user.right
                            : user.left;
                    auto other_byte = unwrap_input(other);
                    if (other_byte) {
                        compare_other_rewrites.push_back(
                            {j, make_byte_const(*other_byte, type::make_uchar())});
                    } else if (other.kind == operand_kind::INT_CONST &&
                               other.ival >= 0 && other.ival <= 0xff) {
                        compare_other_rewrites.push_back(
                            {j, make_byte_const(other, type::make_uchar())});
                    } else {
                        ok = false;
                        break;
                    }
                    saw_compare = true;
                    continue;
                }

                if (user.op == icode_op::IFX &&
                    user.left.is_temp() && user.left.temp_id == produced_temp_id) {
                    ifx_users.push_back(j);
                    continue;
                }

                ok = false;
                break;
            }

            if (!ok || !saw_compare)
                continue;

            type_ptr byte_type = type::make_uchar();
            ic.left = make_byte_const(*lhs, byte_type);
            if (rhs)
                ic.right = make_byte_const(*rhs, byte_type);
            ic.result.type = byte_type;

            for (const auto &[user_idx, other_rewrite] : compare_other_rewrites) {
                auto &user = fn.icodes[user_idx];
                if (user.left.is_temp() && user.left.temp_id == produced_temp_id) {
                    user.left.type = byte_type;
                    user.right = other_rewrite;
                } else {
                    user.right.type = byte_type;
                    user.left = other_rewrite;
                }
            }
            for (size_t user_idx : ifx_users)
                fn.icodes[user_idx].left.type = byte_type;

            changed = true;
        }

        if (!changed)
            return false;

        std::vector<icode> filtered;
        filtered.reserve(fn.icodes.size());
        for (size_t i = 0; i < fn.icodes.size(); ++i) {
            if (!erase[i])
                filtered.push_back(std::move(fn.icodes[i]));
        }
        fn.icodes = std::move(filtered);
        return true;
    }
};

class rotate_combine_pass final : public ir_pass {
public:
    const char *name() const override { return "rotate_combine"; }

    bool run(ir_function &fn) override {
        std::unordered_map<int, const icode *> temp_defs;
        for (const auto &ic : fn.icodes) {
            if (ic.op != icode_op::SET_VALUE_AT &&
                ic.result.is_temp())
                temp_defs[ic.result.temp_id] = &ic;
        }

        auto lookup_shift_def = [&](const operand &op,
                                    icode_op wanted) -> const icode * {
            if (!op.is_temp())
                return nullptr;
            auto it = temp_defs.find(op.temp_id);
            if (it == temp_defs.end() || !it->second)
                return nullptr;
            if (it->second->op != wanted)
                return nullptr;
            if (it->second->right.kind != operand_kind::INT_CONST)
                return nullptr;
            return it->second;
        };

        bool changed = false;
        for (auto &ic : fn.icodes) {
            if (ic.op != icode_op::BOR || !ic.result.type)
                continue;

            const icode *lhs_shl = lookup_shift_def(ic.left, icode_op::SHL);
            const icode *lhs_shr = lookup_shift_def(ic.left, icode_op::SHR);
            const icode *rhs_shl = lookup_shift_def(ic.right, icode_op::SHL);
            const icode *rhs_shr = lookup_shift_def(ic.right, icode_op::SHR);

            const icode *left_shift = nullptr;
            const icode *right_shift = nullptr;
            icode_op rotate_op = icode_op::ROL;
            int rotate_count = 0;

            if (lhs_shl && rhs_shr &&
                same_value_operand(lhs_shl->left, rhs_shr->left)) {
                left_shift = lhs_shl;
                right_shift = rhs_shr;
                rotate_op = icode_op::ROL;
                rotate_count = static_cast<int>(lhs_shl->right.ival);
            } else if (lhs_shr && rhs_shl &&
                       same_value_operand(lhs_shr->left, rhs_shl->left)) {
                left_shift = rhs_shl;
                right_shift = lhs_shr;
                rotate_op = icode_op::ROL;
                rotate_count = static_cast<int>(rhs_shl->right.ival);
            } else {
                continue;
            }

            type_ptr src_type = left_shift->left.type ? left_shift->left.type : ic.result.type;
            if (!src_type || src_type->size() != 2 || !src_type->is_unsigned())
                continue;

            const int left_count = static_cast<int>(left_shift->right.ival) & 31;
            const int right_count = static_cast<int>(right_shift->right.ival) & 31;
            if (left_count <= 0 || right_count <= 0)
                continue;
            if (left_count + right_count != 16)
                continue;

            rotate_count = left_count & 15;
            if (rotate_count == 0)
                continue;
            if (rotate_count > 8) {
                rotate_op = icode_op::ROR;
                rotate_count = 16 - rotate_count;
            }

            ic.op = rotate_op;
            ic.left = left_shift->left;
            ic.right = operand::make_int(rotate_count, ic.right.type ? ic.right.type : left_shift->right.type);
            changed = true;
        }

        return changed;
    }
};

class pack_bytes_pass final : public ir_pass {
public:
    const char *name() const override { return "pack_bytes"; }

    bool run(ir_function &fn) override {
        std::unordered_map<int, const icode *> temp_defs;
        for (const auto &ic : fn.icodes) {
            if (ic.result.is_temp())
                temp_defs[ic.result.temp_id] = &ic;
        }

        std::function<std::optional<operand>(const operand &)> resolve_any_byte_source;
        resolve_any_byte_source = [&](const operand &op) -> std::optional<operand> {
            if (op.kind == operand_kind::INT_CONST) {
                if (op.ival < 0 || op.ival > 0xff)
                    return std::nullopt;
                return op;
            }
            if (op.type && op.type->size() == 1)
                return op;
            if (!op.is_temp())
                return std::nullopt;
            auto it = temp_defs.find(op.temp_id);
            if (it == temp_defs.end() || !it->second)
                return std::nullopt;
            const icode *def = it->second;
            if (def->op == icode_op::ASSIGN)
                return resolve_any_byte_source(def->left);
            if (def->op == icode_op::CAST && def->result.type &&
                def->result.type->size() >= 2)
                return resolve_any_byte_source(def->left);
            return std::nullopt;
        };

        std::function<std::optional<operand>(const operand &)> resolve_zero_extended_byte_source;
        resolve_zero_extended_byte_source = [&](const operand &op) -> std::optional<operand> {
            if (op.kind == operand_kind::INT_CONST) {
                if (op.ival < 0 || op.ival > 0xff)
                    return std::nullopt;
                return op;
            }
            if (op.type && op.type->size() == 1 && op.type->is_unsigned())
                return op;
            if (!op.is_temp())
                return std::nullopt;
            auto it = temp_defs.find(op.temp_id);
            if (it == temp_defs.end() || !it->second)
                return std::nullopt;
            const icode *def = it->second;
            if (def->op == icode_op::ASSIGN)
                return resolve_zero_extended_byte_source(def->left);
            if (def->op == icode_op::CAST && def->result.type &&
                def->result.type->size() >= 2)
                return resolve_zero_extended_byte_source(def->left);
            return std::nullopt;
        };

        auto resolve_high_shift_source = [&](const operand &op) -> std::optional<operand> {
            if (!op.is_temp())
                return std::nullopt;
            auto it = temp_defs.find(op.temp_id);
            if (it == temp_defs.end() || !it->second)
                return std::nullopt;
            const icode *def = it->second;
            if (def->op != icode_op::SHL || def->right.kind != operand_kind::INT_CONST)
                return std::nullopt;
            if ((def->right.ival & 0xff) != 8)
                return std::nullopt;
            if (!def->result.type || def->result.type->size() != 2)
                return std::nullopt;
            return resolve_any_byte_source(def->left);
        };

        bool changed = false;
        for (auto &ic : fn.icodes) {
            if (ic.op != icode_op::BOR || !ic.result.type || ic.result.type->size() != 2)
                continue;

            std::optional<operand> low_src;
            std::optional<operand> high_src;

            if ((low_src = resolve_zero_extended_byte_source(ic.left)) &&
                (high_src = resolve_high_shift_source(ic.right))) {
                ic.op = icode_op::PACK_BYTES;
                ic.left = *low_src;
                ic.right = *high_src;
                changed = true;
                continue;
            }
            if ((low_src = resolve_zero_extended_byte_source(ic.right)) &&
                (high_src = resolve_high_shift_source(ic.left))) {
                ic.op = icode_op::PACK_BYTES;
                ic.left = *low_src;
                ic.right = *high_src;
                changed = true;
                continue;
            }
        }

        return changed;
    }
};

class label_indexed_byte_access_pass final : public ir_pass {
public:
    const char *name() const override { return "label_indexed_byte_access"; }

    bool run(ir_function &fn) override {
        std::unordered_map<int, const icode *> temp_defs;
        for (const auto &ic : fn.icodes) {
            if (ic.result.is_temp())
                temp_defs[ic.result.temp_id] = &ic;
        }

        auto unwrap_index = [&](const operand &op) -> std::optional<operand> {
            if (fits_u8_like_value(op))
                return op;
            if (!op.is_temp())
                return std::nullopt;
            auto it = temp_defs.find(op.temp_id);
            if (it == temp_defs.end() || !it->second)
                return std::nullopt;
            const icode *def = it->second;
            if ((def->op == icode_op::CAST || def->op == icode_op::ASSIGN) &&
                fits_u8_like_value(def->left)) {
                return def->left;
            }
            return std::nullopt;
        };

        std::function<bool(const operand &, operand &, operand &)> match_ptr;
        match_ptr = [&](const operand &ptr,
                        operand &base,
                        operand &index) -> bool {
            if (!ptr.is_temp())
                return false;
            auto it = temp_defs.find(ptr.temp_id);
            if (it == temp_defs.end() || !it->second)
                return false;
            const icode *def = it->second;
            if ((def->op == icode_op::ASSIGN || def->op == icode_op::CAST) &&
                def->left.is_temp()) {
                return match_ptr(def->left, base, index);
            }
            if (def->op != icode_op::ADD)
                return false;

            auto direct_data_base = [](const operand &op) {
                return op.kind == operand_kind::SYMBOL &&
                       op.is_global &&
                       !op.is_tls &&
                       !op.is_sfr &&
                       !op.is_func &&
                       !op.is_param;
            };

            if (def->left.kind == operand_kind::LABEL_REF || direct_data_base(def->left)) {
                auto idx = unwrap_index(def->right);
                if (!idx)
                    return false;
                base = def->left.kind == operand_kind::LABEL_REF
                           ? def->left
                           : operand::make_label(def->left.name);
                index = *idx;
                return true;
            }
            if (def->right.kind == operand_kind::LABEL_REF || direct_data_base(def->right)) {
                auto idx = unwrap_index(def->left);
                if (!idx)
                    return false;
                base = def->right.kind == operand_kind::LABEL_REF
                           ? def->right
                           : operand::make_label(def->right.name);
                index = *idx;
                return true;
            }
            return false;
        };

        bool changed = false;
        for (auto &ic : fn.icodes) {
            operand base;
            operand index;
            const int result_size =
                (ic.result.type && ic.result.type->size() > 0) ? ic.result.type->size() : 2;
            const int left_size =
                (ic.left.type && ic.left.type->size() > 0) ? ic.left.type->size() : 2;
            if (ic.op == icode_op::GET_VALUE_AT &&
                result_size == 1 &&
                match_ptr(ic.left, base, index)) {
                ic.left = base;
                ic.right = index;
                changed = true;
                continue;
            }
            if (ic.op == icode_op::SET_VALUE_AT &&
                left_size == 1 &&
                match_ptr(ic.result, base, index)) {
                ic.result = base;
                ic.right = index;
                changed = true;
                continue;
            }
        }
        return changed;
    }
};

class dead_code_elim_pass final : public ir_pass {
public:
    const char *name() const override { return "dead_code_elim"; }

    bool run(ir_function &fn) override {
        if (fn.icodes.empty()) return false;

        alias_info alias = build_alias_info(fn);
        control_flow_graph cfg(fn);
        auto reachable = cfg.reachable_blocks();
        auto order = cfg.reverse_postorder();

        std::vector<std::unordered_set<std::string>> use(cfg.blocks().size());
        std::vector<std::unordered_set<std::string>> def(cfg.blocks().size());
        std::vector<std::unordered_set<std::string>> live_in(cfg.blocks().size());
        std::vector<std::unordered_set<std::string>> live_out(cfg.blocks().size());

        for (auto &block : cfg.blocks()) {
            if (!reachable.count(block.id)) continue;
            for (size_t i = block.begin; i < block.end; ++i) {
                std::string key = def_key(fn.icodes[i], alias);
                for_each_use_operand(fn.icodes[i], [&](const operand &op) {
                    std::string use_key = trackable_key(op, alias);
                    if (use_key.empty()) return;
                    if (!def[block.id].count(use_key))
                        use[block.id].insert(use_key);
                });
                if (!key.empty()) def[block.id].insert(key);
            }
        }

        bool changed;
        do {
            changed = false;
            for (auto it = order.rbegin(); it != order.rend(); ++it) {
                size_t block_id = *it;
                if (!reachable.count(block_id)) continue;

                std::unordered_set<std::string> new_out;
                for (size_t succ : cfg.block(block_id).succs) {
                    if (!reachable.count(succ)) continue;
                    new_out.insert(live_in[succ].begin(), live_in[succ].end());
                }

                std::unordered_set<std::string> new_in = use[block_id];
                for (auto &value : new_out)
                    if (!def[block_id].count(value)) new_in.insert(value);

                if (new_out != live_out[block_id]) {
                    live_out[block_id] = std::move(new_out);
                    changed = true;
                }
                if (new_in != live_in[block_id]) {
                    live_in[block_id] = std::move(new_in);
                    changed = true;
                }
            }
        } while (changed);

        std::unordered_set<size_t> dead;
        for (auto it = order.rbegin(); it != order.rend(); ++it) {
            size_t block_id = *it;
            if (!reachable.count(block_id)) continue;

            std::unordered_set<std::string> live = live_out[block_id];
            const auto &block = cfg.block(block_id);
            for (size_t i = block.end; i-- > block.begin; ) {
                const icode &ic = fn.icodes[i];
                std::string key = def_key(ic, alias);
                bool removable = !key.empty() &&
                                 is_removable_if_dead(ic.op) &&
                                 !live.count(key);
                if (removable) {
                    dead.insert(i);
                    continue;
                }

                if (!key.empty()) live.erase(key);
                for_each_use_operand(ic, [&](const operand &op) {
                    std::string use_key = trackable_key(op, alias);
                    if (!use_key.empty()) live.insert(use_key);
                });
            }
        }

        if (dead.empty()) return false;
        fn.icodes = rebuild_with_insertions(fn.icodes, dead, {});
        return true;
    }
};

class local_frame_compaction_pass final : public ir_pass {
public:
    const char *name() const override { return "local_frame_compaction"; }

    bool run(ir_function &fn) override {
        struct local_slot_info {
            operand exemplar;
            int extent = 0;
        };

        auto is_stack_local_symbol = [](const operand &op) {
            return op.is_symbol() && !op.is_global && !op.is_param;
        };
        auto observed_extent = [](const operand &op) {
            int size = 1;
            if (op.type && op.type->size() > 0)
                size = op.type->size();
            return op.byte_offset + size;
        };

        std::unordered_map<std::string, local_slot_info> locals;
        std::vector<std::string> local_keys;

        auto note_local = [&](const operand &op) {
            if (!is_stack_local_symbol(op))
                return;

            const std::string key = base_symbol_key(op);
            auto [it, inserted] = locals.emplace(
                key, local_slot_info{op, observed_extent(op)});
            if (inserted) {
                local_keys.push_back(key);
                return;
            }
            it->second.extent = std::max(it->second.extent, observed_extent(op));
            if (op.stack_offset > it->second.exemplar.stack_offset)
                it->second.exemplar = op;
        };

        for (auto &ic : fn.icodes) {
            note_local(ic.result);
            note_local(ic.left);
            note_local(ic.right);
        }

        std::sort(local_keys.begin(), local_keys.end(),
                  [&](const std::string &lhs, const std::string &rhs) {
                      return locals[lhs].exemplar.stack_offset >
                             locals[rhs].exemplar.stack_offset;
                  });

        std::unordered_map<std::string, int> new_offsets;
        int next_offset = 0;
        for (const auto &key : local_keys) {
            const int extent = std::max(1, locals[key].extent);
            next_offset -= extent;
            new_offsets.emplace(key, next_offset);
        }

        bool changed = fn.local_bytes != -next_offset;

        auto remap_local = [&](operand &op) {
            if (!is_stack_local_symbol(op))
                return;
            auto it = new_offsets.find(base_symbol_key(op));
            if (it == new_offsets.end())
                return;
            if (op.stack_offset != it->second) {
                op.stack_offset = it->second;
                changed = true;
            }
        };

        for (auto &ic : fn.icodes) {
            remap_local(ic.result);
            remap_local(ic.left);
            remap_local(ic.right);
            if (ic.op == icode_op::FUNCTION && ic.local_bytes != -next_offset) {
                ic.local_bytes = -next_offset;
                changed = true;
            }
        }

        if (changed)
            fn.local_bytes = -next_offset;
        return changed;
    }
};

} // namespace

std::vector<std::unique_ptr<ir_pass>>
ir_optimizer::build_pipeline(const optimization_settings &settings) {
    std::vector<std::unique_ptr<ir_pass>> passes;
    if (settings.cfg_cleanup)
        passes.push_back(std::make_unique<cfg_cleanup_pass>());
    if (settings.jump_threading)
        passes.push_back(std::make_unique<jump_threading_pass>());
    if (settings.address_deref_fold)
        passes.push_back(std::make_unique<address_deref_fold_pass>());
    if (settings.cfg_cleanup)
        passes.push_back(std::make_unique<global_address_const_pass>());
    if (settings.scalar_local_promotion)
        passes.push_back(std::make_unique<scalar_local_promotion_pass>());
    if (settings.reg_param_promotion)
        passes.push_back(std::make_unique<reg_param_promotion_pass>());
    if (settings.value_propagation)
        passes.push_back(std::make_unique<compare_bool_normalize_pass>());
    if (settings.short_circuit_bool_ifx)
        passes.push_back(std::make_unique<short_circuit_bool_ifx_pass>());
    if (settings.value_propagation)
        passes.push_back(std::make_unique<ifx_bool_wrapper_elide_pass>());
    if (settings.value_propagation)
        passes.push_back(std::make_unique<compare_bool_normalize_pass>());
    if (settings.narrow_counted_byte_loops)
        passes.push_back(std::make_unique<narrow_counted_byte_loops_pass>());
    if (settings.loop_pointer_walk)
        passes.push_back(std::make_unique<loop_pointer_walk_pass>());
    if (settings.promoted_byte_compare)
        passes.push_back(std::make_unique<direct_byte_eq_ne_pass>());
    if (settings.promoted_byte_compare)
        passes.push_back(std::make_unique<promoted_byte_compare_pass>());
    if (settings.promoted_byte_ops)
        passes.push_back(std::make_unique<promoted_byte_ops_pass>());
    if (settings.promoted_byte_ops)
        passes.push_back(std::make_unique<pack_bytes_pass>());
    if (settings.promoted_byte_ops)
        passes.push_back(std::make_unique<label_indexed_byte_access_pass>());
    if (settings.promoted_byte_ops)
        passes.push_back(std::make_unique<available_byte_load_pass>());
    if (settings.rotate_combine)
        passes.push_back(std::make_unique<rotate_combine_pass>());
    if (settings.value_propagation)
        passes.push_back(std::make_unique<value_propagation_pass>());
    if (settings.value_propagation)
        passes.push_back(std::make_unique<calc_temp_fusion_pass>());
    if (settings.value_propagation)
        passes.push_back(std::make_unique<noop_temp_assign_elide_pass>());
    if (settings.constant_folding)
        passes.push_back(std::make_unique<constant_fold_pass>());
    if (settings.algebraic_simplify)
        passes.push_back(std::make_unique<algebraic_simplify_pass>());
    if (settings.local_cse)
        passes.push_back(std::make_unique<local_cse_pass>());
    if (settings.loop_licm)
        passes.push_back(std::make_unique<loop_licm_pass>());
    if (settings.loop_induction)
        passes.push_back(std::make_unique<loop_induction_pass>());
    if (settings.strength_reduction)
        passes.push_back(std::make_unique<strength_reduction_pass>());
    if (settings.dead_code_elim)
        passes.push_back(std::make_unique<dead_code_elim_pass>());
    if (settings.duplicate_block_merge)
        passes.push_back(std::make_unique<duplicate_block_merge_pass>());
    if (settings.merge_tails)
        passes.push_back(std::make_unique<tail_merge_pass>());
    if (settings.local_frame_compaction)
        passes.push_back(std::make_unique<local_frame_compaction_pass>());
    return passes;
}

void ir_optimizer::optimize(ir_function &fn,
                            const optimization_settings &settings) {
    auto passes = build_pipeline(settings);
    for (int iter = 0; iter < 16; ++iter) {
        bool changed = false;
        for (auto &pass : passes)
            changed |= pass->run(fn);
        if (!changed) break;
    }
}

} // namespace xcc
