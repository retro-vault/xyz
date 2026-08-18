#include <algorithm>
#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include <xprog/errors.h>
#include <xprog/tape.h>

namespace xprog {
namespace {

void append16(std::vector<std::uint8_t>& out, std::uint16_t value)
{
    out.push_back(static_cast<std::uint8_t>(value));
    out.push_back(static_cast<std::uint8_t>(value >> 8));
}

void append_block(std::vector<std::uint8_t>& tap, std::uint8_t flag,
                  const std::vector<std::uint8_t>& payload)
{
    if (payload.size() > 65533U)
        throw error("Spectrum tape block is too large");
    append16(tap, static_cast<std::uint16_t>(payload.size() + 2U));
    tap.push_back(flag);
    std::uint8_t checksum = flag;
    for (const auto byte : payload) {
        tap.push_back(byte);
        checksum ^= byte;
    }
    tap.push_back(checksum);
}

std::array<std::uint8_t, 10> tape_name(const std::string& name)
{
    if (name.empty() || name.size() > 10)
        throw error("Spectrum tape name must contain 1 to 10 bytes");
    std::array<std::uint8_t, 10> result{};
    result.fill(' ');
    std::copy(name.begin(), name.end(), result.begin());
    return result;
}

std::vector<std::uint8_t> header(std::uint8_t type,
                                 const std::array<std::uint8_t, 10>& name,
                                 std::uint16_t length,
                                 std::uint16_t parameter1,
                                 std::uint16_t parameter2)
{
    std::vector<std::uint8_t> result;
    result.reserve(17);
    result.push_back(type);
    result.insert(result.end(), name.begin(), name.end());
    append16(result, length);
    append16(result, parameter1);
    append16(result, parameter2);
    return result;
}

void append_text(std::vector<std::uint8_t>& out, std::uint16_t value)
{
    const auto text = std::to_string(value);
    out.insert(out.end(), text.begin(), text.end());
}

std::vector<std::uint8_t> loader(std::uint16_t load_address,
                                 std::uint16_t entry_point,
                                 std::uint16_t binary_size)
{
    // Keep the loader itself below the normal 48K BASIC program address.
    // The machine-code tail of line 1 starts at 0x5cd0: line 10 calls it,
    // it consumes the following CODE header and data blocks through the ROM
    // loader, then returns from the ROM directly to the program entry point.
    // This deliberately permits the CODE block to overwrite the BASIC loader,
    // making 0x5ccb (the first byte after the standard system variables) a
    // usable load address.
    constexpr std::uint16_t loader_entry = 0x5cd0;
    constexpr std::uint16_t header_buffer = 0x5b00;

    std::vector<std::uint8_t> machine = {
        0xdd, 0x21,
        static_cast<std::uint8_t>(header_buffer),
        static_cast<std::uint8_t>(header_buffer >> 8), // LD IX,header_buffer
        0x11, 0x11, 0x00,                              // LD DE,17
        0xaf,                                          // XOR A (header flag)
        0x37,                                          // SCF (load, not verify)
        0xcd, 0x56, 0x05,                              // CALL ROM LD-BYTES
        0xd0,                                          // RET NC on tape error
        0xdd, 0x21,
        static_cast<std::uint8_t>(load_address),
        static_cast<std::uint8_t>(load_address >> 8),  // LD IX,load_address
        0x11,
        static_cast<std::uint8_t>(binary_size),
        static_cast<std::uint8_t>(binary_size >> 8),   // LD DE,binary_size
        0x3e, 0xff,                                    // LD A,code flag
        0x37,                                          // SCF
        0x21,
        static_cast<std::uint8_t>(entry_point),
        static_cast<std::uint8_t>(entry_point >> 8),   // LD HL,entry_point
        0xe5,                                          // PUSH HL
        0xc3, 0x56, 0x05                               // JP ROM LD-BYTES
    };

    std::vector<std::uint8_t> program;
    program.push_back(0x00); // line 1, big-endian
    program.push_back(0x01);
    append16(program, static_cast<std::uint16_t>(machine.size() + 2U));
    program.push_back(0xea); // REM
    program.insert(program.end(), machine.begin(), machine.end());
    program.push_back(0x0d);

    std::vector<std::uint8_t> body;
    body.push_back(0xf9); // RANDOMIZE
    body.push_back(' ');
    body.push_back(0xc0); // USR
    body.push_back(' ');
    body.push_back(0xb0); // VAL
    body.push_back('"');
    append_text(body, loader_entry);
    body.push_back('"');
    body.push_back(0x0d);

    program.push_back(0x00); // line 10, big-endian
    program.push_back(0x0a);
    append16(program, static_cast<std::uint16_t>(body.size()));
    program.insert(program.end(), body.begin(), body.end());
    return program;
}

} // namespace

std::vector<std::uint8_t> build_tap(
    const std::vector<std::uint8_t>& binary,
    std::uint16_t load_address,
    std::uint16_t entry_point,
    const std::string& name)
{
    if (binary.empty())
        throw error("cannot create a tape from an empty binary");
    if (load_address == 0)
        throw error("Spectrum tape load address must be nonzero");
    if (binary.size() > 65533U
        || binary.size() > 0x10000U - load_address) {
        throw error("binary does not fit at the Spectrum load address");
    }
    const auto end = static_cast<std::uint32_t>(load_address) + binary.size();
    if (entry_point < load_address || entry_point >= end)
        throw error("Spectrum tape entry point lies outside the binary");

    const auto encoded_name = tape_name(name);
    const auto basic = loader(load_address, entry_point,
                              static_cast<std::uint16_t>(binary.size()));
    std::vector<std::uint8_t> tap;
    append_block(tap, 0x00, header(0, encoded_name,
                                  static_cast<std::uint16_t>(basic.size()),
                                  10, static_cast<std::uint16_t>(basic.size())));
    append_block(tap, 0xff, basic);
    append_block(tap, 0x00, header(3, encoded_name,
                                  static_cast<std::uint16_t>(binary.size()),
                                  load_address, 0x8000));
    append_block(tap, 0xff, binary);
    return tap;
}

std::vector<std::uint8_t> tap_to_tzx(const std::vector<std::uint8_t>& tap)
{
    const std::array<std::uint8_t, 10> signature = {
        'Z','X','T','a','p','e','!',0x1a,1,20
    };
    std::vector<std::uint8_t> tzx(signature.begin(), signature.end());
    std::size_t cursor = 0;
    while (cursor < tap.size()) {
        if (tap.size() - cursor < 2)
            throw error("truncated TAP block length");
        const std::uint16_t size = static_cast<std::uint16_t>(tap[cursor])
            | static_cast<std::uint16_t>(tap[cursor + 1] << 8);
        cursor += 2;
        if (size == 0 || size > tap.size() - cursor)
            throw error("truncated TAP block");
        tzx.push_back(0x10); // standard-speed data block
        append16(tzx, 1000); // one-second pause
        append16(tzx, size);
        tzx.insert(tzx.end(), tap.begin() + cursor, tap.begin() + cursor + size);
        cursor += size;
    }
    return tzx;
}

} // namespace xprog
