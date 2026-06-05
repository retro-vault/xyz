//
// assembler listing parser
//
// MIT License (see: LICENSE)
// copyright (C) 2021 tomaz stih
//
#include <fstream>
#include <regex>
#include <string>

#include <xld/lst_parser.h>
#include <xld/errors.h>

namespace xld {

    namespace {

        static std::string trim(const std::string& s) {
            auto start = s.find_first_not_of(" \t\r\n");
            if (start == std::string::npos) return "";
            auto end = s.find_last_not_of(" \t\r\n");
            return s.substr(start, end - start + 1);
        }

    } // namespace

    std::vector<lst_line_entry> lst_parser::parse(
        const std::filesystem::path& path)
    {
        std::ifstream input(path);
        if (!input.is_open())
            throw parse_error("cannot open listing file: " + path.string());

        std::vector<lst_line_entry> result;
        std::string current_area;
        std::string line;
        std::regex area_re(R"(\.area\s+([A-Za-z0-9_\.]+))");
        std::regex address_re(R"(([0-9A-Fa-f]{8}))");
        std::regex line_re(R"(\s+([0-9]+)\s+(\S.*)$)");
        std::smatch match;

        while (std::getline(input, line)) {
            if (std::regex_search(line, match, area_re)) {
                current_area = match[1];
            }

            if (current_area.empty())
                continue;

            if (!std::regex_search(line, match, address_re))
                continue;
            uint16_t offset = static_cast<uint16_t>(
                std::stoul(match[1], nullptr, 16));

            std::string suffix = line.substr(match.position() + match.length());
            std::smatch line_match;
            if (!std::regex_search(suffix, line_match, line_re))
                continue;

            std::string source_text = trim(line_match[2]);
            if (source_text.empty() || source_text[0] == ';')
                continue;

            lst_line_entry entry;
            entry.area_name = current_area;
            entry.offset = offset;
            entry.line = static_cast<uint32_t>(
                std::stoul(line_match[1], nullptr, 10));
            result.push_back(entry);
        }

        return result;
    }

} // namespace xld
