//
// Amstrad CPC firmware cassette and AMSDOS disk image builders.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//

#ifndef XPROG_CPC_H
#define XPROG_CPC_H

#include <cstdint>
#include <string>
#include <vector>

namespace xprog {

// Build a CPC Digital Tape image containing a firmware binary file.
std::vector<std::uint8_t> build_cdt(
    const std::vector<std::uint8_t>& binary,
    std::uint16_t load_address,
    std::uint16_t entry_point,
    const std::string& name);

// Build a standard CPCEMU data disk containing one AMSDOS binary file.
std::vector<std::uint8_t> build_dsk(
    const std::vector<std::uint8_t>& binary,
    std::uint16_t load_address,
    std::uint16_t entry_point,
    const std::string& name);

} // namespace xprog

#endif
