// Declares small reference tables for ZX ROM routines and system variables
// used by the analyzer and reporting helpers in `appmake`.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih

#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace appmake {

// Description record for one ROM routine entry point.
struct rom_routine_info {
    uint16_t addr = 0;
    std::string_view description;
};

// Description record for one ZX Spectrum system-variable range.
struct sysvar_info {
    uint16_t begin = 0;
    uint16_t end = 0;
    std::string_view name;
    std::string_view description;
};

// Look up one known ROM routine by address.
const rom_routine_info* lookup_rom_routine(uint16_t addr);
// Look up one known system-variable range by address.
const sysvar_info* lookup_sysvar(uint16_t addr);

// Format one ROM routine reference for display.
std::string format_rom_routine(uint16_t addr);
// Format one system-variable reference for display.
std::string format_sysvar(uint16_t addr);

}  // namespace appmake
