//
// NoICE .noi emitter
//
// MIT License (see: LICENSE)
// copyright (C) 2021 tomaz stih
//
#include <algorithm>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <xlink/debug_info.hpp>
#include <xlink/errors.hpp>
#include <xlink/noice_emitter.hpp>

namespace xlink {

    namespace {

        static bool starts_with(const std::string& s, const std::string& prefix) {
            return s.rfind(prefix, 0) == 0;
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

        static std::string hex16(uint32_t value) {
            std::ostringstream out;
            out << "0x"
                << std::uppercase << std::hex
                << std::setw(4) << std::setfill('0')
                << (value & 0xffffu);
            return out.str();
        }

        static std::string upper_copy(std::string text) {
            std::transform(text.begin(), text.end(), text.begin(),
                           [](unsigned char ch) {
                               return static_cast<char>(std::toupper(ch));
                           });
            return text;
        }

        static std::string format_file_argument(
            const std::filesystem::path& path)
        {
            const std::string normalized =
                debug_info_builder::normalize_path_string(path);
            if (normalized.find_first_of(" \t") == std::string::npos)
                return normalized;
            return "\"" + normalized + "\"";
        }

        static std::optional<std::string> noice_type_expression(
            const std::string& type_name)
        {
            auto trim = [](std::string value) {
                auto start = value.find_first_not_of(" \t\r\n");
                if (start == std::string::npos)
                    return std::string();
                auto end = value.find_last_not_of(" \t\r\n");
                return value.substr(start, end - start + 1);
            };

            std::string text = trim(type_name);
            if (text.empty() || text == "void")
                return std::nullopt;

            auto open = text.rfind('[');
            if (open != std::string::npos && text.back() == ']') {
                auto base = trim(text.substr(0, open));
                auto count = text.substr(open + 1, text.size() - open - 2);
                auto mapped = noice_type_expression(base);
                if (mapped.has_value())
                    return mapped.value() + "[" + count + "]";
                return std::nullopt;
            }

            if (text.back() == '*') {
                auto base = trim(text.substr(0, text.size() - 1));
                auto mapped = noice_type_expression(base);
                if (mapped.has_value() && !mapped->empty() && mapped->front() == '%')
                    return "%*" + mapped->substr(1);
                return std::nullopt;
            }

            if (text == "unsigned char") return "%U8";
            if (text == "char") return "%S8";
            if (text == "unsigned int") return "%U16";
            if (text == "int") return "%S16";

            return std::nullopt;
        }

        static std::optional<std::string> noice_value_expression(
            const adb_symbol_info& symbol)
        {
            switch (symbol.storage) {
            case adb_storage_class::address:
                return std::nullopt;
            case adb_storage_class::frame_relative:
                if (!symbol.offset.has_value())
                    return std::nullopt;
                if (symbol.offset.value() >= 0)
                    return "IX+" + std::to_string(symbol.offset.value());
                return "IX" + std::to_string(symbol.offset.value());
            case adb_storage_class::register_name:
                if (!symbol.register_name.has_value())
                    return std::nullopt;
                return "&" + upper_copy(symbol.register_name.value());
            case adb_storage_class::register_pair:
                if (!symbol.register_name.has_value())
                    return std::nullopt;
                return "&" + upper_copy(symbol.register_name.value());
            case adb_storage_class::unknown:
                return std::nullopt;
            }
            return std::nullopt;
        }

        static std::optional<uint32_t> module_low_address(
            const linked_module_debug_info& info)
        {
            std::optional<uint32_t> lowest;
            if (!info.line_by_address.empty())
                lowest = info.line_by_address.begin()->first;

            for (const auto& [_, function] : info.functions) {
                if (!lowest.has_value() || function.start_address < lowest.value())
                    lowest = function.start_address;
            }
            return lowest;
        }

        static std::optional<uint32_t> module_high_address(
            const linked_module_debug_info& info)
        {
            std::optional<uint32_t> highest;
            if (!info.line_by_address.empty())
                highest = info.line_by_address.rbegin()->first;

            for (const auto& [_, function] : info.functions) {
                uint32_t end = function.end_address > function.start_address
                    ? function.end_address - 1
                    : function.start_address;
                if (!highest.has_value() || end > highest.value())
                    highest = end;
            }
            return highest;
        }

        static bool global_symbol_exists(const link_context& ctx,
                                         const std::string& name) {
            return ctx.global_symbols.find(name) != ctx.global_symbols.end()
                || ctx.linker_symbols.find(name) != ctx.linker_symbols.end();
        }

        static void emit_def(std::ostream& out,
                             const std::string& name,
                             uint32_t address)
        {
            out << "DEF " << name << " " << hex16(address) << "\n";
        }

        static void emit_deferred_type(std::ostream& out,
                                       const std::optional<std::string>& type_expr)
        {
            if (type_expr.has_value())
                out << " " << type_expr.value();
            out << "\n";
        }

        static void emit_scoped_address_symbol(
            std::ostream& out,
            const std::string& name,
            uint32_t address,
            const std::optional<std::string>& type_expr)
        {
            out << "DEFSCOPE " << name << " " << hex16(address);
            emit_deferred_type(out, type_expr);
        }

        static void emit_scoped_value_symbol(
            std::ostream& out,
            const std::string& name,
            const std::string& value_expr,
            const std::optional<std::string>& type_expr)
        {
            out << "DEFSCOPE " << name << " " << value_expr;
            emit_deferred_type(out, type_expr);
        }

    } // namespace

    void noice_emitter::emit(const std::filesystem::path& path,
                             const std::filesystem::path& image_path,
                             const link_context& ctx) const
    {
        std::ofstream out(path);
        if (!out.is_open())
            throw xlink::xlink_error("cannot open NoICE output file: "
                                     + path.string());

        auto debug = debug_info_builder::build(image_path, ctx);

        out << "LASTFILELOADED\n";
        out << "CLEARLINEINFO Y\n";

        std::vector<std::pair<std::string, uint32_t>> globals;
        globals.reserve(ctx.global_symbols.size() + ctx.linker_symbols.size());
        for (const auto& [name, where] : ctx.global_symbols) {
            const auto* mod = where.first;
            int idx = where.second;
            const auto& sym = mod->symbol_by_index(idx);
            globals.push_back({name, debug_info_builder::symbol_absolute_addr(mod, sym)});
        }
        for (const auto& [name, value] : ctx.linker_symbols)
            globals.push_back({name, value});

        std::sort(globals.begin(), globals.end(),
                  [](const auto& a, const auto& b) {
                      if (a.second != b.second)
                          return a.second < b.second;
                      return a.first < b.first;
                  });

        for (const auto& [name, address] : globals)
            emit_def(out, name, address);

        std::set<std::string> emitted_scoped_keys;

        for (const auto& info : debug.modules) {
            auto base = module_low_address(info);
            auto end = module_high_address(info);

            if (!base.has_value() && info.functions.empty())
                continue;

            if (!base.has_value())
                base = 0;

            out << "FILE " << format_file_argument(info.source_path)
                << " " << hex16(base.value()) << "\n";

            for (const auto& [address, line] : info.line_by_address) {
                out << "LINE " << line << " "
                    << hex16(address - base.value()) << "\n";
            }

            if (info.adb.has_value()) {
                for (const auto& global : info.adb->globals) {
                    if (!global.file_local)
                        continue;

                    auto address = debug_info_builder::find_symbol_address(
                        ctx, info.mod, global.raw_name);
                    if (!address.has_value())
                        continue;

                    const std::string key =
                        global.display_name + "@" + std::to_string(address.value());
                    if (!emitted_scoped_keys.insert(key).second)
                        continue;

                    emit_scoped_address_symbol(
                        out,
                        global.display_name,
                        address.value(),
                        noice_type_expression(global.type.name));
                }
            }

            std::vector<const debug_function_info*> functions;
            functions.reserve(info.functions.size());
            for (const auto& [_, function] : info.functions)
                functions.push_back(&function);
            std::sort(functions.begin(), functions.end(),
                      [](const auto* a, const auto* b) {
                          if (a->start_address != b->start_address)
                              return a->start_address < b->start_address;
                          return a->display_name < b->display_name;
                      });

            for (const auto* function : functions) {
                out << (function->file_local ? "STATICFUNCTION " : "FUNCTION ")
                    << function->display_name << " "
                    << hex16(function->start_address) << "\n";

                if (info.adb.has_value()) {
                    for (const auto& local : info.adb->locals) {
                        if (!local.parent_name.has_value()
                            || local.parent_name.value() != function->display_name) {
                            continue;
                        }

                        auto type_expr = noice_type_expression(local.type.name);
                        auto value_expr = noice_value_expression(local);
                        if (local.storage == adb_storage_class::address) {
                            auto address = debug_info_builder::find_symbol_address(
                                ctx, info.mod, local.raw_name);
                            if (!address.has_value())
                                continue;
                            emit_scoped_address_symbol(
                                out, local.display_name, address.value(), type_expr);
                        } else if (value_expr.has_value()) {
                            emit_scoped_value_symbol(
                                out, local.display_name, value_expr.value(), type_expr);
                        }
                    }
                }

                uint32_t highest = function->end_address > function->start_address
                    ? function->end_address - 1
                    : function->start_address;
                out << "ENDFUNCTION " << hex16(highest) << "\n";
            }

            for (const auto& sym : info.mod->symbols()) {
                if (!sym.is_def())
                    continue;
                if (global_symbol_exists(ctx, sym.name()))
                    continue;
                if (is_internal_symbol_name(sym.name(), info.mod->name()))
                    continue;

                bool is_function_symbol = false;
                for (const auto* function : functions) {
                    if (function->display_name == sym.name()
                        || function->fallback_name == sym.name()) {
                        is_function_symbol = true;
                        break;
                    }
                }
                if (is_function_symbol)
                    continue;

                const auto address =
                    debug_info_builder::symbol_absolute_addr(info.mod, sym);
                const std::string key =
                    sym.name() + "@" + std::to_string(address);
                if (!emitted_scoped_keys.insert(key).second)
                    continue;

                emit_scoped_address_symbol(out, sym.name(), address, std::nullopt);
            }

            if (end.has_value())
                out << "ENDFILE " << hex16(end.value()) << "\n";
            else
                out << "ENDFILE\n";
        }
    }

} // namespace xlink
