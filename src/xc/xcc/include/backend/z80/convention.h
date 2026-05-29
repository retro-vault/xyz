//
// convention.h — Z80 calling-convention abstraction for the xcc backend.
//
// Each C23 [[sdcc::sdccall(N)]] / [[sdcc::naked]] / [[sdcc::interrupt]] /
// [[sdcc::critical]] attribute maps to a concrete abi_convention subclass.
// z80_gen selects the right subclass at the start of each function and
// delegates all ABI-specific emission through it, so the main code
// generator stays mode-agnostic.
//
// Interface:
//   emit_prologue  — callee: function entry label + frame setup
//   emit_epilogue  — callee: frame teardown + return instruction
//   emit_send      — caller: pass one argument (register or stack)
//   emit_receive   — callee: accept one incoming parameter
//   emit_call_cleanup — caller: remove stack-passed arguments after CALL
//   stack_bytes_for   — how many bytes argreg N contributes to the stack
//
// Concrete subclasses: cc_default, cc_sdcccall1, cc_naked, cc_interrupt,
// cc_critical (all defined in z80gen_convention.cpp).
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//

#pragma once
#include "ir/icode.h"
#include <memory>

namespace xcc {

class z80_gen;  // forward – defined in z80gen.h

// ---------------------------------------------------------------------------
// Abstract base
// ---------------------------------------------------------------------------

struct abi_convention {
    virtual ~abi_convention() = default;

    // Callee side.
    virtual void emit_prologue  (z80_gen &g, const ir_function &fn) = 0;
    virtual void emit_epilogue  (z80_gen &g, const ir_function &fn) = 0;
    virtual void emit_receive   (z80_gen &g, const icode       &ic) = 0;

    // Caller side.
    virtual void emit_send         (z80_gen &g, const icode &ic) = 0;
    virtual void emit_call_cleanup (z80_gen &g, const icode &ic) = 0;

    // How many bytes argreg N pushes onto the stack.
    // Returns 0 for register-passed arguments (e.g. sdccall(1) args 0-2).
    virtual int stack_bytes_for(int /*argreg*/, int arg_sz) const {
        return (arg_sz <= 1) ? 2 : (arg_sz + 1) & ~1;
    }

protected:
    // Shared helpers implemented in z80gen_convention.cpp.
    // All have access to z80_gen private members via friendship.
    static void std_prologue_frame(z80_gen &g, const ir_function &fn);
    static void std_epilogue_frame(z80_gen &g, const ir_function &fn);
    static void std_send_push     (z80_gen &g, const icode &ic);
    static void std_call_cleanup  (z80_gen &g, const icode &ic);
};

// ---------------------------------------------------------------------------
// Factory — returns the convention for the given ABI tag
// ---------------------------------------------------------------------------

std::unique_ptr<abi_convention> make_abi_convention(call_abi abi);

} // namespace xcc
