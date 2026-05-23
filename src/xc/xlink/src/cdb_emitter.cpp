//
// SDCC .cdb emitter
//
// MIT License (see: LICENSE)
// copyright (C) 2021 tomaz stih
//
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <optional>
#include <regex>
#include <sstream>
#include <set>
#include <string>
#include <vector>

#include <xlink/cdb_emitter.hpp>
#include <xlink/debug_info.hpp>
#include <xlink/errors.hpp>
#include <xlink/lst_parser.hpp>

namespace xlink {

    namespace {

        struct compiler_record {
            char type = '\0';
            std::string line;
            std::string raw_key;
            char address_space = 'Z';
            bool on_stack = false;
        };

        static std::optional<std::filesystem::path> sidecar_path(
            const module& mod, const char* extension)
        {
            auto text = mod.path().string();
            if (text.find('[') != std::string::npos
                && text.find(']') != std::string::npos) {
                return std::nullopt;
            }

            auto path = mod.path();
            if (path.extension() != ".rel")
                return std::nullopt;

            path.replace_extension(extension);
            if (!std::filesystem::exists(path))
                return std::nullopt;
            return path;
        }

        static std::string trim(const std::string& s) {
            auto start = s.find_first_not_of(" \t\r\n");
            if (start == std::string::npos) return "";
            auto end = s.find_last_not_of(" \t\r\n");
            return s.substr(start, end - start + 1);
        }

        static std::string hex16(uint32_t value) {
            std::ostringstream out;
            out << std::uppercase << std::hex << (value & 0xffffu);
            return out.str();
        }

        static std::optional<compiler_record> parse_cdb_record_line(
            const std::string& line)
        {
            if (line.size() < 2 || line[1] != ':')
                return std::nullopt;

            compiler_record record;
            record.type = line[0];
            record.line = line;
            if (record.type == 'M' || record.type == 'T')
                return record;
            if (record.type != 'F' && record.type != 'S')
                return std::nullopt;

            auto open = line.find('(', 2);
            if (open == std::string::npos)
                return std::nullopt;
            record.raw_key = line.substr(2, open - 2);

            auto close = line.find("),", open);
            if (close == std::string::npos)
                return record;

            auto tail = line.substr(close + 2);
            std::vector<std::string> fields;
            std::stringstream ss(tail);
            std::string field;
            while (std::getline(ss, field, ','))
                fields.push_back(trim(field));
            if (!fields.empty() && !fields[0].empty())
                record.address_space = fields[0][0];
            if (fields.size() > 1)
                record.on_stack = fields[1] == "1";
            return record;
        }

        static std::optional<std::string> normalize_adb_scope_key(
            const std::string& payload)
        {
            auto parts = std::vector<std::string>();
            std::stringstream ss(payload);
            std::string part;
            while (std::getline(ss, part, '$'))
                parts.push_back(part);

            if (parts.size() < 4)
                return std::nullopt;

            const std::string scope_part = parts[0];
            const std::string& name = parts[1];
            const std::string& level_block = parts[2];
            auto underscore = level_block.find('_');
            if (underscore == std::string::npos)
                return std::nullopt;
            const std::string level = level_block.substr(0, underscore);
            const std::string block = level_block.substr(underscore + 1);

            if (scope_part == "G")
                return "G$" + name + "$" + level + "$" + block;

            if (!scope_part.empty() && scope_part[0] == 'F')
                return scope_part + "$" + name + "$" + level + "$" + block;

            if (!scope_part.empty() && scope_part[0] == 'L') {
                auto owner = scope_part.substr(1);
                auto dot = owner.rfind('.');
                if (dot != std::string::npos)
                    owner = owner.substr(dot + 1);
                return "L" + owner + "$" + name + "$" + level + "$" + block;
            }

            return std::nullopt;
        }

        static std::optional<compiler_record> normalize_adb_record_line(
            const std::string& line)
        {
            if (line.size() < 2 || line[1] != ':')
                return std::nullopt;

            if (line[0] == 'M') {
                compiler_record record;
                record.type = 'M';
                record.line = line;
                return record;
            }

            if (line[0] != 'F' && line[0] != 'S')
                return std::nullopt;

            auto open = line.find('(', 2);
            if (open == std::string::npos)
                return std::nullopt;
            auto normalized_key = normalize_adb_scope_key(
                line.substr(2, open - 2));
            if (!normalized_key.has_value())
                return std::nullopt;

            std::string normalized = line.substr(0, 2)
                + normalized_key.value() + line.substr(open);
            return parse_cdb_record_line(normalized);
        }

        static std::vector<compiler_record> read_compiler_records(
            const module& mod)
        {
            std::vector<compiler_record> records;

            if (auto cdb = sidecar_path(mod, ".cdb"); cdb.has_value()) {
                std::ifstream in(cdb.value());
                if (!in.is_open())
                    throw parse_error("cannot open cdb file: " + cdb->string());

                std::string line;
                while (std::getline(in, line)) {
                    auto record = parse_cdb_record_line(trim(line));
                    if (record.has_value()
                        && (record->type == 'M' || record->type == 'F'
                            || record->type == 'S' || record->type == 'T')) {
                        records.push_back(record.value());
                    }
                }
                return records;
            }

            if (auto adb = sidecar_path(mod, ".adb"); adb.has_value()) {
                std::ifstream in(adb.value());
                if (!in.is_open())
                    throw parse_error("cannot open adb file: " + adb->string());

                std::string line;
                while (std::getline(in, line)) {
                    auto record = normalize_adb_record_line(trim(line));
                    if (record.has_value()
                        && (record->type == 'M' || record->type == 'F'
                            || record->type == 'S')) {
                        records.push_back(record.value());
                    }
                }
                return records;
            }

            return records;
        }

        static std::optional<uint32_t> module_symbol_address(
            const module& mod, const std::string& name)
        {
            for (const auto& sym : mod.symbols()) {
                if (sym.is_def() && sym.name() == name)
                    return debug_info_builder::symbol_absolute_addr(&mod, sym);
            }
            return std::nullopt;
        }

        static std::vector<lst_line_entry> read_lst_lines(const module& mod) {
            if (auto lst = sidecar_path(mod, ".lst"); lst.has_value())
                return lst_parser::parse(lst.value());
            return {};
        }

        static std::optional<std::tuple<std::string, uint32_t, uint32_t, uint32_t>>
        parse_c_line_symbol(const std::string& name)
        {
            static const std::regex re(
                R"(^C\$(.+)\$([0-9]+)\$([0-9]+)_([0-9]+)\$([0-9]+)$)");
            std::smatch match;
            if (!std::regex_match(name, match, re))
                return std::nullopt;

            return std::make_tuple(
                match[1].str(),
                static_cast<uint32_t>(std::stoul(match[2].str(), nullptr, 10)),
                static_cast<uint32_t>(std::stoul(match[3].str(), nullptr, 10)),
                static_cast<uint32_t>(std::stoul(match[4].str(), nullptr, 10)));
        }

        static bool emit_record_address(std::ostream& out,
                                        const module& mod,
                                        const compiler_record& record)
        {
            auto address = module_symbol_address(mod, record.raw_key);
            if (!address.has_value())
                return false;

            out << "L:" << record.raw_key << ":" << hex16(address.value()) << "\n";
            return true;
        }

    } // namespace

    void cdb_emitter::emit(const std::filesystem::path& path,
                           const std::filesystem::path& image_path,
                           const link_context& ctx) const
    {
        (void)image_path;

        std::ofstream out(path);
        if (!out.is_open())
            throw xlink::xlink_error("cannot open CDB output file: "
                                     + path.string());

        for (const auto& mod_ptr : ctx.modules) {
            const auto& mod = *mod_ptr;
            auto records = read_compiler_records(mod);
            auto lst_lines = read_lst_lines(mod);
            if (records.empty() && !lst_lines.empty()) {
                out << "M:" << mod.name() << "\n";
            }

            for (const auto& record : records)
                out << record.line << "\n";

            std::set<std::string> emitted_start_records;
            std::set<std::string> emitted_end_records;
            for (const auto& record : records) {
                if (record.type == 'S') {
                    if (record.address_space == 'R' || record.on_stack)
                        continue;
                    if (emitted_start_records.insert(record.raw_key).second)
                        emit_record_address(out, mod, record);
                } else if (record.type == 'F') {
                    if (emitted_start_records.insert(record.raw_key).second)
                        emit_record_address(out, mod, record);

                    auto end_address = module_symbol_address(mod, "X" + record.raw_key);
                    if (end_address.has_value()
                        && emitted_end_records.insert(record.raw_key).second) {
                        out << "L:X" << record.raw_key << ":"
                            << hex16(end_address.value()) << "\n";
                    }
                }
            }

            std::map<std::string, uint32_t> bases;
            for (const auto& area : mod.areas()) {
                if (area.placed_addr().has_value())
                    bases[area.name()] = area.placed_addr().value();
            }

            for (const auto& sym : mod.symbols()) {
                if (!sym.is_def())
                    continue;
                auto parsed = parse_c_line_symbol(sym.name());
                if (!parsed.has_value())
                    continue;

                const auto absolute =
                    debug_info_builder::symbol_absolute_addr(&mod, sym);
                out << "L:C$" << std::get<0>(parsed.value())
                    << "$" << std::get<1>(parsed.value())
                    << "$" << std::get<2>(parsed.value())
                    << "$" << std::get<3>(parsed.value())
                    << ":" << hex16(absolute) << "\n";
            }

            const std::string asm_name = mod.path().stem().string();
            for (const auto& entry : lst_lines) {
                auto base_it = bases.find(entry.area_name);
                if (base_it == bases.end())
                    continue;
                out << "L:A$" << asm_name
                    << "$" << entry.line
                    << ":" << hex16(base_it->second + entry.offset) << "\n";
            }
        }
    }

} // namespace xlink
