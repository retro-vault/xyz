// ar_writer.cpp
//
// Writes a bfd::bfd archive to disk in one of two formats:
//
//   flavour::ar_text   — SDCC text-index: a newline-delimited list of
//                        relative member paths.  Members must be external
//                        files; in-memory data is not supported.
//
//   flavour::ar_binary — GNU ar format ("!<arch>\n").  Each member's
//                        raw data is written as a self-contained entry.
//                        Uses the GNU extended-name table (//) for
//                        member names longer than 15 characters.
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
//
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

#include <xbfd/bfd.hpp>
#include <xbfd/errors.hpp>

namespace bfd {

    namespace {

        static void emit_ar_header(std::ostream& out,
                                   const std::string& name,
                                   size_t size)
        {
            // ar member header is exactly 60 bytes.
            auto field = [&](const std::string& v, int w) {
                std::string padded = v.substr(0, static_cast<size_t>(w));
                while (static_cast<int>(padded.size()) < w) padded += ' ';
                out << padded;
            };
            field(name, 16);
            field("0", 12);          // mtime = 0
            field("0", 6);           // uid = 0
            field("0", 6);           // gid = 0
            field("644", 8);         // mode
            field(std::to_string(size), 10);
            out << "`\n";
        }

    } // anonymous namespace

    // -------------------------------------------------------------------------
    // emit_archive_text — SDCC text-index format
    // -------------------------------------------------------------------------

    void emit_archive_text(const bfd& arc, std::ostream& out)
    {
        out << "# xbfd text-index library\n";
        for (const auto& m : arc.members()) {
            // Use the path relative to the archive location when available.
            std::string entry = m.path.empty() ? m.name
                                               : m.path.filename().string();
            out << entry << "\n";
        }
    }

    // -------------------------------------------------------------------------
    // emit_archive_ar — GNU ar binary format
    // -------------------------------------------------------------------------

    void emit_archive_ar(const bfd& arc, std::ostream& out)
    {
        out << "!<arch>\n";

        // Build GNU extended name table for names > 15 chars.
        std::string name_table;
        std::vector<std::string> encoded_names;

        for (const auto& m : arc.members()) {
            if (m.name.size() <= 15) {
                encoded_names.push_back(m.name + "/");
            } else {
                size_t off = name_table.size();
                name_table += m.name + "/\n";
                encoded_names.push_back("/" + std::to_string(off));
            }
        }

        if (!name_table.empty()) {
            if (name_table.size() & 1) name_table += '\n';
            emit_ar_header(out, "//", name_table.size());
            out.write(name_table.data(),
                      static_cast<std::streamsize>(name_table.size()));
        }

        for (size_t i = 0; i < arc.members().size(); ++i) {
            const auto& m = arc.members()[i];

            std::string member_bytes;
            if (m.data.has_value()) {
                member_bytes = *m.data;
            } else if (!m.path.empty()) {
                // Load from external file.
                std::ifstream f(m.path, std::ios::binary);
                if (!f.is_open())
                    throw io_error("cannot open archive member: "
                                   + m.path.string());
                member_bytes.assign(
                    (std::istreambuf_iterator<char>(f)),
                    std::istreambuf_iterator<char>());
            }

            emit_ar_header(out, encoded_names[i], member_bytes.size());
            out.write(member_bytes.data(),
                      static_cast<std::streamsize>(member_bytes.size()));
            if (member_bytes.size() & 1) out.put('\n');
        }
    }

} // namespace bfd
