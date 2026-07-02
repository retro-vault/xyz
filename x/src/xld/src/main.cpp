// main.cpp
//
// xld linker main
//
// MIT License (see: LICENSE)
// copyright (C) 2021 tomaz stih
//
// 2021-07-28   tstih
#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>

#include <xld/cdb_emitter.h>
#include <xld/cli.h>
#include <xld/linker.h>
#include <xld/binary_emitter.h>
#include <xld/debug_emitter.h>
#include <xld/elf_debug_emitter.h>
#include <xld/errors.h>
#include <xld/map_emitter.h>
#include <xld/runtime.h>

#ifndef XLD_VERSION
#define XLD_VERSION "0.1.0"
#endif

static std::filesystem::path replace_extension(const std::filesystem::path& path,
                                               const std::string& ext)
{
    auto out = path;
    out.replace_extension(ext);
    return out;
}

int main(int argc, char* argv[]) {
    try {
        auto opts = xld::cli::parse(argc, argv);

        if (opts.show_help) {
            xld::cli::print_usage(argv[0]);
            return 0;
        }
        if (opts.show_version) {
            std::cout << "xld " << XLD_VERSION
                      << " (X Tools Linker for Z80)\n";
            return 0;
        }

        if (opts.mode == xld::link_mode::sdcc)
            xld::runtime::apply_sdcc_runtime(opts);
        xld::cli::resolve_libraries(opts);

        xld::link_context ctx;
        ctx.entry_name = opts.entry_symbol;
        ctx.holes = opts.reserved_ranges;
        ctx.area_bases = opts.area_bases;
        ctx.area_order = opts.area_order;
        ctx.output_range = opts.output_range;
        ctx.format = opts.format;
        ctx.verbose = opts.verbose;
        ctx.print_map = opts.print_map;

        if (ctx.verbose)
            std::cout << "xld " << XLD_VERSION << "\n";

        xld::linker::link(ctx, opts);
        xld::binary_emitter::emit(opts.output_file, ctx);

        if (opts.print_map) {
            xld::map_emitter::emit(std::cout, ctx);
        }
        if (opts.map_file.has_value()) {
            xld::map_emitter::emit(opts.map_file.value(), ctx);
        }

        if (opts.debug_info) {
            opts.cdb_file = replace_extension(
                opts.output_file,
                opts.mode == xld::link_mode::gnu ? ".elf" : ".cdb");
        }

        if (opts.cdb_file.has_value()) {
            if (opts.mode == xld::link_mode::gnu) {
                xld::elf_debug_emitter emitter;
                const xld::debug_emitter& debug = emitter;
                debug.emit(opts.cdb_file.value(), opts.output_file, ctx);
            } else {
                xld::cdb_emitter emitter;
                const xld::debug_emitter& debug = emitter;
                debug.emit(opts.cdb_file.value(), opts.output_file, ctx);
            }
        }

        if (ctx.verbose) {
            uint32_t output_size = 0;
            if (ctx.format == xld::output_format::xl) {
                output_size = 12
                            + static_cast<uint32_t>(ctx.reloc_table.size()) * 4
                            + ctx.code_size;
            } else if (ctx.output_range.has_value()) {
                output_size = static_cast<uint32_t>(
                    ctx.output_range->end - ctx.output_range->start + 1);
            } else {
                output_size = ctx.code_size > 0x10000u
                    ? 0x10000u
                    : ctx.code_size;
            }
            std::cout << "Output: " << opts.output_file << " ("
                      << output_size << " bytes)\n";
        }

        return 0;
    } catch (const xld::xld_error& e) {
        std::cerr << "xld: error: " << e.what() << "\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "xld: internal error: " << e.what() << "\n";
        return 2;
    }
}
