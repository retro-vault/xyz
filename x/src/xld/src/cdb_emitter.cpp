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
#include <map>
#include <optional>
#include <regex>
#include <sstream>
#include <set>
#include <string>
#include <vector>

#include <xld/cdb_emitter.h>
#include <xld/debug_info.h>
#include <xld/errors.h>
#include <xld/lst_parser.h>

namespace xld {

    namespace {

        struct compiler_record {
            char type = '\0';
            std::string line;
            std::string raw_key;
            std::string linked_key;
            char address_space = 'Z';
            bool on_stack = false;
        };

        struct xcc_adb_function_record {
            std::string source_file;
            std::string mangled_name;
            uint32_t line = 0;
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

        static uint32_t area_end_address(const module& mod, int area_index)
        {
            if (area_index < 0
                || area_index >= static_cast<int>(mod.areas().size())) {
                return 0;
            }

            const auto& area = mod.areas()[area_index];
            if (!area.placed_addr().has_value())
                return 0;

            return static_cast<uint32_t>(
                area.placed_addr().value() + area.size());
        }

        static std::optional<std::filesystem::path> source_path(
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
            {
                const auto basename = path.filename();
                std::vector<std::filesystem::path> matches;
                std::error_code ec;
                for (auto it = std::filesystem::recursive_directory_iterator(
                         std::filesystem::current_path(),
                         std::filesystem::directory_options::skip_permission_denied,
                         ec);
                     !ec && it != std::filesystem::recursive_directory_iterator();
                     it.increment(ec)) {
                    if (ec) break;
                    if (!it->is_regular_file())
                        continue;
                    if (it->path().filename() == basename)
                        matches.push_back(it->path());
                }

                if (matches.empty())
                    return std::nullopt;

                std::sort(matches.begin(), matches.end(),
                          [](const auto& a, const auto& b) {
                              const auto as = a.string();
                              const auto bs = b.string();
                              const bool a_build = as.find("/build/") != std::string::npos;
                              const bool b_build = bs.find("/build/") != std::string::npos;
                              if (a_build != b_build)
                                  return !a_build;
                              return as < bs;
                          });
                return matches.front();
            }
            return path;
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
            record.linked_key = record.raw_key;

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

        static std::string normalize_function_symbol_key(
            const std::string& key)
        {
            static const std::regex function_key_re(
                R"(^([A-Z])\$([^$]+)\$([0-9]+)_([0-9]+)\$([0-9]+)$)");
            std::smatch match;
            if (!std::regex_match(key, match, function_key_re)
                || match.size() != 6) {
                return key;
            }

            return match[1].str() + "$"
                + match[2].str() + "$"
                + match[3].str() + "$"
                + match[5].str();
        }

        static void finalize_compiler_record(compiler_record& record)
        {
            record.linked_key = record.raw_key;
            if (record.type == 'F'
                || (record.type == 'S' && record.address_space == 'C')) {
                record.linked_key = normalize_function_symbol_key(
                    record.raw_key);
            }
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
                // Preserve the full "module.function" owner so CDB parsers
                // (e.g. udap) can associate locals with their enclosing function.
                // Scope_part is "L<module>.<function>" — keep everything after 'L'.
                auto owner = scope_part.substr(1);
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
            auto record = parse_cdb_record_line(normalized);
            if (record.has_value())
                finalize_compiler_record(record.value());
            return record;
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
                        finalize_compiler_record(record.value());
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

        static std::vector<xcc_adb_function_record> read_xcc_adb_functions(
            const module& mod)
        {
            auto adb = sidecar_path(mod, ".adb");
            if (!adb.has_value())
                return {};

            std::ifstream in(adb.value());
            if (!in.is_open())
                throw parse_error("cannot open adb file: " + adb->string());

            std::vector<xcc_adb_function_record> records;
            std::string source_file;
            std::string line;
            while (std::getline(in, line)) {
                line = trim(line);
                if (line.empty())
                    continue;

                if (line.front() == '[' && line.back() == ']' && line.size() > 2) {
                    source_file = line.substr(1, line.size() - 2);
                    continue;
                }

                if (line.front() != 'F')
                    continue;

                const auto first_colon = line.find(':', 1);
                if (first_colon == std::string::npos)
                    continue;

                const auto second_colon = line.find(':', first_colon + 1);
                const std::string mangled = line.substr(1, first_colon - 1);
                const std::string line_text = second_colon == std::string::npos
                    ? line.substr(first_colon + 1)
                    : line.substr(first_colon + 1, second_colon - first_colon - 1);

                try {
                    xcc_adb_function_record record;
                    record.source_file = source_file;
                    record.mangled_name = mangled;
                    record.line = static_cast<uint32_t>(
                        std::stoul(line_text, nullptr, 10));
                    records.push_back(std::move(record));
                } catch (...) {
                }
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

        static std::string display_name_from_mangled(const std::string& mangled)
        {
            if (mangled.size() > 1 && mangled[0] == '_')
                return mangled.substr(1);
            return mangled;
        }

        static std::string cdb_global_key_from_mangled(const std::string& mangled)
        {
            return "G$" + display_name_from_mangled(mangled) + "$0$0";
        }

        static std::map<std::string, uint32_t> compute_xcc_function_ends(
            const module& mod,
            const std::vector<xcc_adb_function_record>& functions)
        {
            struct located_function {
                std::string mangled_name;
                uint32_t start = 0;
                uint32_t limit = 0;
            };

            std::vector<located_function> located;
            for (const auto& fn : functions) {
                auto start = module_symbol_address(mod, fn.mangled_name);
                if (!start.has_value())
                    continue;

                uint32_t limit = start.value() + 1;
                for (const auto& sym : mod.symbols()) {
                    if (!sym.is_def() || sym.name() != fn.mangled_name)
                        continue;
                    const auto area_limit = area_end_address(mod, sym.area_index());
                    if (area_limit > start.value())
                        limit = area_limit;
                    break;
                }

                located.push_back({fn.mangled_name, start.value(), limit});
            }

            std::sort(located.begin(), located.end(),
                      [](const auto& a, const auto& b) {
                          if (a.start != b.start)
                              return a.start < b.start;
                          return a.mangled_name < b.mangled_name;
                      });

            std::map<std::string, uint32_t> result;
            for (std::size_t i = 0; i < located.size(); ++i) {
                uint32_t end = located[i].limit;
                if (i + 1 < located.size()
                    && located[i + 1].start > located[i].start) {
                    end = std::min(end, located[i + 1].start);
                }
                if (end <= located[i].start)
                    end = located[i].start + 1;
                result[located[i].mangled_name] = end;
            }

            return result;
        }

        static std::vector<std::pair<uint32_t, uint32_t>>
        read_source_label_lines(const module& mod)
        {
            auto src = source_path(mod, ".s");
            if (!src.has_value())
                return {};

            std::ifstream in(src.value());
            if (!in.is_open())
                throw parse_error("cannot open source file: " + src->string());

            std::map<std::string, uint32_t> label_lines;
            static const std::regex label_re(
                R"(^\s*([A-Za-z_.$?][A-Za-z0-9_.$?]*)::?)");

            std::string line;
            uint32_t line_number = 0;
            while (std::getline(in, line)) {
                ++line_number;
                const auto comment = line.find(';');
                if (comment != std::string::npos)
                    line.erase(comment);

                std::smatch match;
                if (std::regex_search(line, match, label_re) && match.size() > 1) {
                    label_lines.emplace(match[1].str(), line_number);
                }
            }

            std::vector<std::pair<uint32_t, uint32_t>> result;
            for (const auto& sym : mod.symbols()) {
                if (!sym.is_def())
                    continue;
                auto it = label_lines.find(sym.name());
                if (it == label_lines.end())
                    continue;
                result.push_back({
                    it->second,
                    debug_info_builder::symbol_absolute_addr(&mod, sym)
                });
            }

            std::sort(result.begin(), result.end(),
                      [](const auto& a, const auto& b) {
                          if (a.second != b.second)
                              return a.second < b.second;
                          return a.first < b.first;
                      });

            result.erase(std::unique(result.begin(), result.end(),
                          [](const auto& a, const auto& b) {
                              return a.second == b.second;
                          }),
                         result.end());

            return result;
        }

        static bool emit_record_address(std::ostream& out,
                                        const module& mod,
                                        const compiler_record& record)
        {
            auto address = module_symbol_address(mod, record.linked_key);
            if (!address.has_value())
                return false;

            out << "L:" << record.linked_key << ":"
                << hex16(address.value()) << "\n";
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
            throw xld::xld_error("cannot open CDB output file: "
                                     + path.string());

        for (const auto& mod_ptr : ctx.modules) {
            const auto& mod = *mod_ptr;
            auto records = read_compiler_records(mod);
            auto lst_lines = read_lst_lines(mod);
            const auto xcc_functions = records.empty()
                ? read_xcc_adb_functions(mod)
                : std::vector<xcc_adb_function_record>{};
            const auto fallback_asm_lines = lst_lines.empty()
                ? read_source_label_lines(mod)
                : std::vector<std::pair<uint32_t, uint32_t>>{};

            if (records.empty()
                && (!lst_lines.empty()
                    || !fallback_asm_lines.empty()
                    || !xcc_functions.empty())) {
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

                    auto end_address = module_symbol_address(
                        mod, "X" + record.linked_key);
                    if (end_address.has_value()
                        && emitted_end_records.insert(record.raw_key).second) {
                        out << "L:X" << record.linked_key << ":"
                            << hex16(end_address.value()) << "\n";
                    }
                }
            }

            if (!xcc_functions.empty()) {
                const auto function_ends = compute_xcc_function_ends(
                    mod, xcc_functions);

                for (const auto& fn : xcc_functions) {
                    auto address = module_symbol_address(mod, fn.mangled_name);
                    if (!address.has_value())
                        continue;

                    const auto key = cdb_global_key_from_mangled(fn.mangled_name);
                    out << "F:" << key << "({2}DF,SI:S),C,0,0,0,0,0\n";
                    out << "L:" << key << ":" << hex16(address.value()) << "\n";

                    auto end_it = function_ends.find(fn.mangled_name);
                    if (end_it != function_ends.end()) {
                        out << "L:X" << key << ":" << hex16(end_it->second)
                            << "\n";
                    }

                    const auto& source_file = fn.source_file.empty()
                        ? mod.name()
                        : fn.source_file;
                    out << "L:C$" << source_file
                        << "$" << fn.line
                        << "$0$0:" << hex16(address.value()) << "\n";
                }
            }

            std::map<std::string, uint32_t> bases;
            for (const auto& area : mod.areas()) {
                if (area.placed_addr().has_value())
                    bases[area.name()] = area.placed_addr().value();
            }

            for (const auto& sym : mod.symbols()) {
                if (!sym.is_def() || sym.name().rfind("C$", 0) != 0)
                    continue;

                const auto absolute =
                    debug_info_builder::symbol_absolute_addr(&mod, sym);
                out << "L:" << sym.name()
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

            for (const auto& [line_num, address] : fallback_asm_lines) {
                out << "L:A$" << asm_name
                    << "$" << line_num
                    << ":" << hex16(address) << "\n";
            }
        }
    }

} // namespace xld
