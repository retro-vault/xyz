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

#include <xld/runtime.h>
#include <xld/errors.h>

static std::filesystem::path make_temp_dir(const char* pattern) {
    char dir_template[64];
    std::snprintf(dir_template, sizeof(dir_template), "%s", pattern);
    char* dir = mkdtemp(dir_template);
    ASSERT(dir != nullptr);
    return std::filesystem::path(dir);
}

TEST(runtime_apply_sdcc_runtime_noop_without_option) {
    xld::cli_options opts;
    opts.input_files = {"main.rel"};

    xld::runtime::apply_sdcc_runtime(opts);

    ASSERT_EQ(static_cast<int>(opts.input_files.size()), 1);
    ASSERT_EQ(opts.input_files[0], std::filesystem::path("main.rel"));
}

TEST(runtime_apply_sdcc_runtime_respects_disable_default_runtime) {
    xld::cli_options opts;
    opts.disable_default_sdcc_runtime = true;
    opts.input_files = {"main.rel"};

    xld::runtime::apply_sdcc_runtime(opts);

    ASSERT_EQ(static_cast<int>(opts.input_files.size()), 1);
    ASSERT_EQ(opts.input_files[0], std::filesystem::path("main.rel"));
}

TEST(runtime_apply_sdcc_runtime_noop_in_gnu_mode) {
    auto dir = make_temp_dir("/tmp/xld-runtime-XXXXXX");
    auto crt0 = dir / "crt0.rel";
    auto lib = dir / "libruntime.a";

    {
        std::ofstream(crt0) << "XL\nM crt0\n";
        std::ofstream(lib) << "# runtime library index\nhelper.rel\n";
    }

    xld::cli_options opts;
    opts.mode = xld::link_mode::gnu;
    opts.sdcc_runtime_dir = dir;
    opts.input_files = {"main.o"};

    xld::runtime::apply_sdcc_runtime(opts);

    ASSERT_EQ(static_cast<int>(opts.input_files.size()), 1);
    ASSERT_EQ(opts.input_files[0], std::filesystem::path("main.o"));

    std::filesystem::remove_all(dir);
}

TEST(runtime_apply_sdcc_runtime_injects_crt0_and_lib) {
    auto dir = make_temp_dir("/tmp/xld-runtime-XXXXXX");
    auto crt0 = dir / "crt0.rel";
    auto lib = dir / "libruntime.a";

    {
        std::ofstream(crt0) << "XL\nM crt0\n";
        std::ofstream(lib) << "# runtime library index\nhelper.rel\n";
    }

    xld::cli_options opts;
    opts.sdcc_runtime_dir = dir;
    opts.input_files = {"main.rel"};

    xld::runtime::apply_sdcc_runtime(opts);

    ASSERT_EQ(static_cast<int>(opts.input_files.size()), 3);
    ASSERT_EQ(opts.input_files[0], crt0);
    ASSERT_EQ(opts.input_files[1], std::filesystem::path("main.rel"));
    ASSERT_EQ(opts.input_files[2], lib);

    std::filesystem::remove_all(dir);
}

TEST(runtime_apply_sdcc_runtime_injects_optional_libc) {
    auto dir = make_temp_dir("/tmp/xld-runtime-XXXXXX");
    auto crt0 = dir / "crt0.rel";
    auto libc = dir / "libc.a";
    auto lib = dir / "libruntime.a";

    {
        std::ofstream(crt0) << "XL\nM crt0\n";
        std::ofstream(libc) << "!<arch>\n";
        std::ofstream(lib) << "# runtime library index\nhelper.rel\n";
    }

    xld::cli_options opts;
    opts.sdcc_runtime_dir = dir;
    opts.input_files = {"main.rel"};

    xld::runtime::apply_sdcc_runtime(opts);

    ASSERT_EQ(static_cast<int>(opts.input_files.size()), 4);
    ASSERT_EQ(opts.input_files[0], crt0);
    ASSERT_EQ(opts.input_files[1], std::filesystem::path("main.rel"));
    ASSERT_EQ(opts.input_files[2], libc);
    ASSERT_EQ(opts.input_files[3], lib);

    std::filesystem::remove_all(dir);
}

TEST(runtime_apply_sdcc_runtime_injects_optional_platform_lib) {
    auto dir = make_temp_dir("/tmp/xld-runtime-XXXXXX");
    auto crt0 = dir / "crt0.rel";
    auto platform = dir / "platform.lib";
    auto libc = dir / "libc.a";
    auto lib = dir / "libruntime.a";

    {
        std::ofstream(crt0) << "XL\nM crt0\n";
        std::ofstream(platform) << "# platform library index\nsys_putchar.rel\n";
        std::ofstream(libc) << "!<arch>\n";
        std::ofstream(lib) << "# runtime library index\nhelper.rel\n";
    }

    xld::cli_options opts;
    opts.sdcc_runtime_dir = dir;
    opts.input_files = {"main.rel"};

    xld::runtime::apply_sdcc_runtime(opts);

    ASSERT_EQ(static_cast<int>(opts.input_files.size()), 5);
    ASSERT_EQ(opts.input_files[0], crt0);
    ASSERT_EQ(opts.input_files[1], std::filesystem::path("main.rel"));
    ASSERT_EQ(opts.input_files[2], platform);
    ASSERT_EQ(opts.input_files[3], libc);
    ASSERT_EQ(opts.input_files[4], lib);

    std::filesystem::remove_all(dir);
}

TEST(runtime_apply_sdcc_runtime_requires_runtime_dir_to_exist) {
    auto missing = make_temp_dir("/tmp/xld-missing-runtime-XXXXXX");
    std::filesystem::remove_all(missing);

    xld::cli_options opts;
    opts.sdcc_runtime_dir = missing;
    opts.input_files = {"main.rel"};

    ASSERT_THROWS(xld::runtime::apply_sdcc_runtime(opts), xld::xld_error);
}

TEST(runtime_apply_sdcc_runtime_honors_nostartfiles) {
    auto dir = make_temp_dir("/tmp/xld-runtime-XXXXXX");
    auto crt0 = dir / "crt0.rel";
    auto lib = dir / "libruntime.a";

    {
        std::ofstream(crt0) << "XL\nM crt0\n";
        std::ofstream(lib) << "# runtime library index\nhelper.rel\n";
    }

    xld::cli_options opts;
    opts.sdcc_runtime_dir = dir;
    opts.no_startfiles = true;
    opts.input_files = {"main.rel"};

    xld::runtime::apply_sdcc_runtime(opts);

    ASSERT_EQ(static_cast<int>(opts.input_files.size()), 2);
    ASSERT_EQ(opts.input_files[0], std::filesystem::path("main.rel"));
    ASSERT_EQ(opts.input_files[1], lib);

    std::filesystem::remove_all(dir);
}

TEST(runtime_apply_sdcc_runtime_honors_nostartfiles_with_optional_libc) {
    auto dir = make_temp_dir("/tmp/xld-runtime-XXXXXX");
    auto crt0 = dir / "crt0.rel";
    auto libc = dir / "libc.a";
    auto lib = dir / "libruntime.a";

    {
        std::ofstream(crt0) << "XL\nM crt0\n";
        std::ofstream(libc) << "!<arch>\n";
        std::ofstream(lib) << "# runtime library index\nhelper.rel\n";
    }

    xld::cli_options opts;
    opts.sdcc_runtime_dir = dir;
    opts.no_startfiles = true;
    opts.input_files = {"main.rel"};

    xld::runtime::apply_sdcc_runtime(opts);

    ASSERT_EQ(static_cast<int>(opts.input_files.size()), 3);
    ASSERT_EQ(opts.input_files[0], std::filesystem::path("main.rel"));
    ASSERT_EQ(opts.input_files[1], libc);
    ASSERT_EQ(opts.input_files[2], lib);

    std::filesystem::remove_all(dir);
}

TEST(runtime_apply_sdcc_runtime_honors_nostdlib) {
    auto dir = make_temp_dir("/tmp/xld-runtime-XXXXXX");
    auto crt0 = dir / "crt0.rel";
    auto lib = dir / "libruntime.a";

    {
        std::ofstream(crt0) << "XL\nM crt0\n";
        std::ofstream(lib) << "# runtime library index\nhelper.rel\n";
    }

    xld::cli_options opts;
    opts.sdcc_runtime_dir = dir;
    opts.no_stdlib = true;
    opts.input_files = {"main.rel"};

    xld::runtime::apply_sdcc_runtime(opts);

    ASSERT_EQ(static_cast<int>(opts.input_files.size()), 1);
    ASSERT_EQ(opts.input_files[0], std::filesystem::path("main.rel"));

    std::filesystem::remove_all(dir);
}

TEST(runtime_find_default_sdcc_runtime_dir_prefers_prefix_lib) {
    auto prefix = make_temp_dir("/tmp/xld-prefix-XXXXXX");
    auto bindir = prefix / "bin";
    auto libdir = prefix / "lib";
    std::filesystem::create_directories(bindir);
    std::filesystem::create_directories(libdir);
    std::ofstream(libdir / "crt0.rel") << "XL\nM crt0\n";
    std::ofstream(libdir / "libruntime.a") << "!<arch>\n";

    xld::cli_options opts;
    auto detected = xld::runtime::find_default_sdcc_runtime_dir(
        opts, bindir / "xld");

    ASSERT(detected.has_value());
    ASSERT_EQ(*detected, libdir);

    std::filesystem::remove_all(prefix);
}

TEST(runtime_find_default_sdcc_runtime_dir_prefers_target_z80_lib) {
    auto prefix = make_temp_dir("/tmp/xld-prefix-XXXXXX");
    auto bindir = prefix / "bin";
    auto hostlib = prefix / "lib";
    auto targetlib = prefix / "z80" / "lib";
    std::filesystem::create_directories(bindir);
    std::filesystem::create_directories(hostlib);
    std::filesystem::create_directories(targetlib);
    std::ofstream(hostlib / "crt0.rel") << "XL\nM crt0\n";
    std::ofstream(hostlib / "libruntime.a") << "!<arch>\n";
    std::ofstream(targetlib / "crt0.rel") << "XL\nM crt0\n";
    std::ofstream(targetlib / "libruntime.a") << "!<arch>\n";

    xld::cli_options opts;
    auto detected = xld::runtime::find_default_sdcc_runtime_dir(
        opts, bindir / "xld");

    ASSERT(detected.has_value());
    ASSERT_EQ(*detected, targetlib);

    std::filesystem::remove_all(prefix);
}

TEST(runtime_find_default_sdcc_runtime_dir_falls_back_to_compat_runtime_dir) {
    auto prefix = make_temp_dir("/tmp/xld-prefix-XXXXXX");
    auto bindir = prefix / "bin";
    auto compat = prefix / "libexec" / "xcc" / "runtime";
    std::filesystem::create_directories(bindir);
    std::filesystem::create_directories(compat);
    std::ofstream(compat / "crt0.rel") << "XL\nM crt0\n";
    std::ofstream(compat / "libruntime.a") << "!<arch>\n";

    xld::cli_options opts;
    auto detected = xld::runtime::find_default_sdcc_runtime_dir(
        opts, bindir / "xld");

    ASSERT(detected.has_value());
    ASSERT_EQ(*detected, compat);

    std::filesystem::remove_all(prefix);
}

TEST(runtime_find_default_sdcc_runtime_dir_prefers_invocation_target_runtime) {
    auto prefix = make_temp_dir("/tmp/xld-prefix-XXXXXX");
    auto bindir = prefix / "bin";
    auto libdir = prefix / "lib";
    auto targetdir = prefix / "targets" / "z80-cpm3" / "lib";
    std::filesystem::create_directories(bindir);
    std::filesystem::create_directories(libdir);
    std::filesystem::create_directories(targetdir);
    std::ofstream(targetdir / "crt0.rel") << "XL\nM cpm3_crt0\n";
    std::ofstream(libdir / "libruntime.a") << "!<arch>\n";

    xld::cli_options opts;
    opts.invocation_target = "z80-cpm3";
    auto detected = xld::runtime::find_default_sdcc_runtime_dir(
        opts, bindir / "z80-cpm3-xld");

    ASSERT(detected.has_value());
    ASSERT_EQ(*detected, targetdir);

    std::filesystem::remove_all(prefix);
}

TEST(runtime_find_default_sdcc_runtime_dir_prefers_z80_none_target_runtime) {
    auto prefix = make_temp_dir("/tmp/xld-prefix-XXXXXX");
    auto bindir = prefix / "bin";
    auto libdir = prefix / "lib";
    auto targetdir = prefix / "targets" / "z80-none" / "lib";
    std::filesystem::create_directories(bindir);
    std::filesystem::create_directories(libdir);
    std::filesystem::create_directories(targetdir);
    std::ofstream(targetdir / "crt0.rel") << "XL\nM none_crt0\n";
    std::ofstream(libdir / "libruntime.a") << "!<arch>\n";

    xld::cli_options opts;
    opts.invocation_target = "z80-none";
    auto detected = xld::runtime::find_default_sdcc_runtime_dir(
        opts, bindir / "z80-none-xld");

    ASSERT(detected.has_value());
    ASSERT_EQ(*detected, targetdir);

    std::filesystem::remove_all(prefix);
}

TEST(runtime_find_default_sdcc_runtime_dir_honors_explicit_platform_name) {
    auto prefix = make_temp_dir("/tmp/xld-prefix-XXXXXX");
    auto bindir = prefix / "bin";
    auto libdir = prefix / "lib";
    auto targetdir = prefix / "targets" / "z80-none" / "lib";
    std::filesystem::create_directories(bindir);
    std::filesystem::create_directories(libdir);
    std::filesystem::create_directories(targetdir);
    std::ofstream(targetdir / "crt0.rel") << "XL\nM explicit_none_crt0\n";
    std::ofstream(libdir / "libruntime.a") << "!<arch>\n";

    xld::cli_options opts;
    opts.platform_name = std::string("z80-none");
    auto detected = xld::runtime::find_default_sdcc_runtime_dir(
        opts, bindir / "xld");

    ASSERT(detected.has_value());
    ASSERT_EQ(*detected, targetdir);

    std::filesystem::remove_all(prefix);
}

TEST(runtime_apply_sdcc_runtime_uses_common_libs_with_target_runtime_dir) {
    auto prefix = make_temp_dir("/tmp/xld-prefix-XXXXXX");
    auto libdir = prefix / "lib";
    auto targetdir = prefix / "targets" / "z80-cpm3" / "lib";
    auto crt0 = targetdir / "crt0.rel";
    auto platform = targetdir / "platform.lib";
    auto libc = libdir / "libc.a";
    auto lib = libdir / "libruntime.a";

    std::filesystem::create_directories(libdir);
    std::filesystem::create_directories(targetdir);

    {
        std::ofstream(crt0) << "XL\nM cpm3_crt0\n";
        std::ofstream(platform) << "# cpm3 platform library index\nsys_putchar.rel\n";
        std::ofstream(libc) << "!<arch>\n";
        std::ofstream(lib) << "# runtime library index\nhelper.rel\n";
    }

    xld::cli_options opts;
    opts.sdcc_runtime_dir = targetdir;
    opts.input_files = {"main.rel"};

    xld::runtime::apply_sdcc_runtime(opts);

    ASSERT_EQ(static_cast<int>(opts.input_files.size()), 5);
    ASSERT_EQ(opts.input_files[0], crt0);
    ASSERT_EQ(opts.input_files[1], std::filesystem::path("main.rel"));
    ASSERT_EQ(opts.input_files[2], platform);
    ASSERT_EQ(opts.input_files[3], libc);
    ASSERT_EQ(opts.input_files[4], lib);

    std::filesystem::remove_all(prefix);
}
