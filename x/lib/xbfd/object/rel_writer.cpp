// object/rel_writer.cpp — SDCC .rel text-format writer (XL4 variant).
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
//
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <optional>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

#include <xbfd/xbfd.h>

namespace bfd {
namespace {

class rel_writer {
public:
    void emit(const xbfd::object& obj, std::ostream& out) {
        const auto syms = collect_symbols(obj);
        const auto header_order = build_header_order(obj);
        const auto sec_index = build_index_map(obj, header_order);
        write_header(obj, syms, out);
        write_option(obj, out);
        write_pre_area_symbols(syms, out);
        write_areas_and_defs(obj, header_order, syms, out);
        write_text_reloc_pairs(obj, sec_index, syms, out);
    }

private:
    struct emitted_symbol {
        const xbfd::symbol* sym = nullptr;
        std::string         name;
        int                 index = -1;
        int                 section_index = -1;
    };

    static constexpr const char* ABS_SYMBOL = ".__.ABS.";

    void write_header(const xbfd::object& obj,
                      const std::vector<emitted_symbol>& syms,
                      std::ostream& out) {
        out << "XL4\n";
        out << "H " << hex_any(obj.sections.size()) << " areas "
            << hex_any(syms.size()) << " global symbols\n";
        const std::string mod = obj.module_name.empty() ? "module" : obj.module_name;
        out << "M " << mod << "\n";
    }

    void write_option(const xbfd::object& obj, std::ostream& out) {
        switch (obj.default_calling_convention) {
        case xbfd::calling_convention::xcc_sdcccall0:
            out << "O -mz80 sdcccall(0)\n";
            break;
        case xbfd::calling_convention::xcc_sdcccall1:
            out << "O -mz80 sdcccall(1)\n";
            break;
        case xbfd::calling_convention::xcc_z88dk_fastcall:
            out << "O -mz80 z88dk::fastcall\n";
            break;
        case xbfd::calling_convention::xcc_z88dk_callee:
            out << "O -mz80 z88dk::callee\n";
            break;
        default:
            break;
        }
    }

    void write_pre_area_symbols(const std::vector<emitted_symbol>& syms, std::ostream& out) {
        for (const auto& entry : syms) {
            if (entry.section_index == -1)
                write_symbol_record(entry, out);
        }
    }

    void write_areas_and_defs(const xbfd::object& obj,
                              const std::vector<size_t>& header_order,
                              const std::vector<emitted_symbol>& syms,
                              std::ostream& out) {
        for (size_t si : header_order) {
            const auto& sec = obj.sections[si];
            uint8_t flags = 0;
            if (has_flag(sec.flags, xbfd::section_flags::overlay)) flags |= 0x01;
            if (has_flag(sec.flags, xbfd::section_flags::abs))     flags |= 0x08;
            out << "A " << sec.name << " size " << hex_any(sec.size)
                << " flags " << hex_any(flags)
                << " addr " << hex_any(sec.vma);
            out << "\n";

            for (const auto& entry : syms) {
                if (entry.section_index == static_cast<int>(si))
                    write_symbol_record(entry, out);
            }
        }
    }

    void write_text_reloc_pairs(const xbfd::object& obj,
                                const std::vector<std::pair<std::string,int>>& sec_index,
                                const std::vector<emitted_symbol>& syms,
                                std::ostream& out) {
        for (size_t si = 0; si < obj.sections.size(); ++si) {
            const auto& sec  = obj.sections[si];
            const int header_si = lookup_index(sec_index, sec.name);
            if (!sec.emitted_items.empty()) {
                write_emitted_text_reloc_pairs(sec, header_si, sec_index, syms, out);
                continue;
            }
            const auto& data = sec.data;
            if (data.empty()) continue;
            static constexpr size_t CHUNK = 12;
            write_t_record(data, 0, 0, out);
            write_r_record(sec, header_si, 0, 0, sec_index, syms, out);
            for (size_t pos = 0; pos < data.size(); pos += CHUNK) {
                const size_t len = std::min(CHUNK, data.size() - pos);
                write_t_record(data, pos, len, out);
                write_r_record(sec, header_si, pos, len, sec_index, syms, out);
            }
        }
    }

    void write_emitted_text_reloc_pairs(const xbfd::section& sec, int header_si,
                                        const std::vector<std::pair<std::string,int>>& sec_idx,
                                        const std::vector<emitted_symbol>& syms,
                                        std::ostream& out) {
        static constexpr size_t DATA_CAP  = 12;
        static constexpr size_t RELOC_CAP = 3;

        struct chunk_reloc {
            xbfd::reloc_entry reloc;
            size_t            chunk_byte_offset = 0;
        };

        std::vector<uint8_t> bytes;
        std::vector<chunk_reloc> rels;
        uint32_t chunk_start = 0;
        uint32_t abs_off = 0;

        auto flush = [&]() {
            if (bytes.empty())
                return;
            write_t_record_bytes(chunk_start, bytes, out);
            out << "R 00 00 "
                << hex2(static_cast<uint8_t>(header_si & 0xFF)) << " "
                << hex2(static_cast<uint8_t>((header_si >> 8) & 0xFF));
            for (const auto& cr : rels) {
                const uint8_t mode = encode_mode_xl4(cr.reloc.type, cr.reloc.sym_relative);
                const uint8_t t_off = static_cast<uint8_t>(cr.chunk_byte_offset + 4);
                int ref = 0;
                if (cr.reloc.sym_relative) {
                    ref = find_sym_index(syms, cr.reloc.name);
                } else {
                    for (const auto& [name, idx] : sec_idx) {
                        if (name == cr.reloc.name) {
                            ref = idx;
                            break;
                        }
                    }
                }
                out << " " << hex2(mode) << " " << hex2(t_off)
                    << " " << hex2(ref & 0xFF) << " " << hex2((ref >> 8) & 0xFF);
            }
            out << "\n";
            bytes.clear();
            rels.clear();
        };

        auto write_empty_pair = [&](uint32_t off) {
            write_t_record(sec.data, off, 0, out);
            write_r_record(sec, header_si, off, 0, sec_idx, syms, out);
        };

        for (const auto& item : sec.emitted_items) {
            if (item.label_marker) {
                flush();
                chunk_start = abs_off;
                write_empty_pair(abs_off);
                continue;
            }
            if (item.reserve_bytes) {
                flush();
                write_empty_pair(abs_off);
                abs_off += item.reserve_bytes;
                chunk_start = abs_off;
                continue;
            }
            const size_t item_size = item.data.size();
            const size_t item_relocs = item.reloc ? 1u : 0u;
            if (!bytes.empty() &&
                (bytes.size() + item_size > DATA_CAP ||
                 rels.size() + item_relocs > RELOC_CAP)) {
                flush();
                chunk_start = abs_off;
            }
            if (bytes.empty())
                chunk_start = abs_off;
            const size_t item_offset_in_chunk = bytes.size();
            bytes.insert(bytes.end(), item.data.begin(), item.data.end());
            if (item.reloc)
                rels.push_back(chunk_reloc{*item.reloc, item_offset_in_chunk});
            abs_off += static_cast<uint32_t>(item_size);
        }
        flush();
    }

    void write_t_record_bytes(uint32_t off, const std::vector<uint8_t>& bytes,
                              std::ostream& out) {
        out << "T " << hex2(static_cast<uint8_t>(off & 0xFF))
            << " " << hex2(static_cast<uint8_t>((off >> 8) & 0xFF))
            << " 00 00";
        for (uint8_t b : bytes)
            out << " " << hex2(b);
        out << "\n";
    }

    void write_t_record(const std::vector<uint8_t>& data, size_t pos,
                         size_t len, std::ostream& out) {
        const auto off = static_cast<uint16_t>(pos);
        out << "T " << hex2(off & 0xFF) << " " << hex2(off >> 8) << " 00 00";
        for (size_t i = 0; i < len; ++i) out << " " << hex2(data[pos + i]);
        out << "\n";
    }
    void write_r_record(const xbfd::section& sec, int header_si, size_t pos, size_t len,
                         const std::vector<std::pair<std::string,int>>& sec_idx,
                         const std::vector<emitted_symbol>& syms,
                         std::ostream& out) {
        out << "R 00 00 "
            << hex2(static_cast<uint8_t>(header_si & 0xFF)) << " "
            << hex2(static_cast<uint8_t>((header_si >> 8) & 0xFF));
        for (const auto& r : sec.relocs) {
            if (r.offset < pos || r.offset >= pos + len) continue;
            const uint8_t mode  = encode_mode_xl4(r.type, r.sym_relative);
            const uint8_t t_off = static_cast<uint8_t>((r.offset - pos) + 4);
            int ref = 0;
            if (r.sym_relative) {
                ref = find_sym_index(syms, r.name);
            } else {
                for (const auto& [name, idx] : sec_idx) if (name == r.name) { ref = idx; break; }
            }
            out << " " << hex2(mode) << " " << hex2(t_off)
                << " " << hex2(ref & 0xFF) << " " << hex2((ref >> 8) & 0xFF);
        }
        out << "\n";
    }

    static void write_symbol_record(const emitted_symbol& entry, std::ostream& out) {
        const auto& name = entry.sym ? entry.sym->name : entry.name;
        if (!entry.sym && name == ABS_SYMBOL) {
            out << "S " << name << " Def00000000\n";
        } else if (!entry.sym || !entry.sym->is_defined()) {
            out << "S " << name << " Ref00000000\n";
        } else {
            out << "S " << name << " Def" << hex8(static_cast<uint32_t>(entry.sym->value)) << "\n";
        }
    }

    static std::vector<emitted_symbol> collect_symbols(const xbfd::object& obj) {
        static constexpr int NHASH = 64;

        auto bucket_of = [](const std::string& name) {
            int h = 0;
            for (unsigned char ch : name)
                h += (ch & 0x7F);
            return h & (NHASH - 1);
        };

        std::vector<std::vector<const xbfd::symbol*>> pre_area(NHASH);
        std::vector<std::vector<std::vector<const xbfd::symbol*>>> per_area(
            obj.sections.size(), std::vector<std::vector<const xbfd::symbol*>>(NHASH));
        bool have_abs = false;

        for (const auto& sym : obj.symbols) {
            if (!should_emit(sym))
                continue;
            if (sym.name == ABS_SYMBOL)
                have_abs = true;

            if (sym.section_name.empty() || sym.is_absolute() || !sym.is_defined()) {
                pre_area[bucket_of(sym.name)].push_back(&sym);
                continue;
            }

            for (size_t si = 0; si < obj.sections.size(); ++si) {
                if (obj.sections[si].name == sym.section_name) {
                    per_area[si][bucket_of(sym.name)].push_back(&sym);
                    break;
                }
            }
        }

        if (!have_abs)
            pre_area[bucket_of(ABS_SYMBOL)].push_back(nullptr);

        std::vector<emitted_symbol> out;
        int next_index = 0;

        for (int bucket = 0; bucket < NHASH; ++bucket) {
            for (auto it = pre_area[bucket].rbegin(); it != pre_area[bucket].rend(); ++it) {
                const auto* sym = *it;
                out.push_back(emitted_symbol{
                    sym,
                    sym ? sym->name : std::string(ABS_SYMBOL),
                    next_index++,
                    -1
                });
            }
        }

        for (size_t si = 0; si < obj.sections.size(); ++si) {
            for (int bucket = 0; bucket < NHASH; ++bucket) {
                for (auto it = per_area[si][bucket].rbegin();
                     it != per_area[si][bucket].rend(); ++it) {
                    const auto* sym = *it;
                    out.push_back(emitted_symbol{
                        sym,
                        sym->name,
                        next_index++,
                        static_cast<int>(si)
                    });
                }
            }
        }

        return out;
    }

    static bool should_emit(const xbfd::symbol& sym) {
        if (!sym.is_defined()) return true;
        if (sym.name == ABS_SYMBOL) return true;
        if (sym.is_absolute()) return sym.is_global();
        return sym.is_global();
    }

    static std::string hex2(uint8_t v) {
        std::ostringstream ss;
        ss << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<unsigned>(v);
        return ss.str();
    }

    static std::string hex8(uint32_t v) {
        std::ostringstream ss;
        ss << std::hex << std::uppercase << std::setw(8) << std::setfill('0') << v;
        return ss.str();
    }

    static std::string hex_any(uint64_t v) {
        std::ostringstream ss;
        ss << std::hex << std::uppercase << v;
        return ss.str();
    }

    static uint8_t encode_mode_base(xbfd::reloc_type t, bool sym_rel) {
        uint8_t m = 0;
        // ASxxxx/SDCC REL encoding:
        //   bit0 = 0 for word relocations, 1 for byte relocations
        //   bit1 = 0 for area-relative, 1 for symbol-relative
        //   bit2 = PC-relative byte relocation
        //   bit3 = select a byte out of a 16-bit value (R3_BYTX)
        //   bit7 = choose the MSB of that 16-bit value
        if (t == xbfd::reloc_type::z80_8 || t == xbfd::reloc_type::z80_pc8
            || t == xbfd::reloc_type::z80_16_msb)
            m |= 0x01;
        if (sym_rel)
            m |= 0x02;
        if (t == xbfd::reloc_type::z80_pc8)
            m |= 0x04;
        if (t == xbfd::reloc_type::z80_16_msb)
            m |= 0x08 | 0x80;
        return m;
    }

    static uint8_t encode_mode_xl4(xbfd::reloc_type t, bool sym_rel) {
        return encode_mode_base(t, sym_rel);
    }

    static int find_sym_index(const std::vector<emitted_symbol>& syms, const std::string& name) {
        for (const auto& entry : syms) {
            if (entry.name == name)
                return entry.index;
        }
        return 0;
    }

    static std::vector<size_t> build_header_order(const xbfd::object& obj) {
        std::vector<size_t> result;
        result.reserve(obj.sections.size());
        std::optional<size_t> text_index;
        for (size_t i = 0; i < obj.sections.size(); ++i) {
            if (obj.sections[i].name == "_CODE" || obj.sections[i].name == ".text") {
                text_index = i;
                result.push_back(i);
                break;
            }
        }
        for (size_t i = 0; i < obj.sections.size(); ++i) {
            if (text_index && i == *text_index)
                continue;
            result.push_back(i);
        }
        return result;
    }

    static std::vector<std::pair<std::string,int>> build_index_map(
            const xbfd::object& obj, const std::vector<size_t>& order) {
        std::vector<std::pair<std::string,int>> result;
        result.reserve(order.size());
        for (size_t i = 0; i < order.size(); ++i)
            result.emplace_back(obj.sections[order[i]].name, static_cast<int>(i));
        return result;
    }

    static int lookup_index(const std::vector<std::pair<std::string,int>>& sec_idx,
                            const std::string& name) {
        for (const auto& [sec_name, idx] : sec_idx) {
            if (sec_name == name)
                return idx;
        }
        return 0;
    }
};

} // namespace

void emit_rel(const xbfd::object& obj, std::ostream& out) {
    rel_writer{}.emit(obj, out);
}

} // namespace bfd

// -------------------------------------------------------------------------
// xbfd::rel_writer
// -------------------------------------------------------------------------

namespace xbfd {

void rel_writer::write(const std::string& path, const object& obj) {
    std::ofstream f(path, std::ios::trunc);
    if (!f.is_open()) throw io_error("cannot write: " + path);
    bfd::emit_rel(obj, f);
}

} // namespace xbfd
