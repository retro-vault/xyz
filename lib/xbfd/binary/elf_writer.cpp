// binary/elf_writer.cpp — ELF32 Z80 relocatable object writer.
//
// Layout: ELF header | section data | .symtab | .strtab | REL sections |
//         .shstrtab | section header table
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
//
#include <algorithm>
#include <cstring>
#include <fstream>
#include <ostream>
#include <string>
#include <vector>

#include <xbfd/xbfd.h>

namespace bfd {
namespace {

static constexpr uint32_t SHT_NULL     = 0;
static constexpr uint32_t SHT_PROGBITS = 1;
static constexpr uint32_t SHT_SYMTAB   = 2;
static constexpr uint32_t SHT_STRTAB   = 3;
static constexpr uint32_t SHT_REL      = 9;
static constexpr uint32_t SHT_NOBITS   = 8;
static constexpr uint32_t SHF_WRITE    = 0x01;
static constexpr uint32_t SHF_ALLOC    = 0x02;
static constexpr uint32_t SHF_EXECINSTR= 0x04;
static constexpr uint16_t EM_Z80       = 220;

// -------------------------------------------------------------------------
// elf_builder — accumulates a byte buffer then writes it as ELF32
// -------------------------------------------------------------------------

class elf_builder {
public:
    void emit(const xbfd::object& obj, std::ostream& out) {
        buf_.reserve(4096);
        buf_.resize(52, 0); // ELF header placeholder

        build_string_tables(obj);
        build_symbol_table(obj);
        build_section_data(obj);
        build_rel_sections(obj);
        build_shstrtab();
        build_section_headers(obj);
        patch_elf_header(obj);

        out.write(reinterpret_cast<const char*>(buf_.data()),
                  static_cast<std::streamsize>(buf_.size()));
    }

private:
    void build_string_tables(const xbfd::object& obj) {
        strtab_add(shstrtab_, "");
        strtab_add(strtab_,   "");
        for (const auto& sec : obj.sections)
            sec_shnames_.push_back(strtab_add(shstrtab_, sec.name));
        shname_symtab_   = strtab_add(shstrtab_, ".symtab");
        shname_strtab_   = strtab_add(shstrtab_, ".strtab");
        shname_shstrtab_ = strtab_add(shstrtab_, ".shstrtab");
        for (const auto& sec : obj.sections)
            rel_shnames_.push_back(
                sec.relocs.empty() ? 0 : strtab_add(shstrtab_, ".rel" + sec.name));
    }

    void build_symbol_table(const xbfd::object& obj) {
        sym_entries_.push_back({0, 0, 0, 0});
        sec_shidx_.resize(obj.sections.size());
        for (size_t i = 0; i < obj.sections.size(); ++i)
            sec_shidx_[i] = static_cast<uint16_t>(1 + i);

        auto add_syms = [&](bool want_global) {
            for (const auto& sym : obj.symbols) {
                if (sym.is_global() != want_global) continue;
                const uint32_t name_off = strtab_add(strtab_, sym.name);
                const uint8_t  bind     = want_global ? 1 : 0;
                const uint8_t  info     = static_cast<uint8_t>(bind << 4);
                uint16_t       shndx    = 0;
                if (!sym.is_absolute() && !sym.section_name.empty())
                    for (size_t i = 0; i < obj.sections.size(); ++i)
                        if (obj.sections[i].name == sym.section_name)
                            { shndx = sec_shidx_[i]; break; }
                if (sym.is_absolute()) shndx = 0xFFF1;
                sym_entries_.push_back({name_off, static_cast<uint32_t>(sym.value), info, shndx});
            }
        };
        add_syms(false);
        first_global_ = static_cast<uint32_t>(sym_entries_.size());
        add_syms(true);
    }

    void build_section_data(const xbfd::object& obj) {
        sec_offsets_.resize(obj.sections.size());
        sec_sizes_.resize(obj.sections.size());
        for (size_t i = 0; i < obj.sections.size(); ++i) {
            const auto& sec = obj.sections[i];
            pad_to(4);
            sec_offsets_[i] = static_cast<uint32_t>(buf_.size());
            buf_.insert(buf_.end(), sec.data.begin(), sec.data.end());
            sec_sizes_[i] = static_cast<uint32_t>(
                has_flag(sec.flags, xbfd::section_flags::never_load)
                    ? sec.size : sec.data.size());
        }
        pad_to(4);
        symtab_off_ = static_cast<uint32_t>(buf_.size());
        for (const auto& se : sym_entries_) {
            write_u32le(se.st_name); write_u32le(se.st_value);
            write_u32le(0);          write_u8(se.st_info);
            write_u8(0);             write_u16le(se.st_shndx);
        }
        symtab_size_ = static_cast<uint32_t>(buf_.size()) - symtab_off_;
        strtab_off_  = static_cast<uint32_t>(buf_.size());
        buf_.insert(buf_.end(), strtab_.begin(), strtab_.end());
        strtab_size_ = static_cast<uint32_t>(strtab_.size());
    }

    void build_rel_sections(const xbfd::object& obj) {
        rel_infos_.resize(obj.sections.size(), {0, 0, 0});
        for (size_t i = 0; i < obj.sections.size(); ++i) {
            const auto& sec = obj.sections[i];
            if (sec.relocs.empty()) continue;
            pad_to(4);
            const uint32_t roff = static_cast<uint32_t>(buf_.size());
            for (const auto& r : sec.relocs) {
                uint32_t sym_idx = 0;
                for (size_t si = 1; si < sym_entries_.size(); ++si) {
                    const char* nm = reinterpret_cast<const char*>(strtab_.data())
                                   + sym_entries_[si].st_name;
                    if (std::string(nm) == r.name) { sym_idx = static_cast<uint32_t>(si); break; }
                }
                write_u32le(static_cast<uint32_t>(r.offset));
                write_u32le((sym_idx << 8) | encode_reloc(r.type));
            }
            rel_infos_[i] = {roff, static_cast<uint32_t>(buf_.size()) - roff, sec_shidx_[i]};
        }
    }

    void build_shstrtab() {
        shstrtab_off_  = static_cast<uint32_t>(buf_.size());
        buf_.insert(buf_.end(), shstrtab_.begin(), shstrtab_.end());
        shstrtab_size_ = static_cast<uint32_t>(shstrtab_.size());
    }

    void build_section_headers(const xbfd::object& obj) {
        pad_to(4);
        shoff_ = static_cast<uint32_t>(buf_.size());
        const auto     nsecs         = static_cast<uint32_t>(obj.sections.size());
        const uint32_t symtab_shidx  = nsecs + 1;
        const uint32_t strtab_shidx  = nsecs + 2;

        emit_shdr({});
        for (size_t i = 0; i < nsecs; ++i) {
            const auto& sec = obj.sections[i];
            const uint32_t sht = has_flag(sec.flags, xbfd::section_flags::never_load) ? SHT_NOBITS : SHT_PROGBITS;
            uint32_t shf = 0;
            if (has_flag(sec.flags, xbfd::section_flags::alloc)) shf |= SHF_ALLOC;
            if (has_flag(sec.flags, xbfd::section_flags::code))  shf |= SHF_EXECINSTR;
            if (has_flag(sec.flags, xbfd::section_flags::data))  shf |= SHF_WRITE;
            emit_shdr({sec_shnames_[i], sht, shf, static_cast<uint32_t>(sec.vma),
                       sec_offsets_[i], sec_sizes_[i]});
        }
        emit_shdr({shname_symtab_, SHT_SYMTAB, 0, 0, symtab_off_, symtab_size_,
                   strtab_shidx, first_global_, 4, 16});
        emit_shdr({shname_strtab_,   SHT_STRTAB, 0, 0, strtab_off_,   strtab_size_});
        emit_shdr({shname_shstrtab_, SHT_STRTAB, 0, 0, shstrtab_off_, shstrtab_size_});
        for (size_t i = 0; i < obj.sections.size(); ++i) {
            const auto& ri = rel_infos_[i];
            if (ri.size == 0) continue;
            emit_shdr({rel_shnames_[i], SHT_REL, 0, 0, ri.offset, ri.size,
                       symtab_shidx, ri.target_shidx, 4, 8});
        }
    }

    void patch_elf_header(const xbfd::object& obj) {
        const auto nsecs  = static_cast<uint32_t>(obj.sections.size());
        uint32_t nrels = 0;
        for (const auto& ri : rel_infos_) nrels += (ri.size > 0) ? 1 : 0;
        const uint16_t total_sh = static_cast<uint16_t>(1 + nsecs + 3 + nrels);
        const uint16_t shstrndx = static_cast<uint16_t>(nsecs + 3);

        uint8_t* h = buf_.data();
        h[0]=0x7F; h[1]='E'; h[2]='L'; h[3]='F';
        h[4]=1; h[5]=1; h[6]=1;
        std::fill(h+7, h+16, 0);
        wu16le(h+16, 1); wu16le(h+18, EM_Z80); wu32le(h+20, 1);
        wu32le(h+24, 0); wu32le(h+28, 0); wu32le(h+32, shoff_);
        wu32le(h+36, 0); wu16le(h+40, 52); wu16le(h+42, 0);
        wu16le(h+44, 0); wu16le(h+46, 40); wu16le(h+48, total_sh);
        wu16le(h+50, shstrndx);
    }

    struct sym_entry { uint32_t st_name, st_value; uint8_t st_info; uint16_t st_shndx; };
    struct rel_info  { uint32_t offset, size, target_shidx; };

    void write_u8  (uint8_t  v) { buf_.push_back(v); }
    void write_u16le(uint16_t v) { buf_.push_back(v&0xFF); buf_.push_back(v>>8); }
    void write_u32le(uint32_t v) {
        buf_.push_back(v&0xFF); buf_.push_back((v>>8)&0xFF);
        buf_.push_back((v>>16)&0xFF); buf_.push_back((v>>24)&0xFF);
    }
    static void wu16le(uint8_t* p, uint16_t v) { p[0]=v&0xFF; p[1]=v>>8; }
    static void wu32le(uint8_t* p, uint32_t v) {
        p[0]=v&0xFF; p[1]=(v>>8)&0xFF; p[2]=(v>>16)&0xFF; p[3]=(v>>24)&0xFF;
    }
    void pad_to(uint32_t align) { while (buf_.size() % align) buf_.push_back(0); }

    struct shdr {
        uint32_t name=0, type=0, flags=0, addr=0, offset=0,
                 size=0, link=0, info=0, addralign=1, entsize=0;
    };
    void emit_shdr(const shdr& s) {
        write_u32le(s.name);   write_u32le(s.type);  write_u32le(s.flags);
        write_u32le(s.addr);   write_u32le(s.offset);write_u32le(s.size);
        write_u32le(s.link);   write_u32le(s.info);  write_u32le(s.addralign);
        write_u32le(s.entsize);
    }

    static uint32_t strtab_add(std::vector<uint8_t>& tab, const std::string& s) {
        const uint32_t off = static_cast<uint32_t>(tab.size());
        for (char c : s) tab.push_back(static_cast<uint8_t>(c));
        tab.push_back(0);
        return off;
    }
    static uint8_t encode_reloc(xbfd::reloc_type t) {
        switch (t) {
        case xbfd::reloc_type::z80_8:      return 1;
        case xbfd::reloc_type::z80_pc8:    return 3;
        case xbfd::reloc_type::z80_16:     return 4;
        case xbfd::reloc_type::z80_16_msb: return 5;
        default:                           return 0;
        }
    }

    std::vector<uint8_t>     buf_;
    std::vector<uint8_t>     shstrtab_, strtab_;
    std::vector<uint32_t>    sec_shnames_, rel_shnames_;
    uint32_t                 shname_symtab_=0, shname_strtab_=0, shname_shstrtab_=0;
    std::vector<sym_entry>   sym_entries_;
    std::vector<uint16_t>    sec_shidx_;
    uint32_t                 first_global_=0;
    std::vector<uint32_t>    sec_offsets_, sec_sizes_;
    uint32_t                 symtab_off_=0, symtab_size_=0;
    uint32_t                 strtab_off_=0, strtab_size_=0;
    uint32_t                 shstrtab_off_=0, shstrtab_size_=0;
    uint32_t                 shoff_=0;
    std::vector<rel_info>    rel_infos_;
};

} // namespace

void emit_elf(const xbfd::object& obj, std::ostream& out) {
    elf_builder{}.emit(obj, out);
}

} // namespace bfd

// -------------------------------------------------------------------------
// xbfd::elf_writer
// -------------------------------------------------------------------------

namespace xbfd {

void elf_writer::write(const std::string& path, const object& obj) {
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f.is_open()) throw io_error("cannot write: " + path);
    bfd::emit_elf(obj, f);
}

} // namespace xbfd
