/*
 * Implements the xdbg document parser and serializer used to read,
 * validate, and emit hosted debugger metadata files and streams.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>
#include <string>
#include <utility>

#include <xdbg/error.hpp>
#include <xdbg/io.hpp>

namespace xdbg {

    namespace {

        using field_map = std::map<std::string, std::string>;

        std::string trim(const std::string& value) {
            const auto start = value.find_first_not_of(" \t\r\n");
            if (start == std::string::npos)
                return "";

            const auto end = value.find_last_not_of(" \t\r\n");
            return value.substr(start, end - start + 1);
        }

        bool is_identifier_char(char ch) {
            return std::isalnum(static_cast<unsigned char>(ch))
                || ch == '_' || ch == '-';
        }

        std::string read_token(const std::string& line, std::size_t& pos) {
            while (pos < line.size()
                   && std::isspace(static_cast<unsigned char>(line[pos]))) {
                ++pos;
            }

            if (pos >= line.size())
                return "";

            if (line[pos] == '"') {
                ++pos;
                std::string result;
                bool escaped = false;
                while (pos < line.size()) {
                    const char ch = line[pos++];
                    if (escaped) {
                        result.push_back(ch);
                        escaped = false;
                    } else if (ch == '\\') {
                        escaped = true;
                    } else if (ch == '"') {
                        return result;
                    } else {
                        result.push_back(ch);
                    }
                }
                throw error("unterminated quoted string");
            }

            const std::size_t start = pos;
            while (pos < line.size()
                   && !std::isspace(static_cast<unsigned char>(line[pos]))) {
                ++pos;
            }
            return line.substr(start, pos - start);
        }

        field_map parse_fields(const std::string& line, std::size_t line_number) {
            field_map fields;
            std::size_t pos = 0;
            while (true) {
                while (pos < line.size()
                       && std::isspace(static_cast<unsigned char>(line[pos]))) {
                    ++pos;
                }
                if (pos >= line.size())
                    break;

                const std::size_t key_start = pos;
                while (pos < line.size() && is_identifier_char(line[pos])) {
                    ++pos;
                }
                if (key_start == pos || pos >= line.size() || line[pos] != '=') {
                    throw parse_error(line_number, "expected key=value field");
                }

                const std::string key = line.substr(key_start, pos - key_start);
                ++pos; // skip '='
                std::string value = read_token(line, pos);
                fields[key] = value;
            }
            return fields;
        }

        std::string require_field(const field_map& fields,
                                  const std::string& key,
                                  std::size_t line_number) {
            const auto it = fields.find(key);
            if (it == fields.end()) {
                throw parse_error(line_number, "missing field '" + key + "'");
            }
            return it->second;
        }

        std::optional<std::string> optional_string_field(
            const field_map& fields, const std::string& key)
        {
            const auto it = fields.find(key);
            if (it == fields.end())
                return std::nullopt;
            return it->second;
        }

        uint32_t parse_u32(const std::string& value, std::size_t line_number) {
            char* end = nullptr;
            const unsigned long parsed = std::strtoul(value.c_str(), &end, 0);
            if (end == value.c_str() || *end != '\0') {
                throw parse_error(line_number, "invalid number '" + value + "'");
            }
            return static_cast<uint32_t>(parsed);
        }

        int32_t parse_i32(const std::string& value, std::size_t line_number) {
            char* end = nullptr;
            const long parsed = std::strtol(value.c_str(), &end, 0);
            if (end == value.c_str() || *end != '\0') {
                throw parse_error(line_number, "invalid signed number '" + value + "'");
            }
            return static_cast<int32_t>(parsed);
        }

        std::optional<uint32_t> optional_u32_field(
            const field_map& fields, const std::string& key, std::size_t line_number)
        {
            const auto it = fields.find(key);
            if (it == fields.end())
                return std::nullopt;
            return parse_u32(it->second, line_number);
        }

        std::optional<int32_t> optional_i32_field(
            const field_map& fields, const std::string& key, std::size_t line_number)
        {
            const auto it = fields.find(key);
            if (it == fields.end())
                return std::nullopt;
            return parse_i32(it->second, line_number);
        }

        std::string hex_u32(uint32_t value) {
            std::ostringstream out;
            out << "0x"
                << std::uppercase << std::hex
                << value
                << std::nouppercase << std::dec;
            return out.str();
        }

        std::string escape_string(const std::string& value) {
            std::string result;
            result.reserve(value.size());
            for (char ch : value) {
                if (ch == '"' || ch == '\\')
                    result.push_back('\\');
                result.push_back(ch);
            }
            return result;
        }

        std::string quote(const std::string& value) {
            return "\"" + escape_string(value) + "\"";
        }

        const char* to_string(language_kind value) {
            switch (value) {
            case language_kind::c: return "c";
            case language_kind::cxx: return "cxx";
            case language_kind::assembly: return "assembly";
            case language_kind::unknown: return "unknown";
            }
            return "unknown";
        }

        language_kind parse_language_kind(const std::string& value,
                                          std::size_t line_number) {
            if (value == "unknown") return language_kind::unknown;
            if (value == "c") return language_kind::c;
            if (value == "cxx") return language_kind::cxx;
            if (value == "assembly") return language_kind::assembly;
            throw parse_error(line_number, "unknown language '" + value + "'");
        }

        const char* to_string(symbol_kind value) {
            switch (value) {
            case symbol_kind::function: return "function";
            case symbol_kind::global: return "global";
            case symbol_kind::local: return "local";
            case symbol_kind::parameter: return "parameter";
            case symbol_kind::label: return "label";
            case symbol_kind::section: return "section";
            case symbol_kind::type: return "type";
            case symbol_kind::constant: return "constant";
            case symbol_kind::object: return "object";
            case symbol_kind::unknown: return "unknown";
            }
            return "unknown";
        }

        symbol_kind parse_symbol_kind(const std::string& value,
                                      std::size_t line_number) {
            if (value == "unknown") return symbol_kind::unknown;
            if (value == "function") return symbol_kind::function;
            if (value == "global") return symbol_kind::global;
            if (value == "local") return symbol_kind::local;
            if (value == "parameter") return symbol_kind::parameter;
            if (value == "label") return symbol_kind::label;
            if (value == "section") return symbol_kind::section;
            if (value == "type") return symbol_kind::type;
            if (value == "constant") return symbol_kind::constant;
            if (value == "object") return symbol_kind::object;
            throw parse_error(line_number, "unknown symbol kind '" + value + "'");
        }

        const char* to_string(storage_kind value) {
            switch (value) {
            case storage_kind::address: return "address";
            case storage_kind::stack: return "stack";
            case storage_kind::register_name: return "register_name";
            case storage_kind::register_pair: return "register_pair";
            case storage_kind::frame_relative: return "frame_relative";
            case storage_kind::unknown: return "unknown";
            }
            return "unknown";
        }

        storage_kind parse_storage_kind(const std::string& value,
                                        std::size_t line_number) {
            if (value == "unknown") return storage_kind::unknown;
            if (value == "address") return storage_kind::address;
            if (value == "stack") return storage_kind::stack;
            if (value == "register_name") return storage_kind::register_name;
            if (value == "register_pair") return storage_kind::register_pair;
            if (value == "frame_relative") return storage_kind::frame_relative;
            throw parse_error(line_number, "unknown storage kind '" + value + "'");
        }

        void write_optional_u32(std::ostream& output,
                                const char* key,
                                const std::optional<uint32_t>& value) {
            if (value.has_value()) {
                output << ' ' << key << '=' << hex_u32(value.value());
            }
        }

        void write_optional_i32(std::ostream& output,
                                const char* key,
                                const std::optional<int32_t>& value) {
            if (value.has_value()) {
                output << ' ' << key << '=' << value.value();
            }
        }

        void write_optional_string(std::ostream& output,
                                   const char* key,
                                   const std::optional<std::string>& value) {
            if (value.has_value()) {
                output << ' ' << key << '=' << quote(value.value());
            }
        }

        void write_optional_file_line(std::ostream& output,
                                      const std::optional<uint32_t>& file_id,
                                      const std::optional<uint32_t>& line,
                                      const std::optional<uint32_t>& column) {
            write_optional_u32(output, "file", file_id);
            write_optional_u32(output, "line", line);
            write_optional_u32(output, "column", column);
        }

    } // namespace

    document read(std::istream& input) {
        document doc;
        std::string line;
        std::size_t line_number = 0;
        bool saw_header = false;

        while (std::getline(input, line)) {
            ++line_number;
            line = trim(line);
            if (line.empty() || line[0] == '#')
                continue;

            std::size_t pos = 0;
            const std::string record = read_token(line, pos);

            if (record == "xdbg") {
                const std::string version = read_token(line, pos);
                if (version.empty()) {
                    throw parse_error(line_number, "missing xdbg version");
                }
                doc.version = parse_u32(version, line_number);
                saw_header = true;
                continue;
            }

            if (!saw_header) {
                throw parse_error(line_number, "missing xdbg header");
            }

            const field_map fields = parse_fields(line.substr(pos), line_number);

            if (record == "image") {
                doc.image_path = require_field(fields, "path", line_number);
            } else if (record == "entry") {
                doc.entry_address = parse_u32(
                    require_field(fields, "address", line_number), line_number);
            } else if (record == "file") {
                source_file file;
                file.id = parse_u32(require_field(fields, "id", line_number),
                                    line_number);
                file.path = require_field(fields, "path", line_number);
                if (const auto language = optional_string_field(fields, "language")) {
                    file.language = parse_language_kind(language.value(), line_number);
                }
                doc.files.push_back(std::move(file));
            } else if (record == "symbol") {
                symbol item;
                item.name = require_field(fields, "name", line_number);
                item.kind = parse_symbol_kind(
                    require_field(fields, "kind", line_number), line_number);
                item.address = parse_u32(
                    require_field(fields, "address", line_number), line_number);
                item.size = optional_u32_field(fields, "size", line_number);
                item.file_id = optional_u32_field(fields, "file", line_number);
                item.line = optional_u32_field(fields, "line", line_number);
                item.column = optional_u32_field(fields, "column", line_number);
                item.type_name = optional_string_field(fields, "type");
                item.parent_name = optional_string_field(fields, "parent");
                if (const auto language = optional_string_field(fields, "language")) {
                    item.language = parse_language_kind(language.value(), line_number);
                }
                doc.symbols.push_back(std::move(item));
            } else if (record == "function") {
                function item;
                item.name = require_field(fields, "name", line_number);
                item.start_address = parse_u32(
                    require_field(fields, "start", line_number), line_number);
                item.end_address = parse_u32(
                    require_field(fields, "end", line_number), line_number);
                item.file_id = optional_u32_field(fields, "file", line_number);
                item.line = optional_u32_field(fields, "line", line_number);
                item.column = optional_u32_field(fields, "column", line_number);
                item.return_type = optional_string_field(fields, "return_type");
                if (const auto language = optional_string_field(fields, "language")) {
                    item.language = parse_language_kind(language.value(), line_number);
                }
                doc.functions.push_back(std::move(item));
            } else if (record == "line") {
                line_entry item;
                item.address = parse_u32(
                    require_field(fields, "address", line_number), line_number);
                item.file_id = parse_u32(
                    require_field(fields, "file", line_number), line_number);
                item.line = parse_u32(
                    require_field(fields, "line", line_number), line_number);
                if (const auto column = optional_u32_field(fields, "column", line_number)) {
                    item.column = column.value();
                }
                doc.lines.push_back(item);
            } else if (record == "variable") {
                variable item;
                item.name = require_field(fields, "name", line_number);
                item.kind = parse_symbol_kind(
                    require_field(fields, "kind", line_number), line_number);
                item.parent_name = optional_string_field(fields, "parent");
                item.storage = parse_storage_kind(
                    require_field(fields, "storage", line_number), line_number);
                item.address = optional_u32_field(fields, "address", line_number);
                item.offset = optional_i32_field(fields, "offset", line_number);
                item.register_name = optional_string_field(fields, "register");
                item.type_name = optional_string_field(fields, "type");
                item.start_address = optional_u32_field(fields, "start", line_number);
                item.end_address = optional_u32_field(fields, "end", line_number);
                item.file_id = optional_u32_field(fields, "file", line_number);
                item.line = optional_u32_field(fields, "line", line_number);
                item.column = optional_u32_field(fields, "column", line_number);
                if (const auto language = optional_string_field(fields, "language")) {
                    item.language = parse_language_kind(language.value(), line_number);
                }
                doc.variables.push_back(std::move(item));
            } else {
                throw parse_error(line_number, "unknown record type '" + record + "'");
            }
        }

        if (!saw_header) {
            throw error("missing xdbg header");
        }

        return doc;
    }

    document read_file(const std::filesystem::path& path) {
        std::ifstream input(path);
        if (!input.is_open()) {
            throw error("cannot open xdbg file: " + path.string());
        }
        return read(input);
    }

    void write(std::ostream& output, const document& doc) {
        output << "xdbg " << doc.version << "\n";

        if (doc.image_path.has_value()) {
            output << "image path=" << quote(doc.image_path.value()) << "\n";
        }
        if (doc.entry_address.has_value()) {
            output << "entry address=" << hex_u32(doc.entry_address.value()) << "\n";
        }

        for (const auto& file : doc.files) {
            output << "file"
                   << " id=" << file.id
                   << " path=" << quote(file.path)
                   << " language=" << quote(to_string(file.language))
                   << "\n";
        }

        for (const auto& item : doc.symbols) {
            output << "symbol"
                   << " name=" << quote(item.name)
                   << " kind=" << quote(to_string(item.kind))
                   << " address=" << hex_u32(item.address)
                   << " language=" << quote(to_string(item.language));
            write_optional_u32(output, "size", item.size);
            write_optional_file_line(output, item.file_id, item.line, item.column);
            write_optional_string(output, "type", item.type_name);
            write_optional_string(output, "parent", item.parent_name);
            output << "\n";
        }

        for (const auto& item : doc.functions) {
            output << "function"
                   << " name=" << quote(item.name)
                   << " start=" << hex_u32(item.start_address)
                   << " end=" << hex_u32(item.end_address)
                   << " language=" << quote(to_string(item.language));
            write_optional_file_line(output, item.file_id, item.line, item.column);
            write_optional_string(output, "return_type", item.return_type);
            output << "\n";
        }

        for (const auto& item : doc.lines) {
            output << "line"
                   << " address=" << hex_u32(item.address)
                   << " file=" << item.file_id
                   << " line=" << item.line;
            if (item.column != 0) {
                output << " column=" << item.column;
            }
            output << "\n";
        }

        for (const auto& item : doc.variables) {
            output << "variable"
                   << " name=" << quote(item.name)
                   << " kind=" << quote(to_string(item.kind))
                   << " storage=" << quote(to_string(item.storage))
                   << " language=" << quote(to_string(item.language));
            write_optional_string(output, "parent", item.parent_name);
            write_optional_u32(output, "address", item.address);
            write_optional_i32(output, "offset", item.offset);
            write_optional_string(output, "register", item.register_name);
            write_optional_string(output, "type", item.type_name);
            write_optional_u32(output, "start", item.start_address);
            write_optional_u32(output, "end", item.end_address);
            write_optional_file_line(output, item.file_id, item.line, item.column);
            output << "\n";
        }
    }

    void write_file(const std::filesystem::path& path, const document& doc) {
        std::ofstream output(path);
        if (!output.is_open()) {
            throw error("cannot open xdbg file for writing: " + path.string());
        }
        write(output, doc);
    }

} // namespace xdbg
