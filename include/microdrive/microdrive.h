// Declares the reusable ZX Spectrum Microdrive image library used by host
// tools and tests to create, inspect, and modify `.mdr` cartridge images.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih

#ifndef MICRODRIVE_H
#define MICRODRIVE_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace microdrive {

namespace fs = std::filesystem; // Filesystem namespace alias used by the public API.

constexpr int k_num_sectors = 254; // Standard number of sectors in a Microdrive cartridge image.
constexpr int k_sector_size = 543; // Byte size of one encoded Microdrive sector.
constexpr int k_image_size = k_num_sectors * k_sector_size + 1; // Full image byte size including the write-protect byte.
constexpr int k_data_size = 512; // Maximum payload bytes stored in one data record.
constexpr int k_name_len = 10; // Fixed Microdrive filename and cartridge-label length.

constexpr int k_off_header = 0; // Byte offset of the header record within one sector image.
constexpr int k_off_record = 15; // Byte offset of the file record within one sector image.
constexpr int k_off_data = 30; // Byte offset of the payload area within one sector image.
constexpr int k_off_data_chk = 542; // Byte offset of the payload checksum byte within one sector image.

#pragma pack(push, 1)
// On-disk sector header stored ahead of each record.
struct header_t {
    uint8_t flag;
    uint8_t sector_num;
    uint8_t unused[2];
    char cart_name[k_name_len];
    uint8_t hdr_chk;
};
static_assert(sizeof(header_t) == 15);

// On-disk file record metadata stored ahead of each payload block.
struct record_t {
    uint8_t flag;
    uint8_t rec_num;
    uint16_t length;
    char filename[k_name_len];
    uint8_t rec_chk;
};
static_assert(sizeof(record_t) == 15);
#pragma pack(pop)

// Normalized directory entry returned by the host-side library.
struct directory_entry_t {
    std::string name;
    std::size_t bytes;
    int sectors;
};

// Compatibility mode used when reading or writing image bytes.
enum class compatibility_t {
    standard,  // Use the library's native image behavior.
    fuse       // Match Fuse emulator image quirks when needed.
};

// Mutable Microdrive cartridge image object.
class image_t {
public:
    // Construct an empty image wrapper.
    image_t() = default;

    // Load an image file using the default compatibility mode.
    static image_t load(const fs::path& path);
    // Load an image file using an explicit compatibility mode.
    static image_t load(const fs::path& path, compatibility_t compatibility);
    // Create a blank standard-size cartridge image with the supplied label.
    static image_t create_blank(std::string_view cart_name);
    // Create a blank cartridge image with an explicit sector count.
    static image_t create_blank(std::string_view cart_name, int sector_count);

    // Write the current image bytes to disk.
    void save(const fs::path& path) const;

    // Return the normalized directory listing for the image.
    std::vector<directory_entry_t> directory() const;
    // Add or replace one named file payload in the image.
    void put(std::string_view name, std::span<const uint8_t> payload);
    // Read one named file payload from the image.
    std::vector<uint8_t> get(std::string_view name) const;
    // Remove one named file and return the freed sector count.
    int remove(std::string_view name);

    // Return the total image byte size.
    std::size_t size() const;
    // Return the sector count represented by this image.
    int block_count() const;
    // Check whether the write-protect flag byte is set.
    bool write_protected() const;
    // Enable or disable the write-protect flag byte.
    void set_write_protected(bool enabled);

    // Read one raw image byte by absolute offset.
    uint8_t raw_byte(std::size_t index) const;
    // Write one raw image byte by absolute offset.
    void set_raw_byte(std::size_t index, uint8_t value);

    // Check whether the sector is free and unallocated.
    bool is_free(int sector) const;
    // Validate the stored checksum of the sector header.
    bool header_checksum_ok(int sector) const;
    // Validate the stored checksum of the sector record and payload.
    bool record_checksum_ok(int sector) const;

private:
    // Build an image object from raw bytes and explicit geometry.
    explicit image_t(std::vector<uint8_t> data, int sector_count);

    // Access one on-disk sector header.
    header_t& header(int sector);
    // Access one on-disk sector header through a const view.
    const header_t& header(int sector) const;
    // Access one on-disk file record.
    record_t& record(int sector);
    // Access one on-disk file record through a const view.
    const record_t& record(int sector) const;
    // Return a pointer to the sector payload bytes.
    uint8_t* sector_data(int sector);
    // Return a const pointer to the sector payload bytes.
    const uint8_t* sector_data(int sector) const;
    // Access the stored payload checksum byte.
    uint8_t& data_chk(int sector);
    // Access the stored payload checksum byte through a const view.
    const uint8_t& data_chk(int sector) const;
    // Recompute and store all checksums for one sector.
    void fix_checksums(int sector);

    std::vector<uint8_t> data_;
    int sector_count_ = k_num_sectors;
};

// Pad a host filename to the fixed on-disk Microdrive width.
std::array<char, k_name_len> pad_name(std::string_view name);
// Trim trailing spaces from a fixed-width Microdrive name field.
std::string trim_name(const char* field);
// Compare a fixed-width on-disk name against a host string.
bool name_match(const char* field, std::string_view want);
// Compute the Microdrive checksum for a byte sequence.
uint8_t checksum(const uint8_t* data, int len);

}  // namespace microdrive

#endif
