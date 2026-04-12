#pragma once

#include <filesystem>
#include <set>
#include <string_view>
#include <vector>

#include "appmake/types.h"

namespace appmake {

namespace fs = std::filesystem;

std::vector<address_range> make_ranges_from_bytes(const std::set<uint16_t>& bytes);
std::vector<address_range> invert_ranges(const std::set<uint16_t>& code_bytes, uint32_t begin, uint32_t end);
analysis_report analyze_tap(const fs::path& path);
analysis_report analyze_tzx(const fs::path& path);
void print_ranges(std::string_view title, const std::vector<address_range>& ranges);
void print_analysis_report(const fs::path& path, const analysis_report& report);

}  // namespace appmake
