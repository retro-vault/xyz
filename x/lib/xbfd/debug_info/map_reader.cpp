// debug_info/map_reader.cpp — xbfd::map_reader: SDCC MAP file reader.
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
//
#include <fstream>
#include <regex>
#include <string_view>

#include <xbfd/xbfd.h>

namespace xbfd {

namespace {

std::optional<std::vector<std::string>> read_lines(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open())
        return std::nullopt;

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(file, line))
        lines.push_back(line);
    return lines;
}

std::string_view trim(std::string_view str) {
    const auto first = str.find_first_not_of(" \t\r\n");
    if (first == std::string_view::npos)
        return {};
    const auto last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, last - first + 1);
}

bool symbol_names_match(std::string_view left, std::string_view right) {
    return left == right
        || (left.size() == right.size() + 1
            && left.front() == '_'
            && left.substr(1) == right)
        || (right.size() == left.size() + 1
            && right.front() == '_'
            && right.substr(1) == left);
}

void merge_symbol_address(debug_info& info,
                          const std::string& name,
                          uint32_t address) {
    bool matched_symbol = false;

    for (auto& symbol : info.symbols) {
        if (!symbol_names_match(symbol.name, name))
            continue;
        symbol.address = address;
        matched_symbol = true;
    }

    for (auto& variable : info.variables) {
        if (!variable.parent.empty())
            continue;
        if (!symbol_names_match(variable.name, name))
            continue;
        variable.offset = static_cast<int>(address);
    }

    if (!matched_symbol)
        info.symbols.push_back({name, address});
}

std::optional<debug_info> parse_map_file(const std::string& path) {
    auto lines = read_lines(path);
    if (!lines)
        return std::nullopt;

    debug_info info;

    const std::regex segment_re(
        R"(^\s*([A-Za-z0-9_.\$]+)\s+((?:0[xX])?[0-9A-Fa-f]{4,8})\s+((?:0[xX])?[0-9A-Fa-f]{4,8}).*\(([^)]*)\)\s*$)");
    const std::regex symbol_re(
        R"(^\s*((?:0[xX])?[0-9A-Fa-f]{4,8})\s+([^\s]+)(?:\s+([^\s]+))?\s*$)");
    const std::regex column_sym_re(
        R"(\s*((?:0[xX])?[0-9A-Fa-f]{4,8})\s+([^\s|]+))");

    for (const auto& raw : *lines) {
        std::string line(raw);
        const auto cleaned = trim(line);
        if (cleaned.empty())
            continue;

        std::match_results<std::string_view::const_iterator> match;
        if (std::regex_match(cleaned.begin(), cleaned.end(), match, segment_re))
            continue;

        if (std::regex_match(cleaned.begin(), cleaned.end(), match, symbol_re)) {
            if (match[1].str() == "Value" || match[2].str() == "Global")
                continue;

            try {
                merge_symbol_address(
                    info,
                    match[2].str(),
                    static_cast<uint32_t>(std::stoul(match[1].str(), nullptr, 16)));
                continue;
            } catch (...) {}
        }

        if (line.find('|') == std::string::npos)
            continue;

        for (auto it = std::sregex_iterator(line.begin(), line.end(), column_sym_re);
             it != std::sregex_iterator();
             ++it) {
            try {
                const auto address = (*it)[1].str();
                const auto name = (*it)[2].str();
                if (address == "Value" || name == "Global" || name == "------")
                    continue;

                merge_symbol_address(
                    info,
                    name,
                    static_cast<uint32_t>(std::stoul(address, nullptr, 16)));
            } catch (...) {}
        }
    }

    return info;
}

} // namespace

std::optional<debug_info> map_reader::read(const std::string& path) {
    return parse_map_file(path);
}

std::optional<debug_info> map_reader::read(const std::string& path, debug_info base) {
    auto supplement = read(path);
    if (!supplement)
        return std::nullopt;
    return merge(std::move(base), *supplement);
}

debug_info map_reader::merge(debug_info base, const debug_info& supplement) {
    for (const auto& file : supplement.files)
        base.files.push_back(file);
    for (const auto& function : supplement.functions)
        base.functions.push_back(function);
    for (const auto& variable : supplement.variables)
        base.variables.push_back(variable);
    for (const auto& line : supplement.lines)
        base.lines.push_back(line);
    for (const auto& symbol : supplement.symbols)
        merge_symbol_address(base, symbol.name, symbol.address);
    return base;
}

} // namespace xbfd
