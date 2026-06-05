// object/ar_reader.cpp — archive reader (text-index and GNU ar formats).
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
//
#include <algorithm>
#include <cctype>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include <xbfd/xbfd.h>

namespace bfd {
namespace {

class ar_reader {
public:
    std::vector<xbfd::archive_member> parse(const std::filesystem::path& path,
                                             const std::string& data) {
        if (is_ar_magic(data)) return read_ar(path, data);
        return read_text_index(path, data);
    }

private:
    static std::vector<xbfd::archive_member> read_text_index(
            const std::filesystem::path& lib_path, const std::string& data) {
        std::vector<xbfd::archive_member> result;
        const auto lib_dir = lib_path.parent_path();
        size_t start = 0;
        while (start <= data.size()) {
            const auto end  = next_line_end(data, start);
            const auto line = trim(data.substr(start, end - start));
            if (!line.empty() && line[0] != '#') {
                xbfd::archive_member m;
                m.name = line; m.path = (lib_dir / line).string();
                result.push_back(std::move(m));
            }
            if (end == data.size()) break;
            start = end + 1;
        }
        return result;
    }

    static std::vector<xbfd::archive_member> read_ar(
            const std::filesystem::path& path, const std::string& data) {
        std::vector<xbfd::archive_member> result;
        std::string gnu_name_table;
        size_t pos = 8;
        while (pos + 60 <= data.size()) {
            const std::string header = data.substr(pos, 60);
            pos += 60;
            if (header.substr(58, 2) != "`\n")
                throw xbfd::format_error("malformed ar member header in: " + path.string());
            const std::string raw_name = trim(header.substr(0, 16));
            const std::string size_str = trim(header.substr(48, 10));
            const size_t      msize    = parse_size(size_str, path);
            if (pos + msize > data.size())
                throw xbfd::format_error("truncated ar member in: " + path.string());
            std::string mdata = data.substr(pos, msize);
            pos += msize;
            if ((msize & 1U) && pos < data.size()) ++pos;
            if (raw_name == "/" || raw_name.empty()) continue;
            if (raw_name == "//") { gnu_name_table = mdata; continue; }
            const auto mname = decode_name(raw_name, mdata, gnu_name_table, path);
            if (!is_object_file(mname)) continue;
            xbfd::archive_member m;
            m.name = mname; m.path = path.string() + "[" + mname + "]"; m.data = mdata;
            result.push_back(std::move(m));
        }
        return result;
    }

    static bool   is_ar_magic(const std::string& data) {
        return data.size() >= 8 && data.compare(0, 8, "!<arch>\n") == 0;
    }
    static size_t next_line_end(const std::string& data, size_t start) {
        const auto end = data.find('\n', start);
        return end == std::string::npos ? data.size() : end;
    }
    static size_t parse_size(const std::string& s, const std::filesystem::path& path) {
        try { return static_cast<size_t>(std::stoul(s, nullptr, 10)); }
        catch (...) { throw xbfd::format_error("invalid ar member size in: " + path.string()); }
    }
    static std::string decode_name(const std::string& raw, std::string& mdata,
                                    const std::string& gnu_table,
                                    const std::filesystem::path& path) {
        if (raw.rfind("#1/", 0) == 0) {
            const auto len = static_cast<size_t>(std::stoul(raw.substr(3), nullptr, 10));
            if (len > mdata.size()) throw xbfd::format_error("invalid BSD ar name in: " + path.string());
            const auto name = mdata.substr(0, len); mdata.erase(0, len); return name;
        }
        if (raw.size() > 1 && raw[0] == '/' && std::isdigit(static_cast<unsigned char>(raw[1]))) {
            const auto off = static_cast<size_t>(std::stoul(raw.substr(1), nullptr, 10));
            return gnu_long_name(gnu_table, off, path);
        }
        return ar_trim(raw);
    }
    static std::string gnu_long_name(const std::string& table, size_t off,
                                      const std::filesystem::path& path) {
        if (off >= table.size()) throw xbfd::format_error("invalid GNU ar string table offset");
        size_t end = off;
        while (end < table.size()) {
            if (table[end] == '/' && end + 1 < table.size() && table[end+1] == '\n') break;
            ++end;
        }
        return table.substr(off, end - off);
    }
    static bool is_object_file(const std::string& name) {
        std::string low = name;
        std::transform(low.begin(), low.end(), low.begin(), ::tolower);
        return (low.size() >= 4 && low.substr(low.size()-4) == ".rel")
            || (low.size() >= 2 && low.substr(low.size()-2) == ".o");
    }
    static std::string trim(const std::string& s) {
        const auto a = s.find_first_not_of(" \t\r\n");
        if (a == std::string::npos) return {};
        return s.substr(a, s.find_last_not_of(" \t\r\n") - a + 1);
    }
    static std::string ar_trim(std::string s) {
        s = trim(s);
        while (!s.empty() && s.back() == '/') s.pop_back();
        return s;
    }
};

} // namespace

std::vector<xbfd::archive_member> parse_archive(const std::filesystem::path& path,
                                                  const std::string& data) {
    return ar_reader{}.parse(path, data);
}

} // namespace bfd

// -------------------------------------------------------------------------
// xbfd::ar_reader
// -------------------------------------------------------------------------

namespace xbfd {

std::optional<object> ar_reader::read(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return std::nullopt;
    std::string data((std::istreambuf_iterator<char>(file)), {});
    object obj;
    obj.format  = obj_format::archive;
    obj.members = bfd::parse_archive(path, data);
    // Distinguish binary ar from text-index by magic.
    obj.flavour = (data.size() >= 8 && data.compare(0, 8, "!<arch>\n") == 0)
                  ? obj_flavour::ar_binary : obj_flavour::ar_text;
    return obj;
}

} // namespace xbfd
