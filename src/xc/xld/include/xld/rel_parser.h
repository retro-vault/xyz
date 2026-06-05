// rel_parser.h
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
#include <istream>
#include <memory>
#include <string>
#include <vector>

#include <xld/module.h>

namespace xld {

    class rel_parser {
    public:
        // Parse a .rel file into a module.
        static std::shared_ptr<module> parse(const std::filesystem::path& path);
        static std::shared_ptr<module> parse(const std::string& source_name,
                                             std::istream& input);

        // Quick scan: extract defined symbol names without full parse.
        static std::vector<std::string> scan_defs(
            const std::filesystem::path& path);
        static std::vector<std::string> scan_defs(
            const std::string& source_name,
            std::istream& input);
    };

} // namespace xld

#endif // XLINK_REL_PARSER_HPP
