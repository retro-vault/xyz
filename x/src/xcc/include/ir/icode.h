//
// icode.h — three-address intermediate representation for xcc.
//
// Defines the icode (instruction), operand, and the ir_function /
// ir_module containers that IrGen produces and Z80Gen consumes.
//
// The IR uses a three-address form simplified for the Z80 target.  Every instruction has the three-address form:
//
//   result = left  op  right
//
// Not all fields are used by every opcode; see icode_op for details.
//
// Critical convention for SET_VALUE_AT:
//   ic.result = pointer address (where to write)
//   ic.left   = value to store
// This is the inverse of the usual "result = left op right" pattern
// and must be followed consistently to avoid silent store-to-zero bugs.
//
// Operands are tagged with operand_kind to distinguish temporaries
// (anonymous, spill to stack), named symbols (locals/params/globals),
// constants, and label references.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//

#pragma once
#include "frontend/types.h"
#include "frontend/symtab.h"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace xcc {

enum class abi_arg_loc : uint8_t {
    STACK,
    REG_A,
    REG_L,
    REG_HL,
    REG_DE,
    REG_DEHL,
};

// ----- icode_op ------------------------------------------------------

enum class icode_op : uint8_t {
    // ----- control flow ----------------------------------------------
    LABEL,          // label:  (label_name)
    GOTO,           // goto label_name
    IFX,            // if (left) goto true_lbl else goto false_lbl

    // ----- function boundaries ---------------------------------------
    FUNCTION,       // function prologue (func_name, num_params, local_bytes)
    ENDFUNCTION,    // function epilogue
    RETURN,         // return left (left = NONE for void return)

    // ----- parameter passing ------------------------------------------
    SEND,           // push argument: left = value, argreg = arg index
    RECEIVE,        // receive param: result = param[argreg]
    CALL,           // call func_name; num_params args already SENDed

    // ----- data movement ---------------------------------------------
    ASSIGN,         // result = left
    ADDRESS_OF,     // result = &left   (left must be a symbol)
    GET_VALUE_AT,   // result = *left   (pointer dereference)
    SET_VALUE_AT,   // *result = left   (result is pointer, left is value)
    BLOCK_FILL,     // fill result[0..right) with byte left

    // ----- arithmetic ------------------------------------------------
    ADD,
    SUB,
    MUL,
    DIV,
    MOD,
    NEG,            // result = -left

    // ----- bitwise ---------------------------------------------------
    BAND,           // &
    BOR,            // |
    BXOR,           // ^
    BNOT,           // ~left
    SHL,
    SHR,
    ROL,            // rotate left  (16- or 32-bit unsigned integer)
    ROR,            // rotate right (16- or 32-bit unsigned integer)
    PACK_BYTES,     // result(16-bit) = low-byte left | (high-byte right << 8)

    // ----- comparison (result is int 0 or 1) -------------------------
    EQ, NE, LT, LE, GT, GE,

    // ----- type conversion -------------------------------------------
    CAST,           // result = (result.type)left

    // ----- floating-point arithmetic (soft-float via runtime stubs) --
    FADD,           // result = left + right  (float)
    FSUB,           // result = left - right  (float)
    FMUL,           // result = left * right  (float)
    FDIV,           // result = left / right  (float)
    FITOSF,         // result = (float)left   (integer to soft-float)
    FSTOI,          // result = (int)left     (soft-float to integer)

    // ----- stack allocation ------------------------------------------
    ALLOCA,         // result = alloca(left bytes): SP -= left, result = new SP

    // ----- inline assembly -------------------------------------------
    INLINE_ASM,     // verbatim assembly text; stored in icode::asm_text

    // ----- complex number construction --------------------------------
    // Pack two 4-byte soft-float temps into one 8-byte complex slot.
    //   result (COMPLEX, 8 bytes) = { left (float, re), right (float, im) }
    MAKE_COMPLEX,
};

//
// Return a short human-readable name for op (e.g., "ADD", "CALL").
// The returned pointer is static; do not free it.
//
const char *icode_op_name(icode_op op);

// ----- operand_kind --------------------------------------------------

enum class operand_kind : uint8_t {
    NONE,           // operand not used
    TEMP,           // anonymous temporary: identified by temp_id
    SYMBOL,         // named variable / function: name + addressing info
    INT_CONST,      // integer constant: ival
    FLOAT_CONST,    // float constant: fval
    LABEL_REF,      // reference to a code label: name
};

// ----- operand -------------------------------------------------------

struct operand {
    operand_kind kind = operand_kind::NONE;
    type_ptr     type;

    int64_t     ival        = 0;
    double      fval        = 0;
    int         temp_id     = -1;
    std::string name;
    bool        is_global   = false;
    bool        is_param    = false;
    bool        is_func     = false;
    bool        is_tls      = false; // _Thread_local global: access via __tls_base
    bool        is_sfr      = false; // [[sdcc::sfr(N)]]: read via IN, write via OUT
    int         sfr_port    = -1;    // port number when is_sfr is true
    int         stack_offset = 0;
    int         byte_offset  = 0;   // sub-object byte offset (for complex re/im access)

    bool is_none()  const { return kind == operand_kind::NONE;       }
    bool is_temp()  const { return kind == operand_kind::TEMP;       }
    bool is_symbol()const { return kind == operand_kind::SYMBOL;     }
    bool is_const() const { return kind == operand_kind::INT_CONST ||
                                   kind == operand_kind::FLOAT_CONST; }
    bool is_label() const { return kind == operand_kind::LABEL_REF;  }

    //
    // Factory: construct a "no operand" sentinel.
    //
    static operand make_none() { return {}; }

    //
    // Factory: construct an anonymous temporary with the given id and type.
    //
    static operand make_temp(int id, type_ptr t) {
        operand o;
        o.kind = operand_kind::TEMP;
        o.temp_id = id;
        o.type = t;
        return o;
    }

    //
    // Factory: construct an integer constant.
    // If t is nullptr, type defaults to int.
    //
    static operand make_int(int64_t v, type_ptr t = nullptr) {
        operand o;
        o.kind = operand_kind::INT_CONST;
        o.ival = v;
        o.type = t ? t : type::make_int();
        return o;
    }

    //
    // Factory: construct a floating constant.
    //
    static operand make_float(double v, type_ptr t = nullptr) {
        operand o;
        o.kind = operand_kind::FLOAT_CONST;
        o.fval = v;
        o.type = t ? t : type::make_double();
        return o;
    }

    //
    // Factory: construct a symbol operand for a named variable or function.
    // global selects global addressing (label); otherwise IX-relative.
    // param selects parameter addressing (IX+offset+4).
    //
    static operand make_symbol(std::string name, type_ptr t,
                               bool global, bool param, int offset,
                               bool func = false,
                               bool tls = false,
                               bool sfr = false, int sfr_p = -1) {
        operand o;
        o.kind         = operand_kind::SYMBOL;
        o.name         = std::move(name);
        o.type         = t;
        o.is_global    = global;
        o.is_param     = param;
        o.is_func      = func;
        o.is_tls       = tls;
        o.is_sfr       = sfr;
        o.sfr_port     = sfr_p;
        o.stack_offset = offset;
        return o;
    }

    //
    // Factory: construct a label reference (for GOTO and IFX targets).
    //
    static operand make_label(std::string lbl) {
        operand o;
        o.kind = operand_kind::LABEL_REF;
        o.name = std::move(lbl);
        return o;
    }

    //
    // Return a human-readable string representation.
    // Used in IR dumps and error messages.
    //
    std::string to_string() const;
};

// ----- icode ---------------------------------------------------------

struct icode {
    icode_op op        = icode_op::ASSIGN;
    operand  result;
    operand  left;
    operand  right;

    // For LABEL / GOTO / IFX
    std::string label_name;
    std::string true_lbl;
    std::string false_lbl;

    // For FUNCTION / ENDFUNCTION / CALL
    std::string func_name;

    // For INLINE_ASM
    std::string asm_text;
    int         num_params  = 0;
    int         local_bytes = 0;
    int         argreg      = 0; // SEND/RECEIVE: argument slot index
    abi_arg_loc arg_loc     = abi_arg_loc::STACK; // SEND/RECEIVE: concrete ABI location

    int      line        = 0;
    int      arg_bytes   = 0;               // CALL: total bytes pushed by SEND (for SP cleanup)
    call_abi callee_abi  = call_abi::DEFAULT; // SEND/CALL: ABI of the callee function
    bool     callee_cleans_stack = false;   // CALL: callee drops stack-passed args before returning
    bool     callee_noreturn = false;        // CALL: direct callee cannot return

    //
    // Print a human-readable dump of this instruction to stdout.
    //
    void dump() const;
};

// ----- ir_function ---------------------------------------------------

struct ir_function {
    std::string         name;
    bool                is_global  = true;
    bool                discard_if_unused = false;
    std::vector<icode>  icodes;
    type_ptr            ret_type;
    int                 local_bytes = 0;
    int                 num_params  = 0;
    int                 bank        = -1; // [[xcc::bank(N)]] section placement (-1 = default code)

    // Calling-convention variant from [[sdcc::...]] attributes
    call_abi            abi              = call_abi::DEFAULT;
    int                 orig_local_bytes = 0; // sdccall(1): local_bytes before register spill area
    int                 stack_param_bytes = 0; // sdccall(1): bytes physically passed on stack
    bool                is_variadic      = false;
    bool                callee_cleans_stack = false; // epilogue drops stack params before returning
    bool                is_noreturn      = false; // [[noreturn]]: epilogue unreachable
    bool                tail_local_addresses_noescape = false;

    //
    // Dump all instructions of this function to stdout.
    //
    void dump() const;
};

// ----- ir_module -----------------------------------------------------

struct ir_module {
    std::vector<ir_function> functions;

    struct global_var {
        std::string name;
        type_ptr    type;
        int64_t     init_val = 0;
        bool        has_init = false;
        std::string str_init; // set for string literals
        int         char_width = 1; // 1=.db, 2=.dw (char16), 4=.dl (char32) for string literals

        bool    is_tls     = false; // _Thread_local: placed in TLS block, not _DATA
        bool    is_static  = false; // static linkage: do not emit .globl
        int64_t at_address = -1;   // [[sdcc::at(N)]]: emit as absolute symbol (not in _DATA)
        int     sfr_port   = -1;   // [[sdcc::sfr(N)]]: no data emitted, reads/writes use IN/OUT
        int     bank       = -1;   // [[xcc::bank(N)]]: place object in banked data section

        // Each entry is (value, byte_size), or label if non-empty for pointer
        // initializers such as static char *v[] = {"x"}.
        struct init_elem { int64_t value = 0; int size = 2; std::string label; };
        std::vector<init_elem> init_vals;
    };

    std::vector<global_var> globals;
    std::vector<global_var> string_literals;

    //
    // Dump the entire module (globals + all functions) to stdout.
    //
    void dump() const;
};

} // namespace xcc
