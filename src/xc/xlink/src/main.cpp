// main.cpp
//
// xlink linker main
//
// MIT License (see: LICENSE)
// copyright (C) 2021 tomaz stih
//
// 2021-07-28   tstih
#include <iostream>
#include <fstream>
#include <iomanip>

#include <xlink/cdb_emitter.hpp>
#include <xlink/cli.hpp>
#include <xlink/linker.hpp>
#include <xlink/binary_emitter.hpp>
#include <xlink/debug_emitter.hpp>
#include <xlink/errors.hpp>
#include <xlink/noice_emitter.hpp>
#include <xlink/runtime.hpp>
#include <xlink/xdbg_emitter.hpp>

#define XLINK_VERSION "1.0.0"

int main(int argc, char* argv[]) {
    try {
        auto opts = xlink::cli::parse(argc, argv);

        if (opts.show_help) {
            xlink::cli::print_usage();
            return 0;
        }

        xlink::runtime::apply_sdcc_runtime(opts);

        xlink::link_context ctx;
        ctx.entry_name = opts.entry_symbol;
        ctx.holes = opts.reserved_ranges;
        ctx.area_bases = opts.area_bases;
        ctx.output_range = opts.output_range;
        ctx.format = opts.format;
        ctx.verbose = opts.verbose;
        ctx.print_map = opts.print_map;

        if (ctx.verbose)
            std::cout << "xlink " << XLINK_VERSION << "\n";

        xlink::linker::link(ctx, opts);
        xlink::binary_emitter::emit(opts.output_file, ctx);
        if (opts.symbol_file.has_value()) {
            xlink::noice_emitter emitter;
            const xlink::debug_emitter& debug = emitter;
            debug.emit(opts.symbol_file.value(), opts.output_file, ctx);
        }
        if (opts.xdbg_file.has_value()) {
            xlink::xdbg_emitter emitter;
            const xlink::debug_emitter& debug = emitter;
            debug.emit(opts.xdbg_file.value(), opts.output_file, ctx);
        }
        if (opts.cdb_file.has_value()) {
            xlink::cdb_emitter emitter;
            const xlink::debug_emitter& debug = emitter;
            debug.emit(opts.cdb_file.value(), opts.output_file, ctx);
        }

        if (ctx.verbose) {
            uint32_t output_size = 0;
            if (ctx.format == xlink::output_format::xl) {
                output_size = 12
                            + static_cast<uint32_t>(ctx.reloc_table.size()) * 4
                            + ctx.code_size;
            } else if (ctx.output_range.has_value()) {
                output_size = static_cast<uint32_t>(
                    ctx.output_range->end - ctx.output_range->start + 1);
            } else {
                output_size = ctx.code_size;
            }
            std::cout << "Output: " << opts.output_file << " ("
                      << output_size << " bytes)\n";
        }

        return 0;
    } catch (const xlink::xlink_error& e) {
        std::cerr << "xlink: error: " << e.what() << "\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "xlink: internal error: " << e.what() << "\n";
        return 2;
    }
}
