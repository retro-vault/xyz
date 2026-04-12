#pragma once

#include <filesystem>
#include <vector>

#include "appmake/types.h"

namespace appmake {

namespace fs = std::filesystem;

snapshot_48 parse_sna48(const fs::path& path);
std::vector<uint8_t> build_snapshot_state(const snapshot_48& sna, uint16_t sp);
void validate_range(uint16_t load_addr, std::size_t size, uint16_t entry_addr);
std::vector<uint8_t> build_app(
    const app_header& header,
    const std::vector<uint8_t>& state,
    const std::vector<uint8_t>& payload
);

}  // namespace appmake
