#include <algorithm>
#include <array>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>

#include <xprog/errors.h>
#include <xprog/cpc.h>
#include <xprog/package.h>
#include <xprog/tape.h>

namespace xprog {
namespace {

std::uint16_t get16(const std::vector<std::uint8_t>& data, std::size_t offset)
{
    return static_cast<std::uint16_t>(data[offset])
        | static_cast<std::uint16_t>(data[offset + 1] << 8);
}

std::uint32_t get32(const std::vector<std::uint8_t>& data, std::size_t offset)
{
    return static_cast<std::uint32_t>(data[offset])
        | (static_cast<std::uint32_t>(data[offset + 1]) << 8)
        | (static_cast<std::uint32_t>(data[offset + 2]) << 16)
        | (static_cast<std::uint32_t>(data[offset + 3]) << 24);
}

void put16(std::vector<std::uint8_t>& data, std::size_t offset,
           std::uint16_t value)
{
    data[offset] = static_cast<std::uint8_t>(value);
    data[offset + 1] = static_cast<std::uint8_t>(value >> 8);
}

void put32(std::vector<std::uint8_t>& data, std::size_t offset,
           std::uint32_t value)
{
    for (unsigned i = 0; i < 4; ++i)
        data[offset + i] = static_cast<std::uint8_t>(value >> (8 * i));
}

std::uint32_t crc32(const std::vector<std::uint8_t>& data)
{
    std::uint32_t crc = 0xffffffffU;
    for (const auto byte : data) {
        crc ^= byte;
        for (unsigned bit = 0; bit < 8; ++bit)
            crc = (crc >> 1) ^ (0xedb88320U & (0U - (crc & 1U)));
    }
    return ~crc;
}

void check_range(std::size_t offset, std::size_t length, std::size_t size,
                 const std::string& what)
{
    if (offset > size || length > size - offset)
        throw error(what + " lies outside the image");
}

} // namespace

xl_info parse_xl(const std::vector<std::uint8_t>& bytes)
{
    if (bytes.size() < 12 || bytes[0] != 'X' || bytes[1] != 'L')
        throw error("input is not an XL image");
    xl_info info;
    info.version = bytes[2];
    info.flags = bytes[3];
    info.entry_point = get16(bytes, 4);
    info.code_size = get16(bytes, 6);
    info.relocation_count = get16(bytes, 8);
    if (info.version != 1)
        throw error("unsupported XL version " + std::to_string(info.version));
    info.code_offset = 12U + static_cast<std::size_t>(info.relocation_count) * 4U;
    const auto expected = info.code_offset + info.code_size;
    if (expected != bytes.size())
        throw error("malformed XL size (expected " + std::to_string(expected)
                    + ", got " + std::to_string(bytes.size()) + ")");
    if (info.code_size == 0 || info.entry_point >= info.code_size)
        throw error("XL entry point lies outside its code");
    for (std::uint16_t i = 0; i < info.relocation_count; ++i) {
        const auto record = 12U + static_cast<std::size_t>(i) * 4U;
        const auto offset = get16(bytes, record);
        const auto width = bytes[record + 2];
        if ((width != 1 && width != 2)
            || offset >= info.code_size || width > info.code_size - offset) {
            throw error("XL relocation " + std::to_string(i)
                        + " lies outside its code");
        }
    }
    return info;
}

std::uint32_t name_id(const std::string& name)
{
    std::uint32_t hash = 2166136261U;
    for (const unsigned char byte : name) {
        hash ^= byte;
        hash *= 16777619U;
    }
    return hash;
}

std::vector<std::uint8_t> build_image(
    const cli_options& options, const std::vector<std::uint8_t>& xl_bytes)
{
    const auto xl = parse_xl(xl_bytes);
    if (options.command != command_kind::process
        && options.command != command_kind::service) {
        throw error("packaging requires process or service mode");
    }
    if (options.name.empty() || options.name.size() > 15)
        throw error("image name must contain 1 to 15 bytes");
    if (options.command == command_kind::process) {
        if (!options.stack_size.has_value() || options.stack_size.value() == 0)
            throw error("process stack size must be nonzero");
        if (!options.exports.empty())
            throw error("a process cannot contain a service JP table");
    } else {
        if (options.stack_size.has_value())
            throw error("a service cannot request a process stack");
        if (options.exports.empty())
            throw error("service must contain at least one JP entry");
    }
    if (options.exports.size() > UINT16_MAX)
        throw error("too many exports");
    for (std::size_t i = 0; i < options.exports.size(); ++i) {
        if (options.exports[i] >= xl.code_size)
            throw error("export " + std::to_string(i)
                        + " target lies outside XL code");
    }

    const std::size_t metadata = header_size
        + options.exports.size() * jump_entry_size;
    if (metadata > UINT16_MAX || xl_bytes.size() > UINT32_MAX)
        throw error("image is too large for XPRG version 1");
    std::vector<std::uint8_t> image(metadata + xl_bytes.size(), 0);
    image[0] = 'X'; image[1] = 'P'; image[2] = 'R'; image[3] = 'G';
    image[4] = format_version;
    image[5] = static_cast<std::uint8_t>(
        options.command == command_kind::process
            ? image_kind::process : image_kind::service);
    image[6] = options.abi_version;
    std::uint8_t flags = 0;
    if (options.require_fixed_load) flags |= fixed_load;
    if (options.command == command_kind::process || options.entry_point.has_value())
        flags |= has_entry;
    if (!options.exports.empty()) flags |= has_jump_table;
    image[7] = flags;
    put16(image, 8, static_cast<std::uint16_t>(metadata));
    put16(image, 10, static_cast<std::uint16_t>(metadata));
    put32(image, 12, static_cast<std::uint32_t>(xl_bytes.size()));
    put32(image, 16, crc32(xl_bytes));
    put32(image, 20, options.image_id.value_or(name_id(options.name)));
    put16(image, 24, options.load_address);
    const auto entry = options.entry_point.value_or(
        options.command == command_kind::process ? xl.entry_point : no_entry);
    if (options.command == command_kind::process && entry == no_entry)
        throw error("process must have an entry point");
    if (entry != no_entry && entry >= xl.code_size)
        throw error("entry point lies outside XL code");
    put16(image, 26, entry);
    put16(image, 28, options.command == command_kind::process
        ? options.stack_size.value_or(0) : 0);
    if (options.command == command_kind::process
        && get16(image, 28) == 0)
        throw error("process stack size must be nonzero");
    put16(image, 30, options.minimum_os_version);

    std::size_t cursor = header_size;
    if (!options.exports.empty()) {
        put16(image, 32, static_cast<std::uint16_t>(cursor));
        put16(image, 34, static_cast<std::uint16_t>(options.exports.size()));
        for (const auto target : options.exports) {
            image[cursor] = 0xc3;
            put16(image, cursor + 1, target);
            cursor += jump_entry_size;
        }
    }
    std::copy(options.name.begin(), options.name.end(), image.begin() + 40);
    std::copy(xl_bytes.begin(), xl_bytes.end(), image.begin() + metadata);
    return image;
}

image_info parse_image(const std::vector<std::uint8_t>& bytes)
{
    if (bytes.size() < header_size || bytes[0] != 'X' || bytes[1] != 'P'
        || bytes[2] != 'R' || bytes[3] != 'G')
        throw error("input is not an XPRG image");
    if (bytes[4] != format_version)
        throw error("unsupported XPRG version " + std::to_string(bytes[4]));
    if (bytes[5] != static_cast<std::uint8_t>(image_kind::process)
        && bytes[5] != static_cast<std::uint8_t>(image_kind::service))
        throw error("invalid XPRG image kind");

    image_info info;
    info.kind = static_cast<image_kind>(bytes[5]);
    info.abi_version = bytes[6];
    info.flags = bytes[7];
    constexpr std::uint8_t known_flags = fixed_load | has_entry | has_jump_table;
    if ((info.flags & ~known_flags) != 0)
        throw error("XPRG image uses unknown flags");
    info.metadata_size = get16(bytes, 8);
    const auto payload_offset = get16(bytes, 10);
    info.payload_size = get32(bytes, 12);
    info.payload_crc32 = get32(bytes, 16);
    info.image_id = get32(bytes, 20);
    info.preferred_load_address = get16(bytes, 24);
    info.entry_point = get16(bytes, 26);
    info.stack_size = get16(bytes, 28);
    info.minimum_os_version = get16(bytes, 30);
    if (info.metadata_size < header_size || payload_offset != info.metadata_size)
        throw error("invalid XPRG metadata/payload offsets");
    check_range(payload_offset, info.payload_size, bytes.size(), "XPRG payload");
    if (static_cast<std::size_t>(payload_offset) + info.payload_size != bytes.size())
        throw error("trailing data after XPRG payload");
    const auto zero = std::find(bytes.begin() + 40, bytes.begin() + 56, 0);
    if (zero == bytes.begin() + 56)
        throw error("XPRG image name is not NUL terminated");
    info.name.assign(bytes.begin() + 40, zero);
    if (info.name.empty())
        throw error("XPRG image has an empty name");

    const auto jump_offset = get16(bytes, 32);
    const auto jump_count = get16(bytes, 34);
    if ((jump_count == 0) != (jump_offset == 0))
        throw error("inconsistent XPRG jump table");
    if (jump_count) {
        if (jump_offset != header_size
            || info.metadata_size != header_size
                + static_cast<std::size_t>(jump_count) * jump_entry_size) {
            throw error("invalid XPRG jump-table layout");
        }
        check_range(jump_offset,
                    static_cast<std::size_t>(jump_count) * jump_entry_size,
                    info.metadata_size, "XPRG jump table");
        for (std::uint16_t i = 0; i < jump_count; ++i) {
            const auto offset = jump_offset + i * jump_entry_size;
            if (bytes[offset] != 0xc3)
                throw error("invalid opcode in XPRG jump table");
            info.exports.push_back(get16(bytes, offset + 1));
        }
    }
    if (!jump_count && info.metadata_size != header_size)
        throw error("unexpected XPRG metadata");
    if ((jump_count != 0) != ((info.flags & has_jump_table) != 0))
        throw error("XPRG jump-table flag does not match its count");
    if ((info.entry_point != no_entry) != ((info.flags & has_entry) != 0))
        throw error("XPRG entry flag does not match its value");
    if (!std::all_of(bytes.begin() + 36, bytes.begin() + 40,
                     [](std::uint8_t byte) { return byte == 0; })
        || !std::all_of(bytes.begin() + 56, bytes.begin() + 64,
                        [](std::uint8_t byte) { return byte == 0; })) {
        throw error("nonzero reserved bytes in XPRG header");
    }
    info.payload.assign(bytes.begin() + payload_offset, bytes.end());
    if (crc32(info.payload) != info.payload_crc32)
        throw error("XPRG payload checksum mismatch");
    info.xl = parse_xl(info.payload);
    if (info.entry_point != no_entry && info.entry_point >= info.xl.code_size)
        throw error("XPRG entry lies outside XL code");
    for (const auto target : info.exports)
        if (target >= info.xl.code_size)
            throw error("XPRG jump target lies outside XL code");
    if (info.kind == image_kind::process) {
        if (info.stack_size == 0)
            throw error("XPRG process has zero stack size");
        if (info.entry_point == no_entry)
            throw error("XPRG process has no entry point");
        if (!info.exports.empty())
            throw error("XPRG process has a service JP table");
    } else {
        if (info.stack_size != 0)
            throw error("XPRG service has a stack size");
        if (info.exports.empty())
            throw error("XPRG service has no JP table");
    }
    return info;
}

std::vector<std::uint8_t> read_file(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input)
        throw error("cannot open input file: " + path.string());
    input.seekg(0, std::ios::end);
    const auto length = input.tellg();
    if (length < 0)
        throw error("cannot determine input size: " + path.string());
    input.seekg(0);
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(length));
    if (!bytes.empty())
        input.read(reinterpret_cast<char*>(bytes.data()), length);
    if (!input)
        throw error("cannot read input file: " + path.string());
    return bytes;
}

void write_file(const std::filesystem::path& path,
                const std::vector<std::uint8_t>& bytes)
{
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output)
        throw error("cannot open output file: " + path.string());
    if (!bytes.empty())
        output.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    if (!output)
        throw error("cannot write output file: " + path.string());
}

void inspect_image(const image_info& image, std::ostream& out)
{
    out << "format: XPRG v1\n"
        << "kind: " << (image.kind == image_kind::process ? "process" : "service") << "\n"
        << "name: " << image.name << "\n"
        << "id: 0x" << std::hex << std::setw(8) << std::setfill('0') << image.image_id << "\n"
        << "abi: " << std::dec << static_cast<unsigned>(image.abi_version) << "\n"
        << "minimum OS ABI: " << image.minimum_os_version << "\n"
        << "preferred load address: 0x" << std::hex << std::setw(4)
        << image.preferred_load_address << "\n"
        << "fixed load: " << ((image.flags & fixed_load) ? "yes" : "no") << "\n"
        << "entry: ";
    if (image.entry_point == no_entry)
        out << "none\n";
    else
        out << "0x" << std::hex << std::setw(4) << image.entry_point << "\n";
    out << "stack size: " << std::dec << image.stack_size << "\n"
        << "jump entries: " << image.exports.size() << "\n";
    for (std::size_t i = 0; i < image.exports.size(); ++i) {
        const auto resident = static_cast<std::uint32_t>(
            image.preferred_load_address) + i * jump_entry_size;
        out << "  [" << i << "] address=0x" << std::hex << std::setw(4)
            << resident << " target-offset=0x" << std::setw(4)
            << image.exports[i] << "\n";
    }
    out << std::dec << "XL code size: " << image.xl.code_size << "\n"
        << "XL relocations: " << image.xl.relocation_count << "\n"
        << "payload bytes: " << image.payload_size << "\n"
        << "payload CRC32: 0x" << std::hex << std::setw(8)
        << image.payload_crc32 << std::dec << "\n";
}

void run(const cli_options& options, std::ostream& out)
{
    if (options.command == command_kind::inspect) {
        inspect_image(parse_image(read_file(options.input_file)), out);
        return;
    }
    if (options.command == command_kind::tap
        || options.command == command_kind::tzx) {
        auto tape = build_tap(read_file(options.input_file),
                              options.load_address,
                              options.entry_point.value_or(options.load_address),
                              options.name);
        if (options.command == command_kind::tzx)
            tape = tap_to_tzx(tape);
        write_file(options.output_file, tape);
        return;
    }
    if (options.command == command_kind::cdt) {
        write_file(options.output_file,
                   build_cdt(read_file(options.input_file),
                             options.load_address,
                             options.entry_point.value_or(
                                 options.load_address),
                             options.name));
        return;
    }
    if (options.command == command_kind::dsk) {
        write_file(options.output_file,
                   build_dsk(read_file(options.input_file),
                             options.load_address,
                             options.entry_point.value_or(
                                 options.load_address),
                             options.name));
        return;
    }
    const auto image = build_image(options, read_file(options.input_file));
    write_file(options.output_file, image);
}

} // namespace xprog
