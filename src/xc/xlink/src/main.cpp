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

#include <xlink/cli.hpp>
#include <xlink/linker.hpp>
#include <xlink/binary_emitter.hpp>
#include <xlink/errors.hpp>

#define XLINK_VERSION "1.0.0"

static uint16_t symbol_absolute_addr(const xlink::module* mod,
                                     const xlink::symbol& sym) {
    uint16_t addr = sym.value();
    int area_idx = sym.area_index();
    if (area_idx >= 0 && area_idx < static_cast<int>(mod->areas().size())) {
        auto& area = mod->areas()[area_idx];
        if (area.placed_addr().has_value())
            addr = static_cast<uint16_t>(addr + area.placed_addr().value());
    } else if (!mod->areas().empty() && mod->areas()[0].placed_addr().has_value()) {
        addr = static_cast<uint16_t>(addr + mod->areas()[0].placed_addr().value());
    }
    return addr;
}

static void emit_symbol_file(const std::filesystem::path& path,
                             const xlink::link_context& ctx) {
    std::ofstream out(path);
    if (!out.is_open())
        throw xlink::xlink_error("cannot open symbol output file: "
                                 + path.string());

    for (const auto& [name, where] : ctx.global_symbols) {
        const auto* mod = where.first;
        int idx = where.second;
        const auto& sym = mod->symbol_by_index(idx);
        uint16_t addr = symbol_absolute_addr(mod, sym);
        out << "DEF " << name << " 0x"
            << std::uppercase << std::hex
            << std::setw(4) << std::setfill('0') << addr
            << std::nouppercase << std::dec << "\n";
    }
    for (const auto& [name, addr] : ctx.linker_symbols) {
        out << "DEF " << name << " 0x"
            << std::uppercase << std::hex
            << std::setw(4) << std::setfill('0') << addr
            << std::nouppercase << std::dec << "\n";
    }
}

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
        ctx.area_bases = opts.area_bases;
        ctx.output_range = opts.output_range;
        ctx.format = opts.format;
        ctx.verbose = opts.verbose;
        ctx.print_map = opts.print_map;

        if (ctx.verbose)
            std::cout << "xlink " << XLINK_VERSION << "\n";

        xlink::linker::link(ctx, opts);
        xlink::binary_emitter::emit(opts.output_file, ctx);
        if (opts.symbol_file.has_value())
            emit_symbol_file(opts.symbol_file.value(), ctx);

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
