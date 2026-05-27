//
// sdcc_debug.h — SDCC-style debug info emitter for the xcc Z80 code generator.
//
// sdcc_debug_emitter implements debug_info_emitter for the sdasz80 dialect.
// It emits SDCC ;! directives inline in the .s assembly file and writes a
// companion .adb file that maps source lines to assembly labels for SDCDB.
//
// ;! directive summary:
//   ;!FILE path        — open compilation unit
//   ;!DEFT name %type  — declare a named type alias
//   ;!FUNC name %type  — begin function, %type = return type
//   ;!DEFS name %type loc — declare symbol; loc = IX+n, D, or SP+n
//   ;!LINE n           — source line
//   ;!ENDF             — end function
//   ;!ENDFILE          — close compilation unit
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//

#pragma once
#include "backend/z80/debug_info.h"
#include "frontend/types.h"
#include <iosfwd>
#include <string>
#include <vector>

namespace xcc {

class sdcc_debug_emitter : public debug_info_emitter {
public:
    //
    // out        — stream for .s inline ;! directives
    // source_file — path of the .c source being compiled
    // adb_path   — output path for the .adb line-number file
    //
    sdcc_debug_emitter(std::ostream &out,
                       const std::string &source_file,
                       const std::string &adb_path);

    void begin_module() override;
    void emit_location(int line) override;
    void begin_function(const ir_function &fn,
                        const std::string &mangled) override;
    void end_function(const ir_function &fn) override;
    void end_module(const std::string &producer) override;

private:
    struct line_entry { std::string mangled; int line; };

    std::ostream       &out_;
    std::string         source_file_;
    std::string         adb_path_;
    int                 cur_line_ = -1;
    std::vector<line_entry> line_entries_;

    //
    // Return the SDCC ;! type suffix for t (e.g., "%S16", "%U8", "%*%S16").
    // Returns "%V" for void or unknown types.
    //
    static std::string type_suffix(const type *t);

    //
    // Write the .adb file from accumulated line_entries_.
    //
    void write_adb() const;
};

} // namespace xcc
