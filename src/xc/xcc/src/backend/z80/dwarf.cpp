//
// dwarf.cpp — DWARF 2 debug info emitter for the xcc Z80 code generator.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//
#include "backend/z80/dwarf.h"
#include <ostream>

namespace xcc {

dwarf_emitter::dwarf_emitter(std::ostream &out, const std::string &source_file)
    : out_(out), source_file_(source_file) {}

std::string dwarf_emitter::end_label(const std::string &c_name) {
    return ".Ldbg_" + c_name + "_end";
}

void dwarf_emitter::emit_string(const std::string &s) {
    if (s.empty()) { out_ << "\t.byte 0\n"; return; }
    out_ << "\t.ascii \"" << s << "\"\n";
    out_ << "\t.byte 0\n";
}

// ----- debug_info_emitter interface ------------------------------------

void dwarf_emitter::begin_module() {
    std::string name = source_file_.empty() ? "unknown.c" : source_file_;
    out_ << "\t.file 1 \"" << name << "\"\n";
}

void dwarf_emitter::emit_location(int line) {
    if (line <= 0 || line == cur_line_) return;
    cur_line_ = line;
    out_ << "\t.loc 1 " << line << " 0\n";
}

void dwarf_emitter::begin_function(const ir_function &fn,
                                   const std::string &mangled) {
    functions_.push_back({fn.name, mangled, fn.is_global});
}

void dwarf_emitter::end_function(const ir_function &fn) {
    out_ << end_label(fn.name) << ":\n";
}

void dwarf_emitter::end_module(const std::string &producer) {
    if (functions_.empty()) return;
    emit_debug_abbrev();
    emit_debug_info(producer);
    emit_debug_aranges();
}

// ----- .debug_abbrev ---------------------------------------------------

void dwarf_emitter::emit_debug_abbrev() {
    out_ << "\n\t.section .debug_abbrev\n";

    out_ << "\t.byte 1\n";            // abbrev code
    out_ << "\t.byte 0x11\n";         // DW_TAG_compile_unit
    out_ << "\t.byte 0x01\n";         // DW_CHILDREN_yes
    out_ << "\t.byte 0x25, 0x08\n";   // DW_AT_producer, DW_FORM_string
    out_ << "\t.byte 0x13, 0x05\n";   // DW_AT_language, DW_FORM_data2
    out_ << "\t.byte 0x03, 0x08\n";   // DW_AT_name, DW_FORM_string
    out_ << "\t.byte 0x11, 0x01\n";   // DW_AT_low_pc, DW_FORM_addr
    out_ << "\t.byte 0x12, 0x01\n";   // DW_AT_high_pc, DW_FORM_addr
    out_ << "\t.byte 0x10, 0x06\n";   // DW_AT_stmt_list, DW_FORM_data4
    out_ << "\t.byte 0x00, 0x00\n";   // end of attributes

    out_ << "\t.byte 2\n";            // abbrev code
    out_ << "\t.byte 0x2e\n";         // DW_TAG_subprogram
    out_ << "\t.byte 0x00\n";         // DW_CHILDREN_no
    out_ << "\t.byte 0x03, 0x08\n";   // DW_AT_name, DW_FORM_string
    out_ << "\t.byte 0x11, 0x01\n";   // DW_AT_low_pc, DW_FORM_addr
    out_ << "\t.byte 0x12, 0x01\n";   // DW_AT_high_pc, DW_FORM_addr
    out_ << "\t.byte 0x3f, 0x0c\n";   // DW_AT_external, DW_FORM_flag
    out_ << "\t.byte 0x00, 0x00\n";   // end of attributes

    out_ << "\t.byte 0x00\n";         // end of table
}

// ----- .debug_info -----------------------------------------------------

void dwarf_emitter::emit_debug_info(const std::string &producer) {
    const auto &first = functions_.front();
    const auto &last  = functions_.back();

    out_ << "\n\t.section .debug_info\n";
    out_ << ".Ldebug_info_begin:\n";

    out_ << "\t.long .Ldebug_info_end - .Ldebug_info_begin - 4\n";
    out_ << "\t.word 2\n";
    out_ << "\t.long 0\n";
    out_ << "\t.byte 2\n";            // address_size = 2 (Z80)

    // compile unit DIE
    out_ << "\t.byte 1\n";
    emit_string(producer);
    out_ << "\t.word 0x000c\n";       // DW_LANG_C99
    emit_string(source_file_.empty() ? "unknown.c" : source_file_);
    out_ << "\t.word " << first.mangled << "\n";
    out_ << "\t.word " << end_label(last.c_name) << "\n";
    out_ << "\t.long 0\n";

    for (auto &fn : functions_) {
        out_ << "\t.byte 2\n";
        emit_string(fn.c_name);
        out_ << "\t.word " << fn.mangled << "\n";
        out_ << "\t.word " << end_label(fn.c_name) << "\n";
        out_ << "\t.byte " << (fn.is_global ? 1 : 0) << "\n";
    }

    out_ << "\t.byte 0\n";
    out_ << ".Ldebug_info_end:\n";
}

// ----- .debug_aranges --------------------------------------------------

void dwarf_emitter::emit_debug_aranges() {
    out_ << "\n\t.section .debug_aranges\n";
    out_ << ".Ldebug_aranges_begin:\n";

    out_ << "\t.long .Ldebug_aranges_end - .Ldebug_aranges_begin - 4\n";
    out_ << "\t.word 2\n";
    out_ << "\t.long 0\n";
    out_ << "\t.byte 2\n";
    out_ << "\t.byte 0\n";
    out_ << "\t.word 0\n";            // padding

    for (auto &fn : functions_) {
        out_ << "\t.word " << fn.mangled << "\n";
        out_ << "\t.word " << end_label(fn.c_name) << " - " << fn.mangled << "\n";
    }

    out_ << "\t.word 0\n";
    out_ << "\t.word 0\n";
    out_ << ".Ldebug_aranges_end:\n";
}

} // namespace xcc
