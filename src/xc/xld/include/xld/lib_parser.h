// lib_parser.h
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

#include <xld/library_reader.h>

namespace xld {

    class lib_parser {
    public:
        // Parse either an xld text-index .lib or an ar-style SDCC archive.
        static std::vector<lib_member> parse(
            const std::filesystem::path& path);
    };

} // namespace xld

#endif // XLINK_LIB_PARSER_HPP
