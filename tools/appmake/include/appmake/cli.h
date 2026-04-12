#pragma once

#include <filesystem>
#include <string_view>
#include <vector>

#include "appmake/types.h"

namespace appmake {

namespace fs = std::filesystem;

cli_options parse_options(const std::vector<std::string_view>& args, std::size_t start_index);
void print_list(const fs::path& tape_path, const std::vector<tape_list_entry>& entries);
void cmd_list(const fs::path& tape_path);
void cmd_analyze(const fs::path& input);
void cmd_make(const fs::path& input, const fs::path& mdr_path, const cli_options& options);
void cmd_tap(const fs::path& input, const fs::path& output, const cli_options& options);
void cmd_sna(const fs::path& input, const fs::path& output, const cli_options& options);
void print_usage();

}  // namespace appmake
