//
// irgen_expr.cpp — expression visitors for the xcc IR lowering pass.
//
// Covers: literal visitors, binary/unary/cast/call/sizeof/conditional
// expression lowering, and the arithmetic helper methods.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//
#include "ir/irgen.h"
#include "backend/z80/convention.h"

namespace xcc {

// ----- bin_op → icode_op mapping -------------------------------------

static icode_op bin_op_to_icode(bin_op op) {
    switch (op) {
    case bin_op::ADD: case bin_op::ADD_ASSIGN: return icode_op::ADD;
    case bin_op::SUB: case bin_op::SUB_ASSIGN: return icode_op::SUB;
    case bin_op::MUL: case bin_op::MUL_ASSIGN: return icode_op::MUL;
    case bin_op::DIV: case bin_op::DIV_ASSIGN: return icode_op::DIV;
    case bin_op::MOD: case bin_op::MOD_ASSIGN: return icode_op::MOD;
    case bin_op::AND: case bin_op::AND_ASSIGN: return icode_op::BAND;
    case bin_op::OR:  case bin_op::OR_ASSIGN:  return icode_op::BOR;
    case bin_op::XOR: case bin_op::XOR_ASSIGN: return icode_op::BXOR;
    case bin_op::SHL: case bin_op::SHL_ASSIGN: return icode_op::SHL;
    case bin_op::SHR: case bin_op::RHS_ASSIGN: return icode_op::SHR;
    case bin_op::EQ:  return icode_op::EQ;
    case bin_op::NE:  return icode_op::NE;
    case bin_op::LT:  return icode_op::LT;
    case bin_op::LE:  return icode_op::LE;
    case bin_op::GT:  return icode_op::GT;
    case bin_op::GE:  return icode_op::GE;
    default:          return icode_op::ADD;
    }
}

static icode_op float_op_to_icode(bin_op op) {
    switch (op) {
    case bin_op::ADD: case bin_op::ADD_ASSIGN: return icode_op::FADD;
    case bin_op::SUB: case bin_op::SUB_ASSIGN: return icode_op::FSUB;
    case bin_op::MUL: case bin_op::MUL_ASSIGN: return icode_op::FMUL;
    case bin_op::DIV: case bin_op::DIV_ASSIGN: return icode_op::FDIV;
    default:                                   return icode_op::FADD;
    }
}

// ----- Literal visitors ----------------------------------------------

void ir_gen::visit(int_literal_expr &e) {
    expr_result_ = operand::make_int(e.value, e.type ? e.type : type::make_int());
}

void ir_gen::visit(float_literal_expr &e) {
    expr_result_ = operand::make_float(e.value);
}

void ir_gen::visit(char_literal_expr &e) {
    expr_result_ = operand::make_int(e.value, type::make_int());
}

void ir_gen::visit(string_literal_expr &e) {
    std::string lbl = "__str_" + std::to_string(next_lbl_++);
    ir_module::global_var gv;
    gv.name       = lbl;
    gv.type       = type::make_array(type::make_char(), static_cast<int>(e.value.size()) + 1);
    gv.str_init   = e.value;
    gv.char_width = e.char_width;
    gv.has_init   = true;
    mod_->string_literals.push_back(std::move(gv));

    operand res = new_temp(type::make_pointer(type::make_char()));
    icode ic;
    ic.op     = icode_op::ADDRESS_OF;
    ic.result = res;
    ic.left   = operand::make_symbol(lbl, type::make_char(), true, false, 0);
    emit(ic);
    expr_result_ = res;
}

void ir_gen::visit(ident_expr &e) {
    if (!e.sym) {
        expr_result_ = operand::make_int(0, type::make_int());
        return;
    }
    if (e.sym->kind == sym_kind::ENUM_CONST) {
        expr_result_ = operand::make_int(e.sym->enum_val, type::make_int());
        return;
    }
    expr_result_ = sym_to_operand(*e.sym, e.type);
}

// ----- Arithmetic helpers --------------------------------------------

void ir_gen::gen_complex_arith(operand lhs, operand rhs, operand result, bin_op op) {
    auto float_t = type::make_float();

    operand l_re = lhs; l_re.byte_offset = 0; l_re.type = float_t;
    operand l_im = lhs; l_im.byte_offset = 4; l_im.type = float_t;
    operand r_re = rhs; r_re.byte_offset = 0; r_re.type = float_t;
    operand r_im = rhs; r_im.byte_offset = 4; r_im.type = float_t;

    auto efadd = [&](operand res, operand a, operand b) {
        icode ic; ic.op = icode_op::FADD; ic.result = res; ic.left = a; ic.right = b; emit(ic);
    };
    auto efsub = [&](operand res, operand a, operand b) {
        icode ic; ic.op = icode_op::FSUB; ic.result = res; ic.left = a; ic.right = b; emit(ic);
    };
    auto efmul = [&](operand res, operand a, operand b) {
        icode ic; ic.op = icode_op::FMUL; ic.result = res; ic.left = a; ic.right = b; emit(ic);
    };

    operand t_re = new_temp(float_t);
    operand t_im = new_temp(float_t);

    switch (op) {
    case bin_op::ADD: case bin_op::ADD_ASSIGN:
        efadd(t_re, l_re, r_re);
        efadd(t_im, l_im, r_im);
        break;
    case bin_op::SUB: case bin_op::SUB_ASSIGN:
        efsub(t_re, l_re, r_re);
        efsub(t_im, l_im, r_im);
        break;
    case bin_op::MUL: case bin_op::MUL_ASSIGN: {
        operand ac = new_temp(float_t); efmul(ac, l_re, r_re);
        operand bd = new_temp(float_t); efmul(bd, l_im, r_im);
        operand ad = new_temp(float_t); efmul(ad, l_re, r_im);
        operand bc = new_temp(float_t); efmul(bc, l_im, r_re);
        efsub(t_re, ac, bd);
        efadd(t_im, ad, bc);
        break;
    }
    default:
        break;
    }

    icode mk; mk.op = icode_op::MAKE_COMPLEX; mk.result = result; mk.left = t_re; mk.right = t_im;
    emit(mk);
}

void ir_gen::gen_binary_arith(binary_expr &e) {
    operand lhs = gen_expr(*e.left);
    operand rhs = gen_expr(*e.right);

    type_ptr expr_type = e.type;
    if (!expr_type && lhs.type && rhs.type &&
        lhs.type->is_arith() && rhs.type->is_arith())
        expr_type = usual_arith_conv(lhs.type, rhs.type);
    if (!expr_type && lhs.type)
        expr_type = lhs.type;
    if (!expr_type)
        expr_type = type::make_int();

    if (expr_type->kind == type_kind::COMPLEX) {
        operand res = new_temp(expr_type);
        gen_complex_arith(lhs, rhs, res, e.op);
        expr_result_ = res;
        return;
    }

    bool is_float = expr_type->kind == type_kind::FLOAT ||
                    expr_type->kind == type_kind::DOUBLE;
    icode_op op = is_float ? float_op_to_icode(e.op) : bin_op_to_icode(e.op);
    expr_result_ = emit_binop(op, lhs, rhs, expr_type);
}

void ir_gen::gen_binary_compare(binary_expr &e) {
    operand lhs = gen_expr(*e.left);
    operand rhs = gen_expr(*e.right);
    expr_result_ = emit_binop(bin_op_to_icode(e.op), lhs, rhs, type::make_int());
}

void ir_gen::gen_binary_logical(binary_expr &e) {
    bool is_and = (e.op == bin_op::LAND);

    operand res = new_temp(type::make_int());
    std::string short_lbl = new_label();
    std::string end_lbl   = new_label();

    operand lhs = gen_expr(*e.left);

    {
        operand cond = emit_binop(icode_op::NE, lhs,
                                  operand::make_int(0, lhs.type), type::make_int());
        icode ic; ic.op = icode_op::IFX; ic.left = cond;
        if (is_and) { ic.true_lbl = end_lbl; ic.false_lbl = short_lbl; }
        else         { ic.true_lbl = short_lbl; ic.false_lbl = end_lbl; }
        emit(ic);
    }

    std::string rhs_lbl = new_label();
    { icode lbl; lbl.op = icode_op::LABEL; lbl.label_name = end_lbl; emit(lbl); }
    operand rhs = gen_expr(*e.right);
    {
        icode ic; ic.op = icode_op::NE; ic.result = res;
        ic.left = rhs; ic.right = operand::make_int(0, rhs.type); emit(ic);
    }
    { icode jmp; jmp.op = icode_op::GOTO; jmp.label_name = rhs_lbl; emit(jmp); }

    { icode lbl; lbl.op = icode_op::LABEL; lbl.label_name = short_lbl; emit(lbl); }
    emit_assign(res, operand::make_int(is_and ? 0 : 1, type::make_int()));

    { icode lbl; lbl.op = icode_op::LABEL; lbl.label_name = rhs_lbl; emit(lbl); }

    expr_result_ = res;
}

void ir_gen::gen_assign(binary_expr &e) {
    operand rhs = gen_expr(*e.right);
    expr_result_ = gen_lvalue_write(*e.left, rhs);
}

void ir_gen::gen_compound_assign(binary_expr &e) {
    operand lhs_src = gen_expr(*e.left);
    operand rhs     = gen_expr(*e.right);
    operand tmp     = emit_binop(bin_op_to_icode(e.op), lhs_src, rhs, lhs_src.type);
    expr_result_ = gen_lvalue_write(*e.left, tmp);
}

// ----- Expression visitors -------------------------------------------

void ir_gen::visit(binary_expr &e) {
    switch (e.op) {
    case bin_op::ASSIGN:
        gen_assign(e);
        return;
    case bin_op::ADD_ASSIGN: case bin_op::SUB_ASSIGN: case bin_op::MUL_ASSIGN:
    case bin_op::DIV_ASSIGN: case bin_op::MOD_ASSIGN: case bin_op::AND_ASSIGN:
    case bin_op::OR_ASSIGN:  case bin_op::XOR_ASSIGN: case bin_op::SHL_ASSIGN:
    case bin_op::RHS_ASSIGN:
        gen_compound_assign(e);
        return;
    case bin_op::EQ: case bin_op::NE: case bin_op::LT:
    case bin_op::LE: case bin_op::GT: case bin_op::GE:
        gen_binary_compare(e);
        return;
    case bin_op::LAND: case bin_op::LOR:
        gen_binary_logical(e);
        return;
    case bin_op::COMMA:
        gen_expr(*e.left);
        gen_expr(*e.right);
        return;
    default:
        gen_binary_arith(e);
        return;
    }
}

void ir_gen::visit(unary_expr &e) {
    switch (e.op) {
    case unary_op::NEG:
        expr_result_ = emit_unop(icode_op::NEG, gen_expr(*e.operand),
                                 e.type ? e.type : type::make_int());
        break;
    case unary_op::NOT: {
        operand op = gen_expr(*e.operand);
        expr_result_ = emit_binop(icode_op::EQ, op,
                                  operand::make_int(0, op.type), type::make_int());
        break;
    }
    case unary_op::BNOT:
        expr_result_ = emit_unop(icode_op::BNOT, gen_expr(*e.operand),
                                 e.type ? e.type : type::make_int());
        break;
    case unary_op::ADDR:
        expr_result_ = emit_unop(icode_op::ADDRESS_OF, gen_expr(*e.operand),
                                 e.type ? e.type : type::make_pointer(type::make_void()));
        break;
    case unary_op::DEREF:
        expr_result_ = emit_unop(icode_op::GET_VALUE_AT, gen_expr(*e.operand),
                                 e.type ? e.type : type::make_int());
        break;
    case unary_op::PRE_INC: case unary_op::PRE_DEC: {
        bool inc = (e.op == unary_op::PRE_INC);
        operand op  = gen_expr(*e.operand);
        operand one = operand::make_int(1, op.type);
        icode ic; ic.op = inc ? icode_op::ADD : icode_op::SUB;
        ic.result = op; ic.left = op; ic.right = one; emit(ic);
        expr_result_ = op;
        break;
    }
    case unary_op::POST_INC: case unary_op::POST_DEC: {
        bool inc = (e.op == unary_op::POST_INC);
        operand op  = gen_expr(*e.operand);
        operand old = new_temp(op.type ? op.type : type::make_int());
        emit_assign(old, op);
        operand one = operand::make_int(1, op.type);
        icode ic; ic.op = inc ? icode_op::ADD : icode_op::SUB;
        ic.result = op; ic.left = op; ic.right = one; emit(ic);
        expr_result_ = old;
        break;
    }
    }
}

void ir_gen::visit(cast_expr &e) {
    expr_result_ = emit_unop(icode_op::CAST, gen_expr(*e.operand), e.target_type);
}

void ir_gen::visit(call_expr &e) {
    std::vector<operand> arg_ops;
    for (auto &a : e.args)
        arg_ops.push_back(gen_expr(*a));

    // Look up callee ABI so caller can use the right convention.
    call_abi c_abi = call_abi::DEFAULT;
    if (auto *id = dynamic_cast<ident_expr*>(e.callee.get()))
        if (id->sym && id->sym->kind == sym_kind::FUNC)
            c_abi = id->sym->abi;

    const type *fn_type = nullptr;
    if (e.callee && e.callee->type) {
        if (e.callee->type->is_func())
            fn_type = e.callee->type.get();
        else if (e.callee->type->is_ptr() && e.callee->type->base &&
                 e.callee->type->base->is_func())
            fn_type = e.callee->type->base.get();
    }

    std::vector<type_ptr> arg_types;
    arg_types.reserve(arg_ops.size());
    for (size_t i = 0; i < arg_ops.size(); ++i) {
        type_ptr abi_type = arg_ops[i].type ? arg_ops[i].type : type::make_int();
        if (fn_type && i < fn_type->params.size() && fn_type->params[i])
            abi_type = fn_type->params[i];
        arg_types.push_back(abi_type);
        if (arg_ops[i].type != abi_type) {
            bool needs_materialized_cast =
                arg_ops[i].type &&
                arg_ops[i].type->size() < abi_type->size() &&
                arg_ops[i].kind != operand_kind::INT_CONST &&
                arg_ops[i].kind != operand_kind::FLOAT_CONST;
            if (needs_materialized_cast) {
                arg_ops[i] = emit_unop(icode_op::CAST, arg_ops[i], abi_type);
            } else {
                arg_ops[i].type = abi_type;
            }
        }
    }
    const auto &conv = get_abi_convention(c_abi);
    auto arg_locs = conv.classify_args(arg_types);

    // Emit SEND icodes right-to-left (standard C push order).
    int total_arg_bytes = 0;
    for (int i = static_cast<int>(arg_ops.size()) - 1; i >= 0; --i) {
        total_arg_bytes += conv.stack_arg_bytes(arg_types[i], arg_locs[i]);
        icode ic;
        ic.op         = icode_op::SEND;
        ic.left       = arg_ops[i];
        ic.argreg     = i;
        ic.arg_loc    = arg_locs[i];
        ic.callee_abi = c_abi;
        emit(ic);
    }

    type_ptr ret_type = e.type ? e.type : type::make_int();

    operand result;
    if (ret_type->kind != type_kind::VOID)
        result = new_temp(ret_type);

    icode ic;
    ic.op         = icode_op::CALL;
    ic.num_params = static_cast<int>(e.args.size());
    ic.arg_bytes  = total_arg_bytes;
    ic.result     = result;
    ic.callee_abi = c_abi;

    if (auto *id = dynamic_cast<ident_expr*>(e.callee.get())) {
        if (id->sym && id->sym->kind == sym_kind::FUNC)
            ic.func_name = id->name;
        else
            ic.left = gen_expr(*e.callee);
    } else {
        ic.left = gen_expr(*e.callee);
    }

    emit(ic);
    expr_result_ = result;
}

void ir_gen::visit(sizeof_expr &e) {
    if (!e.is_alignof && e.sizeof_expr_op) {
        if (auto *id = dynamic_cast<ident_expr*>(e.sizeof_expr_op.get())) {
            if (id->sym && id->sym->vla_size_sym) {
                expr_result_ = sym_to_operand(*id->sym->vla_size_sym, type::make_uint());
                return;
            }
        }
    }
    int sz = 0;
    if (e.sizeof_type)
        sz = e.is_alignof ? e.sizeof_type->align() : e.sizeof_type->size();
    else if (e.sizeof_expr_op && e.sizeof_expr_op->type)
        sz = e.is_alignof ? e.sizeof_expr_op->type->align() : e.sizeof_expr_op->type->size();
    expr_result_ = operand::make_int(sz, type::make_int());
}

void ir_gen::visit(stmt_expr &e) {
    if (!e.body) {
        expr_result_ = operand::make_int(0, type::make_int());
        return;
    }
    auto *cs = dynamic_cast<compound_stmt*>(e.body.get());
    if (!cs || cs->body.empty()) {
        expr_result_ = operand::make_int(0, type::make_int());
        return;
    }
    size_t n = cs->body.size();
    auto *last_es = dynamic_cast<expr_stmt*>(cs->body[n - 1].get());
    size_t gen_count = last_es ? n - 1 : n;
    for (size_t i = 0; i < gen_count; ++i)
        if (cs->body[i]) gen_stmt(*cs->body[i]);
    if (last_es && last_es->expr)
        expr_result_ = gen_expr(*last_es->expr);
    else {
        if (cs->body[n - 1]) gen_stmt(*cs->body[n - 1]);
        expr_result_ = operand::make_int(0, e.type ? e.type : type::make_int());
    }
}

void ir_gen::visit(conditional_expr &e) {
    std::string then_lbl = new_label();
    std::string else_lbl = new_label();
    std::string end_lbl  = new_label();

    operand cond = gen_expr(*e.cond);
    operand res  = new_temp(e.type ? e.type : type::make_int());

    { icode ic; ic.op = icode_op::IFX; ic.left = cond;
      ic.true_lbl = then_lbl; ic.false_lbl = else_lbl; emit(ic); }

    { icode lbl; lbl.op = icode_op::LABEL; lbl.label_name = then_lbl; emit(lbl); }
    operand t = gen_expr(*e.then_expr);
    emit_assign(res, t);
    { icode jmp; jmp.op = icode_op::GOTO; jmp.label_name = end_lbl; emit(jmp); }

    { icode lbl; lbl.op = icode_op::LABEL; lbl.label_name = else_lbl; emit(lbl); }
    operand f = gen_expr(*e.else_expr);
    emit_assign(res, f);

    { icode lbl; lbl.op = icode_op::LABEL; lbl.label_name = end_lbl; emit(lbl); }
    expr_result_ = res;
}

} // namespace xcc
