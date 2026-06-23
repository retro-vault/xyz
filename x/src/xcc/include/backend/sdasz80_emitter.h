//
// sdasz80_emitter.h — SDCC sdasz80 assembler syntax output.
//
// Implements asm_emitter for the SDCC sdasz80 assembler.
// Selected with -masm=sdasz80 (the default).
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//
#pragma once
#include "backend/asm_emitter.h"
#include <ostream>

namespace xcc {

class sdasz80_emitter : public asm_emitter {
public:
    explicit sdasz80_emitter(std::ostream &out) : out_(out) {}

    void instr(const std::string &text) override;
    void label(const std::string &name, bool global) override;
    void comment(const std::string &text) override;
    void raw(const std::string &text) override;

    void module_header() override;
    void section_code() override;
    void section_data() override;
    void section_rodata() override;
    void section_tls() override;

    void global_decl(const std::string &name) override;
    void symbol_assign(const std::string &name, long long val) override;

    void db(int val) override;
    void db_list(const std::vector<int> &vals) override;
    void dw(int val) override;
    void dw_sym(const std::string &name) override;
    void dl(int val) override;
    void ds(int bytes) override;

    std::string imm(long long val) const override;
    std::string imm_sym(const std::string &mangled) const override;
    std::string imm_sym_lo(const std::string &mangled) const override;
    std::string imm_sym_hi(const std::string &mangled) const override;
    std::string ix_rel(int off) const override;
    std::string indir_global(const std::string &mangled, int off = 0) const override;

    std::ostream &stream() override { return out_; }

private:
    std::ostream &out_;
};

} // namespace xcc
