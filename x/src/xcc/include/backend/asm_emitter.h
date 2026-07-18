//
// asm_emitter.h — abstract assembler output interface.
//
// z80_gen calls these methods instead of writing directly to a stream.
// Two concrete implementations are provided:
//   sdasz80_emitter  — SDCC sdasz80 syntax (default, -masm=sdasz80)
//   gnuas_emitter    — GNU binutils as syntax (-masm=gnuas)
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//
#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <iosfwd>

namespace xcc {

enum class call_abi : uint8_t;

class asm_emitter {
public:
    virtual ~asm_emitter() = default;

    // -- Instruction / label / comment ------------------------------------

    // Emit one instruction line (caller provides the body; tab + newline added).
    virtual void instr(const std::string &text) = 0;

    // Emit a label definition, optionally exported.
    virtual void label(const std::string &name, bool global) = 0;

    // Emit a comment line.
    virtual void comment(const std::string &text) = 0;

    // Emit verbatim text without any transformation (inline asm, debug sections).
    virtual void raw(const std::string &text) = 0;

    // -- Module / section directives --------------------------------------

    virtual void module_header() = 0;
    virtual void default_calling_convention(call_abi) {}
    virtual void section_code() = 0;
    virtual void section_code_named(const std::string &name) = 0;
    virtual void section_data() = 0;
    virtual void section_data_named(const std::string &name) = 0;
    virtual void section_rodata() = 0;
    virtual void section_tls() = 0;

    // -- Symbol directives ------------------------------------------------

    virtual void global_decl(const std::string &name) = 0;
    virtual void symbol_assign(const std::string &name, long long val) = 0;
    virtual void symbol_type_function(const std::string &) {}
    virtual void symbol_type_object(const std::string &) {}
    virtual void symbol_size(const std::string &, const std::string &) {}

    // -- Data directives --------------------------------------------------

    virtual void db(int val) = 0;
    virtual void db_list(const std::vector<int> &vals) = 0; // all on one line
    virtual void dw(int val) = 0;                           // 2-byte word
    virtual void dw_sym(const std::string &name) = 0;       // 2-byte symbol address
    virtual void dl(int val) = 0;                           // 4-byte long
    virtual void ds(int bytes) = 0;                         // reserve N bytes

    // -- Instruction operand format helpers -------------------------------

    // Immediate integer: "#42" (sdasz80) or "42" (GNU as).
    virtual std::string imm(long long val) const = 0;

    // Immediate symbol address: "#_foo" (sdasz80) or "_foo" (GNU as).
    virtual std::string imm_sym(const std::string &mangled) const = 0;

    // Low/high byte of a symbol address as an immediate expression.
    virtual std::string imm_sym_lo(const std::string &mangled) const = 0;
    virtual std::string imm_sym_hi(const std::string &mangled) const = 0;

    // IX-relative memory location: "-2(ix)" (sdasz80) or "(ix-2)" (GNU as).
    virtual std::string ix_rel(int off) const = 0;

    // Indirect global: "(#_sym)" or "(#_sym + N)" vs "(_sym)" or "(_sym + N)".
    virtual std::string indir_global(const std::string &mangled, int off = 0) const = 0;

    // -- Underlying stream (for DWARF which writes directly) --------------
    virtual std::ostream &stream() = 0;
};

} // namespace xcc
