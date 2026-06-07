//
// convention.h — Z80 calling-convention abstraction for the xcc backend.
//
// Each supported function ABI maps to a concrete abi_convention subclass.
// The same convention objects are consulted by both IR lowering and the
// Z80 backend so argument layout, stack accounting, and emitted calling
// sequences stay in sync.
//
// Interface:
//   classify_args           — map params to concrete ABI locations
//   stack_arg_bytes         — physical bytes consumed on the caller stack
//   spill_bytes             — frame bytes needed to spill register params
//   emit_prologue/epilogue  — callee entry/exit
//   emit_send               — caller-side argument passing
//   emit_call_cleanup       — caller-side stack repair after direct/indirect call
//   emit_indirect_call      — ABI-specific trampoline for function pointers
//   emit_return_value       — place current function's return value in ABI regs
//   emit_store_call_result  — store a call result from ABI regs into an operand
//
// Concrete subclasses live in z80gen_convention.cpp.
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

    virtual call_abi abi_tag() const = 0;

    virtual std::vector<abi_arg_loc>
    classify_args(const std::vector<type_ptr> &types) const = 0;

    virtual int stack_arg_bytes(type_ptr type, abi_arg_loc loc) const = 0;

    virtual int spill_bytes(type_ptr type, abi_arg_loc loc) const {
        if (loc == abi_arg_loc::STACK) return 0;
        return type ? type->size() : 2;
    }

    // Callee side.
    virtual void emit_prologue  (z80_gen &g, const ir_function &fn) = 0;
    virtual void emit_epilogue  (z80_gen &g, const ir_function &fn) = 0;
    virtual void emit_receive   (z80_gen &g, const icode       &ic) = 0;

    // Caller side.
    virtual void emit_send         (z80_gen &g, const icode &ic) = 0;
    virtual void emit_call_cleanup (z80_gen &g, const icode &ic) = 0;
    virtual void emit_indirect_call(z80_gen &g, const icode &ic) const = 0;
    virtual void emit_return_value(z80_gen &g, const operand &value) const = 0;
    virtual void emit_store_call_result(z80_gen &g, const icode &ic) const = 0;

protected:
    // Shared helpers implemented in z80gen_convention.cpp.
    // All have access to z80_gen private members via friendship.
    static void std_prologue_frame(z80_gen &g, const ir_function &fn);
    static void std_epilogue_frame(z80_gen &g, const ir_function &fn);
    static void std_send_push     (z80_gen &g, const icode &ic);
    static void std_call_cleanup  (z80_gen &g, const icode &ic);
    static void exact_stack_drop  (z80_gen &g, int bytes);
    static void callee_stack_return(z80_gen &g, int bytes);
    static void emit_bc_indirect_call(z80_gen &g, const operand &target,
                                      bool preserve_af,
                                      bool preserve_hl,
                                      bool preserve_de);
    static void emit_legacy_return_value(z80_gen &g, const operand &value);
    static void emit_store_legacy_result(z80_gen &g, const icode &ic);
    static void emit_modern_return_value(z80_gen &g, const operand &value);
    static void emit_store_modern_result(z80_gen &g, const icode &ic);
    static void spill_fastcall_receive(z80_gen &g, const icode &ic);
    static void spill_modern_receive(z80_gen &g, const icode &ic);
    static void materialize_modern_receive(z80_gen &g, const icode &ic);
    static void spill_leading_receives(z80_gen &g, const ir_function &fn,
                                       void (*spill_one)(z80_gen &, const icode &));
};

// ---------------------------------------------------------------------------
// Factory — returns the convention for the given ABI tag
// ---------------------------------------------------------------------------

call_abi effective_call_abi(call_abi abi);
abi_convention &get_abi_convention(call_abi abi);

} // namespace xcc
