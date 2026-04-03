// binary_emitter.cpp
//
// output file writer
//
// MIT License (see: LICENSE)
// copyright (C) 2021 tomaz stih
//
// 2021-07-28   tstih
#include <fstream>
#include <vector>
#include <algorithm>

#include <xlink/binary_emitter.hpp>
#include <xlink/errors.hpp>

namespace xlink {

    // Helper: write uint16_t in little-endian.
    static void write_le16(std::ofstream& out, uint16_t val) {
        uint8_t buf[2] = {
            static_cast<uint8_t>(val & 0xFF),
            static_cast<uint8_t>((val >> 8) & 0xFF)
        };
        out.write(reinterpret_cast<const char*>(buf), 2);
    }

    void binary_emitter::emit(const std::filesystem::path& path,
                              const link_context& ctx)
    {
        std::ofstream out(path, std::ios::binary);
        if (!out.is_open())
            throw xlink_error("cannot open output file: " + path.string());

        if (ctx.format == output_format::bin) {
            uint16_t start = 0;
            uint16_t end = 0;
            if (ctx.output_range.has_value()) {
                start = ctx.output_range->start;
                end = ctx.output_range->end;
                if (start > end)
                    throw xlink_error("invalid binary output range");
            } else if (!ctx.code_buffer.empty()) {
                end = static_cast<uint16_t>(ctx.code_buffer.size() - 1);
            }

            const uint32_t out_size = static_cast<uint32_t>(end - start + 1);
            std::vector<uint8_t> image(out_size, 0x00);

            for (uint32_t addr = start; addr <= end; ++addr) {
                if (addr < ctx.code_buffer.size()) {
                    image[addr - start] = ctx.code_buffer[addr];
                }
            }

            // For protected ranges, emit jump trampolines that skip
            // the full protected region. Place one at the range start,
            // then every 8 bytes inside the same protected range.
            for (const auto& hole : ctx.holes) {
                uint16_t hs = std::max<uint16_t>(hole.start, start);
                uint16_t he = std::min<uint16_t>(hole.end, end);
                if (hs > he)
                    continue;
                if (static_cast<uint32_t>(he) + 1 > 0xFFFF)
                    continue;
                uint16_t target = static_cast<uint16_t>(he + 1);

                for (uint16_t guard = hs; static_cast<uint32_t>(guard) + 2 <= he; ) {
                    const uint32_t idx = static_cast<uint32_t>(guard - start);
                    image[idx + 0] = 0xC3;  // jp nn
                    image[idx + 1] = static_cast<uint8_t>(target & 0xFF);
                    image[idx + 2] = static_cast<uint8_t>((target >> 8) & 0xFF);

                    if (static_cast<uint32_t>(guard) + 8 > 0xFFFF)
                        break;
                    uint16_t next = static_cast<uint16_t>(guard + 8);
                    if (next <= guard)
                        break;
                    guard = next;
                }
            }

            for (uint8_t byte : image) {
                out.put(static_cast<char>(byte));
            }

            out.close();
            return;
        }

        // Header (12 bytes).
        // Magic: 'X', 'L'
        out.put(0x58);
        out.put(0x4C);

        // Version.
        out.put(0x01);

        // Flags (Z80 little-endian).
        out.put(0x00);

        // Entry point.
        write_le16(out, ctx.entry_point);

        // Code size.
        write_le16(out, ctx.code_size);

        // Reloc count.
        uint16_t reloc_count = static_cast<uint16_t>(ctx.reloc_table.size());
        write_le16(out, reloc_count);

        // Reserved.
        write_le16(out, 0x0000);

        // Relocation table.
        for (auto& r : ctx.reloc_table) {
            write_le16(out, r.offset);
            out.put(static_cast<char>(r.size));
            out.put(static_cast<char>(r.pad));
        }

        // Code data.
        out.write(reinterpret_cast<const char*>(ctx.code_buffer.data()),
                  ctx.code_buffer.size());

        out.close();
    }

} // namespace xlink
