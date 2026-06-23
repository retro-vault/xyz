// Declares the tape-analysis helpers used by `appmake analyze` to infer
// code ranges, ROM dependencies, and executable entry assumptions.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih

#pragma once

#include <filesystem>
#include <set>
#include <string_view>
#include <vector>

#include "appmake/types.h"

namespace appmake {

// Filesystem namespace alias used by the public API.
namespace fs = std::filesystem;

// Convert a sparse address set into merged half-open ranges.
std::vector<address_range> make_ranges_from_bytes(const std::set<uint16_t>& bytes);
// Return the complement of known code ranges inside one address window.
std::vector<address_range> invert_ranges(const std::set<uint16_t>& code_bytes, uint32_t begin, uint32_t end);
// Analyze a TAP image and return a structured report.
analysis_report analyze_tap(const fs::path& path);
// Analyze a TZX image and return a structured report.
analysis_report analyze_tzx(const fs::path& path);
// Print one labeled address-range list.
void print_ranges(std::string_view title, const std::vector<address_range>& ranges);
// Print a full human-readable analysis report.
void print_analysis_report(const fs::path& path, const analysis_report& report);

}  // namespace appmake
