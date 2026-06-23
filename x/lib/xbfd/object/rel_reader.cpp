// object/rel_reader.cpp — SDCC .rel text-format reader (XL1/XL2/XL4).
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
//
#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <xbfd/xbfd.h>

namespace bfd {
namespace {

enum class rel_version { xl1, xl2, xl4 };

class rel_parser {
public:
    explicit rel_parser(std::string src) : src_(std::move(src)) {}

    xbfd::object parse(std::istream& in) {
        int lineno = 0;
        std::string line;
        while (std::getline(in, line)) {
            ++lineno;
            line = trim(line);
            if (line.empty() || line[0] == ';') continue;
            dispatch(line, lineno);
        }
        flush();
        return std::move(obj_);
    }

private:
    static std::string lower_copy(std::string value) {
        std::transform(value.begin(), value.end(), value.begin(),
                       [](unsigned char ch) {
                           return static_cast<char>(std::tolower(ch));
                       });
        return value;
    }

    static xbfd::section_flags classify_named_flags(const std::string& name,
                                                    xbfd::section_flags flags)
    {
        const std::string lower = lower_copy(name);
        if (lower == "_code" || lower == ".text"
            || lower == "_gsinit" || lower == "_gsfinal") {
            return flags | xbfd::section_flags::code;
        }
        if (lower == "_bss" || lower == ".bss" || lower == "_heap") {
            return flags | xbfd::section_flags::data
                         | xbfd::section_flags::never_load;
        }
        if (lower == "_data" || lower == ".data"
            || lower == "_initialized" || lower == "_dabs") {
            return flags | xbfd::section_flags::data;
        }
        if (lower == "_const" || lower == ".rodata"
            || lower == "_initializer") {
            return flags | xbfd::section_flags::readonly;
        }
        return flags;
    }

    void dispatch(const std::string& line, int lineno) {
        const char        rec  = line[0];
        const std::string rest = line.size() > 2 ? line.substr(2) : "";
        switch (rec) {
        case 'X': on_version(line);        break;
        case 'M': on_module(rest);         break;
        case 'O': on_option(line);         break;
        case 'A': on_area(rest, lineno);   break;
        case 'S': on_symbol(rest, lineno); break;
        case 'T': on_text(rest, lineno);   break;
        case 'R': on_reloc(rest, lineno);  break;
        default:  break;
        }
    }

    void on_version(const std::string& line) {
        if (line.size() < 2 || line[1] != 'L') return;
        if (line.size() >= 3 && line[2] == '4') ver_ = rel_version::xl4;
        else if (line.size() >= 3 && line[2] == '2') ver_ = rel_version::xl2;
    }
    void on_module(const std::string& rest) {
        const auto toks = split(rest);
        if (!toks.empty()) obj_.module_name = toks[0];
    }
    void on_option(const std::string& text) {
        if (text.find("sdcccall(0)") != std::string::npos) {
            obj_.default_calling_convention = xbfd::calling_convention::xcc_sdcccall0;
        } else if (text.find("sdcccall(1)") != std::string::npos) {
            obj_.default_calling_convention = xbfd::calling_convention::xcc_sdcccall1;
        } else if (text.find("z88dk::fastcall") != std::string::npos) {
            obj_.default_calling_convention = xbfd::calling_convention::xcc_z88dk_fastcall;
        } else if (text.find("z88dk::callee") != std::string::npos) {
            obj_.default_calling_convention = xbfd::calling_convention::xcc_z88dk_callee;
        }
    }
    void on_area(const std::string& rest, int lineno) {
        const auto toks = split(rest);
        if (toks.size() < 5) throw xbfd::format_error(src_, lineno, "malformed A record");
        xbfd::section sec;
        sec.name  = toks[0];
        sec.size  = hex16(toks[2]);
        sec.flags = classify_named_flags(sec.name, decode_area_flags(hex8(toks[4])));
        if (toks.size() >= 7 && toks[5] == "addr") sec.vma = hex16(toks[6]);
        area_names_.push_back(sec.name);
        obj_.sections.push_back(std::move(sec));
    }
    void on_symbol(const std::string& rest, int lineno) {
        const auto toks = split(rest);
        if (toks.size() < 2) throw xbfd::format_error(src_, lineno, "malformed S record");
        const bool is_def = toks[1].substr(0, 3) == "Def";
        if (!is_def && toks[1].substr(0, 3) != "Ref")
            throw xbfd::format_error(src_, lineno, "unknown symbol type in S record");
        xbfd::symbol sym;
        sym.name  = toks[0];
        sym.value = hex16(toks[1].substr(3));
        sym.flags = is_def ? xbfd::symbol_flags::global
                           : (xbfd::symbol_flags::global | xbfd::symbol_flags::undefined);
        if (is_def && !area_names_.empty()) sym.section_name = area_names_.back();
        obj_.symbols.push_back(std::move(sym));
    }
    void on_text(const std::string& rest, int lineno) {
        const auto toks = split(rest);
        const size_t offlen = (ver_ == rel_version::xl4) ? 4 : 2;
        if (toks.size() < offlen) throw xbfd::format_error(src_, lineno, "malformed T record");
        pending_t pt;
        pt.offset = static_cast<uint16_t>(hex8(toks[0]) | (static_cast<uint16_t>(hex8(toks[1])) << 8));
        for (size_t i = offlen; i < toks.size(); ++i) pt.data.push_back(hex8(toks[i]));
        pending_.push_back(std::move(pt));
        cur_idx_ = pending_.size() - 1;
    }
    void on_reloc(const std::string& rest, int lineno) {
        const auto toks = split(rest);
        if (toks.size() < 4) throw xbfd::format_error(src_, lineno, "malformed R record");
        if (!cur_idx_) return;
        auto& pt = pending_[*cur_idx_];
        const int area_idx = hex8(toks[2]) | (static_cast<int>(hex8(toks[3])) << 8);
        if (area_idx < static_cast<int>(area_names_.size()))
            pt.section_name = area_names_[area_idx];
        for (size_t i = 4; i + 3 < toks.size(); i += 4) {
            const uint8_t mode = hex8(toks[i]);
            const uint8_t b1   = hex8(toks[i + 1]);
            const uint8_t b2   = hex8(toks[i + 2]);
            const uint8_t b3   = hex8(toks[i + 3]);
            xbfd::reloc_type rtype; bool sym_rel;
            decode_mode(mode, ver_ == rel_version::xl4, rtype, sym_rel);
            uint32_t t_off; int ref_idx;
            if (ver_ == rel_version::xl4) {
                t_off = b1 >= 4 ? b1 - 4u : 0u; ref_idx = b2 | (static_cast<int>(b3) << 8);
            } else if (ver_ == rel_version::xl2) {
                t_off = b1; ref_idx = b2 | (static_cast<int>(b3) << 8);
            } else {
                t_off = b1 | (static_cast<uint32_t>(b2) << 8); ref_idx = b3;
            }
            const std::string ref_name = sym_rel
                ? (ref_idx < static_cast<int>(obj_.symbols.size())  ? obj_.symbols[ref_idx].name  : "")
                : (ref_idx < static_cast<int>(area_names_.size())   ? area_names_[ref_idx]         : "");
            if (auto* sec = find_section(pt.section_name)) {
                xbfd::reloc_entry r;
                r.offset = pt.offset + t_off; r.type = rtype;
                r.sym_relative = sym_rel; r.name = ref_name;
                sec->relocs.push_back(r);
            }
        }
    }
    void flush() {
        for (auto& pt : pending_) {
            if (pt.section_name.empty() && !area_names_.empty()) pt.section_name = area_names_[0];
            if (auto* sec = find_section(pt.section_name)) {
                const size_t end = static_cast<size_t>(pt.offset) + pt.data.size();
                if (sec->data.size() < end)
                    sec->data.resize(end, 0);
                std::copy(pt.data.begin(), pt.data.end(),
                          sec->data.begin() + static_cast<size_t>(pt.offset));
                if (sec->size < sec->data.size())
                    sec->size = sec->data.size();
            }
        }

        for (auto& sec : obj_.sections) {
            for (auto& reloc : sec.relocs) {
                if (reloc.offset >= sec.data.size()) {
                    reloc.addend = 0;
                    continue;
                }
                switch (reloc.type) {
                case xbfd::reloc_type::z80_16:
                    if (reloc.offset + 1 < sec.data.size()) {
                        reloc.addend = static_cast<int32_t>(
                            sec.data[reloc.offset]
                            | (static_cast<uint16_t>(sec.data[reloc.offset + 1]) << 8));
                    } else {
                        reloc.addend = static_cast<int32_t>(sec.data[reloc.offset]);
                    }
                    break;
                case xbfd::reloc_type::z80_pc8:
                    reloc.addend = static_cast<int32_t>(
                        static_cast<int8_t>(sec.data[reloc.offset]));
                    break;
                case xbfd::reloc_type::z80_8:
                case xbfd::reloc_type::z80_16_msb:
                default:
                    reloc.addend = static_cast<int32_t>(sec.data[reloc.offset]);
                    break;
                }
            }
        }
    }

    xbfd::section* find_section(const std::string& name) {
        for (auto& s : obj_.sections) if (s.name == name) return &s;
        return nullptr;
    }
    static uint16_t hex16(const std::string& s) { return static_cast<uint16_t>(std::stoul(s, nullptr, 16)); }
    static uint8_t  hex8 (const std::string& s) { return static_cast<uint8_t> (std::stoul(s, nullptr, 16)); }
    static std::vector<std::string> split(const std::string& s) {
        std::vector<std::string> out;
        std::istringstream iss(s);
        std::string tok;
        while (iss >> tok) {
            out.push_back(tok);
        }
        return out;
    }
    static std::string trim(const std::string& s) {
        const auto a = s.find_first_not_of(" \t\r\n");
        if (a == std::string::npos) return {};
        return s.substr(a, s.find_last_not_of(" \t\r\n") - a + 1);
    }
    static xbfd::section_flags decode_area_flags(uint8_t raw) {
        auto f = xbfd::section_flags::alloc | xbfd::section_flags::load | xbfd::section_flags::reloc;
        if (raw & 0x01) f = f | xbfd::section_flags::overlay;
        if (raw & 0x04) f = f | xbfd::section_flags::abs;
        if (raw & 0x08) f = f | xbfd::section_flags::abs;
        return f;
    }
    static void decode_mode(uint8_t mode, bool, xbfd::reloc_type& type, bool& sym_rel) {
        sym_rel            = (mode & 0x02) != 0;
        const bool byte    = (mode & 0x01) != 0;
        const bool pc_rel  = (mode & 0x04) != 0;
        const bool bytx    = (mode & 0x08) != 0;
        const bool msb     = (mode & 0x80) != 0;
        type = pc_rel               ? xbfd::reloc_type::z80_pc8
             : (byte && bytx && msb) ? xbfd::reloc_type::z80_16_msb
             : byte                 ? xbfd::reloc_type::z80_8
                                    : xbfd::reloc_type::z80_16;
    }

    struct pending_t { std::string section_name; uint16_t offset = 0; std::vector<uint8_t> data; };

    std::string              src_;
    xbfd::object             obj_;
    rel_version              ver_       = rel_version::xl1;
    std::vector<std::string> area_names_;
    std::vector<pending_t>   pending_;
    std::optional<size_t>    cur_idx_;
};

} // namespace

void parse_rel(xbfd::object& obj, const std::string& src, std::istream& in) {
    obj = rel_parser(src).parse(in);
}

} // namespace bfd

// -------------------------------------------------------------------------
// xbfd::rel_reader
// -------------------------------------------------------------------------

namespace xbfd {

std::optional<object> rel_reader::read(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return std::nullopt;
    object obj;
    bfd::parse_rel(obj, path, file);
    obj.format  = obj_format::object;
    obj.flavour = obj_flavour::rel;
    return obj;
}

} // namespace xbfd
