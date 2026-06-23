// map_emitter.cpp
//
// linker map writer
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
//
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <map>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

#include <xld/errors.h>
#include <xld/map_emitter.h>

namespace xld {

    namespace {

        static uint32_t symbol_absolute_addr(const module* mod,
                                             const symbol& sym)
        {
            uint16_t addr = sym.value();
            int area_idx = sym.area_index();
            if (area_idx >= 0
                && area_idx < static_cast<int>(mod->areas().size())) {
                const auto& area = mod->areas()[area_idx];
                if (area.placed_addr().has_value()) {
                    addr = static_cast<uint16_t>(
                        addr + area.placed_addr().value());
                }
            } else if (!mod->areas().empty()
                       && mod->areas()[0].placed_addr().has_value()) {
                addr = static_cast<uint16_t>(
                    addr + mod->areas()[0].placed_addr().value());
            }
            return addr;
        }

    } // namespace

    void map_emitter::emit(std::ostream& out, const link_context& ctx) {
        out << "Memory map:\n";
        out << "  Area              Addr   Size   Flags\n";
        out << "  ----              ----   ----   -----\n";

        for (const auto& mod : ctx.modules) {
            for (const auto& area : mod->areas()) {
                if (!area.placed_addr().has_value())
                    continue;

                out << "  "
                    << std::left << std::setw(16) << area.name()
                    << "  "
                    << std::right << std::uppercase << std::hex
                    << std::setfill('0') << std::setw(4)
                    << area.placed_addr().value()
                    << "   "
                    << std::setw(4) << area.size()
                    << std::setfill(' ') << std::dec
                    << "   "
                    << (area.is_abs() ? "ABS" : "REL")
                    << (area.is_ovr() ? " OVR" : " CON")
                    << "\n";
            }
        }

        out << "  Total code size: 0x"
            << std::uppercase << std::hex << ctx.code_size
            << std::dec << "\n\n";

        std::vector<std::tuple<uint32_t, std::string, bool>> symbols;
        symbols.reserve(ctx.global_symbols.size() + ctx.linker_symbols.size());

        for (const auto& [name, def] : ctx.global_symbols) {
            const auto* mod = def.first;
            int idx = def.second;
            symbols.emplace_back(symbol_absolute_addr(mod, mod->symbol_by_index(idx)),
                                 name,
                                 false);
        }

        for (const auto& [name, addr] : ctx.linker_symbols)
            symbols.emplace_back(addr, name, true);

        std::sort(symbols.begin(), symbols.end(),
                  [](const auto& lhs, const auto& rhs) {
                      if (std::get<0>(lhs) != std::get<0>(rhs))
                          return std::get<0>(lhs) < std::get<0>(rhs);
                      if (std::get<2>(lhs) != std::get<2>(rhs))
                          return std::get<2>(lhs) < std::get<2>(rhs);
                      return std::get<1>(lhs) < std::get<1>(rhs);
                  });

        out << "Symbols:\n";
        for (const auto& [addr, name, is_linker] : symbols) {
            out << std::uppercase << std::hex
                << std::setfill('0') << std::setw(8) << addr
                << " "
                << name;
            if (is_linker)
                out << " ; linker";
            out << "\n";
        }
        out << std::dec;
    }

    void map_emitter::emit(const std::filesystem::path& path,
                           const link_context& ctx)
    {
        std::ofstream out(path);
        if (!out.is_open())
            throw xld_error("cannot open map file: " + path.string());
        emit(out, ctx);
    }

} // namespace xld
