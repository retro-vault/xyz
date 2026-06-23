//
// assembler listing parser
//
// MIT License (see: LICENSE)
// copyright (C) 2021 tomaz stih
//
#ifndef XLINK_LST_PARSER_HPP
#define XLINK_LST_PARSER_HPP

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace xld {

    struct lst_line_entry {
        std::string area_name;
        uint16_t offset = 0;
        uint32_t line = 0;
    };

    class lst_parser {
    public:
        static std::vector<lst_line_entry> parse(
            const std::filesystem::path& path);
    };

} // namespace xld

#endif // XLINK_LST_PARSER_HPP
