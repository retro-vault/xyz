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

static int pointer_step(type_ptr ty) {
    if (!ty)
        return 1;
    type_ptr t = ty->unqual();
    if (!t || !t->is_ptr() || !t->base)
        return 1;
    int step = t->base->size();
    return step > 0 ? step : 1;
}

static const char *alternate_float_prefix() {
    switch (get_float_format()) {
    case float_format::IEEE16:     return "ieee16";
    case float_format::FIXED8_8:   return "fixed8_8";
    case float_format::FIXED16_16: return "fixed16_16";
    case float_format::FIXED24_8:  return "fixed24_8";
    case float_format::IEEE32:
        return "";
    }
    return "";
}

static std::string alternate_float_call_name(const std::string &name) {
    if (get_float_format() == float_format::IEEE32)
        return name;

    const char *prefix = alternate_float_prefix();
    auto prefixed = [&](const char *suffix) {
        return std::string(prefix) + suffix;
    };

    if (name == "__libc_fpclassifyf") return prefixed("_fpclassify");
    if (name == "__libc_signbitf")    return prefixed("_signbit");
    if (name == "__libc_isfinitef")   return prefixed("_isfinite");
    if (name == "__libc_isinff")      return prefixed("_isinf");
    if (name == "__libc_isnanf")      return prefixed("_isnan");
    if (name == "fabsf")              return prefixed("_abs");
    if (get_float_format() == float_format::IEEE16) {
        if (name == "sinf")               return prefixed("_sin");
        if (name == "cosf")               return prefixed("_cos");
        if (name == "tanf")               return prefixed("_tan");
        if (name == "asinf")              return prefixed("_asin");
        if (name == "acosf")              return prefixed("_acos");
        if (name == "atanf")              return prefixed("_atan");
        if (name == "atan2f")             return prefixed("_atan2");
        if (name == "sinhf")              return prefixed("_sinh");
        if (name == "coshf")              return prefixed("_cosh");
        if (name == "tanhf")              return prefixed("_tanh");
        if (name == "asinhf")             return prefixed("_asinh");
        if (name == "acoshf")             return prefixed("_acosh");
        if (name == "atanhf")             return prefixed("_atanh");
        if (name == "expf")               return prefixed("_exp");
        if (name == "exp2f")              return prefixed("_exp2");
        if (name == "expm1f")             return prefixed("_expm1");
        if (name == "logf")               return prefixed("_log");
        if (name == "log2f")              return prefixed("_log2");
        if (name == "log10f")             return prefixed("_log10");
        if (name == "log1pf")             return prefixed("_log1p");
        if (name == "powf")               return prefixed("_pow");
        if (name == "cbrtf")              return prefixed("_cbrt");
        if (name == "erff")               return prefixed("_erf");
        if (name == "erfcf")              return prefixed("_erfc");
        if (name == "tgammaf")            return prefixed("_tgamma");
        if (name == "lgammaf")            return prefixed("_lgamma");
    }
    if (name == "sqrtf")              return prefixed("_sqrt");
    if (name == "hypotf")             return prefixed("_hypot");
    if (name == "ceilf")              return prefixed("_ceil");
    if (name == "floorf")             return prefixed("_floor");
    if (name == "truncf")             return prefixed("_trunc");
    if (name == "roundf")             return prefixed("_round");
    if (name == "roundevenf")
        return get_float_format() == float_format::IEEE16 ?
               prefixed("_roundeven") : prefixed("_round");
    if (name == "nearbyintf")
        return get_float_format() == float_format::IEEE16 ?
               prefixed("_nearbyint") : prefixed("_round");
    if (name == "rintf")
        return get_float_format() == float_format::IEEE16 ?
               prefixed("_rint") : prefixed("_round");
    if (name == "lroundf")            return prefixed("_lround");
    if (name == "lrintf")
        return get_float_format() == float_format::IEEE16 ?
               prefixed("_lrint") : prefixed("_lround");
    if (name == "llroundf")           return prefixed("_llround");
    if (name == "llrintf")            return prefixed("_llrint");
    if (name == "fmaxf")              return prefixed("_fmax");
    if (name == "fminf")              return prefixed("_fmin");
    if (name == "fdimf")              return prefixed("_fdim");
    if (name == "fmodf")              return prefixed("_fmod");
    if (name == "remainderf")         return prefixed("_remainder");
    if (name == "remquof")            return prefixed("_remquo");
    if (name == "fmaf")               return prefixed("_fma");
    if (name == "frexpf")             return prefixed("_frexp");
    if (name == "ldexpf")             return prefixed("_ldexp");
    if (name == "modff")              return prefixed("_modf");
    if (name == "scalbnf")            return prefixed("_scalbn");
    if (name == "scalblnf")           return prefixed("_scalbln");
    if (name == "ilogbf")             return prefixed("_ilogb");
    if (name == "logbf")              return prefixed("_logb");
    if (name == "significandf")       return prefixed("_significand");
    if (name == "copysignf")          return prefixed("_copysign");
    if (name == "nextafterf")         return prefixed("_nextafter");
    if (name == "nextupf")            return prefixed("_nextup");
    if (name == "nextdownf")          return prefixed("_nextdown");
    if (name == "nanf")               return prefixed("_nan");
    if (name == "fromfpf")            return prefixed("_fromfp");
    if (name == "ufromfpf")           return prefixed("_ufromfp");
    if (name == "fromfpxf")           return prefixed("_fromfpx");
    if (name == "ufromfpxf")          return prefixed("_ufromfpx");
    if (name == "fmaximumf")          return prefixed("_fmaximum");
    if (name == "fminimumf")          return prefixed("_fminimum");
    if (name == "fmaximum_magf")      return prefixed("_fmaximum_mag");
    if (name == "fminimum_magf")      return prefixed("_fminimum_mag");
    if (name == "fmaximum_numf")      return prefixed("_fmaximum_num");
    if (name == "fminimum_numf")      return prefixed("_fminimum_num");
    if (name == "fmaximum_mag_numf")  return prefixed("_fmaximum_mag_num");
    if (name == "fminimum_mag_numf")  return prefixed("_fminimum_mag_num");
    if (name == "getpayloadf")        return prefixed("_getpayload");
    if (name == "setpayloadf")        return prefixed("_setpayload");
    if (name == "setpayloadsigf")     return prefixed("_setpayloadsig");
    if (name == "totalorderf")        return prefixed("_totalorder");
    if (name == "totalordermagf")     return prefixed("_totalordermag");

    return name;
}

static type_ptr default_arg_promote_type(type_ptr t) {
    if (!t)
        return type::make_int();

    t = t->unqual();
    if (!t)
        return type::make_int();

    if (t->kind == type_kind::FLOAT)
        return type::make_double();

    return integer_promote(t);
}

// ----- Literal visitors ----------------------------------------------

void ir_gen::visit(int_literal_expr &e) {
    expr_result_ = operand::make_int(e.value, e.type ? e.type : type::make_int());
}

void ir_gen::visit(float_literal_expr &e) {
    expr_result_ = operand::make_float(e.value,
                                       e.type ? e.type : type::make_double());
}

void ir_gen::visit(char_literal_expr &e) {
    expr_result_ = operand::make_int(e.value, type::make_int());
}

void ir_gen::visit(string_literal_expr &e) {
    std::string lbl = "__xcc_str_" + std::to_string(next_lbl_++);
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
        expr_result_ = operand::make_int(
            e.sym->enum_val, e.sym->type ? e.sym->type : type::make_int());
        return;
    }
    operand sym_op = sym_to_operand(*e.sym, e.type);
    if (e.type && e.type->is_array() && e.type->base) {
        if (e.sym->is_dynamic_aligned) {
            expr_result_ = sym_to_operand(
                *e.sym, type::make_pointer(e.type->base));
            return;
        }
        expr_result_ = emit_unop(icode_op::ADDRESS_OF, sym_op,
                                 type::make_pointer(e.type->base));
        return;
    }
    expr_result_ = sym_op;
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

    auto scale_index = [&](operand index, int scale) -> operand {
        if (scale <= 1)
            return index;
        type_ptr idx_type = index.type ? index.type : type::make_int();
        if (index.kind == operand_kind::INT_CONST) {
            index.ival *= scale;
            index.type = idx_type;
            return index;
        }
        return emit_binop(icode_op::MUL, index,
                          operand::make_int(scale, idx_type), idx_type);
    };

    if (e.op == bin_op::ADD) {
        if (lhs.type && lhs.type->is_ptr() && rhs.type && rhs.type->is_integer()) {
            rhs = scale_index(rhs, pointer_step(lhs.type));
            expr_result_ = emit_binop(icode_op::ADD, lhs, rhs, lhs.type);
            return;
        }
        if (lhs.type && lhs.type->is_integer() && rhs.type && rhs.type->is_ptr()) {
            lhs = scale_index(lhs, pointer_step(rhs.type));
            expr_result_ = emit_binop(icode_op::ADD, rhs, lhs, rhs.type);
            return;
        }
    } else if (e.op == bin_op::SUB) {
        if (lhs.type && lhs.type->is_ptr() && rhs.type && rhs.type->is_integer()) {
            rhs = scale_index(rhs, pointer_step(lhs.type));
            expr_result_ = emit_binop(icode_op::SUB, lhs, rhs, lhs.type);
            return;
        }
        if (lhs.type && lhs.type->is_ptr() && rhs.type && rhs.type->is_ptr()) {
            operand raw = emit_binop(icode_op::SUB, lhs, rhs, type::make_int());
            int scale = pointer_step(lhs.type);
            if (scale > 1)
                raw = emit_binop(icode_op::DIV, raw,
                                 operand::make_int(scale, raw.type), raw.type);
            expr_result_ = raw;
            return;
        }
    }

    type_ptr expr_type = e.type;
    if (!expr_type && lhs.type && rhs.type &&
        lhs.type->is_arith() && rhs.type->is_arith())
        expr_type = usual_arith_conv(lhs.type, rhs.type);
    if (!expr_type && lhs.type)
        expr_type = lhs.type;
    if (!expr_type)
        expr_type = type::make_int();

    auto coerce_operand = [&](operand op, const type_ptr &target) -> operand {
        if (!target)
            return op;
        if (!op.type) {
            op.type = target;
            return op;
        }
        if (target->kind == type_kind::COMPLEX &&
            op.type->kind != type_kind::COMPLEX) {
            return emit_unop(icode_op::CAST, op, target);
        }
        bool same_type =
            op.type->kind == target->kind &&
            op.type->size() == target->size() &&
            op.type->is_unsigned() == target->is_unsigned();
        if (same_type) {
            op.type = target;
            return op;
        }
        op = coerce_const_operand(op, target);
        if (op.kind == operand_kind::INT_CONST ||
            op.kind == operand_kind::FLOAT_CONST) {
            op.type = target;
            return op;
        }
        return emit_unop(icode_op::CAST, op, target);
    };

    if (expr_type->kind == type_kind::COMPLEX) {
        lhs = coerce_operand(lhs, expr_type);
        rhs = coerce_operand(rhs, expr_type);
        operand res = new_temp(expr_type);
        gen_complex_arith(lhs, rhs, res, e.op);
        expr_result_ = res;
        return;
    }

    bool is_float = expr_type->kind == type_kind::FLOAT ||
                    expr_type->kind == type_kind::DOUBLE;
    if (lhs.type && rhs.type && lhs.type->is_arith() && rhs.type->is_arith()) {
        lhs = coerce_operand(lhs, expr_type);
        rhs = coerce_operand(rhs, expr_type);
    }
    icode_op op = is_float ? float_op_to_icode(e.op) : bin_op_to_icode(e.op);
    expr_result_ = emit_binop(op, lhs, rhs, expr_type);
}

void ir_gen::gen_binary_compare(binary_expr &e) {
    operand lhs = gen_expr(*e.left);
    operand rhs = gen_expr(*e.right);
    auto coerce_operand = [&](operand op, const type_ptr &target) -> operand {
        if (!target)
            return op;
        if (!op.type) {
            op.type = target;
            return op;
        }
        bool same_type =
            op.type->kind == target->kind &&
            op.type->size() == target->size() &&
            op.type->is_unsigned() == target->is_unsigned();
        if (same_type) {
            op.type = target;
            return op;
        }
        op = coerce_const_operand(op, target);
        if (op.kind == operand_kind::INT_CONST ||
            op.kind == operand_kind::FLOAT_CONST) {
            op.type = target;
            return op;
        }
        return emit_unop(icode_op::CAST, op, target);
    };
    if (lhs.type && rhs.type && lhs.type->is_arith() && rhs.type->is_arith()) {
        type_ptr cmp_type = usual_arith_conv(lhs.type, rhs.type);
        lhs = coerce_operand(lhs, cmp_type);
        rhs = coerce_operand(rhs, cmp_type);
    }
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
    operand lhs_for_op = lhs_src;
    operand rhs_for_op = rhs;
    type_ptr op_type = lhs_src.type ? lhs_src.type : type::make_int();

    auto coerce_operand = [&](operand op, const type_ptr &target) -> operand {
        if (!target)
            return op;
        if (!op.type) {
            op.type = target;
            return op;
        }
        bool same_type =
            op.type->kind == target->kind &&
            op.type->size() == target->size() &&
            op.type->is_unsigned() == target->is_unsigned();
        if (same_type) {
            op.type = target;
            return op;
        }
        op = coerce_const_operand(op, target);
        if (op.kind == operand_kind::INT_CONST ||
            op.kind == operand_kind::FLOAT_CONST) {
            op.type = target;
            return op;
        }
        return emit_unop(icode_op::CAST, op, target);
    };

    if (lhs_src.type && rhs.type &&
        lhs_src.type->is_arith() && rhs.type->is_arith()) {
        op_type = usual_arith_conv(lhs_src.type, rhs.type);
        lhs_for_op = coerce_operand(lhs_src, op_type);
        rhs_for_op = coerce_operand(rhs, op_type);
    }

    bool is_float = op_type &&
                    (op_type->kind == type_kind::FLOAT ||
                     op_type->kind == type_kind::DOUBLE);
    operand tmp = emit_binop(is_float ? float_op_to_icode(e.op)
                                      : bin_op_to_icode(e.op),
                             lhs_for_op, rhs_for_op, op_type);
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
        expr_result_ = gen_lvalue_addr(*e.operand,
                                       e.type ? e.type
                                              : type::make_pointer(type::make_void()));
        break;
    case unary_op::DEREF: {
        operand ptr = gen_expr(*e.operand);
        if (e.type && e.type->is_array() && e.type->base) {
            // Dereferencing a pointer-to-array yields an array lvalue, which
            // immediately decays to an element pointer in value context.
            ptr.type = (ptr.type && ptr.type->is_far_ptr())
                           ? type::make_far_pointer(e.type->base)
                           : type::make_pointer(e.type->base);
            expr_result_ = ptr;
            break;
        }
        expr_result_ = emit_unop(icode_op::GET_VALUE_AT, ptr,
                                 e.type ? e.type : type::make_int());
        break;
    }
    case unary_op::PRE_INC: case unary_op::PRE_DEC: {
        bool inc = (e.op == unary_op::PRE_INC);
        operand op = gen_expr(*e.operand);
        type_ptr op_type = op.type ? op.type : type::make_int();
        bool is_float = op_type &&
                        (op_type->kind == type_kind::FLOAT ||
                         op_type->kind == type_kind::DOUBLE);
        int step_value = 1;
        if (op.type && op.type->is_ptr() && op.type->base)
            step_value = op.type->base->size();
        operand step = is_float
                           ? operand::make_float(static_cast<double>(step_value), op_type)
                           : operand::make_int(step_value, type::make_int());
        operand tmp = emit_binop(is_float
                                     ? (inc ? icode_op::FADD : icode_op::FSUB)
                                     : (inc ? icode_op::ADD : icode_op::SUB),
                                 op, step, op_type);
        expr_result_ = gen_lvalue_write(*e.operand, tmp);
        break;
    }
    case unary_op::POST_INC: case unary_op::POST_DEC: {
        bool inc = (e.op == unary_op::POST_INC);
        operand op = gen_expr(*e.operand);
        type_ptr op_type = op.type ? op.type : type::make_int();
        bool is_float = op_type &&
                        (op_type->kind == type_kind::FLOAT ||
                         op_type->kind == type_kind::DOUBLE);
        operand old = new_temp(op_type);
        emit_assign(old, op);
        int step_value = 1;
        if (op.type && op.type->is_ptr() && op.type->base)
            step_value = op.type->base->size();
        operand step = is_float
                           ? operand::make_float(static_cast<double>(step_value), op_type)
                           : operand::make_int(step_value, type::make_int());
        operand tmp = emit_binop(is_float
                                     ? (inc ? icode_op::FADD : icode_op::FSUB)
                                     : (inc ? icode_op::ADD : icode_op::SUB),
                                 op, step, op_type);
        gen_lvalue_write(*e.operand, tmp);
        expr_result_ = old;
        break;
    }
    }
}

void ir_gen::visit(cast_expr &e) {
    expr_result_ = emit_unop(icode_op::CAST, gen_expr(*e.operand), e.target_type);
}

static bool uses_small_printf_format(const call_expr &e) {
    if (e.args.empty())
        return false;

    const auto *format = dynamic_cast<const string_literal_expr*>(e.args[0].get());
    if (!format || format->char_width != 1)
        return false;

    for (size_t i = 0; i < format->value.size(); ++i) {
        const unsigned char ch =
            static_cast<unsigned char>(format->value[i]);
        if (ch == '\0')
            break;
        if (ch != '%')
            continue;
        if (++i == format->value.size())
            return false;

        const char conversion = format->value[i];
        if (conversion != '%' && conversion != 'd' &&
            conversion != 'i' && conversion != 's') {
            return false;
        }
    }
    return true;
}

void ir_gen::visit(call_expr &e) {
    std::string direct_func_name;
    bool direct_callee_noreturn = false;
    operand indirect_callee;
    if (auto *id = dynamic_cast<ident_expr*>(e.callee.get())) {
        if (id->sym && id->sym->kind == sym_kind::FUNC) {
            direct_func_name = alternate_float_call_name(id->name);
            if (direct_func_name == "printf" &&
                id->sym->type && id->sym->type->is_func() &&
                id->sym->type->variadic &&
                uses_small_printf_format(e)) {
                direct_func_name = "__printf_sd";
            }
            direct_callee_noreturn = id->sym->attr_noreturn;
        }
    }
    if (direct_func_name.empty() && e.callee) {
        // Evaluate an indirect callee before SENDs.  On register ABIs the SEND
        // sequence materializes arguments in HL/DE/A, and evaluating a member
        // function pointer afterwards would clobber those registers.
        indirect_callee = gen_expr(*e.callee);
    }

    std::vector<operand> arg_ops;
    for (auto &a : e.args)
        arg_ops.push_back(gen_expr(*a));

    // Look up callee ABI so caller can use the right convention.
    // Variadic function types must stay stack-only even when there is
    // no direct named callee symbol (for example, through a function
    // pointer), so the type-level variadic bit also participates here.
    call_abi c_abi = call_abi::DEFAULT;
    const type *fn_type = nullptr;
    if (e.callee && e.callee->type) {
        if (e.callee->type->is_func())
            fn_type = e.callee->type.get();
        else if (e.callee->type->is_ptr() && e.callee->type->base &&
                 e.callee->type->base->is_func())
            fn_type = e.callee->type->base.get();
    }

    if (fn_type && fn_type->func_abi != call_abi::DEFAULT)
        c_abi = fn_type->func_abi;

    if (auto *id = dynamic_cast<ident_expr*>(e.callee.get())) {
        if (id->sym && id->sym->kind == sym_kind::FUNC &&
            (id->sym->abi != call_abi::DEFAULT || c_abi == call_abi::DEFAULT)) {
            c_abi = id->sym->abi;
        }
    }

    const bool callee_variadic = fn_type && fn_type->variadic;

    if (callee_variadic && c_abi == call_abi::DEFAULT)
        c_abi = call_abi::SDCCCALL0;

    std::vector<type_ptr> arg_types;
    arg_types.reserve(arg_ops.size());
    for (size_t i = 0; i < arg_ops.size(); ++i) {
        type_ptr abi_type = arg_ops[i].type ? arg_ops[i].type : type::make_int();
        const bool is_variadic_tail =
            callee_variadic && i >= fn_type->params.size();
        if (is_variadic_tail) {
            abi_type = default_arg_promote_type(abi_type);
        } else if (fn_type && i < fn_type->params.size() && fn_type->params[i]) {
            abi_type = fn_type->params[i];
        }
        arg_ops[i] = coerce_const_operand(arg_ops[i], abi_type);
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

    type_ptr ret_type = e.type ? e.type : type::make_int();
    const bool result_via_sret =
        abi_returns_aggregate_via_hidden_pointer(c_abi, ret_type);

    operand result;
    if (ret_type->kind != type_kind::VOID)
        result = new_temp(ret_type);

    operand sret_address;
    if (result_via_sret) {
        sret_address = emit_unop(
            icode_op::ADDRESS_OF, result, type::make_pointer(ret_type));
    }

    // Emit SEND icodes in ABI order so the caller stack matches the callee's
    // expected parameter layout.
    int total_arg_bytes = 0;
    if (conv.caller_sends_right_to_left()) {
        for (int i = static_cast<int>(arg_ops.size()) - 1; i >= 0; --i) {
            const int stack_bytes =
                conv.stack_arg_bytes(arg_types[i], arg_locs[i]);
            total_arg_bytes += stack_bytes;
            icode ic;
            ic.op         = icode_op::SEND;
            ic.left       = arg_ops[i];
            ic.argreg     = i;
            ic.arg_loc    = arg_locs[i];
            ic.send_bytes = stack_bytes;
            ic.callee_abi = c_abi;
            emit(ic);
        }
    } else {
        for (int i = 0; i < static_cast<int>(arg_ops.size()); ++i) {
            const int stack_bytes =
                conv.stack_arg_bytes(arg_types[i], arg_locs[i]);
            total_arg_bytes += stack_bytes;
            icode ic;
            ic.op         = icode_op::SEND;
            ic.left       = arg_ops[i];
            ic.argreg     = i;
            ic.arg_loc    = arg_locs[i];
            ic.send_bytes = stack_bytes;
            ic.callee_abi = c_abi;
            emit(ic);
        }
    }

    // SDCC passes a pointer to caller-owned aggregate result storage in the
    // stack slot nearest the return address. It is outside source parameter
    // numbering and is pushed after every ordinary argument under both ABI 0
    // and ABI 1.
    if (result_via_sret) {
        icode hidden;
        hidden.op = icode_op::SEND;
        hidden.left = sret_address;
        hidden.argreg = -1;
        hidden.arg_loc = abi_arg_loc::STACK;
        hidden.send_bytes = 2;
        hidden.callee_abi = c_abi;
        emit(hidden);
        total_arg_bytes += 2;

        // Materializing and pushing the hidden pointer uses HL and can also
        // disturb other argument registers. Reload ABI 1 register arguments
        // after the push; stack arguments stay below the hidden slot.
        if (effective_call_abi(c_abi) == call_abi::SDCCCALL1) {
            auto resend_register_arg = [&](int i) {
                if (arg_locs[i] == abi_arg_loc::STACK)
                    return;
                icode resend;
                resend.op = icode_op::SEND;
                resend.left = arg_ops[i];
                resend.argreg = i;
                resend.arg_loc = arg_locs[i];
                resend.send_bytes = 0;
                resend.callee_abi = c_abi;
                emit(resend);
            };
            if (conv.caller_sends_right_to_left()) {
                for (int i = static_cast<int>(arg_ops.size()) - 1; i >= 0; --i)
                    resend_register_arg(i);
            } else {
                for (int i = 0; i < static_cast<int>(arg_ops.size()); ++i)
                    resend_register_arg(i);
            }
        }
    }

    icode ic;
    ic.op         = icode_op::CALL;
    ic.num_params = static_cast<int>(e.args.size());
    ic.arg_bytes  = total_arg_bytes;
    ic.result     = result;
    ic.callee_abi = c_abi;
    ic.callee_cleans_stack =
        abi_callee_cleans_stack(c_abi, ret_type, arg_types, callee_variadic);
    ic.callee_noreturn = direct_callee_noreturn;
    ic.result_via_sret = result_via_sret;

    if (!direct_func_name.empty())
        ic.func_name = direct_func_name;
    else
        ic.left = indirect_callee;

    emit(ic);
    expr_result_ = result;
}

void ir_gen::visit(sizeof_expr &e) {
    if (e.is_countof) {
        if (e.sizeof_expr_op) {
            if (auto *id = dynamic_cast<ident_expr*>(e.sizeof_expr_op.get())) {
                if (id->sym && id->sym->vla_size_sym) {
                    operand bytes = sym_to_operand(*id->sym->vla_size_sym, type::make_uint());
                    int elem_sz = (id->sym->type && id->sym->type->is_ptr() && id->sym->type->base)
                        ? id->sym->type->base->size()
                        : 0;
                    if (elem_sz > 1) {
                        operand result = new_temp(type::make_uint());
                        icode ic;
                        ic.op     = icode_op::DIV;
                        ic.result = result;
                        ic.left   = bytes;
                        ic.right  = operand::make_int(elem_sz, type::make_uint());
                        emit(ic);
                        expr_result_ = result;
                    } else {
                        expr_result_ = bytes;
                    }
                    return;
                }
            }
            if (e.sizeof_expr_op->type && e.sizeof_expr_op->type->kind == type_kind::ARRAY) {
                expr_result_ = operand::make_int(
                    e.sizeof_expr_op->type->array_size,
                    type::make_int());
                return;
            }
        }
        if (e.sizeof_type && e.sizeof_type->kind == type_kind::ARRAY) {
            expr_result_ = operand::make_int(e.sizeof_type->array_size, type::make_int());
            return;
        }
        expr_result_ = operand::make_int(0, type::make_int());
        return;
    }
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
        sz = e.is_alignof ? (e.align_override > 0 ? e.align_override
                                                  : e.sizeof_expr_op->type->align())
                          : e.sizeof_expr_op->type->size();
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
