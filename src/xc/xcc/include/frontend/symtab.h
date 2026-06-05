//
// symtab.h — scoped symbol table for the xcc parser and IR generator.
//
// Manages the two C namespaces that overlap in a translation unit:
//   ordinary namespace — variables, functions, typedefs, enum constants
//   tag namespace      — struct/union/enum tags
//
// Scopes are pushed on function entry and compound-statement entry, and
// popped on exit.  Global scope (depth 0) is always present.
//
// symbol holds all per-identifier information the rest of the compiler
// needs: kind, type, storage class, and its location in the Z80 stack
// frame (IX-relative offset for locals and params, or "use the label"
// for globals).
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//

#pragma once
#include "types.h"
#include "token.h"
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace xcc {

// ----- storage_class -------------------------------------------------

enum class storage_class {
    NONE, AUTO, EXTERN, STATIC, REGISTER, TYPEDEF
};

// ----- call_abi ------------------------------------------------------
//
// Calling convention for a function, set by vendor attributes such as
// [[sdcc::sdccall(N)]] or [[z88dk::fastcall]] / [[z88dk::callee]].
// DEFAULT maps to SDCCCALL1, matching modern SDCC's z80 default ABI.

enum class call_abi {
    DEFAULT,     // same as SDCCCALL1 — register-based ABI, current xcc default
    SDCCCALL0,   // [[sdcc::sdccall(0)]] — explicit stack-based ABI
    SDCCCALL1,   // [[sdcc::sdccall(1)]] — SDCC 4.3+ register-based ABI
    Z88DK_FASTCALL, // [[z88dk::fastcall]] — one arg in L/HL/DEHL, std return regs
    Z88DK_CALLEE,   // [[z88dk::callee]]   — stack args, callee repairs stack
    NAKED,       // [[sdcc::naked]]      — no prologue/epilogue emitted
    INTERRUPT,   // [[sdcc::interrupt]]  — ISR: save all regs, reti
    CRITICAL,    // [[sdcc::critical]]   — wrap body with di/ei
};

// ----- sym_kind ------------------------------------------------------

enum class sym_kind {
    VAR,         // variable (local, global, or parameter)
    FUNC,        // function name
    TYPE,        // typedef alias
    ENUM_CONST,  // enumerator value
    STRUCT_TAG,  // struct tag (tag namespace)
    UNION_TAG,   // union tag
    ENUM_TAG     // enum tag
};

// ----- symbol --------------------------------------------------------

struct symbol {
    std::string  name;
    sym_kind     kind        = sym_kind::VAR;
    type_ptr     type;
    storage_class storage   = storage_class::NONE;
    int          scope_depth = 0;

    // Stack frame location on the Z80:
    //   locals:  negative offset from IX  (e.g., -2 = IX-2)
    //   params:  0-based index * 2; real offset = stack_offset + 4
    //   globals: 0 (address is the mangled label name)
    int  stack_offset = 0;
    bool is_param     = false;
    bool is_global    = false;
    bool is_tls       = false; // _Thread_local: access via __tls_base + offset

    // Set for sym_kind::ENUM_CONST; holds the integer value.
    int64_t enum_val = 0;

    // Compile-time constant value for const-qualified variables with
    // integer initializers (used by const_eval for constant folding).
    std::optional<int64_t> const_val;

    // For static locals: the mangled assembly-level name (e.g. _func__var_0).
    // Empty for all other symbols; when non-empty, used by irgen/z80gen instead of name.
    std::string asm_name;

    std::shared_ptr<symbol> vla_size_sym; // for VLA pointer vars: hidden local storing byte count

    source_loc defined_at;

    // ----- C23 standard attributes -----------------------------------
    bool        attr_noreturn    = false;
    bool        attr_deprecated  = false;
    std::string deprecated_msg;
    bool        attr_nodiscard   = false;
    std::string nodiscard_msg;
    bool        attr_maybe_unused = false;
    bool        attr_unsequenced  = false;
    bool        attr_reproducible = false;

    // ----- SDCC vendor attributes (namespace sdcc::) ----------------
    call_abi    abi       = call_abi::DEFAULT;
    int64_t     at_address = -1;  // [[sdcc::at(addr)]] — absolute address (-1 = not set)
    int         sfr_port   = -1;  // [[sdcc::sfr(port)]] — SFR port number (-1 = not set)
};

using sym_ptr = std::shared_ptr<symbol>;

// ----- scope ---------------------------------------------------------
//
// A single lexical scope.  symbol_table holds a stack of these.

class scope {
public:
    //
    // Construct a scope at the given nesting depth.
    //
    explicit scope(int depth) : depth_(depth) {}

    //
    // Insert name → sym into this scope.
    // Any existing entry for the same name is overwritten.
    //
    void insert(const std::string &name, sym_ptr sym) {
        table_[name] = std::move(sym);
    }

    //
    // Look up name in this scope only.
    // Returns nullptr if the name is not present.
    //
    sym_ptr lookup(const std::string &name) const {
        auto it = table_.find(name);
        return it != table_.end() ? it->second : nullptr;
    }

    //
    // Return the nesting depth of this scope (0 = global).
    //
    int depth() const { return depth_; }

private:
    std::unordered_map<std::string, sym_ptr> table_;
    int depth_;
};

// ----- symbol_table --------------------------------------------------

class symbol_table {
public:
    //
    // Construct a symbol table with an empty global scope (depth 0).
    //
    symbol_table();

    //
    // Push a new inner scope.  Called at the opening { of a compound
    // statement or function body.
    //
    void push_scope();

    //
    // Pop the innermost scope and discard all symbols defined in it.
    //
    void pop_scope();

    //
    // Return the depth of the current (innermost) scope.
    //
    int  current_depth() const {
        return static_cast<int>(scopes_.size()) - 1;
    }

    //
    // Return true if the current scope is the global (file) scope.
    //
    bool is_global() const { return scopes_.size() == 1; }

    //
    // Insert sym into the current scope.
    // Returns false if a symbol with the same name is already defined
    // in the current scope (redeclaration error).
    //
    bool insert(sym_ptr sym);

    //
    // Search for name from the innermost scope outward.
    // Returns nullptr if not found in any scope.
    //
    sym_ptr lookup(const std::string &name) const;

    //
    // Search for name only in the current (innermost) scope.
    // Used to detect redeclarations within the same block.
    //
    sym_ptr lookup_current(const std::string &name) const;

    //
    // Insert a struct/union/enum tag into the current tag scope.
    // Returns false if the tag already exists in the current scope.
    //
    bool    insert_tag(const std::string &name, type_ptr ty);

    //
    // Search for a tag name from the innermost tag scope outward.
    // Returns nullptr if not found.
    //
    type_ptr lookup_tag(const std::string &name) const;

private:
    std::vector<scope>                                    scopes_;
    std::vector<std::unordered_map<std::string, type_ptr>> tag_scopes_;
};

// ----- scope_guard ---------------------------------------------------
//
// RAII wrapper: calls push_scope() on construction and pop_scope() on
// destruction.  Use in place of matched push/pop calls to eliminate
// the risk of unbalanced scope depth on early return or exception.

class scope_guard {
public:
    explicit scope_guard(symbol_table &syms) : syms_(syms) {
        syms_.push_scope();
    }
    ~scope_guard() { syms_.pop_scope(); }
    scope_guard(const scope_guard &) = delete;
    scope_guard &operator=(const scope_guard &) = delete;
private:
    symbol_table &syms_;
};

} // namespace xcc
