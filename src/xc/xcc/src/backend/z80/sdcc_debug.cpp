//
// sdcc_debug.cpp — SDCC-style ;! debug directives emitter.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//
#include "backend/z80/sdcc_debug.h"
#include <fstream>
#include <ostream>
#include <cstdio>

namespace xcc {

sdcc_debug_emitter::sdcc_debug_emitter(std::ostream &out,
                                       const std::string &source_file,
                                       const std::string &adb_path)
    : out_(out), source_file_(source_file), adb_path_(adb_path) {}

// ----- type_suffix ---------------------------------------------------

std::string sdcc_debug_emitter::type_suffix(const type *t) {
    if (!t) return "%V";
    switch (t->kind) {
    case type_kind::VOID:                  return "%V";
    case type_kind::BOOL:                  return "%U8";
    case type_kind::CHAR:                  return "%S8";
    case type_kind::UCHAR:                 return "%U8";
    case type_kind::SHORT:                 return "%S16";
    case type_kind::USHORT:                return "%U16";
    case type_kind::INT:                   return "%S16";
    case type_kind::UINT:                  return "%U16";
    case type_kind::LONG:                  return "%S32";
    case type_kind::ULONG:                 return "%U32";
    case type_kind::LLONG:                 return "%S64";
    case type_kind::ULLONG:                return "%U64";
    case type_kind::FLOAT:                 return "%SF";
    case type_kind::DOUBLE:                return "%SF";
    case type_kind::ENUM:                  return "%S16";
    case type_kind::COMPLEX:               return "%SF";
    case type_kind::STRUCT:
        if (!t->tag.empty())
            return "%struct:" + t->tag;
        return "%struct:?";
    case type_kind::UNION:
        if (!t->tag.empty())
            return "%union:" + t->tag;
        return "%union:?";
    case type_kind::POINTER:
        // char* is spelled %ASCII; all other pointers are %*basetype
        if (t->base && (t->base->kind == type_kind::CHAR ||
                        t->base->kind == type_kind::UCHAR))
            return "%ASCII";
        return "%*" + type_suffix(t->base.get());
    case type_kind::ARRAY:
        return "%[" + type_suffix(t->base.get()) + "]";
    case type_kind::FUNCTION:
        return "%V";  // function pointers shown as void
    }
    return "%V";
}

// ----- debug_info_emitter interface ----------------------------------

void sdcc_debug_emitter::begin_module() {
    out_ << "; !FILE " << source_file_ << "\n";

    // Emit common fundamental type aliases so the debugger has names.
    out_ << "; !DEFT char %S8\n";
    out_ << "; !DEFT unsigned char %U8\n";
    out_ << "; !DEFT short %S16\n";
    out_ << "; !DEFT unsigned short %U16\n";
    out_ << "; !DEFT int %S16\n";
    out_ << "; !DEFT unsigned int %U16\n";
    out_ << "; !DEFT long %S32\n";
    out_ << "; !DEFT unsigned long %U32\n";
    out_ << "; !DEFT long long %S64\n";
    out_ << "; !DEFT unsigned long long %U64\n";
    out_ << "; !DEFT float %SF\n";
    out_ << "; !DEFT double %SF\n";
}

void sdcc_debug_emitter::emit_location(int line) {
    if (line <= 0 || line == cur_line_) return;
    cur_line_ = line;
    out_ << "; !LINE " << line << "\n";
}

void sdcc_debug_emitter::begin_function(const ir_function &fn,
                                        const std::string &mangled) {
    std::string ret_sfx = type_suffix(fn.ret_type.get());
    out_ << "; !FUNC " << fn.name << " " << ret_sfx << "\n";

    // Scan RECEIVE icodes to emit parameter declarations.
    for (auto &ic : fn.icodes) {
        if (ic.op != icode_op::RECEIVE) continue;
        const operand &p = ic.result;
        if (p.name.empty()) continue;
        std::string sfx = type_suffix(p.type.get());
        // Params live at IX + 4 + stack_offset on Z80.
        int ix_off = 4 + p.stack_offset;
        char loc[32];
        if (ix_off >= 0)
            snprintf(loc, sizeof(loc), "IX+%d", ix_off);
        else
            snprintf(loc, sizeof(loc), "IX%d", ix_off);
        out_ << "; !DEFS " << p.name << " " << sfx << " " << loc << "\n";
    }

    // Record first mangled label for this function in the .adb map.
    line_entries_.push_back({mangled, cur_line_ > 0 ? cur_line_ : 1});
}

void sdcc_debug_emitter::end_function(const ir_function &) {
    out_ << "; !ENDF\n";
}

void sdcc_debug_emitter::end_module(const std::string &) {
    out_ << "; !ENDFILE\n";
    write_adb();
}

// ----- .adb file -----------------------------------------------------

void sdcc_debug_emitter::write_adb() const {
    if (adb_path_.empty()) return;
    std::ofstream f(adb_path_);
    if (!f) return;

    f << "[" << source_file_ << "]\n";
    for (auto &e : line_entries_)
        f << "F" << e.mangled << ":" << e.line << ":0:0:0\n";
}

} // namespace xcc
