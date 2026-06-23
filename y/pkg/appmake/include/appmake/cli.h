// Declares command-line entry helpers for the `appmake` host tool.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih

#pragma once

#include <filesystem>
#include <string_view>
#include <vector>

#include "appmake/types.h"

namespace appmake {

// Filesystem namespace alias used by the public API.
namespace fs = std::filesystem;

// Parse command-line switches starting at the given argument index.
cli_options parse_options(const std::vector<std::string_view>& args, std::size_t start_index);
// Print a human-readable listing of decoded tape entries.
void print_list(const fs::path& tape_path, const std::vector<tape_list_entry>& entries);
// Run the `list` subcommand.
void cmd_list(const fs::path& tape_path);
// Run the `analyze` subcommand.
void cmd_analyze(const fs::path& input);
// Run the `make` subcommand.
void cmd_make(const fs::path& input, const fs::path& mdr_path, const cli_options& options);
// Run the `tap` conversion subcommand.
void cmd_tap(const fs::path& input, const fs::path& output, const cli_options& options);
// Run the `sna` conversion subcommand.
void cmd_sna(const fs::path& input, const fs::path& output, const cli_options& options);
// Print command-line usage help.
void print_usage();

}  // namespace appmake
