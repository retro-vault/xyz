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

static int64_t cast_int_value(int64_t v, const type_ptr &type);

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

static int64_t fold_binary(icode_op op, int64_t l, int64_t r,
                           const type_ptr &type) {
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
    const bool use_unsigned = type && (type->is_unsigned() || type->is_ptr());
    const unsigned bits =
        (!type || type->size() <= 0 || type->size() >= 8)
            ? 64U
            : static_cast<unsigned>(type->size() * 8);
    const uint64_t mask =
        bits >= 64 ? ~0ULL : ((1ULL << bits) - 1ULL);
    const uint64_t ul = static_cast<uint64_t>(l) & mask;
    const uint64_t ur = static_cast<uint64_t>(r) & mask;
    const unsigned sh = static_cast<unsigned>(ur) & (bits - 1U);

    auto normalize = [&](int64_t value) -> int64_t {
        return cast_int_value(value, type);
    };

    switch (op) {
    case icode_op::ADD:
        return normalize(static_cast<int64_t>(ul + ur));
    case icode_op::SUB:
        return normalize(static_cast<int64_t>(ul - ur));
    case icode_op::MUL:
        return normalize(static_cast<int64_t>(ul * ur));
    case icode_op::DIV:
        if (r == 0)
            return 0;
        if (use_unsigned)
            return normalize(static_cast<int64_t>(ul / ur));
        if (l == std::numeric_limits<int64_t>::min() && r == -1)
            return normalize(l);
        return normalize(l / r);
    case icode_op::MOD:
        if (r == 0)
            return 0;
        if (use_unsigned)
            return normalize(static_cast<int64_t>(ul % ur));
        if (l == std::numeric_limits<int64_t>::min() && r == -1)
            return 0;
        return normalize(l % r);
    case icode_op::BAND:
        return normalize(static_cast<int64_t>(ul & ur));
    case icode_op::BOR:
        return normalize(static_cast<int64_t>(ul | ur));
    case icode_op::BXOR:
        return normalize(static_cast<int64_t>(ul ^ ur));
    case icode_op::SHL:
        return normalize(static_cast<int64_t>(ul << sh));
    case icode_op::SHR:
        if (use_unsigned)
            return normalize(static_cast<int64_t>(ul >> sh));
        return normalize(l >> sh);
    case icode_op::ROL:
        return normalize(static_cast<int64_t>(rotl(ul, static_cast<unsigned>(ur), bits)));
    case icode_op::ROR:
        return normalize(static_cast<int64_t>(rotr(ul, static_cast<unsigned>(ur), bits)));
    case icode_op::PACK_BYTES:
        return normalize(static_cast<int64_t>((ul & 0xffULL) | ((ur & 0xffULL) << 8)));
    case icode_op::EQ:
        return use_unsigned ? (ul == ur ? 1 : 0) : (l == r ? 1 : 0);
    case icode_op::NE:
        return use_unsigned ? (ul != ur ? 1 : 0) : (l != r ? 1 : 0);
    case icode_op::LT:
        return use_unsigned ? (ul < ur ? 1 : 0) : (l < r ? 1 : 0);
    case icode_op::LE:
        return use_unsigned ? (ul <= ur ? 1 : 0) : (l <= r ? 1 : 0);
    case icode_op::GT:
        return use_unsigned ? (ul > ur ? 1 : 0) : (l > r ? 1 : 0);
    case icode_op::GE:
        return use_unsigned ? (ul >= ur ? 1 : 0) : (l >= r ? 1 : 0);
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
    case 8:
        return static_cast<int64_t>(static_cast<uint64_t>(v));
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

static bool bit_preserving_scalar_copy_type(const type_ptr &dst,
                                            const type_ptr &src) {
    if (!dst || !src)
        return true;

    type_ptr d = dst->unqual();
    type_ptr s = src->unqual();
    if (!d || !s)
        return false;
    if (d->size() <= 0 || d->size() != s->size())
        return false;

    if (d->is_integer() && s->is_integer())
        return true;

    if (d->is_ptr() && s->is_ptr())
        return d->is_far_ptr() == s->is_far_ptr();

    return same_type_shape(d, s);
}

static bool cast_can_fold_to_int_const(const type_ptr &target) {
    if (!target)
        return true;
    return target->is_integer() ||
           target->kind == type_kind::POINTER ||
           target->kind == type_kind::ENUM;
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

static bool same_symbol_slot(const operand &a, const operand &b) {
    return !base_symbol_key(a).empty() &&
           base_symbol_key(a) == base_symbol_key(b);
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
    // SSA-style value propagation is only sound for scalar locals here.
    // Aggregate objects (especially unions / overlapping subobjects) may be
    // partially read or written through derived addresses, so treating the
    // whole base symbol as an interchangeable "value" lets later DCE drop
    // required stores.
    if (!op.type || !op.type->is_scalar()) return false;
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
    auto is_fp_compare_operand = [](const operand &op) {
        return op.type &&
               (op.type->kind == type_kind::FLOAT ||
                op.type->kind == type_kind::DOUBLE);
    };

    if (lhs.tag == ssa_value::kind::INT_CONST &&
        rhs.tag == ssa_value::kind::INT_CONST &&
        is_binary_foldable(ic.op)) {
        const type_ptr fold_type =
            ic.left.type ? ic.left.type : (ic.right.type ? ic.right.type : ic.result.type);
        return ssa_value::int_const(fold_binary(ic.op, lhs.ival, rhs.ival, fold_type));
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
        const bool fp_compare =
            is_fp_compare_operand(ic.left) || is_fp_compare_operand(ic.right);
        switch (ic.op) {
        case icode_op::EQ:
        case icode_op::LE:
        case icode_op::GE:
            if (fp_compare)
                break;
            return ssa_value::int_const(1);
        case icode_op::NE:
        case icode_op::LT:
        case icode_op::GT:
            if (fp_compare) {
                break;
            }
            [[fallthrough]];
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
        if (lhs.tag == ssa_value::kind::INT_CONST &&
            cast_can_fold_to_int_const(ic.result.type))
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
    case icode_op::BLOCK_FILL:
        if (!ic.result.is_none()) fn(ic.result);
        if (!ic.left.is_none())   fn(ic.left);
        if (!ic.right.is_none())  fn(ic.right);
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
        if (!ic.right.is_none())  fn(ic.right);
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
    case icode_op::BLOCK_FILL:
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

    // Above this basic-block count the iterative dominator computation
    // (super-linear in the number of blocks) dominates compile time on
    // pathological machine-generated functions — e.g. the C23 translation
    // limit tests with 127 nested blocks.  Loop optimizations are optional,
    // so for such oversized functions we skip them (report no loops) and
    // fall back to the cheaper, correctness-preserving passes.
    static constexpr size_t kMaxLoopOptBlocks = 400;

    std::vector<natural_loop> natural_loops() const {
        std::map<size_t, natural_loop> by_header;
        if (blocks_.size() > kMaxLoopOptBlocks)
            return {};
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
           std::to_string(static_cast<int>(ic.callee_abi)) + "|" +
           std::to_string(ic.callee_cleans_stack ? 1 : 0) + "|" +
           std::to_string(ic.callee_noreturn ? 1 : 0) + "|" +
           std::to_string(ic.result_via_sret ? 1 : 0);
}

static std::string block_body_signature(const ir_function &fn,
                                        const basic_block &block,
                                        size_t body_end) {
    size_t first = first_non_label_index(fn, block);
    if (first >= body_end)
        return {};

    std::string sig;
    for (size_t i = first; i < body_end; ++i) {
        // Whole-block merging does not rewrite operands, so temp IDs must be
        // exact.  Renaming them here can redirect a branch into a block that
        // reads a different incoming temporary.
        sig += icode_signature(fn.icodes[i]);
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
        lhs.callee_abi != rhs.callee_abi ||
        lhs.callee_cleans_stack != rhs.callee_cleans_stack ||
        lhs.callee_noreturn != rhs.callee_noreturn ||
        lhs.result_via_sret != rhs.result_via_sret) {
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

static bool icode_defines_result_temp(const icode &ic) {
    if (!ic.result.is_temp())
        return false;

    switch (ic.op) {
    case icode_op::LABEL:
    case icode_op::GOTO:
    case icode_op::IFX:
    case icode_op::RETURN:
    case icode_op::SEND:
    case icode_op::SET_VALUE_AT:
    case icode_op::BLOCK_FILL:
    case icode_op::FUNCTION:
    case icode_op::ENDFUNCTION:
    case icode_op::INLINE_ASM:
        return false;
    default:
        return true;
    }
}

static type_ptr temp_type_in_function(const ir_function &fn, int temp_id) {
    for (const auto &ic : fn.icodes) {
        if (ic.result.is_temp() && ic.result.temp_id == temp_id && ic.result.type)
            return ic.result.type;
        if (ic.left.is_temp() && ic.left.temp_id == temp_id && ic.left.type)
            return ic.left.type;
        if (ic.right.is_temp() && ic.right.temp_id == temp_id && ic.right.type)
            return ic.right.type;
    }
    return type::make_int();
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
    auto types_compatible_for_value_rewrite = [](const operand &use,
                                                 const operand &repl) {
        if (!use.type || !repl.type)
            return true;

        if (use.type->size() != repl.type->size())
            return false;

        const bool use_ptr = use.type->is_ptr();
        const bool repl_ptr = repl.type->is_ptr();
        const bool use_far_ptr = use.type->is_far_ptr();
        const bool repl_far_ptr = repl.type->is_far_ptr();
        const bool use_int = use.type->is_integer();
        const bool repl_int = repl.type->is_integer();

        if (use_ptr != repl_ptr || use_far_ptr != repl_far_ptr ||
            use_int != repl_int) {
            return false;
        }

        if (use_int && repl_int &&
            use.type->is_unsigned() != repl.type->is_unsigned()) {
            return false;
        }

        return true;
    };

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
        if (!types_compatible_for_value_rewrite(op, repl))
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
    explicit address_deref_fold_pass(bool canonicalize_fixed_addresses)
        : canonicalize_fixed_addresses_(canonicalize_fixed_addresses) {}

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
                   op.type &&
                   op.type->size() > 0 &&
                   (op.is_global ||
                    (op.type->is_integer() && op.type->size() <= 2));
        };

        std::unordered_map<int, int> temp_def_count;
        for (const auto &ic : fn.icodes) {
            if (defines_result(ic) && ic.result.is_temp())
                ++temp_def_count[ic.result.temp_id];
        }

        std::unordered_map<int, addr_expr> temp_addr;
        bool changed = false;
        for (auto &ic : fn.icodes) {
            switch (ic.op) {
            case icode_op::FUNCTION:
            case icode_op::ENDFUNCTION:
                temp_addr.clear();
                break;
            case icode_op::LABEL:
            case icode_op::GOTO:
            case icode_op::IFX:
            case icode_op::CALL:
            case icode_op::BLOCK_FILL:
            case icode_op::RETURN:
            case icode_op::INLINE_ASM:
                // The address of an ordinary fixed object is invariant across
                // control-flow boundaries.  The fold below never propagates
                // object contents, only its compile-time base and a bounded
                // byte offset.  Direct aggregate byte offsets are currently
                // proven in the backend only for globals; retain the original
                // small-integer restriction for IX-relative local objects.
                for (auto it = temp_addr.begin(); it != temp_addr.end();) {
                    auto count = temp_def_count.find(it->first);
                    const bool fixed_base =
                        is_foldable_base(it->second.base);
                    if (count == temp_def_count.end() || count->second != 1 ||
                        !fixed_base) {
                        it = temp_addr.erase(it);
                    } else {
                        ++it;
                    }
                }
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
                        if (canonicalize_fixed_addresses_) {
                            const auto &address =
                                temp_addr.at(ic.result.temp_id);
                            operand direct = address.base;
                            direct.byte_offset += address.byte_offset;
                            ic.op = icode_op::ADDRESS_OF;
                            ic.left = std::move(direct);
                            ic.right = operand::make_none();
                            changed = true;
                        }
                        continue;
                    }
                }

                if (ic.op == icode_op::SUB) {
                    if (propagate_add(ic.left, ic.right, -1)) {
                        if (canonicalize_fixed_addresses_) {
                            const auto &address =
                                temp_addr.at(ic.result.temp_id);
                            operand direct = address.base;
                            direct.byte_offset += address.byte_offset;
                            ic.op = icode_op::ADDRESS_OF;
                            ic.left = std::move(direct);
                            ic.right = operand::make_none();
                            changed = true;
                        }
                        continue;
                    }
                }
            }
        }

        return changed;
    }

private:
    bool canonicalize_fixed_addresses_ = false;
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

class repeated_compare_edge_fold_pass final : public ir_pass {
public:
    const char *name() const override { return "repeated_compare_edge_fold"; }

    bool run(ir_function &fn) override {
        if (fn.icodes.empty())
            return false;

        control_flow_graph cfg(fn);

        auto compare_before_ifx = [&](const basic_block &block,
                                      const icode *&compare,
                                      const icode *&branch) {
            compare = nullptr;
            branch = nullptr;
            if (block.end < block.begin + 2)
                return false;
            const icode &term = fn.icodes[block.end - 1];
            const icode &cmp = fn.icodes[block.end - 2];
            if (term.op != icode_op::IFX || !term.left.is_temp() ||
                !is_compare_opcode(cmp.op) || !cmp.result.is_temp() ||
                cmp.result.temp_id != term.left.temp_id) {
                return false;
            }
            auto volatile_operand = [](const operand &op) {
                return op.type && op.type->is_volatile;
            };
            if (volatile_operand(cmp.left) || volatile_operand(cmp.right))
                return false;
            compare = &cmp;
            branch = &term;
            return true;
        };

        auto same_compare = [](const icode &lhs, const icode &rhs) {
            return lhs.op == rhs.op &&
                   same_value_operand(lhs.left, rhs.left) &&
                   same_value_operand(lhs.right, rhs.right);
        };

        auto operand_preserved_until = [&](const operand &value,
                                           size_t begin,
                                           size_t end) {
            for (size_t i = begin; i < end; ++i) {
                const icode &ic = fn.icodes[i];
                if (value.is_temp()) {
                    if (defines_result(ic) && ic.result.is_temp() &&
                        ic.result.temp_id == value.temp_id)
                        return false;
                    continue;
                }
                if (!value.is_symbol())
                    continue;

                if (ic.result.is_symbol() &&
                    same_symbol_slot(ic.result, value)) {
                    return false;
                }
                // A call or indirect/opaque memory operation may change a
                // symbol even when the IR does not name that symbol directly.
                if (ic.op == icode_op::CALL ||
                    ic.op == icode_op::SET_VALUE_AT ||
                    ic.op == icode_op::INLINE_ASM ||
                    ic.op == icode_op::ALLOCA) {
                    return false;
                }
            }
            return true;
        };

        for (const basic_block &block : cfg.blocks()) {
            const icode *first_cmp = nullptr;
            const icode *first_branch = nullptr;
            if (!compare_before_ifx(block, first_cmp, first_branch))
                continue;

            struct edge_fact {
                std::string label;
                bool value = false;
            };
            const edge_fact edges[] = {
                {first_branch->true_lbl, true},
                {first_branch->false_lbl, false},
            };
            for (const edge_fact &edge : edges) {
                if (edge.label.empty())
                    continue;
                auto successor_id = cfg.block_for_label(edge.label);
                if (!successor_id)
                    continue;
                const basic_block &successor = cfg.block(*successor_id);
                if (successor.preds.size() != 1 ||
                    successor.preds.front() != block.id) {
                    continue;
                }

                const icode *second_cmp = nullptr;
                const icode *second_branch = nullptr;
                if (!compare_before_ifx(successor, second_cmp, second_branch) ||
                    !same_compare(*first_cmp, *second_cmp)) {
                    continue;
                }
                const size_t second_cmp_idx = successor.end - 2;
                if (!operand_preserved_until(first_cmp->left,
                                             successor.begin,
                                             second_cmp_idx) ||
                    !operand_preserved_until(first_cmp->right,
                                             successor.begin,
                                             second_cmp_idx)) {
                    continue;
                }

                const std::string &target =
                    edge.value ? second_branch->true_lbl
                               : second_branch->false_lbl;
                if (target.empty())
                    continue;

                icode &folded = fn.icodes[successor.end - 1];
                folded.op = icode_op::GOTO;
                folded.label_name = target;
                folded.true_lbl.clear();
                folded.false_lbl.clear();
                folded.left = operand::make_none();
                folded.right = operand::make_none();
                return true;
            }
        }
        return false;
    }
};

class global_address_const_pass final : public ir_pass {
public:
    const char *name() const override { return "global_address_const"; }

    bool run(ir_function &fn) override {
        std::unordered_map<int, operand> global_addr_defs;
        std::unordered_map<int, int> temp_def_counts;

        for (const auto &ic : fn.icodes) {
            if (defines_result(ic) && ic.result.is_temp())
                ++temp_def_counts[ic.result.temp_id];
        }

        for (const auto &ic : fn.icodes) {
            if (ic.op != icode_op::ADDRESS_OF || !ic.result.is_temp())
                continue;
            auto count = temp_def_counts.find(ic.result.temp_id);
            if (count == temp_def_counts.end() || count->second != 1)
                continue;
            if (!ic.left.is_global || ic.left.is_tls || ic.left.is_sfr)
                continue;
            // A bare label reference represents the object's base.  Keep an
            // offset ADDRESS_OF explicit until label-reference offsets are
            // represented by this pass; otherwise `&object[n]` silently
            // collapses to `&object[0]`.
            if (ic.left.byte_offset != 0)
                continue;

            operand ref = operand::make_label(ic.left.name);
            ref.type = ic.result.type;
            global_addr_defs[ic.result.temp_id] = ref;
        }

        // A single loop-invariant address temp is often cheaper to keep once
        // than to rematerialize at every use.  Rematerialization pays when a
        // function hoists several global/label addresses and those temps
        // inflate the backend spill frame.
        if (global_addr_defs.size() < 4)
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
    explicit scalar_local_promotion_pass(bool terminal_backend_form = false)
        : terminal_backend_form_(terminal_backend_form) {}

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
        std::unordered_map<std::string, int> definition_count;
        const bool function_has_call = std::any_of(
            fn.icodes.begin(), fn.icodes.end(),
            [](const icode &ic) { return ic.op == icode_op::CALL; });
        std::unordered_map<std::string, int> dispatch_comparisons;
        for (size_t i = 0; i + 1 < fn.icodes.size(); ++i) {
            const icode &compare = fn.icodes[i];
            const icode &branch = fn.icodes[i + 1];
            if ((compare.op != icode_op::EQ && compare.op != icode_op::NE) ||
                !compare.result.is_temp() || branch.op != icode_op::IFX ||
                !branch.left.is_temp() ||
                branch.left.temp_id != compare.result.temp_id) {
                continue;
            }
            const operand *selector = nullptr;
            if (compare.left.kind == operand_kind::INT_CONST)
                selector = &compare.right;
            else if (compare.right.kind == operand_kind::INT_CONST)
                selector = &compare.left;
            if (!selector)
                continue;
            if (selector->is_temp()) {
                ++dispatch_comparisons["T:" +
                                       std::to_string(selector->temp_id)];
            } else if (selector->is_symbol()) {
                ++dispatch_comparisons["S:" + base_symbol_key(*selector)];
            }
        }
        const bool function_has_dense_dispatch = std::any_of(
            dispatch_comparisons.begin(), dispatch_comparisons.end(),
            [](const auto &entry) { return entry.second >= 3; });
        auto promotable_local_type = [](const type_ptr &type) {
            if (!type)
                return false;
            if (type->is_array() || type->is_func() ||
                type->kind == type_kind::STRUCT ||
                type->kind == type_kind::UNION)
                return false;
            return type->size() == 1 || type->size() == 2 ||
                   type->size() == 4;
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

            // A plain TEMP is not an SSA value in the current backend. Signed
            // locals with multiple static definitions are particularly unsafe
            // to promote: data-dependent decrement loops (insertion sort's j)
            // can be conflated with values from an outer backedge. Unsigned
            // counters and pointers deliberately remain eligible; the
            // bounded/countdown/pointer passes consume precisely those
            // canonical loop forms.
            if (defines_result(ic) && ic.result.is_symbol() &&
                !ic.result.is_global && !ic.result.is_param &&
                !ic.result.is_tls && !ic.result.is_sfr &&
                !ic.result.is_func) {
                const std::string key = base_symbol_key(ic.result);
                if (!key.empty())
                    ++definition_count[key];
            }

            block_if_needed(ic.result);
            block_if_needed(ic.left);
            block_if_needed(ic.right);
        }

        auto is_control_only_counted_local = [&](const std::string &key) {
            bool saw_init = false;
            bool saw_compare = false;
            bool saw_update = false;
            auto is_key = [&](const operand &op) {
                return op.is_symbol() && base_symbol_key(op) == key;
            };
            for (const auto &ic : fn.icodes) {
                const bool in_result = is_key(ic.result);
                const bool in_left = is_key(ic.left);
                const bool in_right = is_key(ic.right);
                if (!in_result && !in_left && !in_right)
                    continue;

                if (ic.op == icode_op::ASSIGN && in_result && !in_left &&
                    !in_right && ic.left.kind == operand_kind::INT_CONST) {
                    saw_init = true;
                    continue;
                }
                if (ic.op == icode_op::ADD && in_result && in_left &&
                    !in_right && ic.right.kind == operand_kind::INT_CONST &&
                    ic.right.ival == 1) {
                    saw_update = true;
                    continue;
                }
                if (ic.op == icode_op::LT && !in_result && in_left &&
                    !in_right && ic.right.kind == operand_kind::INT_CONST &&
                    ic.right.ival > 1 && ic.right.ival <= 65535 &&
                    (!function_has_call || ic.right.ival > 255)) {
                    saw_compare = true;
                    continue;
                }
                return false;
            }
            return saw_init && saw_compare && saw_update;
        };

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
            const bool dense_dispatch_terminal =
                terminal_backend_form_ && function_has_dense_dispatch;
            if (key.empty() || blocked.count(key) ||
                (op.type->size() == 4 && definition_count[key] != 1) ||
                (definition_count[key] != 1 && !op.type->is_ptr() &&
                 (!op.type->is_integer() ||
                  (!op.type->is_unsigned() &&
                   !is_control_only_counted_local(key) &&
                   !dense_dispatch_terminal))))
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

private:
    bool terminal_backend_form_ = false;
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

// Immutable stack-passed byte pointers are safe SSA-like values.  Normalize
// them before loop lowering so ABI0 receives can use the same pointer-walk and
// frameless leaf paths as register-passed ABI1 receives.
class immutable_stack_pointer_param_promotion_pass final : public ir_pass {
public:
    const char *name() const override {
        return "immutable_stack_pointer_param_promotion";
    }

    bool run(ir_function &fn) override {
        if (effective_call_abi(fn.abi) != call_abi::SDCCCALL0)
            return false;

        std::unordered_map<std::string, size_t> label_indices;
        bool has_backedge = false;
        for (size_t i = 0; i < fn.icodes.size(); ++i) {
            const auto &ic = fn.icodes[i];
            if (ic.op == icode_op::CALL ||
                ic.op == icode_op::INLINE_ASM ||
                ic.op == icode_op::ALLOCA) {
                return false;
            }
            if (ic.op == icode_op::LABEL)
                label_indices[ic.label_name] = i;
        }
        for (size_t i = 0; i < fn.icodes.size(); ++i) {
            const auto &ic = fn.icodes[i];
            auto backward = [&](const std::string &label) {
                auto it = label_indices.find(label);
                return it != label_indices.end() && it->second < i;
            };
            if ((ic.op == icode_op::GOTO && backward(ic.label_name)) ||
                (ic.op == icode_op::IFX &&
                 (backward(ic.true_lbl) || backward(ic.false_lbl)))) {
                has_backedge = true;
                break;
            }
        }
        if (!has_backedge)
            return false;

        alias_info alias = build_alias_info(fn);
        struct candidate {
            operand param;
            operand temp;
        };
        std::map<std::string, candidate> promoted;
        std::vector<std::string> promotion_order;
        int next_temp = next_temp_id(fn);

        for (const auto &receive : fn.icodes) {
            if (receive.op != icode_op::RECEIVE ||
                receive.arg_loc != abi_arg_loc::STACK ||
                !receive.result.is_symbol() ||
                !receive.result.is_param ||
                receive.result.is_global ||
                receive.result.byte_offset != 0 ||
                !receive.result.type ||
                !receive.result.type->is_ptr() ||
                receive.result.type->is_far_ptr() ||
                receive.result.type->is_volatile ||
                !receive.result.type->base ||
                receive.result.type->base->size() != 1 ||
                receive.result.type->base->is_volatile ||
                base_symbol_address_taken(alias, receive.result)) {
                continue;
            }

            const std::string key = base_symbol_key(receive.result);
            int uses = 0;
            int definitions = 0;
            bool only_private_init = true;
            for (const auto &ic : fn.icodes) {
                const bool in_result =
                    ic.result.is_symbol() &&
                    base_symbol_key(ic.result) == key;
                const bool in_left =
                    ic.left.is_symbol() &&
                    base_symbol_key(ic.left) == key;
                const bool in_right =
                    ic.right.is_symbol() &&
                    base_symbol_key(ic.right) == key;
                if (in_result && defines_result(ic) &&
                    ic.op != icode_op::RECEIVE) {
                    ++definitions;
                }
                uses += static_cast<int>(in_left) +
                        static_cast<int>(in_right);
                const bool private_init =
                    ic.op == icode_op::ASSIGN && in_left && !in_right &&
                    ic.result.is_temp();
                if ((in_left || in_right) && !private_init)
                    only_private_init = false;
            }
            if (definitions != 0 || uses == 0 || only_private_init)
                continue;

            auto [it, inserted] = promoted.emplace(
                key, candidate{receive.result,
                               operand::make_temp(next_temp++,
                                                  receive.result.type)});
            if (inserted)
                promotion_order.push_back(it->first);
        }

        if (promoted.empty())
            return false;

        auto remap = [&](operand &op) {
            if (!op.is_symbol())
                return;
            auto it = promoted.find(base_symbol_key(op));
            if (it == promoted.end())
                return;
            operand replacement = it->second.temp;
            replacement.type = op.type ? op.type : replacement.type;
            replacement.byte_offset = op.byte_offset;
            op = std::move(replacement);
        };
        for (auto &ic : fn.icodes) {
            if (ic.op == icode_op::RECEIVE)
                continue;
            remap(ic.result);
            remap(ic.left);
            remap(ic.right);
        }

        size_t insert_at = 1;
        while (insert_at < fn.icodes.size() &&
               fn.icodes[insert_at].op == icode_op::RECEIVE) {
            ++insert_at;
        }
        std::map<size_t, std::vector<icode>> insertion;
        for (const auto &key : promotion_order) {
            const auto &candidate = promoted.at(key);
            icode initialize;
            initialize.op = icode_op::ASSIGN;
            initialize.result = candidate.temp;
            initialize.left = candidate.param;
            insertion[insert_at].push_back(std::move(initialize));
        }
        fn.icodes = rebuild_with_insertions(fn.icodes, {}, insertion);
        return true;
    }
};

// Stack-linkage parameters are caller-owned memory slots, but an unaliased
// pointer parameter still has ordinary local-value semantics.  Give one hot
// byte-pointer parameter a private backend value so the target allocator can
// retain a loop cursor in a register instead of updating the caller's stack
// slot on every iteration.  This is terminal because the promoted value may
// have multiple control-flow definitions.
class stack_param_promotion_pass final : public ir_pass {
public:
    const char *name() const override {
        return "stack_param_promotion";
    }

    bool run(ir_function &fn) override {
        if (effective_call_abi(fn.abi) != call_abi::SDCCCALL0)
            return false;

        for (const auto &ic : fn.icodes) {
            if (ic.op == icode_op::INLINE_ASM)
                return false;
        }

        alias_info alias = build_alias_info(fn);
        struct candidate {
            operand param;
            std::string key;
            int score = 0;
        };
        std::optional<candidate> best;

        for (const auto &receive : fn.icodes) {
            if (receive.op != icode_op::RECEIVE ||
                receive.arg_loc != abi_arg_loc::STACK ||
                !receive.result.is_symbol() ||
                !receive.result.is_param ||
                receive.result.is_global ||
                receive.result.byte_offset != 0 ||
                !receive.result.type ||
                receive.result.type->is_volatile ||
                base_symbol_address_taken(alias, receive.result)) {
                continue;
            }
            const bool near_byte_pointer =
                receive.result.type->is_ptr() &&
                !receive.result.type->is_far_ptr() &&
                receive.result.type->base &&
                receive.result.type->base->size() == 1 &&
                !receive.result.type->base->is_volatile;
            if (!near_byte_pointer)
                continue;

            const std::string key = base_symbol_key(receive.result);
            int mentions = 0;
            int definitions = 0;
            int dereferences = 0;
            for (const auto &ic : fn.icodes) {
                const bool in_result =
                    ic.result.is_symbol() &&
                    base_symbol_key(ic.result) == key;
                const bool in_left =
                    ic.left.is_symbol() &&
                    base_symbol_key(ic.left) == key;
                const bool in_right =
                    ic.right.is_symbol() &&
                    base_symbol_key(ic.right) == key;
                mentions += static_cast<int>(in_result) +
                            static_cast<int>(in_left) +
                            static_cast<int>(in_right);
                if (in_result && defines_result(ic) &&
                    ic.op != icode_op::RECEIVE) {
                    ++definitions;
                }
                if ((ic.op == icode_op::GET_VALUE_AT && in_left) ||
                    (ic.op == icode_op::SET_VALUE_AT && in_result)) {
                    ++dereferences;
                }
            }

            // A post-increment cursor is normally copied to a temporary
            // before the dereference, so textual direct dereferences may be
            // zero.  Multiple mentions plus a write still provide a strict
            // profitability floor; immutable one-use arguments stay in place.
            if (mentions < 4 || definitions == 0)
                continue;
            const int score = mentions * 8 + definitions * 12 +
                              dereferences * 16;
            if (!best || score > best->score)
                best = candidate{receive.result, key, score};
        }

        if (!best)
            return false;

        int next_temp = next_temp_id(fn);
        operand promoted =
            operand::make_temp(next_temp++, best->param.type);
        auto remap = [&](operand &op) {
            if (!op.is_symbol() || base_symbol_key(op) != best->key)
                return;
            operand replacement = promoted;
            replacement.type = op.type ? op.type : replacement.type;
            replacement.byte_offset = op.byte_offset;
            op = std::move(replacement);
        };

        std::map<size_t, std::vector<icode>> insertion;
        for (size_t i = 0; i < fn.icodes.size(); ++i) {
            auto &ic = fn.icodes[i];
            if (ic.op == icode_op::RECEIVE)
                continue;
            const bool split_in_place_update =
                (ic.op == icode_op::ADD || ic.op == icode_op::SUB) &&
                ic.result.is_symbol() && ic.left.is_symbol() &&
                base_symbol_key(ic.result) == best->key &&
                base_symbol_key(ic.left) == best->key;
            remap(ic.result);
            remap(ic.left);
            remap(ic.right);
            if (split_in_place_update) {
                operand step =
                    operand::make_temp(next_temp++, promoted.type);
                ic.result = step;
                icode commit;
                commit.op = icode_op::ASSIGN;
                commit.result = promoted;
                commit.left = step;
                commit.line = ic.line;
                insertion[i + 1].push_back(std::move(commit));
            }
        }

        size_t insert_at = 1;
        while (insert_at < fn.icodes.size() &&
               fn.icodes[insert_at].op == icode_op::RECEIVE) {
            ++insert_at;
        }
        icode initialize;
        initialize.op = icode_op::ASSIGN;
        initialize.result = promoted;
        initialize.left = best->param;
        insertion[insert_at].push_back(std::move(initialize));
        fn.icodes = rebuild_with_insertions(fn.icodes, {}, insertion);
        return true;
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
        auto truthy_type = [](const type_ptr &type) {
            return type &&
                   !type->is_far_ptr() &&
                   (type->is_integer() || type->is_ptr());
        };
        auto truthy_operand = [&](const operand &op) {
            return truthy_type(op.type);
        };
        auto boolean_operand = [](const operand &op) {
            return op.type && op.type->kind == type_kind::BOOL;
        };
        auto truth_preserving_cast = [&](const icode &ic) {
            return ic.op == icode_op::CAST &&
                   truthy_type(ic.left.type) &&
                   truthy_type(ic.result.type) &&
                   ic.left.type->size() > 0 &&
                   ic.result.type->size() >= ic.left.type->size();
        };
        auto swapped_compare = [](icode_op op) {
            switch (op) {
            case icode_op::LT: return icode_op::GT;
            case icode_op::LE: return icode_op::GE;
            case icode_op::GT: return icode_op::LT;
            case icode_op::GE: return icode_op::LE;
            default: return op;
            }
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

                if (truth_preserving_cast(def)) {
                    if (def.left.is_temp()) {
                        auto nested =
                            resolve_truth_preserving_source(def.left, visiting);
                        if (nested) {
                            operand repl = *nested;
                            repl.type = def.result.type ? def.result.type : repl.type;
                            return repl;
                        }
                    }
                    if (boolean_operand(def.left)) {
                        operand repl = def.left;
                        repl.type = def.result.type ? def.result.type : repl.type;
                        return repl;
                    }
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

                // `x == 1` is equivalent to the truth value of x only when x
                // is proven Boolean.  Merely having an integer/pointer type
                // is insufficient: rewriting an opcode test such as
                // `node == BOL` into `if (node)` changes every nonzero opcode
                // into BOL.  Non-Boolean `x != 0` forms remain available to
                // later IFX lowering without claiming that x itself is a
                // stored 0/1 value.
                if (boolean_operand(op)) {
                    operand repl = op;
                    repl.type = def.result.type ? def.result.type : repl.type;
                    return repl;
                }

                return std::nullopt;
            };

        auto identity_source = [&](const icode &ic) -> std::optional<operand> {
            const operand *source = nullptr;
            icode_op effective_op = ic.op;
            const operand *constant = nullptr;
            if (ic.left.is_temp() && ic.right.kind == operand_kind::INT_CONST) {
                source = &ic.left;
                constant = &ic.right;
            } else if (ic.right.is_temp() && ic.left.kind == operand_kind::INT_CONST) {
                source = &ic.right;
                constant = &ic.left;
                effective_op = swapped_compare(ic.op);
            }
            bool identity = false;
            if (source && constant) {
                identity =
                    (effective_op == icode_op::NE && zero_const(*constant)) ||
                    (effective_op == icode_op::EQ && one_const(*constant));
                if (!identity &&
                    truthy_operand(*source) &&
                    (source->type->is_unsigned() || source->type->is_ptr())) {
                    identity =
                        (effective_op == icode_op::GT && zero_const(*constant)) ||
                        (effective_op == icode_op::GE && one_const(*constant));
                }
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

class bounded_word_counter_narrow_pass final : public ir_pass {
public:
    const char *name() const override { return "bounded_word_counter_narrow"; }

    bool run(ir_function &fn) override {
        if (fn.icodes.empty())
            return false;

        control_flow_graph cfg(fn);
        if (cfg.blocks().empty())
            return false;

        std::unordered_set<int> candidates;
        for (const auto &ic : fn.icodes) {
            auto note = [&](const operand &op) {
                if (op.is_temp() && op.type && op.type->size() == 2 &&
                    op.type->is_integer() && op.type->is_unsigned()) {
                    candidates.insert(op.temp_id);
                }
            };
            note(ic.result);
            note(ic.left);
            note(ic.right);
        }

        type_ptr byte_type = type::make_uchar();
        for (int tid : candidates) {
            std::unordered_set<int> step_temps;
            std::unordered_set<int> committed_steps;
            bool saw_init = false;
            bool saw_guard = false;
            bool saw_increment = false;
            bool uses_safe = true;

            auto is_candidate = [&](const operand &op) {
                return op.is_temp() && op.temp_id == tid;
            };
            auto mentions_candidate = [&](const icode &ic) {
                return is_candidate(ic.result) || is_candidate(ic.left) ||
                       is_candidate(ic.right);
            };

            for (const auto &ic : fn.icodes) {
                if (!mentions_candidate(ic))
                    continue;

                if (ic.op == icode_op::ASSIGN && is_candidate(ic.result)) {
                    if (ic.left.kind == operand_kind::INT_CONST &&
                        ic.left.ival >= 0 && ic.left.ival <= 255) {
                        saw_init = true;
                        continue;
                    }
                    if (ic.left.is_temp()) {
                        committed_steps.insert(ic.left.temp_id);
                        continue;
                    }
                    uses_safe = false;
                    break;
                }

                if (ic.op == icode_op::ADD && ic.result.is_temp() &&
                    is_candidate(ic.left) &&
                    ic.right.kind == operand_kind::INT_CONST &&
                    ic.right.ival == 1) {
                    step_temps.insert(ic.result.temp_id);
                    saw_increment = true;
                    continue;
                }

                if (ic.op == icode_op::LT && is_candidate(ic.left) &&
                    ic.right.kind == operand_kind::INT_CONST &&
                    ic.right.ival > 0 && ic.right.ival <= 255) {
                    saw_guard = true;
                    continue;
                }

                if (ic.op == icode_op::CAST && is_candidate(ic.left) &&
                    ic.result.type && ic.result.type->size() == 1) {
                    continue;
                }

                uses_safe = false;
                break;
            }
            if (!uses_safe || !saw_init || !saw_guard || !saw_increment ||
                step_temps.empty() || committed_steps.empty()) {
                continue;
            }
            for (int committed : committed_steps) {
                if (!step_temps.count(committed)) {
                    uses_safe = false;
                    break;
                }
            }
            if (!uses_safe)
                continue;

            for (const auto &ic : fn.icodes) {
                for (int step_tid : step_temps) {
                    const bool mentions_step =
                        (ic.result.is_temp() &&
                         ic.result.temp_id == step_tid) ||
                        (ic.left.is_temp() && ic.left.temp_id == step_tid) ||
                        (ic.right.is_temp() && ic.right.temp_id == step_tid);
                    if (!mentions_step)
                        continue;

                    const bool defines_step =
                        ic.op == icode_op::ADD && ic.result.is_temp() &&
                        ic.result.temp_id == step_tid &&
                        is_candidate(ic.left) &&
                        ic.right.kind == operand_kind::INT_CONST &&
                        ic.right.ival == 1;
                    const bool commits_step =
                        ic.op == icode_op::ASSIGN &&
                        is_candidate(ic.result) && ic.left.is_temp() &&
                        ic.left.temp_id == step_tid;
                    if (!defines_step && !commits_step) {
                        uses_safe = false;
                        break;
                    }
                }
                if (!uses_safe)
                    break;
            }
            if (!uses_safe)
                continue;

            // Explore both abstract states at joins. `true` means the current
            // value is proven <= 254, so one byte increment cannot wrap.
            struct work_state {
                size_t block = 0;
                bool increment_safe = false;
            };
            std::vector<work_state> work{{0, false}};
            std::unordered_set<size_t> visited;
            bool range_safe = true;

            while (!work.empty() && range_safe) {
                const work_state state = work.back();
                work.pop_back();
                const size_t key = state.block * 2 +
                                   (state.increment_safe ? 1 : 0);
                if (!visited.insert(key).second)
                    continue;

                const auto &block = cfg.block(state.block);
                bool safe = state.increment_safe;
                for (size_t i = block.begin; i < block.end; ++i) {
                    const icode &ic = fn.icodes[i];
                    if (ic.op == icode_op::ASSIGN &&
                        is_candidate(ic.result) &&
                        ic.left.kind == operand_kind::INT_CONST) {
                        safe = ic.left.ival >= 0 && ic.left.ival < 255;
                        continue;
                    }
                    if (ic.op == icode_op::ADD &&
                        is_candidate(ic.left) && ic.result.is_temp() &&
                        step_temps.count(ic.result.temp_id)) {
                        if (!safe) {
                            range_safe = false;
                            break;
                        }
                        safe = false;
                    }
                }
                if (!range_safe)
                    break;

                const icode *term =
                    block.begin < block.end ? &fn.icodes[block.end - 1]
                                            : nullptr;
                const icode *guard = nullptr;
                if (term && term->op == icode_op::IFX &&
                    term->left.is_temp()) {
                    for (size_t i = block.end - 1; i-- > block.begin;) {
                        const icode &cand = fn.icodes[i];
                        if (!cand.result.is_temp() ||
                            cand.result.temp_id != term->left.temp_id) {
                            continue;
                        }
                        if (cand.op == icode_op::LT &&
                            is_candidate(cand.left) &&
                            cand.right.kind == operand_kind::INT_CONST &&
                            cand.right.ival > 0 && cand.right.ival <= 255) {
                            guard = &cand;
                        }
                        break;
                    }
                }

                std::optional<size_t> true_block;
                if (guard && term)
                    true_block = cfg.block_for_label(term->true_lbl);
                for (size_t succ : block.succs) {
                    bool edge_safe = safe;
                    if (guard)
                        edge_safe = true_block && succ == *true_block;
                    work.push_back({succ, edge_safe});
                }
            }
            if (!range_safe)
                continue;

            auto retag = [&](operand &op) {
                if ((op.is_temp() && op.temp_id == tid) ||
                    (op.is_temp() && step_temps.count(op.temp_id))) {
                    op.type = byte_type;
                }
            };
            for (auto &ic : fn.icodes) {
                retag(ic.result);
                retag(ic.left);
                retag(ic.right);
                if (ic.op == icode_op::ASSIGN && is_candidate(ic.result) &&
                    ic.left.kind == operand_kind::INT_CONST) {
                    ic.left.type = byte_type;
                }
                if (ic.op == icode_op::LT && is_candidate(ic.left) &&
                    ic.right.kind == operand_kind::INT_CONST) {
                    ic.right.type = byte_type;
                }
            }
            return true;
        }

        return false;
    }
};

class boolean_phi_ifx_fold_pass final : public ir_pass {
public:
    const char *name() const override { return "boolean_phi_ifx_fold"; }

    bool run(ir_function &fn) override {
        if (fn.icodes.empty())
            return false;
        control_flow_graph cfg(fn);

        for (const auto &join : cfg.blocks()) {
            const size_t first = first_non_label_index(fn, join);
            if (first >= join.end || first + 1 != join.end)
                continue;
            const icode &ifx = fn.icodes[first];
            if (ifx.op != icode_op::IFX || !ifx.left.is_temp() ||
                ifx.true_lbl.empty() || ifx.false_lbl.empty() ||
                join.preds.size() < 2) {
                continue;
            }
            const int value_tid = ifx.left.temp_id;

            struct incoming_def {
                size_t index = 0;
                bool truth = false;
                size_t block_end = 0;
            };
            std::vector<incoming_def> incoming;
            bool safe = true;
            for (size_t pred_id : join.preds) {
                const auto &pred = cfg.block(pred_id);
                size_t def_idx = fn.icodes.size();
                bool truth = false;
                for (size_t i = pred.begin; i < pred.end; ++i) {
                    const icode &ic = fn.icodes[i];
                    if (defines_result(ic) && ic.result.is_temp() &&
                        ic.result.temp_id == value_tid) {
                        if (def_idx != fn.icodes.size() ||
                            ic.op != icode_op::ASSIGN ||
                            ic.left.kind != operand_kind::INT_CONST ||
                            (ic.left.ival != 0 && ic.left.ival != 1)) {
                            safe = false;
                            break;
                        }
                        def_idx = i;
                        truth = ic.left.ival != 0;
                    }
                }
                if (!safe || def_idx == fn.icodes.size()) {
                    safe = false;
                    break;
                }
                for (size_t i = def_idx + 1; i < pred.end; ++i) {
                    if (fn.icodes[i].op != icode_op::LABEL &&
                        fn.icodes[i].op != icode_op::GOTO) {
                        safe = false;
                        break;
                    }
                }
                if (!safe)
                    break;
                incoming.push_back({def_idx, truth, pred.end});
            }
            if (!safe)
                continue;

            size_t definitions = 0;
            size_t uses = 0;
            for (size_t i = 0; i < fn.icodes.size(); ++i) {
                const icode &ic = fn.icodes[i];
                if (defines_result(ic) && ic.result.is_temp() &&
                    ic.result.temp_id == value_tid) {
                    ++definitions;
                }
                for_each_use_operand(ic, [&](const operand &op) {
                    if (op.is_temp() && op.temp_id == value_tid)
                        ++uses;
                });
            }
            if (definitions != incoming.size() || uses != 1)
                continue;

            std::unordered_set<size_t> erase;
            for (const incoming_def &edge : incoming) {
                icode &def = fn.icodes[edge.index];
                def.op = icode_op::GOTO;
                def.label_name = edge.truth ? ifx.true_lbl : ifx.false_lbl;
                def.true_lbl.clear();
                def.false_lbl.clear();
                def.result = operand::make_none();
                def.left = operand::make_none();
                def.right = operand::make_none();
                for (size_t i = edge.index + 1; i < edge.block_end; ++i) {
                    if (fn.icodes[i].op != icode_op::LABEL)
                        erase.insert(i);
                }
            }
            fn.icodes = rebuild_with_insertions(fn.icodes, erase, {});
            return true;
        }
        return false;
    }
};

class narrow_stack_counted_byte_loops_pass final : public ir_pass {
public:
    explicit narrow_stack_counted_byte_loops_pass(
        bool preserve_pointer_counters)
        : preserve_pointer_counters_(preserve_pointer_counters) {}

    const char *name() const override {
        return "narrow_stack_counted_byte_loops";
    }

    bool run(ir_function &fn) override {
        if (fn.icodes.empty())
            return false;

        control_flow_graph cfg(fn);
        const auto loops = cfg.natural_loops();
        if (loops.empty())
            return false;

        auto same_local = [](const operand &lhs, const operand &rhs) {
            return lhs.is_symbol() && rhs.is_symbol() &&
                   !lhs.is_global && !rhs.is_global &&
                   !lhs.is_param && !rhs.is_param &&
                   base_symbol_key(lhs) == base_symbol_key(rhs);
        };
        auto mentions = [&](const icode &ic, const operand &symbol) {
            return same_local(ic.result, symbol) ||
                   same_local(ic.left, symbol) ||
                   same_local(ic.right, symbol);
        };

        for (const auto &loop : loops) {
            if (loop.outside_preds.size() != 1 || loop.latches.size() != 1)
                continue;

            const auto &header = cfg.block(loop.header);
            const auto &preheader = cfg.block(loop.outside_preds.front());
            const auto &latch = cfg.block(loop.latches.front());
            const size_t header_first = first_non_label_index(fn, header);
            if (header_first + 2 != header.end)
                continue;

            const icode &cmp = fn.icodes[header_first];
            const icode &ifx = fn.icodes[header_first + 1];
            if (cmp.op != icode_op::LT ||
                !cmp.left.is_symbol() || cmp.left.is_global ||
                cmp.left.is_param || !cmp.left.type ||
                !cmp.left.type->is_integer() ||
                cmp.left.type->size() != 2 ||
                cmp.left.type->is_volatile ||
                cmp.right.kind != operand_kind::INT_CONST ||
                cmp.right.ival <= 1 || cmp.right.ival > 255 ||
                !cmp.result.is_temp() || ifx.op != icode_op::IFX ||
                !ifx.left.is_temp() ||
                ifx.left.temp_id != cmp.result.temp_id) {
                continue;
            }
            const operand counter = cmp.left;

            size_t init_idx = fn.icodes.size();
            for (size_t i = preheader.begin; i < preheader.end; ++i) {
                const icode &ic = fn.icodes[i];
                if (ic.op == icode_op::ASSIGN &&
                    same_local(ic.result, counter) &&
                    ic.left.kind == operand_kind::INT_CONST &&
                    ic.left.ival >= 0 && ic.left.ival < cmp.right.ival) {
                    if (init_idx != fn.icodes.size()) {
                        init_idx = fn.icodes.size();
                        break;
                    }
                    init_idx = i;
                }
            }
            if (init_idx == fn.icodes.size())
                continue;

            size_t update_idx = fn.icodes.size();
            for (size_t i = latch.begin; i < latch.end; ++i) {
                const icode &ic = fn.icodes[i];
                if (ic.op == icode_op::ADD &&
                    same_local(ic.result, counter) &&
                    same_local(ic.left, counter) &&
                    ic.right.kind == operand_kind::INT_CONST &&
                    ic.right.ival == 1) {
                    if (update_idx != fn.icodes.size()) {
                        update_idx = fn.icodes.size();
                        break;
                    }
                    update_idx = i;
                }
            }
            if (update_idx == fn.icodes.size())
                continue;

            std::vector<bool> in_loop(fn.icodes.size(), false);
            for (size_t block_id : loop.blocks) {
                const auto &block = cfg.block(block_id);
                for (size_t i = block.begin; i < block.end; ++i)
                    in_loop[i] = true;
            }

            bool safe = true;
            for (size_t i = 0; i < fn.icodes.size(); ++i) {
                const icode &ic = fn.icodes[i];
                if (!mentions(ic, counter))
                    continue;
                if (ic.op == icode_op::ADDRESS_OF ||
                    (!in_loop[i] && i != init_idx)) {
                    safe = false;
                    break;
                }
                // Keep a word counter when it participates directly in
                // pointer address formation.  Later copy propagation may
                // otherwise fold away the explicit zero-extension inserted
                // below while retaining the word-sized address add.
                if (preserve_pointer_counters_ &&
                    (ic.op == icode_op::ADD || ic.op == icode_op::SUB) &&
                    ic.result.type && ic.result.type->is_ptr()) {
                    safe = false;
                    break;
                }
                if (same_local(ic.result, counter) &&
                    i != init_idx && i != update_idx) {
                    safe = false;
                    break;
                }
            }
            if (!safe)
                continue;

            const type_ptr original_type = counter.type;
            const type_ptr byte_type = type::make_uchar();
            auto retag_counter = [&](operand &op) {
                if (same_local(op, counter))
                    op.type = byte_type;
            };

            retag_counter(fn.icodes[init_idx].result);
            fn.icodes[init_idx].left.type = byte_type;
            retag_counter(fn.icodes[header_first].left);
            fn.icodes[header_first].right.type = byte_type;
            retag_counter(fn.icodes[update_idx].result);
            retag_counter(fn.icodes[update_idx].left);
            fn.icodes[update_idx].right.type = byte_type;

            int next_temp = next_temp_id(fn);
            std::map<size_t, std::vector<icode>> insertions;
            for (size_t i = 0; i < fn.icodes.size(); ++i) {
                if (i == init_idx || i == header_first || i == update_idx)
                    continue;
                icode &ic = fn.icodes[i];
                const bool use_left = same_local(ic.left, counter);
                const bool use_right = same_local(ic.right, counter);
                if (!use_left && !use_right)
                    continue;

                operand byte_counter = counter;
                byte_counter.type = byte_type;
                operand widened = operand::make_temp(next_temp++, original_type);
                icode cast;
                cast.op = icode_op::CAST;
                cast.result = widened;
                cast.left = byte_counter;
                cast.line = ic.line;
                insertions[i].push_back(cast);
                if (use_left)
                    ic.left = widened;
                if (use_right)
                    ic.right = widened;
            }

            fn.icodes = rebuild_with_insertions(fn.icodes, {}, insertions);
            return true;
        }
        return false;
    }

private:
    bool preserve_pointer_counters_ = false;
};

class narrow_counted_byte_loops_pass final : public ir_pass {
public:
    explicit narrow_counted_byte_loops_pass(bool allow_short_coupled = false)
        : allow_short_coupled_(allow_short_coupled) {}

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

        // Prove a small unsigned upper bound without relying on source names
        // or loop constants. This lets a word induction variable remain byte
        // wide when its dynamic bound is itself a byte-range expression (for
        // example, `(byte & 15) + 1`). Ambiguous or cyclic definitions simply
        // make the candidate ineligible.
        std::unordered_map<int, icode *> unique_defs;
        std::unordered_set<int> ambiguous_defs;
        for (auto &ic : fn.icodes) {
            if (!defines_result(ic) || !ic.result.is_temp())
                continue;
            const int temp_id = ic.result.temp_id;
            if (!unique_defs.emplace(temp_id, &ic).second) {
                unique_defs.erase(temp_id);
                ambiguous_defs.insert(temp_id);
            }
        }

        std::function<std::optional<unsigned long long>(
            const operand &, std::unordered_set<int> &)> unsigned_max =
            [&](const operand &op, std::unordered_set<int> &visiting)
                -> std::optional<unsigned long long> {
            if (op.kind == operand_kind::INT_CONST) {
                if (op.ival < 0)
                    return std::nullopt;
                return static_cast<unsigned long long>(op.ival);
            }
            if (op.type && op.type->size() == 1 && op.type->is_unsigned())
                return 0xffu;
            if (!op.is_temp() || ambiguous_defs.count(op.temp_id) ||
                !visiting.insert(op.temp_id).second) {
                return std::nullopt;
            }

            auto def_it = unique_defs.find(op.temp_id);
            if (def_it == unique_defs.end()) {
                visiting.erase(op.temp_id);
                return std::nullopt;
            }
            icode &def = *def_it->second;
            if (!def.result.type || !def.result.type->is_unsigned()) {
                visiting.erase(op.temp_id);
                return std::nullopt;
            }
            auto max_of = [&](const operand &value) {
                return unsigned_max(value, visiting);
            };
            std::optional<unsigned long long> result;
            switch (def.op) {
            case icode_op::ASSIGN:
            case icode_op::CAST:
                result = max_of(def.left);
                break;
            case icode_op::BAND: {
                const operand *mask = nullptr;
                if (def.left.kind == operand_kind::INT_CONST)
                    mask = &def.left;
                else if (def.right.kind == operand_kind::INT_CONST)
                    mask = &def.right;
                if (mask && mask->ival >= 0)
                    result = static_cast<unsigned long long>(mask->ival);
                break;
            }
            case icode_op::ADD:
            case icode_op::MUL: {
                auto lhs = max_of(def.left);
                auto rhs = max_of(def.right);
                if (!lhs || !rhs)
                    break;
                if (def.op == icode_op::ADD) {
                    if (*lhs <= 0xffu && *rhs <= 0xffu &&
                        *lhs + *rhs <= 0xffu) {
                        result = *lhs + *rhs;
                    }
                } else if (*lhs == 0 || *rhs <= 0xffu / *lhs) {
                    const auto product = *lhs * *rhs;
                    if (product <= 0xffu)
                        result = product;
                }
                break;
            }
            case icode_op::SHR: {
                auto lhs = max_of(def.left);
                if (lhs && def.right.kind == operand_kind::INT_CONST &&
                    def.right.ival >= 0 && def.right.ival < 64) {
                    result = *lhs >> def.right.ival;
                }
                break;
            }
            default:
                break;
            }
            visiting.erase(op.temp_id);
            return result;
        };

        auto is_byte_bounded = [&](const operand &op) {
            std::unordered_set<int> visiting;
            auto maximum = unsigned_max(op, visiting);
            return maximum && *maximum <= 0xffu;
        };

        std::function<void(const operand &, std::unordered_set<int> &)>
            retag_byte_expression =
            [&](const operand &op, std::unordered_set<int> &visited) {
            if (!op.is_temp() || !visited.insert(op.temp_id).second ||
                !is_byte_bounded(op)) {
                return;
            }
            auto def_it = unique_defs.find(op.temp_id);
            if (def_it == unique_defs.end())
                return;

            icode &def = *def_it->second;
            retag_byte_expression(def.left, visited);
            retag_byte_expression(def.right, visited);
            for (auto &ic : fn.icodes) {
                auto retag = [&](operand &candidate) {
                    if (candidate.is_temp() &&
                        candidate.temp_id == op.temp_id) {
                        candidate.type = byte_type;
                    }
                };
                retag(ic.result);
                retag(ic.left);
                retag(ic.right);
            }

            // Constants participating in an exact byte-range expression can
            // use the same byte operation without changing the value.
            auto retag_const = [&](operand &candidate) {
                if (candidate.kind == operand_kind::INT_CONST &&
                    candidate.ival >= 0 && candidate.ival <= 255) {
                    candidate.type = byte_type;
                }
            };
            retag_const(def.left);
            retag_const(def.right);
        };

        auto retag_temp = [&](int temp_id, size_t begin, size_t end) {
            for (size_t i = begin; i < end; ++i) {
                auto &ic = fn.icodes[i];
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
                    const bool constant_byte_bound =
                        def.right.kind == operand_kind::INT_CONST &&
                        def.right.ival > 0 && def.right.ival <= 255;
                    const bool dynamic_byte_bound =
                        def.right.kind != operand_kind::INT_CONST &&
                        is_byte_bounded(def.right);
                    bool match =
                        def.op == icode_op::LT &&
                        def.left.is_temp() &&
                        def.left.type && def.left.type->size() == 2 &&
                        def.left.type->is_integer() &&
                        (constant_byte_bound ||
                         (def.left.type->is_unsigned() &&
                          dynamic_byte_bound));
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
            size_t init_idx = fn.icodes.size();
            for (size_t i = preheader.begin; i < preheader.end; ++i) {
                auto &ic = fn.icodes[i];
                if (ic.op == icode_op::ASSIGN &&
                    ic.result.is_temp() &&
                    ic.result.temp_id == iv_temp &&
                    ic.left.kind == operand_kind::INT_CONST &&
                    ic.left.ival >= 0 && ic.left.ival <= 255) {
                    found_init = true;
                    init_idx = i;
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
                        const bool inplace_add_one =
                            ic.op == icode_op::ADD &&
                            ic.left.is_temp() &&
                            ic.left.temp_id == iv_temp &&
                            ic.right.kind == operand_kind::INT_CONST &&
                            ic.right.ival == 1;
                        if (ic.op != icode_op::ASSIGN && !inplace_add_one)
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
                if (inc.result.temp_id == iv_temp) {
                    found_update = true;
                    break;
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
            // Earlier canonicalization can coalesce `next = i + 1; i = next`
            // into an in-place ADD.  That form has no separate next temp, but
            // is otherwise the same safely bounded induction variable.
            if (!found_update)
                continue;

            // Other independent induction streams in the same latch do not
            // invalidate the byte proof for this counter.  For short fixed
            // loops, allow a parallel pointer cursor: the whole-function use
            // audit below still requires the counter itself to appear only in
            // init/compare/increment operations.  Keep longer coupled loops on
            // the established allocation path, where narrowing can displace
            // a more valuable word cursor or accumulator.
            size_t latch_operations = 0;
            for (size_t i = latch.begin; i < latch.end; ++i) {
                if (fn.icodes[i].op != icode_op::LABEL)
                    ++latch_operations;
            }
            const size_t canonical_latch_operations = next_temp >= 0 ? 3 : 2;
            if (latch_operations != canonical_latch_operations) {
                const icode &loop_compare = fn.icodes[compare_idx];
                if (!allow_short_coupled_ ||
                    loop_compare.right.kind != operand_kind::INT_CONST ||
                    loop_compare.right.ival > 32) {
                    continue;
                }
            }

            std::vector<bool> in_loop(fn.icodes.size(), false);
            size_t last_loop_index = 0;
            for (size_t block_id : loop.blocks) {
                const auto &block = cfg.block(block_id);
                for (size_t i = block.begin; i < block.end; ++i) {
                    in_loop[i] = true;
                    last_loop_index = std::max(last_loop_index, i);
                }
            }

            auto mentions_temp_id = [](const icode &ic, int temp_id) {
                auto mentions = [&](const operand &op) {
                    return op.is_temp() && op.temp_id == temp_id;
                };
                return mentions(ic.result) || mentions(ic.left) || mentions(ic.right);
            };

            auto global_byte_counter_uses_safe =
                [&](std::unordered_set<int> &next_temps) {
                next_temps.clear();
                std::unordered_set<int> assigned_to_iv_temps;

                for (const auto &ic : fn.icodes) {
                    if (!mentions_temp_id(ic, iv_temp))
                        continue;

                    if (ic.op == icode_op::ASSIGN &&
                        ic.result.is_temp() &&
                        ic.result.temp_id == iv_temp &&
                        ic.left.kind == operand_kind::INT_CONST &&
                        ic.left.ival >= 0 && ic.left.ival <= 255) {
                        continue;
                    }

                    if (ic.op == icode_op::LT &&
                        ic.left.is_temp() &&
                        ic.left.temp_id == iv_temp &&
                        ic.right.kind == operand_kind::INT_CONST &&
                        ic.right.ival > 0 &&
                        ic.right.ival <= 255) {
                        continue;
                    }

                    if (ic.op == icode_op::ADD &&
                        ic.result.is_temp() &&
                        ic.left.is_temp() &&
                        ic.left.temp_id == iv_temp &&
                        ic.right.kind == operand_kind::INT_CONST &&
                        ic.right.ival == 1) {
                        next_temps.insert(ic.result.temp_id);
                        continue;
                    }

                    if (ic.op == icode_op::CAST &&
                        ic.result.type && ic.result.type->size() == 1 &&
                        ic.left.is_temp() &&
                        ic.left.temp_id == iv_temp) {
                        continue;
                    }

                    bool assign_from_known_next = false;
                    if (ic.op == icode_op::ASSIGN &&
                        ic.result.is_temp() &&
                        ic.result.temp_id == iv_temp &&
                        ic.left.is_temp()) {
                        // The producing ADD may appear later in the scan for
                        // another loop body, so validate these uses below.
                        assigned_to_iv_temps.insert(ic.left.temp_id);
                        assign_from_known_next = true;
                    }
                    if (!assign_from_known_next)
                        return false;
                }

                if (next_temps.empty())
                    return false;
                for (int assigned : assigned_to_iv_temps) {
                    if (next_temps.find(assigned) == next_temps.end())
                        return false;
                }

                for (const auto &ic : fn.icodes) {
                    for (int nt : next_temps) {
                        if (!mentions_temp_id(ic, nt))
                            continue;

                        const bool defines_next =
                            ic.op == icode_op::ADD &&
                            ic.result.is_temp() &&
                            ic.result.temp_id == nt &&
                            ic.left.is_temp() &&
                            ic.left.temp_id == iv_temp &&
                            ic.right.kind == operand_kind::INT_CONST &&
                            ic.right.ival == 1;
                        const bool assigns_next_to_iv =
                            ic.op == icode_op::ASSIGN &&
                            ic.result.is_temp() &&
                            ic.result.temp_id == iv_temp &&
                            ic.left.is_temp() &&
                            ic.left.temp_id == nt;
                        if (!defines_next && !assigns_next_to_iv)
                            return false;
                    }
                }

                return true;
            };

            bool confined_to_loop = true;
            size_t lifetime_end = fn.icodes.size();
            for (size_t i = init_idx; i < fn.icodes.size(); ++i) {
                const bool allowed = in_loop[i] || i == init_idx;
                if (allowed)
                    continue;
                if (mentions_temp_id(fn.icodes[i], next_temp)) {
                    confined_to_loop = false;
                    break;
                }
                if (!mentions_temp_id(fn.icodes[i], iv_temp))
                    continue;

                // A later assignment starts a separate lifetime of the same
                // source variable. Keep that lifetime word-sized and retag
                // only the range-proven loop currently being considered.
                const icode &redefine = fn.icodes[i];
                const bool pure_redefinition =
                    i > last_loop_index && defines_result(redefine) &&
                    redefine.result.is_temp() &&
                    redefine.result.temp_id == iv_temp &&
                    !(redefine.left.is_temp() &&
                      redefine.left.temp_id == iv_temp) &&
                    !(redefine.right.is_temp() &&
                      redefine.right.temp_id == iv_temp);
                if (pure_redefinition) {
                    lifetime_end = i;
                    break;
                }
                confined_to_loop = false;
                break;
            }
            std::unordered_set<int> global_next_temps;
            bool global_scope = false;
            if (!confined_to_loop &&
                !global_byte_counter_uses_safe(global_next_temps))
                continue;
            if (!confined_to_loop)
                global_scope = true;

            fn.icodes[init_idx].result.type = byte_type;
            fn.icodes[init_idx].left.type = byte_type;
            const size_t retag_begin = global_scope ? 0 : init_idx;
            const size_t retag_end = global_scope ? fn.icodes.size()
                                                   : lifetime_end;
            retag_temp(iv_temp, retag_begin, retag_end);
            retag_temp(next_temp, retag_begin, retag_end);
            for (int nt : global_next_temps)
                retag_temp(nt, 0, fn.icodes.size());

            auto &cmp = fn.icodes[compare_idx];
            if (cmp.left.is_temp() && cmp.left.temp_id == iv_temp)
                cmp.left.type = byte_type;
            if (cmp.right.kind != operand_kind::INT_CONST &&
                is_byte_bounded(cmp.right)) {
                std::unordered_set<int> retagged;
                retag_byte_expression(cmp.right, retagged);
            }
            if (cmp.right.kind == operand_kind::INT_CONST ||
                is_byte_bounded(cmp.right)) {
                cmp.right.type = byte_type;
            }

            changed = true;
        }

        return changed;
    }

private:
    bool allow_short_coupled_ = false;
};

class block_fill_loop_pass final : public ir_pass {
public:
    const char *name() const override { return "block_fill_loop"; }

    bool run(ir_function &fn) override {
        if (fn.icodes.empty())
            return false;

        control_flow_graph cfg(fn);
        const auto loops = cfg.natural_loops();
        for (const auto &loop : loops) {
            if (loop.blocks.size() != 3 || loop.outside_preds.size() != 1 ||
                loop.latches.size() != 1) {
                continue;
            }

            const auto &header = cfg.block(loop.header);
            const auto &preheader = cfg.block(loop.outside_preds.front());
            const auto &latch = cfg.block(loop.latches.front());
            const size_t header_first = first_non_label_index(fn, header);
            if (header_first + 2 != header.end)
                continue;

            const icode &cmp = fn.icodes[header_first];
            const icode &ifx = fn.icodes[header_first + 1];
            const bool temp_counter = cmp.left.is_temp();
            const bool stack_counter =
                cmp.left.is_symbol() && !cmp.left.is_global &&
                !cmp.left.is_param && cmp.left.type &&
                cmp.left.type->is_integer() &&
                cmp.left.type->size() == 2 &&
                !cmp.left.type->is_volatile;
            if (cmp.op != icode_op::LT || !cmp.result.is_temp() ||
                (!temp_counter && !stack_counter) ||
                cmp.right.kind != operand_kind::INT_CONST ||
                cmp.right.ival < 2 || cmp.right.ival > 65535 ||
                ifx.op != icode_op::IFX || !ifx.left.is_temp() ||
                ifx.left.temp_id != cmp.result.temp_id) {
                continue;
            }

            const auto body_id = cfg.block_for_label(ifx.true_lbl);
            const auto exit_id = cfg.block_for_label(ifx.false_lbl);
            if (!body_id || !exit_id || !loop.blocks.count(*body_id) ||
                loop.blocks.count(*exit_id) || *body_id == loop.header ||
                *body_id == loop.latches.front()) {
                continue;
            }
            const auto &body = cfg.block(*body_id);
            const size_t body_first = first_non_label_index(fn, body);
            size_t store_index = body_first;
            if (body_first + 2 == body.end) {
                // The terminal lockstep-pointer lowering intentionally does
                // not rerun DCE.  It can leave the original `base + counter`
                // address calculation immediately before the rewritten store.
                // Accept only an ADD whose result is now completely unused.
                const icode &dead_address = fn.icodes[body_first];
                int uses = 0;
                if (dead_address.result.is_temp()) {
                    const int temp_id = dead_address.result.temp_id;
                    for (const auto &candidate : fn.icodes) {
                        if (candidate.left.is_temp() &&
                            candidate.left.temp_id == temp_id)
                            ++uses;
                        if (candidate.right.is_temp() &&
                            candidate.right.temp_id == temp_id)
                            ++uses;
                    }
                }
                if (dead_address.op != icode_op::ADD ||
                    !dead_address.result.is_temp() || uses != 0)
                    continue;
                store_index = body_first + 1;
            } else if (body_first + 1 != body.end) {
                continue;
            }
            const icode &store = fn.icodes[store_index];
            if (store.op != icode_op::SET_VALUE_AT ||
                !store.result.is_temp() ||
                store.left.kind != operand_kind::INT_CONST ||
                store.left.ival < 0 || store.left.ival > 255 ||
                !store.left.type ||
                store.left.type->size() != 1) {
                continue;
            }

            const operand counter = cmp.left;
            const operand pointer = store.result;
            if (same_value_operand(counter, pointer))
                continue;

            struct update_shape {
                size_t add = static_cast<size_t>(-1);
                size_t commit = static_cast<size_t>(-1);
                int next_temp = -1;
            };
            auto find_unit_update = [&](const operand &state)
                -> std::optional<update_shape> {
                update_shape shape;
                for (size_t i = latch.begin; i < latch.end; ++i) {
                    const icode &ic = fn.icodes[i];
                    if (ic.op != icode_op::ADD ||
                        !same_value_operand(ic.left, state) ||
                        ic.right.kind != operand_kind::INT_CONST ||
                        ic.right.ival != 1) {
                        continue;
                    }
                    if (shape.add != static_cast<size_t>(-1))
                        return std::nullopt;
                    shape.add = i;
                    if (same_value_operand(ic.result, state)) {
                        shape.commit = i;
                    } else if (state.is_temp() && ic.result.is_temp()) {
                        shape.next_temp = ic.result.temp_id;
                        for (size_t j = i + 1; j < latch.end; ++j) {
                            const icode &commit = fn.icodes[j];
                            if (commit.op == icode_op::ASSIGN &&
                                same_value_operand(commit.result, state) &&
                                commit.left.is_temp() &&
                                commit.left.temp_id == shape.next_temp) {
                                shape.commit = j;
                                break;
                            }
                        }
                    } else {
                        continue;
                    }
                }
                if (shape.add == static_cast<size_t>(-1) ||
                    shape.commit == static_cast<size_t>(-1)) {
                    return std::nullopt;
                }
                return shape;
            };

            auto counter_update = find_unit_update(counter);
            auto pointer_update = find_unit_update(pointer);
            if (!counter_update || !pointer_update)
                continue;

            const std::string header_label = first_label_in_block(fn, header);
            size_t latch_ops = 0;
            bool valid_latch = !header_label.empty();
            for (size_t i = latch.begin; i < latch.end; ++i) {
                const icode &ic = fn.icodes[i];
                if (ic.op == icode_op::LABEL)
                    continue;
                ++latch_ops;
                const bool scaffold =
                    i == counter_update->add ||
                    i == counter_update->commit ||
                    i == pointer_update->add ||
                    i == pointer_update->commit ||
                    (ic.op == icode_op::GOTO &&
                     ic.label_name == header_label);
                if (!scaffold)
                    valid_latch = false;
            }
            std::unordered_set<size_t> expected_latch_indices{
                counter_update->add, counter_update->commit,
                pointer_update->add, pointer_update->commit};
            const size_t expected_latch_ops = expected_latch_indices.size() + 1;
            if (!valid_latch || latch_ops != expected_latch_ops)
                continue;

            size_t counter_init = fn.icodes.size();
            size_t pointer_init = fn.icodes.size();
            for (size_t i = preheader.begin; i < preheader.end; ++i) {
                const icode &ic = fn.icodes[i];
                if (ic.op == icode_op::ASSIGN &&
                    same_value_operand(ic.result, counter) &&
                    ic.left.kind == operand_kind::INT_CONST &&
                    ic.left.ival == 0) {
                    if (counter_init != fn.icodes.size()) {
                        counter_init = fn.icodes.size();
                        break;
                    }
                    counter_init = i;
                }
                if ((ic.op == icode_op::ASSIGN ||
                     ic.op == icode_op::ADDRESS_OF) &&
                    ic.result.is_temp() &&
                    ic.result.temp_id == pointer.temp_id) {
                    if (pointer_init != fn.icodes.size()) {
                        pointer_init = fn.icodes.size();
                        break;
                    }
                    pointer_init = i;
                }
            }
            if (counter_init == fn.icodes.size() ||
                pointer_init == fn.icodes.size()) {
                continue;
            }
            operand destination = fn.icodes[pointer_init].left;
            size_t destination_source_def = fn.icodes.size();
            if (destination.is_temp()) {
                const int source_temp = destination.temp_id;
                size_t source_def = fn.icodes.size();
                for (size_t i = preheader.begin; i < pointer_init; ++i) {
                    const icode &ic = fn.icodes[i];
                    if ((ic.op == icode_op::ASSIGN ||
                         ic.op == icode_op::ADDRESS_OF) &&
                        ic.result.is_temp() &&
                        ic.result.temp_id == source_temp) {
                        if (source_def != fn.icodes.size()) {
                            source_def = fn.icodes.size();
                            break;
                        }
                        source_def = i;
                    }
                }
                if (source_def != fn.icodes.size()) {
                    destination = fn.icodes[source_def].left;
                    int uses = 0;
                    for (const icode &candidate : fn.icodes) {
                        for_each_use_operand(candidate, [&](const operand &op) {
                            if (op.is_temp() && op.temp_id == source_temp)
                                ++uses;
                        });
                    }
                    if (uses == 1)
                        destination_source_def = source_def;
                }
            }
            const bool direct_object =
                destination.kind == operand_kind::LABEL_REF ||
                (destination.kind == operand_kind::SYMBOL &&
                 destination.is_global && destination.byte_offset == 0);
            if (!direct_object)
                continue;

            auto mentions_temp = [](const icode &ic, int temp_id) {
                auto mentions = [&](const operand &op) {
                    return op.is_temp() && op.temp_id == temp_id;
                };
                return mentions(ic.result) || mentions(ic.left) ||
                       mentions(ic.right);
            };
            std::vector<bool> in_loop(fn.icodes.size(), false);
            size_t last_loop_index = 0;
            for (size_t block_id : loop.blocks) {
                const auto &block = cfg.block(block_id);
                for (size_t i = block.begin; i < block.end; ++i) {
                    in_loop[i] = true;
                    last_loop_index = std::max(last_loop_index, i);
                }
            }

            std::vector<size_t> block_for_index(
                fn.icodes.size(), static_cast<size_t>(-1));
            for (const auto &block : cfg.blocks()) {
                for (size_t i = block.begin; i < block.end; ++i)
                    block_for_index[i] = block.id;
            }
            const auto dominators = cfg.dominators();

            auto lifetime_confined = [&](int temp_id, int next_temp,
                                         size_t init_index) {
                std::vector<size_t> later_mentions;
                for (size_t i = init_index; i < fn.icodes.size(); ++i) {
                    if (i == init_index || in_loop[i])
                        continue;
                    if (next_temp >= 0 && mentions_temp(fn.icodes[i], next_temp))
                        return false;
                    if (mentions_temp(fn.icodes[i], temp_id))
                        later_mentions.push_back(i);
                }
                if (later_mentions.empty())
                    return true;

                const size_t redefine_index = later_mentions.front();
                const icode &redefine = fn.icodes[redefine_index];
                const bool new_lifetime =
                    redefine_index > last_loop_index &&
                    defines_result(redefine) && redefine.result.is_temp() &&
                    redefine.result.temp_id == temp_id &&
                    !(redefine.left.is_temp() &&
                      redefine.left.temp_id == temp_id) &&
                    !(redefine.right.is_temp() &&
                      redefine.right.temp_id == temp_id);
                if (!new_lifetime)
                    return false;

                const size_t redefine_block =
                    block_for_index[redefine_index];
                for (size_t mention_index : later_mentions) {
                    const size_t mention_block =
                        block_for_index[mention_index];
                    const bool dominated =
                        mention_block == redefine_block
                            ? mention_index >= redefine_index
                            : mention_block < dominators.size() &&
                              dominators[mention_block].count(
                                  redefine_block) != 0;
                    if (!dominated)
                        return false;
                }
                return true;
            };
            bool counter_confined = true;
            if (temp_counter) {
                counter_confined = lifetime_confined(
                    counter.temp_id, counter_update->next_temp, counter_init);
            } else {
                // A stack counter is removable only when its complete
                // lifetime belongs to this loop.  This excludes counters
                // whose final value is observed, reused, or address-taken;
                // those require an explicit assignment of the trip count.
                for (size_t i = 0; i < fn.icodes.size(); ++i) {
                    if (i == counter_init || in_loop[i])
                        continue;
                    const icode &ic = fn.icodes[i];
                    if (same_symbol_slot(ic.result, counter) ||
                        same_symbol_slot(ic.left, counter) ||
                        same_symbol_slot(ic.right, counter)) {
                        counter_confined = false;
                        break;
                    }
                }
            }
            if (!counter_confined ||
                !lifetime_confined(pointer.temp_id,
                                   pointer_update->next_temp, pointer_init)) {
                continue;
            }

            std::unordered_set<size_t> erase{counter_init, pointer_init};
            if (destination_source_def != fn.icodes.size())
                erase.insert(destination_source_def);
            for (size_t block_id : loop.blocks) {
                const auto &block = cfg.block(block_id);
                for (size_t i = block.begin; i < block.end; ++i)
                    erase.insert(i);
            }
            if (preheader.begin < preheader.end) {
                const size_t tail = preheader.end - 1;
                if (fn.icodes[tail].op == icode_op::GOTO &&
                    fn.icodes[tail].label_name == header_label) {
                    erase.insert(tail);
                }
            }

            icode fill;
            fill.op = icode_op::BLOCK_FILL;
            fill.result = destination.kind == operand_kind::SYMBOL
                              ? operand::make_label(destination.name)
                              : destination;
            fill.result.type = fn.icodes[pointer_init].result.type;
            fill.left = store.left;
            fill.right = cmp.right;
            fill.line = store.line;
            std::map<size_t, std::vector<icode>> insertions;
            insertions[std::min(counter_init, pointer_init)].push_back(fill);
            fn.icodes = rebuild_with_insertions(fn.icodes, erase, insertions);
            return true;
        }
        return false;
    }
};

class countdown_dead_loops_pass final : public ir_pass {
public:
    const char *name() const override { return "countdown_dead_loops"; }

    bool run(ir_function &fn) override {
        if (fn.icodes.empty())
            return false;

        control_flow_graph cfg(fn);
        const auto loops = cfg.natural_loops();
        if (loops.empty())
            return false;

        struct byte_range {
            unsigned int minimum = 0;
            unsigned int maximum = 0xff;
        };
        std::unordered_map<int, const icode *> unique_defs;
        std::unordered_set<int> ambiguous_defs;
        for (const auto &ic : fn.icodes) {
            if (!defines_result(ic) || !ic.result.is_temp())
                continue;
            if (!unique_defs.emplace(ic.result.temp_id, &ic).second) {
                unique_defs.erase(ic.result.temp_id);
                ambiguous_defs.insert(ic.result.temp_id);
            }
        }

        std::function<std::optional<byte_range>(
            const operand &, std::unordered_set<int> &)> infer_byte_range =
            [&](const operand &op, std::unordered_set<int> &visiting)
                -> std::optional<byte_range> {
            if (op.kind == operand_kind::INT_CONST) {
                if (op.ival < 0 || op.ival > 255)
                    return std::nullopt;
                const auto value = static_cast<unsigned int>(op.ival);
                return byte_range{value, value};
            }
            if (!op.is_temp()) {
                if (op.type && op.type->size() == 1 &&
                    op.type->is_unsigned()) {
                    return byte_range{};
                }
                return std::nullopt;
            }
            if (ambiguous_defs.count(op.temp_id) ||
                !visiting.insert(op.temp_id).second) {
                return op.type && op.type->size() == 1 &&
                               op.type->is_unsigned()
                           ? std::optional<byte_range>(byte_range{})
                           : std::nullopt;
            }

            auto def_it = unique_defs.find(op.temp_id);
            if (def_it == unique_defs.end()) {
                visiting.erase(op.temp_id);
                return op.type && op.type->size() == 1 &&
                               op.type->is_unsigned()
                           ? std::optional<byte_range>(byte_range{})
                           : std::nullopt;
            }
            const icode &def = *def_it->second;
            auto range_of = [&](const operand &value) {
                return infer_byte_range(value, visiting);
            };
            std::optional<byte_range> range;
            switch (def.op) {
            case icode_op::ASSIGN:
            case icode_op::CAST:
                range = range_of(def.left);
                break;
            case icode_op::BAND: {
                const operand *mask = nullptr;
                if (def.left.kind == operand_kind::INT_CONST)
                    mask = &def.left;
                else if (def.right.kind == operand_kind::INT_CONST)
                    mask = &def.right;
                if (mask && mask->ival >= 0 && mask->ival <= 255)
                    range = byte_range{
                        0u, static_cast<unsigned int>(mask->ival)};
                break;
            }
            case icode_op::ADD: {
                auto lhs = range_of(def.left);
                auto rhs = range_of(def.right);
                if (lhs && rhs &&
                    lhs->maximum + rhs->maximum <= 255u) {
                    range = byte_range{
                        lhs->minimum + rhs->minimum,
                        lhs->maximum + rhs->maximum};
                }
                break;
            }
            case icode_op::MUL: {
                auto lhs = range_of(def.left);
                auto rhs = range_of(def.right);
                if (lhs && rhs &&
                    lhs->maximum * rhs->maximum <= 255u) {
                    range = byte_range{
                        lhs->minimum * rhs->minimum,
                        lhs->maximum * rhs->maximum};
                }
                break;
            }
            case icode_op::SHR: {
                auto lhs = range_of(def.left);
                if (lhs && def.right.kind == operand_kind::INT_CONST &&
                    def.right.ival >= 0 && def.right.ival < 8) {
                    range = byte_range{
                        lhs->minimum >> def.right.ival,
                        lhs->maximum >> def.right.ival};
                }
                break;
            }
            default:
                break;
            }
            visiting.erase(op.temp_id);
            return range;
        };

        auto positive_byte_bound = [&](const operand &op) {
            std::unordered_set<int> visiting;
            auto range = infer_byte_range(op, visiting);
            return range && range->minimum > 0 && range->maximum <= 255;
        };

        auto mentions_temp = [](const icode &ic, int temp_id) {
            auto mentions = [&](const operand &op) {
                return op.is_temp() && op.temp_id == temp_id;
            };
            return mentions(ic.result) || mentions(ic.left) ||
                   mentions(ic.right);
        };

        for (const auto &loop : loops) {
            if (loop.outside_preds.size() != 1 || loop.latches.size() != 1)
                continue;

            const auto &header = cfg.block(loop.header);
            const auto &preheader = cfg.block(loop.outside_preds.front());
            const auto &latch = cfg.block(loop.latches.front());
            const size_t header_first = first_non_label_index(fn, header);
            if (header_first + 2 != header.end)
                continue;

            const icode &cmp = fn.icodes[header_first];
            const icode &ifx = fn.icodes[header_first + 1];
            const bool constant_bound =
                cmp.right.kind == operand_kind::INT_CONST &&
                cmp.right.ival > 1 && cmp.right.ival <= 65535;
            const bool dynamic_bound =
                cmp.right.kind != operand_kind::INT_CONST &&
                cmp.right.type && cmp.right.type->size() == 1 &&
                cmp.right.type->is_unsigned() &&
                positive_byte_bound(cmp.right);
            if (cmp.op != icode_op::LT || !cmp.result.is_temp() ||
                !cmp.left.is_temp() || cmp.left.byte_offset != 0 ||
                (cmp.left.type && cmp.left.type->is_volatile) ||
                // Leave one-trip loops to one_trip_counted_loop_fold, which
                // removes their control flow completely later in the pass
                // pipeline.
                (!constant_bound && !dynamic_bound) ||
                ifx.op != icode_op::IFX || !ifx.left.is_temp() ||
                ifx.left.temp_id != cmp.result.temp_id ||
                ifx.true_lbl.empty() || ifx.false_lbl.empty()) {
                continue;
            }

            const auto body_id = cfg.block_for_label(ifx.true_lbl);
            const auto exit_id = cfg.block_for_label(ifx.false_lbl);
            if (!body_id || !exit_id || !loop.blocks.count(*body_id) ||
                loop.blocks.count(*exit_id) || *body_id == loop.header ||
                header.end != cfg.block(*body_id).begin) {
                continue;
            }

            const int iv_temp = cmp.left.temp_id;
            size_t init_idx = fn.icodes.size();
            for (size_t i = preheader.begin; i < preheader.end; ++i) {
                const icode &ic = fn.icodes[i];
                if (ic.op == icode_op::ASSIGN && ic.result.is_temp() &&
                    ic.result.temp_id == iv_temp &&
                    ic.left.kind == operand_kind::INT_CONST &&
                    ic.left.ival == 0) {
                    if (init_idx != fn.icodes.size()) {
                        init_idx = fn.icodes.size();
                        break;
                    }
                    init_idx = i;
                }
            }
            if (init_idx == fn.icodes.size())
                continue;

            const size_t latch_first = first_non_label_index(fn, latch);
            const size_t latch_insns = latch.end - latch_first;
            if (latch_insns != 2 && latch_insns != 3)
                continue;

            const icode &inc = fn.icodes[latch_first];
            const icode &back = fn.icodes[latch.end - 1];
            if (inc.op != icode_op::ADD || !inc.result.is_temp() ||
                !inc.left.is_temp() || inc.left.temp_id != iv_temp ||
                inc.left.byte_offset != 0 ||
                inc.right.kind != operand_kind::INT_CONST ||
                inc.right.ival != 1 || back.op != icode_op::GOTO) {
                continue;
            }

            const std::string header_label = first_label_in_block(fn, header);
            if (header_label.empty() || back.label_name != header_label)
                continue;

            int next_temp = -1;
            size_t commit_idx = fn.icodes.size();
            if (latch_insns == 2) {
                if (inc.result.temp_id != iv_temp)
                    continue;
            } else {
                const icode &commit = fn.icodes[latch_first + 1];
                if (commit.op != icode_op::ASSIGN ||
                    !commit.result.is_temp() ||
                    commit.result.temp_id != iv_temp ||
                    !commit.left.is_temp() ||
                    commit.left.temp_id != inc.result.temp_id) {
                    continue;
                }
                next_temp = inc.result.temp_id;
                commit_idx = latch_first + 1;
            }

            // The counter and its increment result must be confined to loop
            // control for this lifetime. A later definition may start a new
            // lifetime, but only when it dominates every subsequent mention.
            std::vector<bool> in_loop(fn.icodes.size(), false);
            size_t last_loop_index = 0;
            for (size_t block_id : loop.blocks) {
                const auto &block = cfg.block(block_id);
                for (size_t i = block.begin; i < block.end; ++i) {
                    in_loop[i] = true;
                    last_loop_index = std::max(last_loop_index, i);
                }
            }

            bool confined = true;
            std::vector<size_t> later_mentions;
            for (size_t i = init_idx; i < fn.icodes.size(); ++i) {
                const bool scaffold =
                    i == init_idx || i == header_first ||
                    i == header_first + 1 || i == latch_first ||
                    i == commit_idx || i == latch.end - 1;
                if (scaffold)
                    continue;
                if (next_temp >= 0 && mentions_temp(fn.icodes[i], next_temp)) {
                    confined = false;
                    break;
                }
                if (!mentions_temp(fn.icodes[i], iv_temp))
                    continue;
                if (in_loop[i] || i <= last_loop_index) {
                    confined = false;
                    break;
                }
                later_mentions.push_back(i);
            }
            if (!confined)
                continue;

            size_t lifetime_end = fn.icodes.size();
            if (!later_mentions.empty()) {
                const size_t redefine_idx = later_mentions.front();
                const icode &redefine = fn.icodes[redefine_idx];
                const bool pure_redefinition =
                    defines_result(redefine) && redefine.result.is_temp() &&
                    redefine.result.temp_id == iv_temp &&
                    !(redefine.left.is_temp() &&
                      redefine.left.temp_id == iv_temp) &&
                    !(redefine.right.is_temp() &&
                      redefine.right.temp_id == iv_temp);
                if (!pure_redefinition)
                    continue;

                std::vector<size_t> block_for_index(
                    fn.icodes.size(), static_cast<size_t>(-1));
                for (const auto &block : cfg.blocks()) {
                    for (size_t i = block.begin; i < block.end; ++i)
                        block_for_index[i] = block.id;
                }
                const auto dom = cfg.dominators();
                const size_t redefine_block = block_for_index[redefine_idx];
                for (size_t mention_idx : later_mentions) {
                    const size_t mention_block = block_for_index[mention_idx];
                    const bool dominated =
                        mention_block == redefine_block
                            ? mention_idx >= redefine_idx
                            : mention_block < dom.size() &&
                              dom[mention_block].count(redefine_block) != 0;
                    if (!dominated) {
                        confined = false;
                        break;
                    }
                }
                if (!confined)
                    continue;
                lifetime_end = redefine_idx;
            }

            // Likewise, the comparison result may only feed this branch.
            for (size_t i = 0; i < fn.icodes.size() && confined; ++i) {
                if (i != header_first && i != header_first + 1 &&
                    mentions_temp(fn.icodes[i], cmp.result.temp_id)) {
                    confined = false;
                }
            }
            if (!confined)
                continue;

            const bool byte_count =
                cmp.right.kind != operand_kind::INT_CONST ||
                cmp.right.ival <= 255;
            const type_ptr count_type =
                byte_count ? type::make_uchar()
                           : (cmp.left.type ? cmp.left.type : type::make_int());
            auto retag_temp = [&](int temp_id) {
                if (temp_id < 0)
                    return;
                for (size_t i = init_idx; i < lifetime_end; ++i) {
                    auto &ic = fn.icodes[i];
                        auto retag = [&](operand &op) {
                            if (op.is_temp() && op.temp_id == temp_id)
                                op.type = count_type;
                    };
                    retag(ic.result);
                    retag(ic.left);
                    retag(ic.right);
                }
            };
            retag_temp(iv_temp);
            retag_temp(next_temp);

            const operand trip_count = cmp.right;
            auto &init = fn.icodes[init_idx];
            init.left = trip_count;
            init.left.type = count_type;
            init.result.type = count_type;

            auto &step = fn.icodes[latch_first];
            step.op = icode_op::SUB;
            step.result = operand::make_temp(iv_temp, count_type);
            step.left = step.result;
            step.right = operand::make_int(1, count_type);

            auto &branch = fn.icodes[latch.end - 1];
            branch.op = icode_op::IFX;
            branch.left = operand::make_temp(iv_temp, count_type);
            branch.result = operand::make_none();
            branch.right = operand::make_none();
            branch.label_name.clear();
            branch.true_lbl = ifx.true_lbl;
            branch.false_lbl = ifx.false_lbl;

            std::unordered_set<size_t> erase{
                header_first, header_first + 1
            };
            if (commit_idx != fn.icodes.size())
                erase.insert(commit_idx);
            fn.icodes = rebuild_with_insertions(fn.icodes, erase, {});
            return true;
        }

        return false;
    }
};

class one_trip_counted_loop_fold_pass final : public ir_pass {
public:
    const char *name() const override { return "one_trip_counted_loop_fold"; }

    bool run(ir_function &fn) override {
        if (fn.icodes.empty())
            return false;

        control_flow_graph cfg(fn);
        auto loops = cfg.natural_loops();
        if (loops.empty())
            return false;

        alias_info alias = build_alias_info(fn);
        auto mentions_temp_id = [](const icode &ic, int temp_id) {
            auto mentions = [&](const operand &op) {
                return op.is_temp() && op.temp_id == temp_id;
            };
            return mentions(ic.result) || mentions(ic.left) || mentions(ic.right);
        };

        for (const auto &loop : loops) {
            if (loop.outside_preds.size() != 1 || loop.latches.size() != 1)
                continue;
            if (loop.blocks.size() != 3)
                continue;

            const auto &header = cfg.block(loop.header);
            const auto &preheader = cfg.block(loop.outside_preds.front());
            const auto &latch = cfg.block(loop.latches.front());
            if (header.end == 0 || latch.end == 0)
                continue;

            size_t ifx_idx = header.end - 1;
            if (fn.icodes[ifx_idx].op != icode_op::IFX ||
                !fn.icodes[ifx_idx].left.is_temp()) {
                continue;
            }
            const icode &ifx = fn.icodes[ifx_idx];
            auto body_block_id = cfg.block_for_label(ifx.true_lbl);
            auto exit_block_id = cfg.block_for_label(ifx.false_lbl);
            if (!body_block_id || !exit_block_id)
                continue;
            if (!loop.blocks.count(*body_block_id) ||
                loop.blocks.count(*exit_block_id) ||
                *body_block_id == loop.header ||
                *body_block_id == loop.latches.front()) {
                continue;
            }
            const auto &body = cfg.block(*body_block_id);

            // Keep this first version simple: header, body, latch, then exit.
            if (!(header.end == body.begin && body.end == latch.begin))
                continue;
            if (!body.succs.empty() &&
                !(body.succs.size() == 1 && body.succs.front() == loop.latches.front())) {
                continue;
            }

            const size_t header_first = first_non_label_index(fn, header);
            if (header_first + 2 != header.end)
                continue;
            const icode &cmp = fn.icodes[header_first];
            if (cmp.op != icode_op::LT ||
                !cmp.result.is_temp() ||
                cmp.result.temp_id != ifx.left.temp_id ||
                cmp.right.kind != operand_kind::INT_CONST ||
                cmp.right.ival != 1) {
                continue;
            }
            const bool iv_is_temp = cmp.left.is_temp();
            const bool iv_is_local_symbol =
                cmp.left.is_symbol() && !cmp.left.is_global &&
                !cmp.left.is_param && !cmp.left.is_tls &&
                !cmp.left.is_sfr && !cmp.left.is_func &&
                cmp.left.byte_offset == 0 && cmp.left.type &&
                !cmp.left.type->is_volatile &&
                !base_symbol_address_taken(alias, cmp.left);
            if (!iv_is_temp && !iv_is_local_symbol)
                continue;

            const int iv_temp = iv_is_temp ? cmp.left.temp_id : -1;
            const operand iv_value = cmp.left;
            auto same_iv = [&](const operand &op) {
                if (iv_is_temp)
                    return op.is_temp() && op.temp_id == iv_temp;
                return op.is_symbol() && op.byte_offset == 0 &&
                       same_symbol_slot(op, iv_value);
            };
            auto mentions_iv = [&](const icode &ic) {
                return same_iv(ic.result) || same_iv(ic.left) ||
                       same_iv(ic.right);
            };

            size_t init_idx = fn.icodes.size();
            for (size_t i = preheader.begin; i < preheader.end; ++i) {
                const auto &ic = fn.icodes[i];
                if (ic.op == icode_op::ASSIGN &&
                    same_iv(ic.result) &&
                    ic.left.kind == operand_kind::INT_CONST &&
                    ic.left.ival == 0) {
                    init_idx = i;
                }
            }
            if (init_idx == fn.icodes.size())
                continue;

            const size_t latch_first = first_non_label_index(fn, latch);
            const size_t latch_insns = latch.end - latch_first;
            if (latch_insns != 2 && latch_insns != 3)
                continue;
            const icode &inc = fn.icodes[latch_first];
            const icode &back = fn.icodes[latch.end - 1];
            if (inc.op != icode_op::ADD ||
                !same_iv(inc.left) ||
                inc.right.kind != operand_kind::INT_CONST ||
                inc.right.ival != 1 ||
                back.op != icode_op::GOTO) {
                continue;
            }
            int next_temp = -1;
            if (latch_insns == 2) {
                // add_const_chain can coalesce `next = i + 1; i = next`
                // into a direct loop-carried update before this pass runs.
                if (!same_iv(inc.result))
                    continue;
            } else {
                const icode &assign = fn.icodes[latch_first + 1];
                if (assign.op != icode_op::ASSIGN ||
                    !same_iv(assign.result) ||
                    !inc.result.is_temp() ||
                    !assign.left.is_temp() ||
                    assign.left.temp_id != inc.result.temp_id) {
                    continue;
                }
                next_temp = inc.result.temp_id;
            }
            const std::string header_label = first_label_in_block(fn, header);
            if (header_label.empty() || back.label_name != header_label)
                continue;

            bool confined = true;
            for (size_t i = 0; i < fn.icodes.size(); ++i) {
                const bool loop_scaffold =
                    i == init_idx ||
                    (i >= header.begin && i < header.end) ||
                    (i >= latch.begin && i < latch.end);
                if (!loop_scaffold &&
                    (mentions_iv(fn.icodes[i]) ||
                     (next_temp >= 0 &&
                      mentions_temp_id(fn.icodes[i], next_temp)))) {
                    confined = false;
                    break;
                }
            }
            if (!confined)
                continue;

            std::unordered_set<size_t> erase;
            erase.insert(init_idx);
            for (size_t i = header.begin; i < header.end; ++i)
                erase.insert(i);
            for (size_t i = latch.begin; i < latch.end; ++i)
                erase.insert(i);

            fn.icodes = rebuild_with_insertions(fn.icodes, erase, {});
            return true;
        }

        return false;
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
        std::unordered_map<int, size_t> temp_def_index;
        for (const auto &ic : fn.icodes) {
            if (ic.result.is_temp())
                temp_defs[ic.result.temp_id] = &ic;
        }
        for (size_t i = 0; i < fn.icodes.size(); ++i) {
            if (fn.icodes[i].result.is_temp())
                temp_def_index[fn.icodes[i].result.temp_id] = i;
        }

        auto loop_index_scale = [&](const operand &op, int iv_temp)
            -> std::optional<int64_t> {
            std::function<std::optional<int64_t>(const operand &, int)> match =
                [&](const operand &cur, int depth) -> std::optional<int64_t> {
                if (depth > 6)
                    return std::nullopt;
                if (cur.is_temp() && cur.temp_id == iv_temp)
                    return 1;
                if (!cur.is_temp())
                    return std::nullopt;
                auto it = temp_defs.find(cur.temp_id);
                if (it == temp_defs.end() || !it->second)
                    return std::nullopt;
                const icode *def = it->second;
                if ((def->op == icode_op::ASSIGN || def->op == icode_op::CAST) &&
                    def->left.is_temp()) {
                    return match(def->left, depth + 1);
                }
                if (def->op == icode_op::SHL &&
                    def->right.kind == operand_kind::INT_CONST &&
                    def->right.ival >= 0 && def->right.ival <= 8) {
                    auto scale = match(def->left, depth + 1);
                    if (scale)
                        return *scale << def->right.ival;
                }
                if (def->op == icode_op::MUL) {
                    const operand *value = &def->left;
                    const operand *constant = &def->right;
                    if (value->kind == operand_kind::INT_CONST)
                        std::swap(value, constant);
                    if (constant->kind == operand_kind::INT_CONST &&
                        constant->ival > 0 && constant->ival <= 256) {
                        auto scale = match(*value, depth + 1);
                        if (scale && *scale <= 256 / constant->ival)
                            return *scale * constant->ival;
                    }
                }
                return std::nullopt;
            };
            return match(op, 0);
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

        auto is_pointer_base = [](const operand &op) {
            if (op.kind == operand_kind::LABEL_REF)
                return op.type && (op.type->is_array() || op.type->is_ptr());
            if (op.kind != operand_kind::SYMBOL &&
                op.kind != operand_kind::TEMP)
                return false;
            if (op.kind == operand_kind::SYMBOL &&
                (op.is_tls || op.is_sfr || op.is_func))
                return false;
            if (op.byte_offset != 0 || !op.type)
                return false;
            if (op.type->is_array())
                return op.type->base && !op.type->base->is_volatile;
            if (op.type->is_ptr())
                return !op.type->is_far_ptr() &&
                       op.type->base &&
                       !op.type->base->is_volatile;
            return false;
        };

        auto base_key = [](const operand &op) {
            if (op.kind == operand_kind::LABEL_REF)
                return std::string("label|") + op.name;
            if (op.kind == operand_kind::TEMP)
                return std::string("temp|") + std::to_string(op.temp_id);
            return symbol_key(op);
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
                int64_t stride = 1;
            };
            std::unordered_map<std::string, candidate_group> groups;

            auto loop_only_temp = [&](int temp_id) {
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
                    if (!in_loop_inst[i])
                        return false;
                }
                return saw_use;
            };

            auto base_stable_in_loop = [&](const operand &base) {
                if (base.kind == operand_kind::LABEL_REF)
                    return true;

                if (base.kind == operand_kind::TEMP) {
                    auto it = temp_def_index.find(base.temp_id);
                    if (it == temp_def_index.end())
                        return false;
                    if (in_loop_inst[it->second])
                        return false;
                    for (size_t block_id : loop.blocks) {
                        const auto &block = cfg.block(block_id);
                        for (size_t i = block.begin; i < block.end; ++i) {
                            if (fn.icodes[i].result.is_temp() &&
                                fn.icodes[i].result.temp_id == base.temp_id) {
                                return false;
                            }
                        }
                    }
                    return true;
                }

                if (base.kind == operand_kind::SYMBOL) {
                    for (size_t block_id : loop.blocks) {
                        const auto &block = cfg.block(block_id);
                        for (size_t i = block.begin; i < block.end; ++i) {
                            if (same_symbol_slot(fn.icodes[i].result, base))
                                return false;
                        }
                    }
                    return true;
                }

                return false;
            };

            for (size_t block_id : loop.blocks) {
                const auto &block = cfg.block(block_id);
                for (size_t i = block.begin; i < block.end; ++i) {
                    const auto &ic = fn.icodes[i];
                    if (ic.op != icode_op::ADD || !ic.result.is_temp())
                        continue;

                    operand base;
                    std::optional<int64_t> stride;
                    if (is_pointer_base(ic.left)) {
                        stride = loop_index_scale(ic.right, iv_temp);
                        if (stride)
                            base = ic.left;
                    }
                    if (!stride && is_pointer_base(ic.right)) {
                        stride = loop_index_scale(ic.left, iv_temp);
                        if (stride)
                            base = ic.right;
                    }
                    if (base.is_none() || !stride || *stride <= 0 || *stride > 256) {
                        continue;
                    }

                    if (!loop_only_temp(ic.result.temp_id))
                        continue;
                    if (!base_stable_in_loop(base))
                        continue;

                    std::string key = base_key(base) + "|stride=" +
                                      std::to_string(*stride);
                    auto &group = groups[key];
                    if (group.base.is_none()) {
                        group.base = base;
                        group.ptr_type = pointer_type_for_base(base);
                        group.stride = *stride;
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
                init.op =
                    (group.base.kind == operand_kind::LABEL_REF ||
                     (group.base.type && group.base.type->is_ptr()))
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
                bump.right = operand::make_int(group.stride, type::make_int());
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

class tail_recursion_elim_pass final : public ir_pass {
public:
    const char *name() const override { return "tail_recursion_elim"; }

    bool run(ir_function &fn) override {
        if (fn.is_variadic || fn.num_params < 0 || fn.icodes.size() < 3 ||
            fn.icodes.front().op != icode_op::FUNCTION ||
            fn.icodes.back().op != icode_op::ENDFUNCTION) {
            return false;
        }

        // Reusing an activation record is not observable for ordinary scalar
        // locals, but it can be observable when an automatic object's address
        // escapes. Dynamic stack state and inline assembly have the same
        // problem, so leave those functions recursive.
        bool has_local_address = false;
        for (const auto &ic : fn.icodes) {
            if (ic.op == icode_op::ALLOCA || ic.op == icode_op::INLINE_ASM)
                return false;
            if (ic.op == icode_op::ADDRESS_OF && ic.left.is_symbol() &&
                !ic.left.is_global) {
                has_local_address = true;
            }
        }
        if (has_local_address && !fn.tail_local_addresses_noescape)
            return false;

        std::vector<operand> params(
            static_cast<size_t>(fn.num_params), operand::make_none());
        size_t entry_index = 1;
        while (entry_index + 1 < fn.icodes.size() &&
               fn.icodes[entry_index].op == icode_op::RECEIVE) {
            const icode &receive = fn.icodes[entry_index];
            if (receive.argreg < 0 || receive.argreg >= fn.num_params)
                return false;
            params[static_cast<size_t>(receive.argreg)] = receive.result;
            ++entry_index;
        }
        for (const operand &param : params) {
            if (param.is_none())
                return false;
        }

        size_t call_index = fn.icodes.size();
        size_t skip_end = fn.icodes.size();
        for (size_t i = entry_index; i + 1 < fn.icodes.size(); ++i) {
            const icode &call = fn.icodes[i];
            if (call.op != icode_op::CALL || call.func_name != fn.name ||
                call.num_params != fn.num_params) {
                continue;
            }

            const bool implicit_void_tail =
                call.result.is_none() &&
                i + 1 < fn.icodes.size() &&
                fn.icodes[i + 1].op == icode_op::ENDFUNCTION &&
                fn.ret_type && fn.ret_type->kind == type_kind::VOID;

            bool explicit_tail = false;
            if (i + 2 < fn.icodes.size() &&
                fn.icodes[i + 1].op == icode_op::RETURN &&
                fn.icodes[i + 2].op == icode_op::ENDFUNCTION) {
                const operand &ret = fn.icodes[i + 1].left;
                explicit_tail =
                    (call.result.is_none() && ret.is_none()) ||
                    (!call.result.is_none() &&
                     same_value_operand(call.result, ret));
            }

            if (!implicit_void_tail && !explicit_tail)
                continue;
            call_index = i;
            skip_end = i + (explicit_tail ? 2 : 1);
            break;
        }
        if (call_index == fn.icodes.size() ||
            call_index < static_cast<size_t>(fn.num_params)) {
            return false;
        }

        const size_t send_begin =
            call_index - static_cast<size_t>(fn.num_params);
        std::vector<operand> args(
            static_cast<size_t>(fn.num_params), operand::make_none());
        for (size_t i = send_begin; i < call_index; ++i) {
            const icode &send = fn.icodes[i];
            if (send.op != icode_op::SEND || send.argreg < 0 ||
                send.argreg >= fn.num_params) {
                return false;
            }
            args[static_cast<size_t>(send.argreg)] = send.left;
        }
        for (const operand &arg : args) {
            if (arg.is_none())
                return false;
        }

        std::unordered_set<std::string> labels;
        for (const auto &ic : fn.icodes) {
            if (ic.op == icode_op::LABEL)
                labels.insert(ic.label_name);
        }
        std::string entry_label = "__xcc_tailrec_" + fn.name;
        for (int suffix = 0; labels.count(entry_label) != 0; ++suffix)
            entry_label = "__xcc_tailrec_" + fn.name + "_" +
                          std::to_string(suffix);

        int next_temp = next_temp_id(fn);
        std::vector<icode> parallel_values;
        std::vector<icode> commits;
        for (size_t i = 0; i < params.size(); ++i) {
            if (same_value_operand(params[i], args[i]))
                continue;

            type_ptr value_type = params[i].type ? params[i].type : args[i].type;
            operand saved = make_fresh_temp(next_temp, value_type);

            icode save;
            save.op = icode_op::ASSIGN;
            save.result = saved;
            save.left = args[i];
            save.line = fn.icodes[call_index].line;
            parallel_values.push_back(std::move(save));

            icode commit;
            commit.op = icode_op::ASSIGN;
            commit.result = params[i];
            commit.left = saved;
            commit.line = fn.icodes[call_index].line;
            commits.push_back(std::move(commit));
        }

        icode backedge;
        backedge.op = icode_op::GOTO;
        backedge.label_name = entry_label;
        backedge.line = fn.icodes[call_index].line;

        std::vector<icode> rewritten;
        rewritten.reserve(fn.icodes.size() + parallel_values.size() +
                          commits.size() + 2);
        for (size_t i = 0; i < fn.icodes.size();) {
            if (i == entry_index) {
                icode label;
                label.op = icode_op::LABEL;
                label.label_name = entry_label;
                rewritten.push_back(std::move(label));
            }
            if (i == send_begin) {
                rewritten.insert(rewritten.end(), parallel_values.begin(),
                                 parallel_values.end());
                rewritten.insert(rewritten.end(), commits.begin(), commits.end());
                rewritten.push_back(backedge);
                i = skip_end;
                continue;
            }
            rewritten.push_back(fn.icodes[i]);
            ++i;
        }

        fn.icodes = std::move(rewritten);
        return true;
    }
};

class ifx_bool_wrapper_elide_pass final : public ir_pass {
public:
    const char *name() const override { return "ifx_bool_wrapper_elide"; }

    bool run(ir_function &fn) override {
        std::unordered_map<int, int> use_counts;
        std::unordered_map<int, std::vector<const icode *>> temp_defs;
        for (const auto &ic : fn.icodes) {
            for_each_use_operand(ic, [&](const operand &op) {
                if (op.is_temp())
                    ++use_counts[op.temp_id];
            });
            if (ic.result.is_temp())
                temp_defs[ic.result.temp_id].push_back(&ic);
        }

        std::function<bool(const operand &, int,
                           std::unordered_set<int> &)> is_boolean_value;
        is_boolean_value = [&](const operand &value, int depth,
                               std::unordered_set<int> &visiting) {
            if (value.type && value.type->kind == type_kind::BOOL)
                return true;
            if (value.kind == operand_kind::INT_CONST)
                return value.ival == 0 || value.ival == 1;
            if (!value.is_temp() || depth > 8 || visiting.count(value.temp_id))
                return false;

            auto defs = temp_defs.find(value.temp_id);
            if (defs == temp_defs.end() || defs->second.empty())
                return false;

            visiting.insert(value.temp_id);
            for (const icode *def : defs->second) {
                if (is_compare_op(def->op))
                    continue;
                if ((def->op == icode_op::ASSIGN || def->op == icode_op::CAST) &&
                    is_boolean_value(def->left, depth + 1, visiting)) {
                    continue;
                }
                visiting.erase(value.temp_id);
                return false;
            }
            visiting.erase(value.temp_id);
            return true;
        };

        auto is_boolean_value_root = [&](const operand &value) {
            std::unordered_set<int> visiting;
            return is_boolean_value(value, 0, visiting);
        };

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
            if (ic.op == icode_op::NE && other == 0)
                return std::make_pair(source, true);
            if (ic.op == icode_op::EQ && other == 0)
                return std::make_pair(source, false);
            if (other == 1 && is_boolean_value_root(source)) {
                return std::make_pair(source, ic.op == icode_op::EQ);
            }
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

class branch_bool_arithmetic_pass final : public ir_pass {
public:
    const char *name() const override { return "branch_bool_arithmetic"; }

    bool run(ir_function &fn) override {
        if (fn.icodes.size() < 2)
            return false;

        std::unordered_map<int, int> use_count;
        for (const icode &ic : fn.icodes) {
            for_each_use_operand(ic, [&](const operand &op) {
                if (op.is_temp())
                    ++use_count[op.temp_id];
            });
        }

        for (size_t i = 0; i + 1 < fn.icodes.size(); ++i) {
            const icode &compare = fn.icodes[i];
            const icode &adjust = fn.icodes[i + 1];
            if (!is_compare_opcode(compare.op) ||
                !compare.result.is_temp() ||
                use_count[compare.result.temp_id] != 1 ||
                (adjust.op != icode_op::ADD &&
                 adjust.op != icode_op::SUB) ||
                !adjust.result.type ||
                !adjust.result.type->is_integer() ||
                adjust.result.type->is_volatile) {
                continue;
            }

            operand base;
            bool matched = false;
            if (adjust.op == icode_op::ADD) {
                if (adjust.left.is_temp() &&
                    adjust.left.temp_id == compare.result.temp_id) {
                    base = adjust.right;
                    matched = true;
                } else if (adjust.right.is_temp() &&
                           adjust.right.temp_id == compare.result.temp_id) {
                    base = adjust.left;
                    matched = true;
                }
            } else if (adjust.right.is_temp() &&
                       adjust.right.temp_id == compare.result.temp_id) {
                base = adjust.left;
                matched = true;
            }
            if (!matched || !same_value_operand(adjust.result, base) ||
                (base.type && base.type->is_volatile)) {
                continue;
            }

            const std::string adjust_label =
                make_unique_label(fn, "__xcc_bool_adjust_");
            const std::string done_label =
                make_unique_label(fn, "__xcc_bool_adjust_done_");

            icode branch;
            branch.op = icode_op::IFX;
            branch.left = compare.result;
            branch.true_lbl = adjust_label;
            branch.false_lbl = done_label;
            branch.line = adjust.line;

            icode adjust_label_ic;
            adjust_label_ic.op = icode_op::LABEL;
            adjust_label_ic.label_name = adjust_label;
            adjust_label_ic.line = adjust.line;

            icode compact_adjust = adjust;
            compact_adjust.left = base;
            compact_adjust.right = operand::make_int(1, adjust.result.type);

            icode done_label_ic;
            done_label_ic.op = icode_op::LABEL;
            done_label_ic.label_name = done_label;
            done_label_ic.line = adjust.line;

            std::vector<icode> replacement;
            replacement.reserve(5);
            replacement.push_back(compare);
            replacement.push_back(std::move(branch));
            replacement.push_back(std::move(adjust_label_ic));
            replacement.push_back(std::move(compact_adjust));
            replacement.push_back(std::move(done_label_ic));

            fn.icodes.erase(fn.icodes.begin() +
                                static_cast<std::ptrdiff_t>(i),
                            fn.icodes.begin() +
                                static_cast<std::ptrdiff_t>(i + 2));
            fn.icodes.insert(fn.icodes.begin() +
                                 static_cast<std::ptrdiff_t>(i),
                             replacement.begin(), replacement.end());
            return true;
        }
        return false;
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

class widened_byte_operand_forward_pass final : public ir_pass {
public:
    const char *name() const override {
        return "widened_byte_operand_forward";
    }

    bool run(ir_function &fn) override {
        struct cast_candidate {
            size_t def_index = 0;
            operand widened;
            operand byte;
        };

        std::unordered_map<int, int> definition_counts;
        for (const auto &ic : fn.icodes) {
            if (ic.op != icode_op::SET_VALUE_AT && ic.result.is_temp())
                ++definition_counts[ic.result.temp_id];
        }

        std::vector<cast_candidate> candidates;
        for (size_t i = 0; i < fn.icodes.size(); ++i) {
            const icode &ic = fn.icodes[i];
            if (ic.op != icode_op::CAST || !ic.result.is_temp() ||
                definition_counts[ic.result.temp_id] != 1 ||
                !ic.result.type || ic.result.type->size() != 2 ||
                !ic.result.type->is_integer() || !ic.left.type ||
                ic.left.type->size() != 1 || !ic.left.type->is_integer() ||
                ic.result.type->is_volatile || ic.left.type->is_volatile) {
                continue;
            }
            candidates.push_back({i, ic.result, ic.left});
        }

        auto source_redefined_between = [&](const cast_candidate &candidate,
                                            size_t use_index) {
            if (use_index <= candidate.def_index)
                return true;
            for (size_t i = candidate.def_index + 1; i < use_index; ++i) {
                const operand &result = fn.icodes[i].result;
                if (candidate.byte.is_temp() && result.is_temp() &&
                    result.temp_id == candidate.byte.temp_id)
                    return true;
                if (candidate.byte.kind == operand_kind::SYMBOL &&
                    result.kind == operand_kind::SYMBOL &&
                    same_value_operand(result, candidate.byte))
                    return true;
            }
            return false;
        };

        bool changed = false;
        std::unordered_set<size_t> erase;
        for (const cast_candidate &candidate : candidates) {
            std::vector<std::pair<size_t, bool>> rewrites;
            bool safe = true;
            for (size_t i = candidate.def_index + 1;
                 i < fn.icodes.size(); ++i) {
                const icode &use = fn.icodes[i];
                const bool left_use =
                    use.left.is_temp() &&
                    use.left.temp_id == candidate.widened.temp_id;
                const bool right_use =
                    use.right.is_temp() &&
                    use.right.temp_id == candidate.widened.temp_id;
                if (!left_use && !right_use)
                    continue;
                if (left_use && right_use) {
                    safe = false;
                    break;
                }
                if (source_redefined_between(candidate, i)) {
                    safe = false;
                    break;
                }

                bool supported = false;
                switch (use.op) {
                case icode_op::EQ:
                case icode_op::NE:
                    supported = true;
                    break;
                case icode_op::LT:
                case icode_op::LE:
                case icode_op::GT:
                case icode_op::GE:
                    supported = right_use && use.left.type &&
                        use.left.type->size() == 2 &&
                        use.left.type->is_integer() &&
                        use.left.type->is_unsigned() ==
                            candidate.widened.type->is_unsigned();
                    break;
                case icode_op::ADD:
                case icode_op::SUB:
                case icode_op::BAND:
                case icode_op::BOR:
                case icode_op::BXOR:
                    supported = use.result.type &&
                        use.result.type->size() == 2 &&
                        use.result.type->is_integer();
                    break;
                default:
                    break;
                }
                if (!supported) {
                    safe = false;
                    break;
                }
                rewrites.push_back({i, left_use});
            }
            if (!safe || rewrites.empty())
                continue;
            for (const auto &[index, replace_left] : rewrites) {
                operand replacement = candidate.byte;
                const operand &old =
                    replace_left ? fn.icodes[index].left
                                 : fn.icodes[index].right;
                replacement.byte_offset += old.byte_offset;
                if (replace_left)
                    fn.icodes[index].left = replacement;
                else
                    fn.icodes[index].right = replacement;
            }
            // Every use of the widening CAST was just replaced, so consume
            // it here.  This pass is also used as a terminal backend-form
            // lowering, where no later DCE iteration is available.
            erase.insert(candidate.def_index);
            changed = true;
        }
        if (!erase.empty()) {
            std::vector<icode> out;
            out.reserve(fn.icodes.size() - erase.size());
            for (size_t i = 0; i < fn.icodes.size(); ++i) {
                if (!erase.count(i))
                    out.push_back(std::move(fn.icodes[i]));
            }
            fn.icodes = std::move(out);
        }
        return changed;
    }
};

class widened_unsigned_word_mul_pass final : public ir_pass {
public:
    const char *name() const override {
        return "widened_unsigned_word_mul";
    }

    bool run(ir_function &fn) override {
        std::unordered_map<int, size_t> definitions;
        std::unordered_map<int, int> definition_counts;
        std::unordered_map<int, int> use_counts;
        for (size_t i = 0; i < fn.icodes.size(); ++i) {
            const auto &ic = fn.icodes[i];
            if (ic.op != icode_op::SET_VALUE_AT && ic.result.is_temp()) {
                definitions[ic.result.temp_id] = i;
                ++definition_counts[ic.result.temp_id];
            }
            for_each_use_operand(ic, [&](const operand &op) {
                if (op.is_temp())
                    ++use_counts[op.temp_id];
            });
        }

        auto narrow_source = [&](const operand &op)
            -> std::optional<operand> {
            if (!op.is_temp() || !op.type || op.type->size() != 4 ||
                !op.type->is_integer() || !op.type->is_unsigned() ||
                definition_counts[op.temp_id] != 1 ||
                use_counts[op.temp_id] != 1) {
                return std::nullopt;
            }
            auto it = definitions.find(op.temp_id);
            if (it == definitions.end())
                return std::nullopt;
            const icode &cast = fn.icodes[it->second];
            if (cast.op != icode_op::CAST || !cast.left.type ||
                cast.left.type->size() != 2 ||
                !cast.left.type->is_integer() ||
                !cast.left.type->is_unsigned()) {
                return std::nullopt;
            }
            return cast.left;
        };

        bool changed = false;
        std::unordered_set<size_t> erase;
        for (auto &ic : fn.icodes) {
            if (ic.op != icode_op::MUL || !ic.result.type ||
                ic.result.type->size() != 4 ||
                !ic.result.type->is_integer() ||
                !ic.result.type->is_unsigned()) {
                continue;
            }
            auto left = narrow_source(ic.left);
            auto right = narrow_source(ic.right);
            if (!left || !right)
                continue;
            const size_t left_cast = definitions[ic.left.temp_id];
            const size_t right_cast = definitions[ic.right.temp_id];
            ic.left = *left;
            ic.right = *right;
            erase.insert(left_cast);
            erase.insert(right_cast);
            changed = true;
        }
        if (!erase.empty()) {
            std::vector<icode> out;
            out.reserve(fn.icodes.size() - erase.size());
            for (size_t i = 0; i < fn.icodes.size(); ++i) {
                if (!erase.count(i))
                    out.push_back(std::move(fn.icodes[i]));
            }
            fn.icodes = std::move(out);
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
            if (fn.icodes[end_i - 1].op != icode_op::RETURN)
                continue;

            for (size_t bj = bi + 1; bj < cfg.blocks().size(); ++bj) {
                const auto &block_j = cfg.block(bj);
                size_t first_j = first_non_label_index(fn, block_j);
                size_t end_j = logical_block_end_for_tail_compare(fn, cfg, block_j);
                if (first_j >= block_j.end ||
                    first_j >= end_j ||
                    fn.icodes[first_j].op == icode_op::FUNCTION) {
                    continue;
                }
                if (fn.icodes[end_j - 1].op != icode_op::RETURN)
                    continue;

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

        std::unordered_set<int> shared_defs;
        std::vector<int> shared_live_in_order;
        std::unordered_set<int> shared_live_in_seen;
        for (const auto &ic : shared_tail) {
            for_each_use_operand(ic, [&](const operand &op) {
                if (!op.is_temp() || shared_defs.count(op.temp_id))
                    return;
                if (shared_live_in_seen.insert(op.temp_id).second)
                    shared_live_in_order.push_back(op.temp_id);
            });
            if (icode_defines_result_temp(ic))
                shared_defs.insert(ic.result.temp_id);
        }

        int next_temp = next_temp_id(fn);
        std::vector<icode> bridge_prefix;
        std::vector<icode> bridge_copies;
        for (int canon_temp : shared_live_in_order) {
            int other_temp = -1;
            for (const auto &[mapped_other, mapped_canon] :
                 best->other_to_canon_temps) {
                if (mapped_canon == canon_temp) {
                    other_temp = mapped_other;
                    break;
                }
            }
            if (other_temp < 0 || other_temp == canon_temp)
                continue;

            type_ptr bridge_type = temp_type_in_function(fn, canon_temp);
            if (!bridge_type)
                bridge_type = temp_type_in_function(fn, other_temp);
            operand saved = make_fresh_temp(next_temp, bridge_type);

            icode save_ic;
            save_ic.op = icode_op::ASSIGN;
            save_ic.result = saved;
            save_ic.left = operand::make_temp(other_temp, bridge_type);

            icode copy_ic;
            copy_ic.op = icode_op::ASSIGN;
            copy_ic.result = operand::make_temp(canon_temp, bridge_type);
            copy_ic.left = saved;

            bridge_prefix.push_back(std::move(save_ic));
            bridge_copies.push_back(std::move(copy_ic));
        }

        auto replacement_sequence = [&](size_t start,
                                       const std::vector<icode> *prefix)
            -> std::vector<icode> {
            std::vector<icode> seq;
            if (prefix) {
                seq.insert(seq.end(), prefix->begin(), prefix->end());
                seq.insert(seq.end(), bridge_copies.begin(), bridge_copies.end());
            }

            icode goto_ic;
            goto_ic.op = icode_op::GOTO;
            goto_ic.label_name = tail_label;
            if (start < fn.icodes.size())
                goto_ic.line = fn.icodes[start].line;
            seq.push_back(std::move(goto_ic));
            return seq;
        };

        auto replace_with_sequence = [&](size_t start,
                                         size_t end,
                                         const std::vector<icode> *prefix) {
            auto erase_begin =
                fn.icodes.begin() + static_cast<std::ptrdiff_t>(start);
            auto erase_end =
                fn.icodes.begin() + static_cast<std::ptrdiff_t>(end);
            fn.icodes.erase(erase_begin, erase_end);
            auto seq = replacement_sequence(start, prefix);
            fn.icodes.insert(
                fn.icodes.begin() + static_cast<std::ptrdiff_t>(start),
                seq.begin(), seq.end());
        };

        replace_with_sequence(best->other_split, best->other_end, &bridge_prefix);
        replace_with_sequence(best->canon_split, best->canon_end, nullptr);

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

        // Large lowered switch dispatchers create high-fanout cyclic CFGs.
        // The current symbol-value merge is not conservative enough for
        // those graphs (notably VM dispatch loops), so leave them to the
        // temp-only propagation pass below.
        const size_t branch_count = static_cast<size_t>(std::count_if(
            fn.icodes.begin(), fn.icodes.end(), [](const icode &ic) {
                return ic.op == icode_op::IFX;
            }));
        if (branch_count > 8)
            return false;

        alias_info alias = build_alias_info(fn);
        control_flow_graph cfg(fn);
        auto reachable = cfg.reachable_blocks();
        auto order = cfg.reverse_postorder();

        std::unordered_map<std::string, operand> operand_bank;
        for (auto &ic : fn.icodes) {
            auto remember = [&](const operand &op) {
                std::string key = trackable_key(op, alias);
                if (key.empty())
                    return;
                auto it = operand_bank.find(key);
                if (it == operand_bank.end()) {
                    operand_bank.emplace(key, op);
                    return;
                }
                if (op.kind != operand_kind::SYMBOL)
                    return;
                const int cur_size =
                    it->second.type && it->second.type->size() > 0
                        ? it->second.type->size()
                        : std::numeric_limits<int>::max();
                const int new_size =
                    op.type && op.type->size() > 0
                        ? op.type->size()
                        : std::numeric_limits<int>::max();
                if (new_size < cur_size)
                    it->second = op;
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

// Propagate only integer constants held in compiler temporaries.  Unlike the
// broader value-propagation pass, this analysis never treats C objects as SSA
// values, so calls and aliased stores cannot invalidate its facts.  This still
// catches common accumulator initialization such as `sum = 0; call();
// sum += value`, after which algebraic simplification can remove the add.
class temp_constant_propagation_pass final : public ir_pass {
public:
    const char *name() const override { return "temp_constant_propagation"; }

    bool run(ir_function &fn) override {
        if (fn.icodes.empty())
            return false;

        control_flow_graph cfg(fn);
        const auto reachable = cfg.reachable_blocks();
        const auto order = cfg.reverse_postorder();
        const alias_info no_aliases;

        auto rewrite_known_temp = [](operand &op, const ssa_env &env) {
            if (!op.is_temp())
                return false;
            const auto found = env.find("t:" + std::to_string(op.temp_id));
            if (found == env.end() ||
                found->second.tag != ssa_value::kind::INT_CONST) {
                return false;
            }

            int64_t value = found->second.ival;
            if (op.byte_offset != 0) {
                if (op.byte_offset < 0 || op.byte_offset > 7)
                    return false;
                const unsigned shift =
                    static_cast<unsigned>(op.byte_offset * 8);
                value = static_cast<int64_t>(
                    (static_cast<uint64_t>(value) >> shift) & 0xffu);
            }
            if (op.type)
                value = cast_int_value(value, op.type);
            op = operand::make_int(value, op.type);
            return true;
        };

        auto transfer = [&](const basic_block &block, const ssa_env &start) {
            ssa_env env = start;
            for (size_t i = block.begin; i < block.end; ++i) {
                icode evaluated = fn.icodes[i];
                for_each_use_operand(evaluated, [&](operand &op) {
                    rewrite_known_temp(op, env);
                });

                if (!defines_result(evaluated) ||
                    !evaluated.result.is_temp()) {
                    continue;
                }

                const std::string key =
                    "t:" + std::to_string(evaluated.result.temp_id);
                if (evaluated.result.byte_offset != 0) {
                    env.erase(key);
                    continue;
                }

                const ssa_value value = evaluate_defined_value(
                    evaluated, i, env, no_aliases);
                if (value.tag == ssa_value::kind::INT_CONST)
                    env[key] = value;
                else
                    env.erase(key);
            }
            return env;
        };

        std::vector<ssa_env> in(cfg.blocks().size());
        std::vector<ssa_env> out(cfg.blocks().size());
        bool dataflow_changed = true;
        while (dataflow_changed) {
            dataflow_changed = false;
            for (size_t block_id : order) {
                if (!reachable.count(block_id))
                    continue;

                ssa_env merged;
                bool have_pred = false;
                const basic_block &block = cfg.block(block_id);
                for (size_t pred : block.preds) {
                    if (!reachable.count(pred))
                        continue;
                    if (!have_pred) {
                        merged = out[pred];
                        have_pred = true;
                        continue;
                    }
                    for (auto it = merged.begin(); it != merged.end();) {
                        const auto found = out[pred].find(it->first);
                        if (found == out[pred].end() ||
                            found->second != it->second) {
                            it = merged.erase(it);
                        } else {
                            ++it;
                        }
                    }
                }
                if (!have_pred)
                    merged.clear();

                if (!env_equal(in[block_id], merged)) {
                    in[block_id] = merged;
                    dataflow_changed = true;
                }
                ssa_env new_out = transfer(block, in[block_id]);
                if (!env_equal(out[block_id], new_out)) {
                    out[block_id] = std::move(new_out);
                    dataflow_changed = true;
                }
            }
        }

        bool changed = false;
        for (size_t block_id : order) {
            if (!reachable.count(block_id))
                continue;
            ssa_env env = in[block_id];
            const basic_block &block = cfg.block(block_id);
            for (size_t i = block.begin; i < block.end; ++i) {
                icode &ic = fn.icodes[i];
                for_each_use_operand(ic, [&](operand &op) {
                    changed |= rewrite_known_temp(op, env);
                });

                if (!defines_result(ic) || !ic.result.is_temp())
                    continue;
                const std::string key =
                    "t:" + std::to_string(ic.result.temp_id);
                if (ic.result.byte_offset != 0) {
                    env.erase(key);
                    continue;
                }
                const ssa_value value = evaluate_defined_value(
                    ic, i, env, no_aliases);
                if (value.tag == ssa_value::kind::INT_CONST)
                    env[key] = value;
                else
                    env.erase(key);
            }
        }
        return changed;
    }
};

static bool is_rematerializable_global_scalar(const operand &op,
                                              const alias_info &alias) {
    if (!op.is_symbol() || !op.is_global || op.is_param || op.is_func ||
        op.is_tls || op.is_sfr || op.byte_offset != 0) {
        return false;
    }
    if (!op.type || !op.type->is_scalar())
        return false;
    if (op.type->size() <= 0 || op.type->size() > 2)
        return false;
    return !base_symbol_address_taken(alias, op);
}

static bool global_remat_replacement_compatible(const operand &use,
                                                const operand &replacement) {
    if (!use.type || !replacement.type)
        return true;
    if (!use.type->is_scalar() || !replacement.type->is_scalar())
        return false;
    if (use.type->size() != replacement.type->size())
        return false;
    if (use.type->is_ptr() != replacement.type->is_ptr())
        return false;
    return true;
}

static bool global_remat_maps_equal(
        const std::unordered_map<int, operand> &lhs,
        const std::unordered_map<int, operand> &rhs) {
    if (lhs.size() != rhs.size())
        return false;
    for (const auto &it : lhs) {
        auto found = rhs.find(it.first);
        if (found == rhs.end() ||
            !same_value_operand(it.second, found->second)) {
            return false;
        }
    }
    return true;
}

class global_scalar_remat_pass final : public ir_pass {
public:
    const char *name() const override { return "global_scalar_remat"; }

    bool run(ir_function &fn) override {
        if (fn.icodes.empty())
            return false;

        alias_info alias = build_alias_info(fn);
        std::unordered_map<int, int> temp_use_count;
        for (const auto &ic : fn.icodes) {
            for_each_use_operand(ic, [&](const operand &op) {
                if (op.is_temp())
                    ++temp_use_count[op.temp_id];
            });
        }

        auto direct_global_load = [&](const icode &ic, operand &global) {
            if (ic.op != icode_op::ASSIGN || !ic.result.is_temp() ||
                !is_rematerializable_global_scalar(ic.left, alias)) {
                return false;
            }
            auto count_it = temp_use_count.find(ic.result.temp_id);
            if (count_it == temp_use_count.end() || count_it->second != 1)
                return false;
            global = ic.left;
            return true;
        };

        auto kills_global_values = [](const icode &ic) {
            // Copy-sink fusion can turn `tmp = global + c; global = tmp`
            // into an arithmetic instruction whose destination is the global
            // itself.  Any defining instruction, not merely ASSIGN, therefore
            // advances the memory version and invalidates saved snapshots of
            // globals (notably the old value required by `*cursor++`).
            if (defines_result(ic) && ic.result.is_symbol() &&
                (ic.result.is_global || ic.result.is_tls || ic.result.is_sfr)) {
                return true;
            }
            switch (ic.op) {
            case icode_op::CALL:
            case icode_op::SET_VALUE_AT:
            case icode_op::BLOCK_FILL:
            case icode_op::INLINE_ASM:
            case icode_op::ALLOCA:
                return true;
            default:
                return false;
            }
        };

        auto transfer = [&](const basic_block &block,
                            const std::unordered_map<int, operand> &start) {
            std::unordered_map<int, operand> current = start;
            for (size_t i = block.begin; i < block.end; ++i) {
                const icode &ic = fn.icodes[i];

                if (kills_global_values(ic))
                    current.clear();

                if (defines_result(ic) && ic.result.is_temp())
                    current.erase(ic.result.temp_id);

                operand global;
                if (direct_global_load(ic, global))
                    current[ic.result.temp_id] = global;
            }
            return current;
        };

        control_flow_graph cfg(fn);
        auto reachable = cfg.reachable_blocks();
        auto order = cfg.reverse_postorder();
        const size_t nblocks = cfg.blocks().size();

        std::vector<std::unordered_map<int, operand>> in(nblocks), out(nblocks);
        bool dataflow_changed = true;
        while (dataflow_changed) {
            dataflow_changed = false;
            for (size_t block_id : order) {
                if (!reachable.count(block_id))
                    continue;

                std::unordered_map<int, operand> merged;
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
                            !same_value_operand(it->second, found->second)) {
                            it = merged.erase(it);
                        } else {
                            ++it;
                        }
                    }
                }
                if (!have_pred)
                    merged.clear();

                if (!global_remat_maps_equal(in[block_id], merged)) {
                    in[block_id] = merged;
                    dataflow_changed = true;
                }

                auto new_out = transfer(block, in[block_id]);
                if (!global_remat_maps_equal(out[block_id], new_out)) {
                    out[block_id] = std::move(new_out);
                    dataflow_changed = true;
                }
            }
        }

        bool changed = false;
        for (size_t block_id : order) {
            if (!reachable.count(block_id))
                continue;

            std::unordered_map<int, operand> current = in[block_id];
            const auto &block = cfg.block(block_id);
            for (size_t i = block.begin; i < block.end; ++i) {
                icode &ic = fn.icodes[i];

                for_each_use_operand(ic, [&](operand &op) {
                    if (!op.is_temp())
                        return;
                    auto found = current.find(op.temp_id);
                    if (found == current.end())
                        return;
                    if (!global_remat_replacement_compatible(op, found->second))
                        return;
                    operand replacement = found->second;
                    if (op.type)
                        replacement.type = op.type;
                    op = replacement;
                    changed = true;
                });

                if (kills_global_values(ic))
                    current.clear();

                if (defines_result(ic) && ic.result.is_temp())
                    current.erase(ic.result.temp_id);

                operand global;
                if (direct_global_load(ic, global))
                    current[ic.result.temp_id] = global;
            }
        }

        return changed;
    }
};

class immutable_const_temp_remat_pass final : public ir_pass {
public:
    const char *name() const override { return "immutable_const_temp_remat"; }

    bool run(ir_function &fn) override {
        if (fn.icodes.empty())
            return false;

        control_flow_graph cfg(fn);
        std::vector<size_t> block_for_index(fn.icodes.size(),
                                            static_cast<size_t>(-1));
        for (const auto &block : cfg.blocks()) {
            for (size_t i = block.begin; i < block.end && i < fn.icodes.size(); ++i)
                block_for_index[i] = block.id;
        }

        const auto dom =
            (cfg.blocks().size() <= control_flow_graph::kMaxLoopOptBlocks)
                ? cfg.dominators()
                : std::vector<std::unordered_set<size_t>>{};

        auto dominates = [&](size_t def_idx, size_t use_idx) {
            if (def_idx >= block_for_index.size() ||
                use_idx >= block_for_index.size()) {
                return false;
            }
            const size_t def_block = block_for_index[def_idx];
            const size_t use_block = block_for_index[use_idx];
            if (def_block == static_cast<size_t>(-1) ||
                use_block == static_cast<size_t>(-1)) {
                return false;
            }
            if (def_block == use_block)
                return use_idx > def_idx;
            return !dom.empty() &&
                   use_block < dom.size() &&
                   dom[use_block].count(def_block) != 0;
        };

        std::unordered_map<int, int> temp_def_count;
        std::unordered_map<int, size_t> temp_def_index;
        std::unordered_map<int, std::vector<size_t>> temp_use_indices;

        for (size_t i = 0; i < fn.icodes.size(); ++i) {
            const auto &ic = fn.icodes[i];
            if (ic.result.is_temp() && defines_result(ic)) {
                ++temp_def_count[ic.result.temp_id];
                temp_def_index[ic.result.temp_id] = i;
            }
            for_each_use_operand(ic, [&](const operand &op) {
                if (op.is_temp())
                    temp_use_indices[op.temp_id].push_back(i);
            });
        }

        auto candidate_const = [&](const icode &ic) -> std::optional<operand> {
            if (!ic.result.is_temp() ||
                (ic.op != icode_op::ASSIGN && ic.op != icode_op::CAST) ||
                !ic.right.is_none() ||
                ic.left.kind != operand_kind::INT_CONST) {
                return std::nullopt;
            }
            if (ic.op == icode_op::CAST &&
                ic.left.type && ic.result.type &&
                (ic.left.type->is_far_ptr() || ic.result.type->is_far_ptr())) {
                return std::nullopt;
            }

            type_ptr target = ic.result.type ? ic.result.type : ic.left.type;
            if (ic.op == icode_op::CAST && target &&
                (target->kind == type_kind::FLOAT ||
                 target->kind == type_kind::DOUBLE)) {
                double value;
                if (ic.left.type && ic.left.type->is_unsigned()) {
                    uint64_t raw = static_cast<uint64_t>(ic.left.ival);
                    const int bytes = ic.left.type->size();
                    if (bytes > 0 && bytes < 8)
                        raw &= (uint64_t{1} << (bytes * 8)) - 1;
                    value = static_cast<double>(raw);
                } else {
                    value = static_cast<double>(
                        cast_int_value(ic.left.ival, ic.left.type));
                }
                return operand::make_float(value, target);
            }
            if (ic.op == icode_op::CAST &&
                !cast_can_fold_to_int_const(target)) {
                return std::nullopt;
            }
            operand repl = operand::make_int(cast_int_value(ic.left.ival, target),
                                             target);
            return repl;
        };

        auto replacement_for_use =
            [&](const operand &base, const operand &use) -> std::optional<operand> {
                operand repl = base;
                if (base.kind == operand_kind::FLOAT_CONST) {
                    if (use.byte_offset != 0 ||
                        (use.type && use.type->kind != type_kind::FLOAT &&
                         use.type->kind != type_kind::DOUBLE)) {
                        return std::nullopt;
                    }
                    if (use.type)
                        repl.type = use.type;
                    return repl;
                }
                if (use.byte_offset != 0) {
                    if (use.byte_offset < 0 || use.byte_offset > 7)
                        return std::nullopt;
                    const unsigned shift =
                        static_cast<unsigned>(use.byte_offset * 8);
                    const uint64_t bits = static_cast<uint64_t>(base.ival);
                    repl.ival = static_cast<int64_t>((bits >> shift) & 0xffu);
                    repl.type = type::make_uchar();
                    repl.byte_offset = 0;
                    return repl;
                }

                if (use.type) {
                    repl.ival = cast_int_value(repl.ival, use.type);
                    repl.type = use.type;
                }
                repl.byte_offset = 0;
                return repl;
            };

        std::vector<bool> erase(fn.icodes.size(), false);
        bool changed = false;

        for (const auto &[temp_id, def_idx] : temp_def_index) {
            if (temp_def_count[temp_id] != 1)
                continue;

            const auto repl_opt = candidate_const(fn.icodes[def_idx]);
            if (!repl_opt)
                continue;

            auto uses_it = temp_use_indices.find(temp_id);
            if (uses_it == temp_use_indices.end() || uses_it->second.empty())
                continue;

            bool can_replace = true;
            for (size_t use_idx : uses_it->second) {
                if (!dominates(def_idx, use_idx)) {
                    can_replace = false;
                    break;
                }

                bool use_ok = true;
                for_each_use_operand(fn.icodes[use_idx], [&](const operand &op) {
                    if (!use_ok || !op.is_temp() || op.temp_id != temp_id)
                        return;
                    if (!replacement_for_use(*repl_opt, op))
                        use_ok = false;
                });
                if (!use_ok) {
                    can_replace = false;
                    break;
                }
            }

            if (!can_replace)
                continue;

            for (size_t use_idx : uses_it->second) {
                for_each_use_operand(fn.icodes[use_idx], [&](operand &op) {
                    if (!op.is_temp() || op.temp_id != temp_id)
                        return;
                    auto replacement = replacement_for_use(*repl_opt, op);
                    if (replacement)
                        op = *replacement;
                });
            }

            erase[def_idx] = true;
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

class calc_temp_fusion_pass final : public ir_pass {
public:
    const char *name() const override { return "calc_temp_fusion"; }

    bool run(ir_function &fn) override {
        if (fn.icodes.empty())
            return false;

        control_flow_graph cfg(fn);

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
                    const int sink_size =
                        consumer.result.type
                            ? consumer.result.type->size()
                            : (producer.result.type
                                   ? producer.result.type->size()
                                   : 0);
                    const bool induction_update =
                        consumer.result.is_temp() &&
                        producer.left.is_temp() &&
                        producer.left.temp_id == consumer.result.temp_id &&
                        (producer.op == icode_op::ADD ||
                         producer.op == icode_op::SUB) &&
                        producer.right.kind == operand_kind::INT_CONST;
                    if (induction_update && sink_size != 1)
                        continue;
                    if (cfg.blocks().size() != 1 &&
                        (sink_size <= 0 || sink_size > 2)) {
                        continue;
                    }
                    // Writing a symbol earlier is only safe for the canonical
                    // adjacent `temp = expr; sink = temp` lowering. Temp-to-
                    // temp fusion may span other pure instructions because it
                    // changes only the virtual name, not observable state.
                    if (!consumer.result.is_temp() && consumer_idx != i + 1)
                        continue;
                    if (consumer.result.is_temp()) {
                        bool old_sink_used_between = false;
                        auto sink_uses = temp_use_indices.find(
                            consumer.result.temp_id);
                        if (sink_uses != temp_use_indices.end()) {
                            for (size_t use_idx : sink_uses->second) {
                                if (use_idx > i && use_idx < consumer_idx &&
                                    !erase[use_idx]) {
                                    old_sink_used_between = true;
                                    break;
                                }
                            }
                        }
                        for (size_t k = i + 1;
                             !old_sink_used_between && k < consumer_idx; ++k) {
                            if (!erase[k] && defines_result(fn.icodes[k]) &&
                                fn.icodes[k].result.is_temp() &&
                                fn.icodes[k].result.temp_id ==
                                    consumer.result.temp_id) {
                                old_sink_used_between = true;
                            }
                        }
                        if (old_sink_used_between)
                            continue;
                    }
                    producer.result = consumer.result;
                    erase[consumer_idx] = true;
                    changed = true;
                    continue;
                }

                // Destructive temp-to-temp coalescing changes the producer's
                // virtual destination. Keep that broader transformation on
                // the original straight-line domain; branchy functions use
                // only the adjacent copy-sink case above.
                if (cfg.blocks().size() != 1)
                    continue;

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

class branch_copy_sink_coalesce_pass final : public ir_pass {
public:
    const char *name() const override { return "branch_copy_sink_coalesce"; }

    bool run(ir_function &fn) override {
        if (fn.icodes.empty())
            return false;

        control_flow_graph cfg(fn);
        std::vector<size_t> block_for_index(
            fn.icodes.size(), static_cast<size_t>(-1));
        for (const auto &block : cfg.blocks()) {
            for (size_t i = block.begin; i < block.end; ++i)
                block_for_index[i] = block.id;
        }

        std::unordered_map<int, std::vector<size_t>> temp_defs;
        std::unordered_map<int, std::vector<size_t>> temp_uses;
        for (size_t i = 0; i < fn.icodes.size(); ++i) {
            const auto &ic = fn.icodes[i];
            if (defines_result(ic) && ic.result.is_temp())
                temp_defs[ic.result.temp_id].push_back(i);
            for_each_use_operand(ic, [&](const operand &op) {
                if (op.is_temp())
                    temp_uses[op.temp_id].push_back(i);
            });
        }

        auto scalar_low_byte = [](const type_ptr &ty) {
            return ty && (ty->is_integer() || ty->is_ptr()) &&
                   !ty->is_volatile && ty->size() > 0 && ty->size() <= 2;
        };
        auto truncating_def_safe = [&](const icode &ic, int phi_temp) {
            if (!ic.result.is_temp() || ic.result.temp_id != phi_temp ||
                !scalar_low_byte(ic.result.type)) {
                return false;
            }
            switch (ic.op) {
            case icode_op::ASSIGN:
            case icode_op::CAST:
                return ic.right.is_none() && scalar_low_byte(ic.left.type);
            case icode_op::ADD:
            case icode_op::SUB:
            case icode_op::MUL:
            case icode_op::BAND:
            case icode_op::BOR:
            case icode_op::BXOR:
                return scalar_low_byte(ic.left.type) &&
                       scalar_low_byte(ic.right.type);
            case icode_op::NEG:
            case icode_op::BNOT:
                return ic.right.is_none() && scalar_low_byte(ic.left.type);
            case icode_op::SHL:
                return scalar_low_byte(ic.left.type) &&
                       ic.right.kind == operand_kind::INT_CONST &&
                       ic.right.ival >= 0 && ic.right.ival <= 7;
            default:
                return false;
            }
        };

        std::vector<bool> erase(fn.icodes.size(), false);
        bool changed = false;

        for (const auto &join : cfg.blocks()) {
            if (join.preds.size() < 2)
                continue;

            size_t copy_idx = join.begin;
            while (copy_idx < join.end &&
                   fn.icodes[copy_idx].op == icode_op::LABEL) {
                ++copy_idx;
            }
            if (copy_idx >= join.end || erase[copy_idx])
                continue;

            const icode &copy = fn.icodes[copy_idx];
            if (copy.op != icode_op::ASSIGN || !copy.result.is_temp() ||
                !copy.left.is_temp() || !copy.right.is_none() ||
                !copy.result.type || !copy.result.type->is_integer() ||
                copy.result.type->kind == type_kind::BOOL ||
                copy.result.type->is_volatile ||
                copy.result.type->size() != 1) {
                continue;
            }

            const int phi_temp = copy.left.temp_id;
            auto uses = temp_uses.find(phi_temp);
            auto defs = temp_defs.find(phi_temp);
            if (uses == temp_uses.end() || uses->second.size() != 1 ||
                uses->second.front() != copy_idx || defs == temp_defs.end() ||
                defs->second.size() != join.preds.size()) {
                continue;
            }

            std::vector<size_t> predecessor_defs;
            bool safe = true;
            for (size_t pred_id : join.preds) {
                const auto &pred = cfg.block(pred_id);
                size_t pred_def = fn.icodes.size();
                for (size_t def_idx : defs->second) {
                    if (block_for_index[def_idx] != pred_id)
                        continue;
                    if (pred_def != fn.icodes.size()) {
                        safe = false;
                        break;
                    }
                    pred_def = def_idx;
                }
                if (!safe || pred_def == fn.icodes.size() ||
                    !truncating_def_safe(fn.icodes[pred_def], phi_temp)) {
                    safe = false;
                    break;
                }
                for (size_t i = pred_def + 1; i < pred.end; ++i) {
                    if (fn.icodes[i].op != icode_op::GOTO) {
                        safe = false;
                        break;
                    }
                }
                if (!safe)
                    break;
                predecessor_defs.push_back(pred_def);
            }
            if (!safe || predecessor_defs.size() != join.preds.size())
                continue;

            for (size_t def_idx : predecessor_defs)
                fn.icodes[def_idx].result = copy.result;
            erase[copy_idx] = true;
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

class noop_temp_assign_elide_pass final : public ir_pass {
public:
    const char *name() const override { return "noop_temp_assign_elide"; }

    bool run(ir_function &fn) override {
        control_flow_graph cfg(fn);
        std::vector<bool> erase(fn.icodes.size(), false);
        bool changed = false;

        std::vector<size_t> block_for_index(fn.icodes.size(), static_cast<size_t>(-1));
        for (const auto &block : cfg.blocks()) {
            for (size_t i = block.begin; i < block.end && i < fn.icodes.size(); ++i)
                block_for_index[i] = block.id;
        }

        std::unordered_map<int, int> temp_def_count;
        std::unordered_map<int, std::vector<size_t>> temp_def_indices;
        std::unordered_map<int, std::vector<size_t>> temp_use_indices;

        for (size_t i = 0; i < fn.icodes.size(); ++i) {
            const auto &ic = fn.icodes[i];
            if (ic.result.is_temp() && defines_result(ic)) {
                ++temp_def_count[ic.result.temp_id];
                temp_def_indices[ic.result.temp_id].push_back(i);
            }
            for_each_use_operand(ic, [&](const operand &op) {
                if (op.is_temp())
                    temp_use_indices[op.temp_id].push_back(i);
            });
        }

        const auto dom =
            (cfg.blocks().size() <= control_flow_graph::kMaxLoopOptBlocks)
                ? cfg.dominators()
                : std::vector<std::unordered_set<size_t>>{};

        auto temp_defined_after = [&](int temp_id, size_t index) {
            auto it = temp_def_indices.find(temp_id);
            if (it == temp_def_indices.end())
                return false;
            for (size_t def_idx : it->second)
                if (def_idx > index)
                    return true;
            return false;
        };

        auto temp_defined_between = [&](int temp_id, size_t begin, size_t end) {
            auto it = temp_def_indices.find(temp_id);
            if (it == temp_def_indices.end())
                return false;
            for (size_t def_idx : it->second)
                if (def_idx >= begin && def_idx <= end)
                    return true;
            return false;
        };

        auto copy_dominates_use = [&](size_t copy_idx, size_t use_idx) {
            if (copy_idx >= block_for_index.size() ||
                use_idx >= block_for_index.size()) {
                return false;
            }
            const size_t copy_block = block_for_index[copy_idx];
            const size_t use_block = block_for_index[use_idx];
            if (copy_block == static_cast<size_t>(-1) ||
                use_block == static_cast<size_t>(-1)) {
                return false;
            }
            if (copy_block == use_block)
                return use_idx > copy_idx;
            return !dom.empty() &&
                   use_block < dom.size() &&
                   dom[use_block].count(copy_block) != 0;
        };

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

        for (size_t i = 0; i < fn.icodes.size(); ++i) {
            auto &ic = fn.icodes[i];
            if (erase[i] ||
                (ic.op != icode_op::ASSIGN && ic.op != icode_op::CAST))
                continue;
            if (!ic.result.is_temp() || !ic.left.is_temp() || !ic.right.is_none())
                continue;
            if (ic.result.temp_id == ic.left.temp_id)
                continue;
            if (ic.op == icode_op::CAST && !is_noop_scalar_cast(ic))
                continue;
            if (!bit_preserving_scalar_copy_type(ic.result.type, ic.left.type))
                continue;

            const int dst_temp = ic.result.temp_id;
            const int src_temp = ic.left.temp_id;
            if (temp_def_count[dst_temp] != 1)
                continue;

            auto use_it = temp_use_indices.find(dst_temp);
            if (use_it == temp_use_indices.end() || use_it->second.empty())
                continue;

            bool can_forward = true;
            bool all_uses_same_block = true;
            size_t max_use_idx = i;
            for (size_t use_idx : use_it->second) {
                if (erase[use_idx] || !copy_dominates_use(i, use_idx)) {
                    can_forward = false;
                    break;
                }
                if (block_for_index[use_idx] != block_for_index[i])
                    all_uses_same_block = false;
                max_use_idx = std::max(max_use_idx, use_idx);
            }
            if (!can_forward)
                continue;
            if (all_uses_same_block) {
                if (temp_defined_between(src_temp, i + 1, max_use_idx))
                    continue;
            } else if (temp_defined_after(src_temp, i)) {
                continue;
            }

            const operand src = ic.left;
            for (size_t use_idx : use_it->second) {
                for_each_use_operand(fn.icodes[use_idx], [&](operand &op) {
                    if (op.is_temp() && op.temp_id == dst_temp) {
                        operand repl = src;
                        if (op.type && bit_preserving_scalar_copy_type(op.type,
                                                                       repl.type))
                            repl.type = op.type;
                        repl.byte_offset += op.byte_offset;
                        op = repl;
                    }
                });
            }

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
                type_ptr fold_type =
                    ic.left.type ? ic.left.type
                                 : (ic.right.type ? ic.right.type
                                                  : ic.result.type);
                // Most IR arithmetic has already undergone the language's
                // usual conversions, so its operands and result share a
                // width.  Target-form lowering may intentionally retain a
                // narrower source (for example u16*u16->u32 or u8-u8->int).
                // The operation still takes place at the result width.  Do
                // not apply this rule to comparisons, whose Boolean result
                // says nothing about the operand comparison width.
                if (!is_compare_op(ic.op) && ic.result.type &&
                    (!fold_type ||
                     ic.result.type->size() > fold_type->size())) {
                    fold_type = ic.result.type;
                }
                const int64_t folded = fold_binary(ic.op, ic.left.ival, ic.right.ival,
                                                   fold_type);
                ic.op    = icode_op::ASSIGN;
                ic.left  = operand::make_int(folded, ic.result.type);
                ic.right = operand::make_none();
                changed  = true;
            } else if (ic.op == icode_op::NEG &&
                       ic.left.kind == operand_kind::INT_CONST) {
                const uint64_t raw = 0ULL - static_cast<uint64_t>(ic.left.ival);
                ic.op   = icode_op::ASSIGN;
                ic.left = operand::make_int(cast_int_value(static_cast<int64_t>(raw),
                                                           ic.result.type),
                                            ic.result.type);
                changed = true;
            } else if (ic.op == icode_op::BNOT &&
                       ic.left.kind == operand_kind::INT_CONST) {
                ic.op   = icode_op::ASSIGN;
                ic.left = operand::make_int(cast_int_value(~ic.left.ival, ic.result.type),
                                            ic.result.type);
                changed = true;
            } else if (ic.op == icode_op::CAST &&
                       ic.left.kind == operand_kind::INT_CONST &&
                       ic.result.type &&
                       cast_can_fold_to_int_const(ic.result.type)) {
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
        std::unordered_map<int, int> temp_def_count;
        std::unordered_map<int, const icode *> temp_def;
        for (const auto &candidate : fn.icodes) {
            if (defines_result(candidate) && candidate.result.is_temp()) {
                ++temp_def_count[candidate.result.temp_id];
                temp_def[candidate.result.temp_id] = &candidate;
            }
        }

        bool changed = false;
        for (auto &ic : fn.icodes) {
            if (ic.op == icode_op::CAST && ic.left.is_temp() &&
                ic.left.type && ic.result.type &&
                ic.result.type->size() > 0 &&
                ic.result.type->size() < ic.left.type->size() &&
                ic.result.type->size() <= 2 &&
                temp_def_count[ic.left.temp_id] == 1) {
                const icode *def = temp_def[ic.left.temp_id];
                if (def && def->op == icode_op::BAND) {
                    const operand *mask = nullptr;
                    const operand *value = nullptr;
                    if (def->left.kind == operand_kind::INT_CONST) {
                        mask = &def->left;
                        value = &def->right;
                    } else if (def->right.kind == operand_kind::INT_CONST) {
                        mask = &def->right;
                        value = &def->left;
                    }
                    const unsigned bits =
                        static_cast<unsigned>(ic.result.type->size() * 8);
                    const uint64_t low_mask = (uint64_t{1} << bits) - 1;
                    if (mask && value &&
                        (static_cast<uint64_t>(mask->ival) & low_mask) ==
                            low_mask) {
                        ic.left = *value;
                        changed = true;
                    }
                }
            }
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

class add_const_chain_pass final : public ir_pass {
public:
    const char *name() const override { return "add_const_chain"; }

    bool run(ir_function &fn) override {
        if (fn.icodes.empty())
            return false;

        control_flow_graph cfg(fn);
        std::unordered_map<int, int> temp_def_count;
        std::unordered_map<int, std::vector<size_t>> temp_use_indices;

        for (size_t i = 0; i < fn.icodes.size(); ++i) {
            const auto &ic = fn.icodes[i];
            if (ic.result.is_temp())
                ++temp_def_count[ic.result.temp_id];
            for_each_use_operand(ic, [&](const operand &op) {
                if (op.is_temp())
                    temp_use_indices[op.temp_id].push_back(i);
            });
        }

        auto barrier_between = [&](size_t begin, size_t end) {
            for (size_t i = begin; i < end; ++i) {
                if (is_local_cse_barrier(fn.icodes[i]))
                    return true;
            }
            return false;
        };

        auto extract_const_chain = [&](const icode &ic, operand &base,
                                       int64_t &delta) {
            if (ic.op == icode_op::ADD) {
                if (ic.right.kind == operand_kind::INT_CONST &&
                    ic.left.kind != operand_kind::INT_CONST) {
                    base = ic.left;
                    delta = ic.right.ival;
                    return true;
                }
                if (ic.left.kind == operand_kind::INT_CONST &&
                    ic.right.kind != operand_kind::INT_CONST) {
                    base = ic.right;
                    delta = ic.left.ival;
                    return true;
                }
                return false;
            }

            if (ic.op == icode_op::SUB &&
                ic.right.kind == operand_kind::INT_CONST &&
                ic.left.kind != operand_kind::INT_CONST) {
                base = ic.left;
                delta = -ic.right.ival;
                return true;
            }

            return false;
        };

        bool changed = false;

        for (const auto &block : cfg.blocks()) {
            for (size_t i = block.begin; i < block.end; ++i) {
                auto &producer = fn.icodes[i];
                if (!producer.result.is_temp())
                    continue;

                operand producer_base;
                int64_t producer_delta = 0;
                if (!extract_const_chain(producer, producer_base, producer_delta))
                    continue;

                const int produced_temp = producer.result.temp_id;
                if (temp_def_count[produced_temp] != 1)
                    continue;

                auto use_it = temp_use_indices.find(produced_temp);
                if (use_it == temp_use_indices.end() || use_it->second.size() != 1)
                    continue;

                const size_t consumer_idx = use_it->second.front();
                if (consumer_idx <= i || consumer_idx >= block.end)
                    continue;
                if (barrier_between(i + 1, consumer_idx))
                    continue;

                auto &consumer = fn.icodes[consumer_idx];
                operand consumer_base;
                int64_t consumer_delta = 0;
                bool uses_temp = false;

                if (consumer.op == icode_op::ADD) {
                    if (consumer.left.is_temp() &&
                        consumer.left.temp_id == produced_temp &&
                        consumer.right.kind == operand_kind::INT_CONST) {
                        consumer_base = producer_base;
                        consumer_delta = consumer.right.ival;
                        uses_temp = true;
                    } else if (consumer.right.is_temp() &&
                               consumer.right.temp_id == produced_temp &&
                               consumer.left.kind == operand_kind::INT_CONST) {
                        consumer_base = producer_base;
                        consumer_delta = consumer.left.ival;
                        uses_temp = true;
                    }
                } else if (consumer.op == icode_op::SUB &&
                           consumer.left.is_temp() &&
                           consumer.left.temp_id == produced_temp &&
                           consumer.right.kind == operand_kind::INT_CONST) {
                    consumer_base = producer_base;
                    consumer_delta = -consumer.right.ival;
                    uses_temp = true;
                }

                if (!uses_temp)
                    continue;

                int64_t combined = producer_delta + consumer_delta;
                consumer.left = producer_base;
                if (combined == 0) {
                    consumer.op = icode_op::ASSIGN;
                    consumer.right = operand::make_none();
                } else if (combined > 0) {
                    consumer.op = icode_op::ADD;
                    consumer.right = operand::make_int(
                        combined,
                        consumer.result.type ? consumer.result.type : producer.result.type);
                } else {
                    consumer.op = icode_op::SUB;
                    consumer.right = operand::make_int(
                        -combined,
                        consumer.result.type ? consumer.result.type : producer.result.type);
                }
                changed = true;
            }
        }

        return changed;
    }
};

class post_update_recover_pass final : public ir_pass {
public:
    const char *name() const override { return "post_update_recover"; }

    bool run(ir_function &fn) override {
        if (fn.icodes.size() < 3)
            return false;

        control_flow_graph cfg(fn);
        bool changed = false;

        auto same_loc = [&](const operand &a, const operand &b) {
            return a.kind == b.kind &&
                   a.byte_offset == b.byte_offset &&
                   a.is_global == b.is_global &&
                   a.is_param == b.is_param &&
                   a.stack_offset == b.stack_offset &&
                   a.temp_id == b.temp_id &&
                   a.name == b.name;
        };

        auto is_copy_sink = [&](const icode &ic, const operand &target,
                                int temp_id) {
            if (!(ic.left.is_temp() && ic.left.temp_id == temp_id))
                return false;
            if (!same_loc(ic.result, target))
                return false;
            return ic.op == icode_op::ASSIGN ||
                   (ic.op == icode_op::CAST && is_noop_scalar_cast(ic));
        };

        auto extract_step = [&](const icode &ic, operand &base, int64_t &step) {
            if (ic.op == icode_op::ADD &&
                ic.right.kind == operand_kind::INT_CONST &&
                ic.left.kind != operand_kind::INT_CONST) {
                base = ic.left;
                step = ic.right.ival;
                return true;
            }
            if (ic.op == icode_op::SUB &&
                ic.right.kind == operand_kind::INT_CONST &&
                ic.left.kind != operand_kind::INT_CONST) {
                base = ic.left;
                step = -ic.right.ival;
                return true;
            }
            return false;
        };

        for (const auto &block : cfg.blocks()) {
            for (size_t i = block.begin; i + 2 < block.end; ++i) {
                auto &update_ic = fn.icodes[i];
                if (!update_ic.result.is_temp())
                    continue;

                operand base;
                int64_t step = 0;
                if (!extract_step(update_ic, base, step) || step == 0)
                    continue;
                if (base.type && base.type->is_ptr())
                    continue;

                auto &copy_ic = fn.icodes[i + 1];
                if (!is_copy_sink(copy_ic, base, update_ic.result.temp_id))
                    continue;

                auto &recover_ic = fn.icodes[i + 2];
                operand recover_base;
                int64_t recover_step = 0;
                if (!extract_step(recover_ic, recover_base, recover_step))
                    continue;
                if (!same_loc(recover_base, base) || recover_step != -step)
                    continue;

                if (recover_ic.result.is_temp() &&
                    recover_ic.result.temp_id == update_ic.result.temp_id)
                    continue;

                icode moved = recover_ic;
                moved.op = icode_op::ASSIGN;
                moved.left = base;
                moved.right = operand::make_none();

                fn.icodes[i] = moved;
                fn.icodes[i + 1] = update_ic;
                fn.icodes[i + 2] = copy_ic;
                changed = true;
                ++i;
            }
        }

        // Recover `array[i++]` after scalar promotion has produced
        // `old=i; next=i+1; i=next; address(base,old); store`. Delay the pure
        // update through the complete old-value dependency chain so the
        // backend can use `i` directly and avoid materializing `old`.
        for (const auto &block : cfg.blocks()) {
            for (size_t i = block.begin; i + 3 < block.end; ++i) {
                const icode &save = fn.icodes[i];
                const icode &update = fn.icodes[i + 1];
                const icode &commit = fn.icodes[i + 2];
                if (save.op != icode_op::ASSIGN ||
                    !save.result.is_temp() || !save.left.is_temp() ||
                    save.result.temp_id == save.left.temp_id ||
                    update.op != icode_op::ADD ||
                    !update.result.is_temp() || !update.left.is_temp() ||
                    update.left.temp_id != save.left.temp_id ||
                    update.right.kind != operand_kind::INT_CONST ||
                    update.right.ival == 0 ||
                    commit.op != icode_op::ASSIGN ||
                    !commit.result.is_temp() ||
                    commit.result.temp_id != save.left.temp_id ||
                    !commit.left.is_temp() ||
                    commit.left.temp_id != update.result.temp_id) {
                    continue;
                }

                const int saved_temp = save.result.temp_id;
                const int base_temp = save.left.temp_id;
                const int next_temp = update.result.temp_id;
                std::unordered_set<int> dependent_temps{saved_temp};
                size_t last_dependent = i;
                bool safe = true;

                auto references_any = [&](const operand &op) {
                    return op.is_temp() &&
                           dependent_temps.count(op.temp_id) != 0;
                };
                auto references_temp = [](const operand &op, int temp_id) {
                    return op.is_temp() && op.temp_id == temp_id;
                };

                for (size_t j = i + 3; j < block.end; ++j) {
                    const icode &consumer = fn.icodes[j];
                    // SEND materializes an ABI argument in a physical register
                    // or on the machine stack.  Moving the update past it can
                    // clobber that argument before CALL (for example in
                    // `putc(*cursor++, ctx)`).  Calls and inline assembly are
                    // barriers for the same reason, even when they do not
                    // mention the recovered value explicitly.
                    if (consumer.op == icode_op::SEND ||
                        consumer.op == icode_op::CALL ||
                        consumer.op == icode_op::INLINE_ASM) {
                        safe = false;
                        break;
                    }
                    if (references_temp(consumer.result, base_temp) ||
                        references_temp(consumer.left, base_temp) ||
                        references_temp(consumer.right, base_temp) ||
                        references_temp(consumer.result, next_temp) ||
                        references_temp(consumer.left, next_temp) ||
                        references_temp(consumer.right, next_temp)) {
                        safe = false;
                        break;
                    }

                    if (defines_result(consumer) &&
                        consumer.result.is_temp() &&
                        dependent_temps.count(consumer.result.temp_id)) {
                        safe = false;
                        break;
                    }

                    const bool result_is_use = !defines_result(consumer);
                    const bool consumes_dependency =
                        references_any(consumer.left) ||
                        references_any(consumer.right) ||
                        (result_is_use && references_any(consumer.result));
                    if (!consumes_dependency)
                        continue;
                    if (consumer.op == icode_op::GOTO ||
                        consumer.op == icode_op::IFX ||
                        consumer.op == icode_op::RETURN ||
                        consumer.op == icode_op::CALL) {
                        safe = false;
                        break;
                    }
                    last_dependent = j;
                    if (defines_result(consumer) && consumer.result.is_temp())
                        dependent_temps.insert(consumer.result.temp_id);
                }
                if (!safe || last_dependent < i + 3)
                    continue;

                // Every derived temporary must die in this basic block;
                // otherwise delaying the update could cross an unseen use.
                for (size_t j = block.end; j < fn.icodes.size() && safe; ++j) {
                    const icode &later = fn.icodes[j];
                    auto escapes = [&](const operand &op) {
                        return op.is_temp() &&
                               dependent_temps.count(op.temp_id) != 0;
                    };
                    if (escapes(later.result) || escapes(later.left) ||
                        escapes(later.right)) {
                        safe = false;
                    }
                }
                if (!safe)
                    continue;

                std::vector<icode> replacement;
                replacement.reserve(last_dependent - i);
                for (size_t j = i + 3; j <= last_dependent; ++j) {
                    icode moved = fn.icodes[j];
                    auto use_base = [&](operand &op) {
                        if (op.is_temp() && op.temp_id == saved_temp)
                            op = save.left;
                    };
                    use_base(moved.result);
                    use_base(moved.left);
                    use_base(moved.right);
                    replacement.push_back(std::move(moved));
                }
                replacement.push_back(update);
                replacement.push_back(commit);

                std::vector<icode> rewritten;
                rewritten.reserve(fn.icodes.size() - 1);
                rewritten.insert(rewritten.end(), fn.icodes.begin(),
                                 fn.icodes.begin() + i);
                rewritten.insert(rewritten.end(), replacement.begin(),
                                 replacement.end());
                rewritten.insert(rewritten.end(),
                                 fn.icodes.begin() + last_dependent + 1,
                                 fn.icodes.end());
                fn.icodes = std::move(rewritten);
                return true;
            }
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
    case icode_op::ALLOCA:
    case icode_op::SET_VALUE_AT:
    case icode_op::BLOCK_FILL:
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
    case icode_op::ADDRESS_OF:
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
                         ic.op != icode_op::ADDRESS_OF &&
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

        std::function<void(const operand &)> invalidate_operand;
        invalidate_operand = [&](const operand &op) {
            std::string dep = local_cse_operand_key(op);
            if (dep.empty())
                return;
            auto it = dep_to_exprs.find(dep);
            if (it == dep_to_exprs.end())
                return;
            std::vector<std::string> invalidated = std::move(it->second);
            dep_to_exprs.erase(it);
            for (const auto &expr_key : invalidated) {
                auto result_it = expr_to_result.find(expr_key);
                if (result_it == expr_to_result.end())
                    continue;
                operand derived_result = result_it->second;
                expr_to_result.erase(result_it);
                // Expressions depending on this cached result are stale too.
                invalidate_operand(derived_result);
            }
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
            // If this temporary is later redefined, the cached expression no
            // longer produces a reusable value through that result operand.
            note_dep(ic.result, expr_key);
        }

        return changed;
    }
};

// Hoist an expression repeated in several mutually exclusive branch regions
// to their nearest common branch dominator.  Local CSE deliberately forgets
// values at every control-flow boundary; that misses the equally important
// switch/parser case where each arm starts by rebuilding the same scaled
// index or address.  This is a small, speculative PRE step rather than a
// source-pattern transform: the expression must be side-effect free, occur
// in at least three distinct blocks, and every path from the insertion block
// to each occurrence must preserve its temporary operands.
class branch_common_expr_hoist_pass final : public ir_pass {
public:
    const char *name() const override { return "branch_common_expr_hoist"; }

    bool run(ir_function &fn) override {
        if (fn.icodes.size() < 8)
            return false;

        control_flow_graph cfg(fn);
        const auto &blocks = cfg.blocks();
        if (blocks.size() < 4 ||
            blocks.size() > control_flow_graph::kMaxLoopOptBlocks) {
            return false;
        }
        const auto dominators = cfg.dominators();

        std::vector<size_t> block_of(fn.icodes.size(), 0);
        for (const auto &block : blocks) {
            for (size_t i = block.begin; i < block.end; ++i)
                block_of[i] = block.id;
        }

        auto speculatable = [](const icode &ic) {
            if (!ic.result.is_temp() || !ic.result.type ||
                ic.result.type->size() > 2) {
                return false;
            }
            switch (ic.op) {
            case icode_op::ADD:
            case icode_op::SUB:
            case icode_op::SHL:
            case icode_op::SHR:
            case icode_op::BAND:
            case icode_op::BOR:
            case icode_op::BXOR:
            case icode_op::NEG:
            case icode_op::BNOT:
            case icode_op::CAST:
                return true;
            default:
                return false;
            }
        };

        std::unordered_map<std::string, std::vector<size_t>> occurrences;
        for (size_t i = 0; i < fn.icodes.size(); ++i) {
            const icode &ic = fn.icodes[i];
            if (!speculatable(ic))
                continue;
            bool has_temp_input = false;
            for_each_use_operand(ic, [&](const operand &op) {
                has_temp_input = has_temp_input || op.is_temp();
            });
            if (!has_temp_input)
                continue;
            const std::string key = local_cse_expr_key(ic);
            if (!key.empty())
                occurrences[key].push_back(i);
        }

        auto defines_temp = [](const icode &ic, int tid) {
            return defines_result(ic) && ic.result.is_temp() &&
                   ic.result.temp_id == tid;
        };

        // Prefer the most frequently duplicated expression.  Stable lexical
        // tie-breaking keeps generated code deterministic.
        std::vector<std::pair<std::string, std::vector<size_t>>> groups;
        for (auto &entry : occurrences) {
            std::unordered_set<size_t> distinct_blocks;
            for (size_t idx : entry.second)
                distinct_blocks.insert(block_of[idx]);
            if (distinct_blocks.size() >= 3)
                groups.push_back({entry.first, std::move(entry.second)});
        }
        std::sort(groups.begin(), groups.end(),
                  [](const auto &lhs, const auto &rhs) {
                      if (lhs.second.size() != rhs.second.size())
                          return lhs.second.size() > rhs.second.size();
                      return lhs.second.front() < rhs.second.front();
                  });

        for (const auto &group : groups) {
            const std::vector<size_t> &sites = group.second;
            std::unordered_set<size_t> common = dominators[block_of[sites[0]]];
            for (size_t j = 1; j < sites.size() && !common.empty(); ++j) {
                std::unordered_set<size_t> next;
                for (size_t candidate : common) {
                    if (dominators[block_of[sites[j]]].count(candidate))
                        next.insert(candidate);
                }
                common = std::move(next);
            }
            if (common.empty())
                continue;

            size_t dom_id = *common.begin();
            for (size_t candidate : common) {
                if (dominators[candidate].size() >
                    dominators[dom_id].size()) {
                    dom_id = candidate;
                }
            }
            const basic_block &dom_block = blocks[dom_id];
            if (dom_block.begin >= dom_block.end)
                continue;
            const size_t terminator = dom_block.end - 1;
            if (fn.icodes[terminator].op != icode_op::IFX)
                continue;
            size_t insert_at = terminator;
            const icode &ifx = fn.icodes[terminator];
            if (terminator > dom_block.begin && ifx.left.is_temp()) {
                const icode &condition = fn.icodes[terminator - 1];
                if (defines_result(condition) && condition.result.is_temp() &&
                    condition.result.temp_id == ifx.left.temp_id &&
                    (condition.op == icode_op::EQ ||
                     condition.op == icode_op::NE ||
                     condition.op == icode_op::LT ||
                     condition.op == icode_op::LE ||
                     condition.op == icode_op::GT ||
                     condition.op == icode_op::GE)) {
                    insert_at = terminator - 1;
                }
            }

            bool site_in_dominator = false;
            for (size_t idx : sites)
                site_in_dominator |= block_of[idx] == dom_id;
            if (site_in_dominator)
                continue;

            const icode &prototype = fn.icodes[sites[0]];
            icode hoisted = prototype;
            std::vector<int> input_temps;
            for_each_use_operand(prototype, [&](const operand &op) {
                if (op.is_temp() &&
                    std::find(input_temps.begin(), input_temps.end(),
                              op.temp_id) == input_temps.end()) {
                    input_temps.push_back(op.temp_id);
                }
            });
            if (input_temps.empty())
                continue;

            // A prior mention in the insertion block proves that every input
            // already has a value at the hoist point.  This admits mutable
            // loop state at a dispatch header without pretending it has one
            // SSA definition.
            bool inputs_available = true;
            for (int tid : input_temps) {
                bool mentioned = false;
                for (size_t i = dom_block.begin; i < insert_at; ++i) {
                    for_each_use_operand(fn.icodes[i], [&](const operand &op) {
                        mentioned = mentioned ||
                                    (op.is_temp() && op.temp_id == tid);
                    });
                    mentioned = mentioned || defines_temp(fn.icodes[i], tid);
                }
                if (!mentioned) {
                    inputs_available = false;
                    break;
                }
            }
            if (!inputs_available)
                continue;

            bool paths_preserve_inputs = true;
            for (size_t site : sites) {
                const size_t target_id = block_of[site];
                std::unordered_set<size_t> can_reach{target_id};
                std::vector<size_t> work{target_id};
                while (!work.empty()) {
                    const size_t id = work.back();
                    work.pop_back();
                    for (size_t pred : blocks[id].preds) {
                        if (pred == dom_id)
                            continue;
                        if (can_reach.insert(pred).second)
                            work.push_back(pred);
                    }
                }

                for (size_t id : can_reach) {
                    const basic_block &block = blocks[id];
                    const size_t end = id == target_id ? site : block.end;
                    for (size_t i = block.begin; i < end; ++i) {
                        for (int tid : input_temps) {
                            if (defines_temp(fn.icodes[i], tid)) {
                                paths_preserve_inputs = false;
                                break;
                            }
                        }
                        if (!paths_preserve_inputs)
                            break;
                    }
                    if (!paths_preserve_inputs)
                        break;
                }
                if (!paths_preserve_inputs)
                    break;
            }
            if (!paths_preserve_inputs)
                continue;

            // Each old result must be an ordinary single-definition temp, so
            // replacing its uses cannot cross another value lifetime.
            std::unordered_map<int, int> aliases;
            const int canonical_tid = prototype.result.temp_id;
            bool unique_results = true;
            for (size_t site : sites) {
                const int old_tid = fn.icodes[site].result.temp_id;
                int definitions = 0;
                for (const icode &ic : fn.icodes)
                    definitions += defines_temp(ic, old_tid) ? 1 : 0;
                if (definitions != 1) {
                    unique_results = false;
                    break;
                }
                aliases[old_tid] = canonical_tid;
            }
            if (!unique_results)
                continue;

            for (icode &ic : fn.icodes) {
                for_each_use_operand(ic, [&](operand &op) {
                    if (!op.is_temp())
                        return;
                    auto alias = aliases.find(op.temp_id);
                    if (alias != aliases.end())
                        op.temp_id = alias->second;
                });
            }

            std::unordered_set<size_t> erase(sites.begin(), sites.end());
            std::vector<icode> rewritten;
            rewritten.reserve(fn.icodes.size() - sites.size() + 1);
            for (size_t i = 0; i < fn.icodes.size(); ++i) {
                if (i == insert_at)
                    rewritten.push_back(hoisted);
                if (!erase.count(i))
                    rewritten.push_back(std::move(fn.icodes[i]));
            }
            fn.icodes = std::move(rewritten);
            return true;
        }

        return false;
    }
};

struct linear_expr {
    std::map<std::string, int64_t> terms;
    int64_t constant = 0;
    std::vector<std::string> deps;
};

static bool value_preserving_cse_cast(const icode &ic) {
    if (ic.op != icode_op::CAST || !ic.left.type || !ic.result.type)
        return false;

    type_ptr src = ic.left.type->unqual();
    type_ptr dst = ic.result.type->unqual();
    if (!src || !dst)
        return false;
    if (src->is_far_ptr() || dst->is_far_ptr())
        return false;

    if (src->is_ptr() && dst->is_ptr())
        return src->size() == dst->size();

    if (!src->is_integer() || !dst->is_integer())
        return false;

    if (src->size() == dst->size())
        return src->is_unsigned() == dst->is_unsigned();

    if (src->size() > dst->size())
        return false;

    // Widening preserves the mathematical value for unsigned sources and for
    // signed-to-signed sources.  Avoid signed-to-unsigned here because negative
    // inputs would change value even though the bit pattern is well-defined.
    return src->is_unsigned() || !dst->is_unsigned();
}

static std::string canonical_linear_leaf_key(
    const operand &op,
    const std::unordered_map<int, const icode *> &,
    int = 0) {
    // A copy is a value snapshot, not a permanent alias. Recursively chasing
    // its source through a map of latest definitions can make an earlier copy
    // appear to change after the source temp is reassigned.
    return local_cse_operand_key(op);
}

static bool linear_expr_add_term(linear_expr &expr,
                                 const std::string &key,
                                 int64_t coeff) {
    if (key.empty() || coeff == 0)
        return true;

    int64_t &slot = expr.terms[key];
    if ((coeff > 0 && slot > std::numeric_limits<int64_t>::max() - coeff) ||
        (coeff < 0 && slot < std::numeric_limits<int64_t>::min() - coeff)) {
        return false;
    }

    slot += coeff;
    if (slot == 0)
        expr.terms.erase(key);
    return true;
}

static bool linear_expr_accumulate(linear_expr &dst,
                                   const linear_expr &src,
                                   int sign) {
    if (sign != 1 && sign != -1)
        return false;
    if ((sign > 0 &&
         dst.constant > std::numeric_limits<int64_t>::max() - src.constant) ||
        (sign < 0 &&
         dst.constant < std::numeric_limits<int64_t>::min() + src.constant)) {
        return false;
    }
    dst.constant += sign * src.constant;

    for (const auto &term : src.terms) {
        if (!linear_expr_add_term(dst, term.first, sign * term.second))
            return false;
    }
    dst.deps.insert(dst.deps.end(), src.deps.begin(), src.deps.end());
    return true;
}

static bool linear_expr_scale(linear_expr &expr, int64_t scale) {
    if (scale == 0) {
        expr.terms.clear();
        expr.constant = 0;
        return true;
    }

    if (expr.constant != 0 &&
        (expr.constant > std::numeric_limits<int64_t>::max() / scale ||
         expr.constant < std::numeric_limits<int64_t>::min() / scale)) {
        return false;
    }
    expr.constant *= scale;

    for (auto &term : expr.terms) {
        if (term.second != 0 &&
            (term.second > std::numeric_limits<int64_t>::max() / scale ||
             term.second < std::numeric_limits<int64_t>::min() / scale)) {
            return false;
        }
        term.second *= scale;
    }
    return true;
}

static std::optional<linear_expr> resolve_linear_expr(
    const operand &op,
    const std::unordered_map<int, const icode *> &temp_defs,
    int depth,
    std::unordered_set<int> &visiting);

static std::optional<linear_expr> resolve_linear_expr_from_def(
    const icode &def,
    const std::unordered_map<int, const icode *> &temp_defs,
    int depth,
    std::unordered_set<int> &visiting) {
    if (depth > 8)
        return std::nullopt;

    switch (def.op) {
    case icode_op::ASSIGN:
    case icode_op::ADDRESS_OF:
        return resolve_linear_expr(def.left, temp_defs, depth + 1, visiting);
    case icode_op::CAST:
        if (!value_preserving_cse_cast(def))
            return std::nullopt;
        return resolve_linear_expr(def.left, temp_defs, depth + 1, visiting);
    case icode_op::ADD:
    case icode_op::SUB: {
        auto left = resolve_linear_expr(def.left, temp_defs, depth + 1,
                                        visiting);
        auto right = resolve_linear_expr(def.right, temp_defs, depth + 1,
                                         visiting);
        if (!left || !right)
            return std::nullopt;
        linear_expr out;
        if (!linear_expr_accumulate(out, *left, 1))
            return std::nullopt;
        if (!linear_expr_accumulate(out, *right,
                                    def.op == icode_op::ADD ? 1 : -1))
            return std::nullopt;
        return out;
    }
    case icode_op::SHL: {
        if (def.right.kind != operand_kind::INT_CONST ||
            def.right.ival < 0 || def.right.ival > 15)
            return std::nullopt;
        auto out = resolve_linear_expr(def.left, temp_defs, depth + 1,
                                       visiting);
        if (!out)
            return std::nullopt;
        if (!linear_expr_scale(*out, int64_t{1} << def.right.ival))
            return std::nullopt;
        return out;
    }
    default:
        return std::nullopt;
    }
}

static std::optional<linear_expr> resolve_linear_expr(
    const operand &op,
    const std::unordered_map<int, const icode *> &temp_defs,
    int depth,
    std::unordered_set<int> &visiting) {
    if (depth > 8)
        return std::nullopt;

    if (op.kind == operand_kind::INT_CONST) {
        linear_expr out;
        out.constant = op.ival;
        return out;
    }

    if (op.is_temp()) {
        if (!visiting.insert(op.temp_id).second)
            return std::nullopt;
        auto cleanup = [&]() { visiting.erase(op.temp_id); };
        auto it = temp_defs.find(op.temp_id);
        if (it != temp_defs.end() && it->second) {
            const icode &def = *it->second;
            const bool self_updates =
                (def.left.is_temp() && def.left.temp_id == op.temp_id) ||
                (def.right.is_temp() && def.right.temp_id == op.temp_id);
            if (def.op != icode_op::ASSIGN &&
                def.op != icode_op::CAST && !self_updates) {
                auto out = resolve_linear_expr_from_def(
                    def, temp_defs, depth + 1, visiting);
                cleanup();
                if (out)
                    return out;
            } else {
                cleanup();
            }
        } else {
            cleanup();
        }
    }

    std::string key = canonical_linear_leaf_key(op, temp_defs);
    if (key.empty())
        return std::nullopt;

    linear_expr out;
    out.terms.emplace(key, 1);
    out.deps.push_back(key);
    return out;
}

static std::string linear_expr_key(const linear_expr &expr,
                                   const type_ptr &result_type) {
    if (expr.terms.empty())
        return {};

    std::string key = "LIN:";
    key += result_type ? std::to_string(result_type->size()) : "0";
    key += ":";
    key += result_type && result_type->is_unsigned() ? "u" : "s";
    key += "|C:";
    key += std::to_string(expr.constant);
    for (const auto &term : expr.terms) {
        key += "|";
        key += term.first;
        key += "*";
        key += std::to_string(term.second);
    }
    return key;
}

static bool linear_expr_address_cse_candidate(const icode &ic,
                                              const linear_expr &expr) {
    if (ic.op != icode_op::ADD && ic.op != icode_op::SUB)
        return false;

    auto pointerish = [](const type_ptr &t) {
        return t && !t->is_far_ptr() && (t->is_ptr() || t->is_array());
    };
    if (!pointerish(ic.result.type) &&
        !pointerish(ic.left.type) &&
        !pointerish(ic.right.type)) {
        return false;
    }

    bool has_direct_base = false;
    bool has_scaled_index = false;
    for (const auto &term : expr.terms) {
        if (term.second == 1 &&
            (term.first.rfind("S:", 0) == 0 ||
             term.first.rfind("L:", 0) == 0)) {
            has_direct_base = true;
        }
        if (term.second != 0 && term.second != 1 && term.second != -1)
            has_scaled_index = true;
    }

    return has_direct_base && has_scaled_index;
}

class linear_expr_cse_pass final : public ir_pass {
public:
    const char *name() const override { return "linear_expr_cse"; }

    bool run(ir_function &fn) override {
        if (fn.icodes.empty())
            return false;

        struct entry {
            operand value;
            std::vector<std::string> deps;
        };

        std::unordered_map<int, const icode *> temp_defs;
        std::unordered_map<std::string, entry> available;
        bool changed = false;

        auto clear_for_barrier = [&](const icode &ic) {
            switch (ic.op) {
            case icode_op::FUNCTION:
            case icode_op::ENDFUNCTION:
            case icode_op::LABEL:
            case icode_op::GOTO:
            case icode_op::IFX:
            case icode_op::CALL:
            case icode_op::RETURN:
            case icode_op::INLINE_ASM:
            case icode_op::RECEIVE:
            case icode_op::ALLOCA:
                return true;
            default:
                return false;
            }
        };

        auto invalidate_dep = [&](const operand &op) {
            if (available.empty())
                return;
            std::unordered_set<std::string> deps;
            std::string direct = local_cse_operand_key(op);
            if (!direct.empty())
                deps.insert(direct);
            std::string canonical = canonical_linear_leaf_key(op, temp_defs);
            if (!canonical.empty())
                deps.insert(canonical);
            if (deps.empty())
                return;
            for (auto it = available.begin(); it != available.end();) {
                bool erase = false;
                for (const auto &entry_dep : it->second.deps) {
                    if (deps.count(entry_dep)) {
                        erase = true;
                        break;
                    }
                }
                if (erase)
                    it = available.erase(it);
                else
                    ++it;
            }
        };
        auto note_temp_def = [&](const icode &ic) {
            if (defines_result(ic) && ic.result.is_temp())
                temp_defs[ic.result.temp_id] = &ic;
        };

        for (auto &ic : fn.icodes) {
            if (clear_for_barrier(ic)) {
                available.clear();
                temp_defs.clear();
                continue;
            }

            if (defines_result(ic)) {
                const bool redefines_temp =
                    ic.result.is_temp() &&
                    temp_defs.find(ic.result.temp_id) != temp_defs.end();
                invalidate_dep(ic.result);
                // Canonical linear keys recursively expand prior temp
                // definitions. Once a mutable temp gets a new definition,
                // cached derived-address keys can otherwise retain that old
                // expansion even when the direct dependency was folded away.
                if (redefines_temp)
                    available.clear();
            }

            if (!is_local_cse_candidate(ic)) {
                note_temp_def(ic);
                continue;
            }

            std::unordered_set<int> visiting;
            auto expr = resolve_linear_expr_from_def(ic, temp_defs, 0,
                                                     visiting);
            if (!expr) {
                note_temp_def(ic);
                continue;
            }
            if (!linear_expr_address_cse_candidate(ic, *expr)) {
                note_temp_def(ic);
                continue;
            }

            std::string key = linear_expr_key(*expr, ic.result.type);
            if (key.empty()) {
                note_temp_def(ic);
                continue;
            }

            auto found = available.find(key);
            if (found != available.end()) {
                ic.op = icode_op::ASSIGN;
                ic.left = found->second.value;
                ic.right = operand::make_none();
                note_temp_def(ic);
                changed = true;
                continue;
            }

            available.emplace(key, entry{ic.result, expr->deps});
            note_temp_def(ic);
        }

        return changed;
    }
};

struct available_byte_load_entry {
    operand value;
    std::vector<std::string> deps;
};

struct available_byte_index_key {
    std::string key;
    std::vector<std::string> deps;
};

using temp_def_map = std::unordered_map<int, const icode *>;

static bool is_available_byte_load_barrier(const icode &ic) {
    switch (ic.op) {
    case icode_op::FUNCTION:
    case icode_op::ENDFUNCTION:
    case icode_op::CALL:
    case icode_op::BLOCK_FILL:
    case icode_op::INLINE_ASM:
    case icode_op::RECEIVE:
    case icode_op::SEND:
    case icode_op::ALLOCA:
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

static available_byte_index_key available_byte_index_operand_key(
    const operand &op,
    const temp_def_map &temp_defs,
    int depth = 0) {
    if (depth > 4)
        return {};

    auto direct_key = [&]() -> available_byte_index_key {
        std::string key = local_cse_operand_key(op);
        if (key.empty())
            return {};
        available_byte_index_key out;
        out.key = key;
        if (op.kind == operand_kind::TEMP || op.kind == operand_kind::SYMBOL)
            out.deps.push_back(key);
        return out;
    };

    if (!op.is_temp())
        return direct_key();

    auto it = temp_defs.find(op.temp_id);
    if (it == temp_defs.end() || !it->second)
        return direct_key();

    const icode &def = *it->second;
    auto type_key = [&]() {
        return std::to_string(def.result.type ? def.result.type->size() : 0) +
               ":" +
               std::to_string(def.result.type && def.result.type->is_unsigned());
    };

    if ((def.op == icode_op::ASSIGN || def.op == icode_op::CAST) &&
        !def.left.is_none()) {
        auto nested = available_byte_index_operand_key(def.left, temp_defs,
                                                       depth + 1);
        if (!nested.key.empty()) {
            nested.key = "IDXCAST|" + type_key() + "|" + nested.key;
            return nested;
        }
    }

    const operand *base = nullptr;
    int64_t delta = 0;
    if (def.op == icode_op::ADD) {
        if (def.right.kind == operand_kind::INT_CONST) {
            base = &def.left;
            delta = def.right.ival;
        } else if (def.left.kind == operand_kind::INT_CONST) {
            base = &def.right;
            delta = def.left.ival;
        }
    } else if (def.op == icode_op::SUB &&
               def.right.kind == operand_kind::INT_CONST) {
        base = &def.left;
        delta = -def.right.ival;
    }

    if (base) {
        auto nested = available_byte_index_operand_key(*base, temp_defs,
                                                       depth + 1);
        if (!nested.key.empty()) {
            nested.key = "IDXADD|" + type_key() + "|" + nested.key + "|" +
                         std::to_string(delta);
            return nested;
        }
    }

    return direct_key();
}

static std::string available_byte_load_key(const icode &ic,
                                           const temp_def_map &temp_defs) {
    if (!is_available_byte_load_candidate(ic))
        return {};

    if (ic.left.kind == operand_kind::LABEL_REF) {
        auto idx = available_byte_index_operand_key(ic.right, temp_defs);
        if (idx.key.empty())
            return {};
        return "LOAD8|" + ic.left.name + "|" + idx.key;
    }

    std::string ptr = local_cse_operand_key(ic.left);
    if (ptr.empty())
        return {};
    return "LOAD8PTR|" + ptr;
}

static std::vector<std::string> available_byte_load_deps(
    const icode &ic,
    const temp_def_map &temp_defs) {
    std::vector<std::string> deps;
    std::string ptr = local_cse_operand_key(ic.left);
    if (!ptr.empty())
        deps.push_back(ptr);
    auto idx = available_byte_index_operand_key(ic.right, temp_defs);
    deps.insert(deps.end(), idx.deps.begin(), idx.deps.end());
    return deps;
}

static bool available_byte_direct_data_base(const operand &op) {
    return op.kind == operand_kind::SYMBOL &&
           op.is_global &&
           !op.is_tls &&
           !op.is_sfr &&
           !op.is_func &&
           !op.is_param;
}

static bool available_byte_extract_data_base(
    const operand &op,
    const temp_def_map &temp_defs,
    std::string &base,
    bool allow_bare_symbol,
    int depth = 0) {
    if (depth > 8)
        return false;

    if (op.kind == operand_kind::LABEL_REF) {
        base = op.name;
        return true;
    }

    if (allow_bare_symbol && available_byte_direct_data_base(op)) {
        base = op.name;
        return true;
    }

    if (!op.is_temp())
        return false;

    auto it = temp_defs.find(op.temp_id);
    if (it == temp_defs.end() || !it->second)
        return false;

    const icode &def = *it->second;
    if ((def.op == icode_op::ASSIGN || def.op == icode_op::CAST) &&
        !def.left.is_none()) {
        return available_byte_extract_data_base(def.left, temp_defs, base,
                                                allow_bare_symbol, depth + 1);
    }

    if (def.op == icode_op::ADDRESS_OF &&
        available_byte_direct_data_base(def.left)) {
        base = def.left.name;
        return true;
    }

    if (def.op == icode_op::ADD) {
        if (available_byte_extract_data_base(def.left, temp_defs, base, true,
                                             depth + 1))
            return true;
        if (available_byte_extract_data_base(def.right, temp_defs, base, true,
                                             depth + 1))
            return true;
    }

    return false;
}

static std::string available_byte_direct_store_base(
    const icode &ic,
    const temp_def_map &temp_defs) {
    if (ic.op != icode_op::SET_VALUE_AT)
        return {};

    std::string base;
    if (available_byte_extract_data_base(ic.result, temp_defs, base, false))
        return base;
    return {};
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

static void available_byte_load_invalidate_store(
    std::unordered_map<std::string, available_byte_load_entry> &avail,
    const icode &ic,
    const temp_def_map &temp_defs) {
    std::string base = available_byte_direct_store_base(ic, temp_defs);
    if (base.empty()) {
        avail.clear();
        return;
    }

    const std::string direct_prefix = "LOAD8|" + base + "|";
    for (auto it = avail.begin(); it != avail.end();) {
        const bool same_direct_object =
            it->first.rfind(direct_prefix, 0) == 0;
        const bool pointer_may_alias =
            it->first.rfind("LOAD8PTR|", 0) == 0;
        if (same_direct_object || pointer_may_alias)
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
    explicit available_byte_load_pass(bool cross_block)
        : cross_block_(cross_block) {}

    const char *name() const override { return "available_byte_load"; }

    bool run(ir_function &fn) override {
        if (fn.icodes.empty())
            return false;

        std::unordered_map<int, const icode *> temp_defs;
        for (const auto &ic : fn.icodes) {
            if (defines_result(ic) && ic.result.is_temp())
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

                if (ic.op == icode_op::SET_VALUE_AT) {
                    available_byte_load_invalidate_store(current, ic,
                                                         temp_defs);
                    continue;
                }

                if (defines_result(ic))
                    available_byte_load_invalidate_operand(current, ic.result);

                std::string key = available_byte_load_key(ic, temp_defs);
                if (!key.empty()) {
                    auto it = current.find(key);
                    if (it == current.end()) {
                        current.emplace(key, available_byte_load_entry{
                                                ic.result,
                                                available_byte_load_deps(ic,
                                                                        temp_defs)});
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

            // The fixed-point predecessor intersection is the memory version
            // proof: an entry survives only when every reachable predecessor
            // provides the same value, while calls and possibly-aliasing stores
            // kill it in transfer().  Seed rewriting with that proven state so
            // speed/balanced profiles can forward across block boundaries.
            // -Os starts empty because extending a value through a join can
            // increase register pressure and code size.
            avail_map current = cross_block_ ? in[block_id] : avail_map{};
            const auto &block = cfg.block(block_id);
            for (size_t i = block.begin; i < block.end; ++i) {
                icode &ic = fn.icodes[i];

                if (is_available_byte_load_barrier(ic)) {
                    current.clear();
                    continue;
                }

                if (ic.op == icode_op::SET_VALUE_AT) {
                    available_byte_load_invalidate_store(current, ic,
                                                         temp_defs);
                    continue;
                }

                if (defines_result(ic))
                    available_byte_load_invalidate_operand(current, ic.result);

                std::string key = available_byte_load_key(ic, temp_defs);
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
                    available_byte_load_deps(ic, temp_defs)
                };
            }
        }

        return rewritten;
    }

private:
    bool cross_block_;
};

struct available_word_load_entry {
    operand value;
    std::vector<std::string> deps;
};

struct available_word_mem_ref {
    std::string base_key;
    int offset = 0;
    std::vector<std::string> deps;
};

static bool is_available_word_load_barrier(const icode &ic) {
    switch (ic.op) {
    case icode_op::FUNCTION:
    case icode_op::ENDFUNCTION:
    case icode_op::CALL:
    case icode_op::INLINE_ASM:
    case icode_op::RECEIVE:
    case icode_op::SEND:
    case icode_op::ALLOCA:
        return true;
    case icode_op::ASSIGN:
        return ic.result.is_symbol() &&
               (ic.result.is_global || ic.result.is_tls || ic.result.is_sfr);
    default:
        return false;
    }
}

static bool is_available_word_load_candidate(const icode &ic) {
    int load_size = 0;
    if (ic.result.type)
        load_size = ic.result.type->size();
    if (load_size <= 0 &&
        ic.left.type &&
        ic.left.type->is_ptr() &&
        ic.left.type->base) {
        load_size = ic.left.type->base->size();
    }
    return ic.op == icode_op::GET_VALUE_AT &&
           ic.result.kind != operand_kind::NONE &&
           load_size == 2 &&
           ic.right.is_none() &&
           !local_cse_operand_key(ic.left).empty();
}

static bool is_available_word_store_candidate(const icode &ic) {
    int store_size = 0;
    if (ic.left.type)
        store_size = ic.left.type->size();
    if (store_size <= 0 &&
        ic.result.type &&
        ic.result.type->is_ptr() &&
        ic.result.type->base) {
        store_size = ic.result.type->base->size();
    }
    return ic.op == icode_op::SET_VALUE_AT &&
           ic.right.is_none() &&
           ic.left.kind != operand_kind::NONE &&
           store_size == 2;
}

static std::optional<available_word_mem_ref> resolve_available_word_mem_ref(
    const operand &ptr,
    const std::unordered_map<int, const icode *> &temp_defs,
    int depth,
    std::unordered_set<int> &visiting) {
    if (depth > 6)
        return std::nullopt;

    auto make_leaf_ref = [&](const operand &leaf, int extra_offset) {
        std::string key = local_cse_operand_key(leaf);
        if (key.empty())
            return std::optional<available_word_mem_ref>{};
        available_word_mem_ref ref;
        ref.base_key = key;
        ref.offset = extra_offset;
        ref.deps.push_back(key);
        return std::optional<available_word_mem_ref>{ref};
    };

    if (ptr.is_temp()) {
        if (!visiting.insert(ptr.temp_id).second)
            return std::nullopt;

        auto erase_visiting = [&]() {
            visiting.erase(ptr.temp_id);
        };

        auto def_it = temp_defs.find(ptr.temp_id);
        if (def_it != temp_defs.end() && def_it->second) {
            const icode &def = *def_it->second;
            if (def.result.is_temp() && def.result.temp_id == ptr.temp_id) {
                switch (def.op) {
                case icode_op::ASSIGN:
                case icode_op::CAST: {
                    auto ref = resolve_available_word_mem_ref(
                        def.left, temp_defs, depth + 1, visiting);
                    erase_visiting();
                    return ref;
                }
                case icode_op::ADDRESS_OF:
                    if (def.left.kind == operand_kind::SYMBOL) {
                        operand base = def.left;
                        int offset = def.left.byte_offset;
                        base.byte_offset = 0;
                        auto ref = make_leaf_ref(base, offset);
                        erase_visiting();
                        return ref;
                    }
                    break;
                case icode_op::ADD:
                case icode_op::SUB: {
                    auto resolve_with_delta = [&](const operand &base,
                                                  const operand &delta,
                                                  int sign) {
                        if (delta.kind != operand_kind::INT_CONST)
                            return std::optional<available_word_mem_ref>{};
                        auto ref = resolve_available_word_mem_ref(
                            base, temp_defs, depth + 1, visiting);
                        if (!ref)
                            return ref;
                        int64_t off64 = static_cast<int64_t>(ref->offset) +
                                        sign * delta.ival;
                        if (off64 < std::numeric_limits<int>::min() ||
                            off64 > std::numeric_limits<int>::max()) {
                            return std::optional<available_word_mem_ref>{};
                        }
                        ref->offset = static_cast<int>(off64);
                        return ref;
                    };

                    auto ref = resolve_with_delta(def.left, def.right, +1);
                    if (!ref && def.op == icode_op::ADD)
                        ref = resolve_with_delta(def.right, def.left, +1);
                    if (!ref && def.op == icode_op::SUB)
                        ref = resolve_with_delta(def.left, def.right, -1);
                    if (ref) {
                        erase_visiting();
                        return ref;
                    }
                    // A dynamic address expression cannot be reduced to a
                    // base+constant key, but the SSA temp itself is still an
                    // exact, stable address within its live range. Fall back
                    // to that identity so repeated loads through the same
                    // pointer temp can be forwarded safely.
                    break;
                }
                default:
                    break;
                }
            }
        }

        erase_visiting();
    }

    return make_leaf_ref(ptr, 0);
}

static std::string available_word_load_key(
    const icode &ic,
    const std::unordered_map<int, const icode *> &temp_defs) {
    if (!is_available_word_load_candidate(ic))
        return {};
    std::unordered_set<int> visiting;
    auto ref = resolve_available_word_mem_ref(ic.left, temp_defs, 0, visiting);
    if (!ref)
        return {};
    return "LOAD16PTR|" + ref->base_key + "|" + std::to_string(ref->offset);
}

static std::string available_word_store_key(
    const icode &ic,
    const std::unordered_map<int, const icode *> &temp_defs) {
    if (!is_available_word_store_candidate(ic))
        return {};
    std::unordered_set<int> visiting;
    auto ref = resolve_available_word_mem_ref(ic.result, temp_defs, 0, visiting);
    if (!ref)
        return {};
    return "LOAD16PTR|" + ref->base_key + "|" + std::to_string(ref->offset);
}

static std::vector<std::string> available_word_load_deps(
    const icode &ic,
    const std::unordered_map<int, const icode *> &temp_defs) {
    std::unordered_set<int> visiting;
    auto ref = resolve_available_word_mem_ref(ic.left, temp_defs, 0, visiting);
    return ref ? ref->deps : std::vector<std::string>{};
}

static std::vector<std::string> available_word_store_deps(
    const icode &ic,
    const std::unordered_map<int, const icode *> &temp_defs) {
    std::unordered_set<int> visiting;
    auto ref = resolve_available_word_mem_ref(ic.result, temp_defs, 0, visiting);
    return ref ? ref->deps : std::vector<std::string>{};
}

static void available_word_load_invalidate_operand(
    std::unordered_map<std::string, available_word_load_entry> &avail,
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

static bool available_word_load_maps_equal(
    const std::unordered_map<std::string, available_word_load_entry> &lhs,
    const std::unordered_map<std::string, available_word_load_entry> &rhs) {
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

class available_word_load_pass final : public ir_pass {
public:
    explicit available_word_load_pass(bool cross_block)
        : cross_block_(cross_block) {}

    const char *name() const override { return "available_word_load"; }

    bool run(ir_function &fn) override {
        if (fn.icodes.empty())
            return false;

        auto has_fp_or_complex = [](const operand &op) {
            return op.type &&
                   (op.type->kind == type_kind::FLOAT ||
                    op.type->is_complex());
        };
        for (const auto &ic : fn.icodes) {
            switch (ic.op) {
            case icode_op::FADD:
            case icode_op::FSUB:
            case icode_op::FMUL:
            case icode_op::FDIV:
            case icode_op::FITOSF:
            case icode_op::FSTOI:
            case icode_op::MAKE_COMPLEX:
                return false;
            default:
                break;
            }
            if (has_fp_or_complex(ic.result) ||
                has_fp_or_complex(ic.left) ||
                has_fp_or_complex(ic.right)) {
                return false;
            }
        }

        std::unordered_map<int, const icode *> temp_defs;
        for (const auto &ic : fn.icodes) {
            if (defines_result(ic) && ic.result.is_temp())
                temp_defs[ic.result.temp_id] = &ic;
        }

        control_flow_graph cfg(fn);
        auto reachable = cfg.reachable_blocks();
        auto order = cfg.reverse_postorder();
        const size_t nblocks = cfg.blocks().size();

        using avail_map = std::unordered_map<std::string, available_word_load_entry>;
        std::vector<avail_map> in(nblocks), out(nblocks);

        auto transfer = [&](const basic_block &block, const avail_map &start) {
            avail_map current = start;
            for (size_t i = block.begin; i < block.end; ++i) {
                const icode &ic = fn.icodes[i];

                if (is_available_word_load_barrier(ic)) {
                    current.clear();
                    continue;
                }

                if (ic.op == icode_op::SET_VALUE_AT) {
                    current.clear();
                    std::string key = available_word_store_key(ic, temp_defs);
                    if (!key.empty()) {
                        current[key] = available_word_load_entry{
                            ic.left,
                            available_word_store_deps(ic, temp_defs)
                        };
                    }
                    continue;
                }

                if (defines_result(ic))
                    available_word_load_invalidate_operand(current, ic.result);

                std::string key = available_word_load_key(ic, temp_defs);
                if (!key.empty()) {
                    auto it = current.find(key);
                    if (it == current.end()) {
                        current.emplace(key, available_word_load_entry{
                                                 ic.result,
                                                 available_word_load_deps(ic, temp_defs)});
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

                if (!available_word_load_maps_equal(in[block_id], merged)) {
                    in[block_id] = merged;
                    changed = true;
                }

                avail_map new_out = transfer(block, in[block_id]);
                if (!available_word_load_maps_equal(out[block_id], new_out)) {
                    out[block_id] = std::move(new_out);
                    changed = true;
                }
            }
        }

        bool rewritten = false;
        for (size_t block_id : order) {
            if (!reachable.count(block_id))
                continue;

            // As in the byte pass, the predecessor intersection plus the
            // conservative all-memory kill for indirect word stores forms the
            // cross-block memory-version proof. As above, -Os deliberately
            // keeps this local to avoid extending live ranges through joins.
            avail_map current = cross_block_ ? in[block_id] : avail_map{};
            const auto &block = cfg.block(block_id);
            for (size_t i = block.begin; i < block.end; ++i) {
                icode &ic = fn.icodes[i];

                if (is_available_word_load_barrier(ic)) {
                    current.clear();
                    continue;
                }

                if (ic.op == icode_op::SET_VALUE_AT) {
                    current.clear();
                    std::string key = available_word_store_key(ic, temp_defs);
                    if (!key.empty()) {
                        current[key] = available_word_load_entry{
                            ic.left,
                            available_word_store_deps(ic, temp_defs)
                        };
                    }
                    continue;
                }

                if (defines_result(ic))
                    available_word_load_invalidate_operand(current, ic.result);

                std::string key = available_word_load_key(ic, temp_defs);
                if (key.empty())
                    continue;

                auto it = current.find(key);
                if (it != current.end()) {
                    ic.op = icode_op::ASSIGN;
                    ic.left = it->second.value;
                    ic.right = operand::make_none();
                    rewritten = true;
                }

                current[key] = available_word_load_entry{
                    ic.result,
                    available_word_load_deps(ic, temp_defs)
                };
            }
        }

        return rewritten;
    }

private:
    bool cross_block_;
};

class local_word_store_forward_pass final : public ir_pass {
public:
    const char *name() const override { return "local_word_store_forward"; }

    bool run(ir_function &fn) override {
        if (fn.icodes.empty())
            return false;

        std::unordered_map<int, const icode *> temp_defs;
        for (const auto &ic : fn.icodes) {
            if (defines_result(ic) && ic.result.is_temp())
                temp_defs[ic.result.temp_id] = &ic;
        }

        auto clear_window = [&](const icode &ic) {
            switch (ic.op) {
            case icode_op::FUNCTION:
            case icode_op::ENDFUNCTION:
            case icode_op::LABEL:
            case icode_op::GOTO:
            case icode_op::IFX:
            case icode_op::RETURN:
            case icode_op::CALL:
            case icode_op::INLINE_ASM:
            case icode_op::RECEIVE:
            case icode_op::SEND:
            case icode_op::ALLOCA:
                return true;
            case icode_op::ASSIGN:
                return ic.result.is_symbol() &&
                       (ic.result.is_global || ic.result.is_tls ||
                        ic.result.is_sfr);
            default:
                return false;
            }
        };

        using avail_map =
            std::unordered_map<std::string, available_word_load_entry>;
        avail_map current;
        bool changed = false;

        for (auto &ic : fn.icodes) {
            if (clear_window(ic)) {
                current.clear();
                continue;
            }

            if (ic.op == icode_op::SET_VALUE_AT) {
                current.clear();
                std::string key = available_word_store_key(ic, temp_defs);
                if (!key.empty()) {
                    current[key] = available_word_load_entry{
                        ic.left,
                        available_word_store_deps(ic, temp_defs)};
                }
                continue;
            }

            if (defines_result(ic))
                available_word_load_invalidate_operand(current, ic.result);

            std::string key = available_word_load_key(ic, temp_defs);
            if (key.empty())
                continue;

            auto found = current.find(key);
            if (found != current.end()) {
                ic.op = icode_op::ASSIGN;
                ic.left = found->second.value;
                ic.right = operand::make_none();
                changed = true;
            }
        }

        return changed;
    }
};

// Split a delayed loop-carried scalar from an early function-wide temporary.
//
// Frontend lowering creates temps at declaration sites.  An accumulator that
// is declared near function entry but first touched in a much later loop then
// appears live across all intervening control flow, even though its initial
// value could be materialized at the loop preheader. On a register-poor target
// that artificial lifetime is especially expensive: one unrelated early clobber
// forces every hot-loop access through the frame.
//
// This pass moves the constant initializer into a fresh temp at the outermost
// natural-loop preheader containing the first update and renames the dominated
// remainder of the live range. It is deliberately terminal: earlier placement
// artificially joins the allocation regions again. The dominance and
// single-preheader checks make the move valid for zero-trip loops and for
// values used after loop exit.
class loop_live_range_split_pass final : public ir_pass {
public:
    const char *name() const override { return "loop_live_range_split"; }

    bool run(ir_function &fn) override {
        if (fn.icodes.empty())
            return false;

        control_flow_graph cfg(fn);
        auto loops = cfg.natural_loops();
        if (loops.empty())
            return false;
        const auto dominators = cfg.dominators();

        std::vector<size_t> instruction_block(fn.icodes.size(), 0);
        for (const auto &block : cfg.blocks()) {
            for (size_t i = block.begin; i < block.end; ++i)
                instruction_block[i] = block.id;
        }

        struct temp_info {
            size_t init = static_cast<size_t>(-1);
            operand original;
            std::vector<size_t> mentions;
            std::vector<size_t> definitions;
        };
        std::unordered_map<int, temp_info> temps;

        for (size_t i = 0; i < fn.icodes.size(); ++i) {
            const icode &ic = fn.icodes[i];
            if (ic.result.is_temp() && defines_result(ic)) {
                auto &info = temps[ic.result.temp_id];
                info.definitions.push_back(i);
                if (info.init == static_cast<size_t>(-1) &&
                    ic.op == icode_op::ASSIGN &&
                    ic.left.kind == operand_kind::INT_CONST &&
                    ic.right.is_none() && ic.result.type &&
                    ic.result.type->is_integer() &&
                    (ic.result.type->size() == 1 ||
                     ic.result.type->size() == 2)) {
                    info.init = i;
                    info.original = ic.result;
                }
            }
            auto note_use = [&](const operand &op) {
                if (op.is_temp())
                    temps[op.temp_id].mentions.push_back(i);
            };
            for_each_use_operand(ic, note_use);
        }

        int next_temp = next_temp_id(fn);
        std::map<size_t, std::vector<icode>> insert_before;
        std::unordered_set<size_t> remove_initializers;
        bool changed = false;

        for (auto &[tid, info] : temps) {
            if (info.init == static_cast<size_t>(-1) ||
                info.definitions.size() < 2 || info.mentions.empty()) {
                continue;
            }

            // The initializer must be the first definition.  Subsequent
            // definitions are the loop-carried updates that will be renamed.
            if (info.definitions.front() != info.init)
                continue;

            size_t first_mention = static_cast<size_t>(-1);
            for (size_t index : info.mentions) {
                if (index > info.init) {
                    first_mention = std::min(first_mention, index);
                }
            }
            if (first_mention == static_cast<size_t>(-1))
                continue;

            // Select the outermost natural loop containing the first use and
            // every later definition.  Choosing an inner loop would reset an
            // accumulator once per enclosing iteration and change semantics.
            const natural_loop *selected = nullptr;
            const size_t first_block = instruction_block[first_mention];
            for (const auto &loop : loops) {
                if (loop.outside_preds.size() != 1 ||
                    !loop.blocks.count(first_block)) {
                    continue;
                }
                bool contains_updates = true;
                for (size_t def_index : info.definitions) {
                    if (def_index == info.init)
                        continue;
                    if (!loop.blocks.count(instruction_block[def_index])) {
                        contains_updates = false;
                        break;
                    }
                }
                if (!contains_updates)
                    continue;
                if (!selected || loop.blocks.size() > selected->blocks.size())
                    selected = &loop;
            }
            if (!selected)
                continue;

            bool has_call = false;
            for (size_t block_id : selected->blocks) {
                const auto &block = cfg.block(block_id);
                for (size_t i = block.begin; i < block.end; ++i) {
                    if (fn.icodes[i].op == icode_op::CALL) {
                        has_call = true;
                        break;
                    }
                }
                if (has_call)
                    break;
            }
            if (has_call)
                continue;

            const size_t preheader_id = selected->outside_preds.front();
            const auto &preheader = cfg.block(preheader_id);
            const size_t insert_at =
                insertion_index_before_terminator(preheader, fn);
            if (insert_at <= info.init + 8)
                continue;

            const size_t init_block = instruction_block[info.init];
            if (preheader_id >= dominators.size() ||
                !dominators[preheader_id].count(init_block)) {
                continue;
            }

            // Every mention must execute after the moved initializer. This also
            // rejects side exits that use the original value without passing
            // through the selected preheader.
            bool dominated = true;
            for (size_t index : info.mentions) {
                if (index == info.init)
                    continue;
                const size_t block_id = instruction_block[index];
                if (block_id == preheader_id) {
                    if (index < insert_at)
                        dominated = false;
                } else if (block_id >= dominators.size() ||
                           !dominators[block_id].count(preheader_id)) {
                    dominated = false;
                }
                if (!dominated)
                    break;
            }
            if (!dominated)
                continue;

            operand split_temp = make_fresh_temp(next_temp, info.original.type);
            icode split;
            split.op = icode_op::ASSIGN;
            split.result = split_temp;
            split.left = fn.icodes[info.init].left;
            split.line = fn.icodes[info.init].line;
            insert_before[insert_at].push_back(split);
            remove_initializers.insert(info.init);

            auto rename = [&](operand &op) {
                if (op.is_temp() && op.temp_id == tid)
                    op = split_temp;
            };
            for (size_t i = 0; i < fn.icodes.size(); ++i) {
                if (i == info.init)
                    continue;
                icode &ic = fn.icodes[i];
                if (defines_result(ic))
                    rename(ic.result);
                for_each_use_operand(ic, rename);
            }
            changed = true;
        }

        if (!changed)
            return false;
        fn.icodes = rebuild_with_insertions(fn.icodes, remove_initializers,
                                            insert_before);
        return true;
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
    explicit loop_induction_pass(bool avoid_call_spills)
        : avoid_call_spills_(avoid_call_spills) {}

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
            bool loop_has_call = false;
            for (size_t block_id : loop.blocks) {
                const auto &block = cfg.block(block_id);
                for (size_t i = block.begin; i < block.end; ++i) {
                    if (fn.icodes[i].op == icode_op::CALL) {
                        loop_has_call = true;
                        break;
                    }
                }
                if (loop_has_call)
                    break;
            }

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
                        const uint64_t magnitude = static_cast<uint64_t>(
                            factor < 0 ? -factor : factor);
                        if (avoid_call_spills_ && loop_has_call &&
                            magnitude <= 4 &&
                            (magnitude & (magnitude - 1)) == 0) {
                            // Calls force loop-carried values out of Z80
                            // registers. Recomputing a one- or two-bit scale
                            // is cheaper than maintaining another spilled
                            // induction variable across every call.
                            continue;
                        }

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

private:
    bool avoid_call_spills_ = false;
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

class bitwise_select_simplify_pass final : public ir_pass {
public:
    const char *name() const override { return "bitwise_select_simplify"; }

    bool run(ir_function &fn) override {
        // Canonical frontend lowering of
        //
        //   (selector & when_set) | (~selector & when_clear)
        //
        // uses two in-place temporary chains.  The equivalent
        //
        //   when_clear ^ (selector & (when_set ^ when_clear))
        //
        // removes one full-width boolean operation.  Require the exact
        // adjacent def/use chain so no intervening observation, volatile
        // access, or alias question is involved.
        for (size_t i = 0; i + 3 < fn.icodes.size(); ++i) {
            icode &first_and = fn.icodes[i];
            icode &complement = fn.icodes[i + 1];
            icode &second_and = fn.icodes[i + 2];
            icode &combine = fn.icodes[i + 3];
            if (first_and.op != icode_op::BAND ||
                complement.op != icode_op::BNOT ||
                second_and.op != icode_op::BAND ||
                combine.op != icode_op::BOR ||
                !first_and.result.is_temp() ||
                !complement.result.is_temp() ||
                !second_and.result.is_temp() ||
                !combine.result.is_temp() ||
                combine.result.temp_id != first_and.result.temp_id ||
                second_and.result.temp_id != complement.result.temp_id ||
                !first_and.result.type ||
                !first_and.result.type->is_integer() ||
                (first_and.result.type->size() != 1 &&
                 first_and.result.type->size() != 2 &&
                 first_and.result.type->size() != 4 &&
                 first_and.result.type->size() != 8)) {
                continue;
            }

            const operand selector = complement.left;
            const operand *when_set = nullptr;
            if (same_value_operand(first_and.left, selector))
                when_set = &first_and.right;
            else if (same_value_operand(first_and.right, selector))
                when_set = &first_and.left;
            if (!when_set)
                continue;

            const operand *when_clear = nullptr;
            const bool complement_on_left =
                second_and.left.is_temp() &&
                second_and.left.temp_id == complement.result.temp_id;
            const bool complement_on_right =
                second_and.right.is_temp() &&
                second_and.right.temp_id == complement.result.temp_id;
            if (complement_on_left != complement_on_right)
                when_clear = complement_on_left ? &second_and.right
                                                : &second_and.left;
            if (!when_clear)
                continue;

            const bool combine_matches =
                ((combine.left.is_temp() &&
                  combine.left.temp_id == first_and.result.temp_id &&
                  combine.right.is_temp() &&
                  combine.right.temp_id == second_and.result.temp_id) ||
                 (combine.right.is_temp() &&
                  combine.right.temp_id == first_and.result.temp_id &&
                  combine.left.is_temp() &&
                  combine.left.temp_id == second_and.result.temp_id));
            if (!combine_matches)
                continue;

            first_and.op = icode_op::BXOR;
            first_and.left = *when_set;
            first_and.right = *when_clear;

            complement.op = icode_op::BAND;
            complement.left = selector;
            complement.right = first_and.result;

            combine.op = icode_op::BXOR;
            combine.left = *when_clear;
            combine.right = complement.result;

            fn.icodes.erase(fn.icodes.begin() +
                            static_cast<std::ptrdiff_t>(i + 2));
            return true;
        }
        return false;
    }
};

// Keep an explicitly truncated byte expression byte-wide from the start.
//
// Integer promotions make even a simple
//
//     (unsigned char)(left + right)
//
// arrive here as a 16-bit ADD followed by a one-byte CAST.  On Z80 that can
// turn one accumulator instruction into a pair of zero extensions, a
// push/pop shuffle, and a discarded high-byte operation.  The low byte of
// ADD, SUB, bitwise operations, unary negation/complement, and a bounded left
// shift depends only on the low byte of their operands, so it is equivalent
// to perform the operation at the destination width.  A bounded right shift
// has the same property when the original byte source is unsigned: integer
// promotion zero-extends it, so a logical byte shift produces the same low
// byte as the promoted word shift.
//
// This deliberately handles only one adjacent producer/cast pair whose
// temporary has one definition and one use, plus the equally local forms in
// which an immediately preceding cast widens the producer's byte input (with
// either a distinct or in-place temporary).  In particular, it does not
// propagate a byte type through a CFG or into an induction variable; the
// older broad promoted-byte pass remains opt-in because doing that without
// complete phi/liveness modelling can narrow loop-carried full-width state.
class direct_truncated_byte_ops_pass final : public ir_pass {
public:
    const char *name() const override { return "direct_truncated_byte_ops"; }

    bool run(ir_function &fn) override {
        if (fn.icodes.size() < 2)
            return false;

        std::unordered_map<int, int> temp_def_count;
        std::unordered_map<int, int> temp_use_count;
        std::unordered_map<int, const icode *> temp_defs;
        std::unordered_set<std::string> volatile_symbols;
        for (const auto &candidate : fn.icodes) {
            if (defines_result(candidate) && candidate.result.is_temp()) {
                ++temp_def_count[candidate.result.temp_id];
                temp_defs[candidate.result.temp_id] = &candidate;
            }
            auto remember_volatile_symbol = [&](const operand &op) {
                if (op.is_symbol() && op.type && op.type->is_volatile)
                    volatile_symbols.insert(base_symbol_key(op));
            };
            remember_volatile_symbol(candidate.result);
            remember_volatile_symbol(candidate.left);
            remember_volatile_symbol(candidate.right);
            for_each_use_operand(candidate, [&](const operand &op) {
                if (op.is_temp())
                    ++temp_use_count[op.temp_id];
            });
        }

        auto is_plain_integer_value = [&](const operand &op) {
            if (op.kind == operand_kind::INT_CONST)
                return true;
            if (!op.type || !op.type->is_integer() || op.type->is_volatile)
                return false;
            if (op.is_symbol() &&
                volatile_symbols.count(base_symbol_key(op)) != 0)
                return false;
            return true;
        };

        std::function<std::optional<operand>(const operand &, int)>
            resolve_low_byte_source;
        resolve_low_byte_source = [&](const operand &op, int depth)
            -> std::optional<operand> {
            if (depth <= 4 && op.is_temp() &&
                temp_def_count[op.temp_id] == 1) {
                auto found = temp_defs.find(op.temp_id);
                if (found != temp_defs.end() && found->second &&
                    found->second->op == icode_op::CAST &&
                    found->second->result.type &&
                    found->second->result.type->is_integer()) {
                    auto source = resolve_low_byte_source(
                        found->second->left, depth + 1);
                    if (source)
                        return source;
                }
            }
            if (is_plain_integer_value(op))
                return op;
            return std::nullopt;
        };

        auto narrow_operand = [](operand &op, const type_ptr &byte_type) {
            if (op.kind == operand_kind::INT_CONST) {
                op = operand::make_int(
                    static_cast<int64_t>(
                        static_cast<uint64_t>(op.ival) & 0xffu),
                    byte_type);
            } else if (!op.type || op.type->size() != 1) {
                op.type = byte_type;
            }
        };

        auto is_byte_ready = [](const operand &op) {
            return op.kind == operand_kind::INT_CONST ||
                   (op.type && op.type->is_integer() &&
                    op.type->size() == 1);
        };

        auto is_unsigned_byte_ready = [](const operand &op) {
            if (op.kind == operand_kind::INT_CONST)
                return op.ival >= 0 && op.ival <= 255;
            return op.type && op.type->is_integer() &&
                   op.type->size() == 1 && op.type->is_unsigned();
        };

        bool changed = false;
        for (size_t i = 0; i + 1 < fn.icodes.size(); ++i) {
            icode &producer = fn.icodes[i];
            icode &truncate = fn.icodes[i + 1];

            if (!producer.result.is_temp() || !producer.result.type ||
                !producer.result.type->is_integer() ||
                producer.result.type->size() <= 1 ||
                producer.result.type->is_volatile) {
                continue;
            }

            const int temp_id = producer.result.temp_id;
            icode *adjacent_widen = nullptr;
            if (temp_def_count[temp_id] != 1 ||
                temp_use_count[temp_id] != 1) {
                if (i == 0 || temp_def_count[temp_id] != 2 ||
                    temp_use_count[temp_id] != 2 ||
                    !producer.left.is_temp() ||
                    producer.left.temp_id != temp_id) {
                    continue;
                }
                icode &candidate = fn.icodes[i - 1];
                if (candidate.op != icode_op::CAST ||
                    !candidate.result.is_temp() ||
                    candidate.result.temp_id != temp_id ||
                    !candidate.result.type ||
                    !candidate.result.type->is_integer() ||
                    candidate.result.type->size() <= 1 ||
                    candidate.result.type->is_volatile ||
                    !is_byte_ready(candidate.left) ||
                    (candidate.left.is_temp() &&
                     candidate.left.temp_id == temp_id)) {
                    continue;
                }
                adjacent_widen = &candidate;
            }
            if (truncate.op != icode_op::CAST ||
                !truncate.left.is_temp() ||
                truncate.left.temp_id != temp_id ||
                !truncate.result.type ||
                !truncate.result.type->is_integer() ||
                truncate.result.type->size() != 1) {
                continue;
            }

            bool supported = false;
            bool binary = false;
            bool logical_right_shift = false;
            switch (producer.op) {
            case icode_op::ADD:
            case icode_op::SUB:
            case icode_op::BAND:
            case icode_op::BOR:
            case icode_op::BXOR:
                supported = true;
                binary = true;
                break;
            case icode_op::SHL:
                supported = producer.right.kind == operand_kind::INT_CONST &&
                            producer.right.ival >= 0 &&
                            producer.right.ival <= 7;
                binary = false;
                break;
            case icode_op::SHR:
                supported = producer.right.kind == operand_kind::INT_CONST &&
                            producer.right.ival >= 0 &&
                            producer.right.ival <= 7;
                binary = false;
                logical_right_shift = true;
                break;
            case icode_op::NEG:
            case icode_op::BNOT:
                supported = true;
                binary = false;
                break;
            default:
                break;
            }
            if (!supported)
                continue;
            if (!adjacent_widen && i > 0 && producer.left.is_temp()) {
                icode &candidate = fn.icodes[i - 1];
                const int source_temp = producer.left.temp_id;
                if (candidate.op == icode_op::CAST &&
                    candidate.result.is_temp() &&
                    candidate.result.temp_id == source_temp &&
                    candidate.result.type &&
                    candidate.result.type->is_integer() &&
                    candidate.result.type->size() > 1 &&
                    !candidate.result.type->is_volatile &&
                    temp_def_count[source_temp] == 1 &&
                    temp_use_count[source_temp] == 1 &&
                    is_byte_ready(candidate.left) &&
                    (!candidate.left.is_temp() ||
                     candidate.left.temp_id != source_temp)) {
                    adjacent_widen = &candidate;
                }
            }
            auto left = adjacent_widen
                ? std::optional<operand>(adjacent_widen->left)
                : resolve_low_byte_source(producer.left, 0);
            if (!left || !is_byte_ready(*left) ||
                (logical_right_shift &&
                 !is_unsigned_byte_ready(*left))) {
                continue;
            }
            std::optional<operand> right;
            if (binary) {
                right = resolve_low_byte_source(producer.right, 0);
                if (!right || !is_byte_ready(*right))
                    continue;
            }

            const type_ptr byte_type = logical_right_shift
                                           ? type::make_uchar()
                                           : truncate.result.type->unqual();
            if (adjacent_widen &&
                (producer.op == icode_op::SHL ||
                 producer.op == icode_op::SHR) &&
                adjacent_widen->result.temp_id != producer.result.temp_id) {
                // A distinct, one-use widening temporary is pure scaffolding
                // around a byte shift.  Bypass it instead of merely retyping
                // it: leaving the copy in place can hide carry-based
                // accumulator idioms such as an immediately shifted/XORed
                // byte from the backend peephole pass.  Other operations keep
                // their byte copy because it can shorten allocation live
                // ranges.  The ordinary DCE pass consumes the now-dead CAST
                // on the next fixed-point round.
                producer.left = adjacent_widen->left;
                narrow_operand(producer.left, byte_type);
            } else if (adjacent_widen) {
                // The in-place form defines the same virtual state consumed
                // and redefined by the producer, so its initial load cannot
                // be bypassed.  Retype that state without duplicating it.
                adjacent_widen->result.type = byte_type;
                producer.left.type = byte_type;
            } else {
                producer.left = *left;
                narrow_operand(producer.left, byte_type);
            }
            if (binary)
                producer.right = *right;
            if (binary)
                narrow_operand(producer.right, byte_type);
            producer.result.type = byte_type;
            truncate.left.type = byte_type;
            changed = true;
        }
        return changed;
    }
};

// Canonicalize a branchless unsigned-byte MSB mask into the equivalent
// conditional shift/XOR diamond:
//
//   (value << 1) ^ ((0 - (value >> 7)) & polynomial)
//
// For an unsigned byte, value>>7 is exactly zero or one and its negation is
// therefore 0x00 or 0xff after byte truncation.  The diamond is a better Z80
// canonical form because ADD A,A computes the shifted byte and exposes the
// original MSB in carry; the backend can then use one carry branch and XOR.
// Keep the recognition strictly local and require every mask scaffold value
// to have one definition/use.  Stack locals are accepted only when their
// address is never taken, allowing ordinary DCE/frame compaction to remove
// them after the rewrite.
class msb_masked_shift_xor_pass final : public ir_pass {
public:
    const char *name() const override { return "msb_masked_shift_xor"; }

    bool run(ir_function &fn) override {
        if (fn.icodes.size() < 5)
            return false;

        std::unordered_map<std::string, int> defs;
        std::unordered_map<std::string, int> uses;
        std::unordered_set<std::string> address_taken;
        auto key = [](const operand &op) {
            if (op.is_temp())
                return std::string("t:") + std::to_string(op.temp_id);
            if (op.is_symbol() && !op.is_global)
                return std::string("s:") + base_symbol_key(op);
            return std::string();
        };
        for (const icode &ic : fn.icodes) {
            const std::string result_key = key(ic.result);
            if (!result_key.empty() && ic.op != icode_op::SET_VALUE_AT)
                ++defs[result_key];
            for_each_use_operand(ic, [&](const operand &op) {
                const std::string use_key = key(op);
                if (!use_key.empty())
                    ++uses[use_key];
            });
            if (ic.op == icode_op::ADDRESS_OF) {
                const std::string address_key = key(ic.left);
                if (!address_key.empty())
                    address_taken.insert(address_key);
            }
        }

        auto private_single_use_value = [&](const operand &op) {
            const std::string value_key = key(op);
            if (value_key.empty() || defs[value_key] != 1 ||
                uses[value_key] != 1) {
                return false;
            }
            if (op.is_symbol() &&
                (op.is_global || op.is_param ||
                 address_taken.count(value_key) != 0)) {
                return false;
            }
            return !op.type || !op.type->is_volatile;
        };

        int next_temp = next_temp_id(fn);
        for (size_t i = 0; i + 4 < fn.icodes.size(); ++i) {
            const icode &shr = fn.icodes[i];
            const icode &negate = fn.icodes[i + 1];
            const icode &shl = fn.icodes[i + 2];
            const icode &band = fn.icodes[i + 3];
            const icode &bxor = fn.icodes[i + 4];

            if (shr.op != icode_op::SHR ||
                shr.right.kind != operand_kind::INT_CONST ||
                shr.right.ival != 7 ||
                !shr.left.type || !shr.left.type->is_integer() ||
                !shr.left.type->is_unsigned() || shr.left.type->size() != 1 ||
                !shr.result.type || shr.result.type->size() != 1 ||
                negate.op != icode_op::SUB ||
                negate.left.kind != operand_kind::INT_CONST ||
                (negate.left.ival & 0xff) != 0 ||
                !same_value_operand(negate.right, shr.result) ||
                !negate.result.type || negate.result.type->size() != 1 ||
                shl.op != icode_op::SHL ||
                shl.right.kind != operand_kind::INT_CONST ||
                shl.right.ival != 1 ||
                !same_value_operand(shl.left, shr.left) ||
                !shl.result.type || shl.result.type->size() != 1 ||
                band.op != icode_op::BAND ||
                !band.result.type || band.result.type->size() != 1 ||
                bxor.op != icode_op::BXOR ||
                !bxor.result.type || bxor.result.type->size() != 1) {
                continue;
            }

            const operand *polynomial = nullptr;
            if (same_value_operand(band.left, negate.result))
                polynomial = &band.right;
            else if (same_value_operand(band.right, negate.result))
                polynomial = &band.left;
            if (!polynomial || !polynomial->type ||
                !polynomial->type->is_integer() ||
                polynomial->type->size() != 1 ||
                polynomial->type->is_volatile) {
                continue;
            }
            const bool xor_matches =
                (same_value_operand(bxor.left, shl.result) &&
                 same_value_operand(bxor.right, band.result)) ||
                (same_value_operand(bxor.right, shl.result) &&
                 same_value_operand(bxor.left, band.result));
            const std::string input_key = key(shr.left);
            const bool safe_symbol_input =
                shr.left.is_symbol() && !shr.left.is_global &&
                (!shr.left.type || !shr.left.type->is_volatile) &&
                !input_key.empty() &&
                address_taken.count(input_key) == 0;
            if (!xor_matches ||
                (!shr.left.is_temp() && !safe_symbol_input) ||
                !bxor.result.is_temp() ||
                !private_single_use_value(shr.result) ||
                !private_single_use_value(negate.result) ||
                !private_single_use_value(shl.result) ||
                !private_single_use_value(band.result)) {
                continue;
            }

            bool input_used_after = false;
            for (size_t after = i + 5; after < fn.icodes.size(); ++after) {
                for_each_use_operand(fn.icodes[after],
                                     [&](const operand &op) {
                    if (same_value_operand(op, shr.left)) {
                        input_used_after = true;
                    }
                });
            }
            if (input_used_after)
                continue;

            const type_ptr byte_type = type::make_uchar();
            const operand condition = make_fresh_temp(next_temp, byte_type);
            const operand true_shift = make_fresh_temp(next_temp, byte_type);
            const operand phi = make_fresh_temp(next_temp, byte_type);
            const operand false_shift = make_fresh_temp(next_temp, byte_type);
            const std::string true_label =
                make_unique_label(fn, "__xcc_msb_mask_true_");
            const std::string false_label =
                make_unique_label(fn, "__xcc_msb_mask_false_");
            const std::string join_label =
                make_unique_label(fn, "__xcc_msb_mask_join_");

            std::vector<icode> replacement;
            replacement.reserve(11);

            icode condition_ic;
            condition_ic.op = icode_op::BAND;
            condition_ic.result = condition;
            condition_ic.left = shr.left;
            condition_ic.right = operand::make_int(0x80, byte_type);
            condition_ic.line = shr.line;
            replacement.push_back(condition_ic);

            icode branch;
            branch.op = icode_op::IFX;
            branch.left = condition;
            branch.true_lbl = true_label;
            branch.false_lbl = false_label;
            branch.line = shr.line;
            replacement.push_back(branch);

            icode label;
            label.op = icode_op::LABEL;
            label.label_name = true_label;
            replacement.push_back(label);

            icode true_shift_ic;
            true_shift_ic.op = icode_op::SHL;
            true_shift_ic.result = true_shift;
            true_shift_ic.left = shr.left;
            true_shift_ic.right = operand::make_int(1, type::make_int());
            true_shift_ic.line = shl.line;
            replacement.push_back(true_shift_ic);

            icode true_xor;
            true_xor.op = icode_op::BXOR;
            true_xor.result = phi;
            true_xor.left = true_shift;
            true_xor.right = *polynomial;
            true_xor.line = bxor.line;
            replacement.push_back(true_xor);

            icode jump;
            jump.op = icode_op::GOTO;
            jump.label_name = join_label;
            replacement.push_back(jump);

            label.label_name = false_label;
            replacement.push_back(label);

            icode false_shift_ic = true_shift_ic;
            false_shift_ic.result = false_shift;
            replacement.push_back(false_shift_ic);

            icode false_assign;
            false_assign.op = icode_op::ASSIGN;
            false_assign.result = phi;
            false_assign.left = false_shift;
            false_assign.line = bxor.line;
            replacement.push_back(false_assign);

            label.label_name = join_label;
            replacement.push_back(label);

            icode store;
            store.op = icode_op::ASSIGN;
            // A dead temporary input can carry the selected result in place,
            // which also permits consecutive steps to remain in A.  ABI-0
            // stack parameters are symbols rather than temporaries; keep
            // their C value unmodified and define the original expression
            // result instead.  The backend diamond fuser accepts either byte
            // target and can therefore return the result directly in L.
            const operand replacement_value =
                shr.left.is_temp() ? shr.left : bxor.result;
            store.result = replacement_value;
            store.left = phi;
            store.line = bxor.line;
            replacement.push_back(store);

            const int old_result_temp = bxor.result.temp_id;

            fn.icodes.erase(fn.icodes.begin() +
                                static_cast<std::ptrdiff_t>(i),
                            fn.icodes.begin() +
                                static_cast<std::ptrdiff_t>(i + 5));
            fn.icodes.insert(fn.icodes.begin() +
                                 static_cast<std::ptrdiff_t>(i),
                             replacement.begin(), replacement.end());

            for (size_t after = i + replacement.size();
                 after < fn.icodes.size(); ++after) {
                for_each_use_operand(fn.icodes[after], [&](operand &op) {
                    if (op.is_temp() &&
                        op.temp_id == old_result_temp) {
                        op = replacement_value;
                    }
                });
            }
            return true;
        }
        return false;
    }
};

class promoted_byte_ops_pass final : public ir_pass {
public:
    explicit promoted_byte_ops_pass(bool allow_low_byte_mul)
        : allow_low_byte_mul_(allow_low_byte_mul) {}

    const char *name() const override { return "promoted_byte_ops"; }

    bool run(ir_function &fn) override {
        std::unordered_map<int, const icode *> temp_defs;
        std::unordered_map<int, int> temp_use_count;
        std::unordered_map<int, std::vector<size_t>> temp_users;

        for (size_t i = 0; i < fn.icodes.size(); ++i) {
            const auto &ic = fn.icodes[i];
            if (ic.result.is_temp())
                temp_defs[ic.result.temp_id] = &ic;
            for_each_use_operand(ic, [&](const operand &op) {
                if (op.is_temp()) {
                    ++temp_use_count[op.temp_id];
                    temp_users[op.temp_id].push_back(i);
                }
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
                case icode_op::MUL:
                    return allow_low_byte_mul_ &&
                           operand_promotable(def->left) &&
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

        auto fits_low_byte_value = [](const operand &op) {
            if (op.kind == operand_kind::INT_CONST)
                return true;
            return op.type &&
                   op.type->is_integer() &&
                   !op.type->is_volatile;
        };

        auto unwrap_low_byte_input = [&](const operand &op) -> std::optional<operand> {
            if (!op.is_temp() && fits_low_byte_value(op))
                return op;
            if (!op.is_temp())
                return std::nullopt;
            auto it = temp_defs.find(op.temp_id);
            if (it == temp_defs.end())
                return std::nullopt;
            const icode *def = it->second;
            if (!def || def->op != icode_op::CAST || !def->result.type) {
                std::unordered_set<int> visiting;
                if (promotable_temp_expr(op.temp_id, visiting))
                    return op;
                if (fits_low_byte_value(op))
                    return op;
                return std::nullopt;
            }
            if (def->result.type->size() < 2 || !fits_low_byte_value(def->left))
                return std::nullopt;
            return def->left;
        };

        auto make_byte_operand = [](operand op, const type_ptr &type) {
            if (op.kind == operand_kind::INT_CONST)
                return operand::make_int(op.ival & 0xff, type);
            op.type = type;
            return op;
        };

        auto set_temp_use_type = [&](int temp_id, const type_ptr &type) {
            auto users = temp_users.find(temp_id);
            if (users == temp_users.end())
                return;
            for (size_t user_idx : users->second) {
                auto &candidate = fn.icodes[user_idx];
                if (candidate.left.is_temp() &&
                    candidate.left.temp_id == temp_id) {
                    candidate.left.type = type;
                }
                if (candidate.right.is_temp() &&
                    candidate.right.temp_id == temp_id) {
                    candidate.right.type = type;
                }
            }
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

            auto users = temp_users.find(temp_id);
            if (users == temp_users.end() || users->second.size() != 1)
                return false;
            const size_t user_idx = users->second.front();

            const auto &user = fn.icodes[user_idx];
            if (user.op == icode_op::CAST && user.result.type &&
                user.result.type->size() == 1)
                return true;
            if (user.op == icode_op::ASSIGN && user.result.type &&
                user.result.type->size() == 1)
                return true;
            if (stores_through_byte_ptr(user, temp_id))
                return true;
            if (user.op == icode_op::ASSIGN &&
                user.left.is_temp() &&
                user.left.temp_id == temp_id &&
                user.result.is_temp()) {
                return flows_to_byte_sink(user.result.temp_id, depth + 1);
            }

            switch (user.op) {
            case icode_op::ADD:
            case icode_op::SUB:
            case icode_op::BAND:
            case icode_op::BOR:
            case icode_op::BXOR:
                break;
            case icode_op::MUL:
                if (!allow_low_byte_mul_)
                    return false;
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
            if (!ic.result.is_temp() || !ic.result.type ||
                ic.result.type->size() <= 1)
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
            case icode_op::MUL:
                candidate = allow_low_byte_mul_;
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

            auto users = temp_users.find(ic.result.temp_id);
            if (users == temp_users.end() || users->second.size() != 1)
                continue;
            const size_t user_idx = users->second.front();
            if (user_idx <= i)
                continue;

            auto &user = fn.icodes[user_idx];
            const int produced_temp_id = ic.result.temp_id;
            const bool cast_consumer =
                user.op == icode_op::CAST && user.result.type &&
                user.result.type->size() == 1 &&
                user.left.is_temp() && user.left.temp_id == ic.result.temp_id;
            const bool byte_store_consumer =
                stores_through_byte_ptr(user, ic.result.temp_id);
            const bool assign_byte_sink_consumer =
                user.op == icode_op::ASSIGN &&
                user.left.is_temp() &&
                user.left.temp_id == ic.result.temp_id &&
                user.result.is_temp() &&
                flows_to_byte_sink(user.result.temp_id, 0);
            const bool direct_byte_assign_consumer =
                user.op == icode_op::ASSIGN &&
                user.left.is_temp() &&
                user.left.temp_id == ic.result.temp_id &&
                user.result.type && user.result.type->size() == 1;

            const bool byte_mask_truth_consumer =
                ic.op == icode_op::BAND &&
                user.op == icode_op::IFX &&
                user.left.is_temp() &&
                user.left.temp_id == ic.result.temp_id &&
                ((ic.left.kind == operand_kind::INT_CONST &&
                  (ic.left.ival & ~0xffLL) == 0) ||
                 (ic.right.kind == operand_kind::INT_CONST &&
                  (ic.right.ival & ~0xffLL) == 0));

            const bool byte_chain_consumer =
                ((user.left.is_temp() && user.left.temp_id == ic.result.temp_id) ||
                 (user.right.is_temp() && user.right.temp_id == ic.result.temp_id)) &&
                (user.op == icode_op::ADD || user.op == icode_op::SUB ||
                 (allow_low_byte_mul_ && user.op == icode_op::MUL) ||
                 user.op == icode_op::BAND ||
                 user.op == icode_op::BOR ||
                 user.op == icode_op::BXOR || user.op == icode_op::SHL ||
                 user.op == icode_op::SHR) &&
                ((user.result.type && user.result.type->size() == 1 &&
                  user.result.type->is_unsigned()) ||
                 (user.result.is_temp() &&
                  flows_to_byte_sink(user.result.temp_id, 0)));

            if (!cast_consumer && !byte_store_consumer &&
                !assign_byte_sink_consumer &&
                !direct_byte_assign_consumer && !byte_mask_truth_consumer &&
                !byte_chain_consumer) {
                continue;
            }

            const bool low_byte_only =
                ic.op == icode_op::ADD ||
                ic.op == icode_op::SUB ||
                (allow_low_byte_mul_ && ic.op == icode_op::MUL) ||
                ic.op == icode_op::BAND ||
                ic.op == icode_op::BOR ||
                ic.op == icode_op::BXOR ||
                ic.op == icode_op::SHL;

            auto lhs = low_byte_only ? unwrap_low_byte_input(ic.left)
                                     : unwrap_input(ic.left);
            if (!lhs)
                continue;

            std::optional<operand> rhs;
            if (ic.op == icode_op::SHL || ic.op == icode_op::SHR) {
                rhs = ic.right;
            } else {
                rhs = low_byte_only ? unwrap_low_byte_input(ic.right)
                                    : unwrap_input(ic.right);
                if (!rhs)
                    continue;
            }

            type_ptr byte_type = user.result.type;
            if (byte_chain_consumer &&
                (!byte_type || byte_type->size() != 1))
                byte_type = type::make_uchar();
            if (byte_store_consumer || byte_mask_truth_consumer ||
                assign_byte_sink_consumer || direct_byte_assign_consumer)
                byte_type = type::make_uchar();

            ic.left = make_byte_operand(*lhs, byte_type);
            if (rhs)
                ic.right = make_byte_operand(*rhs, byte_type);

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

                if (byte_store_consumer)
                    user.left.type = byte_type;
                if (assign_byte_sink_consumer) {
                    user.left.type = byte_type;
                    user.result.type = byte_type;
                    set_temp_use_type(user.result.temp_id, byte_type);
                }
                if (direct_byte_assign_consumer)
                    user.left.type = byte_type;
            }
            changed = true;
        }

        for (size_t i = 0; i < fn.icodes.size(); ++i) {
            auto &ic = fn.icodes[i];
            if (!ic.result.is_temp() || !ic.result.type ||
                ic.result.type->size() <= 1)
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

            const bool low_byte_only =
                ic.op == icode_op::BAND ||
                ic.op == icode_op::BOR ||
                ic.op == icode_op::BXOR ||
                ic.op == icode_op::SHL;

            auto lhs = low_byte_only ? unwrap_low_byte_input(ic.left)
                                     : unwrap_input(ic.left);
            if (!lhs)
                continue;

            std::optional<operand> rhs;
            if (ic.op == icode_op::SHL || ic.op == icode_op::SHR) {
                rhs = ic.right;
            } else {
                rhs = low_byte_only ? unwrap_low_byte_input(ic.right)
                                    : unwrap_input(ic.right);
                if (!rhs)
                    continue;
            }

            bool ok = true;
            bool saw_byte_sink = false;

            auto users = temp_users.find(produced_temp_id);
            if (users == temp_users.end())
                continue;
            size_t previous_user_idx = fn.icodes.size();
            for (size_t user_idx : users->second) {
                if (user_idx <= i || user_idx == previous_user_idx)
                    continue;
                previous_user_idx = user_idx;
                auto &user = fn.icodes[user_idx];

                if (user.op == icode_op::CAST && user.left.is_temp() &&
                    user.left.temp_id == produced_temp_id &&
                    user.result.type && user.result.type->size() == 1) {
                    saw_byte_sink = true;
                    continue;
                }
                if (stores_through_byte_ptr(user, produced_temp_id)) {
                    saw_byte_sink = true;
                    continue;
                }

                ok = false;
                break;
            }

            if (!ok || !saw_byte_sink)
                continue;

            type_ptr byte_type = type::make_uchar();
            ic.left = make_byte_operand(*lhs, byte_type);
            if (rhs)
                ic.right = make_byte_operand(*rhs, byte_type);
            ic.result.type = byte_type;

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

private:
    bool allow_low_byte_mul_ = false;
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
            if (!src_type ||
                (src_type->size() != 2 && src_type->size() != 4) ||
                !src_type->is_unsigned())
                continue;

            const int width = static_cast<int>(src_type->size() * 8);
            const int count_mask = width - 1;
            const int left_count =
                static_cast<int>(left_shift->right.ival) & count_mask;
            const int right_count =
                static_cast<int>(right_shift->right.ival) & count_mask;
            if (left_count <= 0 || right_count <= 0)
                continue;
            if (left_count + right_count != width)
                continue;

            rotate_count = left_count & count_mask;
            if (rotate_count == 0)
                continue;
            if (rotate_count > width / 2) {
                rotate_op = icode_op::ROR;
                rotate_count = width - rotate_count;
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

class adjacent_pack_word_load_pass final : public ir_pass {
public:
    const char *name() const override { return "adjacent_pack_word_load"; }

    bool run(ir_function &fn) override {
        std::unordered_map<int, const icode *> temp_defs;
        std::unordered_map<int, size_t> temp_def_index;
        std::unordered_map<int, int> temp_use_count;

        for (size_t i = 0; i < fn.icodes.size(); ++i) {
            const auto &ic = fn.icodes[i];
            if (ic.result.is_temp()) {
                temp_defs[ic.result.temp_id] = &ic;
                temp_def_index[ic.result.temp_id] = i;
            }
            for_each_use_operand(ic, [&](const operand &op) {
                if (op.is_temp())
                    ++temp_use_count[op.temp_id];
            });
        }

        auto is_byte_load_def = [&](const operand &op) -> const icode * {
            if (!op.is_temp())
                return nullptr;
            auto it = temp_defs.find(op.temp_id);
            if (it == temp_defs.end() || !it->second)
                return nullptr;
            const icode *def = it->second;
            if (def->op != icode_op::GET_VALUE_AT || !def->result.type ||
                def->result.type->size() != 1 || !def->right.is_none()) {
                return nullptr;
            }
            return def;
        };

        auto has_barrier_between = [&](size_t begin, size_t end, size_t skip0,
                                       size_t skip1) {
            for (size_t i = begin; i < end; ++i) {
                if (i == skip0 || i == skip1)
                    continue;
                const icode &scan = fn.icodes[i];
                if (scan.op == icode_op::SET_VALUE_AT ||
                    is_available_word_load_barrier(scan)) {
                    return true;
                }
            }
            return false;
        };

        bool changed = false;
        for (size_t i = 0; i < fn.icodes.size(); ++i) {
            auto &ic = fn.icodes[i];
            if (ic.op != icode_op::PACK_BYTES || !ic.result.type ||
                ic.result.type->size() != 2) {
                continue;
            }

            const icode *low_def = is_byte_load_def(ic.left);
            const icode *high_def = is_byte_load_def(ic.right);
            if (!low_def || !high_def)
                continue;

            if (temp_use_count[ic.left.temp_id] != 1 ||
                temp_use_count[ic.right.temp_id] != 1) {
                continue;
            }

            const size_t low_idx = temp_def_index[ic.left.temp_id];
            const size_t high_idx = temp_def_index[ic.right.temp_id];
            if (!(low_idx < high_idx && high_idx < i))
                continue;

            if (has_barrier_between(low_idx + 1, i, low_idx, high_idx))
                continue;

            std::unordered_set<int> visiting_lo;
            std::unordered_set<int> visiting_hi;
            auto low_ref =
                resolve_available_word_mem_ref(low_def->left, temp_defs, 0,
                                               visiting_lo);
            auto high_ref =
                resolve_available_word_mem_ref(high_def->left, temp_defs, 0,
                                               visiting_hi);
            if (!low_ref || !high_ref)
                continue;
            if (low_ref->base_key != high_ref->base_key ||
                high_ref->offset != low_ref->offset + 1) {
                continue;
            }

            ic.op = icode_op::GET_VALUE_AT;
            ic.left = low_def->left;
            ic.right = operand::make_none();
            changed = true;
        }

        return changed;
    }
};

// Fold a little-endian reconstruction from four adjacent unsigned bytes into
// one 32-bit load.  Besides fixed addresses, accept affine `base + index + K`
// forms so a parser loop can advance its byte and word cursors independently.
// The address normalizer treats multiply-defined temps as opaque atoms and
// the rewrite rejects any dependency redefinition, store, call, or opaque
// barrier between the first byte load and the final OR.
class adjacent_pack_u32_load_pass final : public ir_pass {
public:
    const char *name() const override { return "adjacent_pack_u32_load"; }

    bool run(ir_function &fn) override {
        std::unordered_map<int, const icode *> unique_defs;
        std::unordered_map<int, size_t> def_indices;
        std::unordered_map<int, int> definition_counts;
        for (size_t i = 0; i < fn.icodes.size(); ++i) {
            const icode &ic = fn.icodes[i];
            if (!defines_result(ic) || !ic.result.is_temp())
                continue;
            const int tid = ic.result.temp_id;
            ++definition_counts[tid];
            unique_defs[tid] = &ic;
            def_indices[tid] = i;
        }
        for (const auto &[tid, count] : definition_counts) {
            if (count != 1) {
                unique_defs.erase(tid);
                def_indices.erase(tid);
            }
        }

        struct affine_ref {
            std::vector<std::string> terms;
            std::unordered_set<int> deps;
            int64_t offset = 0;
        };

        std::function<std::optional<affine_ref>(
            const operand &, int, std::unordered_set<int> &)> resolve_affine;
        resolve_affine = [&](const operand &op, int depth,
                             std::unordered_set<int> &visiting)
            -> std::optional<affine_ref> {
            if (depth > 8)
                return std::nullopt;
            if (op.kind == operand_kind::INT_CONST) {
                affine_ref value;
                value.offset = op.ival;
                return value;
            }

            if (op.is_temp()) {
                affine_ref atom;
                if (!visiting.insert(op.temp_id).second) {
                    atom.terms.push_back("t:" + std::to_string(op.temp_id));
                    atom.deps.insert(op.temp_id);
                    return atom;
                }

                auto found = unique_defs.find(op.temp_id);
                if (found != unique_defs.end() && found->second) {
                    const icode &def = *found->second;
                    if (def.op == icode_op::ASSIGN ||
                        def.op == icode_op::CAST) {
                        auto result = resolve_affine(
                            def.left, depth + 1, visiting);
                        visiting.erase(op.temp_id);
                        return result;
                    }
                    if (def.op == icode_op::ADDRESS_OF) {
                        operand base = def.left;
                        const int64_t byte_offset = base.byte_offset;
                        base.byte_offset = 0;
                        std::string key = local_cse_operand_key(base);
                        visiting.erase(op.temp_id);
                        if (key.empty())
                            return std::nullopt;
                        atom.terms.push_back(key);
                        atom.offset = byte_offset;
                        return atom;
                    }
                    if (def.op == icode_op::ADD ||
                        def.op == icode_op::SUB) {
                        auto left = resolve_affine(
                            def.left, depth + 1, visiting);
                        auto right = resolve_affine(
                            def.right, depth + 1, visiting);
                        visiting.erase(op.temp_id);
                        if (!left || !right)
                            return std::nullopt;
                        if (def.op == icode_op::SUB &&
                            !right->terms.empty())
                            return std::nullopt;
                        affine_ref combined = std::move(*left);
                        combined.terms.insert(combined.terms.end(),
                                              right->terms.begin(),
                                              right->terms.end());
                        combined.deps.insert(right->deps.begin(),
                                             right->deps.end());
                        combined.offset +=
                            def.op == icode_op::ADD ? right->offset
                                                   : -right->offset;
                        std::sort(combined.terms.begin(),
                                  combined.terms.end());
                        return combined;
                    }
                }

                visiting.erase(op.temp_id);
                atom.terms.push_back("t:" + std::to_string(op.temp_id));
                atom.deps.insert(op.temp_id);
                return atom;
            }

            std::string key = local_cse_operand_key(op);
            if (key.empty())
                return std::nullopt;
            affine_ref atom;
            atom.terms.push_back(key);
            atom.offset = op.byte_offset;
            return atom;
        };

        struct byte_piece {
            const icode *load = nullptr;
            size_t load_index = 0;
            int shift = 0;
            affine_ref address;
        };

        bool changed = false;
        for (size_t i = 0; i < fn.icodes.size(); ++i) {
            icode &root = fn.icodes[i];
            if (root.op != icode_op::BOR || !root.result.type ||
                root.result.type->size() != 4)
                continue;

            std::vector<byte_piece> pieces;
            std::unordered_set<int> expression_visiting;
            std::function<bool(const operand &, int)> collect =
                [&](const operand &value, int shift) {
                if (!value.is_temp() ||
                    !expression_visiting.insert(value.temp_id).second)
                    return false;
                auto found = unique_defs.find(value.temp_id);
                if (found == unique_defs.end() || !found->second) {
                    expression_visiting.erase(value.temp_id);
                    return false;
                }
                const icode &def = *found->second;
                bool ok = false;
                if (def.op == icode_op::BOR) {
                    ok = collect(def.left, shift) &&
                         collect(def.right, shift);
                } else if (def.op == icode_op::SHL &&
                           def.right.kind == operand_kind::INT_CONST &&
                           def.right.ival >= 0 && def.right.ival <= 24 &&
                           (def.right.ival & 7) == 0) {
                    ok = collect(def.left,
                                 shift + static_cast<int>(def.right.ival));
                } else if (def.op == icode_op::ASSIGN ||
                           def.op == icode_op::CAST) {
                    ok = collect(def.left, shift);
                } else if (def.op == icode_op::GET_VALUE_AT &&
                           def.result.type &&
                           def.result.type->size() == 1 &&
                           def.result.type->is_unsigned() &&
                           !def.result.type->is_volatile &&
                           def.left.type && def.left.type->is_ptr() &&
                           (!def.left.type->base ||
                            !def.left.type->base->is_volatile) &&
                           def.right.is_none() && shift <= 24) {
                    auto index = def_indices.find(value.temp_id);
                    std::unordered_set<int> visiting;
                    auto address = resolve_affine(
                        def.left, 0, visiting);
                    if (index != def_indices.end() && address) {
                        pieces.push_back(
                            {&def, index->second, shift,
                             std::move(*address)});
                        ok = true;
                    }
                }
                expression_visiting.erase(value.temp_id);
                return ok;
            };

            if (!collect(root.left, 0) || !collect(root.right, 0) ||
                pieces.size() != 4)
                continue;

            std::sort(pieces.begin(), pieces.end(),
                      [](const byte_piece &lhs, const byte_piece &rhs) {
                          return lhs.shift < rhs.shift;
                      });
            bool layout_ok = true;
            for (size_t part = 0; part < pieces.size(); ++part) {
                if (pieces[part].shift != static_cast<int>(part * 8) ||
                    pieces[part].address.terms != pieces[0].address.terms ||
                    pieces[part].address.offset !=
                        pieces[0].address.offset + static_cast<int64_t>(part)) {
                    layout_ok = false;
                    break;
                }
            }
            if (!layout_ok)
                continue;

            size_t first_load = i;
            std::unordered_set<int> deps;
            for (const auto &piece : pieces) {
                first_load = std::min(first_load, piece.load_index);
                deps.insert(piece.address.deps.begin(),
                            piece.address.deps.end());
            }

            bool stable = true;
            for (size_t k = first_load + 1; k < i; ++k) {
                const icode &scan = fn.icodes[k];
                if (scan.op == icode_op::SET_VALUE_AT ||
                    is_available_word_load_barrier(scan)) {
                    stable = false;
                    break;
                }
                if (defines_result(scan) && scan.result.is_temp() &&
                    deps.count(scan.result.temp_id)) {
                    stable = false;
                    break;
                }
            }
            if (!stable)
                continue;

            root.op = icode_op::GET_VALUE_AT;
            root.left = pieces[0].load->left;
            root.right = operand::make_none();
            changed = true;
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

        auto direct_data_base = [](const operand &op) {
            return op.kind == operand_kind::SYMBOL &&
                   op.is_global &&
                   !op.is_tls &&
                   !op.is_sfr &&
                   !op.is_func &&
                   !op.is_param &&
                   op.type && op.type->kind == type_kind::ARRAY;
        };

        std::function<bool(const operand &, operand &,
                           std::unordered_set<int> &)> extract_data_base_impl;
        extract_data_base_impl = [&](const operand &cand, operand &out,
                                     std::unordered_set<int> &visiting) {
            if (cand.kind == operand_kind::LABEL_REF) {
                out = cand;
                return true;
            }
            if (direct_data_base(cand)) {
                out = operand::make_label(cand.name);
                return true;
            }
            if (!cand.is_temp())
                return false;
            if (!visiting.insert(cand.temp_id).second)
                return false;
            auto src_it = temp_defs.find(cand.temp_id);
            if (src_it == temp_defs.end() || !src_it->second)
                return false;
            const icode *src_def = src_it->second;
            if ((src_def->op == icode_op::ASSIGN ||
                 src_def->op == icode_op::CAST) &&
                src_def->left.is_temp()) {
                return extract_data_base_impl(src_def->left, out, visiting);
            }
            if (src_def->op == icode_op::ADDRESS_OF &&
                direct_data_base(src_def->left)) {
                out = operand::make_label(src_def->left.name);
                return true;
            }
            return false;
        };
        auto extract_data_base = [&](const operand &cand, operand &out) {
            std::unordered_set<int> visiting;
            return extract_data_base_impl(cand, out, visiting);
        };

        std::unordered_set<std::string> mutable_bases;
        for (const auto &ic : fn.icodes) {
            if (ic.op != icode_op::SET_VALUE_AT)
                continue;
            operand base;
            if (extract_data_base(ic.result, base) &&
                base.kind == operand_kind::LABEL_REF) {
                mutable_bases.insert(base.name);
            }
        }

        std::function<bool(const operand &, operand &, operand &,
                           std::unordered_set<int> &)> match_ptr_impl;
        match_ptr_impl = [&](const operand &ptr, operand &base,
                             operand &index,
                             std::unordered_set<int> &visiting) -> bool {
            if (!ptr.is_temp())
                return false;
            if (!visiting.insert(ptr.temp_id).second)
                return false;
            auto it = temp_defs.find(ptr.temp_id);
            if (it == temp_defs.end() || !it->second)
                return false;
            const icode *def = it->second;
            if ((def->op == icode_op::ASSIGN || def->op == icode_op::CAST) &&
                def->left.is_temp()) {
                return match_ptr_impl(def->left, base, index, visiting);
            }
            if (def->op != icode_op::ADD)
                return false;

            if (extract_data_base(def->left, base)) {
                auto idx = unwrap_index(def->right);
                if (!idx)
                    return false;
                index = *idx;
                return true;
            }
            if (extract_data_base(def->right, base)) {
                auto idx = unwrap_index(def->left);
                if (!idx)
                    return false;
                index = *idx;
                return true;
            }
            return false;
        };
        auto match_ptr = [&](const operand &ptr, operand &base,
                             operand &index) {
            std::unordered_set<int> visiting;
            return match_ptr_impl(ptr, base, index, visiting);
        };

        bool changed = false;
        for (auto &ic : fn.icodes) {
            operand base;
            operand index;
            const int result_size =
                (ic.result.type && ic.result.type->size() > 0) ? ic.result.type->size() : 2;
            if (ic.op == icode_op::GET_VALUE_AT &&
                (result_size == 1 || result_size == 2) &&
                match_ptr(ic.left, base, index) &&
                !(base.kind == operand_kind::LABEL_REF &&
                  mutable_bases.count(base.name))) {
                ic.left = base;
                ic.right = index;
                changed = true;
                continue;
            }
            const int store_size =
                (ic.left.type && ic.left.type->size() > 0) ? ic.left.type->size() : 2;
            if (ic.op == icode_op::SET_VALUE_AT &&
                (store_size == 1 || store_size == 2) &&
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

class loop_lockstep_pointer_pass final : public ir_pass {
public:
    explicit loop_lockstep_pointer_pass(bool size_bias)
        : size_bias_(size_bias) {}

    const char *name() const override { return "loop_lockstep_pointer"; }

    bool run(ir_function &fn) override {
        if (fn.icodes.empty())
            return false;

        control_flow_graph cfg(fn);
        const auto loops = cfg.natural_loops();
        if (loops.empty())
            return false;

        std::vector<size_t> inst_block(fn.icodes.size(), 0);
        for (const auto &block : cfg.blocks()) {
            for (size_t i = block.begin; i < block.end; ++i)
                inst_block[i] = block.id;
        }

        std::unordered_map<int, std::vector<size_t>> temp_defs;
        for (size_t i = 0; i < fn.icodes.size(); ++i) {
            if (defines_result(fn.icodes[i]) && fn.icodes[i].result.is_temp())
                temp_defs[fn.icodes[i].result.temp_id].push_back(i);
        }

        auto same_slot = [](const operand &lhs, const operand &rhs) {
            if (lhs.is_temp() || rhs.is_temp())
                return lhs.is_temp() && rhs.is_temp() &&
                       lhs.temp_id == rhs.temp_id;
            return same_symbol_slot(lhs, rhs);
        };

        auto latest_temp_def_before = [&](int temp_id, size_t before)
            -> std::optional<size_t> {
            auto it = temp_defs.find(temp_id);
            if (it == temp_defs.end())
                return std::nullopt;
            const auto pos = std::lower_bound(it->second.begin(),
                                              it->second.end(), before);
            if (pos == it->second.begin())
                return std::nullopt;
            return *std::prev(pos);
        };

        auto pointer_type_for_base = [](const operand &base) -> type_ptr {
            if (base.type) {
                if (base.type->is_array() && base.type->base)
                    return type::make_pointer(base.type->base);
                if (base.type->is_ptr())
                    return base.type;
            }
            return type::make_pointer(type::make_uchar());
        };

        auto direct_byte_base = [](const operand &op) {
            if (op.kind == operand_kind::LABEL_REF)
                return true;
            return op.kind == operand_kind::SYMBOL && op.is_global &&
                   !op.is_tls && !op.is_sfr && !op.is_func && !op.is_param &&
                   op.byte_offset == 0 && op.type && op.type->is_array() &&
                   op.type->base && op.type->base->size() == 1 &&
                   !op.type->base->is_volatile;
        };

        auto base_key = [](const operand &op) {
            if (op.kind == operand_kind::LABEL_REF)
                return std::string("label|") + op.name;
            if (op.kind == operand_kind::TEMP)
                return std::string("temp|") + std::to_string(op.temp_id);
            return symbol_key(op);
        };

        int next_temp = next_temp_id(fn);
        std::map<size_t, std::vector<icode>> insert_before;
        std::unordered_map<int, operand> replace_temp;
        std::unordered_set<size_t> direct_accesses_consumed;
        std::unordered_set<size_t> dead_secondary_induction_defs;
        bool changed = false;

        for (const auto &loop : loops) {
            if (loop.outside_preds.size() != 1)
                continue;

            const auto &header = cfg.block(loop.header);
            const auto &preheader = cfg.block(loop.outside_preds.front());

            operand index;
            int64_t loop_bound = 0;
            bool loop_bound_known = false;
            for (size_t i = header.begin; i < header.end; ++i) {
                const icode &cmp = fn.icodes[i];
                if (cmp.op != icode_op::LT || !cmp.result.is_temp() ||
                    cmp.right.is_none() || !cmp.left.type ||
                    !cmp.left.type->is_integer() ||
                    cmp.left.type->size() < 1 || cmp.left.type->size() > 2) {
                    continue;
                }

                bool feeds_header_branch = false;
                for (size_t j = i + 1; j < header.end; ++j) {
                    const icode &use = fn.icodes[j];
                    if (use.op == icode_op::IFX && use.left.is_temp() &&
                        use.left.temp_id == cmp.result.temp_id) {
                        feeds_header_branch = true;
                        break;
                    }
                    if (defines_result(use) && use.result.is_temp() &&
                        use.result.temp_id == cmp.result.temp_id) {
                        break;
                    }
                }
                if (!feeds_header_branch)
                    continue;

                index = cmp.left;
                if (cmp.right.kind == operand_kind::INT_CONST &&
                    cmp.right.ival > 0) {
                    loop_bound = cmp.right.ival;
                    loop_bound_known = true;
                }
                break;
            }
            if (index.is_none())
                continue;

            size_t init_idx = fn.icodes.size();
            for (size_t i = preheader.begin; i < preheader.end; ++i) {
                const icode &ic = fn.icodes[i];
                if (ic.op == icode_op::ASSIGN &&
                    same_slot(ic.result, index) && !ic.left.is_none()) {
                    init_idx = i;
                }
            }
            if (init_idx == fn.icodes.size())
                continue;

            std::vector<bool> in_loop_inst(fn.icodes.size(), false);
            bool has_codegen_barrier = false;
            bool has_call = false;
            for (size_t block_id : loop.blocks) {
                const auto &block = cfg.block(block_id);
                for (size_t i = block.begin; i < block.end; ++i) {
                    in_loop_inst[i] = true;
                    const icode_op op = fn.icodes[i].op;
                    has_call = has_call || op == icode_op::CALL;
                    if (op == icode_op::ALLOCA || op == icode_op::INLINE_ASM) {
                        has_codegen_barrier = true;
                    }
                }
            }
            if (has_codegen_barrier ||
                (has_call && index.type->size() == 1))
                continue;

            std::vector<size_t> commits;
            std::unordered_set<int> step_temps;
            bool bad_index_def = false;
            for (size_t block_id : loop.blocks) {
                const auto &block = cfg.block(block_id);
                for (size_t i = block.begin; i < block.end; ++i) {
                    const icode &commit = fn.icodes[i];
                    if (!defines_result(commit) ||
                        !same_slot(commit.result, index)) {
                        continue;
                    }
                    const bool inplace_add_one =
                        commit.op == icode_op::ADD &&
                        ((same_slot(commit.left, index) &&
                          commit.right.kind == operand_kind::INT_CONST &&
                          commit.right.ival == 1) ||
                         (same_slot(commit.right, index) &&
                          commit.left.kind == operand_kind::INT_CONST &&
                          commit.left.ival == 1));
                    if (inplace_add_one) {
                        commits.push_back(i);
                        continue;
                    }
                    if (commit.op != icode_op::ASSIGN ||
                        !commit.left.is_temp()) {
                        bad_index_def = true;
                        break;
                    }

                    auto step_idx = latest_temp_def_before(commit.left.temp_id, i);
                    if (!step_idx || !in_loop_inst[*step_idx]) {
                        bad_index_def = true;
                        break;
                    }
                    const icode &step = fn.icodes[*step_idx];
                    const bool add_one =
                        step.op == icode_op::ADD &&
                        ((same_slot(step.left, index) &&
                          step.right.kind == operand_kind::INT_CONST &&
                          step.right.ival == 1) ||
                         (same_slot(step.right, index) &&
                          step.left.kind == operand_kind::INT_CONST &&
                          step.left.ival == 1));
                    if (!add_one) {
                        bad_index_def = true;
                        break;
                    }
                    for (size_t j = *step_idx + 1; j < i; ++j) {
                        if (defines_result(fn.icodes[j]) &&
                            same_slot(fn.icodes[j].result, index)) {
                            bad_index_def = true;
                            break;
                        }
                    }
                    if (bad_index_def)
                        break;
                    commits.push_back(i);
                    step_temps.insert(commit.left.temp_id);
                }
                if (bad_index_def)
                    break;
            }
            if (bad_index_def || commits.empty())
                continue;

            auto index_reaches_current_value = [&](const operand &op,
                                                   size_t use_idx) {
                size_t snapshot_idx = use_idx;
                operand cur = op;
                bool matched = false;
                for (int depth = 0; depth < 5; ++depth) {
                    if (same_slot(cur, index)) {
                        matched = true;
                        break;
                    }
                    if (!cur.is_temp())
                        break;
                    auto def_idx = latest_temp_def_before(cur.temp_id,
                                                          snapshot_idx);
                    if (!def_idx)
                        break;

                    // A textual "latest definition" is not a reaching-
                    // definition proof around a backedge.  In a nested loop,
                    // an alias such as `inner = outer` may be redefined later
                    // in the listing (`--inner`) and that later definition can
                    // reach an earlier-looking use on the next iteration.
                    // Follow an alias only when its selected definition is the
                    // sole definition that can execute in this natural loop.
                    // The actual loop index is handled by same_slot() above.
                    auto all_defs = temp_defs.find(cur.temp_id);
                    if (all_defs != temp_defs.end()) {
                        bool competing_loop_def = false;
                        for (size_t candidate : all_defs->second) {
                            if (candidate != *def_idx &&
                                in_loop_inst[candidate]) {
                                competing_loop_def = true;
                                break;
                            }
                        }
                        if (competing_loop_def)
                            break;
                    }
                    const icode &def = fn.icodes[*def_idx];
                    if ((def.op != icode_op::ASSIGN &&
                         def.op != icode_op::CAST) || def.left.is_none()) {
                        break;
                    }
                    snapshot_idx = *def_idx;
                    cur = def.left;
                }
                if (!matched)
                    return false;
                for (size_t commit_idx : commits) {
                    if (commit_idx > snapshot_idx && commit_idx < use_idx)
                        return false;
                }
                return true;
            };

            // For an 8-bit induction variable, prove that every increment is
            // reached through a fresh `index < bound` true edge. This keeps a
            // running 16-bit pointer equivalent to wrapping unsigned-byte C
            // arithmetic instead of assuming overflow cannot occur.
            if (index.type->size() == 1) {
                if (!loop_bound_known || loop_bound > 255)
                    continue;

                struct work_state {
                    size_t block = 0;
                    bool increment_safe = false;
                };
                std::vector<work_state> work{{loop.header, false}};
                std::unordered_set<size_t> visited_states;
                bool overflow_proven = true;

                while (!work.empty() && overflow_proven) {
                    work_state state = work.back();
                    work.pop_back();
                    const size_t state_key = state.block * 2 +
                                             (state.increment_safe ? 1 : 0);
                    if (!visited_states.insert(state_key).second)
                        continue;

                    const auto &block = cfg.block(state.block);
                    bool safe = state.increment_safe;
                    for (size_t i = block.begin; i < block.end; ++i) {
                        if (std::find(commits.begin(), commits.end(), i) ==
                            commits.end()) {
                            continue;
                        }
                        if (!safe) {
                            overflow_proven = false;
                            break;
                        }
                        safe = false;
                    }
                    if (!overflow_proven)
                        break;

                    const icode *term =
                        block.begin < block.end ? &fn.icodes[block.end - 1]
                                                : nullptr;
                    const icode *guard = nullptr;
                    if (term && term->op == icode_op::IFX &&
                        term->left.is_temp()) {
                        for (size_t i = block.end - 1; i-- > block.begin;) {
                            const icode &cand = fn.icodes[i];
                            if (!cand.result.is_temp() ||
                                cand.result.temp_id != term->left.temp_id) {
                                continue;
                            }
                            if (cand.op == icode_op::LT &&
                                index_reaches_current_value(cand.left, i) &&
                                cand.right.kind == operand_kind::INT_CONST &&
                                cand.right.ival > 0 &&
                                cand.right.ival <= 255) {
                                guard = &cand;
                            }
                            break;
                        }
                    }

                    std::optional<size_t> true_block;
                    if (guard && term)
                        true_block = cfg.block_for_label(term->true_lbl);
                    for (size_t succ : block.succs) {
                        if (!loop.blocks.count(succ))
                            continue;
                        bool edge_safe = safe;
                        if (guard)
                            edge_safe = true_block && succ == *true_block;
                        work.push_back({succ, edge_safe});
                    }
                }
                if (!overflow_proven)
                    continue;
            }

            auto invariant_pointer_base = [&](const operand &op) {
                if (op.kind == operand_kind::LABEL_REF)
                    return true;
                if (op.is_temp()) {
                    if (!op.type || !op.type->is_ptr())
                        return false;
                    auto defs = temp_defs.find(op.temp_id);
                    if (defs == temp_defs.end())
                        return false;
                    for (size_t def_idx : defs->second) {
                        if (in_loop_inst[def_idx])
                            return false;
                    }
                    return true;
                }
                return op.kind == operand_kind::SYMBOL && op.is_global &&
                       !op.is_tls && !op.is_sfr && !op.is_func &&
                       !op.is_param && op.byte_offset == 0 && op.type &&
                       op.type->is_array() && op.type->base &&
                       !op.type->base->is_volatile;
            };

            struct candidate_group {
                operand base;
                type_ptr ptr_type;
                std::vector<size_t> direct_accesses;
                std::vector<int> address_temps;
                std::vector<int> secondary_inductions;
                int stride = 1;
                int score = 0;
                // Some indexed addresses are affine expressions whose
                // loop-invariant part is not just zero, for example
                // `base + ((row_offset + column) << 1)`.  Preserve the exact
                // expression used for the first iteration so the synthetic
                // pointer can start at that address and subsequently advance
                // by stride bytes.
                operand affine_offset;
                size_t affine_use_idx = 0;
            };
            std::unordered_map<std::string, candidate_group> groups;

            auto add_group_access = [&](const operand &base, size_t access_idx,
                                        int address_temp, int stride,
                                        const std::string &affine_key = {},
                                        const operand &affine_offset =
                                            operand::make_none(),
                                        size_t affine_use_idx = 0) {
                const std::string base_id = base_key(base);
                if (base_id.empty())
                    return;
                const std::string key = base_id + "|" +
                                        std::to_string(stride) + "|" +
                                        affine_key;
                auto &group = groups[key];
                if (group.base.is_none()) {
                    group.base = base;
                    group.ptr_type = pointer_type_for_base(base);
                    group.stride = stride;
                    group.affine_offset = affine_offset;
                    group.affine_use_idx = affine_use_idx;
                }
                if (address_temp >= 0)
                    group.address_temps.push_back(address_temp);
                else
                    group.direct_accesses.push_back(access_idx);
                group.score += 100;
                for (const auto &nested : loops) {
                    if (nested.blocks.count(inst_block[access_idx]))
                        group.score += 25;
                }
            };

            auto address_use_allowed = [&](const icode &ic, int temp_id) {
                if (ic.op == icode_op::GET_VALUE_AT && ic.left.is_temp() &&
                    ic.left.temp_id == temp_id && ic.right.is_none() &&
                    ic.result.type &&
                    (ic.result.type->size() == 1 ||
                     ic.result.type->size() == 2)) {
                    return true;
                }
                if (ic.op == icode_op::SET_VALUE_AT && ic.result.is_temp() &&
                    ic.result.temp_id == temp_id && ic.right.is_none() &&
                    ic.left.type &&
                    (ic.left.type->size() == 1 ||
                     ic.left.type->size() == 2)) {
                    return true;
                }
                // A struct/record base commonly feeds several constant field
                // addresses before the actual loads and stores.  Replacing
                // the base+induction expression with an equivalent running
                // pointer remains valid through those byte-offset additions.
                if (ic.op == icode_op::ADD && ic.result.is_temp()) {
                    const bool left_base = ic.left.is_temp() &&
                                           ic.left.temp_id == temp_id &&
                                           ic.right.kind == operand_kind::INT_CONST;
                    const bool right_base = ic.right.is_temp() &&
                                            ic.right.temp_id == temp_id &&
                                            ic.left.kind == operand_kind::INT_CONST;
                    if (left_base || right_base)
                        return true;
                }
                return ic.op == icode_op::SEND && ic.left.is_temp() &&
                       ic.left.temp_id == temp_id;
            };

            std::unordered_map<int, int> secondary_induction_stride;
            auto index_stride_for_offset = [&](const operand &offset,
                                               size_t use_idx) {
                if (index_reaches_current_value(offset, use_idx))
                    return 1;
                if (!offset.is_temp())
                    return 0;

                // Earlier strength reduction may already have replaced
                // `index * stride` with its own lockstep byte offset.  Prove
                // that secondary induction independently: one constant
                // preheader initialization, one positive constant self-step
                // in this loop, and no uses except forming addresses from an
                // invariant base.  It can then be subsumed by the pointer we
                // are about to introduce instead of making the backend carry
                // both an offset and a pointer.
                int initial_defs = 0;
                int update_defs = 0;
                int secondary_stride = 0;
                auto defs_it = temp_defs.find(offset.temp_id);
                if (defs_it != temp_defs.end()) {
                    bool secondary_ok = true;
                    for (size_t def_idx : defs_it->second) {
                        const icode &def = fn.icodes[def_idx];
                        if (!in_loop_inst[def_idx]) {
                            if (def.op == icode_op::ASSIGN &&
                                def.left.kind == operand_kind::INT_CONST) {
                                ++initial_defs;
                                continue;
                            }
                        } else if (def.op == icode_op::ADD &&
                                   def.left.is_temp() &&
                                   def.left.temp_id == offset.temp_id &&
                                   def.right.kind == operand_kind::INT_CONST &&
                                   def.right.ival > 0 &&
                                   def.right.ival <= 32767) {
                            ++update_defs;
                            secondary_stride =
                                static_cast<int>(def.right.ival);
                            continue;
                        }
                        secondary_ok = false;
                        break;
                    }
                    if (secondary_ok && initial_defs == 1 &&
                        update_defs == 1) {
                        for (size_t i = 0; i < fn.icodes.size(); ++i) {
                            const icode &use = fn.icodes[i];
                            const bool used_left = use.left.is_temp() &&
                                                   use.left.temp_id == offset.temp_id;
                            const bool used_right = use.right.is_temp() &&
                                                    use.right.temp_id == offset.temp_id;
                            if (!used_left && !used_right)
                                continue;
                            const bool self_step =
                                use.op == icode_op::ADD &&
                                use.result.is_temp() &&
                                use.result.temp_id == offset.temp_id &&
                                used_left &&
                                use.right.kind == operand_kind::INT_CONST;
                            const bool address_form =
                                use.op == icode_op::ADD &&
                                use.result.is_temp() &&
                                ((used_left && invariant_pointer_base(use.right)) ||
                                 (used_right && invariant_pointer_base(use.left)));
                            if (!self_step && !address_form) {
                                secondary_ok = false;
                                break;
                            }
                        }
                    } else {
                        secondary_ok = false;
                    }
                    if (secondary_ok) {
                        secondary_induction_stride[offset.temp_id] =
                            secondary_stride;
                        return secondary_stride;
                    }
                }

                auto def_idx = latest_temp_def_before(offset.temp_id, use_idx);
                if (!def_idx || !in_loop_inst[*def_idx])
                    return 0;
                const icode &scale = fn.icodes[*def_idx];
                if (scale.op == icode_op::SHL &&
                    scale.right.kind == operand_kind::INT_CONST &&
                    scale.right.ival == 1 &&
                    index_reaches_current_value(scale.left, *def_idx)) {
                    return 2;
                }
                if (scale.op != icode_op::MUL)
                    return 0;
                if (scale.right.kind == operand_kind::INT_CONST &&
                    scale.right.ival == 2 &&
                    index_reaches_current_value(scale.left, *def_idx)) {
                    return 2;
                }
                if (scale.left.kind == operand_kind::INT_CONST &&
                    scale.left.ival == 2 &&
                    index_reaches_current_value(scale.right, *def_idx)) {
                    return 2;
                }
                return 0;
            };

            struct affine_index_info {
                int stride = 0;
                std::string key;
            };
            auto affine_index_for_offset = [&](const operand &root,
                                               size_t use_idx)
                -> std::optional<affine_index_info> {
                std::unordered_set<int> visiting;
                std::function<std::optional<affine_index_info>(
                    const operand &, size_t, int)> walk;
                walk = [&](const operand &op, size_t before, int depth)
                    -> std::optional<affine_index_info> {
                    if (depth > 10)
                        return std::nullopt;
                    if (index_reaches_current_value(op, before))
                        return affine_index_info{1, "I"};
                    if (op.kind == operand_kind::INT_CONST)
                        return affine_index_info{
                            0, "C" + std::to_string(op.ival)};
                    if (!op.is_temp())
                        return std::nullopt;

                    // A nested-loop induction variable can look constant at
                    // the first textual definition while changing later in
                    // the enclosing natural loop.  Only recurse through
                    // single-definition expression temps; the selected loop
                    // index itself was handled above.
                    auto all_defs = temp_defs.find(op.temp_id);
                    int loop_defs = 0;
                    if (all_defs != temp_defs.end()) {
                        for (size_t candidate : all_defs->second)
                            loop_defs += in_loop_inst[candidate] ? 1 : 0;
                    }
                    if (loop_defs > 1)
                        return std::nullopt;

                    auto def_idx = latest_temp_def_before(op.temp_id, before);
                    if (!def_idx || !in_loop_inst[*def_idx]) {
                        // A value defined outside this loop is an invariant
                        // symbolic part of the affine address.
                        return affine_index_info{
                            0, "T" + std::to_string(op.temp_id)};
                    }
                    if (!visiting.insert(op.temp_id).second)
                        return std::nullopt;
                    const icode &def = fn.icodes[*def_idx];
                    std::optional<affine_index_info> result;
                    if ((def.op == icode_op::ASSIGN ||
                         def.op == icode_op::CAST) && !def.left.is_none()) {
                        result = walk(def.left, *def_idx, depth + 1);
                        if (result)
                            result->key = "U" + std::to_string(
                                static_cast<int>(def.op)) + "(" +
                                result->key + ")";
                    } else if (def.op == icode_op::ADD ||
                               def.op == icode_op::SUB) {
                        auto lhs = walk(def.left, *def_idx, depth + 1);
                        auto rhs = walk(def.right, *def_idx, depth + 1);
                        if (lhs && rhs) {
                            const int stride = def.op == icode_op::ADD
                                                   ? lhs->stride + rhs->stride
                                                   : lhs->stride - rhs->stride;
                            result = affine_index_info{
                                stride,
                                "B" + std::to_string(
                                    static_cast<int>(def.op)) + "(" +
                                    lhs->key + "," + rhs->key + ")"};
                        }
                    } else if (def.op == icode_op::SHL &&
                               def.right.kind == operand_kind::INT_CONST &&
                               def.right.ival >= 0 && def.right.ival <= 8) {
                        auto value = walk(def.left, *def_idx, depth + 1);
                        if (value) {
                            value->stride <<= def.right.ival;
                            value->key = "S" +
                                         std::to_string(def.right.ival) +
                                         "(" + value->key + ")";
                            result = std::move(value);
                        }
                    } else if (def.op == icode_op::MUL) {
                        const operand *value = &def.left;
                        const operand *constant = &def.right;
                        if (value->kind == operand_kind::INT_CONST)
                            std::swap(value, constant);
                        if (constant->kind == operand_kind::INT_CONST &&
                            constant->ival > 0 && constant->ival <= 256) {
                            auto scaled = walk(*value, *def_idx, depth + 1);
                            if (scaled) {
                                scaled->stride *= constant->ival;
                                scaled->key = "M" +
                                              std::to_string(constant->ival) +
                                              "(" + scaled->key + ")";
                                result = std::move(scaled);
                            }
                        }
                    }
                    visiting.erase(op.temp_id);
                    if (result && result->stride != 0) {
                        // A temp may snapshot the induction value before a
                        // post-increment and be dereferenced afterwards.  A
                        // running pointer is advanced at that commit, so it
                        // cannot replace the deliberately stale snapshot.
                        for (size_t commit_idx : commits) {
                            if (commit_idx > *def_idx &&
                                commit_idx < before) {
                                return std::nullopt;
                            }
                        }
                    }
                    if (!result || result->stride < -32767 ||
                        result->stride > 32767) {
                        return std::nullopt;
                    }
                    return result;
                };

                auto result = walk(root, use_idx, 0);
                if (!result || result->stride <= 0)
                    return std::nullopt;
                return result;
            };

            for (size_t block_id : loop.blocks) {
                const auto &block = cfg.block(block_id);
                for (size_t i = block.begin; i < block.end; ++i) {
                    const icode &ic = fn.icodes[i];
                    if (direct_accesses_consumed.count(i))
                        continue;

                    const operand *direct_base = nullptr;
                    const operand *direct_index = nullptr;
                    if (ic.op == icode_op::GET_VALUE_AT &&
                        ic.result.type && ic.result.type->size() == 1 &&
                        direct_byte_base(ic.left) && !ic.right.is_none()) {
                        direct_base = &ic.left;
                        direct_index = &ic.right;
                    } else if (ic.op == icode_op::SET_VALUE_AT &&
                               ic.left.type && ic.left.type->size() == 1 &&
                               direct_byte_base(ic.result) &&
                               !ic.right.is_none()) {
                        direct_base = &ic.result;
                        direct_index = &ic.right;
                    }
                    if (direct_base &&
                        index_reaches_current_value(*direct_index, i)) {
                        add_group_access(*direct_base, i, -1, 1);
                        continue;
                    }

                    if (ic.op != icode_op::ADD || !ic.result.is_temp())
                        continue;
                    const operand *base = nullptr;
                    const operand *offset = nullptr;
                    if (invariant_pointer_base(ic.left)) {
                        base = &ic.left;
                        offset = &ic.right;
                    } else if (invariant_pointer_base(ic.right)) {
                        base = &ic.right;
                        offset = &ic.left;
                    }
                    if (!base || !offset)
                        continue;
                    int stride = index_stride_for_offset(*offset, i);
                    std::string affine_key;
                    operand affine_offset = operand::make_none();
                    if (stride == 0) {
                        auto affine = affine_index_for_offset(*offset, i);
                        if (!affine)
                            continue;
                        stride = affine->stride;
                        affine_key = affine->key;
                        affine_offset = *offset;
                    }

                    int mem_uses = 0;
                    bool only_loop_address_uses = true;
                    size_t first_mem_use = fn.icodes.size();
                    for (size_t use_idx = 0; use_idx < fn.icodes.size(); ++use_idx) {
                        bool uses = false;
                        for_each_use_operand(fn.icodes[use_idx],
                                             [&](const operand &op) {
                            if (op.is_temp() &&
                                op.temp_id == ic.result.temp_id) {
                                uses = true;
                            }
                        });
                        if (!uses)
                            continue;
                        if (!in_loop_inst[use_idx] ||
                            !address_use_allowed(fn.icodes[use_idx],
                                                 ic.result.temp_id)) {
                            only_loop_address_uses = false;
                            break;
                        }
                        ++mem_uses;
                        first_mem_use = std::min(first_mem_use, use_idx);
                    }
                    if (!only_loop_address_uses || mem_uses == 0)
                        continue;
                    add_group_access(*base, first_mem_use, ic.result.temp_id,
                                     stride, affine_key, affine_offset, i);
                    const std::string group_key = base_key(*base) + "|" +
                                                  std::to_string(stride) + "|" +
                                                  affine_key;
                    groups[group_key].score += (mem_uses - 1) * 100;
                    if (offset->is_temp() &&
                        secondary_induction_stride.count(offset->temp_id)) {
                        groups[group_key].secondary_inductions.push_back(
                            offset->temp_id);
                    }
                }
            }
            if (groups.empty())
                continue;

            auto best = std::max_element(
                groups.begin(), groups.end(),
                [](const auto &lhs, const auto &rhs) {
                    if (lhs.second.score != rhs.second.score)
                        return lhs.second.score < rhs.second.score;
                    return lhs.first > rhs.first;
                });
            candidate_group &group = best->second;
            const operand &initial_index = fn.icodes[init_idx].left;
            const bool starts_at_zero =
                initial_index.kind == operand_kind::INT_CONST &&
                initial_index.ival == 0;
            const bool proven_stride_one =
                group.stride == 1 && invariant_pointer_base(group.base) &&
                starts_at_zero && loop_bound_known && !has_call;
            if (group.stride == 1 && !proven_stride_one &&
                group.affine_offset.is_none())
                continue;
            auto group_feeds_reduction = [&]() {
                std::unordered_set<int> address_ids(
                    group.address_temps.begin(), group.address_temps.end());
                std::unordered_set<size_t> direct_ids(
                    group.direct_accesses.begin(), group.direct_accesses.end());
                std::unordered_set<int> loaded_values;
                for (size_t block_id : loop.blocks) {
                    const auto &block = cfg.block(block_id);
                    for (size_t i = block.begin; i < block.end; ++i) {
                        const icode &load = fn.icodes[i];
                        if (load.op != icode_op::GET_VALUE_AT ||
                            !load.result.is_temp()) {
                            continue;
                        }
                        const bool through_address =
                            load.left.is_temp() &&
                            address_ids.count(load.left.temp_id) != 0;
                        if (through_address || direct_ids.count(i) != 0)
                            loaded_values.insert(load.result.temp_id);
                    }
                }
                for (size_t block_id : loop.blocks) {
                    const auto &block = cfg.block(block_id);
                    for (size_t i = block.begin; i < block.end; ++i) {
                        const icode &add = fn.icodes[i];
                        if (add.op != icode_op::ADD ||
                            !add.result.is_temp()) {
                            continue;
                        }
                        const bool lhs_reduction =
                            add.left.is_temp() &&
                            add.left.temp_id == add.result.temp_id &&
                            add.right.is_temp() &&
                            loaded_values.count(add.right.temp_id) != 0;
                        const bool rhs_reduction =
                            add.right.is_temp() &&
                            add.right.temp_id == add.result.temp_id &&
                            add.left.is_temp() &&
                            loaded_values.count(add.left.temp_id) != 0;
                        if (lhs_reduction || rhs_reduction)
                            return true;
                    }
                }
                return false;
            };
            if (size_bias_ && group.stride > 1 &&
                group.address_temps.size() + group.direct_accesses.size() < 2 &&
                group.score < 300 &&
                !(starts_at_zero && group_feeds_reduction())) {
                continue;
            }

            operand ptr = make_fresh_temp(next_temp, group.ptr_type);
            const size_t init_insert =
                insertion_index_before_terminator(preheader, fn);
            auto make_base_init = [&](const operand &result) {
                icode init;
                init.op = group.base.kind == operand_kind::LABEL_REF ||
                                  group.base.is_temp()
                              ? icode_op::ASSIGN
                              : icode_op::ADDRESS_OF;
                init.result = result;
                init.left = group.base;
                return init;
            };

            if (!group.affine_offset.is_none()) {
                std::vector<icode> affine_init;
                operand base_ptr = make_fresh_temp(next_temp, group.ptr_type);
                affine_init.push_back(make_base_init(base_ptr));

                std::unordered_set<int> visiting;
                std::function<std::optional<operand>(const operand &,
                                                     size_t, int)>
                    materialize_initial;
                materialize_initial = [&](const operand &op, size_t before,
                                          int depth) -> std::optional<operand> {
                    if (depth > 10)
                        return std::nullopt;
                    if (index_reaches_current_value(op, before))
                        return index;
                    if (op.kind == operand_kind::INT_CONST)
                        return op;
                    if (!op.is_temp())
                        return std::nullopt;
                    auto def_idx = latest_temp_def_before(op.temp_id, before);
                    if (!def_idx || !in_loop_inst[*def_idx])
                        return op;
                    if (!visiting.insert(op.temp_id).second)
                        return std::nullopt;

                    const icode &def = fn.icodes[*def_idx];
                    const bool unary = def.op == icode_op::ASSIGN ||
                                       def.op == icode_op::CAST;
                    const bool binary = def.op == icode_op::ADD ||
                                        def.op == icode_op::SUB ||
                                        def.op == icode_op::SHL ||
                                        def.op == icode_op::MUL;
                    if ((!unary && !binary) || def.left.is_none()) {
                        visiting.erase(op.temp_id);
                        return std::nullopt;
                    }
                    auto lhs = materialize_initial(def.left, *def_idx,
                                                   depth + 1);
                    std::optional<operand> rhs = operand::make_none();
                    if (binary && !def.right.is_none())
                        rhs = materialize_initial(def.right, *def_idx,
                                                  depth + 1);
                    if (!lhs || !rhs) {
                        visiting.erase(op.temp_id);
                        return std::nullopt;
                    }

                    operand value = make_fresh_temp(
                        next_temp, def.result.type ? def.result.type : op.type);
                    icode cloned = def;
                    cloned.result = value;
                    cloned.left = *lhs;
                    cloned.right = binary ? *rhs : operand::make_none();
                    affine_init.push_back(std::move(cloned));
                    visiting.erase(op.temp_id);
                    return value;
                };

                auto initial_offset = materialize_initial(
                    group.affine_offset, group.affine_use_idx, 0);
                if (!initial_offset)
                    continue;
                icode add;
                add.op = icode_op::ADD;
                add.result = ptr;
                add.left = base_ptr;
                add.right = *initial_offset;
                affine_init.push_back(std::move(add));
                auto &pending = insert_before[init_insert];
                pending.insert(pending.end(), affine_init.begin(),
                               affine_init.end());
            } else if (starts_at_zero) {
                insert_before[init_insert].push_back(make_base_init(ptr));
            } else {
                operand base_ptr = make_fresh_temp(next_temp, group.ptr_type);
                insert_before[init_insert].push_back(
                    make_base_init(base_ptr));

                operand byte_offset = index;
                if (group.stride == 2) {
                    operand scaled = make_fresh_temp(next_temp, index.type);
                    icode scale;
                    scale.op = icode_op::SHL;
                    scale.result = scaled;
                    scale.left = index;
                    scale.right = operand::make_int(1, type::make_int());
                    insert_before[init_insert].push_back(scale);
                    byte_offset = scaled;
                }

                icode add;
                add.op = icode_op::ADD;
                add.result = ptr;
                add.left = base_ptr;
                add.right = byte_offset;
                insert_before[init_insert].push_back(add);
            }

            for (size_t commit_idx : commits) {
                operand advanced = make_fresh_temp(next_temp, group.ptr_type);
                icode bump;
                bump.op = icode_op::ADD;
                bump.result = advanced;
                bump.left = ptr;
                bump.right = operand::make_int(group.stride,
                                               type::make_int());
                insert_before[commit_idx + 1].push_back(bump);

                icode commit;
                commit.op = icode_op::ASSIGN;
                commit.result = ptr;
                commit.left = advanced;
                insert_before[commit_idx + 1].push_back(commit);
            }

            for (size_t access_idx : group.direct_accesses) {
                icode &access = fn.icodes[access_idx];
                if (access.op == icode_op::GET_VALUE_AT)
                    access.left = ptr;
                else
                    access.result = ptr;
                access.right = operand::make_none();
                direct_accesses_consumed.insert(access_idx);
            }
            for (int temp_id : group.address_temps)
                replace_temp[temp_id] = ptr;
            for (int temp_id : group.secondary_inductions) {
                bool all_addresses_subsumed = true;
                for (const auto &use : fn.icodes) {
                    const bool used_left = use.left.is_temp() &&
                                           use.left.temp_id == temp_id;
                    const bool used_right = use.right.is_temp() &&
                                            use.right.temp_id == temp_id;
                    if (!used_left && !used_right)
                        continue;
                    const bool self_step =
                        use.op == icode_op::ADD && use.result.is_temp() &&
                        use.result.temp_id == temp_id && used_left;
                    const bool replaced_address =
                        use.op == icode_op::ADD && use.result.is_temp() &&
                        replace_temp.count(use.result.temp_id) != 0;
                    if (!self_step && !replaced_address) {
                        all_addresses_subsumed = false;
                        break;
                    }
                }
                if (!all_addresses_subsumed)
                    continue;
                auto defs = temp_defs.find(temp_id);
                if (defs != temp_defs.end())
                    dead_secondary_induction_defs.insert(defs->second.begin(),
                                                         defs->second.end());
            }
            changed = true;
        }

        if (!changed)
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

        // Replacing every memory use of `base + index` leaves the old address
        // calculation dead.  This lowering is terminal, so ordinary DCE will
        // not get another turn; remove only the now-unused, side-effect-free
        // definitions that this pass itself made obsolete.
        std::unordered_set<int> used_temps;
        for (const auto &ic : fn.icodes) {
            for_each_use_operand(ic, [&](const operand &op) {
                if (op.is_temp())
                    used_temps.insert(op.temp_id);
            });
        }
        std::unordered_set<size_t> dead_addresses;
        dead_addresses.insert(dead_secondary_induction_defs.begin(),
                              dead_secondary_induction_defs.end());
        for (size_t i = 0; i < fn.icodes.size(); ++i) {
            const icode &ic = fn.icodes[i];
            if (!ic.result.is_temp() ||
                replace_temp.count(ic.result.temp_id) == 0 ||
                used_temps.count(ic.result.temp_id) != 0 ||
                !is_removable_if_dead(ic.op)) {
                continue;
            }
            dead_addresses.insert(i);
        }
        fn.icodes = rebuild_with_insertions(fn.icodes, dead_addresses,
                                            insert_before);

        // The affine form may replace the last uses of an entire address
        // expression (`cast; add invariant; shift; add base`).  Since this is
        // a terminal lowering, perform a conservative temp-only dead-chain
        // sweep here instead of leaving those calculations in the hot loop.
        // Instructions with side effects are never candidates.
        for (;;) {
            std::unordered_set<int> live_temp_uses;
            for (const auto &ic : fn.icodes) {
                for_each_use_operand(ic, [&](const operand &op) {
                    if (op.is_temp())
                        live_temp_uses.insert(op.temp_id);
                });
            }
            std::unordered_set<size_t> dead;
            for (size_t i = 0; i < fn.icodes.size(); ++i) {
                const icode &ic = fn.icodes[i];
                if (ic.result.is_temp() &&
                    live_temp_uses.count(ic.result.temp_id) == 0 &&
                    is_removable_if_dead(ic.op)) {
                    dead.insert(i);
                }
            }
            if (dead.empty())
                break;
            fn.icodes = rebuild_with_insertions(fn.icodes, dead, {});
        }
        return true;
    }

private:
    bool size_bias_ = false;
};

// Strength reduction can expose a byte offset as its own loop-carried value:
//
//     off = off + sizeof(element);
//     value = *(base + off);
//
// Keeping `off` still makes the backend reconstruct base+off for every field
// access.  Turn that secondary induction variable into a pointer only when it
// has one constant update in the unique latch and no uses other than address
// formation.  Initializing from the preheader value also handles non-zero
// starting offsets without making assumptions about the expression that
// produced it.
class dead_call_result_elide_pass final : public ir_pass {
public:
    const char *name() const override { return "dead_call_result_elide"; }

    bool run(ir_function &fn) override {
        if (fn.icodes.empty())
            return false;

        std::unordered_set<int> used_temps;
        for (const auto &ic : fn.icodes) {
            for_each_use_operand(ic, [&](const operand &op) {
                if (op.is_temp())
                    used_temps.insert(op.temp_id);
            });
        }

        bool changed = false;
        for (auto &ic : fn.icodes) {
            if (ic.op != icode_op::CALL || !ic.result.is_temp())
                continue;
            if (used_temps.count(ic.result.temp_id) != 0)
                continue;
            ic.result = operand::make_none();
            changed = true;
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
    if (settings.jump_threading)
        passes.push_back(std::make_unique<repeated_compare_edge_fold_pass>());
    if (settings.address_deref_fold) {
        const bool canonicalize_fixed_addresses =
            settings.level == opt_level::Of ||
            settings.level == opt_level::O3;
        passes.push_back(std::make_unique<address_deref_fold_pass>(
            canonicalize_fixed_addresses));
    }
    if (settings.value_propagation ||
        settings.algebraic_simplify ||
        settings.dead_code_elim)
        passes.push_back(std::make_unique<global_address_const_pass>());
    if (settings.scalar_local_promotion)
        passes.push_back(std::make_unique<scalar_local_promotion_pass>());
    if (settings.reg_param_promotion)
        passes.push_back(std::make_unique<reg_param_promotion_pass>());
    if (settings.reg_param_promotion)
        passes.push_back(
            std::make_unique<immutable_stack_pointer_param_promotion_pass>());
    if (settings.tail_recursion_elim)
        passes.push_back(std::make_unique<tail_recursion_elim_pass>());
    if (settings.value_propagation ||
        settings.level == opt_level::Of ||
        settings.level == opt_level::O3)
        passes.push_back(std::make_unique<compare_bool_normalize_pass>());
    if (settings.short_circuit_bool_ifx)
        passes.push_back(std::make_unique<short_circuit_bool_ifx_pass>());
    if (settings.value_propagation || settings.short_circuit_bool_ifx)
        passes.push_back(std::make_unique<ifx_bool_wrapper_elide_pass>());
    if (settings.cfg_cleanup)
        passes.push_back(std::make_unique<boolean_phi_ifx_fold_pass>());
    if (settings.branch_bool_arithmetic)
        passes.push_back(std::make_unique<branch_bool_arithmetic_pass>());
    if (settings.value_propagation ||
        settings.level == opt_level::Of ||
        settings.level == opt_level::O3)
        passes.push_back(std::make_unique<compare_bool_normalize_pass>());
    if (settings.narrow_counted_byte_loops)
        passes.push_back(std::make_unique<bounded_word_counter_narrow_pass>());
    if (settings.narrow_counted_byte_loops)
        passes.push_back(std::make_unique<narrow_stack_counted_byte_loops_pass>(
            settings.level == opt_level::O3 ||
            settings.level == opt_level::Of));
    if (settings.narrow_counted_byte_loops)
        passes.push_back(std::make_unique<narrow_counted_byte_loops_pass>(
            settings.level == opt_level::Of ||
            settings.level == opt_level::O3 ||
            settings.level == opt_level::Os));
    if (settings.countdown_dead_loops)
        passes.push_back(std::make_unique<countdown_dead_loops_pass>());
    // The legacy address-temp replacement pass can lose its synthetic pointer
    // increment when subsequent loop cleanup rematerializes the preheader
    // address (turning `array[i]` into repeated `array[0]`).  The later
    // loop_lockstep_pointer pass performs the same transformation with
    // explicit backedge and overflow proofs, so keep only that implementation.
    if (settings.promoted_byte_compare)
        passes.push_back(std::make_unique<direct_byte_eq_ne_pass>());
    if (settings.promoted_byte_compare)
        passes.push_back(std::make_unique<promoted_byte_compare_pass>());
    if (settings.promoted_byte_ops)
        // Truncated multiplication is especially valuable on Z80: keeping
        // it byte-wide removes widening, a 16-bit shift/add chain, and the
        // high-byte spill. The pass proves that only the low byte reaches an
        // observable sink before applying this mode.
        passes.push_back(std::make_unique<promoted_byte_ops_pass>(true));
    if (settings.promoted_byte_ops)
        passes.push_back(std::make_unique<pack_bytes_pass>());
    if (settings.promoted_byte_ops)
        passes.push_back(std::make_unique<adjacent_pack_word_load_pass>());
    if (settings.promoted_byte_ops)
        passes.push_back(std::make_unique<adjacent_pack_u32_load_pass>());
    if (settings.promoted_byte_ops)
        passes.push_back(std::make_unique<label_indexed_byte_access_pass>());
    // loop_lockstep_pointer is deliberately run once after the iterative
    // pipeline.  It introduces a loop-carried synthetic pointer temp; feeding
    // that temp back through copy propagation on the next fixed-point round
    // can rematerialize its preheader value and discard the backedge update.
    if (settings.block_fill_loops)
        passes.push_back(std::make_unique<block_fill_loop_pass>());
    if (settings.promoted_byte_ops)
        passes.push_back(std::make_unique<available_byte_load_pass>(
            settings.level != opt_level::Os));
    if (settings.value_propagation)
        passes.push_back(std::make_unique<local_word_store_forward_pass>());
    if (settings.value_propagation ||
        settings.level == opt_level::Of ||
        settings.level == opt_level::O3)
        passes.push_back(std::make_unique<available_word_load_pass>(
            settings.level != opt_level::Os));
    if (settings.rotate_combine)
        passes.push_back(std::make_unique<rotate_combine_pass>());
    if (settings.value_propagation)
        passes.push_back(std::make_unique<value_propagation_pass>());
    if (settings.value_propagation || settings.algebraic_simplify)
        passes.push_back(std::make_unique<global_scalar_remat_pass>());
    if (settings.constant_folding || settings.algebraic_simplify)
        passes.push_back(std::make_unique<temp_constant_propagation_pass>());
    if (settings.value_propagation ||
        settings.algebraic_simplify ||
        settings.dead_code_elim)
        passes.push_back(std::make_unique<immutable_const_temp_remat_pass>());
    if (settings.value_propagation ||
        settings.algebraic_simplify ||
        settings.dead_code_elim)
        passes.push_back(std::make_unique<calc_temp_fusion_pass>());
    if (settings.promoted_byte_ops)
        passes.push_back(std::make_unique<branch_copy_sink_coalesce_pass>());
    if (settings.value_propagation ||
        settings.algebraic_simplify ||
        settings.dead_code_elim)
        passes.push_back(std::make_unique<noop_temp_assign_elide_pass>());
    if (settings.constant_folding)
        passes.push_back(std::make_unique<constant_fold_pass>());
    if (settings.algebraic_simplify)
        passes.push_back(std::make_unique<bitwise_select_simplify_pass>());
    if (settings.level == opt_level::Of ||
        settings.level == opt_level::O3 ||
        settings.level == opt_level::Os)
        passes.push_back(std::make_unique<direct_truncated_byte_ops_pass>());
    if (settings.level == opt_level::Of ||
        settings.level == opt_level::O3 ||
        settings.level == opt_level::Os)
        passes.push_back(std::make_unique<msb_masked_shift_xor_pass>());
    if (settings.algebraic_simplify)
        passes.push_back(std::make_unique<algebraic_simplify_pass>());
    if (settings.algebraic_simplify)
        passes.push_back(std::make_unique<add_const_chain_pass>());
    if (settings.algebraic_simplify)
        passes.push_back(std::make_unique<post_update_recover_pass>());
    if (settings.local_cse)
        passes.push_back(std::make_unique<local_cse_pass>());
    if (settings.loop_licm)
        passes.push_back(std::make_unique<loop_licm_pass>());
    if (settings.loop_induction)
        passes.push_back(std::make_unique<loop_induction_pass>(
            settings.level != opt_level::Os));
    if (settings.loop_induction)
        passes.push_back(std::make_unique<one_trip_counted_loop_fold_pass>());
    if (settings.strength_reduction)
        passes.push_back(std::make_unique<strength_reduction_pass>());
    if (settings.local_cse)
        passes.push_back(std::make_unique<linear_expr_cse_pass>());
    if (settings.dead_code_elim)
        passes.push_back(std::make_unique<dead_call_result_elide_pass>());
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
    // Pathologically large, machine-generated functions (e.g. the C23
    // translation-limit stress tests with a 4095-token expression and 127
    // nested blocks) make the analysis-heavy passes super-linear: value
    // propagation compares whole dataflow environments, and the loop passes
    // run an iterative dominator computation.  For such oversized functions,
    // drop those passes and keep only the cheaper, local optimizations.
    // Correctness is unaffected — these are all optional.
    constexpr size_t kLargeFunctionInsns = 2000;
    optimization_settings eff = settings;
    if (fn.icodes.size() > kLargeFunctionInsns) {
        eff.value_propagation   = false;
        eff.local_cse           = false;
        eff.loop_licm           = false;
        eff.loop_induction      = false;
        eff.tail_recursion_elim = false;
        eff.strength_reduction  = false;
        eff.jump_threading      = false;
        eff.merge_identical_functions = false;
    }

    // Scalar promotion is profitable for pointer/counter kernels but can
    // lengthen the selector live range across dense switch dispatch. Detect
    // the same source-independent EQ/IFX chain shape used by jump-table
    // selection and keep those functions on the stack-homed scalar path.
    bool dense_switch_chain = false;
    if (eff.scalar_local_promotion) {
        for (size_t start = 0; start + 1 < fn.icodes.size() &&
                               !dense_switch_chain; ++start) {
            int selector_tid = -1;
            int cases = 0;
            size_t pos = start;
            while (pos + 1 < fn.icodes.size()) {
                const auto &cmp = fn.icodes[pos];
                const auto &branch = fn.icodes[pos + 1];
                if (cmp.op != icode_op::EQ || branch.op != icode_op::IFX ||
                    !cmp.result.is_temp() || !branch.left.is_temp() ||
                    cmp.result.temp_id != branch.left.temp_id)
                    break;
                const operand *selector = &cmp.left;
                const operand *constant = &cmp.right;
                if (selector->kind == operand_kind::INT_CONST)
                    std::swap(selector, constant);
                if (!selector->is_temp() ||
                    constant->kind != operand_kind::INT_CONST ||
                    (selector_tid >= 0 && selector_tid != selector->temp_id))
                    break;
                selector_tid = selector->temp_id;
                ++cases;
                pos += 2;
            }
            dense_switch_chain = cases >= 4;
        }
        if (dense_switch_chain)
            eff.scalar_local_promotion = false;
    }

    auto passes = build_pipeline(eff);
    for (int iter = 0; iter < 16; ++iter) {
        bool changed = false;
        for (auto &pass : passes)
            changed |= pass->run(fn);
        if (!changed) break;
    }

    // This lowering must be terminal: later temp-rematerialization and DCE
    // passes do not model a newly introduced loop-carried temp as a phi value.
    if (eff.loop_pointer_walk) {
        // Size mode selects one group per loop.  The speed profiles take one
        // additional bounded passes: a few independent running pointers are
        // often cheaper than reconstructing affine word addresses in a hot
        // loop, while the fixed bound prevents unbounded spill growth.
        const bool size_bias = eff.level == opt_level::Os;
        const bool function_has_call = std::any_of(
            fn.icodes.begin(), fn.icodes.end(),
            [](const icode &ic) { return ic.op == icode_op::CALL; });
        // A second synthetic cursor changes the spill/liveness shape across
        // calls enough that the current terminal pass cannot prove every
        // rematerialized argument relationship.  Retain the broadly tested
        // single-cursor lowering for such functions; call-free kernels can
        // safely take the bounded multi-cursor speed path.
        const int pointer_rounds =
            (size_bias || function_has_call) ? 1 : 4;
        for (int round = 0; round < pointer_rounds; ++round) {
            loop_lockstep_pointer_pass final_pointer_walk(size_bias);
            if (!final_pointer_walk.run(fn))
                break;
        }
    }
    // Pointer walking removes indexed uses of the source induction variable.
    // Promote a now-control-only scalar and then give each exposed loop a
    // bounded opportunity to become a compare-free countdown.  These passes
    // only rewrite existing operands/control flow and are safe after the
    // terminal pointer lowering (unlike rematerialization or DCE).
    if (eff.scalar_local_promotion) {
        // No SSA/value-propagation pass runs after this point, so an
        // unaliased scalar with multiple control-flow definitions can safely
        // become one backend virtual register.  Earlier promotion remains
        // conservative because those passes do reason about SSA-like temps.
        scalar_local_promotion_pass final_scalar_promotion(true);
        final_scalar_promotion.run(fn);
        // Terminal promotion exposes mutable loop state as a virtual temp.
        // Reuse branch-common scaled indices and addresses without feeding
        // those multi-definition temps back through SSA-like propagation.
        if (eff.level == opt_level::Of || eff.level == opt_level::O3) {
            for (int round = 0; round < 4; ++round) {
                branch_common_expr_hoist_pass branch_pre;
                if (!branch_pre.run(fn))
                    break;
            }
        }
        local_frame_compaction_pass final_frame_compaction;
        final_frame_compaction.run(fn);
    }
    if (eff.reg_param_promotion) {
        stack_param_promotion_pass final_stack_param;
        final_stack_param.run(fn);
    }
    if (eff.countdown_dead_loops) {
        for (int loop = 0; loop < 4; ++loop) {
            countdown_dead_loops_pass final_countdown;
            if (!final_countdown.run(fn))
                break;
        }
    }
    // Pointer walking exposes constant byte fills as a pointer/counter pair.
    // Give the block-fill recognizer one terminal opportunity to consume that
    // shape; neither lowering may be fed back through rematerialization/DCE.
    if (eff.block_fill_loops) {
        block_fill_loop_pass final_block_fill;
        if (final_block_fill.run(fn)) {
            local_frame_compaction_pass post_fill_frame_compaction;
            post_fill_frame_compaction.run(fn);
        }
    }
    // These two target forms deliberately use narrower source operands with
    // a wider result.  That is useful to Z80 code generation, but it is not a
    // normal language-level arithmetic form: feeding it back into constant
    // folding can truncate an inlined expression at the source width.  Make
    // the rewrites terminal and have each pass consume its now-dead CASTs.
    if (eff.promoted_byte_compare) {
        widened_byte_operand_forward_pass final_widened_bytes;
        final_widened_bytes.run(fn);
    }
    if (eff.promoted_byte_ops) {
        widened_unsigned_word_mul_pass final_widened_word_mul;
        final_widened_word_mul.run(fn);
    }
    // Keep this after every propagation/DCE and structural loop lowering.
    // Its purpose is to preserve two distinct allocation intervals; feeding
    // the inserted copy back into the fixed-point pipeline would immediately
    // coalesce them again.
    if (eff.regalloc && eff.loop_induction) {
        loop_live_range_split_pass final_live_range_split;
        final_live_range_split.run(fn);
    }
}

} // namespace xcc
