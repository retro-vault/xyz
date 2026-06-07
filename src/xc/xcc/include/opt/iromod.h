//
// iromod.h — module-level IR optimizer for xcc.
//
// This layer owns translation-unit-wide optimization decisions that need
// visibility across multiple IR functions, such as:
//
//   - removing dead internal-linkage functions
//   - inlining tiny direct-only static helpers for size
//
// Per-function IR optimization remains in iropt.{h,cpp}.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//

#pragma once
#include "ir/icode.h"
#include "opt/opt_settings.h"
#include <memory>
#include <vector>

namespace xcc {

class ir_module_pass {
public:
    virtual ~ir_module_pass() = default;
    virtual const char *name() const = 0;
    virtual bool run(ir_module &mod) = 0;
};

class ir_module_optimizer {
public:
    // Run the module-level IR pipeline using the selected optimization
    // settings. Individual passes may be enabled either by -O presets or
    // by explicit -f... flags.
    static void optimize(ir_module &mod, const optimization_settings &settings);

private:
    static std::vector<std::unique_ptr<ir_module_pass>>
    build_pipeline(const optimization_settings &settings);
};

} // namespace xcc
