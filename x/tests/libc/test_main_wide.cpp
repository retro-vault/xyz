// test_main_wide.cpp — focused wide-stdio libc tests.
//
// The monolithic libc harness eventually grows past the Z80's 64K flat
// address space, so wide stdio is verified in its own image with the same
// emulator/runtime machinery.
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih

#include <cstdio>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <span>
#include <vector>

#include "runtime_machine.hpp"
#include "libc_wide_symbols.hpp"

static std::vector<uint8_t> load_file(const char* path)
{
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        std::fprintf(stderr, "error: cannot open %s\n", path);
        return {};
    }
    return { std::istreambuf_iterator<char>(f), {} };
}

int main(int argc, char* argv[])
{
    const char* bin_path = argc > 1 ? argv[1] : "build/libc_wide.bin";
    auto code_image = load_file(bin_path);
    if (code_image.empty()) return 1;

    runtime_machine rt(std::span<const uint8_t>(code_image.data(),
                                                code_image.size()));
    g_rt = &rt;
    if (!rt.call16(rt_sym::stdio_wide_putwchar_case, 0, 0)) {
        std::fprintf(stderr, "error: emulator call failed in stdio_wide_putwchar_case\n");
        return 2;
    }

    if (rt.snap().de != 0) {
        std::fprintf(stderr, "error: stdio_wide_putwchar_case returned %u\n",
                     static_cast<unsigned>(rt.snap().de));
        return 3;
    }

    if (!rt.call16(rt_sym::stdio_wide_fputwc_case, 0, 0)) {
        std::fprintf(stderr, "error: emulator call failed in stdio_wide_fputwc_case\n");
        return 4;
    }

    if (rt.snap().de != 0) {
        std::fprintf(stderr, "error: stdio_wide_fputwc_case returned %u\n",
                     static_cast<unsigned>(rt.snap().de));
        return 5;
    }

    if (!rt.call16(rt_sym::stdio_wide_putwc_case, 0, 0)) {
        std::fprintf(stderr, "error: emulator call failed in stdio_wide_putwc_case\n");
        return 6;
    }

    if (rt.snap().de != 0) {
        std::fprintf(stderr, "error: stdio_wide_putwc_case returned %u\n",
                     static_cast<unsigned>(rt.snap().de));
        return 7;
    }

    if (!rt.call16(rt_sym::stdio_wide_fputws_case, 0, 0)) {
        std::fprintf(stderr, "error: emulator call failed in stdio_wide_fputws_case\n");
        return 8;
    }

    if (rt.snap().de != 0) {
        std::fprintf(stderr, "error: stdio_wide_fputws_case returned %u\n",
                     static_cast<unsigned>(rt.snap().de));
        return 9;
    }

    if (!rt.call16(rt_sym::stdio_wide_input_cases, 0, 0)) {
        std::fprintf(stderr, "error: emulator call failed in stdio_wide_input_cases\n");
        return 10;
    }

    if (rt.snap().de != 0) {
        std::fprintf(stderr, "error: stdio_wide_input_cases returned %u\n",
                     static_cast<unsigned>(rt.snap().de));
        return 11;
    }

    if (!rt.call16(rt_sym::stdio_wide_file_open_case, 0, 0)) {
        std::fprintf(stderr, "error: emulator call failed in stdio_wide_file_open_case\n");
        return 12;
    }

    if (rt.snap().de != 0) {
        std::fprintf(stderr, "error: stdio_wide_file_open_case returned %u\n",
                     static_cast<unsigned>(rt.snap().de));
        return 13;
    }

    if (!rt.call16(rt_sym::stdio_wide_file_write_case, 0, 0)) {
        std::fprintf(stderr, "error: emulator call failed in stdio_wide_file_write_case\n");
        return 14;
    }

    if (rt.snap().de != 0) {
        std::fprintf(stderr, "error: stdio_wide_file_write_case returned %u\n",
                     static_cast<unsigned>(rt.snap().de));
        return 15;
    }

    if (!rt.call16(rt_sym::stdio_wide_file_read_case, 0, 0)) {
        std::fprintf(stderr, "error: emulator call failed in stdio_wide_file_read_case\n");
        return 16;
    }

    if (rt.snap().de != 0) {
        std::fprintf(stderr, "error: stdio_wide_file_read_case returned %u\n",
                     static_cast<unsigned>(rt.snap().de));
        return 17;
    }

    return 0;
}
