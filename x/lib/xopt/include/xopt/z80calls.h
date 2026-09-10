//
// Final Z80 caller-save cleanup using transitive machine-code clobbers.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//

#pragma once

#include <string>

namespace xopt {

// Remove compiler-marked BC/IY saves only when every reachable instruction
// in the final local callee preserves the pair. Run after all code rewrites.
std::string remove_preserved_z80_caller_saves(const std::string &assembly);

} // namespace xopt
