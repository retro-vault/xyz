// bfd.cpp
#include <sstream>
//
// bfd::bfd — central dispatch: open_r probes the file and calls the
// appropriate reader; close() dispatches to the right writer.
//
// Format detection order for open_r():
//   1. Starts with "!<arch>\n"         → ar_binary archive
//   2. Starts with ELF magic (7F 45 4C 46) → elf object
//   3. Starts with "XL" or "XH" or "M " or "H " → rel object
//   4. Otherwise try as text-index archive (list of paths)
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
//
#include <algorithm>
#include <fstream>
#include <iterator>

#include <xbfd/bfd.hpp>
#include <xbfd/errors.hpp>

namespace bfd {

    // -------------------------------------------------------------------------
    // Forward declarations of reader/writer functions (defined in other TUs).
    // -------------------------------------------------------------------------

    // rel_reader.cpp
    void parse_rel(bfd& obj, const std::string& src, std::istream& in);

    // elf_reader.cpp
    void parse_elf(bfd& obj, const std::string& src,
                   const std::vector<uint8_t>& raw);

    // ar_reader.cpp
    std::vector<archive_member> parse_archive(
        const std::filesystem::path& path, const std::string& data);

    // rel_writer.cpp
    void emit_rel(const bfd& obj, std::ostream& out);

    // elf_writer.cpp
    void emit_elf(const bfd& obj, std::ostream& out);

    // ar_writer.cpp
    void emit_archive_text(const bfd& arc, std::ostream& out);
    void emit_archive_ar(const bfd& arc, std::ostream& out);

    // -------------------------------------------------------------------------
    // bfd member implementations
    // -------------------------------------------------------------------------

    std::unique_ptr<bfd> bfd::open_r(const std::filesystem::path& path)
    {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open())
            throw io_error("cannot open: " + path.string());

        // Read full content.
        std::string raw_str((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());

        auto obj = std::unique_ptr<bfd>(new bfd());
        obj->path_     = path;
        obj->writable_ = false;

        const bool is_ar = raw_str.size() >= 8
            && raw_str.compare(0, 8, "!<arch>\n") == 0;

        const bool is_elf = raw_str.size() >= 4
            && static_cast<uint8_t>(raw_str[0]) == 0x7F
            && raw_str[1] == 'E' && raw_str[2] == 'L' && raw_str[3] == 'F';

        // A .rel file starts with XL, XL2, XL4, XH, H, M, or a ; comment.
        // SDCC-compiled .rel files often begin with ";!FILE filename.asm".
        auto starts_rel = [&]() {
            if (raw_str.size() < 2) return false;
            if (raw_str[0] == 'X' && (raw_str[1] == 'L' || raw_str[1] == 'H'))
                return true;
            if (raw_str[0] == 'H' || raw_str[0] == 'M') return true;
            // Skip leading comment lines and check for XL/H/M marker.
            if (raw_str[0] == ';') {
                size_t nl = raw_str.find('\n');
                while (nl != std::string::npos && nl + 1 < raw_str.size()) {
                    char c = raw_str[nl + 1];
                    if (c == ';') { nl = raw_str.find('\n', nl + 1); continue; }
                    if (c == 'X' && nl + 2 < raw_str.size()
                        && (raw_str[nl + 2] == 'L' || raw_str[nl + 2] == 'H'))
                        return true;
                    if (c == 'H' || c == 'M') return true;
                    break;
                }
            }
            return false;
        };

        if (is_ar) {
            obj->format_  = format::archive;
            obj->flavour_ = flavour::ar_binary;
            obj->members_ = parse_archive(path, raw_str);
        } else if (is_elf) {
            obj->format_  = format::object;
            obj->flavour_ = flavour::elf;
            std::vector<uint8_t> raw_bytes(raw_str.begin(), raw_str.end());
            parse_elf(*obj, path.string(), raw_bytes);
        } else if (starts_rel()) {
            obj->format_  = format::object;
            obj->flavour_ = flavour::rel;
            std::istringstream iss(raw_str);
            parse_rel(*obj, path.string(), iss);
        } else {
            // Last resort: text-index archive.
            obj->format_  = format::archive;
            obj->flavour_ = flavour::ar_text;
            obj->members_ = parse_archive(path, raw_str);
        }

        return obj;
    }

    std::unique_ptr<bfd> bfd::open_r_stream(const std::string& name,
                                             std::istream& input)
    {
        auto obj = std::unique_ptr<bfd>(new bfd());
        obj->path_     = name;
        obj->writable_ = false;
        obj->format_   = format::object;
        obj->flavour_  = flavour::rel;
        parse_rel(*obj, name, input);
        return obj;
    }

    std::unique_ptr<bfd> bfd::open_w(const std::filesystem::path& path,
                                      flavour fmt)
    {
        auto obj = std::unique_ptr<bfd>(new bfd());
        obj->path_     = path;
        obj->format_   = format::object;
        obj->flavour_  = fmt;
        obj->writable_ = true;
        return obj;
    }

    std::unique_ptr<bfd> bfd::create_archive(const std::filesystem::path& path,
                                              flavour fmt)
    {
        auto obj = std::unique_ptr<bfd>(new bfd());
        obj->path_     = path;
        obj->format_   = format::archive;
        obj->flavour_  = fmt;
        obj->writable_ = true;
        return obj;
    }

    bool bfd::check_format(format fmt) const
    {
        return format_ == fmt;
    }

    section* bfd::find_section(const std::string& name)
    {
        for (auto& s : sections_)
            if (s.name() == name) return &s;
        return nullptr;
    }

    section& bfd::add_section(const std::string& name, section_flags flags,
                               uint64_t vma)
    {
        sections_.emplace_back(name, flags, vma);
        return sections_.back();
    }

    symbol& bfd::add_symbol(const std::string& name, symbol_flags flags,
                             uint64_t value, const std::string& section_name)
    {
        symbols_.emplace_back(name, flags, value, section_name);
        return symbols_.back();
    }

    void bfd::close()
    {
        if (!writable_) return;

        if (format_ == format::archive) {
            std::ofstream f(path_,
                            std::ios::binary | std::ios::trunc);
            if (!f.is_open())
                throw io_error("cannot write: " + path_.string());
            if (flavour_ == flavour::ar_text)
                emit_archive_text(*this, f);
            else
                emit_archive_ar(*this, f);
        } else {
            std::ofstream f(path_,
                            (flavour_ == flavour::elf
                                ? std::ios::binary | std::ios::trunc
                                : std::ios::trunc));
            if (!f.is_open())
                throw io_error("cannot write: " + path_.string());
            if (flavour_ == flavour::elf)
                emit_elf(*this, f);
            else
                emit_rel(*this, f);
        }
        writable_ = false;
    }

} // namespace bfd
