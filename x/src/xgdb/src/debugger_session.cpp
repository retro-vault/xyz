#include "debugger_session.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace {

    // -----------------------------------------------------------------------
    // Z80 register pack / unpack (RSP g/G packet layout, 18 bytes)
    //   bytes 0-1:  AF (LE)   bytes 2-3:  BC (LE)
    //   bytes 4-5:  DE (LE)   bytes 6-7:  HL (LE)
    //   bytes 8-9:  IX (LE)   bytes 10-11: IY (LE)
    //   bytes 12-13: SP (LE)  bytes 14-15: PC (LE)
    //   byte 16: I            byte 17: R
    // -----------------------------------------------------------------------

    static constexpr std::size_t Z80_REG_BYTES = 18;

    xgdb::cpu_state unpack_regs(const std::vector<uint8_t>& d) {
        xgdb::cpu_state s;
        if (d.size() < Z80_REG_BYTES) return s;
        auto u16 = [&](std::size_t i) -> uint16_t {
            return static_cast<uint16_t>(d[i]) |
                   static_cast<uint16_t>(static_cast<uint16_t>(d[i + 1]) << 8);
        };
        s.af = u16(0);  s.bc = u16(2);  s.de = u16(4);  s.hl = u16(6);
        s.ix = u16(8);  s.iy = u16(10); s.sp = u16(12); s.pc = u16(14);
        s.i  = d[16];   s.r  = d[17];
        return s;
    }

    std::vector<uint8_t> pack_regs(const xgdb::cpu_state& s) {
        std::vector<uint8_t> d(Z80_REG_BYTES, 0);
        auto put16 = [&](std::size_t i, uint16_t v) {
            d[i]     = static_cast<uint8_t>(v & 0xFF);
            d[i + 1] = static_cast<uint8_t>((v >> 8) & 0xFF);
        };
        put16(0, s.af);  put16(2, s.bc);  put16(4, s.de);  put16(6, s.hl);
        put16(8, s.ix);  put16(10, s.iy); put16(12, s.sp); put16(14, s.pc);
        d[16] = s.i;     d[17] = s.r;
        return d;
    }

    // Build a target_status from an RSP stop reply and current CPU state.
    xgdb::target_status make_target_status(
        const rsp::stop_reply& reply,
        const xgdb::cpu_state& cpu)
    {
        xgdb::target_status ts;
        ts.pc = cpu.pc;
        ts.registers = cpu;
        switch (reply.type) {
        case rsp::stop_reply::kind::signal:
            ts.state = xgdb::execution_state::stopped;
            if (reply.signal_number == 5)
                ts.reason = xgdb::stop_reason::breakpoint;
            else if (reply.signal_number == 2)
                ts.reason = xgdb::stop_reason::pause;
            else
                ts.reason = xgdb::stop_reason::signal;
            break;
        case rsp::stop_reply::kind::exited:
            ts.state = xgdb::execution_state::terminated;
            ts.reason = xgdb::stop_reason::exited;
            ts.exit_code = static_cast<uint32_t>(reply.exit_code);
            break;
        case rsp::stop_reply::kind::terminated:
            ts.state = xgdb::execution_state::terminated;
            ts.reason = xgdb::stop_reason::halted;
            break;
        }
        return ts;
    }

    uint32_t parse_u32(const std::string& value) {
        char* end = nullptr;
        const unsigned long parsed = std::strtoul(value.c_str(), &end, 0);
        if (end == value.c_str() || *end != '\0')
            throw std::runtime_error("invalid number: " + value);
        return static_cast<uint32_t>(parsed);
    }

    std::string trim(const std::string& value) {
        const auto start = value.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) return "";
        const auto end = value.find_last_not_of(" \t\r\n");
        return value.substr(start, end - start + 1);
    }

    std::string trim_trailing_separators(const std::string& value) {
        std::size_t end = value.size();
        while (end > 0) {
            const char ch = value[end - 1];
            if (ch == ',' || std::isspace(static_cast<unsigned char>(ch))) {
                --end;
                continue;
            }
            break;
        }
        return value.substr(0, end);
    }

    std::pair<std::string, uint16_t> split_host_port(const std::string& value) {
        const auto colon = value.rfind(':');
        if (colon == std::string::npos)
            throw std::runtime_error("expected host:port");
        return {
            value.substr(0, colon),
            static_cast<uint16_t>(parse_u32(value.substr(colon + 1)))
        };
    }

    xgdb::language_kind language_from_path(const std::string& path) {
        const auto dot = path.rfind('.');
        if (dot == std::string::npos) return xgdb::language_kind::unknown;
        const auto ext = path.substr(dot + 1);
        if (ext == "c" || ext == "C") return xgdb::language_kind::c;
        if (ext == "s" || ext == "S" || ext == "asm") return xgdb::language_kind::assembly;
        return xgdb::language_kind::unknown;
    }

    xgdb::language_kind to_lang(xbfd::debug_lang l) {
        switch (l) {
        case xbfd::debug_lang::c:        return xgdb::language_kind::c;
        case xbfd::debug_lang::assembly: return xgdb::language_kind::assembly;
        default:                         return xgdb::language_kind::unknown;
        }
    }

    bool debug_info_has_symbol(const xbfd::debug_info& info,
                               const std::string& name)
    {
        for (const auto& fn : info.functions)
            if (fn.name == name)
                return true;
        for (const auto& sym : info.symbols)
            if (sym.name == name)
                return true;
        return false;
    }

    xbfd::debug_info debug_info_from_elf_object(const xbfd::object& obj) {
        xbfd::debug_info info = obj.debug;

        for (const auto& sym : obj.symbols) {
            if (!sym.is_defined())
                continue;
            if (sym.name.empty())
                continue;
            if (debug_info_has_symbol(info, sym.name))
                continue;

            if (xbfd::has_flag(sym.flags, xbfd::symbol_flags::function)) {
                xbfd::debug_function fn;
                fn.name = sym.name;
                fn.start = static_cast<uint32_t>(sym.value);
                fn.end = static_cast<uint32_t>(
                    sym.value + std::max<uint64_t>(sym.size, 1));
                info.functions.push_back(std::move(fn));
            } else {
                info.symbols.push_back({
                    sym.name,
                    static_cast<uint32_t>(sym.value)
                });
            }
        }

        return info;
    }

    std::vector<uint8_t> read_file_bytes(const std::filesystem::path& path) {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open())
            throw std::runtime_error("cannot open binary: " + path.string());
        return std::vector<uint8_t>(
            std::istreambuf_iterator<char>(file),
            std::istreambuf_iterator<char>());
    }

    uint16_t u16le(const std::vector<uint8_t>& data, std::size_t offset) {
        if (offset + 1 >= data.size())
            throw std::runtime_error("truncated 16-bit field");
        return static_cast<uint16_t>(data[offset])
             | static_cast<uint16_t>(
                   static_cast<uint16_t>(data[offset + 1]) << 8);
    }

    void put_u16le(std::vector<uint8_t>& data, std::size_t offset, uint16_t value) {
        if (offset + 1 >= data.size())
            throw std::runtime_error("relocation offset out of range");
        data[offset] = static_cast<uint8_t>(value & 0xff);
        data[offset + 1] = static_cast<uint8_t>((value >> 8) & 0xff);
    }

    struct xl_image {
        uint16_t entry = 0;
        std::vector<uint8_t> code;
        struct reloc {
            uint16_t offset = 0;
            uint8_t size = 0;
            uint8_t pad = 0;
        };
        std::vector<reloc> relocs;
    };

    std::optional<xl_image> try_read_xl_image(const std::filesystem::path& path) {
        const auto bytes = read_file_bytes(path);
        if (bytes.size() < 2)
            return std::nullopt;
        if (bytes[0] != 'X' || bytes[1] != 'L')
            return std::nullopt;
        if (bytes.size() < 12)
            throw std::runtime_error("truncated XL image: " + path.string());
        if (bytes[2] != 0x01)
            throw std::runtime_error("unsupported XL image version: "
                                     + path.string());

        xl_image image;
        image.entry = u16le(bytes, 4);
        const uint16_t code_size = u16le(bytes, 6);
        const uint16_t reloc_count = u16le(bytes, 8);
        const std::size_t table_offset = 12;
        const std::size_t code_offset =
            table_offset + static_cast<std::size_t>(reloc_count) * 4u;
        if (code_offset > bytes.size()
            || bytes.size() - code_offset < static_cast<std::size_t>(code_size)) {
            throw std::runtime_error("truncated XL image: " + path.string());
        }

        image.relocs.reserve(reloc_count);
        for (uint16_t i = 0; i < reloc_count; ++i) {
            const std::size_t off = table_offset + static_cast<std::size_t>(i) * 4u;
            image.relocs.push_back({
                u16le(bytes, off),
                bytes[off + 2],
                bytes[off + 3]
            });
        }
        image.code.assign(bytes.begin() + static_cast<std::ptrdiff_t>(code_offset),
                          bytes.begin() + static_cast<std::ptrdiff_t>(
                              code_offset + code_size));
        return image;
    }

    void relocate_xl_image(xl_image& image, uint16_t base) {
        for (const auto& reloc : image.relocs) {
            if (reloc.offset >= image.code.size())
                throw std::runtime_error("XL relocation offset out of range");
            if (reloc.size == 2) {
                const uint16_t old = u16le(image.code, reloc.offset);
                put_u16le(image.code, reloc.offset,
                          static_cast<uint16_t>(old + base));
            } else if (reloc.size == 1) {
                const uint32_t byte_offset =
                    static_cast<uint32_t>(reloc.offset)
                    + ((reloc.pad & 0x01u) ? 1u : 0u);
                if (byte_offset >= image.code.size())
                    throw std::runtime_error("XL byte relocation offset out of range");
                const uint16_t old = static_cast<uint16_t>(
                    image.code[byte_offset] << ((reloc.pad & 0x01u) ? 8 : 0));
                const uint16_t patched = static_cast<uint16_t>(old + base);
                image.code[byte_offset] = static_cast<uint8_t>(
                    (reloc.pad & 0x01u) ? (patched >> 8) : patched);
            } else {
                throw std::runtime_error("unsupported XL relocation size");
            }
        }
    }

    bool file_looks_like_xl(const std::filesystem::path& path) {
        if (path.extension() == ".xl" || path.extension() == ".XL")
            return true;

        std::ifstream file(path, std::ios::binary);
        if (!file.is_open())
            return false;
        char magic[3] = {};
        file.read(magic, sizeof(magic));
        return file.gcount() == static_cast<std::streamsize>(sizeof(magic))
            && magic[0] == 'X'
            && magic[1] == 'L'
            && magic[2] == 0x01;
    }

    uint32_t biased_address(uint32_t address,
                            uint32_t bias,
                            bool allow_exclusive_end = false) {
        const uint32_t max = allow_exclusive_end ? 0x10000u : 0xFFFFu;
        if (bias == 0)
            return address;
        if (address > max || bias > max || address + bias > max)
            throw std::runtime_error("relocated debug address is outside Z80 range");
        return address + bias;
    }

    void apply_document_address_bias(xgdb::document& doc, uint32_t bias) {
        if (bias == 0)
            return;
        if (doc.entry_address.has_value())
            doc.entry_address = biased_address(*doc.entry_address, bias);
        for (auto& function : doc.functions) {
            function.start_address = biased_address(function.start_address, bias);
            function.end_address = biased_address(function.end_address, bias, true);
        }
        for (auto& line : doc.lines)
            line.address = biased_address(line.address, bias);
        for (auto& symbol : doc.symbols)
            symbol.address = biased_address(symbol.address, bias);
        for (auto& variable : doc.variables) {
            if (variable.address.has_value())
                variable.address = biased_address(*variable.address, bias);
            if (variable.start_address.has_value())
                variable.start_address = biased_address(*variable.start_address, bias);
            if (variable.end_address.has_value())
                variable.end_address = biased_address(*variable.end_address, bias, true);
        }
    }

    struct ihx_chunk {
        uint16_t address = 0;
        std::vector<uint8_t> bytes;
    };

    int hex_nibble(char ch) {
        if (ch >= '0' && ch <= '9') return ch - '0';
        if (ch >= 'a' && ch <= 'f') return 10 + (ch - 'a');
        if (ch >= 'A' && ch <= 'F') return 10 + (ch - 'A');
        throw std::runtime_error("bad Intel HEX digit");
    }

    uint8_t parse_hex_byte(const std::string& line, std::size_t offset) {
        return static_cast<uint8_t>((hex_nibble(line.at(offset)) << 4)
                                    | hex_nibble(line.at(offset + 1)));
    }

    std::vector<ihx_chunk> read_ihx_chunks(const std::filesystem::path& path) {
        std::ifstream input(path);
        if (!input.is_open())
            throw std::runtime_error("cannot open Intel HEX: " + path.string());

        std::vector<ihx_chunk> chunks;
        std::string line;
        uint32_t ext_base = 0;

        while (std::getline(input, line)) {
            if (line.empty()) continue;
            if (line[0] != ':')
                throw std::runtime_error("invalid Intel HEX record in " + path.string());
            if (line.size() < 11)
                throw std::runtime_error("truncated Intel HEX record in " + path.string());

            const uint8_t len = parse_hex_byte(line, 1);
            const std::size_t expected_size =
                11u + static_cast<std::size_t>(len) * 2u;
            if (line.size() < expected_size)
                throw std::runtime_error("short Intel HEX record in " + path.string());

            const uint16_t addr = static_cast<uint16_t>(
                (parse_hex_byte(line, 3) << 8) | parse_hex_byte(line, 5));
            const uint8_t type = parse_hex_byte(line, 7);
            uint8_t checksum = static_cast<uint8_t>(
                len + static_cast<uint8_t>((addr >> 8) & 0xff)
                + static_cast<uint8_t>(addr & 0xff) + type);

            std::vector<uint8_t> bytes;
            bytes.reserve(len);
            for (uint8_t i = 0; i < len; ++i) {
                const uint8_t byte = parse_hex_byte(line, 9 + i * 2);
                checksum = static_cast<uint8_t>(checksum + byte);
                bytes.push_back(byte);
            }
            checksum = static_cast<uint8_t>(
                checksum + parse_hex_byte(line, 9 + len * 2));
            if (checksum != 0)
                throw std::runtime_error("bad Intel HEX checksum in " + path.string());

            if (type == 0x00) {
                const uint32_t full = ext_base + addr;
                if (full + bytes.size() > 0x10000u)
                    throw std::runtime_error(
                        "Intel HEX image exceeds 64K: " + path.string());
                if (!chunks.empty()
                    && static_cast<uint32_t>(
                        chunks.back().address + chunks.back().bytes.size()) == full) {
                    chunks.back().bytes.insert(
                        chunks.back().bytes.end(), bytes.begin(), bytes.end());
                } else {
                    chunks.push_back({static_cast<uint16_t>(full), std::move(bytes)});
                }
            } else if (type == 0x01) {
                break;
            } else if (type == 0x04) {
                if (bytes.size() != 2)
                    throw std::runtime_error(
                        "bad Intel HEX extended address in " + path.string());
                ext_base = static_cast<uint32_t>(
                    (static_cast<uint32_t>(bytes[0]) << 24)
                    | (static_cast<uint32_t>(bytes[1]) << 16));
            }
        }

        return chunks;
    }

    xgdb::variable make_variable(const xbfd::debug_variable& v,
                                  uint32_t scope_start = 0,
                                  uint32_t scope_end   = 0)
    {
        xgdb::variable out;
        out.name = v.name;
        out.kind = v.is_param ? xgdb::symbol_kind::parameter
                 : v.parent.empty() ? xgdb::symbol_kind::global
                 : xgdb::symbol_kind::local;
        if (!v.parent.empty()) out.parent_name = v.parent;
        if (!v.type_name.empty()) out.type_name = v.type_name;

        switch (v.storage) {
        case xbfd::var_storage::stack:
            out.storage = xgdb::storage_kind::frame_relative;
            out.offset  = v.offset;
            break;
        case xbfd::var_storage::reg:
            out.storage = (v.reg.find(',') != std::string::npos)
                        ? xgdb::storage_kind::register_pair
                        : xgdb::storage_kind::register_name;
            out.register_name = v.reg;
            break;
        case xbfd::var_storage::external:
            out.storage = xgdb::storage_kind::address;
            out.address = static_cast<uint32_t>(v.offset);
            break;
        default:
            out.storage = xgdb::storage_kind::unknown;
            break;
        }

        if (scope_start || scope_end) {
            out.start_address = scope_start;
            out.end_address   = scope_end;
        }
        return out;
    }

} // namespace

// ---------------------------------------------------------------------------
// xbfd debug/symbol metadata → xgdb::document translator
// ---------------------------------------------------------------------------

void debugger_session::rebuild_document() {
    if (!cdb_info_.has_value()) { document_ = std::nullopt; return; }

    // Merge MAP symbols on top of the CDB result if a map file is available.
    xbfd::debug_info info = *cdb_info_;
    if (map_path_) {
        if (auto merged = xbfd::debug_reader::read_map(map_path_->string(), info))
            info = std::move(*merged);
    }

    xgdb::document doc;
    doc.version = 1;
    if (exec_path_.has_value())
        doc.image_path = exec_path_->string();

    // Source files.
    for (const auto& f : info.files) {
        xgdb::source_file sf;
        sf.id       = f.id;
        sf.path     = f.path;
        sf.language = to_lang(f.language);
        doc.files.push_back(sf);
    }

    // Build function name → address range map for scoping locals.
    std::map<std::string, std::pair<uint32_t,uint32_t>> fn_ranges;
    for (const auto& f : info.functions)
        fn_ranges[f.name] = {f.start, f.end};

    // Functions.
    for (const auto& f : info.functions) {
        xgdb::function fn;
        fn.name          = f.name;
        fn.start_address = f.start;
        fn.end_address   = f.end;
        fn.language      = xgdb::language_kind::c;
        if (f.file_id) fn.file_id = f.file_id;
        if (f.line)    fn.line    = f.line;
        doc.functions.push_back(fn);
    }

    // Line mappings.
    for (const auto& l : info.lines) {
        xgdb::line_entry e;
        e.address = l.address;
        e.file_id = l.file_id;
        e.line    = l.line;
        doc.lines.push_back(e);
    }

    // Symbols.
    for (const auto& s : info.symbols) {
        xgdb::symbol sym;
        sym.name    = s.name;
        sym.kind    = xgdb::symbol_kind::label;
        sym.address = s.address;
        doc.symbols.push_back(sym);
    }

    // Variables (locals, params, globals).
    for (const auto& v : info.variables) {
        uint32_t scope_start = 0, scope_end = 0;
        if (!v.parent.empty()) {
            auto it = fn_ranges.find(v.parent);
            if (it != fn_ranges.end()) {
                scope_start = it->second.first;
                scope_end   = it->second.second;
            }
        }
        doc.variables.push_back(make_variable(v, scope_start, scope_end));
    }

    apply_document_address_bias(doc, symbol_address_bias());
    document_ = std::move(doc);
}

uint32_t debugger_session::symbol_address_bias() const {
    if (!exec_path_.has_value())
        return 0;
    if (!file_looks_like_xl(exec_path_.value()))
        return 0;
    return download_origin_;
}

void debugger_session::refresh_breakpoint_addresses() {
    if (breakpoints_.empty())
        return;

    const bool connected = remote_.is_connected();
    if (connected) {
        for (const auto& bp : breakpoints_)
            remote_.remove_breakpoint(bp.address);
    }

    for (auto& bp : breakpoints_)
        bp.address = resolve_address_expression(bp.expression);

    if (connected) {
        for (const auto& bp : breakpoints_)
            remote_.insert_breakpoint(bp.address);
    }
}

// ---------------------------------------------------------------------------
// debugger_host implementation
// ---------------------------------------------------------------------------

void debugger_session::set_exec_path(const std::filesystem::path& path) {
    exec_path_ = path;
    rebuild_document();
    refresh_breakpoint_addresses();
}

const std::optional<std::filesystem::path>& debugger_session::exec_path() const {
    return exec_path_;
}

void debugger_session::load_cdb_file(const std::filesystem::path& path) {
    auto result = xbfd::debug_reader::read_cdb(path.string());
    if (!result.has_value())
        throw std::runtime_error("cannot parse CDB file: " + path.string());
    cdb_info_ = std::move(*result);
    cdb_path_ = path;
    symbol_path_ = path;
    rebuild_document();
    refresh_breakpoint_addresses();
}

void debugger_session::load_elf_file(const std::filesystem::path& path) {
    try {
        auto obj = bfd::bfd::open_r(path);
        if (obj->get_flavour() != bfd::flavour::elf
            || (!obj->check_format(bfd::format::object)
                && !obj->check_format(bfd::format::executable))) {
            throw std::runtime_error("not an ELF object/executable: "
                                     + path.string());
        }

        auto info = debug_info_from_elf_object(obj->object());
        if (info.empty())
            throw std::runtime_error("ELF file contains no debug symbols: "
                                     + path.string());

        cdb_info_ = std::move(info);
        cdb_path_ = std::nullopt;
        symbol_path_ = path;
        rebuild_document();
        refresh_breakpoint_addresses();
    } catch (const xbfd::bfd_error& e) {
        throw std::runtime_error("cannot parse ELF file: "
                                 + path.string() + ": " + e.what());
    }
}

void debugger_session::load_map_file(const std::filesystem::path& path) {
    map_path_ = path;
    rebuild_document();
    refresh_breakpoint_addresses();
}

void debugger_session::maybe_load_default_symbols() {
    if (!exec_path_.has_value()) return;

    if (std::filesystem::exists(exec_path_.value())) {
        try {
            auto obj = bfd::bfd::open_r(exec_path_.value());
            if (obj->get_flavour() == bfd::flavour::elf
                && (obj->check_format(bfd::format::object)
                    || obj->check_format(bfd::format::executable))) {
                load_elf_file(exec_path_.value());
                return;
            }
        } catch (const std::exception&) {
            // Non-ELF binaries continue through the legacy sidecar path.
        }
    }

    auto try_cdb = [&](std::filesystem::path p) {
        if (std::filesystem::exists(p)) { load_cdb_file(p); return true; }
        return false;
    };
    auto cdb = exec_path_.value(); cdb.replace_extension(".cdb");
    if (!try_cdb(cdb)) { auto a = exec_path_.value(); a += ".cdb"; try_cdb(a); }

    auto try_map = [&](std::filesystem::path p) {
        if (std::filesystem::exists(p)) { try { load_map_file(p); } catch (...) {} }
    };
    auto map = exec_path_.value(); map.replace_extension(".map");
    try_map(map);
    auto ma = exec_path_.value(); ma += ".map";
    try_map(ma);
}

const std::optional<std::filesystem::path>& debugger_session::cdb_path() const {
    return cdb_path_;
}

const std::optional<std::filesystem::path>& debugger_session::symbol_path() const {
    return symbol_path_;
}

const std::optional<std::filesystem::path>& debugger_session::map_path() const {
    return map_path_;
}

bool debugger_session::has_symbols() const { return document_.has_value(); }

const xgdb::document* debugger_session::symbols() const {
    return document_.has_value() ? &document_.value() : nullptr;
}

void debugger_session::connect_remote(const std::string& target) {
    const auto [host, port] = split_host_port(target);
    remote_.connect(host, port);
    if (download_enabled_ && exec_path_.has_value()) {
        download_program();
    } else {
        for (const auto& bp : breakpoints_)
            remote_.insert_breakpoint(bp.address);
    }
}

void debugger_session::set_download_enabled(bool enabled) {
    download_enabled_ = enabled;
}

void debugger_session::set_download_origin(uint32_t origin) {
    if (origin > 0xFFFFu)
        throw std::runtime_error("download origin is outside Z80 address range");
    if (origin == download_origin_)
        return;
    download_origin_ = origin;
    rebuild_document();
    refresh_breakpoint_addresses();
}

void debugger_session::set_download_pc(std::optional<uint32_t> pc) {
    if (pc.has_value() && pc.value() > 0xFFFFu)
        throw std::runtime_error("download PC is outside Z80 address range");
    download_pc_ = pc;
}

std::optional<uint32_t> debugger_session::download_program() {
    if (!remote_.is_connected())
        throw std::runtime_error("not connected to remote target");
    if (!exec_path_.has_value())
        throw std::runtime_error("no executable file selected");

    const auto& path = exec_path_.value();
    std::optional<uint32_t> entry;
    bool loaded = false;

    if (std::filesystem::exists(path)) {
        try {
            auto obj = bfd::bfd::open_r(path);
            if (obj->get_flavour() == bfd::flavour::elf
                && (obj->check_format(bfd::format::object)
                    || obj->check_format(bfd::format::executable))) {
                for (const auto& sec : obj->sections()) {
                    if (!xbfd::has_flag(sec.flags, xbfd::section_flags::alloc))
                        continue;
                    if (xbfd::has_flag(sec.flags, xbfd::section_flags::debugging))
                        continue;
                    if (sec.data.empty())
                        continue;
                    if (sec.vma > 0xFFFFu
                        || sec.vma + sec.data.size() > 0x10000u) {
                        throw std::runtime_error(
                            "ELF section out of Z80 address range: " + sec.name);
                    }
                    remote_.write_memory(
                        static_cast<uint32_t>(sec.vma), sec.data);
                }
                if (obj->object().entry > 0xFFFFu)
                    throw std::runtime_error("ELF entry is outside Z80 address range");
                entry = static_cast<uint32_t>(obj->object().entry);
                loaded = true;
            }
        } catch (const xbfd::bfd_error&) {
            // Non-ELF images continue through the extension-based loaders.
        }
    }

    if (!loaded) {
        const auto ext = path.extension().string();
        if (ext == ".ihx" || ext == ".hex") {
            for (const auto& chunk : read_ihx_chunks(path))
                remote_.write_memory(chunk.address, chunk.bytes);
            loaded = true;
        } else if (auto xl = try_read_xl_image(path); xl.has_value()) {
            if (download_origin_ + xl->code.size() > 0x10000u)
                throw std::runtime_error("XL image exceeds Z80 address range");
            relocate_xl_image(*xl, static_cast<uint16_t>(download_origin_));
            remote_.write_memory(download_origin_, xl->code);
            entry = download_origin_ + xl->entry;
            if (entry.value() > 0xFFFFu)
                throw std::runtime_error("XL entry is outside Z80 address range");
            loaded = true;
        } else {
            const auto bytes = read_file_bytes(path);
            if (download_origin_ + bytes.size() > 0x10000u)
                throw std::runtime_error("binary image exceeds Z80 address range");
            remote_.write_memory(download_origin_, bytes);
            entry = download_origin_;
            loaded = true;
        }
    }

    const uint32_t pc = download_pc_.value_or(entry.value_or(download_origin_));
    auto regs = read_registers();
    regs.pc = static_cast<uint16_t>(pc);
    write_registers(regs);
    for (const auto& bp : breakpoints_)
        remote_.insert_breakpoint(bp.address);
    return pc;
}

bool debugger_session::is_connected() const { return remote_.is_connected(); }

breakpoint_entry debugger_session::add_breakpoint(const std::string& expression) {
    const uint32_t address = resolve_address_expression(expression);
    if (remote_.is_connected())
        remote_.insert_breakpoint(address);
    breakpoints_.push_back({next_breakpoint_id_++, address, expression});
    return breakpoints_.back();
}

void debugger_session::delete_breakpoint(int id) {
    auto it = std::find_if(breakpoints_.begin(), breakpoints_.end(),
        [id](const breakpoint_entry& e) { return e.id == id; });
    if (it == breakpoints_.end())
        throw std::runtime_error("breakpoint id not found");
    if (remote_.is_connected())
        remote_.remove_breakpoint(it->address);
    breakpoints_.erase(it);
}

void debugger_session::delete_all_breakpoints() {
    if (remote_.is_connected())
        for (const auto& bp : breakpoints_) remote_.remove_breakpoint(bp.address);
    breakpoints_.clear();
}

const std::vector<breakpoint_entry>& debugger_session::breakpoints() const {
    return breakpoints_;
}

xgdb::cpu_state debugger_session::read_registers() {
    return unpack_regs(remote_.read_registers());
}

void debugger_session::write_registers(const xgdb::cpu_state& state) {
    remote_.write_registers(pack_regs(state));
}

xgdb::target_status debugger_session::raw_status() {
    const auto reply = remote_.query_stop();
    const auto cpu   = unpack_regs(remote_.read_registers());
    return make_target_status(reply, cpu);
}

stop_snapshot debugger_session::make_stop_snapshot(
    const rsp::stop_reply& reply, const xgdb::cpu_state& cpu)
{
    stop_snapshot snap;
    switch (reply.type) {
    case rsp::stop_reply::kind::signal:
        if (reply.signal_number == 5)
            snap.reason = xgdb::stop_reason::breakpoint;
        else if (reply.signal_number == 2)
            snap.reason = xgdb::stop_reason::pause;
        else
            snap.reason = xgdb::stop_reason::signal;
        break;
    case rsp::stop_reply::kind::exited:
        snap.reason = xgdb::stop_reason::exited;
        break;
    case rsp::stop_reply::kind::terminated:
        snap.reason = xgdb::stop_reason::halted;
        break;
    }
    snap.pc     = cpu.pc;
    if (const auto* function = find_function_for_pc(cpu.pc))
        snap.function_name = function->name;
    snap.source = source_location_for_address(cpu.pc);
    if (snap.source.has_value()) {
        if (!snap.source->function_name.has_value() && snap.function_name.has_value())
            snap.source->function_name = snap.function_name;
        snap.source_text = read_source_line(snap.source->file_path, snap.source->line);
    }
    return snap;
}

stop_snapshot debugger_session::status() {
    const auto reply = remote_.query_stop();
    const auto cpu   = unpack_regs(remote_.read_registers());
    return make_stop_snapshot(reply, cpu);
}

stop_snapshot debugger_session::continue_execution() {
    const auto reply = remote_.cont();
    const auto cpu   = unpack_regs(remote_.read_registers());
    return make_stop_snapshot(reply, cpu);
}

stop_snapshot debugger_session::step_instruction() {
    const auto reply = remote_.step();
    const auto cpu   = unpack_regs(remote_.read_registers());
    return make_stop_snapshot(reply, cpu);
}

stop_snapshot debugger_session::pause_execution() {
    remote_.pause();
    const auto reply = remote_.query_stop();
    const auto cpu   = unpack_regs(remote_.read_registers());
    rsp::stop_reply pause_reply = reply;
    if (pause_reply.type == rsp::stop_reply::kind::signal)
        pause_reply.signal_number = 2;  // present as SIGINT
    return make_stop_snapshot(pause_reply, cpu);
}

void debugger_session::detach() {
    remote_.detach();
    remote_.close();
}

uint32_t debugger_session::resolve_address_expression(const std::string& expression) {
    const std::string normalized = trim_trailing_separators(trim(expression));
    if (normalized.empty())
        throw std::runtime_error("missing address expression");
    if (normalized[0] == '*') return parse_u32(normalized.substr(1));
    if (std::isdigit(static_cast<unsigned char>(normalized[0])))
        return parse_u32(normalized);
    if (normalized.find(':') != std::string::npos)
        return info_line_argument(normalized).location.address;
    if (const auto* f = find_function_by_name(normalized)) {
        if (auto loc = source_location_for_function(f->name); loc.has_value())
            return loc->address;
        return f->start_address;
    }
    if (const auto* s = find_symbol_by_name(normalized))   return s->address;
    throw std::runtime_error("unknown symbol or address: " + normalized);
}

std::optional<source_location> debugger_session::current_source_location() {
    return source_location_for_address(read_registers().pc);
}

std::optional<source_location> debugger_session::source_location_for_address(
    uint32_t address) const
{
    const auto* function = find_function_for_pc(address);
    const auto* line     = function
        ? find_line_for_pc_in_function(*function, address)
        : find_line_for_pc(address);
    if (!line) return std::nullopt;
    const auto* file = find_file(line->file_id);
    if (!file) return std::nullopt;

    source_location loc;
    loc.address = address; loc.file_path = file->path;
    loc.line    = line->line; loc.column = line->column;
    if (function) loc.function_name = function->name;
    return loc;
}

std::optional<source_location> debugger_session::source_location_for_function(
    const std::string& function_name) const
{
    const auto* function = find_function_by_name(function_name);
    if (!function) return std::nullopt;
    const auto* line = find_line_for_function(*function);
    if (!line) return std::nullopt;
    const auto* file = find_file(line->file_id);
    if (!file) return std::nullopt;

    source_location loc;
    loc.address = line->address; loc.file_path = file->path;
    loc.line    = line->line; loc.column = line->column;
    loc.function_name = function->name;
    return loc;
}

line_info_result debugger_session::info_line_current() {
    return info_line_argument("*" + std::to_string(read_registers().pc));
}

line_info_result debugger_session::info_line_argument(const std::string& argument) {
    const std::string normalized = trim_trailing_separators(trim(argument));
    const xgdb::line_entry*  line     = nullptr;
    const xgdb::function*    function = nullptr;
    const xgdb::source_file* file     = nullptr;
    uint32_t address = 0;

    if (!normalized.empty() && normalized[0] == '*') {
        address  = resolve_address_expression(normalized);
        function = find_function_for_pc(address);
        line     = function ? find_line_for_pc_in_function(*function, address)
                            : find_line_for_pc(address);
    } else if (const auto* f = find_function_by_name(normalized)) {
        function = f;
        line     = find_line_for_function(*f);
        address  = f->start_address;
    } else {
        const auto colon = normalized.rfind(':');
        if (colon == std::string::npos)
            throw std::runtime_error("cannot resolve info line argument");
        if (!document_.has_value()) throw std::runtime_error("no symbols loaded");

        const std::string fname = normalized.substr(0, colon);
        const auto line_number  = parse_u32(normalized.substr(colon + 1));
        for (const auto& c : document_->files)
            if (c.path == fname || std::filesystem::path(c.path).filename() == fname)
                { file = &c; break; }
        if (!file) throw std::runtime_error("source file not found");

        line = find_line_by_file_and_line(file->id, line_number);
        if (line) { address = line->address; function = find_function_for_pc(address); }
    }

    if (!line) throw std::runtime_error("no source line found");
    if (!file) file = find_file(line->file_id);
    if (!file) throw std::runtime_error("source file id not found");

    line_info_result result;
    result.location.address       = address == 0 ? line->address : address;
    result.location.file_path     = file->path;
    result.location.line          = line->line;
    result.location.column        = line->column;
    if (function) result.location.function_name = function->name;
    result.end_address = find_line_end_address(*line);
    return result;
}

std::vector<source_line_view> debugger_session::list_source(
    const std::optional<std::string>& argument,
    uint32_t context_before,
    uint32_t context_after)
{
    if (!document_.has_value()) throw std::runtime_error("no symbols loaded");

    std::filesystem::path path;
    uint32_t line_number = 0;
    uint32_t start_line = 0;
    uint32_t end_line = 0;

    const auto current_location = is_connected()
        ? current_source_location()
        : std::optional<source_location>{};

    const auto choose_focus_line =
        [&](const std::filesystem::path& candidate_path,
            uint32_t default_line,
            uint32_t range_start,
            uint32_t range_end) -> uint32_t
    {
        if (!current_location.has_value()) return default_line;

        const auto current_path = resolve_source_path(current_location->file_path);
        const auto resolved_candidate = resolve_source_path(candidate_path);
        if (current_path == resolved_candidate &&
            current_location->line >= range_start &&
            current_location->line <= range_end) {
            return current_location->line;
        }
        return default_line;
    };

    const auto set_context_range = [&]() {
        start_line = line_number > context_before ? line_number - context_before : 1;
        end_line = line_number + context_after;
    };

    if (!argument.has_value()) {
        const auto cpu  = read_registers();
        const auto* fn  = find_function_for_pc(cpu.pc);
        const auto* ln  = fn ? find_line_for_pc_in_function(*fn, cpu.pc)
                             : find_line_for_pc(cpu.pc);
        if (!ln) throw std::runtime_error("no source line for current pc");
        const auto* f = find_file(ln->file_id);
        if (!f) throw std::runtime_error("source file id not found");
        path = f->path;
        line_number = ln->line;
        set_context_range();
    } else if (const auto* fn = find_function_by_name(argument.value())) {
        if (!fn->file_id.has_value() || !fn->line.has_value())
            throw std::runtime_error("function has no source location");
        const auto* f = find_file(fn->file_id.value());
        if (!f) throw std::runtime_error("source file id not found");
        path = f->path;
        line_number = fn->line.value();
        set_context_range();
    } else {
        const std::string spec = trim(argument.value());
        std::string line_spec = spec;

        const auto colon = spec.rfind(':');
        if (colon != std::string::npos) {
            path = resolve_source_path(trim(spec.substr(0, colon)));
            line_spec = trim(spec.substr(colon + 1));
        } else if (std::isdigit(static_cast<unsigned char>(spec[0]))) {
            if (!current_location.has_value())
                throw std::runtime_error("cannot resolve list argument");
            path = resolve_source_path(current_location->file_path);
        } else {
            throw std::runtime_error("cannot resolve list argument");
        }

        const auto comma = line_spec.find(',');
        if (comma == std::string::npos) {
            line_number = parse_u32(trim(line_spec));
            set_context_range();
        } else {
            start_line = parse_u32(trim(line_spec.substr(0, comma)));
            end_line = parse_u32(trim(line_spec.substr(comma + 1)));
            if (end_line < start_line) std::swap(start_line, end_line);
            line_number = choose_focus_line(path, start_line, start_line, end_line);
        }
    }

    return source_window_range(path, line_number, start_line, end_line);
}

std::vector<disassembly_line> debugger_session::disassemble(
    uint32_t address, std::size_t count)
{
    // Prefetch the whole region in one RSP packet instead of one byte per
    // round-trip.  Z80 instructions are at most 4 bytes; add a small margin.
    const std::size_t fetch_bytes = count * 4 + 16;
    const auto prefetch = remote_.read_memory(address,
                              std::min(fetch_bytes, std::size_t{65536}));

    class buffered_reader final : public xgdb::memory_reader {
    public:
        buffered_reader(uint32_t base, const std::vector<uint8_t>& buf,
                        rsp::client& r)
            : base_(base), buf_(buf), r_(r) {}

        uint8_t read8(uint32_t a) const override {
            if (a >= base_ && a - base_ < buf_.size())
                return buf_[a - base_];
            // Fall back to live read for anything outside the prefetch window.
            auto b = r_.read_memory(a, 1);
            return b.empty() ? 0 : b[0];
        }
    private:
        uint32_t base_;
        const std::vector<uint8_t>& buf_;
        rsp::client& r_;
    } memory(address, prefetch, remote_);

    auto disassembler = xgdb::make_z80_disassembler();
    auto formatter    = xgdb::make_sdcc_z80_formatter();

    std::vector<disassembly_line> result;
    uint32_t pc = address;
    for (std::size_t i = 0; i < count; ++i) {
        const auto instr = disassembler->disassemble_one(pc, memory);
        result.push_back({pc, instr.bytes, formatter->format(instr)});
        pc += static_cast<uint32_t>(instr.bytes.size());
    }
    return result;
}

std::vector<uint8_t> debugger_session::read_memory(uint32_t address, std::size_t length) {
    return remote_.read_memory(address, length);
}

void debugger_session::write_memory(uint32_t address, const std::vector<uint8_t>& data) {
    remote_.write_memory(address, data);
}

std::vector<const xgdb::variable*> debugger_session::visible_variables(uint32_t pc) const {
    std::vector<const xgdb::variable*> result;
    if (!document_.has_value()) return result;

    const auto* function = find_function_for_pc(pc);
    for (const auto& v : document_->variables) {
        if (v.start_address.has_value() && v.end_address.has_value() &&
            (pc < v.start_address.value() || pc >= v.end_address.value()))
            continue;
        if (function && v.parent_name.has_value() &&
            v.parent_name.value() != function->name)
            continue;
        result.push_back(&v);
    }
    return result;
}

std::vector<std::pair<std::string, uint32_t>> debugger_session::register_values() {
    const auto regs = read_registers();
    return {
        {"af", regs.af}, {"bc", regs.bc}, {"de", regs.de}, {"hl", regs.hl},
        {"ix", regs.ix}, {"iy", regs.iy}, {"sp", regs.sp}, {"pc", regs.pc},
        {"i",  regs.i},  {"r",  regs.r}
    };
}

void debugger_session::add_source_dir(const std::filesystem::path& dir) {
    source_dirs_.push_back(dir);
}

std::filesystem::path debugger_session::resolve_source_path(
    const std::filesystem::path& p) const
{
    if (std::filesystem::exists(p)) return p;
    // Search registered source directories.
    for (const auto& dir : source_dirs_) {
        const auto candidate = dir / p.filename();
        if (std::filesystem::exists(candidate)) return candidate;
    }
    // Fall back to the directory of the CDB file itself.
    if (cdb_path_.has_value()) {
        const auto candidate = cdb_path_->parent_path() / p.filename();
        if (std::filesystem::exists(candidate)) return candidate;
    }

    // Try swapping .c ↔ .s in case CDB recorded the wrong extension.
    auto alt = p;
    if (p.extension() == ".c")      alt.replace_extension(".s");
    else if (p.extension() == ".s") alt.replace_extension(".c");
    if (alt != p) {
        if (std::filesystem::exists(alt)) return alt;
        for (const auto& dir : source_dirs_) {
            const auto candidate = dir / alt.filename();
            if (std::filesystem::exists(candidate)) return candidate;
        }
        if (cdb_path_.has_value()) {
            const auto candidate = cdb_path_->parent_path() / alt.filename();
            if (std::filesystem::exists(candidate)) return candidate;
        }
    }

    return p;  // return as-is; open will fail and caller handles it
}

std::optional<std::string> debugger_session::read_source_line(
    const std::filesystem::path& path, uint32_t target_line) const
{
    std::ifstream input(resolve_source_path(path));
    if (!input.is_open()) return std::nullopt;
    std::string line; uint32_t n = 0;
    while (std::getline(input, line))
        if (++n == target_line) return line;
    return std::nullopt;
}

std::vector<source_line_view> debugger_session::source_window(
    const std::filesystem::path& path,
    uint32_t target_line,
    uint32_t context_before,
    uint32_t context_after) const
{
    const uint32_t start = target_line > context_before ? target_line - context_before : 1;
    const uint32_t end   = target_line + context_after;
    return source_window_range(path, target_line, start, end);
}

std::vector<source_line_view> debugger_session::source_window_range(
    const std::filesystem::path& path,
    uint32_t target_line,
    uint32_t start_line,
    uint32_t end_line) const
{
    const auto resolved = resolve_source_path(path);
    std::ifstream input(resolved);
    if (!input.is_open())
        throw std::runtime_error("cannot open source file: " + resolved.string());

    std::vector<source_line_view> result;
    std::string line; uint32_t n = 0;
    while (std::getline(input, line)) {
        ++n;
        if (n < start_line) continue;
        if (n > end_line)   break;
        result.push_back({n, line, n == target_line});
    }
    return result;
}

const xgdb::source_file* debugger_session::find_file(uint32_t file_id) const {
    if (!document_.has_value()) return nullptr;
    for (const auto& f : document_->files) if (f.id == file_id) return &f;
    return nullptr;
}

const xgdb::function* debugger_session::find_function_by_name(const std::string& name) const {
    if (!document_.has_value()) return nullptr;
    for (const auto& f : document_->functions) if (f.name == name) return &f;
    return nullptr;
}

const xgdb::symbol* debugger_session::find_symbol_by_name(const std::string& name) const {
    if (!document_.has_value()) return nullptr;
    for (const auto& s : document_->symbols) if (s.name == name) return &s;
    return nullptr;
}

const xgdb::function* debugger_session::find_function_for_pc(uint32_t pc) const {
    if (!document_.has_value()) return nullptr;
    for (const auto& f : document_->functions)
        if (pc >= f.start_address && pc < f.end_address) return &f;
    return nullptr;
}

const xgdb::line_entry* debugger_session::find_line_for_pc_in_function(
    const xgdb::function& function, uint32_t pc) const
{
    if (!document_.has_value() || !function.file_id.has_value()) return nullptr;
    const xgdb::line_entry* best = nullptr;
    for (const auto& l : document_->lines) {
        if (l.file_id != function.file_id.value() ||
            l.address < function.start_address ||
            l.address >= function.end_address || l.address > pc) continue;
        if (!best || l.address > best->address) best = &l;
    }
    return best;
}

const xgdb::line_entry* debugger_session::find_line_for_pc(uint32_t pc) const {
    if (!document_.has_value()) return nullptr;
    const xgdb::line_entry* best = nullptr;
    for (const auto& l : document_->lines)
        if (l.address <= pc && (!best || l.address > best->address)) best = &l;
    return best;
}

const xgdb::line_entry* debugger_session::find_line_for_function(
    const xgdb::function& function) const
{
    if (!document_.has_value() || !function.file_id.has_value()) return nullptr;
    if (function.line.has_value())
        for (const auto& l : document_->lines)
            if (l.file_id == function.file_id.value() && l.line == function.line.value())
                return &l;
    const xgdb::line_entry* best = nullptr;
    for (const auto& l : document_->lines) {
        if (l.file_id != function.file_id.value() ||
            l.address < function.start_address ||
            l.address >= function.end_address) continue;
        if (!best || l.address < best->address) best = &l;
    }
    return best;
}

const xgdb::line_entry* debugger_session::find_line_by_file_and_line(
    uint32_t file_id, uint32_t line_number) const
{
    if (!document_.has_value()) return nullptr;
    const xgdb::line_entry* best = nullptr;
    for (const auto& l : document_->lines) {
        if (l.file_id != file_id || l.line < line_number) continue;
        if (!best || l.line < best->line ||
            (l.line == best->line && l.address < best->address)) best = &l;
    }
    return best;
}

std::optional<uint32_t> debugger_session::find_line_end_address(
    const xgdb::line_entry& target) const
{
    if (!document_.has_value()) return std::nullopt;
    uint32_t best = 0; bool found = false;
    for (const auto& l : document_->lines)
        if (l.address > target.address && (!found || l.address < best))
            { best = l.address; found = true; }
    if (found) return best;
    if (const auto* f = find_function_for_pc(target.address)) return f->end_address;
    return std::nullopt;
}
