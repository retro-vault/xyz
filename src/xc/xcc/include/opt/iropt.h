//
// iropt.h — IR-level optimizer for xcc.
//
// The -O2 pipeline is now organized as a sequence of IR passes that run to
// fixed point over each ir_function:
//
//   cfg_cleanup        — fold constant branches, remove unreachable blocks
//   value_propagation  — IR copy/constant propagation across basic blocks
//                        using SSA-style reaching-value tracking
//   constant_fold      — evaluate constant expressions
//   algebraic_simplify — x+0→x, x*1→x, x&0→0, etc.
//   loop_induction     — rewrite canonical loop multiplies into running values
//   strength_reduce    — turn constant multiplies/divides/mods into shifts/adds
//   loop_licm          — hoist loop-invariant pure IR computations
//   dead_code_elim     — delete pure instructions whose result is unused
//
// The optimizer operates entirely on IR. Target-specific peephole cleanup
// remains in the Z80 backend.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//

#pragma once
#include "ir/icode.h"
#include <memory>
#include <vector>

namespace xcc {

class ir_pass {
public:
    virtual ~ir_pass() = default;
    virtual const char *name() const = 0;
    virtual bool run(ir_function &fn) = 0;
};

class ir_optimizer {
public:
    // Run the -O2 IR pipeline to fixed point on fn.
    static void optimize(ir_function &fn);

private:
    static std::vector<std::unique_ptr<ir_pass>> build_pipeline();
};

} // namespace xcc
