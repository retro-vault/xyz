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
#include <iomanip>

#include <xld/binary_emitter.h>
#include <xld/errors.h>

namespace xld {

    static std::optional<std::pair<uint16_t, uint16_t>>
    occupied_window(const link_context& ctx)
    {
        if (ctx.code_occupancy.empty())
            return std::nullopt;

        // Flat Z80 outputs can address only 64 KiB.  Some link images include
        // virtual/reserved areas whose end address reaches past 0xFFFF (for
        // example a heap ending at 0x10C00); do not let that wrap the detected
        // end address back into the low page.
        const uint32_t limit = std::min<uint32_t>(
            static_cast<uint32_t>(ctx.code_occupancy.size()),
            0x10000u);

        uint32_t first = 0;
        while (first < limit && ctx.code_occupancy[first] == 0x00)
            ++first;
        if (first >= limit)
            return std::nullopt;

        uint32_t last = limit;
        while (last > first && ctx.code_occupancy[last - 1] == 0x00)
            --last;

        return std::pair<uint16_t, uint16_t>{
            static_cast<uint16_t>(first),
            static_cast<uint16_t>(last - 1)
        };
    }

    static std::pair<uint16_t, uint16_t> emit_window(const link_context& ctx) {
        if (ctx.output_range.has_value()) {
            const uint16_t start = ctx.output_range->start;
            const uint16_t end = ctx.output_range->end;
            if (start > end)
                throw xld_error("invalid binary output range");
            return {start, end};
        }

        if (auto occupied = occupied_window(ctx))
            return *occupied;

        if (ctx.code_buffer.empty())
            return {0x0000, 0x0000};

        const uint16_t end = ctx.code_buffer.size() > 0x10000u
            ? 0xFFFF
            : static_cast<uint16_t>(ctx.code_buffer.size() - 1u);
        const uint16_t start = ctx.image_base <= end
            ? static_cast<uint16_t>(ctx.image_base)
            : 0x0000;
        return {start, end};
    }

    template <typename Callback>
    static void for_each_hole_guard(
        const link_context& ctx,
        uint16_t start,
        uint16_t end,
        Callback&& callback)
    {
        for (const auto& hole : ctx.holes) {
            uint16_t hs = std::max<uint16_t>(hole.start, start);
            uint16_t he = std::min<uint16_t>(hole.end, end);
            if (hs > he)
                continue;

            uint32_t hole_size = static_cast<uint32_t>(he) - hs + 1u;
            if (static_cast<uint32_t>(hs) >= static_cast<uint32_t>(start) + 2u
                && hole_size <= 0x7Fu
                && static_cast<uint32_t>(he) + 1u <= 0xFFFFu) {
                callback(
                    static_cast<uint16_t>(hs - 2u),
                    std::vector<uint8_t>{
                        0x18,
                        static_cast<uint8_t>(hole_size)
                    });
            } else if (static_cast<uint32_t>(hs)
                           >= static_cast<uint32_t>(start) + 3u
                       && static_cast<uint32_t>(he) + 1u <= 0xFFFFu) {
                const uint16_t target = static_cast<uint16_t>(he + 1u);
                callback(
                    static_cast<uint16_t>(hs - 3u),
                    std::vector<uint8_t>{
                        0xC3,
                        static_cast<uint8_t>(target & 0xFF),
                        static_cast<uint8_t>((target >> 8) & 0xFF)
                    });
            }
        }
    }

    // Helper: write uint16_t in little-endian.
    static void write_le16(std::ofstream& out, uint16_t val) {
        uint8_t buf[2] = {
            static_cast<uint8_t>(val & 0xFF),
            static_cast<uint8_t>((val >> 8) & 0xFF)
        };
        out.write(reinterpret_cast<const char*>(buf), 2);
    }

    static std::vector<uint8_t> build_linear_image(const link_context& ctx) {
        const auto [start, end] = emit_window(ctx);

        const uint32_t out_size = static_cast<uint32_t>(end - start + 1);
        std::vector<uint8_t> image(out_size, 0x00);

        for (uint32_t addr = start; addr <= end; ++addr) {
            if (addr < ctx.code_buffer.size())
                image[addr - start] = ctx.code_buffer[addr];
        }

        // For protected ranges, keep the reserved bytes zero-filled and,
        // when it fits, place a JR or JP immediately before the hole.
        for (const auto& hole : ctx.holes) {
            uint16_t hs = std::max<uint16_t>(hole.start, start);
            uint16_t he = std::min<uint16_t>(hole.end, end);
            if (hs > he)
                continue;

            // Reserved bytes remain untouched in the final image.
            for (uint32_t addr = hs; addr <= he; ++addr)
                image[addr - start] = 0x00;
        }

        // If there are enough bytes before the hole inside the emitted range,
        // place a JR for short skips or a JP for longer ones.
        for_each_hole_guard(ctx, start, end,
            [&](uint16_t guard, const std::vector<uint8_t>& bytes) {
                const uint32_t idx = static_cast<uint32_t>(guard - start);
                for (size_t i = 0; i < bytes.size(); ++i)
                    image[idx + i] = bytes[i];
            });

        return image;
    }

    static std::vector<uint8_t> build_linear_occupancy(
        const link_context& ctx,
        uint16_t start,
        uint16_t end)
    {
        const uint32_t out_size = static_cast<uint32_t>(end - start + 1);
        std::vector<uint8_t> occupancy(out_size, 0x00);
        const bool has_sparse_data = !ctx.code_occupancy.empty();

        for (uint32_t addr = start; addr <= end; ++addr) {
            if (has_sparse_data) {
                if (addr < ctx.code_occupancy.size() && ctx.code_occupancy[addr] != 0)
                    occupancy[addr - start] = 0x01;
            } else if (addr < ctx.code_buffer.size()) {
                occupancy[addr - start] = 0x01;
            }
        }

        for_each_hole_guard(ctx, start, end,
            [&](uint16_t guard, const std::vector<uint8_t>& bytes) {
                const uint32_t idx = static_cast<uint32_t>(guard - start);
                for (size_t i = 0; i < bytes.size(); ++i)
                    occupancy[idx + i] = 0x01;
            });

        return occupancy;
    }

    static void emit_ihx(std::ofstream& out, const link_context& ctx) {
        const auto [start, end] = emit_window(ctx);
        const auto image = build_linear_image(ctx);
        const auto occupancy = build_linear_occupancy(ctx, start, end);

        for (size_t offset = 0; offset < image.size();) {
            while (offset < image.size() && occupancy[offset] == 0x00)
                ++offset;
            if (offset >= image.size())
                break;

            const size_t chunk_start = offset;
            while (offset < image.size()
                   && occupancy[offset] != 0x00
                   && (offset - chunk_start) < 16u) {
                ++offset;
            }

            const uint8_t count = static_cast<uint8_t>(offset - chunk_start);
            const uint16_t addr = static_cast<uint16_t>(start + chunk_start);
            uint8_t checksum = count
                             + static_cast<uint8_t>((addr >> 8) & 0xFF)
                             + static_cast<uint8_t>(addr & 0xFF);

            out << ":"
                << std::uppercase << std::hex << std::setfill('0')
                << std::setw(2) << static_cast<unsigned>(count)
                << std::setw(4) << static_cast<unsigned>(addr)
                << "00";

            for (uint8_t i = 0; i < count; ++i) {
                const uint8_t byte = image[chunk_start + i];
                checksum = static_cast<uint8_t>(checksum + byte);
                out << std::setw(2) << static_cast<unsigned>(byte);
            }

            checksum = static_cast<uint8_t>(0u - checksum);
            out << std::setw(2) << static_cast<unsigned>(checksum) << "\n";
        }

        out << ":00000001FF\n";
    }

    void binary_emitter::emit(const std::filesystem::path& path,
                              const link_context& ctx)
    {
        if (ctx.format == output_format::elf)
            throw xld_error("ELF output must be emitted through elf_emitter");

        std::ofstream out(path, std::ios::binary);
        if (!out.is_open())
            throw xld_error("cannot open output file: " + path.string());

        if (ctx.format == output_format::bin) {
            const auto image = build_linear_image(ctx);
            for (uint8_t byte : image)
                out.put(static_cast<char>(byte));

            out.close();
            return;
        }

        if (ctx.format == output_format::ihx) {
            emit_ihx(out, ctx);
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
        if (ctx.code_size > 0xFFFFu)
            throw xld_error("XL output exceeds 64K address space");
        write_le16(out, static_cast<uint16_t>(ctx.code_size));

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

} // namespace xld
