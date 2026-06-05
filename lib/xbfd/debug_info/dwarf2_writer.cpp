// debug_info/dwarf2_writer.cpp — xbfd::dwarf2_writer: DWARF2 streaming emitter.
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
//
#include <ostream>
#include <vector>

#include <xbfd/xbfd.h>

namespace xbfd {

dwarf2_writer::dwarf2_writer(std::ostream& asm_out, const std::string& src)
    : out_(asm_out), src_(src) {}

void dwarf2_writer::str(const std::string& s) {
    if (s.empty()) { out_ << "\t.byte 0\n"; return; }
    out_ << "\t.ascii \"" << s << "\"\n\t.byte 0\n";
}

std::string dwarf2_writer::el(const std::string& n) { return ".Ldbg_" + n + "_end"; }

void dwarf2_writer::abbrev() {
    out_ << "\n\t.section .debug_abbrev\n"
         << "\t.byte 1\n\t.byte 0x11\n\t.byte 0x01\n"
         << "\t.byte 0x25,0x08\n\t.byte 0x13,0x05\n\t.byte 0x03,0x08\n"
         << "\t.byte 0x11,0x01\n\t.byte 0x12,0x01\n\t.byte 0x10,0x06\n\t.byte 0x00,0x00\n"
         << "\t.byte 2\n\t.byte 0x2e\n\t.byte 0x00\n"
         << "\t.byte 0x03,0x08\n\t.byte 0x11,0x01\n\t.byte 0x12,0x01\n\t.byte 0x3f,0x0c\n"
         << "\t.byte 0x00,0x00\n\t.byte 0x00\n";
}

void dwarf2_writer::info(const std::string& producer) {
    out_ << "\n\t.section .debug_info\n.Ldebug_info_begin:\n"
         << "\t.long .Ldebug_info_end-.Ldebug_info_begin-4\n\t.word 2\n\t.long 0\n\t.byte 2\n\t.byte 1\n";
    str(producer);
    out_ << "\t.word 0x000c\n";
    str(src_);
    out_ << "\t.word " << fns_.front().m << "\n\t.word " << el(fns_.back().c) << "\n\t.long 0\n";
    for (const auto& fn : fns_) {
        out_ << "\t.byte 2\n";
        str(fn.c);
        out_ << "\t.word " << fn.m << "\n\t.word " << el(fn.c) << "\n\t.byte " << (fn.g ? 1 : 0) << "\n";
    }
    out_ << "\t.byte 0\n.Ldebug_info_end:\n";
}

void dwarf2_writer::aranges() {
    out_ << "\n\t.section .debug_aranges\n.Ldebug_aranges_begin:\n"
         << "\t.long .Ldebug_aranges_end-.Ldebug_aranges_begin-4\n\t.word 2\n\t.long 0\n\t.byte 2\n\t.byte 0\n\t.word 0\n";
    for (const auto& fn : fns_)
        out_ << "\t.word " << fn.m << "\n\t.word " << el(fn.c) << "-" << fn.m << "\n";
    out_ << "\t.word 0\n\t.word 0\n.Ldebug_aranges_end:\n";
}

void dwarf2_writer::on_module_begin(const std::string&) {
    out_ << "\t.file 1 \"" << src_ << "\"\n";
}

void dwarf2_writer::on_module_end(const std::string& p) {
    if (!fns_.empty()) { abbrev(); info(p); aranges(); }
}

void dwarf2_writer::on_function_begin(const std::string& c, const std::string& m,
                                       bool g, const type_ref&) {
    fns_.push_back({c, m, g});
}

void dwarf2_writer::on_function_end(const std::string& c) {
    out_ << ".Ldbg_" << c << "_end:\n";
}

void dwarf2_writer::on_source_line(int l) {
    if (l > 0 && l != cur_) { cur_ = l; out_ << "\t.loc 1 " << l << " 0\n"; }
}

} // namespace xbfd
