#include <microdrive/microdrive.h>

#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <vector>

namespace fs = std::filesystem;

namespace {

void cmd_create(const fs::path& mdr_path,
                std::string_view cart_name,
                microdrive::compatibility_t compatibility) {
    const int sector_count = microdrive::k_num_sectors;
    auto image = microdrive::image_t::create_blank(cart_name);
    image.save(mdr_path);
    std::cout << std::format("created {} ({} sectors, cart=\"{}\"{})\n",
                             mdr_path.string(),
                             sector_count,
                             microdrive::trim_name(microdrive::pad_name(cart_name).data()),
                             compatibility == microdrive::compatibility_t::fuse
                                 ? ", fuse-compat parser enabled"
                                 : "");
}

void cmd_format(const fs::path& mdr_path,
                std::string_view cart_name,
                microdrive::compatibility_t compatibility) {
    int sector_count = microdrive::k_num_sectors;
    if (fs::exists(mdr_path)) {
        const auto existing = microdrive::image_t::load(mdr_path, compatibility);
        sector_count = existing.block_count();
    }
    auto image = microdrive::image_t::create_blank(cart_name, sector_count);
    image.save(mdr_path);
    std::cout << std::format("formatted {} ({} sectors, cart=\"{}\"{})\n",
                             mdr_path.string(),
                             sector_count,
                             microdrive::trim_name(microdrive::pad_name(cart_name).data()),
                             compatibility == microdrive::compatibility_t::fuse
                                 ? ", fuse-compat parser enabled"
                                 : "");
}

void cmd_dir(const fs::path& mdr_path, microdrive::compatibility_t compatibility) {
    const auto image = microdrive::image_t::load(mdr_path, compatibility);
    const auto files = image.directory();

    if (files.empty()) {
        std::cout << "no files.\n";
        return;
    }

    std::cout << std::format("{:<12} {:>8}  {}\n", "name", "bytes", "sectors");
    std::cout << std::string(30, '-') << "\n";
    for (const auto& file : files) {
        std::cout << std::format("{:<12} {:>8}  {}\n",
                                 file.name, file.bytes, file.sectors);
    }
}

void cmd_put(const fs::path& mdr_path,
             const fs::path& host_file,
             microdrive::compatibility_t compatibility) {
    if (!fs::exists(host_file)) {
        throw std::runtime_error(std::format("file not found: {}", host_file.string()));
    }

    std::vector<uint8_t> payload(fs::file_size(host_file));
    std::ifstream in(host_file, std::ios::binary);
    if (!in) {
        throw std::runtime_error(std::format("cannot read: {}", host_file.string()));
    }
    in.read(reinterpret_cast<char*>(payload.data()),
            static_cast<std::streamsize>(payload.size()));

    auto image = microdrive::image_t::load(mdr_path, compatibility);
    const auto name = host_file.filename().string();
    image.put(name, payload);
    image.save(mdr_path);

    std::cout << std::format("put: {} -> \"{}\" ({} bytes, {} sector(s))\n",
                             host_file.string(),
                             microdrive::trim_name(microdrive::pad_name(name).data()),
                             payload.size(),
                             static_cast<int>((payload.size() + microdrive::k_data_size - 1) /
                                              microdrive::k_data_size));
}

void cmd_get(const fs::path& mdr_path,
             std::string_view mdr_name,
             const fs::path& out_file,
             microdrive::compatibility_t compatibility) {
    const auto image = microdrive::image_t::load(mdr_path, compatibility);
    const auto payload = image.get(mdr_name);

    std::ofstream out(out_file, std::ios::binary);
    if (!out) {
        throw std::runtime_error(std::format("cannot write: {}", out_file.string()));
    }
    out.write(reinterpret_cast<const char*>(payload.data()),
              static_cast<std::streamsize>(payload.size()));

    std::cout << std::format("get: \"{}\" -> {} ({} bytes)\n",
                             mdr_name, out_file.string(), payload.size());
}

void cmd_del(const fs::path& mdr_path,
             std::string_view mdr_name,
             microdrive::compatibility_t compatibility) {
    auto image = microdrive::image_t::load(mdr_path, compatibility);
    const int deleted = image.remove(mdr_name);
    if (deleted == 0) {
        throw std::runtime_error(std::format("'{}' not found on cartridge", mdr_name));
    }
    image.save(mdr_path);
    std::cout << std::format("del: \"{}\" ({} sector(s) freed)\n", mdr_name, deleted);
}

}  // namespace

int main(int argc, char* argv[]) {
    std::vector<std::string_view> args;
    args.reserve(static_cast<std::size_t>(argc > 1 ? argc - 1 : 0));

    microdrive::compatibility_t compatibility = microdrive::compatibility_t::standard;
    for (int i = 1; i < argc; ++i) {
        const std::string_view arg { argv[i] };
        if (arg == "--fuse") {
            compatibility = microdrive::compatibility_t::fuse;
        } else {
            args.push_back(arg);
        }
    }

    if (args.size() < 2) {
        std::cerr << "usage:\n"
                  << "  microdrive [--fuse] create <mdr_file> <cart_name>\n"
                  << "  microdrive [--fuse] format <mdr_file> <cart_name>\n"
                  << "  microdrive [--fuse] put    <mdr_file> <host_file>\n"
                  << "  microdrive [--fuse] get    <mdr_file> <mdr_name> <out_file>\n"
                  << "  microdrive [--fuse] del    <mdr_file> <mdr_name>\n"
                  << "  microdrive [--fuse] dir    <mdr_file>\n";
        return 1;
    }

    const std::string_view cmd { args[0] };
    const fs::path mdr_path { std::string(args[1]) };

    try {
        if (cmd == "create") {
            if (args.size() < 3) {
                std::cerr << "create requires <cart_name>\n";
                return 1;
            }
            cmd_create(mdr_path, args[2], compatibility);
        } else if (cmd == "format") {
            if (args.size() < 3) {
                std::cerr << "format requires <cart_name>\n";
                return 1;
            }
            cmd_format(mdr_path, args[2], compatibility);
        } else if (cmd == "put") {
            if (args.size() < 3) {
                std::cerr << "put requires <host_file>\n";
                return 1;
            }
            cmd_put(mdr_path, fs::path { std::string(args[2]) }, compatibility);
        } else if (cmd == "get") {
            if (args.size() < 4) {
                std::cerr << "get requires <mdr_name> <out_file>\n";
                return 1;
            }
            cmd_get(mdr_path, args[2], fs::path { std::string(args[3]) }, compatibility);
        } else if (cmd == "del") {
            if (args.size() < 3) {
                std::cerr << "del requires <mdr_name>\n";
                return 1;
            }
            cmd_del(mdr_path, args[2], compatibility);
        } else if (cmd == "dir") {
            cmd_dir(mdr_path, compatibility);
        } else {
            std::cerr << std::format("unknown command: {}\n", cmd);
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << std::format("error: {}\n", e.what());
        return 2;
    }

    return 0;
}
