//
// dwarf.h — DWARF 2 debug info emitter for the xcc Z80 code generator.
//
// dwarf_emitter implements debug_info_emitter for the GNU as dialect.
// It emits three DWARF 2 sections at end-of-module:
//
//   .debug_abbrev  — abbreviation table
//   .debug_info    — compile unit DIE + one subprogram DIE per function
//   .debug_aranges — address ranges (one entry per function)
//
// .debug_line is generated automatically by the assembler from the
// .file / .loc directives emitted inline by emit_location().
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//

#pragma once
#include "backend/z80/debug_info.h"
#include <iosfwd>
#include <string>
#include <vector>

namespace xcc {

class dwarf_emitter : public debug_info_emitter {
public:
    //
    // Construct an emitter that writes to out for source_file.
    //
    dwarf_emitter(std::ostream &out, const std::string &source_file);

    void begin_module() override;
    void emit_location(int line) override;
    void begin_function(const ir_function &fn,
                        const std::string &mangled) override;
    void end_function(const ir_function &fn) override;
    void end_module(const std::string &producer) override;

private:
    struct fn_info {
        std::string c_name;
        std::string mangled;
        bool        is_global = false;
    };

    std::ostream        &out_;
    std::string          source_file_;
    int                  cur_line_ = -1;
    std::vector<fn_info> functions_;

    void emit_debug_abbrev();
    void emit_debug_info(const std::string &producer);
    void emit_debug_aranges();
    void emit_string(const std::string &s);
    static std::string end_label(const std::string &c_name);
};

} // namespace xcc
