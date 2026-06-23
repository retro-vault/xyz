// object/bfd.cpp — bfd::bfd dispatcher implementation.
//
// bfd::bfd is the legacy wrapper that dispatches to format-specific
// readers/writers via the internal bfd:: free-function API.
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
//
#include <algorithm>
#include <fstream>
#include <iterator>
#include <sstream>

#include <xbfd/xbfd.h>

// -------------------------------------------------------------------------
// Forward declarations — implementations live in other translation units.
// -------------------------------------------------------------------------

namespace bfd {
    void parse_rel(xbfd::object& obj, const std::string& src, std::istream& in);
    void parse_elf(xbfd::object& obj, const std::string& src, const std::vector<uint8_t>& raw);
    std::vector<xbfd::archive_member> parse_archive(const std::filesystem::path& path,
                                                     const std::string& data);
    void emit_rel          (const xbfd::object& obj, std::ostream& out);
    void emit_elf          (const xbfd::object& obj, std::ostream& out);
    void emit_archive_text (const xbfd::object& arc, std::ostream& out);
    void emit_archive_ar   (const xbfd::object& arc, std::ostream& out);
} // namespace bfd

// -------------------------------------------------------------------------
// bfd::bfd member implementations
// -------------------------------------------------------------------------

namespace bfd {

std::unique_ptr<bfd> bfd::open_r(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
        throw xbfd::io_error("cannot open: " + path.string());

    std::string raw_str((std::istreambuf_iterator<char>(file)), {});

    auto obj       = std::unique_ptr<bfd>(new bfd());
    obj->path_     = path;
    obj->writable_ = false;

    if (is_ar_magic(raw_str)) {
        obj->obj_.format  = xbfd::obj_format::archive;
        obj->obj_.flavour = xbfd::obj_flavour::ar_binary;
        obj->obj_.members = parse_archive(path, raw_str);
    } else if (is_elf_magic(raw_str)) {
        parse_elf(obj->obj_, path.string(), {raw_str.begin(), raw_str.end()});
        obj->obj_.format  = xbfd::obj_format::object;
        obj->obj_.flavour = xbfd::obj_flavour::elf;
    } else if (is_rel_magic(raw_str)) {
        std::istringstream iss(raw_str);
        parse_rel(obj->obj_, path.string(), iss);
        obj->obj_.format  = xbfd::obj_format::object;
        obj->obj_.flavour = xbfd::obj_flavour::rel;
    } else {
        obj->obj_.format  = xbfd::obj_format::archive;
        obj->obj_.flavour = xbfd::obj_flavour::ar_text;
        obj->obj_.members = parse_archive(path, raw_str);
    }
    return obj;
}

std::unique_ptr<bfd> bfd::open_r_stream(const std::string& name, std::istream& input)
{
    auto obj       = std::unique_ptr<bfd>(new bfd());
    obj->path_     = name;
    obj->writable_ = false;

    std::string raw_str((std::istreambuf_iterator<char>(input)), {});

    if (is_ar_magic(raw_str)) {
        obj->obj_.format  = xbfd::obj_format::archive;
        obj->obj_.flavour = xbfd::obj_flavour::ar_binary;
        obj->obj_.members = parse_archive(obj->path_, raw_str);
    } else if (is_elf_magic(raw_str)) {
        parse_elf(obj->obj_, name, {raw_str.begin(), raw_str.end()});
        obj->obj_.format  = xbfd::obj_format::object;
        obj->obj_.flavour = xbfd::obj_flavour::elf;
    } else if (is_rel_magic(raw_str)) {
        std::istringstream iss(raw_str);
        parse_rel(obj->obj_, name, iss);
        obj->obj_.format  = xbfd::obj_format::object;
        obj->obj_.flavour = xbfd::obj_flavour::rel;
    } else {
        obj->obj_.format  = xbfd::obj_format::archive;
        obj->obj_.flavour = xbfd::obj_flavour::ar_text;
        obj->obj_.members = parse_archive(obj->path_, raw_str);
    }
    return obj;
}

std::unique_ptr<bfd> bfd::open_w(const std::filesystem::path& path, xbfd::obj_flavour fmt)
{
    auto obj       = std::unique_ptr<bfd>(new bfd());
    obj->path_     = path;
    obj->writable_ = true;
    obj->obj_.format  = xbfd::obj_format::object;
    obj->obj_.flavour = fmt;
    return obj;
}

std::unique_ptr<bfd> bfd::create_archive(const std::filesystem::path& path,
                                           xbfd::obj_flavour fmt)
{
    auto obj       = std::unique_ptr<bfd>(new bfd());
    obj->path_     = path;
    obj->writable_ = true;
    obj->obj_.format  = xbfd::obj_format::archive;
    obj->obj_.flavour = fmt;
    return obj;
}

bool bfd::check_format(xbfd::obj_format fmt) const { return obj_.format == fmt; }

xbfd::section* bfd::find_section(const std::string& name) {
    for (auto& s : obj_.sections) if (s.name == name) return &s;
    return nullptr;
}

xbfd::section& bfd::add_section(const std::string& name,
                                  xbfd::section_flags flags, uint64_t vma) {
    xbfd::section s; s.name = name; s.flags = flags; s.vma = vma;
    obj_.sections.push_back(std::move(s));
    return obj_.sections.back();
}

xbfd::symbol& bfd::add_symbol(const std::string& name, xbfd::symbol_flags flags,
                                uint64_t value, const std::string& sec_name) {
    obj_.symbols.push_back({name, flags, value, sec_name});
    return obj_.symbols.back();
}

void bfd::close()
{
    if (!writable_) return;

    if (obj_.format == xbfd::obj_format::archive) {
        std::ofstream f(path_, std::ios::binary | std::ios::trunc);
        if (!f.is_open()) throw xbfd::io_error("cannot write: " + path_.string());
        if (obj_.flavour == xbfd::obj_flavour::ar_text) emit_archive_text(obj_, f);
        else                                             emit_archive_ar  (obj_, f);
    } else {
        const bool binary = (obj_.flavour == xbfd::obj_flavour::elf);
        std::ofstream f(path_, binary ? std::ios::binary | std::ios::trunc : std::ios::trunc);
        if (!f.is_open()) throw xbfd::io_error("cannot write: " + path_.string());
        if (binary) emit_elf(obj_, f);
        else        emit_rel(obj_, f);
    }
    writable_ = false;
}

// -------------------------------------------------------------------------
// Format detection helpers
// -------------------------------------------------------------------------

bool bfd::is_ar_magic(const std::string& s) {
    return s.size() >= 8 && s.compare(0, 8, "!<arch>\n") == 0;
}
bool bfd::is_elf_magic(const std::string& s) {
    return s.size() >= 4
        && static_cast<uint8_t>(s[0]) == 0x7F
        && s[1] == 'E' && s[2] == 'L' && s[3] == 'F';
}
bool bfd::is_rel_magic(const std::string& s) {
    if (s.size() < 2) return false;
    if (s[0] == 'X' && (s[1] == 'L' || s[1] == 'H')) return true;
    if (s[0] == 'H' || s[0] == 'M') return true;
    if (s[0] == ';') {
        auto nl = s.find('\n');
        while (nl != std::string::npos && nl + 1 < s.size()) {
            const char c = s[nl + 1];
            if (c == ';') { nl = s.find('\n', nl + 1); continue; }
            if (c == 'X' && nl + 2 < s.size() && (s[nl+2]=='L'||s[nl+2]=='H')) return true;
            if (c == 'H' || c == 'M') return true;
            break;
        }
    }
    return false;
}

} // namespace bfd
