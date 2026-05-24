//
// xdbg debug sidecar emitter
//
// MIT License (see: LICENSE)
// copyright (C) 2021 tomaz stih
//
#include <algorithm>
#include <filesystem>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include <xdbg/io.hpp>
#include <xdbg/model.hpp>

#include <xlink/debug_info.hpp>
#include <xlink/errors.hpp>
#include <xlink/xdbg_emitter.hpp>

namespace xlink {

    namespace {

        static bool starts_with(const std::string& s, const std::string& prefix) {
            return s.rfind(prefix, 0) == 0;
        }

        static xdbg::language_kind to_xdbg_language(debug_language language) {
            switch (language) {
            case debug_language::c:
                return xdbg::language_kind::c;
            case debug_language::assembly:
                return xdbg::language_kind::assembly;
            case debug_language::unknown:
                return xdbg::language_kind::unknown;
            }
            return xdbg::language_kind::unknown;
        }

        static bool is_data_area_name(const std::string& name) {
            return name == "_DATA"
                || name == "_BSS"
                || name == "_HEAP"
                || name == "_INITIALIZED"
                || name == "_INITIALIZER"
                || name == "_DABS"
                || name == "_CABS";
        }

        static bool is_code_area_name(const std::string& name) {
            return !is_data_area_name(name) && name != ".ABS.";
        }

        static bool is_internal_symbol_name(const std::string& name,
                                            const std::string& module_name)
        {
            if (starts_with(name, "A$") || starts_with(name, "C$")
                || starts_with(name, "G$") || starts_with(name, "XG$")) {
                return true;
            }

            const std::string local_start = "F" + module_name + "$";
            const std::string local_end = "XF" + module_name + "$";
            if (starts_with(name, local_start) || starts_with(name, local_end))
                return true;

            return name.size() > 3 && name[0] == '.'
                && name[1] == '_' && name[2] == '_';
        }

        static std::optional<uint32_t> find_line_for_address(
            const std::map<uint32_t, uint32_t>& lines, uint32_t address)
        {
            auto it = lines.find(address);
            if (it != lines.end())
                return it->second;
            return std::nullopt;
        }

        static bool has_accessible_source_file(
            const std::filesystem::path& path)
        {
            if (path.empty())
                return false;

            std::error_code ec;
            return std::filesystem::exists(path, ec) && !ec;
        }

        static xdbg::storage_kind to_xdbg_storage(adb_storage_class storage) {
            switch (storage) {
            case adb_storage_class::address:
                return xdbg::storage_kind::address;
            case adb_storage_class::frame_relative:
                return xdbg::storage_kind::frame_relative;
            case adb_storage_class::register_name:
                return xdbg::storage_kind::register_name;
            case adb_storage_class::register_pair:
                return xdbg::storage_kind::register_pair;
            case adb_storage_class::unknown:
                return xdbg::storage_kind::unknown;
            }
            return xdbg::storage_kind::unknown;
        }

        class source_file_registry {
        public:
            uint32_t add_file(xdbg::document& doc,
                              const std::filesystem::path& path,
                              xdbg::language_kind language)
            {
                const std::string key =
                    debug_info_builder::normalize_path_string(path) + "#"
                    + std::to_string(static_cast<int>(language));
                auto it = ids_.find(key);
                if (it != ids_.end())
                    return it->second;

                uint32_t id = next_id_++;
                doc.files.push_back({
                    id,
                    debug_info_builder::normalize_path_string(path),
                    language
                });
                ids_[key] = id;
                return id;
            }

        private:
            std::map<std::string, uint32_t> ids_;
            uint32_t next_id_ = 1;
        };

        static std::string choose_unique_name(
            std::set<std::string>& used_names,
            const std::string& preferred,
            const std::string& fallback)
        {
            if (!preferred.empty() && used_names.insert(preferred).second)
                return preferred;
            if (!fallback.empty() && used_names.insert(fallback).second)
                return fallback;

            std::string base = !preferred.empty() ? preferred : fallback;
            int counter = 2;
            while (!used_names.insert(base + "#" + std::to_string(counter)).second)
                counter++;
            return base + "#" + std::to_string(counter);
        }
        static std::string resolve_parent_name(
                                               const std::map<std::string, std::string>& function_names,
                                               const std::string& parent_name)
        {
            auto it = function_names.find(parent_name);
            if (it != function_names.end())
                return it->second;
            return parent_name;
        }

    } // namespace

    void xdbg_emitter::emit(const std::filesystem::path& path,
                            const std::filesystem::path& image_path,
                            const link_context& ctx) const
    {
        auto debug = debug_info_builder::build(image_path, ctx);

        xdbg::document doc;
        doc.version = 1;
        doc.image_path = debug_info_builder::normalize_path_string(debug.image_path);
        doc.entry_address = debug.entry_address;

        source_file_registry files;
        std::set<std::string> used_names;
        std::set<std::string> added_symbol_keys;
        std::vector<std::map<std::string, std::string>> function_names(
            debug.modules.size());
        std::vector<std::optional<uint32_t>> file_ids(
            debug.modules.size());

        for (std::size_t module_index = 0;
             module_index < debug.modules.size();
             ++module_index) {
            const auto& info = debug.modules[module_index];
            auto language = to_xdbg_language(info.language);
            if (language == xdbg::language_kind::unknown
                || info.line_by_address.empty()
                || !has_accessible_source_file(info.source_path)) {
                continue;
            }

            file_ids[module_index] = files.add_file(doc, info.source_path, language);
            for (const auto& [address, line] : info.line_by_address) {
                xdbg::line_entry entry;
                entry.address = address;
                entry.file_id = file_ids[module_index].value();
                entry.line = line;
                entry.column = 1;
                doc.lines.push_back(entry);
            }
        }

        for (std::size_t module_index = 0;
             module_index < debug.modules.size();
             ++module_index) {
            const auto& info = debug.modules[module_index];
            auto language = to_xdbg_language(info.language);
            for (const auto& [display_name, range] : info.functions) {
                std::string actual_name = choose_unique_name(
                    used_names, display_name, range.fallback_name);
                function_names[module_index][display_name] = actual_name;

                xdbg::function function;
                function.name = actual_name;
                function.start_address = range.start_address;
                function.end_address = range.end_address;
                if (file_ids[module_index].has_value()) {
                    function.file_id = file_ids[module_index].value();
                    function.line = range.line;
                    function.column = 1;
                }
                if (range.return_type.has_value())
                    function.return_type = range.return_type.value();
                function.language = language;
                doc.functions.push_back(function);

                xdbg::symbol symbol;
                symbol.name = actual_name;
                symbol.kind = xdbg::symbol_kind::function;
                symbol.address = range.start_address;
                symbol.file_id = function.file_id;
                symbol.line = function.line;
                symbol.column = function.column;
                if (function.end_address > function.start_address) {
                    symbol.size = function.end_address - function.start_address;
                }
                if (function.return_type.has_value())
                    symbol.type_name = function.return_type.value();
                symbol.language = language;

                std::string key = symbol.name + "@"
                    + std::to_string(symbol.address);
                if (added_symbol_keys.insert(key).second)
                    doc.symbols.push_back(symbol);
            }

            if (info.adb.has_value()) {
                for (const auto& global : info.adb->globals) {
                    std::optional<uint32_t> address;
                    if (global.global_scope) {
                        address = debug_info_builder::find_symbol_address(
                            ctx, global.display_name);
                    }
                    if (!address.has_value())
                        address = debug_info_builder::find_symbol_address(
                            ctx, info.mod, global.raw_name);
                    if (!address.has_value())
                        continue;

                    std::string actual_name = choose_unique_name(
                        used_names, global.display_name, global.raw_name);

                    xdbg::symbol symbol;
                    symbol.name = actual_name;
                    symbol.kind = xdbg::symbol_kind::object;
                    symbol.address = address.value();
                    if (global.type.size != 0)
                        symbol.size = global.type.size;
                    if (file_ids[module_index].has_value())
                        symbol.file_id = file_ids[module_index].value();
                    if (global.line.has_value())
                        symbol.line = global.line.value();
                    symbol.column = 1;
                    if (!global.type.name.empty())
                        symbol.type_name = global.type.name;
                    symbol.language = language;

                    std::string key = symbol.name + "@"
                        + std::to_string(symbol.address);
                    if (added_symbol_keys.insert(key).second)
                        doc.symbols.push_back(symbol);
                }

                for (const auto& local : info.adb->locals) {
                    if (!local.parent_name.has_value())
                        continue;

                    std::string resolved_parent = resolve_parent_name(
                        function_names[module_index], local.parent_name.value());
                    uint32_t start = 0;
                    uint32_t end = 0;
                    bool have_range = false;
                    auto original = info.functions.find(local.parent_name.value());
                    if (original != info.functions.end()) {
                        start = original->second.start_address;
                        end = original->second.end_address;
                        have_range = true;
                    }

                    xdbg::variable variable;
                    variable.name = local.display_name;
                    variable.kind = xdbg::symbol_kind::local;
                    variable.parent_name = resolved_parent;
                    variable.storage = to_xdbg_storage(local.storage);
                    if (local.storage == adb_storage_class::address) {
                        variable.address = debug_info_builder::find_symbol_address(
                            ctx, info.mod, local.raw_name);
                    }
                    variable.offset = local.offset;
                    variable.register_name = local.register_name;
                    if (!local.type.name.empty())
                        variable.type_name = local.type.name;
                    if (have_range) {
                        variable.start_address = start;
                        variable.end_address = end;
                    }
                    if (file_ids[module_index].has_value()) {
                        variable.file_id = file_ids[module_index].value();
                    }
                    if (local.line.has_value())
                        variable.line = local.line.value();
                    variable.column = 1;
                    variable.language = language;
                    doc.variables.push_back(variable);
                }
            }

            for (const auto& sym : info.mod->symbols()) {
                if (!sym.is_def())
                    continue;
                if (is_internal_symbol_name(sym.name(), info.mod->name()))
                    continue;

                uint32_t address =
                    debug_info_builder::symbol_absolute_addr(info.mod, sym);
                std::string key = sym.name() + "@" + std::to_string(address);
                if (!added_symbol_keys.insert(key).second)
                    continue;

                xdbg::symbol symbol;
                symbol.name = sym.name();
                symbol.address = address;
                int area_index = sym.area_index();
                if (area_index >= 0
                    && area_index < static_cast<int>(info.mod->areas().size())
                    && is_code_area_name(info.mod->areas()[area_index].name())) {
                    symbol.kind = xdbg::symbol_kind::label;
                } else {
                    symbol.kind = xdbg::symbol_kind::object;
                }
                if (file_ids[module_index].has_value()) {
                    symbol.file_id = file_ids[module_index].value();
                    symbol.line = find_line_for_address(
                        info.line_by_address, address);
                    symbol.column = 1;
                }
                symbol.language = language;
                doc.symbols.push_back(symbol);
            }
        }

        for (const auto& [name, value] : ctx.linker_symbols) {
            std::string key = name + "@" + std::to_string(value);
            if (!added_symbol_keys.insert(key).second)
                continue;

            xdbg::symbol symbol;
            symbol.name = name;
            symbol.address = value;
            symbol.kind = starts_with(name, "l__")
                ? xdbg::symbol_kind::constant
                : xdbg::symbol_kind::section;
            symbol.language = xdbg::language_kind::unknown;
            doc.symbols.push_back(symbol);
        }

        std::sort(doc.files.begin(), doc.files.end(),
                  [](const auto& a, const auto& b) { return a.id < b.id; });
        std::sort(doc.lines.begin(), doc.lines.end(),
                  [](const auto& a, const auto& b) {
                      if (a.address != b.address) return a.address < b.address;
                      if (a.file_id != b.file_id) return a.file_id < b.file_id;
                      return a.line < b.line;
                  });
        std::sort(doc.functions.begin(), doc.functions.end(),
                  [](const auto& a, const auto& b) {
                      if (a.start_address != b.start_address)
                          return a.start_address < b.start_address;
                      return a.name < b.name;
                  });
        std::sort(doc.symbols.begin(), doc.symbols.end(),
                  [](const auto& a, const auto& b) {
                      if (a.address != b.address) return a.address < b.address;
                      return a.name < b.name;
                  });
        std::sort(doc.variables.begin(), doc.variables.end(),
                  [](const auto& a, const auto& b) {
                      if (a.parent_name != b.parent_name)
                          return a.parent_name < b.parent_name;
                      return a.name < b.name;
                  });

        xdbg::write_file(path, doc);
    }

} // namespace xlink
