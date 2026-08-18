//
// irgen.cpp — AST to three-address IR lowering: constructor, helpers,
//             and module entry point.
//
// The IR generation implementation is split across several translation units:
//   irgen.cpp           — this file (constructor, helpers, lower)
//   irgen_decl.cpp      — declaration visitors and gen_func
//   irgen_stmt.cpp      — statement visitors
//   irgen_expr.cpp      — expression visitors and arithmetic helpers
//   irgen_lvalue.cpp    — lvalue read/write, index/member access
//   irgen_init.cpp      — aggregate initialiser lowering
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//
#include "ir/irgen.h"
#include "backend/z80/convention.h"
#include <cassert>
#include <cmath>
#include <unordered_map>

namespace xcc {

namespace {

std::string string_literal_pool_key(const ir_module::global_var &gv) {
    std::string key;
    key.reserve(gv.str_init.size() + 2);
    key.push_back(static_cast<char>(gv.char_width & 0xff));
    key.push_back('\0');
    key += gv.str_init;
    return key;
}

void remap_string_literal_operand(
    operand &op,
    const std::unordered_map<std::string, std::string> &aliases) {
    if (!op.is_symbol())
        return;
    auto found = aliases.find(op.name);
    if (found == aliases.end())
        return;
    op.name = found->second;
}

void merge_duplicate_string_literals(ir_module &mod) {
    if (mod.string_literals.empty())
        return;

    std::unordered_map<std::string, std::string> canonical_by_key;
    std::unordered_map<std::string, std::string> aliases;
    std::vector<ir_module::global_var> pooled;
    pooled.reserve(mod.string_literals.size());

    for (const auto &gv : mod.string_literals) {
        const std::string key = string_literal_pool_key(gv);
        auto found = canonical_by_key.find(key);
        if (found == canonical_by_key.end()) {
            canonical_by_key.emplace(key, gv.name);
            pooled.push_back(gv);
        } else {
            aliases.emplace(gv.name, found->second);
        }
    }

    if (aliases.empty())
        return;

    for (auto &gv : mod.globals) {
        for (auto &elem : gv.init_vals) {
            auto found = aliases.find(elem.label);
            if (found != aliases.end())
                elem.label = found->second;
        }
    }

    for (auto &fn : mod.functions) {
        for (auto &ic : fn.icodes) {
            remap_string_literal_operand(ic.result, aliases);
            remap_string_literal_operand(ic.left, aliases);
            remap_string_literal_operand(ic.right, aliases);
        }
    }

    mod.string_literals = std::move(pooled);
}

enum class escaped_format_family { NONE, PRINTF, SCANF };

escaped_format_family format_family_for_symbol(const std::string &name) {
    static const char *const printf_names[] = {
        "printf", "printk", "fprintf", "sprintf", "snprintf",
        "vprintf", "vfprintf", "vsprintf", "vsnprintf",
    };
    static const char *const scanf_names[] = {
        "scanf", "fscanf", "sscanf", "vscanf", "vfscanf", "vsscanf",
    };
    for (const char *candidate : printf_names) {
        if (name == candidate)
            return escaped_format_family::PRINTF;
    }
    for (const char *candidate : scanf_names) {
        if (name == candidate)
            return escaped_format_family::SCANF;
    }
    return escaped_format_family::NONE;
}

void mark_escaped_format_symbol(ir_module &mod, const std::string &name,
                                const std::unordered_set<std::string> &definitions) {
    if (definitions.count(name))
        return;
    switch (format_family_for_symbol(name)) {
    case escaped_format_family::PRINTF:
        mod.printf_formats.used = true;
        mod.printf_formats.requires_full = true;
        break;
    case escaped_format_family::SCANF:
        mod.scanf_formats.used = true;
        mod.scanf_formats.requires_full = true;
        break;
    case escaped_format_family::NONE:
        break;
    }
}

void mark_escaped_format_functions(
    ir_module &mod,
    const std::unordered_set<std::string> &definitions) {
    for (const auto &gv : mod.globals) {
        for (const auto &elem : gv.init_vals) {
            if (!elem.label.empty())
                mark_escaped_format_symbol(mod, elem.label, definitions);
        }
    }
    for (const auto &fn : mod.functions) {
        for (const auto &ic : fn.icodes) {
            const operand *ops[] = {&ic.result, &ic.left, &ic.right};
            for (const operand *op : ops) {
                if (op->is_symbol() && op->is_func)
                    mark_escaped_format_symbol(mod, op->name, definitions);
            }
        }
    }
}

} // namespace

// ----- Helpers -------------------------------------------------------

ir_gen::ir_gen() {}

operand ir_gen::new_temp(type_ptr t) {
    return operand::make_temp(next_temp_++, t);
}

std::string ir_gen::new_label() {
    return "__xcc_L" + std::to_string(next_lbl_++);
}

void ir_gen::emit(icode ic) {
    assert(cur_fn_ && "emit outside function");
    ic.line = cur_line_;
    cur_fn_->icodes.push_back(std::move(ic));
}

operand ir_gen::coerce_const_operand(operand op, const type_ptr &target) {
    if (!target)
        return op;

    auto narrow_int = [](int64_t value, const type_ptr &ty) -> int64_t {
        if (!ty)
            return value;
        if (ty->kind == type_kind::BOOL)
            return value != 0 ? 1 : 0;
        int bytes = ty->size();
        if (bytes <= 0 || bytes >= 8)
            return value;
        uint64_t mask = (uint64_t{1} << (bytes * 8)) - 1;
        return static_cast<int64_t>(static_cast<uint64_t>(value) & mask);
    };

    if (op.kind == operand_kind::INT_CONST &&
        (target->kind == type_kind::FLOAT ||
         target->kind == type_kind::DOUBLE)) {
        return operand::make_float(static_cast<double>(op.ival), target);
    }
    if (op.kind == operand_kind::FLOAT_CONST && target->is_integer()) {
        return operand::make_int(
            narrow_int(static_cast<int64_t>(op.fval), target), target);
    }
    if (op.kind == operand_kind::INT_CONST && target->is_integer()) {
        return operand::make_int(narrow_int(op.ival, target), target);
    }
    if (op.kind == operand_kind::INT_CONST ||
        op.kind == operand_kind::FLOAT_CONST) {
        op.type = target;
    }
    return op;
}

operand ir_gen::gen_expr(expr &e) {
    e.accept(*this);
    return expr_result_;
}

void ir_gen::gen_stmt(stmt &s) {
    if (s.loc.line > 0) cur_line_ = s.loc.line;
    s.accept(*this);
}

void ir_gen::gen_decl(decl &d) { d.accept(*this); }

operand ir_gen::sym_to_operand(const symbol &sym, type_ptr ty) {
    const std::string &name = !sym.asm_name.empty() ? sym.asm_name : sym.name;
    bool is_sfr = (sym.sfr_port >= 0);
    return operand::make_symbol(name, ty ? ty : sym.type,
                                sym.is_global, sym.is_param,
                                sym.stack_offset, sym.kind == sym_kind::FUNC,
                                sym.is_tls,
                                is_sfr, sym.sfr_port);
}

operand ir_gen::promote(operand op) {
    if (!op.type || !op.type->is_integer()) return op;
    if (op.type->size() < 2) {
        operand t = new_temp(type::make_int());
        icode ic;
        ic.op     = icode_op::CAST;
        ic.result = t;
        ic.left   = op;
        emit(ic);
        return t;
    }
    return op;
}

void ir_gen::emit_assign(operand dst, operand src) {
    icode ic;
    ic.op     = icode_op::ASSIGN;
    ic.result = dst;
    ic.left   = src;
    emit(ic);
}

operand ir_gen::emit_binop(icode_op op, operand left, operand right, type_ptr t) {
    if (t) {
        if (left.kind == operand_kind::FLOAT_CONST)
            left.type = t;
        if (right.kind == operand_kind::FLOAT_CONST)
            right.type = t;
    }
    operand res = new_temp(t);
    icode ic; ic.op = op; ic.result = res; ic.left = left; ic.right = right;
    emit(ic);
    return res;
}

operand ir_gen::emit_unop(icode_op op, operand left, type_ptr t) {
    if (op == icode_op::CAST && t && t->kind == type_kind::COMPLEX &&
        (!left.type || left.type->kind != type_kind::COMPLEX)) {
        type_ptr float_t = type::make_float();
        operand real = left;

        if (real.kind == operand_kind::INT_CONST ||
            real.kind == operand_kind::FLOAT_CONST) {
            real = coerce_const_operand(real, float_t);
        } else if (!real.type || real.type->kind != type_kind::FLOAT) {
            real = emit_unop(icode_op::CAST, real, float_t);
        } else {
            real.type = float_t;
        }

        operand imag = operand::make_float(0.0, float_t);
        operand res = new_temp(t);
        icode ic;
        ic.op = icode_op::MAKE_COMPLEX;
        ic.result = res;
        ic.left = real;
        ic.right = imag;
        emit(ic);
        return res;
    }

    if (t && left.kind == operand_kind::FLOAT_CONST)
        left.type = t;
    operand res = new_temp(t);
    icode ic; ic.op = op; ic.result = res; ic.left = left;
    emit(ic);
    return res;
}

// ----- Module entry --------------------------------------------------

std::unique_ptr<ir_module> ir_gen::lower(translation_unit &tu) {
    mod_ = std::make_unique<ir_module>();
    defined_function_names_.clear();
    for (const auto &d : tu.decls) {
        const auto *fn = dynamic_cast<const func_decl *>(d.get());
        if (fn && fn->body)
            defined_function_names_.insert(fn->name);
    }
    for (auto &d : tu.decls)
        if (d) gen_decl(*d);
    merge_duplicate_string_literals(*mod_);
    mark_escaped_format_functions(*mod_, defined_function_names_);
    return std::move(mod_);
}

} // namespace xcc
