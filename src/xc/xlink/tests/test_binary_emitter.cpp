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

#include <xlink/binary_emitter.hpp>

TEST(binary_emitter_bin_range_with_padding) {
    xlink::link_context ctx;
    ctx.format = xlink::output_format::bin;
    ctx.output_range = xlink::address_range{0x0000, 0x0005};
    ctx.code_buffer = {0x11, 0x22, 0x33};

    char tmp_template[] = "/tmp/xlink-bin-test-XXXXXX";
    int fd = mkstemp(tmp_template);
    ASSERT(fd >= 0);
    close(fd);
    std::filesystem::path out_path = tmp_template;

    xlink::binary_emitter::emit(out_path, ctx);

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
