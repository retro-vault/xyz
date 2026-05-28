// ar_reader.cpp
//
// Reads archive files into a list of bfd::archive_member structs.
// Two formats are supported:
//
//   Text-index (.lib)  — SDCC convention: one relative .rel path per line,
//                        '#' comments allowed.  NOT an ar-magic file.
//
//   GNU ar / BSD ar    — starts with the "!<arch>\n" magic string.  Members
//                        may use either the GNU extended-name table (//) or
//                        the BSD "#1/" scheme for long names.
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

#include <xbfd/bfd.hpp>
#include <xbfd/errors.hpp>

namespace bfd {

    // -------------------------------------------------------------------------
    // Internal helpers
    // -------------------------------------------------------------------------

    namespace {

        static const std::string AR_MAGIC = "!<arch>\n";

        static std::string trim(const std::string& s) {
            auto a = s.find_first_not_of(" \t\r\n");
            if (a == std::string::npos) return {};
            auto b = s.find_last_not_of(" \t\r\n");
            return s.substr(a, b - a + 1);
        }

        static std::string trim_ar_name(std::string s) {
            s = trim(s);
            while (!s.empty() && s.back() == '/')
                s.pop_back();
            return s;
        }

        static std::string gnu_long_name(const std::string& table, size_t off) {
            if (off >= table.size())
                throw format_error("invalid GNU ar string table offset");
            size_t end = off;
            while (end < table.size()) {
                if (table[end] == '/' && end + 1 < table.size()
                    && table[end + 1] == '\n')
                    break;
                ++end;
            }
            return table.substr(off, end - off);
        }

        static bool is_ar(const std::string& data) {
            return data.size() >= 8 && data.compare(0, 8, AR_MAGIC) == 0;
        }

        // ---- Text-index reader ------------------------------------------------

        static std::vector<archive_member> read_text_index(
            const std::filesystem::path& lib_path,
            const std::string& data)
        {
            std::vector<archive_member> result;
            auto lib_dir = lib_path.parent_path();
            size_t start = 0;

            while (start <= data.size()) {
                auto end = data.find('\n', start);
                if (end == std::string::npos) end = data.size();

                auto line = trim(data.substr(start, end - start));
                if (!line.empty() && line[0] != '#') {
                    archive_member m;
                    m.name = line;
                    m.path = lib_dir / line;
                    result.push_back(std::move(m));
                }
                if (end == data.size()) break;
                start = end + 1;
            }
            return result;
        }

        // ---- GNU/BSD ar reader -----------------------------------------------

        static std::vector<archive_member> read_ar(
            const std::filesystem::path& path,
            const std::string& data)
        {
            std::vector<archive_member> result;
            std::string gnu_name_table;
            size_t pos = 8; // skip "!<arch>\n"

            while (pos + 60 <= data.size()) {
                std::string header = data.substr(pos, 60);
                pos += 60;

                if (header.substr(58, 2) != "`\n")
                    throw format_error("malformed ar member header in: "
                                       + path.string());

                std::string raw_name = trim(header.substr(0, 16));
                std::string size_str = trim(header.substr(48, 10));
                size_t member_size = 0;
                try {
                    member_size = static_cast<size_t>(
                        std::stoul(size_str, nullptr, 10));
                } catch (...) {
                    throw format_error("invalid ar member size in: "
                                       + path.string());
                }

                if (pos + member_size > data.size())
                    throw format_error("truncated ar member in: "
                                       + path.string());

                std::string member_data = data.substr(pos, member_size);
                pos += member_size;
                if ((member_size & 1U) && pos < data.size()) ++pos;

                // Symbol index tables and name tables: skip.
                if (raw_name == "/" || raw_name.empty()) continue;
                if (raw_name == "//") {
                    gnu_name_table = member_data;
                    continue;
                }

                // Decode member name.
                std::string member_name;
                if (raw_name.rfind("#1/", 0) == 0) {
                    // BSD long name
                    auto name_len = static_cast<size_t>(
                        std::stoul(raw_name.substr(3), nullptr, 10));
                    if (name_len > member_data.size())
                        throw format_error("invalid BSD ar name in: "
                                           + path.string());
                    member_name = member_data.substr(0, name_len);
                    member_data.erase(0, name_len);
                } else if (raw_name.size() > 1 && raw_name[0] == '/'
                           && std::isdigit(static_cast<unsigned char>(
                                raw_name[1]))) {
                    // GNU long name
                    auto offset = static_cast<size_t>(
                        std::stoul(raw_name.substr(1), nullptr, 10));
                    member_name = gnu_long_name(gnu_name_table, offset);
                } else {
                    member_name = trim_ar_name(raw_name);
                }

                // Only include .rel and .o members.
                std::string lname = member_name;
                std::transform(lname.begin(), lname.end(), lname.begin(),
                               ::tolower);
                bool keep = (lname.size() >= 4 &&
                             (lname.substr(lname.size() - 4) == ".rel" ||
                              lname.substr(lname.size() - 2) == ".o"));
                if (!keep) continue;

                archive_member m;
                m.name = member_name;
                m.path = std::filesystem::path(
                    path.string() + "[" + member_name + "]");
                m.data = member_data;
                result.push_back(std::move(m));
            }
            return result;
        }

    } // anonymous namespace

    // -------------------------------------------------------------------------
    // parse_archive — public entry point
    // -------------------------------------------------------------------------

    std::vector<archive_member> parse_archive(
        const std::filesystem::path& path,
        const std::string& data)
    {
        if (is_ar(data))
            return read_ar(path, data);
        return read_text_index(path, data);
    }

} // namespace bfd
