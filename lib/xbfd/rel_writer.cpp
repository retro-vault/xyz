// rel_writer.cpp
//
// Emits a bfd::bfd object as an SDCC .rel text-format file (XL2 variant).
// The output is compatible with xlink and any SDCC-based linker.
//
// Output record order:
//   XL2            version marker
//   M <module>     module name (derived from filename stem)
//   A ...          one per section
//   S ...          symbols (defs first, then refs)
//   (T + R pairs)  per area: data bytes followed by relocations
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
//
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>

#include <xbfd/bfd.hpp>
#include <xbfd/errors.hpp>

namespace bfd {

    namespace {

        static std::string hex2(uint8_t v) {
            std::ostringstream ss;
            ss << std::hex << std::uppercase
               << std::setw(2) << std::setfill('0')
               << static_cast<unsigned>(v);
            return ss.str();
        }
        static std::string hex4(uint16_t v) {
            std::ostringstream ss;
            ss << std::hex << std::uppercase
               << std::setw(4) << std::setfill('0') << v;
            return ss.str();
        }

        // Encode bfd reloc_type + sym_relative flag back into SDCC mode byte.
        static uint8_t encode_mode(reloc_type t, bool sym_rel) {
            uint8_t m = 0;
            if (t == reloc_type::z80_16)       m |= 0x01; // word
            if (sym_rel)                        m |= 0x02; // symbol ref
            if (t == reloc_type::z80_pc8)       m |= 0x04; // PC-relative
            if (t == reloc_type::z80_16_msb)    m |= 0x80; // MSB
            return m;
        }

    } // anonymous namespace

    void emit_rel(const bfd& obj, std::ostream& out)
    {
        // Header.
        out << "XL2\n";
        out << "H " << obj.sections().size()
            << " areas " << obj.symbols().size() << " global symbols\n";

        // Module name — derive from path stem.
        std::string mod_name = obj.filename().stem().string();
        if (mod_name.empty()) mod_name = "module";
        out << "M " << mod_name << "\n";

        // Build section-name → index map (needed for reloc emission).
        std::vector<std::string> sec_names;
        for (const auto& s : obj.sections())
            sec_names.push_back(s.name());

        // Build symbol-name → index map.
        std::vector<std::string> sym_names;
        for (const auto& s : obj.symbols())
            sym_names.push_back(s.name());

        // Area records.
        for (const auto& sec : obj.sections()) {
            uint8_t flags = 0;
            if (has_flag(sec.flags(), section_flags::overlay)) flags |= 0x01;
            if (has_flag(sec.flags(), section_flags::abs))     flags |= 0x08;

            out << "A " << sec.name()
                << " size " << hex4(static_cast<uint16_t>(sec.size()))
                << " flags " << hex2(flags);
            if (has_flag(sec.flags(), section_flags::abs))
                out << " addr " << hex4(static_cast<uint16_t>(sec.vma()));
            out << "\n";
        }

        // Symbol records — same order as obj.symbols() to keep R-record indices valid.
        for (const auto& sym : obj.symbols()) {
            if (sym.is_defined())
                out << "S " << sym.name()
                    << " Def" << hex4(static_cast<uint16_t>(sym.value())) << "\n";
            else
                out << "S " << sym.name() << " Ref0000\n";
        }

        // T + R pairs, one group per section.
        for (size_t si = 0; si < obj.sections().size(); ++si) {
            const auto& sec = obj.sections()[si];
            const auto& data = sec.contents();
            if (data.empty()) continue;

            // Emit data in chunks of up to 24 bytes to keep lines readable.
            static constexpr size_t CHUNK = 24;
            size_t pos = 0;
            while (pos < data.size()) {
                size_t len = std::min(CHUNK, data.size() - pos);

                // T record.
                uint16_t off = static_cast<uint16_t>(pos);
                out << "T " << hex2(off & 0xFF) << " " << hex2(off >> 8);
                for (size_t i = 0; i < len; ++i)
                    out << " " << hex2(data[pos + i]);
                out << "\n";

                // R record header.
                out << "R 00 00 "
                    << hex2(static_cast<uint8_t>(si & 0xFF)) << " "
                    << hex2(static_cast<uint8_t>((si >> 8) & 0xFF));

                // Emit relocs whose offset falls in [pos, pos+len).
                for (const auto& r : sec.relocs()) {
                    if (r.offset < pos || r.offset >= pos + len) continue;

                    uint8_t mode = encode_mode(r.type, r.sym_relative);
                    uint8_t t_off = static_cast<uint8_t>(r.offset - pos);

                    // Find the ref index.
                    int ref_idx = 0;
                    if (r.sym_relative) {
                        auto it = std::find(sym_names.begin(), sym_names.end(),
                                            r.name);
                        if (it != sym_names.end())
                            ref_idx = static_cast<int>(it - sym_names.begin());
                    } else {
                        auto it = std::find(sec_names.begin(), sec_names.end(),
                                            r.name);
                        if (it != sec_names.end())
                            ref_idx = static_cast<int>(it - sec_names.begin());
                    }

                    out << " " << hex2(mode)
                        << " " << hex2(t_off)
                        << " " << hex2(ref_idx & 0xFF)
                        << " " << hex2((ref_idx >> 8) & 0xFF);
                }
                out << "\n";
                pos += len;
            }
        }
    }

} // namespace bfd
