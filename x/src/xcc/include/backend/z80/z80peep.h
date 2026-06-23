//
// z80peep.h -- compatibility wrapper for the shared libxopt Z80 optimizer.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//

#pragma once

#include <xopt/z80peep.h>

namespace xcc {

using asm_line = xopt::asm_line;
using z80_peep = xopt::z80_peep;

} // namespace xcc
