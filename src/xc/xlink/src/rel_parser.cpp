// rel_parser.cpp
//
// SDCC .rel file parser
//
// MIT License (see: LICENSE)
// copyright (C) 2021 tomaz stih
//
// 2021-07-28   tstih
#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>

#include <xlink/rel_parser.hpp>
#include <xlink/errors.hpp>

namespace xlink {

    enum class rel_format {
        xl1,
        xl2,
        xl4
    };

    // Helper: parse hex string to uint16_t.
    static uint16_t parse_hex16(const std::string& s) {
        return static_cast<uint16_t>(std::stoul(s, nullptr, 16));
    }

    // Helper: parse hex string to uint8_t.
    static uint8_t parse_hex8(const std::string& s) {
        return static_cast<uint8_t>(std::stoul(s, nullptr, 16));
    }

    // Helper: split a string by whitespace.
    static std::vector<std::string> split(const std::string& line) {
        std::vector<std::string> tokens;
        std::istringstream iss(line);
        std::string tok;
        while (iss >> tok) tokens.push_back(tok);
        return tokens;
    }

    // Helper: trim whitespace from both ends.
    static std::string trim(const std::string& s) {
        auto start = s.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) return "";
        auto end = s.find_last_not_of(" \t\r\n");
        return s.substr(start, end - start + 1);
    }

    std::shared_ptr<module> rel_parser::parse(
        const std::filesystem::path& path)
    {
        std::ifstream file(path);
        if (!file.is_open())
            throw parse_error("cannot open file: " + path.string());

        auto mod = std::make_shared<module>("", path);
        std::string line;
        int line_num = 0;
        text_record* current_text = nullptr;
        rel_format format = rel_format::xl1;
        int current_area_idx = -1;

        while (std::getline(file, line)) {
            line_num++;
            line = trim(line);
            if (line.empty()) continue;

            char record_type = line[0];
            std::string rest = (line.size() > 2) ? line.substr(2) : "";

            switch (record_type) {
            case 'X': {
                // XL / XL4: little-endian (version 4 = extended 32-bit format)
                // XH: big-endian
                if (line.size() >= 2) {
                    if (line[1] == 'L') {
                        mod->set_endian(byte_order::little_endian);
                        if (line.size() >= 3 && line[2] == '4') {
                            format = rel_format::xl4;
                        } else if (line.size() >= 3 && line[2] == '2') {
                            format = rel_format::xl2;
                        } else {
                            format = rel_format::xl1;
                        }
                    } else if (line[1] == 'H') {
                        mod->set_endian(byte_order::big_endian);
                    }
                }
                break;
            }
            case 'H': {
                // H <size> areas <size> global symbols
                // We just note this; actual counts come from A/S records.
                break;
            }
            case 'M': {
                // M <module_name>
                auto tokens = split(rest);
                if (!tokens.empty())
                    mod->set_name(tokens[0]);
                break;
            }
            case 'O': {
                // O -mz80 ... (options, ignored)
                break;
            }
            case 'A': {
                // A <name> size <hex> flags <hex> [addr <hex>]
                auto tokens = split(rest);
                if (tokens.size() < 5)
                    throw parse_error(path.string(), line_num,
                        "malformed A record");

                std::string name = tokens[0];
                // tokens[1] = "size", tokens[2] = hex size
                // tokens[3] = "flags", tokens[4] = hex flags
                uint16_t size = parse_hex16(tokens[2]);
                auto flags = static_cast<area_flags>(parse_hex8(tokens[4]));

                std::optional<uint16_t> org_addr;
                // If ABS flag set and there's an addr field.
                if (tokens.size() >= 7 &&
                    tokens[5] == "addr") {
                    org_addr = parse_hex16(tokens[6]);
                }

                int idx = static_cast<int>(mod->areas().size());
                mod->areas().emplace_back(name, size, flags, idx, org_addr);
                current_area_idx = idx;
                break;
            }
            case 'S': {
                // S <name> Def<hex> or S <name> Ref<hex>
                auto tokens = split(rest);
                if (tokens.size() < 2)
                    throw parse_error(path.string(), line_num,
                        "malformed S record");

                std::string name = tokens[0];
                std::string type_val = tokens[1];

                symbol_type stype;
                uint16_t value = 0;

                if (type_val.substr(0, 3) == "Def") {
                    stype = symbol_type::def;
                    value = parse_hex16(type_val.substr(3));
                } else if (type_val.substr(0, 3) == "Ref") {
                    stype = symbol_type::ref;
                    value = parse_hex16(type_val.substr(3));
                } else {
                    throw parse_error(path.string(), line_num,
                        "unknown symbol type in S record");
                }

                int idx = static_cast<int>(mod->symbols().size());
                int sym_area_idx = (stype == symbol_type::def)
                    ? current_area_idx
                    : -1;
                symbol sym(name, stype, value, idx, sym_area_idx);
                sym.set_owner(mod.get());
                mod->symbols().push_back(sym);
                break;
            }
            case 'T': {
                // T record: code/data bytes for an area at a given offset.
                //
                // v1 format: T <off_lo> <off_hi> <byte> ...  (2-byte offset)
                // v4 format: T <b0> <b1> <b2> <b3> <byte> ... (4-byte offset,
                //            only low 16 bits used for Z80)
                auto tokens = split(rest);
                size_t off_bytes = (format == rel_format::xl4) ? 4 : 2;
                if (tokens.size() < off_bytes)
                    throw parse_error(path.string(), line_num,
                        "malformed T record");

                text_record tr;
                uint8_t lo = parse_hex8(tokens[0]);
                uint8_t hi = parse_hex8(tokens[1]);
                tr.offset = static_cast<uint16_t>(lo | (hi << 8));
                // high bytes (v4 tokens[2][3]) are zero for any Z80 code

                for (size_t i = off_bytes; i < tokens.size(); ++i)
                    tr.data.push_back(parse_hex8(tokens[i]));

                tr.area_index = -1; // patched by following R record
                mod->texts().push_back(tr);
                current_text = &mod->texts().back();
                break;
            }
            case 'R': {
                // R 00 00 <area_index_lo> <area_index_hi>
                //   [<mode> <offset_lo> <offset_hi> <ref_lo> <ref_hi>] ...
                auto tokens = split(rest);
                if (tokens.size() < 4)
                    throw parse_error(path.string(), line_num,
                        "malformed R record");

                // First 4 bytes: 00 00 area_lo area_hi
                uint8_t area_lo = parse_hex8(tokens[2]);
                uint8_t area_hi = parse_hex8(tokens[3]);
                int area_idx = area_lo | (area_hi << 8);

                // Patch the most recent T record with its area index.
                if (current_text) {
                    current_text->area_index = area_idx;
                }

                // Parse relocation entries (4 bytes per entry after header).
                //
                // xl1 entry: [mode(1)][off_lo(1)][off_hi(1)][ref(1)]
                //   offset is relative to T data start; ref is 1 byte.
                //   mode bit 0 = word (1) / byte (0).
                //
                // xl2 entry: [mode(1)][offset_in_t(1)][ref_lo(1)][ref_hi(1)]
                //   offset is a 1-byte index into T data; ref is 2 bytes.
                //
                // v4 entry: [mode(1)][offset_from_T_start(1)]
                //           [ref_lo(1)][ref_hi(1)]
                //   offset includes the 4-byte T record prefix → subtract 4 for
                //   data-relative offset. ref is 2 bytes. mode bit 0 inverted:
                //   0 = word, 1 = byte (normalize by XOR 0x01 to match v1).
                size_t i = 4;
                while (i + 3 < tokens.size()) {
                    reloc_entry re;
                    uint8_t mode_byte = parse_hex8(tokens[i]);

                    if (format == rel_format::xl4) {
                        uint8_t from_start = parse_hex8(tokens[i + 1]);
                        uint8_t ref_lo    = parse_hex8(tokens[i + 2]);
                        uint8_t ref_hi    = parse_hex8(tokens[i + 3]);
                        re.mode       = static_cast<reloc_mode>(mode_byte ^ 0x01);
                        re.offset_in_t = static_cast<uint16_t>(
                            from_start >= 4 ? from_start - 4 : 0);
                        re.ref_index  = ref_lo | (ref_hi << 8);
                    } else if (format == rel_format::xl2) {
                        uint8_t offset = parse_hex8(tokens[i + 1]);
                        uint8_t ref_lo = parse_hex8(tokens[i + 2]);
                        uint8_t ref_hi = parse_hex8(tokens[i + 3]);
                        re.mode        = static_cast<reloc_mode>(mode_byte);
                        re.offset_in_t = offset;
                        re.ref_index   = ref_lo | (ref_hi << 8);
                    } else {
                        uint8_t off_lo = parse_hex8(tokens[i + 1]);
                        uint8_t off_hi = parse_hex8(tokens[i + 2]);
                        uint8_t ref    = parse_hex8(tokens[i + 3]);
                        re.mode        = static_cast<reloc_mode>(mode_byte);
                        re.offset_in_t = static_cast<uint16_t>(
                            off_lo | (off_hi << 8));
                        re.ref_index   = ref;
                    }

                    if (current_text)
                        current_text->relocs.push_back(re);
                    i += 4;
                }
                break;
            }
            default:
                // Unknown record type, skip.
                break;
            }
        }

        // Fix up any T records that didn't get an area_index from R.
        // Default to area 0 if not set.
        for (auto& tr : mod->texts()) {
            if (tr.area_index < 0)
                tr.area_index = 0;
        }

        return mod;
    }

    std::vector<std::string> rel_parser::scan_defs(
        const std::filesystem::path& path)
    {
        std::vector<std::string> defs;
        std::ifstream file(path);
        if (!file.is_open()) return defs;

        std::string line;
        while (std::getline(file, line)) {
            line = trim(line);
            if (line.empty() || line[0] != 'S') continue;

            auto tokens = split(line.substr(2));
            if (tokens.size() >= 2 &&
                tokens[1].substr(0, 3) == "Def") {
                defs.push_back(tokens[0]);
            }
        }
        return defs;
    }

} // namespace xlink
