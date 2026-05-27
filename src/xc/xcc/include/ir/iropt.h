//
// iropt.h — IR-level optimizer for xcc.
//
// Runs a fixed-point loop of four passes on an ir_function:
//   constant_fold    — evaluate binary/unary ops whose operands are all INT_CONST
//   copy_prop        — replace ASSIGN(t_dst, t_src) uses with t_src directly
//   dead_code_elim   — remove pure instructions whose result temp is never read
//   strength_reduce  — replace expensive ops with cheaper equivalents:
//                      MUL(x, 2^n)         → SHL(x, n)
//                      DIV(x, 2^n) unsigned → SHR(x, n)
//                      MOD(x, 2^n) unsigned → AND(x, 2^n - 1)
//
// Call ir_optimizer::optimize() once per ir_function, typically after IR
// generation and before code generation.  Activated for -O2 and above.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//

#pragma once
#include "ir/icode.h"

namespace xcc {

class ir_optimizer {
public:
    // Run all passes to fixed-point on fn.
    static void optimize(ir_function &fn);

private:
    // Fold binary/unary ops on INT_CONST operands into a single ASSIGN.
    static bool constant_fold(ir_function &fn);

    // Propagate ASSIGN(t_dst, t_src) — replace t_dst uses with t_src.
    // Only TEMP→TEMP copies are propagated; constant propagation is handled
    // by a second round of constant_fold after copy_prop clears the way.
    static bool copy_prop(ir_function &fn);

    // Remove pure instructions whose result temp has zero remaining uses.
    static bool dead_code_elim(ir_function &fn);

    // Replace MUL/DIV/MOD with cheaper shift/and equivalents when the
    // right-hand operand is a power-of-two constant.
    static bool strength_reduce(ir_function &fn);

    // Fold algebraic identities: x+0→x, x*1→x, x&0→0, x^0→x, etc.
    static bool algebraic_simplify(ir_function &fn);

    // Return true if op (an icode) is side-effect-free and can be deleted
    // when its result is unused.
    static bool is_pure(icode_op op);
};

} // namespace xcc
