// main.cpp
//
// xlink linker main
//
// MIT License (see: LICENSE)
// copyright (C) 2021 tomaz stih
//
// 2021-07-28   tstih
#include <iostream>

#include <xlink/cli.hpp>
#include <xlink/linker.hpp>
#include <xlink/binary_emitter.hpp>
#include <xlink/errors.hpp>

#define XLINK_VERSION "1.0.0"

int main(int argc, char* argv[]) {
    try {
        auto opts = xlink::cli::parse(argc, argv);

        if (opts.show_help) {
            xlink::cli::print_usage();
            return 0;
        }

        xlink::link_context ctx;
        ctx.entry_name = opts.entry_symbol;
        ctx.holes = opts.reserved_ranges;
        ctx.verbose = opts.verbose;
        ctx.print_map = opts.print_map;

        if (ctx.verbose)
            std::cout << "xlink " << XLINK_VERSION << "\n";

        xlink::linker::link(ctx, opts);
        xlink::binary_emitter::emit(opts.output_file, ctx);

        if (ctx.verbose) {
            std::cout << "Output: " << opts.output_file << " ("
                      << (12 + ctx.reloc_table.size() * 4
                          + ctx.code_size)
                      << " bytes)\n";
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
