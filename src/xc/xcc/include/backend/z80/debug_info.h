//
// debug_info.h — Abstract debug information emitter interface.
//
// debug_info_emitter is the common protocol for debug output back-ends.
// Concrete implementations:
//   dwarf_emitter    — DWARF 2 sections (.debug_info etc.) for GNU as
//   sdcc_debug_emitter — SDCC-style ;! directives + .adb for sdasz80
//
// z80_gen holds a std::unique_ptr<debug_info_emitter> and talks through
// this interface; the driver creates the right subclass based on the
// -g flag and the selected assembler dialect (-masm=).
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//

#pragma once
#include "ir/icode.h"
#include <string>

namespace xcc {

class debug_info_emitter {
public:
    virtual ~debug_info_emitter() = default;

    //
    // Called once at the start of module emission.
    // Emits file-level directives (.file for DWARF, ;!FILE for SDCC).
    //
    virtual void begin_module() = 0;

    //
    // Called before each icode; line <= 0 means unknown.
    // Implementations should suppress duplicate consecutive lines.
    //
    virtual void emit_location(int line) = 0;

    //
    // Called before the function entry label is emitted.
    // mangled is the assembly label (e.g., "_main").
    // fn provides full IR metadata (name, ret_type, params via RECEIVE icodes).
    //
    virtual void begin_function(const ir_function &fn,
                                const std::string &mangled) = 0;

    //
    // Called immediately after the function's ret instruction.
    //
    virtual void end_function(const ir_function &fn) = 0;

    //
    // Called once at the end of module emission, after all functions.
    // producer is a tool/version string (e.g., "xcc 0.1.0").
    //
    virtual void end_module(const std::string &producer) = 0;
};

} // namespace xcc
