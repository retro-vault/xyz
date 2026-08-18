#ifndef XPROG_TAPE_H
#define XPROG_TAPE_H

#include <cstdint>
#include <string>
#include <vector>

namespace xprog {

std::vector<std::uint8_t> build_tap(
    const std::vector<std::uint8_t>& binary,
    std::uint16_t load_address,
    std::uint16_t entry_point,
    const std::string& name);

std::vector<std::uint8_t> tap_to_tzx(
    const std::vector<std::uint8_t>& tap);

} // namespace xprog

#endif
