// binary/elf_reader.cpp — ELF32 Z80 relocatable object reader.
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
//
#include <cstring>
#include <fstream>
#include <iterator>
#include <optional>
#include <string>
#include <vector>

#include <xbfd/xbfd.h>

namespace bfd {
namespace {

// -------------------------------------------------------------------------
// ELF32 on-disk structures (little-endian Z80)
// -------------------------------------------------------------------------

struct elf32_hdr  {
    uint8_t  e_ident[16]; uint16_t e_type, e_machine; uint32_t e_version;
    uint32_t e_entry, e_phoff, e_shoff, e_flags;
    uint16_t e_ehsize, e_phentsize, e_phnum, e_shentsize, e_shnum, e_shstrndx;
};
struct elf32_shdr {
    uint32_t sh_name, sh_type, sh_flags, sh_addr, sh_offset, sh_size;
    uint32_t sh_link, sh_info, sh_addralign, sh_entsize;
};
struct elf32_sym {
    uint32_t st_name, st_value, st_size;
    uint8_t  st_info, st_other; uint16_t st_shndx;
};
struct elf32_rel { uint32_t r_offset, r_info; };

static constexpr uint8_t  ELFMAG0     = 0x7F;
static constexpr uint8_t  ELFCLASS32  = 1;
static constexpr uint8_t  ELFDATA2LSB = 1;
static constexpr uint16_t ET_REL      = 1;
static constexpr uint16_t EM_Z80      = 220;
static constexpr uint32_t SHT_PROGBITS= 1;
static constexpr uint32_t SHT_SYMTAB  = 2;
static constexpr uint32_t SHT_STRTAB  = 3;
static constexpr uint32_t SHT_REL     = 9;
static constexpr uint32_t SHT_NOBITS  = 8;
static constexpr uint32_t SHF_WRITE   = 0x01;
static constexpr uint32_t SHF_ALLOC   = 0x02;
static constexpr uint32_t SHF_EXECINSTR = 0x04;
static constexpr uint16_t SHN_UNDEF   = 0;
static constexpr uint16_t SHN_ABS     = 0xFFF1;
static constexpr uint8_t  STB_GLOBAL  = 1;

static uint16_t u16le(const uint8_t* p) {
    return static_cast<uint16_t>(p[0] | (p[1] << 8));
}
static uint32_t u32le(const uint8_t* p) {
    return p[0] | (p[1]<<8u) | (p[2]<<16u) | (p[3]<<24u);
}

static uint32_t read_uleb128(const std::vector<uint8_t>& data, std::size_t& pos) {
    uint32_t value = 0;
    unsigned shift = 0;
    while (pos < data.size()) {
        uint8_t byte = data[pos++];
        value |= static_cast<uint32_t>(byte & 0x7f) << shift;
        if ((byte & 0x80u) == 0)
            break;
        shift += 7;
    }
    return value;
}

static std::string read_cstring(const std::vector<uint8_t>& data, std::size_t& pos) {
    std::string out;
    while (pos < data.size() && data[pos] != 0)
        out.push_back(static_cast<char>(data[pos++]));
    if (pos < data.size())
        ++pos;
    return out;
}

static uint32_t read_addr(const std::vector<uint8_t>& data,
                          std::size_t& pos,
                          uint8_t addr_size)
{
    if (addr_size == 2) {
        if (pos + 2 > data.size())
            return 0;
        uint32_t value = u16le(data.data() + pos);
        pos += 2;
        return value;
    }
    if (pos + 4 > data.size())
        return 0;
    uint32_t value = u32le(data.data() + pos);
    pos += 4;
    return value;
}

// -------------------------------------------------------------------------
// elf_parser — parses raw ELF bytes into xbfd::object
// -------------------------------------------------------------------------

class elf_parser {
public:
    elf_parser(std::string src, const std::vector<uint8_t>& raw)
        : src_(std::move(src)), raw_(raw) {}

    xbfd::object parse() {
        validate_header();
        load_section_headers();
        load_string_tables();
        load_sections();
        load_symbols();
        load_relocations();
        load_debug_functions();
        return std::move(obj_);
    }

private:
    void validate_header() {
        if (raw_.size() < sizeof(elf32_hdr))
            throw xbfd::format_error(src_, 0, "file too small for ELF header");

        hdr_ = reinterpret_cast<const elf32_hdr*>(raw_.data());
        if (hdr_->e_ident[0] != ELFMAG0 || hdr_->e_ident[1] != 'E'
         || hdr_->e_ident[2] != 'L'     || hdr_->e_ident[3] != 'F')
            throw xbfd::format_error(src_, 0, "not an ELF file");
        if (hdr_->e_ident[4] != ELFCLASS32)
            throw xbfd::format_error(src_, 0, "only ELF32 supported");
        if (hdr_->e_ident[5] != ELFDATA2LSB)
            throw xbfd::format_error(src_, 0, "only little-endian ELF supported");

        const uint16_t type    = u16le(reinterpret_cast<const uint8_t*>(&hdr_->e_type));
        const uint16_t machine = u16le(reinterpret_cast<const uint8_t*>(&hdr_->e_machine));
        if (type    != ET_REL) throw xbfd::format_error(src_, 0, "only ET_REL ELF files supported");
        if (machine != EM_Z80) throw xbfd::format_error(src_, 0, "only EM_Z80 ELF files supported");
    }

    void load_section_headers() {
        shoff_    = u32le(reinterpret_cast<const uint8_t*>(&hdr_->e_shoff));
        shnum_    = u16le(reinterpret_cast<const uint8_t*>(&hdr_->e_shnum));
        shstrndx_ = u16le(reinterpret_cast<const uint8_t*>(&hdr_->e_shstrndx));
        shentsize_= u16le(reinterpret_cast<const uint8_t*>(&hdr_->e_shentsize));

        if (shnum_ == 0) return;
        if (shoff_ + shnum_ * shentsize_ > raw_.size())
            throw xbfd::format_error(src_, 0, "section header table out of range");

        for (int i = 0; i < shnum_; ++i) {
            const uint8_t* p = raw_.data() + shoff_ + i * shentsize_;
            elf32_shdr s{};
            s.sh_name      = u32le(p +  0); s.sh_type   = u32le(p +  4);
            s.sh_flags     = u32le(p +  8); s.sh_addr   = u32le(p + 12);
            s.sh_offset    = u32le(p + 16); s.sh_size   = u32le(p + 20);
            s.sh_link      = u32le(p + 24); s.sh_info   = u32le(p + 28);
            s.sh_addralign = u32le(p + 32); s.sh_entsize= u32le(p + 36);
            shdrs_.push_back(s);
        }
    }

    void load_string_tables() {
        if (shstrndx_ < static_cast<uint32_t>(shnum_)) {
            const auto& sh = shdrs_[shstrndx_];
            if (sh.sh_offset + sh.sh_size <= raw_.size())
                shstrtab_.assign(
                    reinterpret_cast<const char*>(raw_.data() + sh.sh_offset),
                    sh.sh_size);
        }
        for (int i = 0; i < shnum_; ++i) {
            if (shdrs_[i].sh_type == SHT_SYMTAB) {
                symtab_idx_ = i;
                const uint32_t link = shdrs_[i].sh_link;
                if (link < static_cast<uint32_t>(shnum_)) {
                    const auto& st = shdrs_[link];
                    if (st.sh_offset + st.sh_size <= raw_.size())
                        strtab_.assign(
                            reinterpret_cast<const char*>(raw_.data() + st.sh_offset),
                            st.sh_size);
                }
                break;
            }
        }
    }

    void load_sections() {
        shidx_to_bfd_.assign(shnum_, -1);
        for (int i = 0; i < shnum_; ++i) {
            const auto& sh = shdrs_[i];
            if (sh.sh_type == 0) continue;
            if (sh.sh_type == SHT_SYMTAB || sh.sh_type == SHT_STRTAB) continue;
            if (sh.sh_type == SHT_REL) continue;

            const std::string name = strtab_get(shstrtab_, sh.sh_name);
            auto sf = xbfd::section_flags::none;
            if (sh.sh_flags & SHF_ALLOC)     sf = sf | xbfd::section_flags::alloc;
            if (sh.sh_flags & SHF_EXECINSTR) sf = sf | xbfd::section_flags::code;
            if (sh.sh_flags & SHF_WRITE)     sf = sf | xbfd::section_flags::data;
            sf = sf | (sh.sh_type == SHT_NOBITS
                       ? xbfd::section_flags::never_load
                       : xbfd::section_flags::load);

            xbfd::section sec;
            sec.name  = name;
            sec.flags = sf;
            sec.vma   = sh.sh_addr;
            sec.size  = sh.sh_size;

            if (sh.sh_type == SHT_PROGBITS && sh.sh_size > 0
             && sh.sh_offset + sh.sh_size <= raw_.size())
                sec.data.assign(raw_.begin() + sh.sh_offset,
                                raw_.begin() + sh.sh_offset + sh.sh_size);

            shidx_to_bfd_[i] = static_cast<int>(obj_.sections.size());
            obj_.sections.push_back(std::move(sec));
        }
    }

    void load_symbols() {
        if (symtab_idx_ < 0) return;
        const auto& sh  = shdrs_[symtab_idx_];
        const uint32_t ent = sh.sh_entsize ? sh.sh_entsize : sizeof(elf32_sym);
        const uint32_t cnt = sh.sh_size / ent;

        for (uint32_t i = 1; i < cnt; ++i) {
            const uint8_t* p = raw_.data() + sh.sh_offset + i * ent;
            elf32_sym sym{};
            sym.st_name  = u32le(p + 0); sym.st_value = u32le(p + 4);
            sym.st_size  = u32le(p + 8); sym.st_info  = p[12];
            sym.st_other = p[13];        sym.st_shndx = u16le(p + 14);

            const char* nm = strtab_get(strtab_, sym.st_name);
            if (!nm || nm[0] == '\0') continue;

            const bool is_global = (sym.st_info >> 4) == STB_GLOBAL;
            const bool is_undef  = sym.st_shndx == SHN_UNDEF;
            const bool is_abs    = sym.st_shndx == SHN_ABS;

            auto sf = is_global ? xbfd::symbol_flags::global : xbfd::symbol_flags::local;
            if (is_undef) sf = sf | xbfd::symbol_flags::undefined;
            if (is_abs)   sf = sf | xbfd::symbol_flags::absolute;

            std::string sec_name;
            if (!is_undef && !is_abs && sym.st_shndx < static_cast<uint32_t>(shnum_))
                sec_name = strtab_get(shstrtab_, shdrs_[sym.st_shndx].sh_name);

            xbfd::symbol s;
            s.name = nm; s.flags = sf; s.value = sym.st_value; s.section_name = sec_name;
            obj_.symbols.push_back(std::move(s));
        }
    }

    void load_relocations() {
        for (int i = 0; i < shnum_; ++i) {
            const auto& sh = shdrs_[i];
            if (sh.sh_type != SHT_REL) continue;
            const uint32_t target = sh.sh_info;
            if (target >= static_cast<uint32_t>(shnum_)) continue;
            if (shidx_to_bfd_[target] < 0) continue;

            auto& tsec = obj_.sections[shidx_to_bfd_[target]];
            const uint32_t ent = sh.sh_entsize ? sh.sh_entsize : sizeof(elf32_rel);
            const uint32_t cnt = sh.sh_size / ent;

            for (uint32_t j = 0; j < cnt; ++j) {
                const uint8_t* p = raw_.data() + sh.sh_offset + j * ent;
                const uint32_t r_info   = u32le(p + 4);
                const uint32_t sym_idx  = r_info >> 8;
                const uint8_t  rel_kind = static_cast<uint8_t>(r_info & 0xFF);

                std::string ref; bool sym_rel = false;
                if (sym_idx > 0 && sym_idx - 1 < obj_.symbols.size()) {
                    ref = obj_.symbols[sym_idx - 1].name; sym_rel = true;
                }
                xbfd::reloc_entry r;
                r.offset = u32le(p + 0); r.type = decode_reloc(rel_kind);
                r.sym_relative = sym_rel; r.name = ref;
                tsec.relocs.push_back(r);
            }
        }
    }

    void load_debug_functions() {
        const auto *debug_info = find_section_bytes(".debug_info");
        if (!debug_info || debug_info->empty())
            return;

        std::size_t unit_pos = 0;
        while (unit_pos + 11 <= debug_info->size()) {
            if (unit_pos + 4 > debug_info->size())
                break;
            const uint32_t unit_length = u32le(debug_info->data() + unit_pos);
            if (unit_length == 0)
                break;
            const std::size_t unit_end = unit_pos + 4u + unit_length;
            if (unit_end > debug_info->size())
                break;

            const uint16_t version = u16le(debug_info->data() + unit_pos + 4);
            (void)version;
            const uint8_t addr_size = (*debug_info)[unit_pos + 10];
            std::size_t pos = unit_pos + 11;

            const uint32_t cu_abbrev = read_uleb128(*debug_info, pos);
            if (cu_abbrev != 1) {
                unit_pos = unit_end;
                continue;
            }

            if (addr_size == 2) {
                (void)read_cstring(*debug_info, pos); // producer
                pos += 2;                             // language
                (void)read_cstring(*debug_info, pos); // source path
                (void)read_addr(*debug_info, pos, addr_size); // low_pc
                (void)read_addr(*debug_info, pos, addr_size); // high_pc
                pos += 4;                             // stmt_list
            } else {
                (void)read_cstring(*debug_info, pos); // producer
                (void)read_cstring(*debug_info, pos); // source path
                pos += 4;                             // stmt_list
                (void)read_addr(*debug_info, pos, addr_size); // low_pc
                (void)read_addr(*debug_info, pos, addr_size); // high_pc
                pos += 2;                             // language
            }

            while (pos < unit_end) {
                const uint32_t abbrev = read_uleb128(*debug_info, pos);
                if (abbrev == 0)
                    break;
                if (abbrev != 2) {
                    pos = unit_end;
                    break;
                }

                xbfd::debug_function fn;
                fn.name = read_cstring(*debug_info, pos);
                fn.start = read_addr(*debug_info, pos, addr_size);
                fn.end = read_addr(*debug_info, pos, addr_size);
                if (pos < unit_end)
                    ++pos; // external flag
                if (pos < unit_end)
                    fn.convention = static_cast<xbfd::calling_convention>((*debug_info)[pos++]);
                obj_.debug.functions.push_back(std::move(fn));
            }

            unit_pos = unit_end;
        }

        std::optional<xbfd::calling_convention> common;
        for (const auto& fn : obj_.debug.functions) {
            if (fn.convention == xbfd::calling_convention::unknown)
                continue;
            if (!common.has_value()) {
                common = fn.convention;
                continue;
            }
            if (*common != fn.convention) {
                common.reset();
                break;
            }
        }
        if (common.has_value())
            obj_.default_calling_convention = *common;
    }

    const std::vector<uint8_t>* find_section_bytes(const std::string& name) const {
        auto matches = [&](const std::string& sec_name) {
            if (sec_name == name)
                return true;
            if (!name.empty() && name[0] == '.'
                && sec_name == name.substr(1)) {
                return true;
            }
            return false;
        };
        for (const auto& sec : obj_.sections) {
            if (matches(sec.name))
                return &sec.data;
        }
        return nullptr;
    }

    static const char* strtab_get(const std::string& tab, uint32_t off) {
        return off < tab.size() ? tab.c_str() + off : "";
    }
    static xbfd::reloc_type decode_reloc(uint8_t t) {
        switch (t) {
        case 1: return xbfd::reloc_type::z80_8;
        case 3: return xbfd::reloc_type::z80_pc8;
        case 4: return xbfd::reloc_type::z80_16;
        case 5: return xbfd::reloc_type::z80_16_msb;
        default: return xbfd::reloc_type::none;
        }
    }

    std::string                  src_;
    const std::vector<uint8_t>&  raw_;
    const elf32_hdr*             hdr_        = nullptr;
    uint32_t                     shoff_      = 0;
    int                          shnum_      = 0;
    uint16_t                     shstrndx_   = 0;
    uint16_t                     shentsize_  = 0;
    std::vector<elf32_shdr>      shdrs_;
    std::string                  shstrtab_;
    std::string                  strtab_;
    int                          symtab_idx_ = -1;
    std::vector<int>             shidx_to_bfd_;
    xbfd::object                 obj_;
};

} // namespace

void parse_elf(xbfd::object& obj, const std::string& src,
               const std::vector<uint8_t>& raw) {
    obj = elf_parser(src, raw).parse();
}

} // namespace bfd

// -------------------------------------------------------------------------
// xbfd::elf_reader
// -------------------------------------------------------------------------

namespace xbfd {

std::optional<object> elf_reader::read(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return std::nullopt;
    std::string raw_str((std::istreambuf_iterator<char>(file)), {});
    object obj;
    bfd::parse_elf(obj, path, {raw_str.begin(), raw_str.end()});
    obj.format  = obj_format::object;
    obj.flavour = obj_flavour::elf;
    return obj;
}

} // namespace xbfd
