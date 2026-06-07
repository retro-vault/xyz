// linker.cpp
//
// pipeline orchestrator
//
// MIT License (see: LICENSE)
// copyright (C) 2021 tomaz stih
//
// 2021-07-28   tstih
#include <algorithm>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>

#include <xld/linker.h>
#include <xld/rel_parser.h>
#include <xld/lib_parser.h>
#include <xld/area_placer.h>
#include <xld/branch_relaxer.h>
#include <xld/relocator.h>
#include <xld/errors.h>

namespace xld {

    static bool is_linker_generated_symbol(const std::string& name) {
        return (name.rfind("s__", 0) == 0 || name.rfind("l__", 0) == 0);
    }

    void linker::link(link_context& ctx, const cli_options& opts) {
        load_inputs(ctx, opts);
        resolve_libraries(ctx, opts);
        resolve_symbols(ctx);
        relax_branches(ctx);
        place_areas(ctx);
        define_linker_symbols(ctx);
        relocate(ctx);
        find_entry_point(ctx);
    }

    void linker::load_inputs(link_context& ctx, const cli_options& opts) {
        for (auto& path : opts.input_files) {
            std::string ext = path.extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            if (ext == ".rel") {
                if (ctx.verbose)
                    std::cout << "Loading " << path << "\n";
                auto mod = rel_parser::parse(path);
                ctx.modules.push_back(mod);
            }
            // .lib files are handled in resolve_libraries.
        }
    }

    void linker::resolve_libraries(link_context& ctx,
                                   const cli_options& opts)
    {
        // Collect library .rel paths and pre-scan their defs.
        struct lib_module_info {
            std::filesystem::path path;
            std::optional<std::string> contents;
            std::vector<std::string> defs;
            bool loaded = false;
        };

        std::vector<lib_module_info> lib_modules;

        for (auto& path : opts.input_files) {
            std::string ext = path.extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            if (ext == ".lib") {
                if (ctx.verbose)
                    std::cout << "Scanning library " << path << "\n";
                auto members = lib_parser::parse(path);
                for (auto& member : members) {
                    lib_module_info info;
                    info.path = member.path;
                    info.contents = member.contents;
                    if (info.contents.has_value()) {
                        std::istringstream input(info.contents.value());
                        info.defs = rel_parser::scan_defs(
                            info.path.string(), input);
                    } else {
                        info.defs = rel_parser::scan_defs(info.path);
                    }
                    lib_modules.push_back(info);
                }
            }
        }

        if (lib_modules.empty()) return;

        // Iterative resolution: keep pulling in library modules
        // until no new undefined symbols can be satisfied.
        bool changed = true;
        while (changed) {
            changed = false;

            // Collect all undefined symbols from loaded modules.
            std::set<std::string> undefined;
            std::set<std::string> defined;

            for (auto& mod : ctx.modules) {
                for (auto& sym : mod->symbols()) {
                    if (sym.is_def())
                        defined.insert(sym.name());
                }
            }
            for (auto& mod : ctx.modules) {
                for (auto& sym : mod->symbols()) {
                    if (sym.is_ref() &&
                        defined.find(sym.name()) == defined.end())
                        undefined.insert(sym.name());
                }
            }

            // Check if any library module satisfies an undefined symbol.
            for (auto& lm : lib_modules) {
                if (lm.loaded) continue;

                for (auto& def_name : lm.defs) {
                    if (undefined.find(def_name) != undefined.end()) {
                        // Load this module.
                        if (ctx.verbose)
                            std::cout << "Including library module "
                                      << lm.path << " (provides "
                                      << def_name << ")\n";
                        std::shared_ptr<module> mod;
                        if (lm.contents.has_value()) {
                            std::istringstream input(lm.contents.value());
                            mod = rel_parser::parse(lm.path.string(), input);
                        } else {
                            mod = rel_parser::parse(lm.path);
                        }
                        ctx.modules.push_back(mod);
                        lm.loaded = true;
                        changed = true;
                        break;
                    }
                }
            }
        }
    }

    void linker::resolve_symbols(link_context& ctx) {
        // Build global definition table.
        for (auto& mod : ctx.modules) {
            for (int i = 0;
                 i < static_cast<int>(mod->symbols().size()); ++i) {
                auto& sym = mod->symbols()[i];
                if (!sym.is_def()) continue;

                // Skip SDCC internal pseudo-symbols (e.g. .__.ABS.).
                // These are defined in every module and must not enter
                // the global table.
                const auto& sname = sym.name();
                if (sname.size() > 3 && sname[0] == '.' &&
                    sname[1] == '_' && sname[2] == '_')
                    continue;

                auto it = ctx.global_symbols.find(sname);
                if (it != ctx.global_symbols.end()) {
                    throw symbol_error(
                        "duplicate symbol '" + sname
                        + "' defined in modules '"
                        + it->second.first->name() + "' and '"
                        + mod->name() + "'");
                }
                ctx.global_symbols[sname] = {mod.get(), i};
            }
        }

        // Check for unresolved references.
        for (auto& mod : ctx.modules) {
            for (auto& sym : mod->symbols()) {
                if (!sym.is_ref()) continue;
                if (ctx.global_symbols.find(sym.name()) ==
                        ctx.global_symbols.end()
                    && !is_linker_generated_symbol(sym.name())) {
                    throw symbol_error(
                        "unresolved symbol '" + sym.name()
                        + "' referenced in module '" + mod->name() + "'");
                }
            }
        }

        if (ctx.verbose) {
            std::cout << "Resolved " << ctx.global_symbols.size()
                      << " global symbols\n";
        }
    }

    void linker::place_areas(link_context& ctx) {
        area_placer::place(ctx);

        if (ctx.verbose) {
            std::cout << "Code size: 0x" << std::hex << ctx.code_size
                      << std::dec << " (" << ctx.code_size << " bytes)\n";
        }
    }

    void linker::relax_branches(link_context& ctx) {
        branch_relaxer::relax(ctx);
    }

    void linker::define_linker_symbols(link_context& ctx) {
        struct area_span {
            uint16_t start = 0xFFFF;
            uint16_t end = 0;
            bool has_data = false;
        };

        std::map<std::string, area_span> spans;
        for (auto& mod : ctx.modules) {
            for (auto& area : mod->areas()) {
                if (!area.placed_addr().has_value())
                    continue;
                if (!area.name().empty() && area.name()[0] == '.')
                    continue; // skip pseudo/internal areas (.ABS.)

                auto& span = spans[area.name()];
                uint16_t start = area.placed_addr().value();
                uint16_t end = static_cast<uint16_t>(start + area.size());
                if (!span.has_data || start < span.start) span.start = start;
                if (!span.has_data || end > span.end) span.end = end;
                span.has_data = true;
            }
        }

        for (const auto& [area_name, span] : spans) {
            if (!span.has_data)
                continue;

            std::string suffix = area_name;
            if (!suffix.empty() && suffix[0] == '_')
                suffix.erase(0, 1);

            std::string s_name = "s__" + suffix;
            std::string l_name = "l__" + suffix;
            ctx.linker_symbols[s_name] = span.start;
            ctx.linker_symbols[l_name] =
                static_cast<uint16_t>(span.end - span.start);
        }
    }

    void linker::relocate(link_context& ctx) {
        relocator::relocate(ctx);

        if (ctx.verbose) {
            std::cout << "Generated " << ctx.reloc_table.size()
                      << " relocation entries\n";
        }
    }

    void linker::find_entry_point(link_context& ctx) {
        auto it = ctx.global_symbols.find(ctx.entry_name);
        if (it == ctx.global_symbols.end()) {
            throw symbol_error(
                "entry point symbol '" + ctx.entry_name + "' not found");
        }

        auto [mod, idx] = it->second;
        auto& sym = mod->symbol_by_index(idx);
        ctx.entry_point = sym.value();

        int entry_area_idx = sym.area_index();
        if (entry_area_idx >= 0
            && entry_area_idx < static_cast<int>(mod->areas().size())) {
            auto& area = mod->area_by_index(entry_area_idx);
            if (area.placed_addr().has_value())
                ctx.entry_point += area.placed_addr().value();
        } else if (!mod->areas().empty()
            && mod->areas()[0].placed_addr().has_value()) {
            // Fallback for objects without symbol->area metadata.
            ctx.entry_point += mod->areas()[0].placed_addr().value();
        }

        if (ctx.verbose) {
            std::cout << "Entry point: " << ctx.entry_name
                      << " at 0x" << std::hex << ctx.entry_point
                      << std::dec << "\n";
        }
    }

} // namespace xld
