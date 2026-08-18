//
// sdcc_lscript.cpp — parse an SDCC/xld-friendly script format.
//
// Supported forms:
//   ENTRY symbol
//   FORMAT xl|bin|elf|ihx
//   AREA _CODE = 0100
//   RANGE 0000-7FFF
//   RESERVE 0100-017F
//   COPY _DATA
//   SDCC command files with one option per line:
//     -b _CODE = 0100
//     -i / -m / -p / -z
//     -k path
//     -l z80
//     crt0.rel
//     main.rel
//     -e
//
// And command-file style aliases:
//   -e symbol
//   -f bin
//   -b _CODE=0100
//   -x 0000-7FFF
//   -r 0100-017F
//   -Ttext=0100 / -Tdata=... / -Tbss=...
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//
#include <xbfd/lscript.h>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace xbfd {
namespace {

static std::string trim(std::string text) {
    auto not_space = [](unsigned char ch) { return !std::isspace(ch); };
    text.erase(text.begin(),
               std::find_if(text.begin(), text.end(), not_space));
    text.erase(std::find_if(text.rbegin(), text.rend(), not_space).base(),
               text.end());
    return text;
}

static std::vector<std::string> split_ws(const std::string& line) {
    std::istringstream input(line);
    std::vector<std::string> out;
    for (std::string part; input >> part; )
        out.push_back(part);
    return out;
}

static uint16_t parse_u16_sdcc(const std::string& text,
                               const std::filesystem::path& path,
                               int line)
{
    try {
        unsigned long value = 0;
        if (text.size() > 2 && text[0] == '0'
            && (text[1] == 'x' || text[1] == 'X')) {
            value = std::stoul(text, nullptr, 16);
        } else {
            value = std::stoul(text, nullptr, 16);
        }
        if (value > 0xFFFFu)
            throw lscript_error(path.string() + ":" + std::to_string(line)
                                + ": address out of 16-bit range");
        return static_cast<uint16_t>(value);
    } catch (const std::exception&) {
        throw lscript_error(path.string() + ":" + std::to_string(line)
                            + ": invalid number '" + text + "'");
    }
}

static lscript_address_range parse_range_sdcc(const std::string& text,
                                              const std::filesystem::path& path,
                                              int line)
{
    auto dash = text.find('-');
    auto comma = text.find(',');
    auto sep = dash != std::string::npos ? dash : comma;
    if (sep == std::string::npos) {
        throw lscript_error(path.string() + ":" + std::to_string(line)
                            + ": expected range start-end");
    }
    return {
        parse_u16_sdcc(trim(text.substr(0, sep)), path, line),
        parse_u16_sdcc(trim(text.substr(sep + 1)), path, line)
    };
}

static lscript_output_format parse_format_sdcc(
    const std::string& text,
    const std::filesystem::path& path,
    int line)
{
    if (text == "bin" || text == "binary")
        return lscript_output_format::bin;
    if (text == "xl")
        return lscript_output_format::xl;
    if (text == "elf")
        return lscript_output_format::elf;
    if (text == "ihx")
        return lscript_output_format::ihx;
    throw lscript_error(path.string() + ":" + std::to_string(line)
                        + ": unsupported format '" + text + "'");
}

static void parse_area_assignment(sdcc_lscript& out,
                                  const std::string& value,
                                  const std::filesystem::path& path,
                                  int line)
{
    auto eq = value.find('=');
    if (eq == std::string::npos) {
        throw lscript_error(path.string() + ":" + std::to_string(line)
                            + ": expected AREA=ADDR");
    }
    const auto name = trim(value.substr(0, eq));
    const auto addr = trim(value.substr(eq + 1));
    out.set_area_base(name, parse_u16_sdcc(addr, path, line));
}

static std::string strip_comments(const std::string& line) {
    std::size_t cut = std::string::npos;
    for (char marker : {';', '#'}) {
        auto pos = line.find(marker);
        if (pos != std::string::npos)
            cut = (cut == std::string::npos) ? pos : std::min(cut, pos);
    }
    if (cut == std::string::npos)
        return line;
    return line.substr(0, cut);
}

static bool looks_like_input_file(const std::string& text) {
    auto ext = std::filesystem::path(text).extension().string();
    return ext == ".rel" || ext == ".lib" || ext == ".o" || ext == ".a";
}

} // namespace

std::unique_ptr<sdcc_lscript> sdcc_lscript::read(
    const std::filesystem::path& path)
{
    std::ifstream input(path);
    if (!input.is_open())
        throw lscript_error("cannot open linker script '" + path.string() + "'");

    auto script = std::make_unique<sdcc_lscript>();
    std::string line;
    int line_no = 0;
    while (std::getline(input, line)) {
        ++line_no;
        auto cleaned = trim(strip_comments(line));
        if (cleaned.empty())
            continue;

        const auto parts = split_ws(cleaned);
        if (parts.empty())
            continue;

        const auto& head = parts[0];
        if (head == "ENTRY") {
            if (parts.size() < 2)
                throw lscript_error(path.string() + ":" + std::to_string(line_no)
                                    + ": ENTRY requires a symbol");
            script->set_entry_symbol(parts[1]);
            continue;
        }
        if (head == "FORMAT") {
            if (parts.size() < 2)
                throw lscript_error(path.string() + ":" + std::to_string(line_no)
                                    + ": FORMAT requires a value");
            script->set_output_format(parse_format_sdcc(parts[1], path, line_no));
            continue;
        }
        if (head == "AREA") {
            auto rest = trim(cleaned.substr(std::string("AREA").size()));
            parse_area_assignment(*script, rest, path, line_no);
            continue;
        }
        if (head == "RANGE") {
            auto rest = trim(cleaned.substr(std::string("RANGE").size()));
            script->set_output_range(parse_range_sdcc(rest, path, line_no));
            continue;
        }
        if (head == "RESERVE") {
            auto rest = trim(cleaned.substr(std::string("RESERVE").size()));
            script->add_reserved_range(parse_range_sdcc(rest, path, line_no));
            continue;
        }
        if (head == "COPY") {
            if (parts.size() != 2)
                throw lscript_error(path.string() + ":" + std::to_string(line_no)
                                    + ": COPY requires one area name");
            script->add_load_copy_area(parts[1]);
            continue;
        }

        if (head == "-e") {
            if (parts.size() == 1)
                break;
            script->set_entry_symbol(parts[1]);
            continue;
        }
        if (head == "-f") {
            if (parts.size() < 2)
                throw lscript_error(path.string() + ":" + std::to_string(line_no)
                                    + ": -f requires a format");
            script->set_output_format(parse_format_sdcc(parts[1], path, line_no));
            continue;
        }
        if (head == "-b") {
            auto rest = trim(cleaned.substr(2));
            if (rest.empty())
                throw lscript_error(path.string() + ":" + std::to_string(line_no)
                                    + ": -b requires AREA=ADDR");
            parse_area_assignment(*script, rest, path, line_no);
            continue;
        }
        if (head == "-x") {
            if (parts.size() < 2)
                throw lscript_error(path.string() + ":" + std::to_string(line_no)
                                    + ": -x requires a range");
            script->set_output_range(parse_range_sdcc(parts[1], path, line_no));
            continue;
        }
        if (head == "-r") {
            if (parts.size() < 2)
                throw lscript_error(path.string() + ":" + std::to_string(line_no)
                                    + ": -r requires a range");
            script->add_reserved_range(parse_range_sdcc(parts[1], path, line_no));
            continue;
        }
        if (head.rfind("-Ttext=", 0) == 0) {
            script->set_area_base("_CODE",
                parse_u16_sdcc(head.substr(std::string("-Ttext=").size()),
                               path, line_no));
            continue;
        }
        if (head.rfind("-Tdata=", 0) == 0) {
            script->set_area_base("_DATA",
                parse_u16_sdcc(head.substr(std::string("-Tdata=").size()),
                               path, line_no));
            continue;
        }
        if (head.rfind("-Tbss=", 0) == 0) {
            script->set_area_base("_BSS",
                parse_u16_sdcc(head.substr(std::string("-Tbss=").size()),
                               path, line_no));
            continue;
        }
        if (head == "-i") {
            script->set_output_format(lscript_output_format::ihx);
            continue;
        }
        if (head == "-p" || head == "-m" || head == "-z") {
            continue;
        }
        if (head == "-k" || head == "-l") {
            if (parts.size() < 2)
                throw lscript_error(path.string() + ":" + std::to_string(line_no)
                                    + ": " + head + " requires an argument");
            continue;
        }
        if (looks_like_input_file(head)) {
            continue;
        }

        throw lscript_error(path.string() + ":" + std::to_string(line_no)
                            + ": unsupported SDCC linker-script line '" + cleaned + "'");
    }

    return script;
}

} // namespace xbfd
