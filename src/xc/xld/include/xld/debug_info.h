//
// linked debug metadata builder
//
// MIT License (see: LICENSE)
// copyright (C) 2021 tomaz stih
//
#ifndef XLINK_DEBUG_INFO_HPP
#define XLINK_DEBUG_INFO_HPP

#include <cstdint>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include <xbfd/xbfd.h>
#include <xld/adb_parser.h>
#include <xld/link_context.h>

namespace xld {

    enum class debug_language {
        unknown,
        c,
        assembly
    };

    struct debug_function_info {
        std::string display_name;
        std::string fallback_name;
        uint32_t start_address = 0;
        uint32_t end_address = 0; // exclusive end
        std::optional<uint32_t> line;
        std::optional<std::string> return_type;
        bool file_local = false;
        xbfd::calling_convention calling_convention = xbfd::calling_convention::unknown;
    };

    struct linked_module_debug_info {
        const module* mod = nullptr;
        debug_language language = debug_language::unknown;
        std::filesystem::path source_path;
        std::map<uint32_t, uint32_t> line_by_address;
        std::map<std::string, debug_function_info> functions;
        std::optional<adb_document> adb;
        std::optional<xbfd::debug_info> cdb;
    };

    struct linked_debug_info {
        std::filesystem::path image_path;
        uint32_t entry_address = 0;
        std::vector<linked_module_debug_info> modules;
    };

    class debug_info_builder {
    public:
        static linked_debug_info build(const std::filesystem::path& image_path,
                                       const link_context& ctx);

        static uint32_t symbol_absolute_addr(const module* mod,
                                             const symbol& sym);

        static std::optional<uint32_t> find_symbol_address(
            const link_context& ctx, const std::string& name);

        static std::optional<uint32_t> find_symbol_address(
            const link_context& ctx,
            const module* preferred_module,
            const std::string& name);

        static std::string normalize_path_string(
            const std::filesystem::path& path);
    };

} // namespace xld

#endif // XLINK_DEBUG_INFO_HPP
