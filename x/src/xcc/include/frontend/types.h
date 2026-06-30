//
// types.h — C11 type system for the Z80 target.
//
// Defines type_kind (the classification of a C type), the type struct
// that represents any C11 type, and the type_ptr alias
// (shared_ptr<type>) used throughout the compiler.
//
// Type sizes for the xcc Z80 target:
//   char / unsigned char     1 byte
//   short / int              2 bytes
//   long                     4 bytes
//   long long                8 bytes
//   float                    selected by --float-format
//   double                   8 bytes  (IEEE 754 double)
//   pointer                  2 bytes  (16-bit near)
//
// Factory functions (type::make_int(), type::make_pointer(), …) are the
// normal way to create type nodes.  Direct construction is allowed only
// inside types.cpp for the factories themselves.
//
// Qualifier methods (is_const, is_volatile, is_restrict) are stored on
// the type node and are inherited by pointers and arrays in the usual
// C way.  unqual() returns a copy with all qualifiers cleared.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//

#pragma once
#include <cassert>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace xcc {

struct type;
using type_ptr = std::shared_ptr<type>;

// The storage and arithmetic ABI used for the C `float` type.  `double`
// remains the existing 64-bit software double; this setting only changes
// scalar float.
enum class float_format : uint8_t {
    IEEE32,
    IEEE16,
    FIXED8_8,
    FIXED16_16,
    FIXED24_8,
};

void set_float_format(float_format format);
float_format get_float_format();
const char *float_format_name(float_format format);
int float_format_size(float_format format);
int float_format_fraction_bits(float_format format);
int64_t encode_float_constant(double value, type_ptr target_type);

// ----- call_abi ------------------------------------------------------
//
// Calling convention for a function type or symbol, set by vendor attributes
// such as [[sdcc::sdccall(N)]] or [[z88dk::fastcall]] / [[z88dk::callee]].
// DEFAULT resolves to the driver-selected default call mode.

enum class call_abi : uint8_t {
    DEFAULT,     // driver-selected default call mode
    SDCCCALL0,   // [[sdcc::sdccall(0)]] / [[z88dk::stdc]] — explicit stack-based ABI
    SDCCCALL1,   // [[sdcc::sdccall(1)]] — SDCC 4.3+ register-based ABI
    Z88DK_SMALLC,   // [[z88dk::smallc]] — Small-C stack ABI, left-to-right args
    Z88DK_FASTCALL, // [[z88dk::fastcall]] — first eligible arg in L/HL/DEHL, rest on stack
    Z88DK_CALLEE,   // [[z88dk::callee]]   — stack args, callee repairs stack
    NAKED,       // [[sdcc::naked]]      — no prologue/epilogue emitted
    INTERRUPT,   // [[sdcc::interrupt]]  — ISR: save all regs, reti
    CRITICAL,    // [[sdcc::critical]]   — wrap body with di/ei
};

void set_default_call_abi(call_abi abi);
call_abi get_default_call_abi();

// ----- type_kind -----------------------------------------------------

enum class type_kind : uint8_t {
    VOID,
    BOOL,           // _Bool
    CHAR,           // signed char
    UCHAR,          // unsigned char
    SHORT,
    USHORT,
    INT,
    UINT,
    LONG,
    ULONG,
    LLONG,
    ULLONG,
    FLOAT,
    DOUBLE,
    POINTER,        // base = pointee type
    ARRAY,          // base = element type, array_size set
    FUNCTION,       // ret = return type, params set
    STRUCT,
    UNION,
    ENUM,           // treated as int
    COMPLEX,        // float _Complex / double _Complex: 8 bytes (re+im as soft-floats)
    BITINT,         // _BitInt(N): bit-precise integer, backed by nearest 1/2/4-byte type
    CHAR8T,         // char8_t: distinct unsigned-char-sized type for UTF-8 (C23)
};

// ----- struct_field --------------------------------------------------

struct struct_field {
    std::string name;
    type_ptr    type;
    int         offset     = 0;  // byte offset of the storage unit
    int         bit_width  = -1; // -1 = not a bit-field; ≥0 = width in bits
    int         bit_offset = 0;  // bit position within the storage unit at offset
};

// ----- type ----------------------------------------------------------

struct type {
    type_kind kind;

    bool is_const    = false;
    bool is_volatile = false;
    bool is_restrict = false;
    bool is_vla      = false; // true for VLA array types (int a[n])
    bool is_far      = false; // POINTER: 24-bit banked pointer ([[xcc::far]])
                              // low 16 bits = address, high 8 bits = bank.
                              // 3 bytes wide; dereference goes through the
                              // far-access runtime trampoline.

    // POINTER and ARRAY: pointee / element type
    type_ptr base;
    // ARRAY: element count (0 = incomplete / unsized)
    int array_size = 0;

    // BITINT: bit width (1–64) and signedness
    int  bitint_width    = 0;
    bool bitint_unsigned = false;

    // FUNCTION: return type, parameter types, and variadic flag
    type_ptr              ret;
    std::vector<type_ptr> params;
    bool                  variadic     = false;
    bool                  is_prototyped = false; // true for f(void)/f(T)/f() in C23
    call_abi              func_abi      = call_abi::DEFAULT;

    // STRUCT / UNION: tag name, field list, and completeness flag
    std::string                tag;
    std::vector<struct_field>  fields;
    bool                       complete = false;

    // ----- factories -------------------------------------------------

    //
    // Return a new void type.
    //
    static type_ptr make_void()   { return std::make_shared<type>(type_kind::VOID);   }

    //
    // Return a new _Bool type.
    //
    static type_ptr make_bool()   { return std::make_shared<type>(type_kind::BOOL);   }

    //
    // Return a new signed char type.
    //
    static type_ptr make_char()   { return std::make_shared<type>(type_kind::CHAR);   }

    //
    // Return a new unsigned char type.
    //
    static type_ptr make_uchar()  { return std::make_shared<type>(type_kind::UCHAR);  }

    //
    // Return a new short type.
    //
    static type_ptr make_short()  { return std::make_shared<type>(type_kind::SHORT);  }

    //
    // Return a new unsigned short type.
    //
    static type_ptr make_ushort() { return std::make_shared<type>(type_kind::USHORT); }

    //
    // Return a new int type.
    //
    static type_ptr make_int()    { return std::make_shared<type>(type_kind::INT);    }

    //
    // Return a new unsigned int type.
    //
    static type_ptr make_uint()   { return std::make_shared<type>(type_kind::UINT);   }

    //
    // Return a new long type (4 bytes on Z80).
    //
    static type_ptr make_long()   { return std::make_shared<type>(type_kind::LONG);   }

    //
    // Return a new unsigned long type.
    //
    static type_ptr make_ulong()  { return std::make_shared<type>(type_kind::ULONG);  }

    //
    // Return a new long long type (8 bytes on Z80).
    //
    static type_ptr make_llong()  { return std::make_shared<type>(type_kind::LLONG);  }

    //
    // Return a new unsigned long long type.
    //
    static type_ptr make_ullong() { return std::make_shared<type>(type_kind::ULLONG); }

    //
    // Return a new float type (storage selected by --float-format).
    //
    static type_ptr make_float()  { return std::make_shared<type>(type_kind::FLOAT);  }

    //
    // Return a new double type (8 bytes on Z80).
    //
    static type_ptr make_double() { return std::make_shared<type>(type_kind::DOUBLE); }

    //
    // Return a pointer type whose pointee is base.
    //
    static type_ptr make_pointer(type_ptr base) {
        auto t = std::make_shared<type>(type_kind::POINTER);
        t->base = std::move(base);
        return t;
    }

    //
    // Return a far (24-bit banked) pointer type whose pointee is base.
    // size() is 3: low 16 bits address, high 8 bits bank selector.
    //
    static type_ptr make_far_pointer(type_ptr base) {
        auto t = std::make_shared<type>(type_kind::POINTER);
        t->base   = std::move(base);
        t->is_far = true;
        return t;
    }

    //
    // Return an array type with element type elem and element count sz.
    // Pass sz=0 for an incomplete (unsized) array.
    //
    static type_ptr make_array(type_ptr elem, int sz) {
        auto t = std::make_shared<type>(type_kind::ARRAY);
        t->base = std::move(elem);
        t->array_size = sz;
        return t;
    }

    //
    // Return a function type with the given return type, parameter
    // type list, and variadic flag.
    //
    static type_ptr make_function(type_ptr ret,
                                  std::vector<type_ptr> params,
                                  bool variadic = false,
                                  call_abi abi = call_abi::DEFAULT) {
        auto t = std::make_shared<type>(type_kind::FUNCTION);
        t->ret       = std::move(ret);
        t->params    = std::move(params);
        t->variadic  = variadic;
        t->func_abi  = abi;
        return t;
    }

    //
    // Return a new incomplete struct type with optional tag.
    // Call type::complete = true and populate type::fields after
    // all members have been parsed.
    //
    static type_ptr make_struct(std::string tag = "") {
        auto t = std::make_shared<type>(type_kind::STRUCT);
        t->tag = std::move(tag);
        return t;
    }

    //
    // Return a new incomplete union type with optional tag.
    //
    static type_ptr make_union(std::string tag = "") {
        auto t = std::make_shared<type>(type_kind::UNION);
        t->tag = std::move(tag);
        return t;
    }

    //
    // Return a new enum type (underlying integer type is int).
    //
    static type_ptr make_enum() {
        return std::make_shared<type>(type_kind::ENUM);
    }

    //
    // Return a float _Complex type (8 bytes: real+imaginary as two soft-floats).
    //
    static type_ptr make_complex() {
        return std::make_shared<type>(type_kind::COMPLEX);
    }

    //
    // Return a _BitInt(N) type with the given width (1–64) and signedness.
    // Size rounds up to the nearest of 1, 2, or 4 bytes (Z80 max 32-bit useful).
    //
    static type_ptr make_char8t() {
        return std::make_shared<type>(type_kind::CHAR8T);
    }

    static type_ptr make_bitint(int width, bool is_unsigned) {
        auto t = std::make_shared<type>(type_kind::BITINT);
        t->bitint_width    = width;
        t->bitint_unsigned = is_unsigned;
        return t;
    }

    // ----- predicates ------------------------------------------------

    //
    // Return true if this is any integer kind including _Bool and enum.
    //
    bool is_integer()  const;

    //
    // Return true if this is an unsigned integer kind.
    //
    bool is_unsigned() const;

    //
    // Return true if this is any arithmetic type (integer or float/double).
    //
    bool is_arith()    const;

    //
    // Return true if this is a scalar type (arithmetic or pointer).
    //
    bool is_scalar()   const;

    bool is_ptr()     const { return kind == type_kind::POINTER;  }
    bool is_far_ptr() const { return kind == type_kind::POINTER && is_far; }
    bool is_array()   const { return kind == type_kind::ARRAY;    }
    bool is_func()    const { return kind == type_kind::FUNCTION; }
    bool is_complex() const { return kind == type_kind::COMPLEX;  }

    // ----- size and alignment ----------------------------------------

    //
    // Return the byte size of this type on the Z80 target.
    // Returns 0 for void and incomplete struct/union.
    //
    int size()  const;

    //
    // Return the alignment requirement in bytes.
    // On Z80, all types align to 1; 2+ byte types use 2 for performance.
    //
    int align() const;

    //
    // Return a human-readable C-style spelling of this type.
    // Used in error messages and IR dumps.
    //
    std::string to_string() const;

    //
    // Return a copy of this type with all CV-qualifiers stripped.
    //
    type_ptr unqual() const {
        auto t = std::make_shared<type>(*this);
        t->is_const = t->is_volatile = t->is_restrict = false;
        return t;
    }

    explicit type(type_kind k) : kind(k) {}
};

// ----- arithmetic conversion helpers ---------------------------------

//
// Apply the usual arithmetic conversions (C11 §6.3.1.8) to a and b and
// return the common result type.  Both operands must be arithmetic.
//
type_ptr usual_arith_conv(type_ptr a, type_ptr b);

//
// Apply integer promotions (C11 §6.3.1.1) to t.
// Returns t unchanged if it already has at least int rank.
//
type_ptr integer_promote(type_ptr t);

} // namespace xcc
