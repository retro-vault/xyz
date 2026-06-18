//
// xopt.h -- public optimizer interface.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//

#pragma once

#include <string>
#include <vector>

namespace xopt {

enum class optimization_level {
    none,
    o1,
    o2,
    os,
    of,
    o3,
};

struct optimizer_options {
    optimization_level level = optimization_level::o2;
    bool cross_file = false;
};

struct assembly_cost {
    long bytes = 0;
    long cycles = 0;
    long unknown_instructions = 0;
};

struct optimization_stats {
    assembly_cost before;
    assembly_cost after;

    long bytes_saved() const { return before.bytes - after.bytes; }
    long cycles_saved() const { return before.cycles - after.cycles; }
};

struct register_touch {
    std::string reg;
    long reads = 0;
    long writes = 0;
    long touches = 0;
};

struct register_coverage_scope {
    std::string name;
    long first_line = 0;
    long instruction_count = 0;
    long branch_count = 0;
    long call_count = 0;
    long memory_touch_count = 0;
    long alternate_touch_count = 0;
    long alternate_switch_count = 0;
    double pressure = 0.0;
    std::vector<register_touch> registers;
};

struct register_coverage_window {
    std::string scope;
    long first_line = 0;
    long instruction_count = 0;
    long alternate_touch_count = 0;
    long alternate_switch_count = 0;
    double pressure = 0.0;
    std::vector<register_touch> registers;
};

struct register_coverage_report {
    register_coverage_scope total;
    std::vector<register_coverage_scope> routines;
    std::vector<register_coverage_window> hot_windows;
};

std::string optimize_assembly(const std::string &asm_text,
                              const optimizer_options &options);

std::string optimize_z80_assembly(const std::string &asm_text,
                                  optimization_level level);

assembly_cost estimate_z80_assembly_cost(const std::string &asm_text);

optimization_stats analyze_assembly_optimization(
    const std::string &asm_text,
    const optimizer_options &options);

register_coverage_report analyze_z80_register_coverage(
    const std::string &asm_text,
    size_t window_size = 16,
    size_t max_windows = 8);

bool uses_speed_biased_rules(optimization_level level);

} // namespace xopt
