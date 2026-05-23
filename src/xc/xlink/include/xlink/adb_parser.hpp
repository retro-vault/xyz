//
// SDCC .adb debug info parser
//
// MIT License (see: LICENSE)
// copyright (C) 2021 tomaz stih
//
#ifndef XLINK_ADB_PARSER_HPP
#define XLINK_ADB_PARSER_HPP

#include <cstdint>
#include <filesystem>
#include <istream>
#include <optional>
#include <string>
#include <vector>

namespace xlink {

    struct adb_type_info {
        uint32_t size = 0;
        std::string name;
        bool is_function = false;
    };

    enum class adb_storage_class {
        unknown,
        address,
        frame_relative,
        register_name,
        register_pair
    };

    struct adb_function_info {
        std::string raw_name;
        std::string display_name;
        adb_type_info return_type;
        std::optional<uint32_t> line;
        bool file_local = false;
    };

    struct adb_symbol_info {
        std::string raw_name;
        std::string display_name;
        std::optional<std::string> parent_name;
        adb_type_info type;
        adb_storage_class storage = adb_storage_class::unknown;
        std::optional<int32_t> offset;
        std::optional<std::string> register_name;
        std::optional<uint32_t> line;
        bool file_local = false;
        bool global_scope = false;
    };

    struct adb_document {
        std::string module_name;
        std::vector<adb_function_info> functions;
        std::vector<adb_symbol_info> globals;
        std::vector<adb_symbol_info> locals;
    };

    class adb_parser {
    public:
        static adb_document parse(const std::filesystem::path& path);
        static adb_document parse(const std::string& source_name,
                                  std::istream& input);
    };

} // namespace xlink

#endif // XLINK_ADB_PARSER_HPP
