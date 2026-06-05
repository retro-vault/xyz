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

#include <algorithm>
#include <cstdint>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace xcc {
namespace {

struct alias_info {
    std::unordered_set<std::string> address_taken_symbols;
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
    case icode_op::EQ: case icode_op::NE:
    case icode_op::LT: case icode_op::LE: case icode_op::GT: case icode_op::GE:
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
    switch (type->size()) {
    case 1: return static_cast<int8_t>(v & 0xFF);
    case 2: return static_cast<int16_t>(v & 0xFFFF);
    case 4: return static_cast<int32_t>(v & 0xFFFFFFFFLL);
    default: return v;
    }
}

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

static alias_info build_alias_info(const ir_function &fn) {
    alias_info info;
    for (auto &ic : fn.icodes) {
        if (ic.op == icode_op::ADDRESS_OF && ic.left.is_symbol())
            info.address_taken_symbols.insert(symbol_key(ic.left));
    }
    return info;
}

static bool is_trackable_symbol(const operand &op, const alias_info &alias) {
    if (!op.is_symbol()) return false;
    if (op.is_global || op.is_tls || op.is_sfr) return false;
    if (op.byte_offset != 0) return false;
    return alias.address_taken_symbols.find(symbol_key(op)) ==
           alias.address_taken_symbols.end();
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

static bool is_all_ones(int64_t v) {
    return v == -1 || v == 0xFF || v == 0xFFFF || v == 0xFFFFFFFFLL;
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
        int64_t cval = left_const ? lhs.ival : rhs.ival;
        const ssa_value &other = left_const ? rhs : lhs;
        switch (ic.op) {
        case icode_op::ADD:
            if (cval == 0) return other;
            break;
        case icode_op::SUB:
            if (right_const && cval == 0) return lhs;
            break;
        case icode_op::MUL:
            if (cval == 0) return ssa_value::int_const(0);
            if (cval == 1) return other;
            break;
        case icode_op::DIV:
            if (right_const && cval == 1) return lhs;
            break;
        case icode_op::BAND:
            if (cval == 0) return ssa_value::int_const(0);
            if (is_all_ones(cval)) return other;
            break;
        case icode_op::BOR:
        case icode_op::BXOR:
            if (cval == 0) return other;
            break;
        case icode_op::SHL:
        case icode_op::SHR:
            if (right_const && cval == 0) return lhs;
            break;
        default:
            break;
        }
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

class constant_fold_pass final : public ir_pass {
public:
    const char *name() const override { return "constant_fold"; }

    bool run(ir_function &fn) override {
        bool changed = false;
        for (auto &ic : fn.icodes) {
            if (is_binary_foldable(ic.op) &&
                ic.left.kind == operand_kind::INT_CONST &&
                ic.right.kind == operand_kind::INT_CONST) {
                ic.op    = icode_op::ASSIGN;
                ic.left  = operand::make_int(fold_binary(ic.op, ic.left.ival, ic.right.ival),
                                             ic.result.type);
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
            if (!left_const && !right_const) continue;

            int64_t cval = left_const ? ic.left.ival : ic.right.ival;
            auto replace_with = [&](operand src) {
                ic.op    = icode_op::ASSIGN;
                ic.left  = src;
                ic.right = operand::make_none();
                changed  = true;
            };
            auto replace_with_zero = [&]() {
                replace_with(operand::make_int(0, ic.result.type));
            };
            auto replace_with_var = [&]() {
                replace_with(left_const ? ic.right : ic.left);
            };

            switch (ic.op) {
            case icode_op::ADD:
                if (cval == 0) replace_with_var();
                break;
            case icode_op::SUB:
                if (right_const && cval == 0) replace_with_var();
                break;
            case icode_op::MUL:
                if (cval == 0) replace_with_zero();
                else if (cval == 1) replace_with_var();
                break;
            case icode_op::DIV:
                if (right_const && cval == 1) replace_with_var();
                break;
            case icode_op::BAND:
                if (cval == 0) replace_with_zero();
                else if (is_all_ones(cval)) replace_with_var();
                break;
            case icode_op::BOR:
            case icode_op::BXOR:
                if (cval == 0) replace_with_var();
                break;
            case icode_op::SHL:
            case icode_op::SHR:
                if (right_const && cval == 0) replace_with_var();
                break;
            default:
                break;
            }
        }
        return changed;
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
            for (size_t block_id : loop.blocks) {
                const auto &block = cfg.block(block_id);
                for (size_t i = block.begin; i < block.end; ++i) {
                    std::string key = def_key(fn.icodes[i], alias);
                    if (!key.empty()) defs_in_loop.insert(key);
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
                        std::string key = trackable_key(ic.result, alias);
                        if (!key.empty()) hoisted_defs.insert(key);
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

} // namespace

std::vector<std::unique_ptr<ir_pass>> ir_optimizer::build_pipeline() {
    std::vector<std::unique_ptr<ir_pass>> passes;
    passes.push_back(std::make_unique<cfg_cleanup_pass>());
    passes.push_back(std::make_unique<value_propagation_pass>());
    passes.push_back(std::make_unique<constant_fold_pass>());
    passes.push_back(std::make_unique<algebraic_simplify_pass>());
    passes.push_back(std::make_unique<loop_licm_pass>());
    passes.push_back(std::make_unique<loop_induction_pass>());
    passes.push_back(std::make_unique<strength_reduction_pass>());
    passes.push_back(std::make_unique<dead_code_elim_pass>());
    return passes;
}

void ir_optimizer::optimize(ir_function &fn) {
    auto passes = build_pipeline();
    for (int iter = 0; iter < 16; ++iter) {
        bool changed = false;
        for (auto &pass : passes)
            changed |= pass->run(fn);
        if (!changed) break;
    }
}

} // namespace xcc
