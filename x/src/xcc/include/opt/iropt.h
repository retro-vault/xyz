//
// iropt.h — IR-level optimizer for xcc.
//
// The -O2 pipeline is now organized as a sequence of IR passes that run to
// fixed point over each ir_function:
//
//   cfg_cleanup        — fold constant branches, remove unreachable blocks
//   address_deref_fold — fold &local followed by direct *p / *p=... back
//                        into normal local accesses
//   value_propagation  — IR copy/constant propagation across basic blocks
//                        using SSA-style reaching-value tracking
//   constant_fold      — evaluate constant expressions
//   algebraic_simplify — x+0→x, x*1→x, x&0→0, etc.
//   loop_induction     — rewrite canonical loop multiplies into running values
//   strength_reduce    — turn constant multiplies/divides/mods into shifts/adds
//   loop_licm          — hoist loop-invariant pure IR computations
//   dead_code_elim     — delete pure instructions whose result is unused
//   local_frame_compaction — drop dead stack locals and pack surviving slots
//
// The simplest local algebraic identities are intentionally table-driven
// so the SSA simplifier and the later algebraic-simplify pass share the
// same small rewrite set instead of drifting apart.
//
// The optimizer operates entirely on IR. Target-specific peephole cleanup
// remains in the Z80 backend.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//

#pragma once
#include "ir/icode.h"
#include "opt/opt_settings.h"
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
    // Run the selected per-function IR pipeline to fixed point on fn.
    static void optimize(ir_function &fn, const optimization_settings &settings);

private:
    static std::vector<std::unique_ptr<ir_pass>>
    build_pipeline(const optimization_settings &settings);
};

} // namespace xcc
