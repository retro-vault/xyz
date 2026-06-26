//
// irgen_decl.cpp — declaration visitors for the xcc IR lowering pass.
//
// Covers: typedef_decl, struct_decl, param_decl, var_decl, func_decl.
// Global aggregate initialiser collection (collect_global_inits) lives
// here because it is only needed when lowering global var_decl nodes.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//
#include "ir/irgen.h"
#include "backend/z80/convention.h"
#include "frontend/const_eval.h"
#include <cassert>

namespace xcc {

// ----- Global aggregate init helpers ---------------------------------

static bool is_char_pointer_type(type_ptr ty)
{
    if (!ty || ty->kind != type_kind::POINTER || !ty->base)
        return false;
    type_kind elem = ty->base->unqual()->kind;
    return elem == type_kind::CHAR ||
           elem == type_kind::UCHAR ||
           elem == type_kind::CHAR8T;
}

static const string_literal_expr *unwrap_string_literal(expr *init)
{
    while (init) {
        if (auto *str = dynamic_cast<string_literal_expr*>(init))
            return str;
        auto *cast = dynamic_cast<cast_expr*>(init);
        if (!cast || !cast->operand)
            break;
        init = cast->operand.get();
    }
    return nullptr;
}

static std::string add_global_string_literal(ir_module &mod, int &next_lbl,
                                             const string_literal_expr &str)
{
    std::string lbl = "__str_" + std::to_string(next_lbl++);
    ir_module::global_var gv;
    gv.name       = lbl;
    gv.type       = type::make_array(type::make_char(),
                    static_cast<int>(str.value.size()) + 1);
    gv.str_init   = str.value;
    gv.char_width = str.char_width;
    gv.has_init   = true;
    mod.string_literals.push_back(std::move(gv));
    return lbl;
}

static bool is_char_array_type(type_ptr ty)
{
    if (!ty || ty->kind != type_kind::ARRAY || !ty->base)
        return false;
    type_kind elem = ty->base->unqual()->kind;
    return elem == type_kind::CHAR ||
           elem == type_kind::UCHAR ||
           elem == type_kind::CHAR8T;
}

static ir_module::global_var::init_elem make_init_elem(int64_t value, int size,
                                                       std::string label = {})
{
    ir_module::global_var::init_elem elem;
    elem.value = value;
    elem.size = size;
    elem.label = std::move(label);
    return elem;
}

static void append_zero_bytes(int count,
                              std::vector<ir_module::global_var::init_elem> &out)
{
    for (int i = 0; i < count; ++i)
        out.push_back(make_init_elem(0, 1));
}

static void append_string_bytes(const string_literal_expr &str, type_ptr array_type,
                                std::vector<ir_module::global_var::init_elem> &out)
{
    int n = array_type ? array_type->size() : 0;
    int init_bytes = static_cast<int>(str.value.size()) + 1;
    if (n > 0 && init_bytes > n)
        init_bytes = n;
    for (int i = 0; i < init_bytes; ++i) {
        int ch = 0;
        if (i < static_cast<int>(str.value.size()))
            ch = static_cast<unsigned char>(str.value[i]);
        out.push_back(make_init_elem(ch, 1));
    }
    for (int i = init_bytes; i < n; ++i)
        out.push_back(make_init_elem(0, 1));
}

static int64_t narrow_static_int(int64_t value, type_ptr ty)
{
    if (!ty)
        return value;
    if (ty->kind == type_kind::BOOL)
        return value != 0 ? 1 : 0;
    int bytes = ty->size();
    if (bytes <= 0 || bytes >= 8)
        return value;
    uint64_t mask = (uint64_t{1} << (bytes * 8)) - 1;
    return static_cast<int64_t>(static_cast<uint64_t>(value) & mask);
}

static int object_size(type_ptr ty)
{
    return ty ? ty->size() : 0;
}

static void append_zero_object(type_ptr ty,
                               std::vector<ir_module::global_var::init_elem> &out)
{
    append_zero_bytes(object_size(ty), out);
}

static void collect_global_init(expr *init, type_ptr target,
                                ir_module &mod, int &next_lbl,
                                std::vector<ir_module::global_var::init_elem> &out)
{
    if (!target) {
        if (init)
            out.push_back(make_init_elem(0, 2));
        return;
    }

    if (!init) {
        append_zero_object(target, out);
        return;
    }

    if (const auto *str = unwrap_string_literal(init);
        str && is_char_array_type(target)) {
        append_string_bytes(*str, target, out);
        return;
    }

    if (auto *il = dynamic_cast<init_list_expr*>(init)) {
        if (is_char_array_type(target) && il->elements.size() == 1) {
            if (const auto *str =
                    unwrap_string_literal(il->elements[0].value.get())) {
                append_string_bytes(*str, target, out);
                return;
            }
        }

        if (target->kind == type_kind::ARRAY && target->base) {
            type_ptr elem_t = target->base;
            int count = target->array_size;
            if (count <= 0)
                count = static_cast<int>(il->elements.size());

            std::vector<expr*> slot_inits(static_cast<size_t>(count), nullptr);
            int64_t next_idx = 0;
            for (auto &e : il->elements) {
                int64_t idx = e.array_index ? *e.array_index : next_idx;
                if (idx >= 0 && idx < count)
                    slot_inits[static_cast<size_t>(idx)] = e.value.get();
                next_idx = idx + 1;
            }

            for (expr *slot_init : slot_inits) {
                if (slot_init)
                    collect_global_init(slot_init, elem_t, mod, next_lbl, out);
                else
                    append_zero_object(elem_t, out);
            }
            return;
        }

        if ((target->kind == type_kind::STRUCT ||
             target->kind == type_kind::UNION) &&
            !target->fields.empty()) {
            std::vector<expr*> field_inits(target->fields.size(), nullptr);
            size_t seq_idx = 0;
            for (auto &e : il->elements) {
                size_t field_idx = target->fields.size();
                if (e.field_name) {
                    for (size_t fi = 0; fi < target->fields.size(); ++fi) {
                        if (target->fields[fi].name == *e.field_name) {
                            field_idx = fi;
                            seq_idx = fi + 1;
                            break;
                        }
                    }
                } else if (seq_idx < target->fields.size()) {
                    field_idx = seq_idx++;
                }

                if (field_idx < target->fields.size()) {
                    field_inits[field_idx] = e.value.get();
                    if (target->kind == type_kind::UNION)
                        break;
                }
            }

            if (target->kind == type_kind::UNION) {
                size_t chosen = target->fields.size();
                for (size_t fi = 0; fi < field_inits.size(); ++fi) {
                    if (field_inits[fi]) {
                        chosen = fi;
                        break;
                    }
                }
                if (chosen < target->fields.size())
                    collect_global_init(field_inits[chosen], target->fields[chosen].type,
                                        mod, next_lbl, out);
                else
                    append_zero_object(target->fields[0].type, out);

                int used = chosen < target->fields.size()
                    ? object_size(target->fields[chosen].type)
                    : object_size(target->fields[0].type);
                if (target->size() > used)
                    append_zero_bytes(target->size() - used, out);
                return;
            }

            int emitted = 0;
            for (size_t fi = 0; fi < target->fields.size(); ++fi) {
                const auto &fld = target->fields[fi];
                if (fld.offset > emitted)
                    append_zero_bytes(fld.offset - emitted, out);

                if (field_inits[fi])
                    collect_global_init(field_inits[fi], fld.type, mod, next_lbl, out);
                else
                    append_zero_object(fld.type, out);

                emitted = fld.offset + object_size(fld.type);
            }
            if (target->size() > emitted)
                append_zero_bytes(target->size() - emitted, out);
            return;
        }

        if (!il->elements.empty()) {
            collect_global_init(il->elements[0].value.get(), target, mod, next_lbl, out);
        } else {
            append_zero_object(target, out);
        }
        return;
    }

    if (target->is_integer()) {
        if (auto cv = const_expr_evaluator::evaluate(init)) {
            out.push_back(make_init_elem(narrow_static_int(*cv, target),
                                         target->size()));
            return;
        }
    }

    if (auto *flit = dynamic_cast<float_literal_expr*>(init)) {
        out.push_back(make_init_elem(encode_float_constant(flit->value, target),
                                     target->size()));
        return;
    }

    if (const auto *str = unwrap_string_literal(init);
        str && is_char_pointer_type(target)) {
        out.push_back(make_init_elem(0, target->size(),
                                     add_global_string_literal(mod, next_lbl, *str)));
        return;
    }

    append_zero_object(target, out);
}

// ----- Declaration visitors ------------------------------------------

void ir_gen::visit(typedef_decl &) {}
void ir_gen::visit(struct_decl  &) {}
void ir_gen::visit(param_decl   &) {}

void ir_gen::visit(var_decl &vd) {
    if (!vd.sym) return;

    auto coerce_for_decl_store = [&](operand value, const type_ptr &target) -> operand {
        if (!target)
            return value;
        if (!value.type) {
            value.type = target;
            return value;
        }
        bool same_type =
            value.type->kind == target->kind &&
            value.type->size() == target->size() &&
            value.type->is_unsigned() == target->is_unsigned();
        if (same_type) {
            value.type = target;
            return value;
        }
        value = coerce_const_operand(value, target);
        if (value.kind == operand_kind::INT_CONST ||
            value.kind == operand_kind::FLOAT_CONST)
            return value;
        return emit_unop(icode_op::CAST, value, target);
    };

    if (vd.vla_size && cur_fn_) {
        operand count = gen_expr(*vd.vla_size);
        int elem_sz = vd.type->is_ptr() ? vd.type->base->size() : 1;
        operand bytes;
        if (elem_sz == 1) {
            bytes = count;
        } else {
            bytes = new_temp(type::make_uint());
            icode ic;
            ic.op     = icode_op::MUL;
            ic.result = bytes;
            ic.left   = count;
            ic.right  = operand::make_int(elem_sz, type::make_uint());
            emit(ic);
        }
        if (vd.sym->vla_size_sym) {
            emit_assign(sym_to_operand(*vd.sym->vla_size_sym, type::make_uint()), bytes);
        }
        operand ptr_res = new_temp(vd.type);
        icode ic2;
        ic2.op     = icode_op::ALLOCA;
        ic2.result = ptr_res;
        ic2.left   = bytes;
        emit(ic2);
        emit_assign(sym_to_operand(*vd.sym, vd.type), ptr_res);

        // C23: int vla[n] = {} → zero-initialise the allocated region.
        if (vd.init) {
            auto *il = dynamic_cast<init_list_expr*>(vd.init.get());
            bool is_empty_init = il && il->elements.empty();
            if (is_empty_init) {
                // Emit: __vla_zero(ptr_res, bytes)
                icode sz_ic;
                sz_ic.op         = icode_op::SEND;
                sz_ic.left       = bytes;
                sz_ic.argreg     = 1; // second arg (byte count)
                emit(sz_ic);
                icode ptr_ic;
                ptr_ic.op        = icode_op::SEND;
                ptr_ic.left      = ptr_res;
                ptr_ic.argreg    = 0; // first arg (pointer)
                emit(ptr_ic);
                icode call_ic;
                call_ic.op        = icode_op::CALL;
                call_ic.func_name = "__vla_zero";
                call_ic.num_params = 2;
                call_ic.arg_bytes  = 4; // 2 bytes ptr + 2 bytes count
                emit(call_ic);
            }
        }
        return;
    }

    if (vd.sym->is_global) {
        if (vd.sym->storage == storage_class::EXTERN) return;

        ir_module::global_var gv;
        gv.name       = vd.name;
        gv.type       = vd.type;
        gv.has_init   = vd.init != nullptr;
        gv.is_tls     = vd.sym->is_tls;
        gv.is_static  = (vd.sym->storage == storage_class::STATIC);
        gv.at_address = vd.sym->at_address;
        gv.sfr_port   = vd.sym->sfr_port;
        if (vd.init) {
            if (auto *str = dynamic_cast<string_literal_expr*>(vd.init.get());
                str && is_char_array_type(vd.type)) {
                int n = vd.type ? vd.type->size() : 0;
                int init_bytes = static_cast<int>(str->value.size()) + 1;
                if (n > 0 && init_bytes > n)
                    init_bytes = n;
                for (int i = 0; i < init_bytes; ++i) {
                    int ch = 0;
                    if (i < static_cast<int>(str->value.size()))
                        ch = static_cast<unsigned char>(str->value[i]);
                    gv.init_vals.push_back(make_init_elem(ch, 1));
                }
                for (int i = init_bytes; i < n; ++i)
                    gv.init_vals.push_back(make_init_elem(0, 1));
            } else if (auto *il = dynamic_cast<init_list_expr*>(vd.init.get())) {
                collect_global_init(il, vd.type, *mod_, next_lbl_, gv.init_vals);
            } else if (vd.type && vd.type->is_integer()) {
                if (auto cv = const_expr_evaluator::evaluate(vd.init.get()))
                    gv.init_val = narrow_static_int(*cv, vd.type);
            } else if (auto *flit = dynamic_cast<float_literal_expr*>(vd.init.get())) {
                gv.init_val = encode_float_constant(flit->value, vd.type);
            } else if (auto *str = dynamic_cast<string_literal_expr*>(vd.init.get());
                       str && is_char_pointer_type(vd.type)) {
                gv.init_vals.push_back({0, vd.type ? vd.type->size() : 2,
                                         add_global_string_literal(*mod_, next_lbl_, *str)});
            }
        }
        mod_->globals.push_back(std::move(gv));
        return;
    }

    if (vd.init && cur_fn_) {
        if (auto *str = dynamic_cast<string_literal_expr*>(vd.init.get());
            str && is_char_array_type(vd.type)) {
            operand base = sym_to_operand(*vd.sym, vd.type->base);
            int n = vd.type->size();
            int init_bytes = static_cast<int>(str->value.size()) + 1;
            if (init_bytes > n)
                init_bytes = n;
            for (int i = 0; i < init_bytes; ++i) {
                int ch = 0;
                if (i < static_cast<int>(str->value.size()))
                    ch = static_cast<unsigned char>(str->value[i]);
                operand dst = base;
                dst.byte_offset += i;
                dst.type = vd.type->base;
                emit_assign(dst, operand::make_int(ch, vd.type->base));
            }
        } else if (auto *il = dynamic_cast<init_list_expr*>(vd.init.get())) {
            gen_init_list(*vd.sym, vd.type, *il);
        } else {
            operand dst = sym_to_operand(*vd.sym, vd.type);
            operand src = gen_expr(*vd.init);
            src = coerce_for_decl_store(src, dst.type);
            emit_assign(dst, src);
        }
    }
}

void ir_gen::visit(func_decl &fd) {
    gen_func(fd);
}

void ir_gen::gen_func(func_decl &fd) {
    if (!fd.body) return;

    ir_function fn;
    fn.name             = fd.name;
    fn.is_global        = (fd.storage != storage_class::STATIC);
    fn.ret_type         = fd.type ? fd.type->ret : type::make_void();
    fn.local_bytes      = fd.local_bytes;
    fn.orig_local_bytes = fd.local_bytes;
    fn.num_params       = static_cast<int>(fd.params.size());
    fn.abi              = fd.sym ? fd.sym->abi : call_abi::DEFAULT;
    fn.is_noreturn      = fd.sym ? fd.sym->attr_noreturn : false;

    std::vector<type_ptr> param_types;
    param_types.reserve(fd.params.size());
    for (auto &p : fd.params)
        param_types.push_back(p ? p->type : type::make_int());

    const auto &conv = get_abi_convention(fn.abi);
    auto param_locs = conv.classify_args(param_types);
    int spill_bytes = 0;
    int stack_bytes = 0;

    for (int i = 0; i < static_cast<int>(fd.params.size()); ++i) {
        auto &p = fd.params[i];
        if (!p || !p->sym) continue;

        abi_arg_loc loc = param_locs[i];
        if (loc == abi_arg_loc::STACK) {
            p->sym->stack_offset = stack_bytes;
            p->sym->is_param     = true;
            stack_bytes += conv.stack_arg_bytes(p->type, loc);
        } else {
            spill_bytes += conv.spill_bytes(p->type, loc);
            p->sym->stack_offset = -(fd.local_bytes + spill_bytes);
            p->sym->is_param     = false;
        }
    }

    fn.local_bytes += spill_bytes;
    fn.stack_param_bytes = stack_bytes;

    mod_->functions.push_back(std::move(fn));
    cur_fn_ = &mod_->functions.back();

    {
        icode ic;
        ic.op          = icode_op::FUNCTION;
        ic.func_name   = fd.name;
        ic.num_params  = static_cast<int>(fd.params.size());
        ic.local_bytes = cur_fn_->local_bytes;
        emit(ic);
    }

    for (int i = 0; i < static_cast<int>(fd.params.size()); ++i) {
        auto &p = fd.params[i];
        if (!p->sym) continue;
        operand dst = sym_to_operand(*p->sym, p->type);
        icode ic;
        ic.op     = icode_op::RECEIVE;
        ic.result = dst;
        ic.argreg = i;
        ic.arg_loc = param_locs[i];
        emit(ic);
    }

    gen_stmt(*fd.body);

    {
        icode ic;
        ic.op        = icode_op::ENDFUNCTION;
        ic.func_name = fd.name;
        emit(ic);
    }

    cur_fn_ = nullptr;
}

} // namespace xcc
