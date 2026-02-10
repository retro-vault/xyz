// lib_parser.hpp
//
// SDCC .lib file parser
//
// MIT License (see: LICENSE)
// copyright (C) 2021 tomaz stih
//
// 2021-07-28   tstih
#ifndef XLINK_LIB_PARSER_HPP
#define XLINK_LIB_PARSER_HPP

#include <filesystem>
#include <vector>

namespace xlink {

    class lib_parser {
    public:
        // Parse a .lib index file, returning paths to .rel modules.
        // Paths are resolved relative to the .lib file's directory.
        static std::vector<std::filesystem::path> parse(
            const std::filesystem::path& path);
    };

} // namespace xlink

#endif // XLINK_LIB_PARSER_HPP
