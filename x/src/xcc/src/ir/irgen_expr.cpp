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
#include <cstdint>

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

    // Compound pointer addition/subtraction has the same element-based
    // semantics as ordinary pointer arithmetic.  The generic arithmetic
    // lowering below deliberately leaves pointer operands out of the usual
    // arithmetic conversions, so scale the integer operand explicitly
    // before emitting the byte-address ADD/SUB used by the backend.
    if ((e.op == bin_op::ADD_ASSIGN || e.op == bin_op::SUB_ASSIGN) &&
        lhs_src.type && lhs_src.type->is_ptr() &&
        rhs.type && rhs.type->is_integer()) {
        const int scale = pointer_step(lhs_src.type);
        if (scale > 1) {
            if (rhs.kind == operand_kind::INT_CONST) {
                rhs_for_op = rhs;
                rhs_for_op.ival *= scale;
            } else {
                type_ptr index_type = rhs.type ? rhs.type : type::make_int();
                rhs_for_op = emit_binop(
                    icode_op::MUL, rhs,
                    operand::make_int(scale, index_type), index_type);
            }
        }
    }

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

namespace {

enum class format_family {
    NONE,
    PRINTF,
    SCANF,
};

struct format_call_desc {
    const char *name;
    format_family family;
    size_t format_arg;
    bool variadic;
};

constexpr format_call_desc format_calls[] = {
    {"printf",    format_family::PRINTF, 0, true},
    {"printk",    format_family::PRINTF, 0, true},
    {"fprintf",   format_family::PRINTF, 1, true},
    {"sprintf",   format_family::PRINTF, 1, true},
    {"snprintf",  format_family::PRINTF, 2, true},
    {"vprintf",   format_family::PRINTF, 0, false},
    {"vfprintf",  format_family::PRINTF, 1, false},
    {"vsprintf",  format_family::PRINTF, 1, false},
    {"vsnprintf", format_family::PRINTF, 2, false},
    {"scanf",     format_family::SCANF,  0, true},
    {"fscanf",    format_family::SCANF,  1, true},
    {"sscanf",    format_family::SCANF,  1, true},
    {"vscanf",    format_family::SCANF,  0, false},
    {"vfscanf",   format_family::SCANF,  1, false},
    {"vsscanf",   format_family::SCANF,  1, false},
};

const format_call_desc *find_format_call(const std::string &name) {
    for (const auto &desc : format_calls) {
        if (name == desc.name)
            return &desc;
    }
    return nullptr;
}

const string_literal_expr *unwrap_format_literal(const expr *value) {
    while (const auto *cast = dynamic_cast<const cast_expr *>(value))
        value = cast->operand.get();
    return dynamic_cast<const string_literal_expr *>(value);
}

struct format_conversion {
    char conversion;
    uint32_t ordinary;
    uint32_t long_value;
    uint32_t long_long_value;
    bool scanf_only;
};

constexpr format_conversion format_conversions[] = {
    {'d', 0x00000001u, 0x00001000u, 0x00000001u, false},
    {'u', 0x00000002u, 0x00002000u, 0x00000002u, false},
    {'x', 0x00000004u, 0x00004000u, 0x00000004u, false},
    {'X', 0x00000008u, 0x00008000u, 0x00000008u, false},
    {'o', 0x00000010u, 0x00010000u, 0x00000010u, false},
    {'n', 0x00000020u, 0x00020000u, 0x00000000u, false},
    {'i', 0x00000040u, 0x00040000u, 0x00000040u, false},
    {'p', 0x00000080u, 0x00080000u, 0x00000000u, false},
    {'B', 0x00000100u, 0x00100000u, 0x00000000u, false},
    {'s', 0x00000200u, 0x00000000u, 0x00000000u, false},
    {'S', 0x02000200u, 0x00000000u, 0x00000000u, false},
    {'c', 0x00000400u, 0x00000000u, 0x00000000u, false},
    {'I', 0x00000800u, 0x00000000u, 0x00000000u, false},
    {'[', 0x00200000u, 0x00200000u, 0x00000000u, true},
    {'a', 0x00400000u, 0x00400000u, 0x00000000u, false},
    {'A', 0x00800000u, 0x00800000u, 0x00000000u, false},
    {'e', 0x01000000u, 0x01000000u, 0x00000000u, false},
    {'E', 0x02000000u, 0x02000000u, 0x00000000u, false},
    {'f', 0x04000000u, 0x04000000u, 0x00000000u, false},
    {'F', 0x08000000u, 0x08000000u, 0x00000000u, false},
    {'g', 0x10000000u, 0x10000000u, 0x00000000u, false},
    {'G', 0x20000000u, 0x20000000u, 0x00000000u, false},
};

bool ascii_digit(char ch) {
    return ch >= '0' && ch <= '9';
}

const format_conversion *find_format_conversion(char conversion,
                                                 bool scanf_family) {
    for (const auto &entry : format_conversions) {
        if (entry.conversion == conversion &&
            (!entry.scanf_only || scanf_family)) {
            return &entry;
        }
    }
    return nullptr;
}

// Parse a constant format into z88dk classic's converter capability mask.
// Returning false means that narrowing would be unsafe, so the caller must
// request the complete formatter.  This is deliberately more conservative
// than z88dk's current watcher for dynamic and unsupported formats.
bool parse_format_capabilities(const string_literal_expr &literal,
                               bool scanf_family,
                               uint32_t &mask, uint32_t &mask2) {
    if (literal.char_width != 1)
        return false;

    const std::string &format = literal.value;
    for (size_t i = 0; i < format.size(); ++i) {
        if (format[i] == '\0')
            break;
        if (format[i] != '%')
            continue;
        if (++i >= format.size())
            return false;
        if (format[i] == '%')
            continue;

        bool needs_flag_parser = false;
        if (scanf_family) {
            if (format[i] == '*') {
                needs_flag_parser = true;
                ++i;
            }
            while (i < format.size() && ascii_digit(format[i])) {
                needs_flag_parser = true;
                ++i;
            }
        } else {
            while (i < format.size() &&
                   (format[i] == '-' || format[i] == '+' ||
                    format[i] == ' ' || format[i] == '#' ||
                    format[i] == '0' || format[i] == '\'')) {
                needs_flag_parser = true;
                ++i;
            }
            if (i < format.size() && format[i] == '*') {
                needs_flag_parser = true;
                ++i;
                // Positional arguments are not supported by the classic
                // capability protocol.
                if (i < format.size() && ascii_digit(format[i]))
                    return false;
            } else {
                bool had_width = false;
                while (i < format.size() && ascii_digit(format[i])) {
                    had_width = true;
                    ++i;
                }
                if (i < format.size() && format[i] == '$')
                    return false;
                needs_flag_parser = needs_flag_parser || had_width;
            }
            if (i < format.size() && format[i] == '.') {
                needs_flag_parser = true;
                ++i;
                if (i < format.size() && format[i] == '*') {
                    ++i;
                    if (i < format.size() && ascii_digit(format[i]))
                        return false;
                } else {
                    while (i < format.size() && ascii_digit(format[i]))
                        ++i;
                }
            }
        }

        if (i >= format.size())
            return false;

        enum class length_kind { ORDINARY, LONG, LONG_LONG, UNSUPPORTED };
        length_kind length = length_kind::ORDINARY;
        if (format[i] == 'h') {
            ++i;
            if (i < format.size() && format[i] == 'h')
                ++i;
        } else if (format[i] == 'l') {
            ++i;
            if (i < format.size() && format[i] == 'l') {
                ++i;
                length = length_kind::LONG_LONG;
            } else {
                length = length_kind::LONG;
            }
        } else if (format[i] == 'j') {
            ++i;
            length = length_kind::UNSUPPORTED;
        } else if (format[i] == 'z' || format[i] == 't') {
            ++i;
        } else if (format[i] == 'L') {
            ++i;
        }
        if (i >= format.size() || length == length_kind::UNSUPPORTED ||
            (scanf_family && length == length_kind::LONG_LONG))
            return false;

        const char conversion = format[i];
        const auto *entry = find_format_conversion(conversion, scanf_family);
        if (!entry)
            return false;

        const uint32_t bit = length == length_kind::LONG
                                 ? entry->long_value
                             : length == length_kind::LONG_LONG
                                 ? entry->long_long_value
                                 : entry->ordinary;
        // A zero long mapping means the runtime does not implement this
        // length/conversion pair through its selectable table.
        if (bit == 0)
            return false;
        if (length == length_kind::LONG_LONG)
            mask2 |= bit;
        else
            mask |= bit;
        if (needs_flag_parser)
            mask |= 0x40000000u;

        if (scanf_family && conversion == '[') {
            size_t scan = i + 1;
            if (scan < format.size() && format[scan] == '^')
                ++scan;
            if (scan < format.size() && format[scan] == ']')
                ++scan;
            while (scan < format.size() && format[scan] != ']')
                ++scan;
            if (scan >= format.size())
                return false;
            i = scan;
        }
    }
    return true;
}

bool uses_small_printf_format(const call_expr &e) {
    if (e.args.empty())
        return false;

    const auto *format = unwrap_format_literal(e.args[0].get());
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

} // namespace

void ir_gen::visit(call_expr &e) {
    std::string direct_func_name;
    bool direct_callee_noreturn = false;
    operand indirect_callee;
    if (auto *id = dynamic_cast<ident_expr*>(e.callee.get())) {
        if (id->sym && id->sym->kind == sym_kind::FUNC) {
            direct_func_name = alternate_float_call_name(
                sym_to_operand(*id->sym, id->type).name);
            const format_call_desc *format_desc = find_format_call(id->name);
            const bool is_external_format_function =
                format_desc && id->sym->type && id->sym->type->is_func() &&
                id->sym->type->variadic == format_desc->variadic &&
                !defined_function_names_.count(id->name);
            if (is_external_format_function) {
                auto &usage = format_desc->family == format_family::PRINTF
                                  ? mod_->printf_formats
                                  : mod_->scanf_formats;
                usage.used = true;
                if (format_desc->format_arg >= e.args.size()) {
                    usage.requires_full = true;
                } else {
                    const auto *literal = unwrap_format_literal(
                        e.args[format_desc->format_arg].get());
                    uint32_t call_mask = 0;
                    uint32_t call_mask2 = 0;
                    if (!literal || !parse_format_capabilities(
                                        *literal,
                                        format_desc->family == format_family::SCANF,
                                        call_mask, call_mask2)) {
                        usage.requires_full = true;
                    } else {
                        usage.mask |= call_mask;
                        usage.mask2 |= call_mask2;
                    }
                }
            }
            if (direct_func_name == "printf" &&
                native_printf_specialization_ &&
                !defined_function_names_.count(id->name) &&
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
        //
        // C permits an explicit function-designator dereference in a call:
        //     (*function_pointer)(arguments)
        // The dereference yields a function designator, not an object value
        // loaded from the first bytes of the function's machine code.  Lower
        // the pointer operand directly in this context.  Ordinary object
        // dereferences still go through unary_expr's GET_VALUE_AT lowering.
        if (auto *deref = dynamic_cast<unary_expr *>(e.callee.get());
            deref && deref->op == unary_op::DEREF && deref->operand &&
            e.callee->type && e.callee->type->is_func()) {
            indirect_callee = gen_expr(*deref->operand);
        } else {
            indirect_callee = gen_expr(*e.callee);
        }
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
        if (!arg_ops[i].type) {
            arg_ops[i].type = abi_type;
        } else {
            // Function designators (and, defensively, arrays) decay to a
            // pointer with an identical bit representation -- the function's
            // entry address literally is the pointer value. Treat these as
            // interchangeable with POINTER here so that decay alone doesn't
            // trigger a materialized CAST below: a spurious CAST severs the
            // direct-symbol operand that later passes (e.g. recursive
            // function-pointer devirtualization) rely on seeing.
            auto is_ptr_like = [](type_kind k) {
                return k == type_kind::POINTER || k == type_kind::FUNCTION ||
                       k == type_kind::ARRAY;
            };
            const bool kinds_compatible =
                arg_ops[i].type->kind == abi_type->kind ||
                (is_ptr_like(arg_ops[i].type->kind) &&
                 is_ptr_like(abi_type->kind));
            const bool same_type =
                kinds_compatible &&
                arg_ops[i].type->size() == abi_type->size() &&
                arg_ops[i].type->is_unsigned() == abi_type->is_unsigned();
            if (same_type) {
                arg_ops[i].type = abi_type;
            } else {
                arg_ops[i] = coerce_const_operand(arg_ops[i], abi_type);
                if (arg_ops[i].kind != operand_kind::INT_CONST &&
                    arg_ops[i].kind != operand_kind::FLOAT_CONST) {
                    arg_ops[i] = emit_unop(
                        icode_op::CAST, arg_ops[i], abi_type);
                }
            }
        }
        arg_types.push_back(abi_type);
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
