// object/rel_writer.cpp — SDCC .rel text-format writer (XL2 variant).
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
//
#include <algorithm>
#include <fstream>
#include <iomanip>
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
        write_header(obj, out);
        write_areas(obj, out);
        write_symbols(obj, out);
        write_text_reloc_pairs(obj, out);
    }

private:
    void write_header(const xbfd::object& obj, std::ostream& out) {
        out << "XL2\n";
        out << "H " << obj.sections.size() << " areas " << obj.symbols.size() << " global symbols\n";
        const std::string mod = obj.module_name.empty() ? "module" : obj.module_name;
        out << "M " << mod << "\n";
    }
    void write_areas(const xbfd::object& obj, std::ostream& out) {
        for (const auto& sec : obj.sections) {
            uint8_t flags = 0;
            if (has_flag(sec.flags, xbfd::section_flags::overlay)) flags |= 0x01;
            if (has_flag(sec.flags, xbfd::section_flags::abs))     flags |= 0x08;
            out << "A " << sec.name << " size " << hex4(static_cast<uint16_t>(sec.size))
                << " flags " << hex2(flags);
            if (has_flag(sec.flags, xbfd::section_flags::abs))
                out << " addr " << hex4(static_cast<uint16_t>(sec.vma));
            out << "\n";
        }
    }
    void write_symbols(const xbfd::object& obj, std::ostream& out) {
        for (const auto& sym : obj.symbols) {
            if (sym.is_defined())
                out << "S " << sym.name << " Def" << hex4(static_cast<uint16_t>(sym.value)) << "\n";
            else
                out << "S " << sym.name << " Ref0000\n";
        }
    }
    void write_text_reloc_pairs(const xbfd::object& obj, std::ostream& out) {
        const auto sec_index = index_map(obj.sections, [](const xbfd::section& s) { return s.name; });
        const auto sym_index = index_map(obj.symbols,  [](const xbfd::symbol& s)  { return s.name; });
        for (size_t si = 0; si < obj.sections.size(); ++si) {
            const auto& sec  = obj.sections[si];
            const auto& data = sec.data;
            if (data.empty()) continue;
            static constexpr size_t CHUNK = 24;
            for (size_t pos = 0; pos < data.size(); pos += CHUNK) {
                const size_t len = std::min(CHUNK, data.size() - pos);
                write_t_record(data, pos, len, out);
                write_r_record(sec, si, pos, len, sec_index, sym_index, out);
            }
        }
    }
    void write_t_record(const std::vector<uint8_t>& data, size_t pos,
                         size_t len, std::ostream& out) {
        const auto off = static_cast<uint16_t>(pos);
        out << "T " << hex2(off & 0xFF) << " " << hex2(off >> 8);
        for (size_t i = 0; i < len; ++i) out << " " << hex2(data[pos + i]);
        out << "\n";
    }
    void write_r_record(const xbfd::section& sec, size_t si, size_t pos, size_t len,
                         const std::vector<std::pair<std::string,int>>& sec_idx,
                         const std::vector<std::pair<std::string,int>>& sym_idx,
                         std::ostream& out) {
        out << "R 00 00 "
            << hex2(static_cast<uint8_t>(si & 0xFF)) << " "
            << hex2(static_cast<uint8_t>((si >> 8) & 0xFF));
        for (const auto& r : sec.relocs) {
            if (r.offset < pos || r.offset >= pos + len) continue;
            const uint8_t mode  = encode_mode(r.type, r.sym_relative);
            const uint8_t t_off = static_cast<uint8_t>(r.offset - pos);
            int ref = 0;
            if (r.sym_relative) {
                for (const auto& [name, idx] : sym_idx) if (name == r.name) { ref = idx; break; }
            } else {
                for (const auto& [name, idx] : sec_idx) if (name == r.name) { ref = idx; break; }
            }
            out << " " << hex2(mode) << " " << hex2(t_off)
                << " " << hex2(ref & 0xFF) << " " << hex2((ref >> 8) & 0xFF);
        }
        out << "\n";
    }

    static std::string hex2(uint8_t v) {
        std::ostringstream ss;
        ss << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<unsigned>(v);
        return ss.str();
    }
    static std::string hex4(uint16_t v) {
        std::ostringstream ss;
        ss << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << v;
        return ss.str();
    }
    static uint8_t encode_mode(xbfd::reloc_type t, bool sym_rel) {
        uint8_t m = 0;
        if (t == xbfd::reloc_type::z80_16)    m |= 0x01;
        if (sym_rel)                            m |= 0x02;
        if (t == xbfd::reloc_type::z80_pc8)    m |= 0x04;
        if (t == xbfd::reloc_type::z80_16_msb) m |= 0x80;
        return m;
    }
    template<typename T, typename NameFn>
    static std::vector<std::pair<std::string,int>> index_map(
            const std::vector<T>& vec, NameFn name_of) {
        std::vector<std::pair<std::string,int>> result;
        result.reserve(vec.size());
        for (size_t i = 0; i < vec.size(); ++i)
            result.emplace_back(name_of(vec[i]), static_cast<int>(i));
        return result;
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
