#include "appmake/basic.h"

#include <cctype>
#include <format>
#include <iostream>
#include <stdexcept>

#include "appmake/tape.h"
#include "appmake/util.h"

namespace appmake {

std::string_view basic_token_text(uint8_t token) {
    switch (token) {
    case 0xa5: return "RND";
    case 0xa6: return "INKEY$";
    case 0xa7: return "PI";
    case 0xa8: return "FN ";
    case 0xa9: return "POINT ";
    case 0xaa: return "SCREEN$";
    case 0xab: return "ATTR ";
    case 0xac: return "AT ";
    case 0xad: return "TAB ";
    case 0xae: return "VAL$ ";
    case 0xaf: return "CODE";
    case 0xb0: return "VAL ";
    case 0xb1: return "LEN ";
    case 0xb2: return "SIN ";
    case 0xb3: return "COS ";
    case 0xb4: return "TAN ";
    case 0xb5: return "ASN ";
    case 0xb6: return "ACS ";
    case 0xb7: return "ATN ";
    case 0xb8: return "LN ";
    case 0xb9: return "EXP ";
    case 0xba: return "INT ";
    case 0xbb: return "SQR ";
    case 0xbc: return "SGN ";
    case 0xbd: return "ABS ";
    case 0xbe: return "PEEK ";
    case 0xbf: return "IN ";
    case 0xc0: return "USR ";
    case 0xc1: return "STR$ ";
    case 0xc2: return "CHR$ ";
    case 0xc3: return "NOT ";
    case 0xc4: return "BIN ";
    case 0xc5: return "OR ";
    case 0xc6: return "AND ";
    case 0xc7: return "<=";
    case 0xc8: return ">=";
    case 0xc9: return "<>";
    case 0xca: return "LINE ";
    case 0xcb: return "THEN ";
    case 0xcc: return "TO ";
    case 0xcd: return "STEP ";
    case 0xce: return "DEF FN ";
    case 0xcf: return "CAT";
    case 0xd0: return "FORMAT ";
    case 0xd1: return "MOVE ";
    case 0xd2: return "ERASE ";
    case 0xd3: return "OPEN #";
    case 0xd4: return "CLOSE #";
    case 0xd5: return "MERGE ";
    case 0xd6: return "VERIFY ";
    case 0xd7: return "BEEP ";
    case 0xd9: return "INK ";
    case 0xda: return "PAPER ";
    case 0xdb: return "FLASH ";
    case 0xdc: return "BRIGHT ";
    case 0xdd: return "INVERSE ";
    case 0xde: return "OVER ";
    case 0xdf: return "OUT ";
    case 0xe0: return "LPRINT ";
    case 0xe1: return "LLIST ";
    case 0xe2: return "STOP ";
    case 0xe3: return "READ ";
    case 0xe4: return "DATA ";
    case 0xe5: return "RESTORE ";
    case 0xe6: return "NEW";
    case 0xe7: return "BORDER ";
    case 0xe8: return "CONTINUE ";
    case 0xe9: return "DIM ";
    case 0xea: return "REM ";
    case 0xeb: return "FOR ";
    case 0xec: return "GO TO ";
    case 0xed: return "GO SUB ";
    case 0xee: return "INPUT ";
    case 0xef: return "LOAD ";
    case 0xf0: return "LIST ";
    case 0xf1: return "LET ";
    case 0xf2: return "PAUSE ";
    case 0xf3: return "NEXT ";
    case 0xf4: return "POKE ";
    case 0xf5: return "PRINT ";
    case 0xf6: return "PLOT ";
    case 0xf7: return "RUN ";
    case 0xf8: return "SAVE ";
    case 0xf9: return "RANDOMIZE ";
    case 0xfa: return "IF ";
    case 0xfb: return "CLS";
    case 0xfc: return "DRAW ";
    case 0xfd: return "CLEAR ";
    case 0xfe: return "RETURN ";
    case 0xff: return "COPY";
    default:   return "";
    }
}

std::string decode_basic_text(std::span<const uint8_t> bytes) {
    std::string out;

    for (std::size_t i = 0; i < bytes.size(); ++i) {
        const uint8_t b = bytes[i];

        if (b == 0x0d) {
            break;
        }

        if (b == 0x0e) {
            if (i + 5 < bytes.size()) {
                i += 5;
            }
            continue;
        }

        if (const std::string_view token = basic_token_text(b); !token.empty()) {
            out += token;
            continue;
        }

        if (b >= 32 && b <= 126) {
            out.push_back(static_cast<char>(b));
            continue;
        }

        out += std::format("{{0x{:02x}}}", b);
    }

    for (std::size_t i = 0; i + 1 < out.size(); ++i) {
        if (out[i] == ':' && out[i + 1] != ' ') {
            out.insert(i + 1, 1, ' ');
            ++i;
        }
    }

    for (std::size_t i = 1; i < out.size(); ++i) {
        if (out.compare(i, 4, "CODE") == 0 && out[i - 1] == '"') {
            out.insert(i, 1, ' ');
            ++i;
        }
    }

    return out;
}

std::optional<uint16_t> extract_keyword_number(std::string_view text, std::string_view keyword) {
    const std::size_t pos = text.find(keyword);
    if (pos == std::string_view::npos) {
        return std::nullopt;
    }

    std::size_t p = pos + keyword.size();
    while (p < text.size() && text[p] == ' ') {
        ++p;
    }

    std::size_t end = p;
    while (end < text.size() && std::isdigit(static_cast<unsigned char>(text[end]))) {
        ++end;
    }

    if (end == p) {
        return std::nullopt;
    }

    return parse_u16(text.substr(p, end - p));
}

basic_program parse_basic_program(const std::vector<uint8_t>& data) {
    basic_program program;
    std::size_t pos = 0;

    while (pos + 4 <= data.size()) {
        const uint16_t line_number = static_cast<uint16_t>((data[pos] << 8) | data[pos + 1]);
        const uint16_t line_len = rd16(data.data() + pos + 2);
        pos += 4;

        if (line_len == 0) {
            break;
        }

        if (pos + line_len > data.size()) {
            throw std::runtime_error("malformed BASIC program: truncated line");
        }

        std::span<const uint8_t> line_bytes(data.data() + pos, line_len);
        basic_line line;
        line.number = line_number;
        line.bytes.assign(line_bytes.begin(), line_bytes.end());
        line.text = decode_basic_text(line_bytes);
        program.lines.push_back(std::move(line));
        pos += line_len;
    }

    for (const auto& line : program.lines) {
        if (!program.clear_addr) {
            program.clear_addr = extract_keyword_number(line.text, "CLEAR");
        }

        if (const auto usr = extract_keyword_number(line.text, "USR"); usr) {
            program.usr_addr = usr;
        }
    }

    return program;
}

basic_program parse_basic_from_files(const std::vector<tap_file>& files) {
    for (const auto& file : files) {
        if (file.header.type == 0x00) {
            return parse_basic_program(file.data);
        }
    }

    throw std::runtime_error("no BASIC program found on tape");
}

basic_program parse_basic_from_tape(const fs::path& path) {
    return parse_basic_from_files(parse_tape_files(path));
}

basic_program parse_basic_from_tap(const fs::path& path) {
    return parse_basic_from_files(parse_tap_files(path));
}

void print_basic_listing(const basic_program& program) {
    for (const auto& line : program.lines) {
        std::cout << std::format("{:>5} {}\n", line.number, line.text);
    }

    if (program.clear_addr || program.usr_addr) {
        std::cout << "\n";
    }
    if (program.clear_addr) {
        std::cout << std::format("clear: {}\n", *program.clear_addr);
    }
    if (program.usr_addr) {
        std::cout << std::format("usr:   {}\n", *program.usr_addr);
    }
}

}  // namespace appmake
