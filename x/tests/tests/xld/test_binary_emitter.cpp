// test_binary_emitter.cpp
//
// binary_emitter unit tests
//
// MIT License (see: LICENSE)
// copyright (C) 2021 tomaz stih
//
// 2021-07-28   tstih
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <unistd.h>
#include <vector>

#include <xld/binary_emitter.h>

TEST(binary_emitter_bin_range_with_padding) {
    xld::link_context ctx;
    ctx.format = xld::output_format::bin;
    ctx.output_range = xld::address_range{0x0000, 0x0005};
    ctx.code_buffer = {0x11, 0x22, 0x33};

    char tmp_template[] = "/tmp/xld-bin-test-XXXXXX";
    int fd = mkstemp(tmp_template);
    ASSERT(fd >= 0);
    close(fd);
    std::filesystem::path out_path = tmp_template;

    xld::binary_emitter::emit(out_path, ctx);

    std::ifstream in(out_path, std::ios::binary);
    ASSERT(in.is_open());
    std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(in)),
                               std::istreambuf_iterator<char>());
    ASSERT_EQ(bytes.size(), 6);
    ASSERT_EQ(bytes[0], 0x11);
    ASSERT_EQ(bytes[1], 0x22);
    ASSERT_EQ(bytes[2], 0x33);
    ASSERT_EQ(bytes[3], 0x00);
    ASSERT_EQ(bytes[4], 0x00);
    ASSERT_EQ(bytes[5], 0x00);

    std::filesystem::remove(out_path);
}

TEST(binary_emitter_bin_pre_hole_jr_and_zero_filled_hole) {
    xld::link_context ctx;
    ctx.format = xld::output_format::bin;
    ctx.output_range = xld::address_range{0x00F8, 0x0113};
    ctx.holes.push_back({0x0100, 0x010F});
    ctx.code_buffer.resize(0x0114, 0x00);

    char tmp_template[] = "/tmp/xld-hole-test-XXXXXX";
    int fd = mkstemp(tmp_template);
    ASSERT(fd >= 0);
    close(fd);
    std::filesystem::path out_path = tmp_template;

    xld::binary_emitter::emit(out_path, ctx);

    std::ifstream in(out_path, std::ios::binary);
    ASSERT(in.is_open());
    std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(in)),
                               std::istreambuf_iterator<char>());
    ASSERT_EQ(bytes.size(), 0x1Cu);

    // JR +0x10 at 0x00FE..0x00FF skips 0x0100..0x010F.
    ASSERT_EQ(bytes[0x00FE - 0x00F8], 0x18);
    ASSERT_EQ(bytes[0x00FF - 0x00F8], 0x10);

    // Reserved bytes remain untouched and zero-filled.
    for (uint32_t addr = 0x0100; addr <= 0x010F; ++addr)
        ASSERT_EQ(bytes[addr - 0x00F8], 0x00);

    std::filesystem::remove(out_path);
}

TEST(binary_emitter_bin_skips_pre_hole_jr_when_too_close_to_start) {
    xld::link_context ctx;
    ctx.format = xld::output_format::bin;
    ctx.output_range = xld::address_range{0x0000, 0x0007};
    ctx.holes.push_back({0x0001, 0x0003});
    ctx.code_buffer = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE};

    char tmp_template[] = "/tmp/xld-hole-near-start-test-XXXXXX";
    int fd = mkstemp(tmp_template);
    ASSERT(fd >= 0);
    close(fd);
    std::filesystem::path out_path = tmp_template;

    xld::binary_emitter::emit(out_path, ctx);

    std::ifstream in(out_path, std::ios::binary);
    ASSERT(in.is_open());
    std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(in)),
                               std::istreambuf_iterator<char>());
    ASSERT_EQ(bytes.size(), 8u);

    // No room for a two-byte pre-hole JR, so byte 0 stays untouched.
    ASSERT_EQ(bytes[0], 0xAA);

    // Reserved bytes remain untouched and zero-filled.
    ASSERT_EQ(bytes[1], 0x00);
    ASSERT_EQ(bytes[2], 0x00);
    ASSERT_EQ(bytes[3], 0x00);

    std::filesystem::remove(out_path);
}

TEST(binary_emitter_bin_pre_hole_jp_for_large_hole) {
    xld::link_context ctx;
    ctx.format = xld::output_format::bin;
    ctx.output_range = xld::address_range{0x0078, 0x0105};
    ctx.holes.push_back({0x0080, 0x0100});
    ctx.code_buffer.resize(0x0106, 0xAA);

    char tmp_template[] = "/tmp/xld-large-hole-test-XXXXXX";
    int fd = mkstemp(tmp_template);
    ASSERT(fd >= 0);
    close(fd);
    std::filesystem::path out_path = tmp_template;

    xld::binary_emitter::emit(out_path, ctx);

    std::ifstream in(out_path, std::ios::binary);
    ASSERT(in.is_open());
    std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(in)),
                               std::istreambuf_iterator<char>());

    ASSERT_EQ(bytes[0x007D - 0x0078], 0xC3);
    ASSERT_EQ(bytes[0x007E - 0x0078], 0x01);
    ASSERT_EQ(bytes[0x007F - 0x0078], 0x01);

    for (uint32_t addr = 0x0080; addr <= 0x0100; ++addr)
        ASSERT_EQ(bytes[addr - 0x0078], 0x00);

    std::filesystem::remove(out_path);
}

TEST(binary_emitter_ihx_emits_sparse_hex_image) {
    xld::link_context ctx;
    ctx.format = xld::output_format::ihx;
    ctx.output_range = xld::address_range{0x0000, 0x0005};
    ctx.code_buffer = {0x11, 0x22, 0x33, 0x00, 0x00, 0x00};
    ctx.code_occupancy = {0x01, 0x01, 0x01, 0x00, 0x00, 0x00};

    char tmp_template[] = "/tmp/xld-ihx-test-XXXXXX";
    int fd = mkstemp(tmp_template);
    ASSERT(fd >= 0);
    close(fd);
    std::filesystem::path out_path = tmp_template;

    xld::binary_emitter::emit(out_path, ctx);

    std::ifstream in(out_path);
    ASSERT(in.is_open());
    std::string text((std::istreambuf_iterator<char>(in)),
                     std::istreambuf_iterator<char>());

    ASSERT(text.find(":0300000011223397\n") != std::string::npos);
    ASSERT(text.find(":0600000011223300000094\n") == std::string::npos);
    ASSERT(text.find(":00000001FF\n") != std::string::npos);

    std::filesystem::remove(out_path);
}

TEST(binary_emitter_ihx_keeps_pre_hole_guard_bytes) {
    xld::link_context ctx;
    ctx.format = xld::output_format::ihx;
    ctx.output_range = xld::address_range{0x00F8, 0x0113};
    ctx.holes.push_back({0x0100, 0x010F});
    ctx.code_buffer.resize(0x0114, 0x00);
    ctx.code_occupancy.resize(0x0114, 0x00);

    char tmp_template[] = "/tmp/xld-ihx-hole-test-XXXXXX";
    int fd = mkstemp(tmp_template);
    ASSERT(fd >= 0);
    close(fd);
    std::filesystem::path out_path = tmp_template;

    xld::binary_emitter::emit(out_path, ctx);

    std::ifstream in(out_path);
    ASSERT(in.is_open());
    std::string text((std::istreambuf_iterator<char>(in)),
                     std::istreambuf_iterator<char>());

    ASSERT(text.find(":0200FE001810D8\n") != std::string::npos);
    ASSERT(text.find(":00000001FF\n") != std::string::npos);

    std::filesystem::remove(out_path);
}

TEST(binary_emitter_bin_implicit_window_uses_occupied_bytes_only) {
    xld::link_context ctx;
    ctx.format = xld::output_format::bin;
    ctx.code_buffer.resize(0x0020, 0x00);
    ctx.code_buffer[0x0010] = 0x11;
    ctx.code_buffer[0x0011] = 0x22;
    ctx.code_buffer[0x001A] = 0x33;
    ctx.code_buffer[0x001B] = 0x44;
    ctx.code_occupancy.resize(0x0020, 0x00);
    ctx.code_occupancy[0x0010] = 0x01;
    ctx.code_occupancy[0x0011] = 0x01;
    ctx.code_occupancy[0x001A] = 0x01;
    ctx.code_occupancy[0x001B] = 0x01;

    char tmp_template[] = "/tmp/xld-bin-implicit-window-test-XXXXXX";
    int fd = mkstemp(tmp_template);
    ASSERT(fd >= 0);
    close(fd);
    std::filesystem::path out_path = tmp_template;

    xld::binary_emitter::emit(out_path, ctx);

    std::ifstream in(out_path, std::ios::binary);
    ASSERT(in.is_open());
    std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(in)),
                               std::istreambuf_iterator<char>());
    ASSERT_EQ(bytes.size(), 12u);
    ASSERT_EQ(bytes[0], 0x11);
    ASSERT_EQ(bytes[1], 0x22);
    for (size_t i = 2; i < 10; ++i)
        ASSERT_EQ(bytes[i], 0x00);
    ASSERT_EQ(bytes[10], 0x33);
    ASSERT_EQ(bytes[11], 0x44);

    std::filesystem::remove(out_path);
}
