#include "libmicrodrive.h"

#include <algorithm>
#include <cstring>
#include <format>
#include <fstream>
#include <map>
#include <stdexcept>

namespace microdrive {

namespace {

int validate_sector_count(int sector_count) {
    if (sector_count < 1 || sector_count > k_num_sectors) {
        throw std::runtime_error(std::format(
            "invalid sector count: {} (allowed range: 1..{})",
            sector_count,
            k_num_sectors));
    }
    return sector_count;
}

int sector_count_from_image_size(std::uintmax_t size,
                                 compatibility_t compatibility,
                                 const fs::path& path) {
    if (size == static_cast<std::uintmax_t>(k_image_size)) {
        return k_num_sectors;
    }

    if (compatibility == compatibility_t::fuse &&
        size > 1 &&
        ((size - 1) % static_cast<std::uintmax_t>(k_sector_size)) == 0) {
        const std::uintmax_t sectors = (size - 1) / static_cast<std::uintmax_t>(k_sector_size);
        if (sectors >= 1 && sectors <= static_cast<std::uintmax_t>(k_num_sectors)) {
            return static_cast<int>(sectors);
        }
    }

    if (compatibility == compatibility_t::fuse) {
        throw std::runtime_error(std::format(
            "{}: expected {} bytes (standard) or (N*{}+1) bytes where N=1..{}, got {}",
            path.string(),
            k_image_size,
            k_sector_size,
            k_num_sectors,
            size));
    }

    throw std::runtime_error(std::format(
        "{}: expected {} bytes, got {}",
        path.string(),
        k_image_size,
        size));
}

}  // namespace

uint8_t checksum(const uint8_t* data, int len) {
    uint16_t sum = 0;
    for (int i = 0; i < len; ++i) {
        sum += data[i];
    }
    sum %= 255;
    return sum == 255 ? 254 : static_cast<uint8_t>(sum);
}

std::array<char, k_name_len> pad_name(std::string_view name) {
    std::array<char, k_name_len> out {};
    out.fill(' ');
    const auto n = std::min(name.size(), static_cast<std::size_t>(k_name_len));
    std::copy_n(name.begin(), n, out.begin());
    return out;
}

std::string trim_name(const char* field) {
    std::string s(field, k_name_len);
    const auto pos = s.find_last_not_of(' ');
    return pos == std::string::npos ? "" : s.substr(0, pos + 1);
}

bool name_match(const char* field, std::string_view want) {
    auto fold = [](std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        return s;
    };

    return fold(trim_name(field)) == fold(std::string(want));
}

image_t::image_t(std::vector<uint8_t> data, int sector_count)
    : data_(std::move(data)),
      sector_count_(validate_sector_count(sector_count)) {}

image_t image_t::load(const fs::path& path) {
    return load(path, compatibility_t::standard);
}

image_t image_t::load(const fs::path& path, compatibility_t compatibility) {
    if (!fs::exists(path)) {
        throw std::runtime_error(std::format("not found: {}", path.string()));
    }

    const auto size = fs::file_size(path);
    const int sector_count = sector_count_from_image_size(size, compatibility, path);

    std::vector<uint8_t> data(size);
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw std::runtime_error(std::format("cannot open: {}", path.string()));
    }
    in.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(size));
    return image_t(std::move(data), sector_count);
}

image_t image_t::create_blank(std::string_view cart_name) {
    return create_blank(cart_name, k_num_sectors);
}

image_t image_t::create_blank(std::string_view cart_name, int sector_count) {
    sector_count = validate_sector_count(sector_count);
    std::vector<uint8_t> data(static_cast<std::size_t>(sector_count) * k_sector_size + 1, 0);
    const auto padded = pad_name(cart_name);

    for (int sector = 0; sector < sector_count; ++sector) {
        auto* sec = data.data() + sector * k_sector_size;

        auto* hdr = reinterpret_cast<header_t*>(sec + k_off_header);
        hdr->flag = 0x0f;
        hdr->sector_num = static_cast<uint8_t>(sector_count - sector);
        hdr->unused[0] = hdr->unused[1] = 0;
        std::copy(padded.begin(), padded.end(), hdr->cart_name);
        hdr->hdr_chk = checksum(reinterpret_cast<uint8_t*>(hdr), 14);

        auto* rec = reinterpret_cast<record_t*>(sec + k_off_record);
        rec->flag = 0;
        rec->rec_num = 0;
        rec->length = 0;
        std::fill(rec->filename, rec->filename + k_name_len, ' ');
        rec->rec_chk = checksum(reinterpret_cast<uint8_t*>(rec), 14);
    }

    data[static_cast<std::size_t>(sector_count) * k_sector_size] = 0;
    return image_t(std::move(data), sector_count);
}

void image_t::save(const fs::path& path) const {
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        throw std::runtime_error(std::format("cannot write: {}", path.string()));
    }
    out.write(reinterpret_cast<const char*>(data_.data()),
              static_cast<std::streamsize>(data_.size()));
}

std::vector<directory_entry_t> image_t::directory() const {
    std::map<std::string, directory_entry_t> files;

    for (int sector = 0; sector < sector_count_; ++sector) {
        if (is_free(sector)) {
            continue;
        }

        const auto& rec = record(sector);
        const auto name = trim_name(rec.filename);
        if (name.empty()) {
            continue;
        }

        auto& entry = files[name];
        entry.name = name;
        entry.bytes += rec.length;
        entry.sectors += 1;
    }

    std::vector<directory_entry_t> out;
    out.reserve(files.size());
    for (const auto& [_, entry] : files) {
        out.push_back(entry);
    }
    return out;
}

void image_t::put(std::string_view name, std::span<const uint8_t> payload) {
    const auto padded = pad_name(name);
    const auto trimmed = trim_name(padded.data());

    for (int sector = 0; sector < sector_count_; ++sector) {
        if (!is_free(sector) && name_match(record(sector).filename, trimmed)) {
            throw std::runtime_error(
                std::format("'{}' already exists on cartridge", trimmed));
        }
    }

    int needed = static_cast<int>((payload.size() + k_data_size - 1) / k_data_size);
    if (needed == 0) {
        needed = 1;
    }

    std::vector<int> free_sectors;
    for (int sector = 0;
         sector < sector_count_ && static_cast<int>(free_sectors.size()) < needed;
         ++sector) {
        if (is_free(sector)) {
            free_sectors.push_back(sector);
        }
    }

    if (static_cast<int>(free_sectors.size()) < needed) {
        throw std::runtime_error(std::format(
            "not enough free sectors: need {}, have {}", needed, free_sectors.size()));
    }

    std::size_t offset = 0;
    for (int i = 0; i < needed; ++i) {
        const int sector = free_sectors[i];
        auto& rec = record(sector);
        const std::size_t chunk =
            std::min<std::size_t>(k_data_size, payload.size() - offset);

        rec.flag = chunk < static_cast<std::size_t>(k_data_size) ? 0x06 : 0x04;
        rec.rec_num = static_cast<uint8_t>(i);
        rec.length = static_cast<uint16_t>(chunk);
        std::copy(padded.begin(), padded.end(), rec.filename);

        std::memcpy(sector_data(sector), payload.data() + offset, chunk);
        if (chunk < k_data_size) {
            std::memset(sector_data(sector) + chunk, 0, k_data_size - chunk);
        }
        offset += chunk;

        fix_checksums(sector);
    }
}

std::vector<uint8_t> image_t::get(std::string_view name) const {
    std::map<int, int> records;
    for (int sector = 0; sector < sector_count_; ++sector) {
        if (is_free(sector)) {
            continue;
        }
        if (name_match(record(sector).filename, name)) {
            records[record(sector).rec_num] = sector;
        }
    }

    if (records.empty()) {
        throw std::runtime_error(std::format("'{}' not found on cartridge", name));
    }

    std::vector<uint8_t> payload;
    for (const auto& [_, sector] : records) {
        const auto* data = sector_data(sector);
        payload.insert(payload.end(), data, data + record(sector).length);
    }
    return payload;
}

int image_t::remove(std::string_view name) {
    int deleted = 0;

    for (int sector = 0; sector < sector_count_; ++sector) {
        if (is_free(sector) || !name_match(record(sector).filename, name)) {
            continue;
        }

        auto& rec = record(sector);
        rec.flag = 0;
        rec.rec_num = 0;
        rec.length = 0;
        std::fill(rec.filename, rec.filename + k_name_len, ' ');
        std::memset(sector_data(sector), 0, k_data_size);
        fix_checksums(sector);
        ++deleted;
    }

    return deleted;
}

std::size_t image_t::size() const {
    return data_.size();
}

int image_t::block_count() const {
    return sector_count_;
}

bool image_t::write_protected() const {
    return data_.at(static_cast<std::size_t>(sector_count_) * k_sector_size) != 0;
}

void image_t::set_write_protected(bool enabled) {
    data_.at(static_cast<std::size_t>(sector_count_) * k_sector_size) = enabled ? 1 : 0;
}

uint8_t image_t::raw_byte(std::size_t index) const {
    return data_.at(index);
}

void image_t::set_raw_byte(std::size_t index, uint8_t value) {
    data_.at(index) = value;
}

bool image_t::is_free(int sector) const {
    return record(sector).flag == 0;
}

bool image_t::header_checksum_ok(int sector) const {
    const auto& hdr = header(sector);
    return checksum(reinterpret_cast<const uint8_t*>(&hdr), 14) == hdr.hdr_chk;
}

bool image_t::record_checksum_ok(int sector) const {
    const auto& rec = record(sector);
    return checksum(reinterpret_cast<const uint8_t*>(&rec), 14) == rec.rec_chk;
}

header_t& image_t::header(int sector) {
    return *reinterpret_cast<header_t*>(data_.data() + sector * k_sector_size + k_off_header);
}

const header_t& image_t::header(int sector) const {
    return *reinterpret_cast<const header_t*>(
        data_.data() + sector * k_sector_size + k_off_header);
}

record_t& image_t::record(int sector) {
    return *reinterpret_cast<record_t*>(data_.data() + sector * k_sector_size + k_off_record);
}

const record_t& image_t::record(int sector) const {
    return *reinterpret_cast<const record_t*>(
        data_.data() + sector * k_sector_size + k_off_record);
}

uint8_t* image_t::sector_data(int sector) {
    return data_.data() + sector * k_sector_size + k_off_data;
}

const uint8_t* image_t::sector_data(int sector) const {
    return data_.data() + sector * k_sector_size + k_off_data;
}

uint8_t& image_t::data_chk(int sector) {
    return data_[sector * k_sector_size + k_off_data_chk];
}

const uint8_t& image_t::data_chk(int sector) const {
    return data_[sector * k_sector_size + k_off_data_chk];
}

void image_t::fix_checksums(int sector) {
    header(sector).hdr_chk = checksum(reinterpret_cast<uint8_t*>(&header(sector)), 14);
    record(sector).rec_chk = checksum(reinterpret_cast<uint8_t*>(&record(sector)), 14);
    data_chk(sector) = checksum(sector_data(sector), k_data_size);
}

}  // namespace microdrive
