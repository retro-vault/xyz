//
// Amstrad CPC firmware cassette and AMSDOS disk image builders.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include <xprog/cpc.h>
#include <xprog/errors.h>

namespace xprog {
namespace {

constexpr std::uint16_t cpc_zero_pulse = 1167;
constexpr std::uint16_t cpc_one_pulse = 2333;

void append16(std::vector<std::uint8_t>& out, std::uint16_t value)
{
    out.push_back(static_cast<std::uint8_t>(value));
    out.push_back(static_cast<std::uint8_t>(value >> 8));
}

void append24(std::vector<std::uint8_t>& out, std::uint32_t value)
{
    out.push_back(static_cast<std::uint8_t>(value));
    out.push_back(static_cast<std::uint8_t>(value >> 8));
    out.push_back(static_cast<std::uint8_t>(value >> 16));
}

void put16(std::vector<std::uint8_t>& out, std::size_t offset,
           std::uint16_t value)
{
    out[offset] = static_cast<std::uint8_t>(value);
    out[offset + 1] = static_cast<std::uint8_t>(value >> 8);
}

void put24(std::vector<std::uint8_t>& out, std::size_t offset,
           std::uint32_t value)
{
    out[offset] = static_cast<std::uint8_t>(value);
    out[offset + 1] = static_cast<std::uint8_t>(value >> 8);
    out[offset + 2] = static_cast<std::uint8_t>(value >> 16);
}

void validate_binary(const std::vector<std::uint8_t>& binary,
                     std::uint16_t load_address,
                     std::uint16_t entry_point,
                     const std::string& medium)
{
    if (binary.empty())
        throw error("cannot create a " + medium + " from an empty binary");
    if (binary.size() > 0x10000U - load_address)
        throw error("binary does not fit at the CPC load address");
    const auto end = static_cast<std::uint32_t>(load_address) + binary.size();
    if (entry_point < load_address || entry_point >= end)
        throw error("CPC entry point lies outside the binary");
}

std::string uppercase_name(const std::string& name, std::size_t maximum,
                           const std::string& medium)
{
    if (name.empty() || name.size() > maximum)
        throw error("CPC " + medium + " name must contain 1 to "
                    + std::to_string(maximum) + " bytes");
    std::string result;
    result.reserve(name.size());
    for (const unsigned char byte : name) {
        if (byte < 0x21 || byte > 0x7e)
            throw error("CPC " + medium + " name contains an invalid byte");
        result.push_back(static_cast<char>(std::toupper(byte)));
    }
    return result;
}

std::uint16_t cassette_crc(const std::uint8_t* begin,
                           const std::uint8_t* end)
{
    std::uint16_t crc = 0xffff;
    while (begin != end) {
        crc ^= static_cast<std::uint16_t>(*begin++) << 8;
        for (unsigned bit = 0; bit < 8; ++bit) {
            crc = static_cast<std::uint16_t>(
                (crc << 1) ^ ((crc & 0x8000) ? 0x1021 : 0));
        }
    }
    return static_cast<std::uint16_t>(~crc);
}

void append_pause(std::vector<std::uint8_t>& cdt, std::uint16_t ms)
{
    cdt.push_back(0x20);
    append16(cdt, ms);
}

void append_leader(std::vector<std::uint8_t>& cdt)
{
    cdt.push_back(0x12); // pure tone: 2,048 one bits
    append16(cdt, cpc_one_pulse);
    append16(cdt, 4096);
    cdt.push_back(0x13); // one zero bit before the sync byte
    cdt.push_back(2);
    append16(cdt, cpc_zero_pulse);
    append16(cdt, cpc_zero_pulse);
}

void append_record(std::vector<std::uint8_t>& cdt, std::uint8_t sync,
                   const std::vector<std::uint8_t>& payload,
                   std::uint16_t pause_ms)
{
    append_leader(cdt);
    std::vector<std::uint8_t> encoded;
    encoded.reserve(1 + payload.size() + (payload.size() + 255) / 256 * 2
                    + 4);
    encoded.push_back(sync);
    std::size_t offset = 0;
    while (offset < payload.size()) {
        const auto count = std::min<std::size_t>(256, payload.size() - offset);
        encoded.insert(encoded.end(), payload.begin() + offset,
                       payload.begin() + offset + count);
        const auto crc = cassette_crc(payload.data() + offset,
                                      payload.data() + offset + count);
        encoded.push_back(static_cast<std::uint8_t>(crc >> 8));
        encoded.push_back(static_cast<std::uint8_t>(crc));
        offset += count;
    }
    encoded.insert(encoded.end(), 4, 0xff); // 32 one-bit trailer

    cdt.push_back(0x14); // pure data block
    append16(cdt, cpc_zero_pulse);
    append16(cdt, cpc_one_pulse);
    cdt.push_back(8);
    append16(cdt, pause_ms);
    append24(cdt, static_cast<std::uint32_t>(encoded.size()));
    cdt.insert(cdt.end(), encoded.begin(), encoded.end());
}

std::vector<std::uint8_t> cassette_header(
    const std::string& name, std::uint8_t block, bool first, bool last,
    std::uint16_t length, std::uint16_t total_length,
    std::uint16_t load_address, std::uint16_t entry_point)
{
    // The meaningful firmware header is 64 bytes, but every cassette record
    // is made from complete 256-byte CRC segments. The rest is zero padding.
    std::vector<std::uint8_t> header(256, 0);
    std::copy(name.begin(), name.end(), header.begin());
    header[16] = block;
    header[17] = last ? 0xff : 0;
    header[18] = 2; // binary
    put16(header, 19, length);
    put16(header, 21, load_address);
    header[23] = first ? 0xff : 0;
    put16(header, 24, total_length);
    put16(header, 26, entry_point);
    return header;
}

struct disk_name {
    std::array<std::uint8_t, 8> base{};
    std::array<std::uint8_t, 3> extension{};
};

disk_name amsdos_name(const std::string& value)
{
    const auto name = uppercase_name(value, 12, "disk");
    const auto dot = name.find('.');
    if (dot != std::string::npos && name.find('.', dot + 1) != std::string::npos)
        throw error("CPC disk name must use 8.3 form");
    const auto base = name.substr(0, dot);
    auto extension = dot == std::string::npos ? std::string("BIN")
                                              : name.substr(dot + 1);
    if (base.empty() || base.size() > 8 || extension.empty()
        || extension.size() > 3) {
        throw error("CPC disk name must use 8.3 form");
    }
    disk_name result;
    result.base.fill(' ');
    result.extension.fill(' ');
    std::copy(base.begin(), base.end(), result.base.begin());
    std::copy(extension.begin(), extension.end(), result.extension.begin());
    return result;
}

std::vector<std::uint8_t> amsdos_file(
    const std::vector<std::uint8_t>& binary, std::uint16_t load_address,
    std::uint16_t entry_point, const disk_name& name)
{
    std::vector<std::uint8_t> file(128, 0);
    file[0] = 0;
    std::copy(name.base.begin(), name.base.end(), file.begin() + 1);
    std::copy(name.extension.begin(), name.extension.end(), file.begin() + 9);
    file[18] = 2;
    const auto length = static_cast<std::uint16_t>(binary.size());
    put16(file, 19, length);
    put16(file, 21, load_address);
    file[23] = 0xff;
    put16(file, 24, length);
    put16(file, 26, entry_point);
    put24(file, 64, static_cast<std::uint32_t>(binary.size()));
    std::uint16_t checksum = 0;
    for (std::size_t i = 0; i <= 66; ++i)
        checksum = static_cast<std::uint16_t>(checksum + file[i]);
    put16(file, 67, checksum);
    file.insert(file.end(), binary.begin(), binary.end());
    return file;
}

std::vector<std::uint8_t> logical_disk(const std::vector<std::uint8_t>& file,
                                       const disk_name& name)
{
    constexpr std::size_t disk_size = 180 * 1024;
    constexpr std::size_t block_size = 1024;
    constexpr std::size_t first_file_block = 2;
    const auto records = (file.size() + 127) / 128;
    const auto blocks = (file.size() + block_size - 1) / block_size;
    const auto extents = (records + 127) / 128;
    if (first_file_block + blocks > 180 || extents > 64)
        throw error("binary does not fit on a CPC data disk");

    std::vector<std::uint8_t> disk(disk_size, 0xe5);
    for (std::size_t extent = 0; extent < extents; ++extent) {
        const auto offset = extent * 32;
        disk[offset] = 0;
        std::copy(name.base.begin(), name.base.end(), disk.begin() + offset + 1);
        std::copy(name.extension.begin(), name.extension.end(),
                  disk.begin() + offset + 9);
        disk[offset + 12] = static_cast<std::uint8_t>(extent & 0x1f);
        disk[offset + 13] = 0;
        disk[offset + 14] = static_cast<std::uint8_t>(extent >> 5);
        const auto first_record = extent * 128;
        disk[offset + 15] = static_cast<std::uint8_t>(
            std::min<std::size_t>(128, records - first_record));
        const auto first_block = first_file_block + extent * 16;
        const auto extent_blocks = std::min<std::size_t>(
            16, blocks - extent * 16);
        for (std::size_t i = 0; i < extent_blocks; ++i)
            disk[offset + 16 + i] = static_cast<std::uint8_t>(first_block + i);
    }
    const auto destination = disk.begin() + first_file_block * block_size;
    std::copy(file.begin(), file.end(), destination);
    return disk;
}

std::vector<std::uint8_t> cpcemu_disk(
    const std::vector<std::uint8_t>& logical)
{
    constexpr std::size_t tracks = 40;
    constexpr std::size_t sectors = 9;
    constexpr std::size_t sector_size = 512;
    constexpr std::size_t track_size = 0x1300;
    constexpr std::array<std::uint8_t, sectors> sector_ids = {
        0xc1, 0xc6, 0xc2, 0xc7, 0xc3, 0xc8, 0xc4, 0xc9, 0xc5
    };
    std::vector<std::uint8_t> dsk(256 + tracks * track_size, 0);
    const std::string signature = "MV - CPCEMU Disk-File\r\nDisk-Info\r\n";
    const std::string creator = "X Tools xprog ";
    std::copy(signature.begin(), signature.end(), dsk.begin());
    std::copy(creator.begin(), creator.end(), dsk.begin() + 0x22);
    dsk[0x30] = tracks;
    dsk[0x31] = 1;
    put16(dsk, 0x32, track_size);

    for (std::size_t track = 0; track < tracks; ++track) {
        const auto start = 256 + track * track_size;
        const std::string track_signature = "Track-Info\r\n";
        std::copy(track_signature.begin(), track_signature.end(),
                  dsk.begin() + start);
        dsk[start + 0x10] = static_cast<std::uint8_t>(track);
        dsk[start + 0x11] = 0;
        dsk[start + 0x14] = 2;
        dsk[start + 0x15] = sectors;
        dsk[start + 0x16] = 0x4e;
        dsk[start + 0x17] = 0xe5;
        for (std::size_t physical = 0; physical < sectors; ++physical) {
            const auto info = start + 0x18 + physical * 8;
            dsk[info] = static_cast<std::uint8_t>(track);
            dsk[info + 1] = 0;
            dsk[info + 2] = sector_ids[physical];
            dsk[info + 3] = 2;
            put16(dsk, info + 6, sector_size);
            const auto logical_sector = track * sectors
                + static_cast<std::size_t>(sector_ids[physical] - 0xc1);
            std::copy_n(logical.begin() + logical_sector * sector_size,
                        sector_size,
                        dsk.begin() + start + 256 + physical * sector_size);
        }
    }
    return dsk;
}

} // namespace

std::vector<std::uint8_t> build_cdt(
    const std::vector<std::uint8_t>& binary,
    std::uint16_t load_address,
    std::uint16_t entry_point,
    const std::string& name)
{
    validate_binary(binary, load_address, entry_point, "tape");
    const auto encoded_name = uppercase_name(name, 16, "tape");
    std::vector<std::uint8_t> cdt = {
        'Z', 'X', 'T', 'a', 'p', 'e', '!', 0x1a, 1, 20
    };
    append_pause(cdt, 1500);
    std::size_t offset = 0;
    std::uint8_t block = 1;
    while (offset < binary.size()) {
        const auto length = std::min<std::size_t>(2048,
                                                  binary.size() - offset);
        const bool first = offset == 0;
        const bool last = offset + length == binary.size();
        append_record(cdt, 0x2c,
                      cassette_header(
                          encoded_name, block, first, last,
                          static_cast<std::uint16_t>(length),
                          static_cast<std::uint16_t>(binary.size()),
                          static_cast<std::uint16_t>(load_address + offset),
                          entry_point),
                      100);
        std::vector<std::uint8_t> data(binary.begin() + offset,
                                       binary.begin() + offset + length);
        data.resize((data.size() + 255) & ~std::size_t(255), 0);
        append_record(cdt, 0x16, data, last ? 1000 : 500);
        offset += length;
        ++block;
    }
    return cdt;
}

std::vector<std::uint8_t> build_dsk(
    const std::vector<std::uint8_t>& binary,
    std::uint16_t load_address,
    std::uint16_t entry_point,
    const std::string& name)
{
    validate_binary(binary, load_address, entry_point, "disk");
    const auto encoded_name = amsdos_name(name);
    return cpcemu_disk(logical_disk(
        amsdos_file(binary, load_address, entry_point, encoded_name),
        encoded_name));
}

} // namespace xprog
