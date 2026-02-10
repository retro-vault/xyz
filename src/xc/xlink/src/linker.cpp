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
#include <string>

#include <xlink/linker.hpp>
#include <xlink/rel_parser.hpp>
#include <xlink/lib_parser.hpp>
#include <xlink/area_placer.hpp>
#include <xlink/relocator.hpp>
#include <xlink/errors.hpp>

namespace xlink {

    void linker::link(link_context& ctx, const cli_options& opts) {
        load_inputs(ctx, opts);
        resolve_libraries(ctx, opts);
        resolve_symbols(ctx);
        place_areas(ctx);
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
                auto rel_paths = lib_parser::parse(path);
                for (auto& rp : rel_paths) {
                    lib_module_info info;
                    info.path = rp;
                    info.defs = rel_parser::scan_defs(rp);
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
                        auto mod = rel_parser::parse(lm.path);
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

                auto it = ctx.global_symbols.find(sym.name());
                if (it != ctx.global_symbols.end()) {
                    throw symbol_error(
                        "duplicate symbol '" + sym.name()
                        + "' defined in modules '"
                        + it->second.first->name() + "' and '"
                        + mod->name() + "'");
                }
                ctx.global_symbols[sym.name()] = {mod.get(), i};
            }
        }

        // Check for unresolved references.
        for (auto& mod : ctx.modules) {
            for (auto& sym : mod->symbols()) {
                if (!sym.is_ref()) continue;
                if (ctx.global_symbols.find(sym.name()) ==
                    ctx.global_symbols.end()) {
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

        // Add the placed address of the symbol's area.
        if (!mod->areas().empty() &&
            mod->areas()[0].placed_addr().has_value()) {
            ctx.entry_point += mod->areas()[0].placed_addr().value();
        }

        if (ctx.verbose) {
            std::cout << "Entry point: " << ctx.entry_name
                      << " at 0x" << std::hex << ctx.entry_point
                      << std::dec << "\n";
        }
    }

} // namespace xlink
