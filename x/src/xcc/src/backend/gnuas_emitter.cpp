//
// gnuas_emitter.cpp — GNU binutils as assembler syntax output.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//
#include "backend/gnuas_emitter.h"
#include <algorithm>

namespace xcc {

void gnuas_emitter::instr(const std::string &text) {
    out_ << "\t" << text << "\n";
}

void gnuas_emitter::label(const std::string &name, bool global) {
    if (global) out_ << "\t.global " << name << "\n";
    out_ << name << ":\n";
}

void gnuas_emitter::comment(const std::string &text) {
    out_ << "\t; " << text << "\n";
}

void gnuas_emitter::raw(const std::string &text) {
    out_ << text;
}

void gnuas_emitter::module_header() {
    // GNU as has no .module directive.
}

void gnuas_emitter::section_code() {
    out_ << "\n\t.text\n\n";
}

void gnuas_emitter::section_code_named(const std::string &name) {
    out_ << "\n\t.section\t" << name << ",\"ax\"\n\n";
}

void gnuas_emitter::section_data() {
    out_ << "\t.data\n";
}

void gnuas_emitter::section_data_named(const std::string &name) {
    out_ << "\t.section\t" << name << ",\"aw\"\n";
}

void gnuas_emitter::section_bss() {
    out_ << "\t.bss\n";
}

void gnuas_emitter::section_rodata() {
    out_ << "\t.section\t.rodata\n";
}

void gnuas_emitter::section_tls() {
    out_ << "\t.section\t.tdata,\"aw\"\n";
}

void gnuas_emitter::global_decl(const std::string &name) {
    out_ << "\t.global " << name << "\n";
}

void gnuas_emitter::symbol_assign(const std::string &name, long long val) {
    out_ << "\t.set " << name << ", " << val << "\n";
}

void gnuas_emitter::symbol_type_function(const std::string &name) {
    out_ << "\t.type " << name << ", @function\n";
}

void gnuas_emitter::symbol_type_object(const std::string &name) {
    out_ << "\t.type " << name << ", @object\n";
}

void gnuas_emitter::symbol_size(const std::string &name, const std::string &expr) {
    out_ << "\t.size " << name << ", " << expr << "\n";
}

void gnuas_emitter::db(int val) {
    out_ << "\t.byte " << val << "\n";
}

void gnuas_emitter::db_list(const std::vector<int> &vals) {
    constexpr size_t values_per_line = 16;
    for (size_t first = 0; first < vals.size(); first += values_per_line) {
        const size_t last = std::min(first + values_per_line, vals.size());
        out_ << "\t.byte ";
        for (size_t i = first; i < last; ++i) {
            if (i != first) out_ << ", ";
            out_ << vals[i];
        }
        out_ << "\n";
    }
}

void gnuas_emitter::dw(int val) {
    out_ << "\t.short " << val << "\n";
}

void gnuas_emitter::dw_sym(const std::string &name) {
    out_ << "\t.short " << name << "\n";
}

void gnuas_emitter::dl(int val) {
    out_ << "\t.long " << val << "\n";
}

void gnuas_emitter::ds(int bytes) {
    out_ << "\t.space " << bytes << "\n";
}

std::string gnuas_emitter::imm(long long val) const {
    return std::to_string(val);
}

std::string gnuas_emitter::imm_sym(const std::string &mangled) const {
    return mangled;
}

std::string gnuas_emitter::imm_sym_lo(const std::string &mangled) const {
    return "(" + mangled + ") & 255";
}

std::string gnuas_emitter::imm_sym_hi(const std::string &mangled) const {
    return "(" + mangled + " >> 8)";
}

std::string gnuas_emitter::ix_rel(int off) const {
    if (off >= 0) return "(ix+" + std::to_string(off) + ")";
    return "(ix" + std::to_string(off) + ")";
}

std::string gnuas_emitter::indir_global(const std::string &mangled, int off) const {
    if (off == 0) return "(" + mangled + ")";
    return "(" + mangled + " + " + std::to_string(off) + ")";
}

} // namespace xcc
