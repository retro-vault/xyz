#include "appmake/tape.h"

#include <algorithm>
#include <array>
#include <format>
#include <stdexcept>

#include "appmake/util.h"

namespace appmake {

namespace {

void flush_pending_header(
    std::vector<tape_list_entry>& entries,
    std::optional<zx_header_block>& pending,
    std::size_t& next_index,
    std::string_view source
) {
    if (!pending) {
        return;
    }

    entries.push_back({
        .index = next_index++,
        .source = std::string(source),
        .kind = std::format("{}_header", zx_block_type_name(pending->type)),
        .role = pending->type == 0x00 ? "basic" : "header",
        .name = pending->name,
        .size = pending->data_len,
        .details = std::format("header only, {}", zx_header_details(*pending)),
    });

    pending.reset();
}

void append_data_entry(
    std::vector<tape_list_entry>& entries,
    std::optional<zx_header_block>& pending,
    std::size_t& next_index,
    std::string_view source,
    std::span<const uint8_t> block,
    std::string_view details
) {
    if (const auto header = decode_zx_header(block)) {
        flush_pending_header(entries, pending, next_index, source);
        pending = *header;
        return;
    }

    if (is_zx_data_block(block)) {
        const std::size_t payload_size = block.size() - 2;
        if (pending && pending->data_len == payload_size) {
            std::string entry_details = zx_header_details(*pending);
            if (!details.empty()) {
                entry_details += std::format(", {}", details);
            }

            entries.push_back({
                .index = next_index++,
                .source = std::string(source),
                .kind = pending->type == 0x03
                    ? zx_code_kind(pending->param1, pending->data_len)
                    : zx_block_type_name(pending->type),
                .role = pending->type == 0x03
                    ? zx_code_role(pending->param1, pending->data_len)
                    : zx_block_type_name(pending->type),
                .name = pending->name,
                .size = payload_size,
                .details = std::move(entry_details),
            });
            pending.reset();
            return;
        }

        flush_pending_header(entries, pending, next_index, source);
        entries.push_back({
            .index = next_index++,
            .source = std::string(source),
            .kind = "data",
            .role = "raw_data",
            .name = "",
            .size = payload_size,
            .details = details.empty()
                ? "raw ROM data block"
                : std::format("raw ROM data block, {}", details),
        });
        return;
    }

    flush_pending_header(entries, pending, next_index, source);
    entries.push_back({
        .index = next_index++,
        .source = std::string(source),
        .kind = "block",
        .role = "non_rom_block",
        .name = "",
        .size = block.size(),
        .details = details.empty()
            ? "non-ROM data block"
            : std::format("non-ROM data block, {}", details),
    });
}

std::string read_tzx_text(std::span<const uint8_t> text) {
    return trim_name(std::string(reinterpret_cast<const char*>(text.data()), text.size()));
}

void append_meta_entry(
    std::vector<tape_list_entry>& entries,
    std::size_t& next_index,
    std::string_view source,
    std::string kind,
    std::string name,
    std::optional<std::size_t> size,
    std::string details
) {
    entries.push_back({
        .index = next_index++,
        .source = std::string(source),
        .kind = std::move(kind),
        .role = "",
        .name = std::move(name),
        .size = size,
        .details = std::move(details),
    });
}

void append_file_from_block(
    std::vector<tap_file>& files,
    std::optional<zx_header_block>& pending,
    std::span<const uint8_t> block
) {
    if (const auto header = decode_zx_header(block)) {
        pending = *header;
        return;
    }

    if (!pending || !is_zx_data_block(block)) {
        pending.reset();
        return;
    }

    const std::size_t payload_size = block.size() - 2;
    if (payload_size != pending->data_len) {
        pending.reset();
        return;
    }

    tap_file file;
    file.header = *pending;
    file.data.assign(block.begin() + 1, block.end() - 1);
    file.tape_flag = block[0];
    file.tape_checksum = block.back();
    files.push_back(std::move(file));
    pending.reset();
}

}  // namespace

std::string zx_block_type_name(uint8_t type) {
    switch (type) {
    case 0x00: return "basic";
    case 0x01: return "number_array";
    case 0x02: return "char_array";
    case 0x03: return "machine_code";
    default:   return std::format("type_{:02x}", type);
    }
}

std::string zx_code_role(uint16_t load_addr, uint16_t data_len) {
    const uint32_t start = load_addr;
    const uint32_t end = start + data_len;

    if (load_addr == 0x4000 && data_len == 6912) {
        return "full_screen";
    }

    if (data_len == 6912) {
        return "screen_data";
    }

    if (start >= 0x4000 && end <= 0x5800) {
        return "screen_bitmap";
    }

    if (load_addr == 0x5800 && data_len == 768) {
        return "screen_attributes";
    }

    if (start >= 0x5800 && end <= 0x5b00) {
        return "partial_attributes";
    }

    if (load_addr >= 0x5b00) {
        return "program_code";
    }

    return "machine code";
}

std::string zx_code_kind(uint16_t, uint16_t data_len) {
    if (data_len == 6912) {
        return "screen";
    }

    return "machine_code";
}

std::string zx_header_details(const zx_header_block& header) {
    switch (header.type) {
    case 0x00:
        return std::format("basic autostart={} vars={}", header.param1, header.param2);
    case 0x01:
    case 0x02:
        return std::format("var=0x{:04x}", header.param1);
    case 0x03:
        return std::format("{} load=0x{:04x} param2=0x{:04x} size={}",
                           zx_code_role(header.param1, header.data_len),
                           header.param1,
                           header.param2,
                           header.data_len);
    default:
        return std::format("param1=0x{:04x} param2=0x{:04x}", header.param1, header.param2);
    }
}

std::optional<zx_header_block> decode_zx_header(std::span<const uint8_t> block) {
    if (block.size() != 19 || block[0] != 0x00 || !tape_checksum_ok(block)) {
        return std::nullopt;
    }

    zx_header_block header;
    header.type = block[1];
    header.name = trim_name(std::string(reinterpret_cast<const char*>(block.data() + 2), 10));
    header.data_len = rd16(block.data() + 12);
    header.param1 = rd16(block.data() + 14);
    header.param2 = rd16(block.data() + 16);
    return header;
}

bool is_zx_data_block(std::span<const uint8_t> block) {
    return block.size() >= 2 && block[0] == 0xff && tape_checksum_ok(block);
}

std::vector<tape_list_entry> parse_tap_list(const fs::path& path) {
    const auto bytes = read_file(path);
    std::vector<tape_list_entry> entries;
    std::optional<zx_header_block> pending;
    std::size_t pos = 0;
    std::size_t next_index = 1;

    while (pos + 2 <= bytes.size()) {
        const uint16_t len = rd16(bytes.data() + pos);
        pos += 2;
        ensure_size(bytes, pos, len, "tap");

        std::span<const uint8_t> block(bytes.data() + pos, len);
        append_data_entry(entries, pending, next_index, "tap", block, "");
        pos += len;
    }

    if (pos != bytes.size()) {
        throw std::runtime_error("malformed tap: trailing byte after block table");
    }

    flush_pending_header(entries, pending, next_index, "tap");
    return entries;
}

std::vector<tape_list_entry> parse_tzx_list(const fs::path& path) {
    const auto bytes = read_file(path);
    if (bytes.size() < 10) {
        throw std::runtime_error("malformed tzx: file too short");
    }

    static constexpr std::array<uint8_t, 8> k_tzx_signature = {
        'Z', 'X', 'T', 'a', 'p', 'e', '!', 0x1a
    };

    if (!std::equal(k_tzx_signature.begin(), k_tzx_signature.end(), bytes.begin())) {
        throw std::runtime_error("invalid tzx signature");
    }

    std::vector<tape_list_entry> entries;
    std::optional<zx_header_block> pending;
    std::size_t pos = 10;
    std::size_t next_index = 1;
    const std::string source = std::format("tzx v{}.{}", bytes[8], bytes[9]);

    while (pos < bytes.size()) {
        const uint8_t id = bytes[pos++];

        switch (id) {
        case 0x10: {
            ensure_size(bytes, pos, 4, "tzx standard data block");
            const uint16_t pause_ms = rd16(bytes.data() + pos);
            const uint16_t data_len = rd16(bytes.data() + pos + 2);
            pos += 4;
            ensure_size(bytes, pos, data_len, "tzx standard data payload");
            std::span<const uint8_t> data(bytes.data() + pos, data_len);
            append_data_entry(entries, pending, next_index, source, data,
                              std::format("block=0x10 pause={}ms", pause_ms));
            pos += data_len;
            break;
        }
        case 0x11: {
            ensure_size(bytes, pos, 18, "tzx turbo data block");
            const uint16_t pause_ms = rd16(bytes.data() + pos + 13);
            const uint32_t data_len = rd24(bytes.data() + pos + 15);
            pos += 18;
            ensure_size(bytes, pos, data_len, "tzx turbo data payload");
            std::span<const uint8_t> data(bytes.data() + pos, data_len);
            append_data_entry(entries, pending, next_index, source, data,
                              std::format("block=0x11 pause={}ms", pause_ms));
            pos += data_len;
            break;
        }
        case 0x12: {
            ensure_size(bytes, pos, 4, "tzx pure tone block");
            const uint16_t pulse_len = rd16(bytes.data() + pos);
            const uint16_t pulse_count = rd16(bytes.data() + pos + 2);
            append_meta_entry(entries, next_index, source, "pure_tone", "", std::nullopt,
                              std::format("pulse_len={} pulses={}", pulse_len, pulse_count));
            pos += 4;
            break;
        }
        case 0x13: {
            ensure_size(bytes, pos, 1, "tzx pulse sequence block");
            const uint8_t pulse_count = bytes[pos];
            ensure_size(bytes, pos, static_cast<std::size_t>(1 + pulse_count * 2), "tzx pulse sequence data");
            append_meta_entry(entries, next_index, source, "pulse_sequence", "", std::nullopt,
                              std::format("pulses={}", pulse_count));
            pos += static_cast<std::size_t>(1 + pulse_count * 2);
            break;
        }
        case 0x14: {
            ensure_size(bytes, pos, 10, "tzx pure data block");
            const uint16_t pause_ms = rd16(bytes.data() + pos + 5);
            const uint32_t data_len = rd24(bytes.data() + pos + 7);
            pos += 10;
            ensure_size(bytes, pos, data_len, "tzx pure data payload");
            std::span<const uint8_t> data(bytes.data() + pos, data_len);
            append_data_entry(entries, pending, next_index, source, data,
                              std::format("block=0x14 pause={}ms", pause_ms));
            pos += data_len;
            break;
        }
        case 0x15: {
            ensure_size(bytes, pos, 8, "tzx direct recording block");
            const uint16_t pause_ms = rd16(bytes.data() + pos + 2);
            const uint32_t data_len = rd24(bytes.data() + pos + 5);
            pos += 8;
            ensure_size(bytes, pos, data_len, "tzx direct recording payload");
            flush_pending_header(entries, pending, next_index, source);
            append_meta_entry(entries, next_index, source, "direct_recording", "",
                              data_len,
                              std::format("pause={}ms", pause_ms));
            pos += data_len;
            break;
        }
        case 0x18: {
            ensure_size(bytes, pos, 4, "tzx csw block");
            const uint32_t block_len = rd32(bytes.data() + pos);
            ensure_size(bytes, pos, static_cast<std::size_t>(4 + block_len), "tzx csw block");
            flush_pending_header(entries, pending, next_index, source);
            append_meta_entry(entries, next_index, source, "csw_recording", "",
                              block_len,
                              "");
            pos += static_cast<std::size_t>(4 + block_len);
            break;
        }
        case 0x19: {
            ensure_size(bytes, pos, 4, "tzx generalized block");
            const uint32_t block_len = rd32(bytes.data() + pos);
            ensure_size(bytes, pos, static_cast<std::size_t>(4 + block_len), "tzx generalized block");
            flush_pending_header(entries, pending, next_index, source);
            append_meta_entry(entries, next_index, source, "generalized_data", "",
                              block_len,
                              "");
            pos += static_cast<std::size_t>(4 + block_len);
            break;
        }
        case 0x20: {
            ensure_size(bytes, pos, 2, "tzx pause block");
            const uint16_t pause_ms = rd16(bytes.data() + pos);
            append_meta_entry(entries, next_index, source, "pause", "", std::nullopt,
                              pause_ms == 0 ? "stop tape" : std::format("{}ms", pause_ms));
            pos += 2;
            break;
        }
        case 0x21: {
            ensure_size(bytes, pos, 1, "tzx group start block");
            const uint8_t text_len = bytes[pos];
            ensure_size(bytes, pos + 1, text_len, "tzx group start text");
            append_meta_entry(entries, next_index, source, "group_start",
                              read_tzx_text({bytes.data() + pos + 1, text_len}),
                              std::nullopt, "");
            pos += static_cast<std::size_t>(1 + text_len);
            break;
        }
        case 0x22:
            append_meta_entry(entries, next_index, source, "group_end", "", std::nullopt, "");
            break;
        case 0x23: {
            ensure_size(bytes, pos, 2, "tzx jump block");
            const int16_t rel = static_cast<int16_t>(rd16(bytes.data() + pos));
            append_meta_entry(entries, next_index, source, "jump", "", std::nullopt,
                              std::format("relative={}", rel));
            pos += 2;
            break;
        }
        case 0x24: {
            ensure_size(bytes, pos, 2, "tzx loop start block");
            const uint16_t repeat = rd16(bytes.data() + pos);
            append_meta_entry(entries, next_index, source, "loop_start", "", std::nullopt,
                              std::format("repeat={}", repeat));
            pos += 2;
            break;
        }
        case 0x25:
            append_meta_entry(entries, next_index, source, "loop_end", "", std::nullopt, "");
            break;
        case 0x26: {
            ensure_size(bytes, pos, 2, "tzx call sequence block");
            const uint16_t call_count = rd16(bytes.data() + pos);
            ensure_size(bytes, pos, static_cast<std::size_t>(2 + call_count * 2), "tzx call sequence data");
            append_meta_entry(entries, next_index, source, "call_sequence", "", std::nullopt,
                              std::format("calls={}", call_count));
            pos += static_cast<std::size_t>(2 + call_count * 2);
            break;
        }
        case 0x27:
            append_meta_entry(entries, next_index, source, "return", "", std::nullopt, "");
            break;
        case 0x28: {
            ensure_size(bytes, pos, 2, "tzx select block");
            const uint16_t block_len = rd16(bytes.data() + pos);
            ensure_size(bytes, pos, static_cast<std::size_t>(2 + block_len), "tzx select data");
            append_meta_entry(entries, next_index, source, "select", "", block_len, "");
            pos += static_cast<std::size_t>(2 + block_len);
            break;
        }
        case 0x2a:
            ensure_size(bytes, pos, 4, "tzx stop-48k block");
            append_meta_entry(entries, next_index, source, "stop_48k", "", std::nullopt, "");
            pos += 4;
            break;
        case 0x2b:
            ensure_size(bytes, pos, 5, "tzx signal level block");
            append_meta_entry(entries, next_index, source, "signal_level", "", std::nullopt,
                              bytes[pos + 4] == 0 ? "low" : "high");
            pos += 5;
            break;
        case 0x30: {
            ensure_size(bytes, pos, 1, "tzx text block");
            const uint8_t text_len = bytes[pos];
            ensure_size(bytes, pos + 1, text_len, "tzx text payload");
            append_meta_entry(entries, next_index, source, "text",
                              read_tzx_text({bytes.data() + pos + 1, text_len}),
                              std::nullopt, "");
            pos += static_cast<std::size_t>(1 + text_len);
            break;
        }
        case 0x31: {
            ensure_size(bytes, pos, 2, "tzx message block");
            const uint8_t duration = bytes[pos];
            const uint8_t text_len = bytes[pos + 1];
            ensure_size(bytes, pos + 2, text_len, "tzx message payload");
            append_meta_entry(entries, next_index, source, "message",
                              read_tzx_text({bytes.data() + pos + 2, text_len}),
                              std::nullopt,
                              std::format("duration={}s", duration));
            pos += static_cast<std::size_t>(2 + text_len);
            break;
        }
        case 0x32: {
            ensure_size(bytes, pos, 2, "tzx archive info block");
            const uint16_t block_len = rd16(bytes.data() + pos);
            ensure_size(bytes, pos, static_cast<std::size_t>(2 + block_len), "tzx archive info payload");
            append_meta_entry(entries, next_index, source, "archive_info", "", block_len, "");
            pos += static_cast<std::size_t>(2 + block_len);
            break;
        }
        case 0x33: {
            ensure_size(bytes, pos, 1, "tzx hardware block");
            const uint8_t count = bytes[pos];
            ensure_size(bytes, pos, static_cast<std::size_t>(1 + count * 3), "tzx hardware payload");
            append_meta_entry(entries, next_index, source, "hardware_info", "", count, "");
            pos += static_cast<std::size_t>(1 + count * 3);
            break;
        }
        case 0x34:
            ensure_size(bytes, pos, 8, "tzx emulation info block");
            append_meta_entry(entries, next_index, source, "emulation_info", "", std::nullopt, "");
            pos += 8;
            break;
        case 0x35: {
            ensure_size(bytes, pos, 20, "tzx custom info block");
            const std::string name = trim_name(std::string(reinterpret_cast<const char*>(bytes.data() + pos), 16));
            const uint32_t data_len = rd32(bytes.data() + pos + 16);
            ensure_size(bytes, pos, static_cast<std::size_t>(20 + data_len), "tzx custom info payload");
            append_meta_entry(entries, next_index, source, "custom_info", name, data_len, "");
            pos += static_cast<std::size_t>(20 + data_len);
            break;
        }
        case 0x40: {
            ensure_size(bytes, pos, 4, "tzx screen block");
            const uint8_t snap_type = bytes[pos];
            const uint32_t snap_len = rd24(bytes.data() + pos + 1);
            ensure_size(bytes, pos, static_cast<std::size_t>(4 + snap_len), "tzx screen payload");
            append_meta_entry(entries, next_index, source, "screen_block", "", snap_len,
                              snap_type == 0 ? "z80 snapshot" : "sna snapshot");
            pos += static_cast<std::size_t>(4 + snap_len);
            break;
        }
        case 0x5a:
            ensure_size(bytes, pos, 9, "tzx glue block");
            append_meta_entry(entries, next_index, source, "glue", "", std::nullopt, "");
            pos += 9;
            break;
        default:
            throw std::runtime_error(std::format("unsupported tzx block id 0x{:02x}", id));
        }
    }

    flush_pending_header(entries, pending, next_index, source);
    return entries;
}

std::vector<tap_file> parse_tap_files(const fs::path& path) {
    const auto bytes = read_file(path);
    std::vector<tap_file> files;
    std::optional<zx_header_block> pending;
    std::size_t pos = 0;

    while (pos + 2 <= bytes.size()) {
        const uint16_t len = rd16(bytes.data() + pos);
        pos += 2;
        ensure_size(bytes, pos, len, "tap");

        std::span<const uint8_t> block(bytes.data() + pos, len);
        pos += len;
        append_file_from_block(files, pending, block);
    }

    return files;
}

std::vector<tap_file> parse_tzx_files(const fs::path& path) {
    const auto bytes = read_file(path);
    if (bytes.size() < 10) {
        throw std::runtime_error("malformed tzx: file too short");
    }

    static constexpr std::array<uint8_t, 8> k_tzx_signature = {
        'Z', 'X', 'T', 'a', 'p', 'e', '!', 0x1a
    };

    if (!std::equal(k_tzx_signature.begin(), k_tzx_signature.end(), bytes.begin())) {
        throw std::runtime_error("invalid tzx signature");
    }

    std::vector<tap_file> files;
    std::optional<zx_header_block> pending;
    std::size_t pos = 10;

    while (pos < bytes.size()) {
        const uint8_t id = bytes[pos++];

        switch (id) {
        case 0x10: {
            ensure_size(bytes, pos, 4, "tzx standard data block");
            const uint16_t data_len = rd16(bytes.data() + pos + 2);
            pos += 4;
            ensure_size(bytes, pos, data_len, "tzx standard data payload");
            append_file_from_block(files, pending, {bytes.data() + pos, data_len});
            pos += data_len;
            break;
        }
        case 0x11: {
            ensure_size(bytes, pos, 18, "tzx turbo data block");
            const uint32_t data_len = rd24(bytes.data() + pos + 15);
            pos += 18;
            ensure_size(bytes, pos, data_len, "tzx turbo data payload");
            append_file_from_block(files, pending, {bytes.data() + pos, data_len});
            pos += data_len;
            break;
        }
        case 0x14: {
            ensure_size(bytes, pos, 10, "tzx pure data block");
            const uint32_t data_len = rd24(bytes.data() + pos + 7);
            pos += 10;
            ensure_size(bytes, pos, data_len, "tzx pure data payload");
            append_file_from_block(files, pending, {bytes.data() + pos, data_len});
            pos += data_len;
            break;
        }
        case 0x12:
            ensure_size(bytes, pos, 4, "tzx pure tone block");
            pos += 4;
            break;
        case 0x13: {
            ensure_size(bytes, pos, 1, "tzx pulse sequence block");
            const uint8_t pulse_count = bytes[pos];
            ensure_size(bytes, pos, static_cast<std::size_t>(1 + pulse_count * 2), "tzx pulse sequence data");
            pos += static_cast<std::size_t>(1 + pulse_count * 2);
            break;
        }
        case 0x15: {
            ensure_size(bytes, pos, 8, "tzx direct recording block");
            const uint32_t data_len = rd24(bytes.data() + pos + 5);
            pos += 8;
            ensure_size(bytes, pos, data_len, "tzx direct recording payload");
            pos += data_len;
            pending.reset();
            break;
        }
        case 0x18:
        case 0x19: {
            ensure_size(bytes, pos, 4, "tzx variable-length block");
            const uint32_t block_len = rd32(bytes.data() + pos);
            ensure_size(bytes, pos, static_cast<std::size_t>(4 + block_len), "tzx variable-length payload");
            pos += static_cast<std::size_t>(4 + block_len);
            pending.reset();
            break;
        }
        case 0x20:
            ensure_size(bytes, pos, 2, "tzx pause block");
            pos += 2;
            break;
        case 0x21: {
            ensure_size(bytes, pos, 1, "tzx group start block");
            const uint8_t text_len = bytes[pos];
            ensure_size(bytes, pos + 1, text_len, "tzx group start text");
            pos += static_cast<std::size_t>(1 + text_len);
            break;
        }
        case 0x22:
            break;
        case 0x23:
        case 0x24: {
            ensure_size(bytes, pos, 2, "tzx control block");
            pos += 2;
            break;
        }
        case 0x25:
        case 0x27:
            break;
        case 0x26: {
            ensure_size(bytes, pos, 2, "tzx call sequence block");
            const uint16_t call_count = rd16(bytes.data() + pos);
            ensure_size(bytes, pos, static_cast<std::size_t>(2 + call_count * 2), "tzx call sequence data");
            pos += static_cast<std::size_t>(2 + call_count * 2);
            break;
        }
        case 0x28: {
            ensure_size(bytes, pos, 2, "tzx select block");
            const uint16_t block_len = rd16(bytes.data() + pos);
            ensure_size(bytes, pos, static_cast<std::size_t>(2 + block_len), "tzx select data");
            pos += static_cast<std::size_t>(2 + block_len);
            break;
        }
        case 0x2a:
            ensure_size(bytes, pos, 4, "tzx stop-48k block");
            pos += 4;
            break;
        case 0x2b:
            ensure_size(bytes, pos, 5, "tzx signal level block");
            pos += 5;
            break;
        case 0x30: {
            ensure_size(bytes, pos, 1, "tzx text block");
            const uint8_t text_len = bytes[pos];
            ensure_size(bytes, pos + 1, text_len, "tzx text payload");
            pos += static_cast<std::size_t>(1 + text_len);
            break;
        }
        case 0x31: {
            ensure_size(bytes, pos, 2, "tzx message block");
            const uint8_t text_len = bytes[pos + 1];
            ensure_size(bytes, pos + 2, text_len, "tzx message payload");
            pos += static_cast<std::size_t>(2 + text_len);
            break;
        }
        case 0x32: {
            ensure_size(bytes, pos, 2, "tzx archive info block");
            const uint16_t block_len = rd16(bytes.data() + pos);
            ensure_size(bytes, pos, static_cast<std::size_t>(2 + block_len), "tzx archive info payload");
            pos += static_cast<std::size_t>(2 + block_len);
            break;
        }
        case 0x33: {
            ensure_size(bytes, pos, 1, "tzx hardware block");
            const uint8_t count = bytes[pos];
            ensure_size(bytes, pos, static_cast<std::size_t>(1 + count * 3), "tzx hardware payload");
            pos += static_cast<std::size_t>(1 + count * 3);
            break;
        }
        case 0x34:
            ensure_size(bytes, pos, 8, "tzx emulation info block");
            pos += 8;
            break;
        case 0x35: {
            ensure_size(bytes, pos, 20, "tzx custom info block");
            const uint32_t data_len = rd32(bytes.data() + pos + 16);
            ensure_size(bytes, pos, static_cast<std::size_t>(20 + data_len), "tzx custom info payload");
            pos += static_cast<std::size_t>(20 + data_len);
            break;
        }
        case 0x40: {
            ensure_size(bytes, pos, 4, "tzx screen block");
            const uint32_t snap_len = rd24(bytes.data() + pos + 1);
            ensure_size(bytes, pos, static_cast<std::size_t>(4 + snap_len), "tzx screen payload");
            pos += static_cast<std::size_t>(4 + snap_len);
            pending.reset();
            break;
        }
        case 0x5a:
            ensure_size(bytes, pos, 9, "tzx glue block");
            pos += 9;
            break;
        default:
            throw std::runtime_error(std::format("unsupported tzx block id 0x{:02x}", id));
        }
    }

    return files;
}

std::vector<tap_file> parse_tape_files(const fs::path& path) {
    const std::string ext = lower_copy(path.extension().string());
    if (ext == ".tap") {
        return parse_tap_files(path);
    }
    if (ext == ".tzx") {
        return parse_tzx_files(path);
    }
    throw std::runtime_error("tape parsing currently supports .tap and .tzx files");
}

tap_code_block parse_tap_code(
    const fs::path& path,
    const std::optional<std::string>& wanted_name
) {
    const auto files = parse_tap_files(path);
    const std::optional<std::string> wanted =
        wanted_name ? std::optional<std::string>(upper_copy(*wanted_name)) : std::nullopt;

    for (const auto& file : files) {
        if (file.header.type != 0x03) {
            continue;
        }

        if (wanted && upper_copy(file.header.name) != *wanted) {
            continue;
        }

        tap_code_block out;
        out.name = file.header.name;
        out.load_addr = file.header.param1;
        out.data = file.data;
        return out;
    }

    if (wanted_name) {
        throw std::runtime_error(std::format("no matching CODE block found in TAP: {}", *wanted_name));
    }
    throw std::runtime_error("no CODE block found in TAP");
}

}  // namespace appmake
