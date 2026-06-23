// Declares helpers that convert snapshots and extracted payloads into the
// xyz application image format consumed by YOS loaders and tooling.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih

#pragma once

#include <filesystem>
#include <vector>

#include "appmake/types.h"

namespace appmake {

// Filesystem namespace alias used by the public API.
namespace fs = std::filesystem;

// Parse a 48K `.sna` snapshot into normalized register and RAM state.
snapshot_48 parse_sna48(const fs::path& path);
// Build the serialized machine-state block stored in one app payload.
std::vector<uint8_t> build_snapshot_state(const snapshot_48& sna, uint16_t sp);
// Validate that one payload range and entry point fit the target layout.
void validate_range(uint16_t load_addr, std::size_t size, uint16_t entry_addr);
// Build one complete xyz application image from header, state, and payload.
std::vector<uint8_t> build_app(
    const app_header& header,
    const std::vector<uint8_t>& state,
    const std::vector<uint8_t>& payload
);

}  // namespace appmake
