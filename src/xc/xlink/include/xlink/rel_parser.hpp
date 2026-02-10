// rel_parser.hpp
//
// SDCC .rel file parser
//
// MIT License (see: LICENSE)
// copyright (C) 2021 tomaz stih
//
// 2021-07-28   tstih
#ifndef XLINK_REL_PARSER_HPP
#define XLINK_REL_PARSER_HPP

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <xlink/module.hpp>

namespace xlink {

    class rel_parser {
    public:
        // Parse a .rel file into a module.
        static std::shared_ptr<module> parse(const std::filesystem::path& path);

        // Quick scan: extract defined symbol names without full parse.
        static std::vector<std::string> scan_defs(
            const std::filesystem::path& path);
    };

} // namespace xlink

#endif // XLINK_REL_PARSER_HPP
