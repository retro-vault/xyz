//
// linked debug metadata builder
//
// MIT License (see: LICENSE)
// copyright (C) 2021 tomaz stih
//
#include <algorithm>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include <xld/adb_parser.h>
#include <xld/debug_info.h>
#include <xld/errors.h>
#include <xld/lst_parser.h>

namespace xld {

    namespace {

        struct c_line_marker {
            std::string file_name;
            uint32_t line = 0;
            uint32_t address = 0;
        };

        struct function_range {
            std::string display_name;
            std::string fallback_name;
            uint32_t start_address = 0;
            std::optional<uint32_t> end_address;
        };

        struct parsed_function_marker {
            std::string display_name;
            std::string fallback_name;
            bool end_marker = false;
        };

        static bool starts_with(const std::string& s, const std::string& prefix) {
            return s.rfind(prefix, 0) == 0;
        }

        static std::optional<std::filesystem::path> sidecar_path(
            const module& mod, const char* extension)
        {
            auto text = mod.path().string();
            if (text.find('[') != std::string::npos
                && text.find(']') != std::string::npos) {
                return std::nullopt;
            }

            auto path = mod.path();
            if (path.extension() != ".rel")
                return std::nullopt;

            path.replace_extension(extension);
            if (!std::filesystem::exists(path))
                return std::nullopt;
            return path;
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

        static std::optional<parsed_function_marker> parse_function_marker(
            const module& mod, const std::string& name)
        {
            parsed_function_marker result;
            const std::string local_start = "F" + mod.name() + "$";
            const std::string local_end = "XF" + mod.name() + "$";

            auto parse_tail = [](const std::string& tail)
                -> std::optional<std::string>
            {
                auto first = tail.find('$');
                if (first == std::string::npos)
                    return std::nullopt;
                auto second = tail.find('$', first + 1);
                if (second == std::string::npos)
                    return std::nullopt;
                auto a = tail.substr(first + 1, second - first - 1);
                auto b = tail.substr(second + 1);
                if (a.empty() || b.empty()
                    || !std::all_of(a.begin(), a.end(), ::isdigit)
                    || !std::all_of(b.begin(), b.end(), ::isdigit)) {
                    return std::nullopt;
                }
                return tail.substr(0, first);
            };

            if (starts_with(name, "G$") || starts_with(name, "XG$")) {
                bool end = starts_with(name, "XG$");
                std::string tail = name.substr(end ? 3 : 2);
                auto parsed = parse_tail(tail);
                if (!parsed.has_value())
                    return std::nullopt;
                result.display_name = "_" + parsed.value();
                result.fallback_name = name;
                result.end_marker = end;
                return result;
            }

            if (starts_with(name, local_start) || starts_with(name, local_end)) {
                bool end = starts_with(name, local_end);
                std::string tail = name.substr((end ? local_end : local_start).size());
                auto parsed = parse_tail(tail);
                if (!parsed.has_value())
                    return std::nullopt;
                result.display_name = parsed.value();
                result.fallback_name = name;
                result.end_marker = end;
                return result;
            }

            return std::nullopt;
        }

        static std::optional<c_line_marker> parse_c_line_symbol(
            const std::string& name, uint32_t address)
        {
            if (!starts_with(name, "C$"))
                return std::nullopt;

            auto first = name.find('$', 2);
            if (first == std::string::npos)
                return std::nullopt;
            auto second = name.find('$', first + 1);
            if (second == std::string::npos)
                return std::nullopt;

            c_line_marker marker;
            marker.file_name = name.substr(2, first - 2);
            marker.line = static_cast<uint32_t>(
                std::stoul(name.substr(first + 1, second - first - 1),
                           nullptr, 10));
            marker.address = address;
            return marker;
        }

        static uint32_t area_end_address(const module& mod, int area_index) {
            if (area_index < 0 || area_index >= static_cast<int>(mod.areas().size()))
                return 0;
            const auto& area = mod.areas()[area_index];
            if (!area.placed_addr().has_value())
                return 0;
            return static_cast<uint32_t>(
                area.placed_addr().value() + area.size());
        }

        static std::optional<uint32_t> find_line_for_address(
            const std::map<uint32_t, uint32_t>& lines, uint32_t address)
        {
            auto it = lines.find(address);
            if (it != lines.end())
                return it->second;
            return std::nullopt;
        }

        class source_path_resolver {
        public:
            source_path_resolver()
                : workspace_root_(std::filesystem::current_path()) {}

            std::filesystem::path resolve(const module& mod,
                                          const std::string& basename) {
                if (basename.empty())
                    return basename;

                std::filesystem::path direct(basename);
                if (direct.is_absolute() && std::filesystem::exists(direct))
                    return direct;
                if (std::filesystem::exists(direct))
                    return direct;

                auto sibling = mod.path().parent_path() / basename;
                if (std::filesystem::exists(sibling))
                    return sibling;

                auto cached = cache_.find(basename);
                if (cached != cache_.end())
                    return cached->second.value_or(std::filesystem::path(basename));

                std::vector<std::filesystem::path> matches;
                std::error_code ec;
                for (auto it = std::filesystem::recursive_directory_iterator(
                         workspace_root_,
                         std::filesystem::directory_options::skip_permission_denied,
                         ec);
                     !ec && it != std::filesystem::recursive_directory_iterator();
                     it.increment(ec)) {
                    if (ec) break;
                    if (!it->is_regular_file())
                        continue;
                    if (it->path().filename() == basename)
                        matches.push_back(it->path());
                }

                if (!matches.empty()) {
                    std::sort(matches.begin(), matches.end(),
                              [](const auto& a, const auto& b) {
                                  const auto as = a.string();
                                  const auto bs = b.string();
                                  const bool a_build = as.find("/build/") != std::string::npos;
                                  const bool b_build = bs.find("/build/") != std::string::npos;
                                  if (a_build != b_build)
                                      return !a_build;
                                  return as < bs;
                              });
                    cache_[basename] = matches.front();
                    return matches.front();
                }

                cache_[basename] = std::nullopt;
                return basename;
            }

        private:
            std::filesystem::path workspace_root_;
            std::map<std::string, std::optional<std::filesystem::path>> cache_;
        };

        static void finalize_function_ranges(
            const module& mod,
            std::map<std::string, function_range>& functions)
        {
            std::vector<function_range*> ordered;
            for (auto& [_, fn] : functions)
                ordered.push_back(&fn);

            std::sort(ordered.begin(), ordered.end(),
                      [](const auto* a, const auto* b) {
                          if (a->start_address != b->start_address)
                              return a->start_address < b->start_address;
                          return a->display_name < b->display_name;
                      });

            for (std::size_t i = 0; i < ordered.size(); ++i) {
                if (ordered[i]->end_address.has_value())
                    continue;

                uint32_t limit = 0;
                for (int sym_index = 0;
                     sym_index < static_cast<int>(mod.symbols().size());
                     ++sym_index) {
                    const auto& sym = mod.symbols()[sym_index];
                    if (!sym.is_def())
                        continue;
                    if (debug_info_builder::symbol_absolute_addr(&mod, sym)
                        != ordered[i]->start_address) {
                        continue;
                    }
                    limit = area_end_address(mod, sym.area_index());
                    break;
                }

                uint32_t next_start = limit;
                for (std::size_t j = i + 1; j < ordered.size(); ++j) {
                    if (ordered[j]->start_address > ordered[i]->start_address) {
                        next_start = ordered[j]->start_address;
                        break;
                    }
                }

                if (next_start > ordered[i]->start_address)
                    ordered[i]->end_address = next_start;
                else
                    ordered[i]->end_address =
                        ordered[i]->start_address + 1;
            }
        }

        static void build_function_ranges(
            const module& mod,
            debug_language& language,
            std::map<std::string, function_range>& functions)
        {
            for (const auto& sym : mod.symbols()) {
                if (!sym.is_def())
                    continue;

                auto parsed = parse_function_marker(mod, sym.name());
                if (!parsed.has_value())
                    continue;

                auto& fn = functions[parsed->display_name];
                fn.display_name = parsed->display_name;
                fn.fallback_name = parsed->fallback_name;
                if (parsed->end_marker) {
                    fn.end_address = debug_info_builder::symbol_absolute_addr(&mod, sym);
                } else {
                    fn.start_address = debug_info_builder::symbol_absolute_addr(&mod, sym);
                }
            }

            if (!functions.empty()) {
                if (language == debug_language::unknown)
                    language = debug_language::c;
                finalize_function_ranges(mod, functions);
                return;
            }

            struct code_symbol {
                std::string name;
                uint32_t address = 0;
                uint32_t area_end = 0;
            };
            std::vector<code_symbol> code_symbols;
            for (const auto& sym : mod.symbols()) {
                if (!sym.is_def())
                    continue;
                if (sym.name().empty() || sym.name()[0] != '_')
                    continue;
                if (is_internal_symbol_name(sym.name(), mod.name()))
                    continue;
                int area_index = sym.area_index();
                if (area_index < 0
                    || area_index >= static_cast<int>(mod.areas().size())) {
                    continue;
                }
                const auto& area = mod.areas()[area_index];
                if (!is_code_area_name(area.name()))
                    continue;

                code_symbol code;
                code.name = sym.name();
                code.address = debug_info_builder::symbol_absolute_addr(&mod, sym);
                code.area_end = area_end_address(mod, area_index);
                code_symbols.push_back(std::move(code));
            }

            std::sort(code_symbols.begin(), code_symbols.end(),
                      [](const auto& a, const auto& b) {
                          if (a.address != b.address)
                              return a.address < b.address;
                          return a.name < b.name;
                      });

            for (std::size_t i = 0; i < code_symbols.size(); ++i) {
                function_range fn;
                fn.display_name = code_symbols[i].name;
                fn.fallback_name = code_symbols[i].name;
                fn.start_address = code_symbols[i].address;
                uint32_t end = code_symbols[i].area_end;
                if (i + 1 < code_symbols.size()
                    && code_symbols[i + 1].address > fn.start_address) {
                    end = std::min(end, code_symbols[i + 1].address);
                }
                fn.end_address = (end > fn.start_address)
                    ? std::optional<uint32_t>(end)
                    : std::optional<uint32_t>(fn.start_address + 1);
                functions[fn.display_name] = fn;
            }

            if (!functions.empty() && language == debug_language::unknown)
                language = debug_language::assembly;
        }

        static void collect_assembly_lines(
            const module& mod,
            debug_language& language,
            std::map<uint32_t, uint32_t>& lines)
        {
            auto lst = sidecar_path(mod, ".lst");
            if (!lst.has_value())
                return;

            auto entries = lst_parser::parse(lst.value());
            if (entries.empty())
                return;

            std::map<std::string, uint32_t> bases;
            for (const auto& area : mod.areas()) {
                if (area.placed_addr().has_value())
                    bases[area.name()] = area.placed_addr().value();
            }

            for (const auto& entry : entries) {
                auto base_it = bases.find(entry.area_name);
                if (base_it == bases.end())
                    continue;
                uint32_t absolute = base_it->second + entry.offset;
                lines.emplace(absolute, entry.line);
            }

            if (!lines.empty())
                language = debug_language::assembly;
        }

        static std::filesystem::path module_source_path(
            source_path_resolver& resolver,
            const module& mod,
            debug_language language)
        {
            if (language == debug_language::c) {
                std::string basename = mod.name() + ".c";
                for (const auto& sym : mod.symbols()) {
                    if (!sym.is_def())
                        continue;
                    auto parsed = parse_c_line_symbol(
                        sym.name(),
                        debug_info_builder::symbol_absolute_addr(&mod, sym));
                    if (parsed.has_value()) {
                        basename = parsed->file_name;
                        break;
                    }
                }
                return resolver.resolve(mod, basename);
            }

            return resolver.resolve(mod, mod.path().stem().string() + ".s");
        }

        static std::optional<adb_function_info> find_adb_function(
            const adb_document& doc, const std::string& display_name)
        {
            for (const auto& fn : doc.functions) {
                if (fn.display_name == display_name)
                    return fn;
            }
            return std::nullopt;
        }

        static std::optional<xbfd::debug_function> find_cdb_function(
            const xbfd::debug_info& info, const std::string& display_name)
        {
            auto canonical = [](const std::string& name) {
                if (!name.empty() && name[0] == '_')
                    return name.substr(1);
                return name;
            };

            const auto want = canonical(display_name);
            for (const auto& fn : info.functions) {
                if (fn.name == display_name || canonical(fn.name) == want)
                    return fn;
            }
            return std::nullopt;
        }

    } // namespace

    uint32_t debug_info_builder::symbol_absolute_addr(const module* mod,
                                                      const symbol& sym) {
        uint16_t addr = sym.value();
        int area_idx = sym.area_index();
        if (area_idx >= 0 && area_idx < static_cast<int>(mod->areas().size())) {
            auto& area = mod->areas()[area_idx];
            if (area.placed_addr().has_value())
                addr = static_cast<uint16_t>(addr + area.placed_addr().value());
        } else if (!sym.is_absolute()
                   && !mod->areas().empty()
                   && mod->areas()[0].placed_addr().has_value()) {
            addr = static_cast<uint16_t>(addr + mod->areas()[0].placed_addr().value());
        }
        return addr;
    }

    std::optional<uint32_t> debug_info_builder::find_symbol_address(
        const link_context& ctx, const std::string& name)
    {
        return find_symbol_address(ctx, nullptr, name);
    }

    std::optional<uint32_t> debug_info_builder::find_symbol_address(
        const link_context& ctx,
        const module* preferred_module,
        const std::string& name)
    {
        if (preferred_module != nullptr) {
            for (const auto& sym : preferred_module->symbols()) {
                if (sym.is_def() && sym.name() == name)
                    return symbol_absolute_addr(preferred_module, sym);
            }
        }

        auto git = ctx.global_symbols.find(name);
        if (git != ctx.global_symbols.end()) {
            const auto* mod = git->second.first;
            int idx = git->second.second;
            return symbol_absolute_addr(mod, mod->symbol_by_index(idx));
        }

        auto lit = ctx.linker_symbols.find(name);
        if (lit != ctx.linker_symbols.end())
            return lit->second;

        return std::nullopt;
    }

    std::string debug_info_builder::normalize_path_string(
        const std::filesystem::path& path)
    {
        return std::filesystem::absolute(path).lexically_normal().string();
    }

    linked_debug_info debug_info_builder::build(
        const std::filesystem::path& image_path, const link_context& ctx)
    {
        linked_debug_info result;
        result.image_path = std::filesystem::absolute(image_path)
            .lexically_normal();
        result.entry_address = ctx.entry_point;

        source_path_resolver resolver;
        result.modules.reserve(ctx.modules.size());

        for (const auto& mod : ctx.modules) {
            linked_module_debug_info info;
            info.mod = mod.get();

            if (auto adb = sidecar_path(*mod, ".adb"); adb.has_value()) {
                info.adb = adb_parser::parse(adb.value());
                info.language = debug_language::c;
            }
            if (auto cdb = sidecar_path(*mod, ".cdb"); cdb.has_value()) {
                info.cdb = xbfd::debug_reader::read_cdb(cdb->string());
                info.language = debug_language::c;
            }

            for (const auto& sym : mod->symbols()) {
                if (!sym.is_def())
                    continue;
                auto parsed = parse_c_line_symbol(
                    sym.name(), symbol_absolute_addr(mod.get(), sym));
                if (parsed.has_value()) {
                    info.language = debug_language::c;
                    info.line_by_address.emplace(parsed->address, parsed->line);
                }
            }

            std::map<std::string, function_range> ranges;
            build_function_ranges(*mod, info.language, ranges);
            if (info.language != debug_language::c)
                collect_assembly_lines(*mod, info.language, info.line_by_address);

            info.source_path = module_source_path(resolver, *mod, info.language);

            for (auto& [name, range] : ranges) {
                debug_function_info fn;
                fn.display_name = range.display_name;
                fn.fallback_name = range.fallback_name;
                fn.start_address = range.start_address;
                fn.end_address = range.end_address.value_or(range.start_address + 1);
                fn.line = find_line_for_address(info.line_by_address, fn.start_address);
                if (info.adb.has_value()) {
                    auto adb_fn = find_adb_function(info.adb.value(), name);
                    if (adb_fn.has_value()) {
                        fn.file_local = adb_fn->file_local;
                        if (!adb_fn->return_type.name.empty())
                            fn.return_type = adb_fn->return_type.name;
                        fn.calling_convention = adb_fn->calling_convention;
                    }
                }
                if (fn.calling_convention == xbfd::calling_convention::unknown
                    && info.cdb.has_value()) {
                    auto cdb_fn = find_cdb_function(info.cdb.value(), name);
                    if (cdb_fn.has_value())
                        fn.calling_convention = cdb_fn->convention;
                }
                info.functions[name] = std::move(fn);
            }

            result.modules.push_back(std::move(info));
        }

        return result;
    }

} // namespace xld
