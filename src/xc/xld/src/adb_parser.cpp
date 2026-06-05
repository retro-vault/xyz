//
// SDCC .adb debug info parser
//
// MIT License (see: LICENSE)
// copyright (C) 2021 tomaz stih
//
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <sstream>

#include <xld/adb_parser.h>
#include <xld/errors.h>

namespace xld {

    namespace {

        struct parsed_name {
            char scope = '\0';
            std::string module_name;
            std::string display_name;
            std::optional<std::string> parent_name;
            std::optional<uint32_t> line;
            bool file_local = false;
            bool global_scope = false;
            bool valid = false;
        };

        static std::string trim(const std::string& s) {
            auto start = s.find_first_not_of(" \t\r\n");
            if (start == std::string::npos) return "";
            auto end = s.find_last_not_of(" \t\r\n");
            return s.substr(start, end - start + 1);
        }

        static std::vector<std::string> split_top_level(
            const std::string& value, char delimiter)
        {
            std::vector<std::string> result;
            std::string current;
            int paren_depth = 0;
            int bracket_depth = 0;

            for (char ch : value) {
                if (ch == '(') paren_depth++;
                else if (ch == ')' && paren_depth > 0) paren_depth--;
                else if (ch == '[') bracket_depth++;
                else if (ch == ']' && bracket_depth > 0) bracket_depth--;

                if (ch == delimiter && paren_depth == 0 && bracket_depth == 0) {
                    result.push_back(trim(current));
                    current.clear();
                } else {
                    current.push_back(ch);
                }
            }

            result.push_back(trim(current));
            return result;
        }

        static std::optional<uint32_t> parse_optional_u32(
            const std::string& value)
        {
            if (value.empty()) return std::nullopt;
            char* end = nullptr;
            unsigned long parsed = std::strtoul(value.c_str(), &end, 10);
            if (end == value.c_str() || *end != '\0')
                return std::nullopt;
            return static_cast<uint32_t>(parsed);
        }

        static std::optional<int32_t> parse_optional_i32(
            const std::string& value)
        {
            if (value.empty()) return std::nullopt;
            char* end = nullptr;
            long parsed = std::strtol(value.c_str(), &end, 10);
            if (end == value.c_str() || *end != '\0')
                return std::nullopt;
            return static_cast<int32_t>(parsed);
        }

        static std::string canonical_pair_name(
            const std::vector<std::string>& regs)
        {
            if (regs.size() != 2)
                return "";
            std::string low = trim(regs[0]);
            std::string high = trim(regs[1]);
            if (low.empty() || high.empty())
                return "";
            return high + low;
        }

        static adb_type_info decode_type(const std::string& type_expr);

        static std::string decode_value_type(const std::vector<std::string>& tokens,
                                             std::size_t& index)
        {
            if (index >= tokens.size())
                return "unknown";

            std::string token = trim(tokens[index++]);
            if (token.empty())
                return "unknown";

            if (token == "DG" || token == "DC") {
                return decode_value_type(tokens, index) + "*";
            }

            if (token.rfind("DA", 0) == 0 && token.back() == 'd') {
                auto count = token.substr(2, token.size() - 3);
                return decode_value_type(tokens, index) + "[" + count + "]";
            }

            if (token.rfind("ST", 0) == 0 && token.size() > 4
                && token.substr(token.size() - 2) == ":S") {
                return "struct " + token.substr(2, token.size() - 4);
            }

            if (token == "SV:S") return "void";
            if (token == "SC:U") return "unsigned char";
            if (token == "SC:S") return "char";
            if (token == "SI:U") return "unsigned int";
            if (token == "SI:S") return "int";
            if (token == ":U") return "unsigned char";
            if (token == ":S") return "char";

            return token;
        }

        static adb_type_info decode_type(const std::string& type_expr) {
            adb_type_info info;

            auto open = type_expr.find('{');
            auto close = type_expr.find('}');
            if (open != std::string::npos && close != std::string::npos
                && close > open + 1) {
                info.size = static_cast<uint32_t>(
                    std::stoul(type_expr.substr(open + 1, close - open - 1),
                               nullptr, 10));
            }

            std::string payload = type_expr;
            if (close != std::string::npos && close + 1 < type_expr.size())
                payload = type_expr.substr(close + 1);

            auto tokens = split_top_level(payload, ',');
            if (tokens.empty()) {
                info.name = "unknown";
                return info;
            }

            std::size_t index = 0;
            if (trim(tokens[0]) == "DF") {
                info.is_function = true;
                index = 1;
            }
            info.name = decode_value_type(tokens, index);
            if (info.name.empty())
                info.name = "unknown";
            return info;
        }

        static parsed_name parse_name(const std::string& raw_name) {
            parsed_name result;
            result.valid = false;

            if (raw_name.size() >= 2 && raw_name[1] == '$'
                && (raw_name[0] == 'G' || raw_name[0] == 'F')) {
                result.scope = raw_name[0];
                result.file_local = (raw_name[0] == 'F');
                result.global_scope = (raw_name[0] == 'G');

                auto first = raw_name.find('$');
                auto second = raw_name.find('$', first + 1);
                if (second == std::string::npos)
                    return result;

                if (raw_name[0] == 'F') {
                    result.module_name = raw_name.substr(1, first - 1);
                }

                std::string base = raw_name.substr(first + 1, second - first - 1);
                if (raw_name[0] == 'G') {
                    result.display_name = "_" + base;
                } else {
                    result.display_name = base;
                }
                result.line = parse_optional_u32(
                    raw_name.substr(raw_name.rfind('$') + 1));
                result.valid = true;
                return result;
            }

            if (raw_name.rfind("L", 0) == 0) {
                result.scope = 'L';
                auto first = raw_name.find('$');
                auto second = raw_name.find('$', first + 1);
                if (first == std::string::npos || second == std::string::npos)
                    return result;

                std::string owner = raw_name.substr(1, first - 1);
                auto dot = owner.rfind('.');
                if (dot == std::string::npos)
                    return result;

                result.module_name = owner.substr(0, dot);
                std::string function_name = owner.substr(dot + 1);
                if (!function_name.empty() && function_name[0] != '_')
                    function_name = "_" + function_name;
                result.parent_name = function_name;
                result.display_name = raw_name.substr(
                    first + 1, second - first - 1);
                result.line = parse_optional_u32(
                    raw_name.substr(raw_name.rfind('$') + 1));
                result.valid = true;
                return result;
            }

            return result;
        }

        static void apply_storage(adb_symbol_info& symbol,
                                  const std::vector<std::string>& fields)
        {
            if (fields.empty())
                return;

            if (fields[0] == "E") {
                symbol.storage = adb_storage_class::address;
                return;
            }

            if (fields[0] == "B") {
                symbol.storage = adb_storage_class::frame_relative;
                if (!fields.empty())
                    symbol.offset = parse_optional_i32(fields.back());
                return;
            }

            if (fields[0] == "R") {
                if (!fields.empty()) {
                    auto regs = fields.back();
                    if (regs.size() >= 2 && regs.front() == '['
                        && regs.back() == ']') {
                        auto inner = regs.substr(1, regs.size() - 2);
                        auto reg_tokens = split_top_level(inner, ',');
                        if (reg_tokens.size() == 2) {
                            std::string pair = canonical_pair_name(reg_tokens);
                            if (!pair.empty()) {
                                symbol.storage = adb_storage_class::register_pair;
                                symbol.register_name = pair;
                            }
                        } else if (reg_tokens.size() == 1) {
                            auto reg = trim(reg_tokens[0]);
                            if (!reg.empty()) {
                                symbol.storage = adb_storage_class::register_name;
                                symbol.register_name = reg;
                            }
                        }
                    }
                }
            }
        }

        static void parse_record(const std::string& source_name,
                                 int line_num,
                                 const std::string& record,
                                 adb_document& doc)
        {
            if (record.size() < 3 || record[1] != ':')
                throw parse_error(source_name, line_num,
                    "malformed adb record prefix");

            const char record_type = record[0];
            const std::string body = record.substr(2);

            auto open = body.find('(');
            if (open == std::string::npos)
                throw parse_error(source_name, line_num,
                    "malformed adb record");

            int depth = 0;
            std::size_t close = std::string::npos;
            for (std::size_t i = open; i < body.size(); ++i) {
                if (body[i] == '(') depth++;
                else if (body[i] == ')') {
                    depth--;
                    if (depth == 0) {
                        close = i;
                        break;
                    }
                }
            }
            if (close == std::string::npos)
                throw parse_error(source_name, line_num,
                    "unterminated adb type payload");

            std::string raw_name = body.substr(0, open);
            std::string type_expr = body.substr(open + 1, close - open - 1);
            std::string rest = (close + 1 < body.size())
                ? body.substr(close + 1) : "";
            if (!rest.empty() && rest[0] == ',')
                rest.erase(0, 1);
            auto fields = split_top_level(rest, ',');

            auto name = parse_name(raw_name);
            if (!name.valid)
                return;

            auto type = decode_type(type_expr);

            if (record_type == 'F') {
                adb_function_info function;
                function.raw_name = raw_name;
                function.display_name = name.display_name;
                function.return_type = type;
                function.line = name.line;
                function.file_local = name.file_local;
                doc.functions.push_back(std::move(function));
                return;
            }

            if (record_type == 'S') {
                if (name.scope == 'L') {
                    adb_symbol_info local;
                    local.raw_name = raw_name;
                    local.display_name = name.display_name;
                    local.parent_name = name.parent_name;
                    local.type = type;
                    local.line = name.line;
                    apply_storage(local, fields);
                    doc.locals.push_back(std::move(local));
                    return;
                }

                if (type.is_function)
                    return;

                adb_symbol_info global;
                global.raw_name = raw_name;
                global.display_name = name.display_name;
                global.type = type;
                global.line = name.line;
                global.file_local = name.file_local;
                global.global_scope = name.global_scope;
                apply_storage(global, fields);
                doc.globals.push_back(std::move(global));
            }
        }

    } // namespace

    adb_document adb_parser::parse(const std::filesystem::path& path) {
        std::ifstream input(path);
        if (!input.is_open())
            throw parse_error("cannot open adb file: " + path.string());
        return parse(path.string(), input);
    }

    adb_document adb_parser::parse(const std::string& source_name,
                                   std::istream& input)
    {
        adb_document doc;
        std::string line;
        int line_num = 0;

        while (std::getline(input, line)) {
            line_num++;
            line = trim(line);
            if (line.empty())
                continue;

            if (line.rfind("M:", 0) == 0) {
                doc.module_name = line.substr(2);
                continue;
            }

            if (line.rfind("F:", 0) == 0 || line.rfind("S:", 0) == 0) {
                parse_record(source_name, line_num, line, doc);
            }
        }

        return doc;
    }

} // namespace xld
