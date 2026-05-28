// elf_writer.cpp
//
// Emits a bfd::bfd object as an ELF32 Z80 relocatable object file
// (e_type=ET_REL, e_machine=EM_Z80=220).
//
// Layout of the generated file:
//   ELF header (52 bytes)
//   Section data (one blob per PROGBITS/NOBITS section)
//   .symtab data
//   .strtab data
//   .shstrtab data
//   REL relocation sections (one per PROGBITS section that has relocs)
//   Section header table
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
//
#include <algorithm>
#include <cstring>
#include <iomanip>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

#include <xbfd/bfd.hpp>
#include <xbfd/errors.hpp>

namespace bfd {

    namespace {

        static void write_u8(std::vector<uint8_t>& out, uint8_t v) {
            out.push_back(v);
        }
        static void write_u16le(std::vector<uint8_t>& out, uint16_t v) {
            out.push_back(v & 0xFF);
            out.push_back((v >> 8) & 0xFF);
        }
        static void write_u32le(std::vector<uint8_t>& out, uint32_t v) {
            out.push_back( v        & 0xFF);
            out.push_back((v >>  8) & 0xFF);
            out.push_back((v >> 16) & 0xFF);
            out.push_back((v >> 24) & 0xFF);
        }
        static void pad_to(std::vector<uint8_t>& out, uint32_t align) {
            while (out.size() % align) out.push_back(0);
        }

        // Append a null-terminated string to a strtab blob; return its offset.
        static uint32_t strtab_add(std::vector<uint8_t>& tab,
                                    const std::string& s)
        {
            uint32_t off = static_cast<uint32_t>(tab.size());
            for (char c : s) tab.push_back(static_cast<uint8_t>(c));
            tab.push_back(0);
            return off;
        }

        static uint8_t encode_reloc_type(reloc_type t) {
            switch (t) {
                case reloc_type::z80_8:      return 1;
                case reloc_type::z80_pc8:    return 3;
                case reloc_type::z80_16:     return 4;
                case reloc_type::z80_16_msb: return 5;
                default:                     return 0;
            }
        }

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

        struct shdr_build {
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

        static void emit_shdr(std::vector<uint8_t>& out, const shdr_build& s) {
            write_u32le(out, s.sh_name);
            write_u32le(out, s.sh_type);
            write_u32le(out, s.sh_flags);
            write_u32le(out, s.sh_addr);
            write_u32le(out, s.sh_offset);
            write_u32le(out, s.sh_size);
            write_u32le(out, s.sh_link);
            write_u32le(out, s.sh_info);
            write_u32le(out, s.sh_addralign);
            write_u32le(out, s.sh_entsize);
        }

    } // anonymous namespace

    // -------------------------------------------------------------------------
    // emit_elf — serialize bfd object to ELF32 byte stream
    // -------------------------------------------------------------------------

    void emit_elf(const bfd& obj, std::ostream& out_stream)
    {
        std::vector<uint8_t> buf;
        buf.reserve(4096);

        // We build the ELF in memory, then write at the end.
        // Placeholder for the 52-byte ELF header (filled in at the end).
        buf.resize(52, 0);

        // ---- Build string tables ----
        std::vector<uint8_t> shstrtab_data;
        strtab_add(shstrtab_data, "");          // index 0: empty

        std::vector<uint8_t> strtab_data;
        strtab_add(strtab_data, "");            // index 0: empty

        // Pre-reserve section name offsets.
        std::vector<uint32_t> sec_shname;
        for (const auto& sec : obj.sections())
            sec_shname.push_back(strtab_add(shstrtab_data, sec.name()));

        uint32_t shname_symtab  = strtab_add(shstrtab_data, ".symtab");
        uint32_t shname_strtab  = strtab_add(shstrtab_data, ".strtab");
        uint32_t shname_shstrtab= strtab_add(shstrtab_data, ".shstrtab");

        // rel section names: ".rel" + sec.name()
        std::vector<uint32_t> rel_shnames;
        for (const auto& sec : obj.sections()) {
            if (sec.relocs().empty())
                rel_shnames.push_back(0);
            else
                rel_shnames.push_back(
                    strtab_add(shstrtab_data, ".rel" + sec.name()));
        }

        // ---- Collect symbols; build strtab ----
        // Each symbol entry: (strtab_offset, sym_flags, value, shndx)
        struct sym_entry {
            uint32_t st_name;
            uint32_t st_value;
            uint8_t  st_info;
            uint16_t st_shndx;
        };
        std::vector<sym_entry> sym_entries;
        // sym[0] = UNDEF
        sym_entries.push_back({0, 0, 0, 0});

        // Build section name → ELF section index map (1-based, skipping NULL).
        std::vector<uint16_t> sec_shidx(obj.sections().size());
        for (size_t i = 0; i < obj.sections().size(); ++i)
            sec_shidx[i] = static_cast<uint16_t>(1 + i);

        // Local symbols first, then global.
        auto add_syms = [&](bool want_global) {
            for (const auto& sym : obj.symbols()) {
                if (sym.is_global() != want_global) continue;
                uint32_t name_off = strtab_add(strtab_data, sym.name());
                uint8_t  bind  = want_global ? 1 : 0; // STB_GLOBAL / LOCAL
                uint8_t  stype = 0;                    // STT_NOTYPE
                uint8_t  info  = static_cast<uint8_t>((bind << 4) | stype);
                uint16_t shndx = 0;
                if (!sym.is_absolute() && !sym.section_name().empty()) {
                    for (size_t i = 0; i < obj.sections().size(); ++i) {
                        if (obj.sections()[i].name() == sym.section_name()) {
                            shndx = sec_shidx[i];
                            break;
                        }
                    }
                } else if (sym.is_absolute()) {
                    shndx = 0xFFF1; // SHN_ABS
                }
                sym_entries.push_back({name_off,
                                       static_cast<uint32_t>(sym.value()),
                                       info, shndx});
            }
        };
        add_syms(false); // locals
        uint32_t first_global = static_cast<uint32_t>(sym_entries.size());
        add_syms(true);  // globals

        // ---- Emit section data ----
        std::vector<uint32_t> sec_offsets(obj.sections().size());
        std::vector<uint32_t> sec_sizes(obj.sections().size());

        for (size_t i = 0; i < obj.sections().size(); ++i) {
            const auto& sec = obj.sections()[i];
            pad_to(buf, 4);
            sec_offsets[i] = static_cast<uint32_t>(buf.size());
            const auto& data = sec.contents();
            if (!data.empty()) {
                buf.insert(buf.end(), data.begin(), data.end());
            }
            sec_sizes[i] = static_cast<uint32_t>(
                has_flag(sec.flags(), section_flags::never_load)
                    ? sec.size()
                    : data.size());
        }

        // ---- Emit .symtab ----
        pad_to(buf, 4);
        uint32_t symtab_off = static_cast<uint32_t>(buf.size());
        for (const auto& se : sym_entries) {
            write_u32le(buf, se.st_name);
            write_u32le(buf, se.st_value);
            write_u32le(buf, 0);                // st_size
            write_u8(buf,   se.st_info);
            write_u8(buf,   0);                 // st_other
            write_u16le(buf, se.st_shndx);
        }
        uint32_t symtab_size = static_cast<uint32_t>(buf.size()) - symtab_off;

        // ---- Emit .strtab ----
        uint32_t strtab_off = static_cast<uint32_t>(buf.size());
        buf.insert(buf.end(), strtab_data.begin(), strtab_data.end());
        uint32_t strtab_size = static_cast<uint32_t>(strtab_data.size());

        // ---- Emit REL sections ----
        struct rel_sec_info {
            uint32_t offset;
            uint32_t size;
            uint32_t target_shidx; // index in section header table
        };
        std::vector<rel_sec_info> rel_infos;

        for (size_t i = 0; i < obj.sections().size(); ++i) {
            const auto& sec = obj.sections()[i];
            if (sec.relocs().empty()) {
                rel_infos.push_back({0, 0, 0});
                continue;
            }
            pad_to(buf, 4);
            uint32_t roff = static_cast<uint32_t>(buf.size());
            for (const auto& r : sec.relocs()) {
                // Find symbol index.
                uint32_t sym_idx = 0;
                for (size_t si = 1; si < sym_entries.size(); ++si) {
                    // match by name via strtab
                    const char* p = reinterpret_cast<const char*>(
                        strtab_data.data()) + sym_entries[si].st_name;
                    if (std::string(p) == r.name) {
                        sym_idx = static_cast<uint32_t>(si);
                        break;
                    }
                }
                uint32_t r_info = (sym_idx << 8) | encode_reloc_type(r.type);
                write_u32le(buf, static_cast<uint32_t>(r.offset));
                write_u32le(buf, r_info);
            }
            rel_infos.push_back({roff,
                static_cast<uint32_t>(buf.size()) - roff,
                sec_shidx[i]});
        }

        // ---- Emit .shstrtab ----
        uint32_t shstrtab_off = static_cast<uint32_t>(buf.size());
        buf.insert(buf.end(), shstrtab_data.begin(), shstrtab_data.end());
        uint32_t shstrtab_size= static_cast<uint32_t>(shstrtab_data.size());

        // ---- Section header table ----
        pad_to(buf, 4);
        uint32_t shoff = static_cast<uint32_t>(buf.size());

        // How many section headers: NULL + progbits + symtab + strtab +
        //                           shstrtab + rel sections
        uint32_t nsecs  = static_cast<uint32_t>(obj.sections().size());
        uint32_t nrels  = 0;
        for (auto& ri : rel_infos) nrels += (ri.size > 0) ? 1 : 0;
        uint32_t total_sh = 1 + nsecs + 3 + nrels; // NULL+prog+sym+str+shstr+rels

        // Index assignments:
        //   0          = NULL
        //   1..nsecs   = PROGBITS/NOBITS sections
        //   nsecs+1    = .symtab
        //   nsecs+2    = .strtab
        //   nsecs+3    = .shstrtab
        //   nsecs+4..  = REL sections
        uint32_t symtab_shidx  = nsecs + 1;
        uint32_t strtab_shidx  = nsecs + 2;
        uint32_t shstrtab_shidx= nsecs + 3;

        // SH[0] NULL
        {
            shdr_build s{};
            emit_shdr(buf, s);
        }
        // SH[1..nsecs] content sections
        for (size_t i = 0; i < nsecs; ++i) {
            const auto& sec = obj.sections()[i];
            uint32_t sht = has_flag(sec.flags(), section_flags::never_load)
                           ? SHT_NOBITS : SHT_PROGBITS;
            uint32_t shf = 0;
            if (has_flag(sec.flags(), section_flags::alloc))
                shf |= SHF_ALLOC;
            if (has_flag(sec.flags(), section_flags::code))
                shf |= SHF_EXECINSTR;
            if (has_flag(sec.flags(), section_flags::data))
                shf |= SHF_WRITE;

            shdr_build s{};
            s.sh_name      = sec_shname[i];
            s.sh_type      = sht;
            s.sh_flags     = shf;
            s.sh_addr      = static_cast<uint32_t>(sec.vma());
            s.sh_offset    = sec_offsets[i];
            s.sh_size      = sec_sizes[i];
            s.sh_addralign = 1;
            emit_shdr(buf, s);
        }
        // .symtab
        {
            shdr_build s{};
            s.sh_name      = shname_symtab;
            s.sh_type      = SHT_SYMTAB;
            s.sh_offset    = symtab_off;
            s.sh_size      = symtab_size;
            s.sh_link      = strtab_shidx;
            s.sh_info      = first_global;
            s.sh_addralign = 4;
            s.sh_entsize   = 16;
            emit_shdr(buf, s);
        }
        // .strtab
        {
            shdr_build s{};
            s.sh_name      = shname_strtab;
            s.sh_type      = SHT_STRTAB;
            s.sh_offset    = strtab_off;
            s.sh_size      = strtab_size;
            s.sh_addralign = 1;
            emit_shdr(buf, s);
        }
        // .shstrtab
        {
            shdr_build s{};
            s.sh_name      = shname_shstrtab;
            s.sh_type      = SHT_STRTAB;
            s.sh_offset    = shstrtab_off;
            s.sh_size      = shstrtab_size;
            s.sh_addralign = 1;
            emit_shdr(buf, s);
        }
        // REL sections
        uint32_t rel_sh_base = nsecs + 4; // base index in SH table
        for (size_t i = 0; i < obj.sections().size(); ++i) {
            const auto& ri = rel_infos[i];
            if (ri.size == 0) continue;
            shdr_build s{};
            s.sh_name      = rel_shnames[i];
            s.sh_type      = SHT_REL;
            s.sh_offset    = ri.offset;
            s.sh_size      = ri.size;
            s.sh_link      = symtab_shidx;
            s.sh_info      = ri.target_shidx;
            s.sh_addralign = 4;
            s.sh_entsize   = 8;
            emit_shdr(buf, s);
            (void)rel_sh_base;
        }

        // ---- Fix up ELF header ----
        {
            uint8_t* h = buf.data();
            h[0] = 0x7F; h[1] = 'E'; h[2] = 'L'; h[3] = 'F';
            h[4] = 1;    // ELFCLASS32
            h[5] = 1;    // ELFDATA2LSB
            h[6] = 1;    // EV_CURRENT
            // e_type = ET_REL = 1
            h[16] = 1; h[17] = 0;
            // e_machine = EM_Z80 = 220
            h[18] = static_cast<uint8_t>(EM_Z80 & 0xFF);
            h[19] = static_cast<uint8_t>((EM_Z80 >> 8) & 0xFF);
            // e_version = 1
            h[20] = 1; h[21] = 0; h[22] = 0; h[23] = 0;
            // e_entry, e_phoff = 0
            // e_shoff
            h[32] = static_cast<uint8_t>( shoff        & 0xFF);
            h[33] = static_cast<uint8_t>((shoff >>  8) & 0xFF);
            h[34] = static_cast<uint8_t>((shoff >> 16) & 0xFF);
            h[35] = static_cast<uint8_t>((shoff >> 24) & 0xFF);
            // e_ehsize = 52
            h[40] = 52; h[41] = 0;
            // e_phentsize, e_phnum = 0
            // e_shentsize = 40
            h[46] = 40; h[47] = 0;
            // e_shnum
            h[48] = static_cast<uint8_t>( total_sh       & 0xFF);
            h[49] = static_cast<uint8_t>((total_sh >> 8) & 0xFF);
            // e_shstrndx
            h[50] = static_cast<uint8_t>( shstrtab_shidx       & 0xFF);
            h[51] = static_cast<uint8_t>((shstrtab_shidx >> 8) & 0xFF);
        }

        out_stream.write(reinterpret_cast<const char*>(buf.data()),
                         static_cast<std::streamsize>(buf.size()));
    }

} // namespace bfd
