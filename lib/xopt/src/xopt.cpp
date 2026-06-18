//
// xopt.cpp -- public optimizer interface implementation.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//

#include "xopt/xopt.h"
#include "xopt/z80peep.h"

namespace xopt {

bool uses_speed_biased_rules(optimization_level level) {
    return level == optimization_level::of ||
           level == optimization_level::o3;
}

std::string optimize_z80_assembly(const std::string &asm_text,
                                  optimization_level level) {
    if (level == optimization_level::none)
        return asm_text;
    return z80_peep::optimize(asm_text,
                              uses_speed_biased_rules(level),
                              level == optimization_level::o3);
}

std::string optimize_assembly(const std::string &asm_text,
                              const optimizer_options &options) {
    return optimize_z80_assembly(asm_text, options.level);
}

} // namespace xopt
