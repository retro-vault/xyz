//
// library readers
//
// MIT License (see: LICENSE)
// copyright (C) 2021 tomaz stih
//
#include <algorithm>
#include <cctype>

#include <xlink/library_reader.hpp>
#include <xlink/errors.hpp>

namespace xlink {

    static std::string trim(const std::string& s) {
        auto start = s.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) return "";
        auto end = s.find_last_not_of(" \t\r\n");
        return s.substr(start, end - start + 1);
    }

    static std::string trim_ar_name(const std::string& s) {
        std::string out = trim(s);
        while (!out.empty() && out.back() == '/')
            out.pop_back();
        return out;
    }

    static std::string parse_gnu_long_name(const std::string& table,
                                           std::size_t offset)
    {
        if (offset >= table.size())
            throw parse_error("invalid GNU ar string table offset");

        std::size_t end = offset;
        while (end < table.size()) {
            if (table[end] == '/' &&
                end + 1 < table.size() &&
                table[end + 1] == '\n') {
                break;
            }
            end++;
        }
        return table.substr(offset, end - offset);
    }

    bool text_index_library_reader::can_read(const std::filesystem::path&,
                                             const std::string& data) const
    {
        return !(data.size() >= 8 && data.compare(0, 8, "!<arch>\n") == 0);
    }

    std::vector<lib_member> text_index_library_reader::read(
        const std::filesystem::path& path,
        const std::string& data) const
    {
        auto lib_dir = path.parent_path();
        std::vector<lib_member> result;
        std::size_t start = 0;

        while (start <= data.size()) {
            auto end = data.find('\n', start);
            if (end == std::string::npos)
                end = data.size();

            auto line = trim(data.substr(start, end - start));
            if (!line.empty() && line[0] != '#') {
                lib_member member;
                member.path = lib_dir / line;
                result.push_back(std::move(member));
            }

            if (end == data.size())
                break;
            start = end + 1;
        }

        return result;
    }

    bool ar_library_reader::can_read(const std::filesystem::path&,
                                     const std::string& data) const
    {
        return data.size() >= 8 && data.compare(0, 8, "!<arch>\n") == 0;
    }

    std::vector<lib_member> ar_library_reader::read(
        const std::filesystem::path& path,
        const std::string& data) const
    {
        std::vector<lib_member> result;
        std::string gnu_name_table;
        std::size_t pos = 8;

        while (pos + 60 <= data.size()) {
            std::string header = data.substr(pos, 60);
            pos += 60;

            if (header.substr(58, 2) != "`\n") {
                throw parse_error("malformed ar member header in: "
                                  + path.string());
            }

            std::string raw_name = trim(header.substr(0, 16));
            std::string size_str = trim(header.substr(48, 10));
            std::size_t member_size = 0;
            try {
                member_size = static_cast<std::size_t>(
                    std::stoul(size_str, nullptr, 10));
            } catch (const std::exception&) {
                throw parse_error("invalid ar member size in: "
                                  + path.string());
            }

            if (pos + member_size > data.size()) {
                throw parse_error("truncated ar member in: " + path.string());
            }

            std::string member_data = data.substr(pos, member_size);
            pos += member_size;
            if ((member_size & 1U) != 0 && pos < data.size())
                pos++;

            if (raw_name == "/" || raw_name.empty())
                continue;

            if (raw_name == "//") {
                gnu_name_table = member_data;
                continue;
            }

            std::string member_name;
            if (raw_name.rfind("#1/", 0) == 0) {
                auto name_len = static_cast<std::size_t>(
                    std::stoul(raw_name.substr(3), nullptr, 10));
                if (name_len > member_data.size()) {
                    throw parse_error("invalid BSD ar member name in: "
                                      + path.string());
                }
                member_name = member_data.substr(0, name_len);
                member_data.erase(0, name_len);
            } else if (raw_name[0] == '/' && raw_name.size() > 1
                       && std::isdigit(static_cast<unsigned char>(raw_name[1]))) {
                auto offset = static_cast<std::size_t>(
                    std::stoul(raw_name.substr(1), nullptr, 10));
                member_name = parse_gnu_long_name(gnu_name_table, offset);
            } else {
                member_name = trim_ar_name(raw_name);
            }

            std::string lowered = member_name;
            std::transform(lowered.begin(), lowered.end(), lowered.begin(),
                           ::tolower);
            if (lowered.size() < 4 ||
                lowered.substr(lowered.size() - 4) != ".rel") {
                continue;
            }

            lib_member member;
            member.path = std::filesystem::path(
                path.string() + "[" + member_name + "]");
            member.contents = member_data;
            result.push_back(std::move(member));
        }

        return result;
    }

} // namespace xlink
