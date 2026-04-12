#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace appmake {

struct rom_routine_info {
    uint16_t addr = 0;
    std::string_view description;
};

struct sysvar_info {
    uint16_t begin = 0;
    uint16_t end = 0;
    std::string_view name;
    std::string_view description;
};

const rom_routine_info* lookup_rom_routine(uint16_t addr);
const sysvar_info* lookup_sysvar(uint16_t addr);

std::string format_rom_routine(uint16_t addr);
std::string format_sysvar(uint16_t addr);

}  // namespace appmake
