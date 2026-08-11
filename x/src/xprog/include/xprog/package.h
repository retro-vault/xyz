#ifndef XPROG_PACKAGE_H
#define XPROG_PACKAGE_H

#include <cstdint>
#include <filesystem>
#include <iosfwd>
#include <string>
#include <vector>

#include <xprog/cli.h>

namespace xprog {

struct xl_info {
    std::uint8_t version = 0;
    std::uint8_t flags = 0;
    std::uint16_t entry_point = 0;
    std::uint16_t code_size = 0;
    std::uint16_t relocation_count = 0;
    std::size_t code_offset = 0;
};

struct image_info {
    image_kind kind = image_kind::process;
    std::uint8_t abi_version = 0;
    std::uint8_t flags = 0;
    std::uint16_t metadata_size = 0;
    std::uint32_t payload_size = 0;
    std::uint32_t payload_crc32 = 0;
    std::uint32_t image_id = 0;
    std::uint16_t preferred_load_address = 0;
    std::uint16_t entry_point = no_entry;
    std::uint16_t stack_size = 0;
    std::uint16_t minimum_os_version = 0;
    std::string name;
    std::vector<std::uint16_t> exports;
    std::vector<std::uint8_t> payload;
    xl_info xl;
};

xl_info parse_xl(const std::vector<std::uint8_t>& bytes);
std::vector<std::uint8_t> build_image(
    const cli_options& options,
    const std::vector<std::uint8_t>& xl_bytes);
image_info parse_image(const std::vector<std::uint8_t>& bytes);
std::vector<std::uint8_t> read_file(const std::filesystem::path& path);
void write_file(const std::filesystem::path& path,
                const std::vector<std::uint8_t>& bytes);
std::uint32_t name_id(const std::string& name);
void inspect_image(const image_info& image, std::ostream& out);
void run(const cli_options& options, std::ostream& out);

} // namespace xprog

#endif
