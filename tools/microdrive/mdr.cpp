// mdr.cpp — ZX Spectrum Microdrive image tool.
//
// Usage:
//   mdr create <mdr_file> <cart_name>       create a blank cartridge
//   mdr put    <mdr_file> <host_file>        store a file (uses host filename)
//   mdr get    <mdr_file> <mdr_name> <out>   extract a file to host
//   mdr del    <mdr_file> <mdr_name>         delete a file
//   mdr dir    <mdr_file>                    list files

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <map>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// MDR format constants.
// ---------------------------------------------------------------------------
static constexpr int NUM_SECTORS = 254;
static constexpr int SECTOR_SIZE = 543;
static constexpr int MDR_SIZE    = NUM_SECTORS * SECTOR_SIZE + 1;
static constexpr int DATA_SIZE   = 512;
static constexpr int NAME_LEN    = 10;

// Byte offsets within one sector.
static constexpr int OFF_HEADER  =   0;  // md_header  (15 bytes)
static constexpr int OFF_RECORD  =  15;  // md_record  (15 bytes)
static constexpr int OFF_DATA    =  30;  // data block (512 bytes)
static constexpr int OFF_DATACHK = 542;  // data checksum (1 byte)

// ---------------------------------------------------------------------------
// Packed on-disk structures.
// ---------------------------------------------------------------------------
#pragma pack(push, 1)
struct md_header {
    uint8_t flag;
    uint8_t sector_num;
    uint8_t unused[2];
    char    cart_name[NAME_LEN];
    uint8_t hdr_chk;
};
static_assert(sizeof(md_header) == 15);

struct md_record {
    uint8_t  flag;
    uint8_t  rec_num;
    uint16_t length;
    char     filename[NAME_LEN];
    uint8_t  rec_chk;
};
static_assert(sizeof(md_record) == 15);
#pragma pack(pop)

// ---------------------------------------------------------------------------
// Checksum: sum of bytes mod 255, result never 255.
// ---------------------------------------------------------------------------
static uint8_t checksum(const uint8_t* data, int len) {
    uint16_t s = 0;
    for (int i = 0; i < len; i++) s += data[i];
    s %= 255;
    return (s == 255) ? 254 : static_cast<uint8_t>(s);
}

// ---------------------------------------------------------------------------
// RAII MDR image — loads the entire file into memory, flushes on save().
// ---------------------------------------------------------------------------
struct mdr_image {
    std::vector<uint8_t> data;

    explicit mdr_image(const fs::path& path) {
        if (!fs::exists(path))
            throw std::runtime_error(std::format("not found: {}", path.string()));
        auto sz = fs::file_size(path);
        if (sz != MDR_SIZE)
            throw std::runtime_error(std::format(
                "{}: expected {} bytes, got {}", path.string(), MDR_SIZE, sz));
        data.resize(sz);
        std::ifstream ifs(path, std::ios::binary);
        if (!ifs) throw std::runtime_error(std::format("cannot open: {}", path.string()));
        ifs.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(sz));
    }

    void save(const fs::path& path) const {
        std::ofstream ofs(path, std::ios::binary);
        if (!ofs) throw std::runtime_error(std::format("cannot write: {}", path.string()));
        ofs.write(reinterpret_cast<const char*>(data.data()),
                  static_cast<std::streamsize>(data.size()));
    }

    md_header& header(int s) {
        return *reinterpret_cast<md_header*>(data.data() + s * SECTOR_SIZE + OFF_HEADER);
    }
    md_record& record(int s) {
        return *reinterpret_cast<md_record*>(data.data() + s * SECTOR_SIZE + OFF_RECORD);
    }
    uint8_t* sector_data(int s) {
        return data.data() + s * SECTOR_SIZE + OFF_DATA;
    }
    uint8_t& data_chk(int s) {
        return data[s * SECTOR_SIZE + OFF_DATACHK];
    }

    void fix_checksums(int s) {
        header(s).hdr_chk = checksum(reinterpret_cast<uint8_t*>(&header(s)), 14);
        record(s).rec_chk = checksum(reinterpret_cast<uint8_t*>(&record(s)), 14);
        data_chk(s)       = checksum(sector_data(s), DATA_SIZE);
    }

    bool is_free(int s) const {
        const auto& r = *reinterpret_cast<const md_record*>(
            data.data() + s * SECTOR_SIZE + OFF_RECORD);
        return r.flag == 0;
    }
};

// ---------------------------------------------------------------------------
// Name helpers.
// ---------------------------------------------------------------------------

// Pad/truncate name to NAME_LEN, filling remainder with spaces.
static std::array<char, NAME_LEN> pad_name(std::string_view name) {
    std::array<char, NAME_LEN> buf;
    buf.fill(' ');
    auto n = std::min(name.size(), static_cast<std::size_t>(NAME_LEN));
    std::copy_n(name.begin(), n, buf.begin());
    return buf;
}

// Trim trailing spaces from a fixed-width NAME_LEN field.
static std::string trim_name(const char* field) {
    std::string s(field, NAME_LEN);
    auto pos = s.find_last_not_of(' ');
    return (pos == std::string::npos) ? "" : s.substr(0, pos + 1);
}

// Case-insensitive, space-trimmed comparison.
static bool name_match(const char* field, std::string_view want) {
    auto fold = [](std::string s) {
        std::transform(s.begin(), s.end(), s.begin(),
                       [](unsigned char c){ return std::tolower(c); });
        return s;
    };
    return fold(trim_name(field)) == fold(std::string(want));
}

// ---------------------------------------------------------------------------
// Commands.
// ---------------------------------------------------------------------------

void cmd_create(const fs::path& mdr_path, std::string_view cart_name) {
    std::vector<uint8_t> img(MDR_SIZE, 0);
    auto padded = pad_name(cart_name);

    for (int i = 0; i < NUM_SECTORS; i++) {
        uint8_t* sec = img.data() + i * SECTOR_SIZE;

        auto* hdr        = reinterpret_cast<md_header*>(sec + OFF_HEADER);
        hdr->flag        = 0;
        hdr->sector_num  = static_cast<uint8_t>(NUM_SECTORS - i);
        hdr->unused[0]   = hdr->unused[1] = 0;
        std::copy(padded.begin(), padded.end(), hdr->cart_name);
        hdr->hdr_chk     = checksum(reinterpret_cast<uint8_t*>(hdr), 14);

        auto* rec        = reinterpret_cast<md_record*>(sec + OFF_RECORD);
        rec->flag        = 0;
        rec->rec_num     = 0;
        rec->length      = 0;
        std::fill(rec->filename, rec->filename + NAME_LEN, ' ');
        rec->rec_chk     = checksum(reinterpret_cast<uint8_t*>(rec), 14);

        // data block is already zeroed; data checksum stays 0
    }
    img[NUM_SECTORS * SECTOR_SIZE] = 0;  // write-protect byte: 0 = writable

    std::ofstream ofs(mdr_path, std::ios::binary);
    if (!ofs) throw std::runtime_error(std::format("cannot create: {}", mdr_path.string()));
    ofs.write(reinterpret_cast<const char*>(img.data()),
              static_cast<std::streamsize>(img.size()));

    std::cout << std::format("created {} ({} sectors, cart=\"{}\")\n",
        mdr_path.string(), NUM_SECTORS, trim_name(padded.data()));
}

void cmd_dir(const fs::path& mdr_path) {
    mdr_image img(mdr_path);

    // filename -> {total_bytes, sector_count}
    std::map<std::string, std::pair<std::size_t, int>> files;
    for (int s = 0; s < NUM_SECTORS; s++) {
        if (img.is_free(s)) continue;
        const auto& rec = img.record(s);
        auto name = trim_name(rec.filename);
        if (name.empty()) continue;
        auto& e = files[name];
        e.first  += rec.length;
        e.second += 1;
    }

    if (files.empty()) { std::cout << "no files.\n"; return; }

    std::cout << std::format("{:<12} {:>8}  {}\n", "name", "bytes", "sectors");
    std::cout << std::string(30, '-') << "\n";
    for (const auto& [name, info] : files)
        std::cout << std::format("{:<12} {:>8}  {}\n", name, info.first, info.second);
}

void cmd_put(const fs::path& mdr_path, const fs::path& host_file) {
    if (!fs::exists(host_file))
        throw std::runtime_error(std::format("file not found: {}", host_file.string()));

    std::vector<uint8_t> payload(fs::file_size(host_file));
    {
        std::ifstream ifs(host_file, std::ios::binary);
        if (!ifs) throw std::runtime_error(std::format("cannot read: {}", host_file.string()));
        ifs.read(reinterpret_cast<char*>(payload.data()),
                 static_cast<std::streamsize>(payload.size()));
    }

    mdr_image img(mdr_path);

    const auto mdr_name     = pad_name(host_file.filename().string());
    const auto mdr_name_str = trim_name(mdr_name.data());

    for (int s = 0; s < NUM_SECTORS; s++)
        if (!img.is_free(s) && name_match(img.record(s).filename, mdr_name_str))
            throw std::runtime_error(
                std::format("'{}' already exists on cartridge", mdr_name_str));

    int needed = static_cast<int>((payload.size() + DATA_SIZE - 1) / DATA_SIZE);
    if (needed == 0) needed = 1;

    std::vector<int> free_sectors;
    for (int s = 0; s < NUM_SECTORS && static_cast<int>(free_sectors.size()) < needed; s++)
        if (img.is_free(s)) free_sectors.push_back(s);

    if (static_cast<int>(free_sectors.size()) < needed)
        throw std::runtime_error(std::format(
            "not enough free sectors: need {}, have {}", needed, free_sectors.size()));

    std::size_t offset = 0;
    for (int i = 0; i < needed; i++) {
        int s       = free_sectors[i];
        auto& rec   = img.record(s);
        rec.flag    = 1;
        rec.rec_num = static_cast<uint8_t>(i);
        std::copy(mdr_name.begin(), mdr_name.end(), rec.filename);

        std::size_t chunk = std::min(static_cast<std::size_t>(DATA_SIZE),
                                     payload.size() - offset);
        rec.length = static_cast<uint16_t>(chunk);
        std::memcpy(img.sector_data(s), payload.data() + offset, chunk);
        if (chunk < DATA_SIZE)
            std::memset(img.sector_data(s) + chunk, 0, DATA_SIZE - chunk);
        offset += chunk;

        img.fix_checksums(s);
    }

    img.save(mdr_path);
    std::cout << std::format("put: {} -> \"{}\" ({} bytes, {} sector(s))\n",
        host_file.string(), mdr_name_str, payload.size(), needed);
}

void cmd_get(const fs::path& mdr_path, std::string_view mdr_name, const fs::path& out_file) {
    mdr_image img(mdr_path);

    // rec_num -> sector index
    std::map<int, int> recs;
    for (int s = 0; s < NUM_SECTORS; s++) {
        if (img.is_free(s)) continue;
        if (name_match(img.record(s).filename, mdr_name))
            recs[img.record(s).rec_num] = s;
    }

    if (recs.empty())
        throw std::runtime_error(std::format("'{}' not found on cartridge", mdr_name));

    std::vector<uint8_t> payload;
    for (const auto& [rec_num, s] : recs) {
        const uint8_t* d = img.sector_data(s);
        payload.insert(payload.end(), d, d + img.record(s).length);
    }

    {
        std::ofstream ofs(out_file, std::ios::binary);
        if (!ofs) throw std::runtime_error(std::format("cannot write: {}", out_file.string()));
        ofs.write(reinterpret_cast<const char*>(payload.data()),
                  static_cast<std::streamsize>(payload.size()));
    }

    std::cout << std::format("get: \"{}\" -> {} ({} bytes, {} sector(s))\n",
        mdr_name, out_file.string(), payload.size(), recs.size());
}

void cmd_del(const fs::path& mdr_path, std::string_view mdr_name) {
    mdr_image img(mdr_path);

    int deleted = 0;
    for (int s = 0; s < NUM_SECTORS; s++) {
        if (img.is_free(s)) continue;
        if (!name_match(img.record(s).filename, mdr_name)) continue;

        auto& rec = img.record(s);
        rec.flag    = 0;
        rec.rec_num = 0;
        rec.length  = 0;
        std::fill(rec.filename, rec.filename + NAME_LEN, ' ');
        std::memset(img.sector_data(s), 0, DATA_SIZE);
        img.fix_checksums(s);
        deleted++;
    }

    if (deleted == 0)
        throw std::runtime_error(std::format("'{}' not found on cartridge", mdr_name));

    img.save(mdr_path);
    std::cout << std::format("del: \"{}\" ({} sector(s) freed)\n", mdr_name, deleted);
}

// ---------------------------------------------------------------------------
// Entry point.
// ---------------------------------------------------------------------------
int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "usage:\n"
                  << "  mdr create <mdr_file> <cart_name>\n"
                  << "  mdr put    <mdr_file> <host_file>\n"
                  << "  mdr get    <mdr_file> <mdr_name> <out_file>\n"
                  << "  mdr del    <mdr_file> <mdr_name>\n"
                  << "  mdr dir    <mdr_file>\n";
        return 1;
    }

    const std::string_view cmd      { argv[1] };
    const fs::path         mdr_path { argv[2] };

    try {
        if (cmd == "create") {
            if (argc < 4) { std::cerr << "create requires <cart_name>\n"; return 1; }
            cmd_create(mdr_path, argv[3]);
        } else if (cmd == "put") {
            if (argc < 4) { std::cerr << "put requires <host_file>\n"; return 1; }
            cmd_put(mdr_path, fs::path { argv[3] });
        } else if (cmd == "get") {
            if (argc < 5) { std::cerr << "get requires <mdr_name> <out_file>\n"; return 1; }
            cmd_get(mdr_path, argv[3], fs::path { argv[4] });
        } else if (cmd == "del") {
            if (argc < 4) { std::cerr << "del requires <mdr_name>\n"; return 1; }
            cmd_del(mdr_path, argv[3]);
        } else if (cmd == "dir") {
            cmd_dir(mdr_path);
        } else {
            std::cerr << std::format("unknown command: {}\n", cmd);
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << std::format("error: {}\n", e.what());
        return 2;
    }
}
