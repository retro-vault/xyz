//
// xopt.cpp -- public optimizer interface implementation.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//

#include "xopt/xopt.h"
#include "xopt/z80peep.h"

namespace xopt {

namespace {

int choose_pass_budget(const std::string &asm_text,
                       optimization_level level) {
    const size_t bytes = asm_text.size();

    if (bytes > 350000) {
        if (level == optimization_level::o1)
            return 0;
        if (level == optimization_level::o2 ||
            level == optimization_level::os ||
            level == optimization_level::of ||
            level == optimization_level::o3)
            return 2;
        return 3;
    }

    if (bytes > 200000) {
        if (level == optimization_level::o1)
            return 2;
        if (level == optimization_level::o2 ||
            level == optimization_level::os ||
            level == optimization_level::of ||
            level == optimization_level::o3)
            return 4;
        return 6;
    }

    return 10;
}

} // namespace

bool uses_speed_biased_rules(optimization_level level) {
    // Direct xopt keeps this laboratory bundle for smoke coverage and future
    // experiments. The xcc driver maps stable -Of/-O3 presets to xopt -Os
    // until individual speed rules prove profitable for compiler output.
    return level == optimization_level::of ||
           level == optimization_level::o3;
}

std::string optimize_z80_assembly(const std::string &asm_text,
                                  optimization_level level) {
    if (level == optimization_level::none)
        return asm_text;
    const int pass_budget = choose_pass_budget(asm_text, level);
    return z80_peep::optimize(asm_text,
                              uses_speed_biased_rules(level),
                              pass_budget);
}

std::string optimize_assembly(const std::string &asm_text,
                              const optimizer_options &options) {
    return optimize_z80_assembly(asm_text, options.level);
}

} // namespace xopt
