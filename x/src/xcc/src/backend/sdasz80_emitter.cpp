//
// sdasz80_emitter.cpp — SDCC sdasz80 assembler syntax output.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//
#include "backend/sdasz80_emitter.h"
#include "frontend/types.h"
#include <algorithm>

namespace xcc {

void sdasz80_emitter::instr(const std::string &text) {
    out_ << "\t" << text << "\n";
}

void sdasz80_emitter::label(const std::string &name, bool global) {
    if (global) out_ << "\t.globl " << name << "\n";
    out_ << name << ":\n";
}

void sdasz80_emitter::comment(const std::string &text) {
    out_ << "\t; " << text << "\n";
}

void sdasz80_emitter::raw(const std::string &text) {
    out_ << text;
}

void sdasz80_emitter::module_header() {
    out_ << "\t.module xcc_output\n\n";
}

void sdasz80_emitter::default_calling_convention(call_abi abi) {
    switch (abi) {
    case call_abi::SDCCCALL0:
        out_ << "\t.optsdcc -mz80 sdcccall(0)\n\n";
        break;
    case call_abi::SDCCCALL1:
        out_ << "\t.optsdcc -mz80 sdcccall(1)\n\n";
        break;
    default:
        break;
    }
}

void sdasz80_emitter::section_code() {
    out_ << "\n\t.area _CODE\n\n";
}

void sdasz80_emitter::section_code_named(const std::string &name) {
    out_ << "\n\t.area " << name << "\n\n";
}

void sdasz80_emitter::section_data() {
    out_ << "\t.area _DATA\n";
}

void sdasz80_emitter::section_data_named(const std::string &name) {
    out_ << "\t.area " << name << "\n";
}

void sdasz80_emitter::section_bss() {
    out_ << "\t.area _BSS\n";
}

void sdasz80_emitter::section_rodata() {
    out_ << "\t.area _CONST\n";
}

void sdasz80_emitter::section_tls() {
    out_ << "\t.area _TLS\n";
}

void sdasz80_emitter::global_decl(const std::string &name) {
    out_ << "\t.globl " << name << "\n";
}

void sdasz80_emitter::symbol_assign(const std::string &name, long long val) {
    out_ << name << " = " << val << "\n";
}

void sdasz80_emitter::db(int val) {
    out_ << "\t.db " << val << "\n";
}

void sdasz80_emitter::db_list(const std::vector<int> &vals) {
    constexpr size_t values_per_line = 16;
    for (size_t first = 0; first < vals.size(); first += values_per_line) {
        const size_t last = std::min(first + values_per_line, vals.size());
        out_ << "\t.db ";
        for (size_t i = first; i < last; ++i) {
            if (i != first) out_ << ", ";
            out_ << vals[i];
        }
        out_ << "\n";
    }
}

void sdasz80_emitter::dw(int val) {
    out_ << "\t.dw " << val << "\n";
}

void sdasz80_emitter::dw_sym(const std::string &name) {
    out_ << "\t.dw " << name << "\n";
}

void sdasz80_emitter::dl(int val) {
    out_ << "\t.dl " << val << "\n";
}

void sdasz80_emitter::ds(int bytes) {
    out_ << "\t.ds " << bytes << "\n";
}

std::string sdasz80_emitter::imm(long long val) const {
    return "#" + std::to_string(val);
}

std::string sdasz80_emitter::imm_sym(const std::string &mangled) const {
    return "#" + mangled;
}

std::string sdasz80_emitter::imm_sym_lo(const std::string &mangled) const {
    return "#<(" + mangled + ")";
}

std::string sdasz80_emitter::imm_sym_hi(const std::string &mangled) const {
    return "#>(" + mangled + ")";
}

std::string sdasz80_emitter::ix_rel(int off) const {
    return std::to_string(off) + "(ix)";
}

std::string sdasz80_emitter::indir_global(const std::string &mangled, int off) const {
    if (off == 0) return "(#" + mangled + ")";
    return "(#" + mangled + " + " + std::to_string(off) + ")";
}

} // namespace xcc
