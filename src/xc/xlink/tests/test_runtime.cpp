//
// runtime helper tests
//
// MIT License (see: LICENSE)
// copyright (C) 2021 tomaz stih
//
#include <filesystem>
#include <fstream>
#include <cstdio>
#include <string>
#include <unistd.h>

#include <xlink/runtime.hpp>
#include <xlink/errors.hpp>

static std::filesystem::path make_temp_dir(const char* pattern) {
    char dir_template[64];
    std::snprintf(dir_template, sizeof(dir_template), "%s", pattern);
    char* dir = mkdtemp(dir_template);
    ASSERT(dir != nullptr);
    return std::filesystem::path(dir);
}

TEST(runtime_apply_sdcc_runtime_noop_without_option) {
    xlink::cli_options opts;
    opts.input_files = {"main.rel"};

    xlink::runtime::apply_sdcc_runtime(opts);

    ASSERT_EQ(static_cast<int>(opts.input_files.size()), 1);
    ASSERT_EQ(opts.input_files[0], std::filesystem::path("main.rel"));
}

TEST(runtime_apply_sdcc_runtime_injects_crt0_and_lib) {
    auto dir = make_temp_dir("/tmp/xlink-runtime-XXXXXX");
    auto crt0 = dir / "crt0.rel";
    auto lib = dir / "z80.lib";

    {
        std::ofstream(crt0) << "XL\nM crt0\n";
        std::ofstream(lib) << "# runtime library index\nhelper.rel\n";
    }

    xlink::cli_options opts;
    opts.sdcc_runtime_dir = dir;
    opts.input_files = {"main.rel"};

    xlink::runtime::apply_sdcc_runtime(opts);

    ASSERT_EQ(static_cast<int>(opts.input_files.size()), 3);
    ASSERT_EQ(opts.input_files[0], crt0);
    ASSERT_EQ(opts.input_files[1], std::filesystem::path("main.rel"));
    ASSERT_EQ(opts.input_files[2], lib);

    std::filesystem::remove_all(dir);
}

TEST(runtime_apply_sdcc_runtime_requires_runtime_dir_to_exist) {
    auto missing = make_temp_dir("/tmp/xlink-missing-runtime-XXXXXX");
    std::filesystem::remove_all(missing);

    xlink::cli_options opts;
    opts.sdcc_runtime_dir = missing;
    opts.input_files = {"main.rel"};

    ASSERT_THROWS(xlink::runtime::apply_sdcc_runtime(opts), xlink::xlink_error);
}

TEST(runtime_apply_sdcc_runtime_honors_nostartfiles) {
    auto dir = make_temp_dir("/tmp/xlink-runtime-XXXXXX");
    auto crt0 = dir / "crt0.rel";
    auto lib = dir / "z80.lib";

    {
        std::ofstream(crt0) << "XL\nM crt0\n";
        std::ofstream(lib) << "# runtime library index\nhelper.rel\n";
    }

    xlink::cli_options opts;
    opts.sdcc_runtime_dir = dir;
    opts.no_startfiles = true;
    opts.input_files = {"main.rel"};

    xlink::runtime::apply_sdcc_runtime(opts);

    ASSERT_EQ(static_cast<int>(opts.input_files.size()), 2);
    ASSERT_EQ(opts.input_files[0], std::filesystem::path("main.rel"));
    ASSERT_EQ(opts.input_files[1], lib);

    std::filesystem::remove_all(dir);
}

TEST(runtime_apply_sdcc_runtime_honors_nostdlib) {
    auto dir = make_temp_dir("/tmp/xlink-runtime-XXXXXX");
    auto crt0 = dir / "crt0.rel";
    auto lib = dir / "z80.lib";

    {
        std::ofstream(crt0) << "XL\nM crt0\n";
        std::ofstream(lib) << "# runtime library index\nhelper.rel\n";
    }

    xlink::cli_options opts;
    opts.sdcc_runtime_dir = dir;
    opts.no_stdlib = true;
    opts.input_files = {"main.rel"};

    xlink::runtime::apply_sdcc_runtime(opts);

    ASSERT_EQ(static_cast<int>(opts.input_files.size()), 1);
    ASSERT_EQ(opts.input_files[0], std::filesystem::path("main.rel"));

    std::filesystem::remove_all(dir);
}
