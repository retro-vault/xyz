// simple_config.h — tiny key=value config parser for X host tools.
//
// Intended convention for X tool configuration:
// - one file per tool, e.g. xemu.conf
// - UTF-8 text
// - one `key = value` assignment per line
// - `#` and `;` start comments
// - blank lines ignored
// - keys are normalized case-insensitively with `-` and `_` ignored
// - later entries override earlier ones
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
#pragma once

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace xconfig {

struct config_error : std::runtime_error {
    using std::runtime_error::runtime_error;
};

inline std::string trim(std::string_view text) {
    std::size_t start = 0;
    while (start < text.size()
           && std::isspace(static_cast<unsigned char>(text[start])) != 0) {
        ++start;
    }

    std::size_t end = text.size();
    while (end > start
           && std::isspace(static_cast<unsigned char>(text[end - 1])) != 0) {
        --end;
    }

    return std::string(text.substr(start, end - start));
}

inline std::string normalize_key(std::string_view value) {
    std::string key = trim(value);
    std::transform(key.begin(), key.end(), key.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    key.erase(
        std::remove_if(key.begin(), key.end(), [](unsigned char ch) {
            return ch == '-' || ch == '_';
        }),
        key.end());
    return key;
}

struct simple_config_entry {
    std::string key;
    std::string value;
};

using simple_config_entries = std::vector<simple_config_entry>;

inline simple_config_entries parse_simple_config(
    std::istream& input,
    const std::string& source_name)
{
    simple_config_entries values;
    std::string line;
    std::size_t line_no = 0;

    while (std::getline(input, line)) {
        ++line_no;
        const auto comment = line.find_first_of("#;");
        if (comment != std::string::npos) {
            line.erase(comment);
        }

        line = trim(line);
        if (line.empty()) {
            continue;
        }

        const auto eq = line.find('=');
        if (eq == std::string::npos) {
            throw config_error(
                source_name + ":" + std::to_string(line_no) + ": expected key = value");
        }

        const auto key = normalize_key(line.substr(0, eq));
        const auto value = trim(line.substr(eq + 1));
        if (key.empty()) {
            throw config_error(
                source_name + ":" + std::to_string(line_no) + ": empty key");
        }
        values.push_back({key, value});
    }

    return values;
}

inline simple_config_entries parse_simple_config_file(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input.is_open()) {
        throw config_error("cannot open config: " + path.string());
    }
    return parse_simple_config(input, path.string());
}

inline std::vector<std::filesystem::path> default_tool_config_candidates(
    std::string_view tool_name)
{
    std::vector<std::filesystem::path> paths;
    const std::string file_name = std::string(tool_name) + ".conf";

    paths.push_back(std::filesystem::current_path() / file_name);

    if (const char* home = std::getenv("HOME"); home != nullptr && *home != '\0') {
        paths.push_back(
            std::filesystem::path(home) / ".config" / "x" / file_name);
    }

    return paths;
}

inline std::optional<std::filesystem::path> find_default_tool_config(
    std::string_view tool_name)
{
    for (const auto& path : default_tool_config_candidates(tool_name)) {
        if (std::filesystem::is_regular_file(path)) {
            return path;
        }
    }
    return std::nullopt;
}

} // namespace xconfig
