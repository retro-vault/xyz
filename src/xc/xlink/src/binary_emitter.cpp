// binary_emitter.cpp
//
// output file writer
//
// MIT License (see: LICENSE)
// copyright (C) 2021 tomaz stih
//
// 2021-07-28   tstih
#include <fstream>

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
