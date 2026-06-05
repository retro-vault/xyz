//
// icode.cpp — IR instruction utilities: opcode name table and dump helpers.
//
// icode_op_name() provides a printable name for each opcode, used when
// dumping the IR for debugging (--dump-ir flag).  operand::to_string(),
// icode::dump(), ir_function::dump(), and ir_module::dump() print the
// full IR in a human-readable form resembling three-address pseudocode.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//
#include "ir/icode.h"
#include <cstdio>
#include <sstream>

namespace xcc {

static const char *abi_arg_loc_name(abi_arg_loc loc) {
    switch (loc) {
    case abi_arg_loc::STACK:   return "stack";
    case abi_arg_loc::REG_A:   return "a";
    case abi_arg_loc::REG_L:   return "l";
    case abi_arg_loc::REG_HL:  return "hl";
    case abi_arg_loc::REG_DE:  return "de";
    case abi_arg_loc::REG_DEHL:return "dehl";
    }
    return "?";
}

const char *icode_op_name(icode_op op) {
    switch (op) {
#define CASE(k) case icode_op::k: return #k
    CASE(LABEL); CASE(GOTO); CASE(IFX);
    CASE(FUNCTION); CASE(ENDFUNCTION); CASE(RETURN);
    CASE(SEND); CASE(RECEIVE); CASE(CALL);
    CASE(ASSIGN); CASE(ADDRESS_OF); CASE(GET_VALUE_AT); CASE(SET_VALUE_AT);
    CASE(ADD); CASE(SUB); CASE(MUL); CASE(DIV); CASE(MOD); CASE(NEG);
    CASE(BAND); CASE(BOR); CASE(BXOR); CASE(BNOT); CASE(SHL); CASE(SHR);
    CASE(EQ); CASE(NE); CASE(LT); CASE(LE); CASE(GT); CASE(GE);
    CASE(CAST);
    CASE(FADD); CASE(FSUB); CASE(FMUL); CASE(FDIV);
    CASE(FITOSF); CASE(FSTOI);
    CASE(ALLOCA);
    CASE(INLINE_ASM);
    CASE(MAKE_COMPLEX);
#undef CASE
    default: return "?";
    }
}

std::string operand::to_string() const {
    switch (kind) {
    case operand_kind::NONE:       return "_";
    case operand_kind::TEMP:       return "t" + std::to_string(temp_id);
    case operand_kind::SYMBOL:
        if (is_global) return name;
        if (is_param)  return name + "(ix+" + std::to_string(stack_offset + 4) + ")";
        return name + "(ix" + std::to_string(stack_offset) + ")";
    case operand_kind::INT_CONST:  return "#" + std::to_string(ival);
    case operand_kind::FLOAT_CONST:return "#" + std::to_string(fval);
    case operand_kind::LABEL_REF:  return name;
    }
    return "?";
}

void icode::dump() const {
    switch (op) {
    case icode_op::LABEL:
        printf("%s:\n", label_name.c_str());
        break;
    case icode_op::GOTO:
        printf("  goto %s\n", label_name.c_str());
        break;
    case icode_op::IFX:
        printf("  if %s goto %s else %s\n",
               left.to_string().c_str(),
               true_lbl.c_str(), false_lbl.c_str());
        break;
    case icode_op::FUNCTION:
        printf("proc %s (params=%d locals=%d)\n",
               func_name.c_str(), num_params, local_bytes);
        break;
    case icode_op::ENDFUNCTION:
        printf("endproc %s\n", func_name.c_str());
        break;
    case icode_op::RETURN:
        if (left.is_none())
            printf("  ret\n");
        else
            printf("  ret %s\n", left.to_string().c_str());
        break;
    case icode_op::SEND:
        printf("  send(%d,%s) %s\n", argreg, abi_arg_loc_name(arg_loc),
               left.to_string().c_str());
        break;
    case icode_op::RECEIVE:
        printf("  %s = recv(%d,%s)\n", result.to_string().c_str(), argreg,
               abi_arg_loc_name(arg_loc));
        break;
    case icode_op::CALL:
        printf("  %s = call %s (%d)\n",
               result.is_none() ? "_" : result.to_string().c_str(),
               func_name.c_str(), num_params);
        break;
    case icode_op::ASSIGN:
        printf("  %s = %s\n",
               result.to_string().c_str(), left.to_string().c_str());
        break;
    case icode_op::ADDRESS_OF:
        printf("  %s = &%s\n",
               result.to_string().c_str(), left.to_string().c_str());
        break;
    case icode_op::GET_VALUE_AT:
        printf("  %s = *%s\n",
               result.to_string().c_str(), left.to_string().c_str());
        break;
    case icode_op::SET_VALUE_AT:
        printf("  *%s = %s\n",
               result.to_string().c_str(), left.to_string().c_str());
        break;
    case icode_op::CAST:
        printf("  %s = (%s)%s\n",
               result.to_string().c_str(),
               result.type ? result.type->to_string().c_str() : "?",
               left.to_string().c_str());
        break;
    default:
        printf("  %s = %s %s %s\n",
               result.to_string().c_str(),
               left.to_string().c_str(),
               icode_op_name(op),
               right.to_string().c_str());
        break;
    }
}

void ir_function::dump() const {
    printf("; === function %s ===\n", name.c_str());
    for (auto &ic : icodes) ic.dump();
}

void ir_module::dump() const {
    for (auto &g : globals) {
        printf("; global %s : %s\n", g.name.c_str(),
               g.type ? g.type->to_string().c_str() : "?");
    }
    for (auto &fn : functions) fn.dump();
}

} // namespace xcc
