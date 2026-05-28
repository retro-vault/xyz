// elf_reader.cpp
//
// Reads ELF32 Z80 relocatable object files (e_machine = EM_Z80 = 220) into
// a bfd::bfd handle.  Only ET_REL (relocatable) files are supported; shared
// objects and executables are rejected.
//
// ELF32 structure:
//   52-byte header → section-header table → sections (PROGBITS, SYMTAB, etc.)
//   REL relocation entries use r_info = (sym_idx << 8) | type.
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
//
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

#include <xbfd/bfd.hpp>
#include <xbfd/errors.hpp>

namespace bfd {

    // -------------------------------------------------------------------------
    // ELF32 on-disk structures (little-endian)
    // -------------------------------------------------------------------------

    namespace {

        struct elf32_header {
            uint8_t  e_ident[16];
            uint16_t e_type;
            uint16_t e_machine;
            uint32_t e_version;
            uint32_t e_entry;
            uint32_t e_phoff;
            uint32_t e_shoff;
            uint32_t e_flags;
            uint16_t e_ehsize;
            uint16_t e_phentsize;
            uint16_t e_phnum;
            uint16_t e_shentsize;
            uint16_t e_shnum;
            uint16_t e_shstrndx;
        };

        struct elf32_shdr {
            uint32_t sh_name;
            uint32_t sh_type;
            uint32_t sh_flags;
            uint32_t sh_addr;
            uint32_t sh_offset;
            uint32_t sh_size;
            uint32_t sh_link;
            uint32_t sh_info;
            uint32_t sh_addralign;
            uint32_t sh_entsize;
        };

        struct elf32_sym {
            uint32_t st_name;
            uint32_t st_value;
            uint32_t st_size;
            uint8_t  st_info;
            uint8_t  st_other;
            uint16_t st_shndx;
        };

        struct elf32_rel {
            uint32_t r_offset;
            uint32_t r_info;
        };

        // ELF constants.
        static constexpr uint8_t  ELFMAG0    = 0x7F;
        static constexpr uint8_t  ELFMAG1    = 'E';
        static constexpr uint8_t  ELFMAG2    = 'L';
        static constexpr uint8_t  ELFMAG3    = 'F';
        static constexpr uint8_t  ELFCLASS32 = 1;
        static constexpr uint8_t  ELFDATA2LSB= 1;
        static constexpr uint16_t ET_REL     = 1;
        static constexpr uint16_t EM_Z80     = 220;
        static constexpr uint32_t SHT_NULL   = 0;
        static constexpr uint32_t SHT_PROGBITS = 1;
        static constexpr uint32_t SHT_SYMTAB = 2;
        static constexpr uint32_t SHT_STRTAB = 3;
        static constexpr uint32_t SHT_REL    = 9;
        static constexpr uint32_t SHT_NOBITS = 8;
        static constexpr uint32_t SHF_WRITE  = 0x01;
        static constexpr uint32_t SHF_ALLOC  = 0x02;
        static constexpr uint32_t SHF_EXECINSTR = 0x04;
        static constexpr uint16_t SHN_UNDEF  = 0;
        static constexpr uint16_t SHN_ABS    = 0xFFF1;
        static constexpr uint8_t  STB_LOCAL  = 0;
        static constexpr uint8_t  STB_GLOBAL = 1;

        static uint16_t read_u16le(const uint8_t* p) {
            return static_cast<uint16_t>(p[0] | (p[1] << 8));
        }
        static uint32_t read_u32le(const uint8_t* p) {
            return static_cast<uint32_t>(
                p[0] | (p[1]<<8) | (p[2]<<16) | (p[3]<<24));
        }

        static const char* strtab_get(const std::string& tab, uint32_t off) {
            if (off >= tab.size()) return "";
            return tab.c_str() + off;
        }

        static reloc_type decode_z80_reloc(uint8_t type) {
            switch (type) {
                case 1: return reloc_type::z80_8;
                case 3: return reloc_type::z80_pc8;
                case 4: return reloc_type::z80_16;
                case 5: return reloc_type::z80_16_msb;
                default: return reloc_type::none;
            }
        }

    } // anonymous namespace

    // -------------------------------------------------------------------------
    // parse_elf — fills a bfd object from raw ELF32 bytes
    // -------------------------------------------------------------------------

    void parse_elf(bfd& obj, const std::string& src_name,
                   const std::vector<uint8_t>& raw)
    {
        if (raw.size() < sizeof(elf32_header))
            throw format_error(src_name, 0, "file too small for ELF header");

        const auto* hdr = reinterpret_cast<const elf32_header*>(raw.data());

        if (hdr->e_ident[0] != ELFMAG0 || hdr->e_ident[1] != ELFMAG1 ||
            hdr->e_ident[2] != ELFMAG2 || hdr->e_ident[3] != ELFMAG3)
            throw format_error(src_name, 0, "not an ELF file");

        if (hdr->e_ident[4] != ELFCLASS32)
            throw format_error(src_name, 0, "only ELF32 supported");
        if (hdr->e_ident[5] != ELFDATA2LSB)
            throw format_error(src_name, 0, "only little-endian ELF supported");

        uint16_t e_type    = read_u16le(reinterpret_cast<const uint8_t*>(&hdr->e_type));
        uint16_t e_machine = read_u16le(reinterpret_cast<const uint8_t*>(&hdr->e_machine));

        if (e_type != ET_REL)
            throw format_error(src_name, 0, "only ET_REL ELF files supported");
        if (e_machine != EM_Z80)
            throw format_error(src_name, 0, "only EM_Z80 ELF files supported");

        uint32_t shoff    = read_u32le(reinterpret_cast<const uint8_t*>(&hdr->e_shoff));
        uint16_t shnum    = read_u16le(reinterpret_cast<const uint8_t*>(&hdr->e_shnum));
        uint16_t shstrndx = read_u16le(reinterpret_cast<const uint8_t*>(&hdr->e_shstrndx));
        uint16_t shentsize= read_u16le(reinterpret_cast<const uint8_t*>(&hdr->e_shentsize));

        if (shnum == 0) return;
        if (shoff + shnum * shentsize > raw.size())
            throw format_error(src_name, 0, "section header table out of range");

        // Load section headers.
        std::vector<elf32_shdr> shdrs(shnum);
        for (int i = 0; i < shnum; ++i) {
            const uint8_t* p = raw.data() + shoff + i * shentsize;
            auto& s = shdrs[i];
            s.sh_name      = read_u32le(p +  0);
            s.sh_type      = read_u32le(p +  4);
            s.sh_flags     = read_u32le(p +  8);
            s.sh_addr      = read_u32le(p + 12);
            s.sh_offset    = read_u32le(p + 16);
            s.sh_size      = read_u32le(p + 20);
            s.sh_link      = read_u32le(p + 24);
            s.sh_info      = read_u32le(p + 28);
            s.sh_addralign = read_u32le(p + 32);
            s.sh_entsize   = read_u32le(p + 36);
        }

        // Load section-header string table (.shstrtab).
        std::string shstrtab;
        if (shstrndx < shnum) {
            const auto& sh = shdrs[shstrndx];
            if (sh.sh_offset + sh.sh_size <= raw.size())
                shstrtab.assign(
                    reinterpret_cast<const char*>(raw.data() + sh.sh_offset),
                    sh.sh_size);
        }

        // Locate .symtab and its .strtab.
        int symtab_idx = -1;
        std::string strtab;
        for (int i = 0; i < shnum; ++i) {
            if (shdrs[i].sh_type == SHT_SYMTAB) {
                symtab_idx = i;
                uint32_t strtab_idx = shdrs[i].sh_link;
                if (strtab_idx < static_cast<uint32_t>(shnum)) {
                    const auto& st = shdrs[strtab_idx];
                    if (st.sh_offset + st.sh_size <= raw.size())
                        strtab.assign(
                            reinterpret_cast<const char*>(
                                raw.data() + st.sh_offset),
                            st.sh_size);
                }
                break;
            }
        }

        // Build section name → bfd section index map.
        // Index 0 = SHN_UNDEF; we skip it.
        std::vector<int> shidx_to_bfd(shnum, -1);

        for (int i = 1; i < shnum; ++i) {
            const auto& sh = shdrs[i];
            uint32_t t = sh.sh_type;
            if (t == SHT_NULL || t == SHT_SYMTAB ||
                t == SHT_STRTAB || t == SHT_REL)
                continue;

            std::string name = strtab_get(shstrtab, sh.sh_name);

            section_flags sf = section_flags::reloc;
            if (sh.sh_flags & SHF_ALLOC)    sf = sf | section_flags::alloc;
            if (sh.sh_flags & SHF_EXECINSTR) sf = sf | section_flags::code;
            if (sh.sh_flags & SHF_WRITE)     sf = sf | section_flags::data;
            if (t == SHT_NOBITS)             sf = sf | section_flags::never_load;
            else                             sf = sf | section_flags::load;

            auto& sec = obj.add_section(name, sf,
                                         static_cast<uint64_t>(sh.sh_addr));
            sec.set_size(sh.sh_size);

            // Load content for PROGBITS sections.
            if (t == SHT_PROGBITS && sh.sh_size > 0) {
                if (sh.sh_offset + sh.sh_size <= raw.size()) {
                    std::vector<uint8_t> data(
                        raw.begin() + sh.sh_offset,
                        raw.begin() + sh.sh_offset + sh.sh_size);
                    sec.set_contents(std::move(data));
                }
            }

            shidx_to_bfd[i] = static_cast<int>(obj.sections().size()) - 1;
        }

        // Load symbols from .symtab.
        std::vector<std::string> sym_sec_names; // per symbol: owning section
        if (symtab_idx >= 0) {
            const auto& sh = shdrs[symtab_idx];
            uint32_t ent = sh.sh_entsize ? sh.sh_entsize : sizeof(elf32_sym);
            uint32_t cnt = sh.sh_size / ent;
            for (uint32_t i = 1; i < cnt; ++i) { // skip sym[0] (UNDEF)
                const uint8_t* p = raw.data() + sh.sh_offset + i * ent;
                elf32_sym sym;
                sym.st_name  = read_u32le(p +  0);
                sym.st_value = read_u32le(p +  4);
                sym.st_size  = read_u32le(p +  8);
                sym.st_info  = p[12];
                sym.st_other = p[13];
                sym.st_shndx = read_u16le(p + 14);

                const char* sym_name = strtab_get(strtab, sym.st_name);
                if (!sym_name || sym_name[0] == '\0') continue;

                uint8_t bind = sym.st_info >> 4;
                bool is_global = (bind == STB_GLOBAL);
                bool is_undef  = (sym.st_shndx == SHN_UNDEF);
                bool is_abs    = (sym.st_shndx == SHN_ABS);

                symbol_flags sf = is_global ? symbol_flags::global
                                            : symbol_flags::local;
                if (is_undef) sf = sf | symbol_flags::undefined;
                if (is_abs)   sf = sf | symbol_flags::absolute;

                std::string sec_name;
                if (!is_undef && !is_abs &&
                    sym.st_shndx < static_cast<uint32_t>(shnum)) {
                    sec_name = strtab_get(shstrtab,
                                          shdrs[sym.st_shndx].sh_name);
                }

                obj.add_symbol(sym_name, sf,
                               static_cast<uint64_t>(sym.st_value), sec_name);
            }
        }

        // Load REL sections and attach relocations.
        for (int i = 0; i < shnum; ++i) {
            const auto& sh = shdrs[i];
            if (sh.sh_type != SHT_REL) continue;

            // The section this REL applies to.
            uint32_t target_shidx = sh.sh_info;
            if (target_shidx >= static_cast<uint32_t>(shnum)) continue;
            if (shidx_to_bfd[target_shidx] < 0) continue;

            auto& target_sec = obj.sections()[shidx_to_bfd[target_shidx]];

            uint32_t ent = sh.sh_entsize ? sh.sh_entsize : sizeof(elf32_rel);
            uint32_t cnt = sh.sh_size / ent;
            for (uint32_t j = 0; j < cnt; ++j) {
                const uint8_t* p = raw.data() + sh.sh_offset + j * ent;
                elf32_rel er;
                er.r_offset = read_u32le(p + 0);
                er.r_info   = read_u32le(p + 4);

                uint32_t sym_idx  = er.r_info >> 8;
                uint8_t  rel_kind = static_cast<uint8_t>(er.r_info & 0xFF);

                std::string ref_name;
                bool sym_rel = false;
                if (sym_idx > 0) {
                    auto& syms = obj.symbols();
                    if (sym_idx - 1 < syms.size()) {
                        ref_name = syms[sym_idx - 1].name();
                        sym_rel  = true;
                    }
                }

                reloc r;
                r.offset       = er.r_offset;
                r.type         = decode_z80_reloc(rel_kind);
                r.sym_relative = sym_rel;
                r.name         = ref_name;
                r.addend       = 0;
                target_sec.add_reloc(r);
            }
        }
    }

} // namespace bfd
