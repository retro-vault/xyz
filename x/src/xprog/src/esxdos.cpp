// Partitioned FAT16 IDE image builder for esxDOS/divIDE.

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <string>
#include <vector>

#include <xprog/errors.h>
#include <xprog/esxdos.h>

namespace xprog {
namespace {

constexpr std::size_t sector_size = 512;
constexpr std::uint32_t disk_sectors = 32768;       // 16 MiB
constexpr std::uint32_t partition_lba = 2048;       // 1 MiB alignment
constexpr std::uint32_t partition_sectors = disk_sectors - partition_lba;
constexpr std::uint16_t root_entries = 512;
constexpr std::uint16_t root_sectors = 32;
constexpr std::uint16_t fat_sectors = 119;
constexpr std::uint32_t data_lba = partition_lba + 1 + 2 * fat_sectors
                                 + root_sectors;
constexpr std::uint32_t data_clusters = partition_sectors - 1
                                      - 2 * fat_sectors - root_sectors;

void put16(std::vector<std::uint8_t>& image, std::size_t offset,
           std::uint16_t value)
{
    image[offset] = static_cast<std::uint8_t>(value);
    image[offset + 1] = static_cast<std::uint8_t>(value >> 8);
}

void put32(std::vector<std::uint8_t>& image, std::size_t offset,
           std::uint32_t value)
{
    for (unsigned byte = 0; byte < 4; ++byte)
        image[offset + byte] = static_cast<std::uint8_t>(value >> (8 * byte));
}

std::array<std::uint8_t, 11> short_name(const std::string& value)
{
    if (value.empty() || value == "." || value == "..")
        throw error("esxDOS filename must use nonempty 8.3 form");
    const auto dot = value.find('.');
    if (dot != value.rfind('.'))
        throw error("esxDOS filename must use 8.3 form");
    const auto base_length = dot == std::string::npos ? value.size() : dot;
    const auto ext_length = dot == std::string::npos ? 0 : value.size() - dot - 1;
    if (base_length == 0 || base_length > 8 || ext_length > 3
        || (dot != std::string::npos && ext_length == 0)) {
        throw error("esxDOS filename must use 8.3 form");
    }

    std::array<std::uint8_t, 11> result{};
    result.fill(' ');
    const auto copy_part = [&](std::size_t source, std::size_t length,
                               std::size_t destination) {
        for (std::size_t index = 0; index < length; ++index) {
            const unsigned char ch = value[source + index];
            if (ch < 0x21 || ch > 0x7e || ch == '"' || ch == '*'
                || ch == '+' || ch == ',' || ch == '/' || ch == ':'
                || ch == ';' || ch == '<' || ch == '=' || ch == '>'
                || ch == '?' || ch == '[' || ch == '\\' || ch == ']'
                || ch == '|') {
                throw error("esxDOS filename contains an invalid FAT character");
            }
            result[destination + index] = static_cast<std::uint8_t>(
                std::toupper(ch));
        }
    };
    copy_part(0, base_length, 0);
    if (ext_length)
        copy_part(dot + 1, ext_length, 8);
    return result;
}

} // namespace

std::vector<std::uint8_t> build_esxdos_disk(
    const std::vector<std::uint8_t>& file, const std::string& name)
{
    if (file.empty())
        throw error("cannot create an esxDOS disk from an empty file");
    const std::uint32_t clusters = static_cast<std::uint32_t>(
        (file.size() + sector_size - 1) / sector_size);
    if (clusters > data_clusters)
        throw error("file does not fit on the esxDOS FAT16 disk");
    const auto encoded_name = short_name(name);

    std::vector<std::uint8_t> image(disk_sectors * sector_size, 0);

    // One bootable FAT16 LBA partition in a conventional MBR.
    const std::size_t partition = 446;
    image[partition] = 0x80;
    image[partition + 1] = 0x01;
    image[partition + 2] = 0x01;
    image[partition + 3] = 0x00;
    image[partition + 4] = 0x06;
    image[partition + 5] = 0xfe;
    image[partition + 6] = 0xff;
    image[partition + 7] = 0xff;
    put32(image, partition + 8, partition_lba);
    put32(image, partition + 12, partition_sectors);
    image[510] = 0x55;
    image[511] = 0xaa;

    const std::size_t boot = partition_lba * sector_size;
    image[boot] = 0xeb; image[boot + 1] = 0x3c; image[boot + 2] = 0x90;
    const std::array<std::uint8_t, 8> oem = {'X','P','R','O','G',' ',' ',' '};
    std::copy(oem.begin(), oem.end(), image.begin() + boot + 3);
    put16(image, boot + 11, sector_size);
    image[boot + 13] = 1;                // one sector per cluster
    put16(image, boot + 14, 1);          // reserved boot sector
    image[boot + 16] = 2;                // two FAT copies
    put16(image, boot + 17, root_entries);
    put16(image, boot + 19, static_cast<std::uint16_t>(partition_sectors));
    image[boot + 21] = 0xf8;
    put16(image, boot + 22, fat_sectors);
    put16(image, boot + 24, 63);
    put16(image, boot + 26, 16);
    put32(image, boot + 28, partition_lba);
    image[boot + 36] = 0x80;
    image[boot + 38] = 0x29;
    put32(image, boot + 39, 0x58505247U); // "GRPX", deterministic ID
    const std::array<std::uint8_t, 11> label =
        {'X','P','R','O','G',' ','D','I','S','K',' '};
    const std::array<std::uint8_t, 8> type = {'F','A','T','1','6',' ',' ',' '};
    std::copy(label.begin(), label.end(), image.begin() + boot + 43);
    std::copy(type.begin(), type.end(), image.begin() + boot + 54);
    image[boot + 510] = 0x55;
    image[boot + 511] = 0xaa;

    const auto set_fat = [&](std::uint32_t copy, std::uint16_t cluster,
                             std::uint16_t value) {
        const std::size_t fat = (partition_lba + 1
            + copy * fat_sectors) * sector_size;
        put16(image, fat + static_cast<std::size_t>(cluster) * 2, value);
    };
    for (std::uint32_t copy = 0; copy < 2; ++copy) {
        set_fat(copy, 0, 0xfff8);
        set_fat(copy, 1, 0xffff);
        for (std::uint32_t index = 0; index < clusters; ++index) {
            const auto cluster = static_cast<std::uint16_t>(2 + index);
            const auto next = static_cast<std::uint16_t>(
                index + 1 == clusters ? 0xffff : cluster + 1);
            set_fat(copy, cluster, next);
        }
    }

    const std::size_t root = (partition_lba + 1 + 2 * fat_sectors)
                           * sector_size;
    std::copy(encoded_name.begin(), encoded_name.end(), image.begin() + root);
    image[root + 11] = 0x20;
    put16(image, root + 16, 0x0021);     // 1980-01-01 creation date
    put16(image, root + 18, 0x0021);     // 1980-01-01 access date
    put16(image, root + 24, 0x0021);     // 1980-01-01 modification date
    put16(image, root + 26, 2);
    put32(image, root + 28, static_cast<std::uint32_t>(file.size()));
    std::copy(file.begin(), file.end(), image.begin() + data_lba * sector_size);
    return image;
}

} // namespace xprog
