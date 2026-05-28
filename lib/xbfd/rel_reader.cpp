// rel_reader.cpp
//
// Reads SDCC .rel text-format object files (versions XL1, XL2, XL4) into
// a bfd::bfd handle.  This is the canonical SDCC object format produced by
// sdasz80 and by xcc in --mode=sdcc.
//
// Record structure:
//   XL / XL2 / XL4   version marker (little-endian; XH = big-endian)
//   H  nAreas nSyms  header (informational, counts ignored)
//   M  name          module name
//   O  flags         options (ignored)
//   A  name size <hex> flags <hex> [addr <hex>]   area declaration
//   S  name Def<hex>|Ref<hex>                      symbol
//   T  off_lo off_hi byte...                       code/data bytes
//   R  00 00 area_lo area_hi [mode off_lo off_hi ref...]  relocations
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
//
#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <xbfd/bfd.hpp>
#include <xbfd/errors.hpp>

namespace bfd {

    // -------------------------------------------------------------------------
    // Internal helpers
    // -------------------------------------------------------------------------

    namespace {

        enum class rel_version { xl1, xl2, xl4 };

        static uint16_t hex16(const std::string& s) {
            return static_cast<uint16_t>(std::stoul(s, nullptr, 16));
        }
        static uint8_t hex8(const std::string& s) {
            return static_cast<uint8_t>(std::stoul(s, nullptr, 16));
        }

        static std::vector<std::string> split_ws(const std::string& line) {
            std::vector<std::string> tokens;
            std::istringstream iss(line);
            std::string tok;
            while (iss >> tok) tokens.push_back(tok);
            return tokens;
        }

        static std::string trim(const std::string& s) {
            auto a = s.find_first_not_of(" \t\r\n");
            if (a == std::string::npos) return {};
            auto b = s.find_last_not_of(" \t\r\n");
            return s.substr(a, b - a + 1);
        }

        // Decode SDCC area flags into bfd section_flags.
        static section_flags decode_area_flags(uint8_t raw) {
            section_flags f = section_flags::alloc | section_flags::load
                              | section_flags::reloc;
            if (raw & 0x01) f = f | section_flags::overlay; // OVR
            if (raw & 0x08) f = f | section_flags::abs;     // ABS (SDCC 4.x)
            if (raw & 0x04) f = f | section_flags::abs;     // ABS legacy
            return f;
        }

        // Decode reloc mode byte into bfd::reloc_type and sym_relative flag.
        static void decode_rel_mode(uint8_t mode, bool xl4,
                                    reloc_type& type, bool& sym_rel)
        {
            if (xl4) mode ^= 0x01; // xl4 inverts word/byte bit
            bool word     = (mode & 0x01) != 0;
            sym_rel       = (mode & 0x02) != 0;
            bool pc_rel   = (mode & 0x04) != 0;
            bool msb      = (mode & 0x80) != 0;

            if (pc_rel) {
                type = reloc_type::z80_pc8;
            } else if (msb) {
                type = reloc_type::z80_16_msb;
            } else if (word) {
                type = reloc_type::z80_16;
            } else {
                type = reloc_type::z80_8;
            }
        }

    } // anonymous namespace

    // -------------------------------------------------------------------------
    // parse_rel — fills a bfd object from an open .rel stream
    // -------------------------------------------------------------------------

    void parse_rel(bfd& obj, const std::string& src_name, std::istream& in)
    {
        rel_version ver    = rel_version::xl1;
        int         lineno = 0;

        // Accumulator for T record data before the following R record.
        struct pending_t {
            std::string section_name;
            uint16_t    offset       = 0;
            std::vector<uint8_t> data;
            // relocs are attached after R record
        };
        std::vector<pending_t> pending;
        pending_t* cur_t = nullptr;

        // Area index → section name (for R record area references).
        std::vector<std::string> area_names;

        std::string line;
        while (std::getline(in, line)) {
            ++lineno;
            line = trim(line);
            if (line.empty()) continue;

            char rec = line[0];
            std::string rest = (line.size() > 2) ? line.substr(2) : "";

            switch (rec) {
            case 'X': {
                if (line.size() >= 2 && line[1] == 'L') {
                    // little-endian is the default
                    if (line.size() >= 3 && line[2] == '4')
                        ver = rel_version::xl4;
                    else if (line.size() >= 3 && line[2] == '2')
                        ver = rel_version::xl2;
                } else if (line.size() >= 2 && line[1] == 'H') {
                    // big-endian target (unusual)
                }
                break;
            }
            case 'H': case 'O':
                break; // informational / ignored
            case 'M': {
                // M <module_name>
                auto toks = split_ws(rest);
                if (!toks.empty()) obj.set_module_name(toks[0]);
                break;
            }

            case 'A': {
                // A <name> size <hex> flags <hex> [addr <hex>]
                auto toks = split_ws(rest);
                if (toks.size() < 5)
                    throw format_error(src_name, lineno, "malformed A record");

                std::string  name  = toks[0];
                uint16_t     sz    = hex16(toks[2]);
                uint8_t      raw_f = hex8(toks[4]);
                section_flags sf   = decode_area_flags(raw_f);

                uint64_t vma = 0;
                if (toks.size() >= 7 && toks[5] == "addr")
                    vma = hex16(toks[6]);

                auto& sec = obj.add_section(name, sf, vma);
                sec.set_size(sz);

                area_names.push_back(name);
                break;
            }
            case 'S': {
                // S <name> Def<hex> | Ref<hex>
                auto toks = split_ws(rest);
                if (toks.size() < 2)
                    throw format_error(src_name, lineno, "malformed S record");

                const std::string& name   = toks[0];
                const std::string& tv     = toks[1];
                bool               is_def = tv.substr(0, 3) == "Def";
                if (!is_def && tv.substr(0, 3) != "Ref")
                    throw format_error(src_name, lineno,
                                       "unknown symbol type in S record");

                uint16_t val = hex16(tv.substr(3));
                symbol_flags sf = is_def ? symbol_flags::global
                                         : (symbol_flags::global
                                            | symbol_flags::undefined);
                // Section name is set when we know which area this symbol
                // belongs to; for refs we leave it empty.
                std::string sec_name;
                if (is_def && !area_names.empty())
                    sec_name = area_names.back();

                obj.add_symbol(name, sf, val, sec_name);
                break;
            }
            case 'T': {
                // T off_lo off_hi byte...  (xl4: T b0 b1 b2 b3 byte...)
                auto toks = split_ws(rest);
                size_t off_bytes = (ver == rel_version::xl4) ? 4 : 2;
                if (toks.size() < off_bytes)
                    throw format_error(src_name, lineno, "malformed T record");

                pending.emplace_back();
                cur_t = &pending.back();
                cur_t->offset = static_cast<uint16_t>(
                    hex8(toks[0]) | (static_cast<uint16_t>(hex8(toks[1])) << 8));

                for (size_t i = off_bytes; i < toks.size(); ++i)
                    cur_t->data.push_back(hex8(toks[i]));
                break;
            }
            case 'R': {
                // R 00 00 area_lo area_hi [mode off_lo ref...]
                auto toks = split_ws(rest);
                if (toks.size() < 4)
                    throw format_error(src_name, lineno, "malformed R record");

                int area_idx = hex8(toks[2]) | (static_cast<int>(hex8(toks[3])) << 8);
                if (area_idx < static_cast<int>(area_names.size()))
                    if (cur_t) cur_t->section_name = area_names[area_idx];

                // Parse relocation entries.
                size_t i = 4;
                while (i + 3 < toks.size()) {
                    uint8_t mode_byte = hex8(toks[i]);
                    uint8_t b1        = hex8(toks[i + 1]);
                    uint8_t b2        = hex8(toks[i + 2]);
                    uint8_t b3        = hex8(toks[i + 3]);

                    reloc_type rtype;
                    bool sym_rel;
                    decode_rel_mode(mode_byte,
                                    ver == rel_version::xl4,
                                    rtype, sym_rel);

                    uint32_t t_off;
                    int      ref_idx;
                    if (ver == rel_version::xl4) {
                        // [mode][off_from_T_start][ref_lo][ref_hi]
                        t_off   = (b1 >= 4) ? b1 - 4 : 0;
                        ref_idx = b2 | (static_cast<int>(b3) << 8);
                    } else if (ver == rel_version::xl2) {
                        // [mode][offset_in_T][ref_lo][ref_hi]
                        t_off   = b1;
                        ref_idx = b2 | (static_cast<int>(b3) << 8);
                    } else {
                        // xl1: [mode][off_lo][off_hi][ref_byte]
                        t_off   = b1 | (static_cast<uint32_t>(b2) << 8);
                        ref_idx = b3;
                    }

                    // Look up name from symbol or area table.
                    std::string ref_name;
                    if (sym_rel) {
                        auto& syms = obj.symbols();
                        if (ref_idx < static_cast<int>(syms.size()))
                            ref_name = syms[ref_idx].name();
                    } else {
                        if (ref_idx < static_cast<int>(area_names.size()))
                            ref_name = area_names[ref_idx];
                    }

                    if (cur_t) {
                        reloc r;
                        r.offset       = cur_t->offset + t_off;
                        r.type         = rtype;
                        r.sym_relative = sym_rel;
                        r.name         = ref_name;
                        r.addend       = 0;

                        // Attach to the section.
                        section* sec = cur_t->section_name.empty()
                            ? nullptr
                            : obj.find_section(cur_t->section_name);
                        if (sec) sec->add_reloc(r);
                    }
                    i += 4;
                }
                break;
            }
            case ';':
                break; // comment line — skip
            default:
                break; // unknown record type — skip
            }
        }

        // Append T record bytes to their respective sections.
        for (auto& pt : pending) {
            if (pt.section_name.empty() && !area_names.empty())
                pt.section_name = area_names[0];
            section* sec = obj.find_section(pt.section_name);
            if (!sec) continue;
            for (auto b : pt.data)
                sec->append_byte(b);
        }
    }

} // namespace bfd
