// object/ar_writer.cpp — archive writer (text-index and GNU ar binary formats).
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
//
#include <fstream>
#include <ostream>
#include <string>
#include <vector>

#include <xbfd/xbfd.h>

namespace bfd {
namespace {

class ar_writer {
public:
    void emit_text(const xbfd::object& arc, std::ostream& out) {
        out << "# xbfd text-index library\n";
        for (const auto& m : arc.members) {
            const auto entry = !m.name.empty()
                ? m.name
                : std::filesystem::path(m.path).filename().generic_string();
            out << entry << "\n";
        }
    }

    void emit_ar(const xbfd::object& arc, std::ostream& out) {
        out << "!<arch>\n";
        const auto [names, encoded] = build_name_table(arc);
        write_name_table(names, out);
        write_members(arc, encoded, out);
    }

private:
    std::pair<std::string, std::vector<std::string>>
    build_name_table(const xbfd::object& arc) {
        std::string gnu_table;
        std::vector<std::string> encoded;
        for (const auto& m : arc.members) {
            if (m.name.size() <= 15) encoded.push_back(m.name + "/");
            else { encoded.push_back("/" + std::to_string(gnu_table.size())); gnu_table += m.name + "/\n"; }
        }
        return {gnu_table, encoded};
    }
    void write_name_table(const std::string& table, std::ostream& out) {
        if (table.empty()) return;
        std::string padded = table;
        if (padded.size() & 1) padded += '\n';
        write_ar_header("//", padded.size(), out);
        out.write(padded.data(), static_cast<std::streamsize>(padded.size()));
    }
    void write_members(const xbfd::object& arc, const std::vector<std::string>& encoded,
                        std::ostream& out) {
        for (size_t i = 0; i < arc.members.size(); ++i) {
            const auto  data = load_member(arc.members[i]);
            write_ar_header(encoded[i], data.size(), out);
            out.write(data.data(), static_cast<std::streamsize>(data.size()));
            if (data.size() & 1) out.put('\n');
        }
    }
    static std::string load_member(const xbfd::archive_member& m) {
        if (m.data.has_value()) return *m.data;
        if (m.path.empty()) return {};
        std::ifstream f(m.path, std::ios::binary);
        if (!f.is_open()) throw xbfd::io_error("cannot open archive member: " + m.path);
        return {std::istreambuf_iterator<char>(f), {}};
    }
    static void write_ar_header(const std::string& name, size_t size, std::ostream& out) {
        auto field = [&](const std::string& v, int w) {
            const auto s = v.substr(0, static_cast<size_t>(w));
            out << s;
            for (int i = static_cast<int>(s.size()); i < w; ++i) out << ' ';
        };
        field(name,                   16);
        field("0",                    12);
        field("0",                     6);
        field("0",                     6);
        field("644",                   8);
        field(std::to_string(size),   10);
        out << "`\n";
    }
};

} // namespace

void emit_archive_text(const xbfd::object& arc, std::ostream& out) { ar_writer{}.emit_text(arc, out); }
void emit_archive_ar  (const xbfd::object& arc, std::ostream& out) { ar_writer{}.emit_ar  (arc, out); }

} // namespace bfd

// -------------------------------------------------------------------------
// xbfd::ar_writer
// -------------------------------------------------------------------------

namespace xbfd {

void ar_writer::write(const std::string& path, const object& obj) {
    const bool binary = (obj.flavour == obj_flavour::ar_binary);
    std::ofstream f(path, binary ? std::ios::binary | std::ios::trunc : std::ios::trunc);
    if (!f.is_open()) throw io_error("cannot write: " + path);
    if (binary) bfd::emit_archive_ar(obj, f);
    else        bfd::emit_archive_text(obj, f);
}

} // namespace xbfd
