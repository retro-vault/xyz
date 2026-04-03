#ifndef LIBMICRODRIVE_H
#define LIBMICRODRIVE_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace microdrive {

namespace fs = std::filesystem;

constexpr int k_num_sectors = 254;
constexpr int k_sector_size = 543;
constexpr int k_image_size = k_num_sectors * k_sector_size + 1;
constexpr int k_data_size = 512;
constexpr int k_name_len = 10;

constexpr int k_off_header = 0;
constexpr int k_off_record = 15;
constexpr int k_off_data = 30;
constexpr int k_off_data_chk = 542;

#pragma pack(push, 1)
struct header_t {
    uint8_t flag;
    uint8_t sector_num;
    uint8_t unused[2];
    char cart_name[k_name_len];
    uint8_t hdr_chk;
};
static_assert(sizeof(header_t) == 15);

struct record_t {
    uint8_t flag;
    uint8_t rec_num;
    uint16_t length;
    char filename[k_name_len];
    uint8_t rec_chk;
};
static_assert(sizeof(record_t) == 15);
#pragma pack(pop)

struct directory_entry_t {
    std::string name;
    std::size_t bytes;
    int sectors;
};

enum class compatibility_t {
    standard,
    fuse
};

class image_t {
public:
    image_t() = default;

    static image_t load(const fs::path& path);
    static image_t load(const fs::path& path, compatibility_t compatibility);
    static image_t create_blank(std::string_view cart_name);
    static image_t create_blank(std::string_view cart_name, int sector_count);

    void save(const fs::path& path) const;

    std::vector<directory_entry_t> directory() const;
    void put(std::string_view name, std::span<const uint8_t> payload);
    std::vector<uint8_t> get(std::string_view name) const;
    int remove(std::string_view name);

    std::size_t size() const;
    int block_count() const;
    bool write_protected() const;
    void set_write_protected(bool enabled);

    uint8_t raw_byte(std::size_t index) const;
    void set_raw_byte(std::size_t index, uint8_t value);

    bool is_free(int sector) const;
    bool header_checksum_ok(int sector) const;
    bool record_checksum_ok(int sector) const;

private:
    explicit image_t(std::vector<uint8_t> data, int sector_count);

    header_t& header(int sector);
    const header_t& header(int sector) const;
    record_t& record(int sector);
    const record_t& record(int sector) const;
    uint8_t* sector_data(int sector);
    const uint8_t* sector_data(int sector) const;
    uint8_t& data_chk(int sector);
    const uint8_t& data_chk(int sector) const;
    void fix_checksums(int sector);

    std::vector<uint8_t> data_;
    int sector_count_ = k_num_sectors;
};

std::array<char, k_name_len> pad_name(std::string_view name);
std::string trim_name(const char* field);
bool name_match(const char* field, std::string_view want);
uint8_t checksum(const uint8_t* data, int len);

}  // namespace microdrive

#endif
