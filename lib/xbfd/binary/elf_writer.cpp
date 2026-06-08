// binary/elf_writer.cpp — ELF32 Z80 relocatable object writer.
//
// Layout: ELF header | section data | .symtab | .strtab | REL sections |
//         .shstrtab | section header table
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
//
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <ostream>
#include <set>
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

static constexpr uint16_t DW_TAG_compile_unit = 0x11;
static constexpr uint16_t DW_TAG_subprogram   = 0x2e;
static constexpr uint16_t DW_AT_name          = 0x03;
static constexpr uint16_t DW_AT_stmt_list     = 0x10;
static constexpr uint16_t DW_AT_low_pc        = 0x11;
static constexpr uint16_t DW_AT_high_pc       = 0x12;
static constexpr uint16_t DW_AT_language      = 0x13;
static constexpr uint16_t DW_AT_producer      = 0x25;
static constexpr uint16_t DW_AT_calling_convention = 0x36;
static constexpr uint16_t DW_AT_external      = 0x3f;
static constexpr uint8_t  DW_FORM_addr        = 0x01;
static constexpr uint8_t  DW_FORM_data2       = 0x05;
static constexpr uint8_t  DW_FORM_data4       = 0x06;
static constexpr uint8_t  DW_FORM_string      = 0x08;
static constexpr uint8_t  DW_FORM_data1       = 0x0b;
static constexpr uint8_t  DW_FORM_flag        = 0x0c;
static constexpr uint8_t  DW_LNS_copy         = 1;
static constexpr uint8_t  DW_LNS_advance_pc   = 2;
static constexpr uint8_t  DW_LNS_advance_line = 3;
static constexpr uint8_t  DW_LNS_set_file     = 4;
static constexpr uint8_t  DW_LNS_set_column   = 5;
static constexpr uint8_t  DW_LNS_negate_stmt  = 6;
static constexpr uint8_t  DW_LNS_set_basic_block = 7;
static constexpr uint8_t  DW_LNS_const_add_pc = 8;
static constexpr uint8_t  DW_LNS_fixed_advance_pc = 9;
static constexpr uint8_t  DW_LNE_end_sequence = 1;
static constexpr uint8_t  DW_LNE_set_address  = 2;

static bool debug_sections_requested(const xbfd::debug_info& debug) {
    return !debug.files.empty()
        || !debug.functions.empty()
        || !debug.lines.empty()
        || !debug.symbols.empty();
}

static uint16_t dwarf_language(xbfd::debug_lang lang) {
    switch (lang) {
    case xbfd::debug_lang::c:
        return 0x0002; // DW_LANG_C
    case xbfd::debug_lang::assembly:
        return 0x8001; // vendor range, used as "assembly"
    default:
        return 0x0000;
    }
}

class dwarf2_section_builder {
public:
    explicit dwarf2_section_builder(const xbfd::object& obj)
        : obj_(obj) {}

    void append_to(xbfd::object& out) {
        if (!debug_sections_requested(obj_.debug))
            return;

        collect_file_state();

        auto abbrev = build_abbrev();
        auto line = build_line();
        auto info = build_info();

        add_section(out, ".debug_abbrev", std::move(abbrev));
        add_section(out, ".debug_line", std::move(line));
        add_section(out, ".debug_info", std::move(info));
    }

private:
    struct file_state {
        xbfd::debug_source_file file;
        std::vector<xbfd::debug_line> lines;
        std::vector<xbfd::debug_function> functions;
        uint32_t stmt_offset = 0;
        uint32_t info_offset = 0;
        uint32_t low_pc = 0;
        uint32_t high_pc = 0;
        bool has_range = false;
    };

    static void add_section(xbfd::object& out,
                            const std::string& name,
                            std::vector<uint8_t> data)
    {
        xbfd::section sec;
        sec.name = name;
        sec.flags = xbfd::section_flags::debugging;
        sec.size = data.size();
        sec.data = std::move(data);
        out.sections.push_back(std::move(sec));
    }

    static void push_u8(std::vector<uint8_t>& out, uint8_t value) {
        out.push_back(value);
    }

    static void push_u16(std::vector<uint8_t>& out, uint16_t value) {
        out.push_back(static_cast<uint8_t>(value & 0xFF));
        out.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    }

    static void push_u32(std::vector<uint8_t>& out, uint32_t value) {
        out.push_back(static_cast<uint8_t>(value & 0xFF));
        out.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
        out.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
        out.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
    }

    static void patch_u32(std::vector<uint8_t>& out,
                          std::size_t offset,
                          uint32_t value)
    {
        out[offset + 0] = static_cast<uint8_t>(value & 0xFF);
        out[offset + 1] = static_cast<uint8_t>((value >> 8) & 0xFF);
        out[offset + 2] = static_cast<uint8_t>((value >> 16) & 0xFF);
        out[offset + 3] = static_cast<uint8_t>((value >> 24) & 0xFF);
    }

    static void push_string(std::vector<uint8_t>& out, const std::string& text) {
        out.insert(out.end(), text.begin(), text.end());
        out.push_back(0);
    }

    static void push_uleb128(std::vector<uint8_t>& out, uint32_t value) {
        do {
            uint8_t byte = static_cast<uint8_t>(value & 0x7F);
            value >>= 7;
            if (value != 0)
                byte |= 0x80;
            out.push_back(byte);
        } while (value != 0);
    }

    static void push_sleb128(std::vector<uint8_t>& out, int32_t value) {
        bool more = true;
        while (more) {
            uint8_t byte = static_cast<uint8_t>(value & 0x7F);
            const bool sign = (byte & 0x40) != 0;
            value >>= 7;
            if ((value == 0 && !sign) || (value == -1 && sign)) {
                more = false;
            } else {
                byte |= 0x80;
            }
            out.push_back(byte);
        }
    }

    static std::string normalize_path(const std::string& path) {
        if (path.empty())
            return "unknown";
        return std::filesystem::path(path).lexically_normal().string();
    }

    static std::string dirname_or_empty(const std::string& path) {
        auto dir = std::filesystem::path(path).parent_path();
        if (dir.empty())
            return "";
        return dir.lexically_normal().string();
    }

    static std::string basename_or_self(const std::string& path) {
        auto p = std::filesystem::path(path);
        auto name = p.filename().string();
        return name.empty() ? p.string() : name;
    }

    bool is_external_function(const std::string& name) const {
        for (const auto& sym : obj_.symbols) {
            if (!sym.is_global())
                continue;
            if (sym.name == name)
                return true;
            if (!name.empty() && name[0] != '_' && sym.name == "_" + name)
                return true;
        }
        return false;
    }

    void update_range(file_state& state, uint32_t start, uint32_t end) {
        if (!state.has_range) {
            state.low_pc = start;
            state.high_pc = end;
            state.has_range = true;
            return;
        }
        state.low_pc = std::min(state.low_pc, start);
        state.high_pc = std::max(state.high_pc, end);
    }

    void collect_file_state() {
        for (const auto& file : obj_.debug.files) {
            file_state state;
            state.file = file;
            file_index_[file.id] = files_.size();
            files_.push_back(std::move(state));
        }

        for (const auto& line : obj_.debug.lines) {
            auto it = file_index_.find(line.file_id);
            if (it == file_index_.end())
                continue;
            auto& state = files_[it->second];
            state.lines.push_back(line);
            update_range(state, line.address, line.address + 1u);
        }

        for (const auto& fn : obj_.debug.functions) {
            auto it = file_index_.find(fn.file_id);
            if (it == file_index_.end())
                continue;
            auto& state = files_[it->second];
            state.functions.push_back(fn);
            update_range(state, fn.start, std::max<uint32_t>(fn.end, fn.start + 1u));
        }

        for (auto& state : files_) {
            std::sort(state.lines.begin(), state.lines.end(),
                      [](const auto& a, const auto& b) {
                          if (a.address != b.address)
                              return a.address < b.address;
                          return a.line < b.line;
                      });
            std::sort(state.functions.begin(), state.functions.end(),
                      [](const auto& a, const auto& b) {
                          if (a.start != b.start)
                              return a.start < b.start;
                          return a.name < b.name;
                      });
        }
    }

    std::vector<uint8_t> build_abbrev() const {
        std::vector<uint8_t> out;
        // abbrev 1: compile_unit with children
        push_u8(out, 1);
        push_u8(out, DW_TAG_compile_unit);
        push_u8(out, 1);
        push_u8(out, DW_AT_producer);  push_u8(out, DW_FORM_string);
        push_u8(out, DW_AT_name);      push_u8(out, DW_FORM_string);
        push_u8(out, DW_AT_stmt_list); push_u8(out, DW_FORM_data4);
        push_u8(out, DW_AT_low_pc);    push_u8(out, DW_FORM_addr);
        push_u8(out, DW_AT_high_pc);   push_u8(out, DW_FORM_addr);
        push_u8(out, DW_AT_language);  push_u8(out, DW_FORM_data2);
        push_u8(out, 0);               push_u8(out, 0);

        // abbrev 2: subprogram without children
        push_u8(out, 2);
        push_u8(out, DW_TAG_subprogram);
        push_u8(out, 0);
        push_u8(out, DW_AT_name);      push_u8(out, DW_FORM_string);
        push_u8(out, DW_AT_low_pc);    push_u8(out, DW_FORM_addr);
        push_u8(out, DW_AT_high_pc);   push_u8(out, DW_FORM_addr);
        push_u8(out, DW_AT_external);  push_u8(out, DW_FORM_flag);
        push_u8(out, DW_AT_calling_convention); push_u8(out, DW_FORM_data1);
        push_u8(out, 0);               push_u8(out, 0);

        push_u8(out, 0);
        return out;
    }

    std::vector<uint8_t> build_line() {
        std::vector<uint8_t> out;

        for (auto& state : files_) {
            state.stmt_offset = static_cast<uint32_t>(out.size());
            const auto unit_start = out.size();
            push_u32(out, 0); // unit_length patch later
            push_u16(out, 2); // DWARF v2
            const auto header_length_pos = out.size();
            push_u32(out, 0); // header_length patch later
            const auto header_start = out.size();

            push_u8(out, 1);  // minimum_instruction_length
            push_u8(out, 1);  // default_is_stmt
            push_u8(out, 0);  // line_base
            push_u8(out, 1);  // line_range
            push_u8(out, 10); // opcode_base
            const uint8_t std_lengths[9] = {0, 1, 1, 1, 1, 0, 0, 0, 1};
            out.insert(out.end(), std::begin(std_lengths), std::end(std_lengths));

            const auto full_path = normalize_path(state.file.path);
            const auto dir = dirname_or_empty(full_path);
            if (!dir.empty())
                push_string(out, dir);
            push_u8(out, 0); // end include dirs

            push_string(out, basename_or_self(full_path));
            push_uleb128(out, dir.empty() ? 0u : 1u);
            push_uleb128(out, 0);
            push_uleb128(out, 0);
            push_u8(out, 0); // end file table

            patch_u32(out, header_length_pos,
                      static_cast<uint32_t>(out.size() - header_start));

            uint32_t current_line = 1;
            for (const auto& row : state.lines) {
                push_u8(out, 0);
                push_uleb128(out, 5);
                push_u8(out, DW_LNE_set_address);
                push_u32(out, row.address);

                if (row.line != current_line) {
                    push_u8(out, DW_LNS_advance_line);
                    push_sleb128(out, static_cast<int32_t>(row.line)
                                       - static_cast<int32_t>(current_line));
                    current_line = row.line;
                }
                push_u8(out, DW_LNS_copy);
            }

            push_u8(out, 0);
            push_uleb128(out, 1);
            push_u8(out, DW_LNE_end_sequence);

            patch_u32(out, unit_start,
                      static_cast<uint32_t>(out.size() - unit_start - 4));
        }

        return out;
    }

    std::vector<uint8_t> build_info() {
        std::vector<uint8_t> out;

        for (auto& state : files_) {
            state.info_offset = static_cast<uint32_t>(out.size());
            const auto unit_start = out.size();
            push_u32(out, 0); // unit_length patch later
            push_u16(out, 2); // DWARF v2
            push_u32(out, 0); // abbrev offset
            push_u8(out, 4);  // address size

            push_u8(out, 1); // CU abbrev
            push_string(out, "xld GNU mode");
            push_string(out, normalize_path(state.file.path));
            push_u32(out, state.stmt_offset);
            push_u32(out, state.low_pc);
            push_u32(out, state.has_range ? state.high_pc : state.low_pc);
            push_u16(out, dwarf_language(state.file.language));

            for (const auto& fn : state.functions) {
                push_u8(out, 2); // subprogram abbrev
                push_string(out, fn.name);
                push_u32(out, fn.start);
                push_u32(out, std::max<uint32_t>(fn.end, fn.start + 1u));
                push_u8(out, is_external_function(fn.name) ? 1 : 0);
                push_u8(out, static_cast<uint8_t>(fn.convention));
            }

            push_u8(out, 0); // end children
            patch_u32(out, unit_start,
                      static_cast<uint32_t>(out.size() - unit_start - 4));
        }

        return out;
    }

    const xbfd::object& obj_;
    std::vector<file_state> files_;
    std::map<uint32_t, std::size_t> file_index_;
};

// -------------------------------------------------------------------------
// elf_builder — accumulates a byte buffer then writes it as ELF32
// -------------------------------------------------------------------------

class elf_builder {
public:
    void emit(const xbfd::object& obj, std::ostream& out) {
        xbfd::object materialized = obj;
        dwarf2_section_builder(materialized).append_to(materialized);

        buf_.reserve(4096);
        buf_.resize(52, 0); // ELF header placeholder

        build_string_tables(materialized);
        build_symbol_table(materialized);
        build_section_data(materialized);
        build_rel_sections(materialized);
        build_shstrtab();
        build_section_headers(materialized);
        patch_elf_header(materialized);

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
