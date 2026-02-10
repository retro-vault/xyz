// lib_parser.cpp
//
// SDCC .lib file parser
//
// MIT License (see: LICENSE)
// copyright (C) 2021 tomaz stih
//
// 2021-07-28   tstih
#include <fstream>
#include <string>

#include <xlink/lib_parser.hpp>
#include <xlink/errors.hpp>

namespace xlink {

    // Helper: trim whitespace.
    static std::string trim(const std::string& s) {
        auto start = s.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) return "";
        auto end = s.find_last_not_of(" \t\r\n");
        return s.substr(start, end - start + 1);
    }

    std::vector<std::filesystem::path> lib_parser::parse(
        const std::filesystem::path& path)
    {
        std::ifstream file(path);
        if (!file.is_open())
            throw parse_error("cannot open library file: " + path.string());

        auto lib_dir = path.parent_path();
        std::vector<std::filesystem::path> result;
        std::string line;

        while (std::getline(file, line)) {
            line = trim(line);
            if (line.empty()) continue;

            // .lib files contain relative paths to .rel files,
            // one per line.
            std::filesystem::path rel_path = lib_dir / line;
            result.push_back(rel_path);
        }

        return result;
    }

} // namespace xlink
