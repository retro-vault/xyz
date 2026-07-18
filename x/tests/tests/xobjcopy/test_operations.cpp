//
// xobjcopy operation tests
//
// MIT License (see: LICENSE)
//
#include <filesystem>
#include <fstream>
#include <string>

#include <xbfd/xbfd.h>

#include <xobjcopy/operations.h>

namespace {

    struct xobjcopy_temp_dir {
        std::filesystem::path path;

        xobjcopy_temp_dir() {
            path = std::filesystem::temp_directory_path()
                 / ("xobjcopy_test_" + std::to_string(std::rand()));
            std::filesystem::create_directories(path);
        }

        ~xobjcopy_temp_dir() {
            std::error_code ec;
            std::filesystem::remove_all(path, ec);
        }
    };

    static void write_text(const std::filesystem::path& path,
                           const std::string& text)
    {
        std::ofstream out(path, std::ios::binary);
        out << text;
    }

    static void write_simple_elf_object(const std::filesystem::path& path,
                                        bool with_debug_section)
    {
        auto obj = bfd::bfd::open_w(path, bfd::flavour::elf);
        obj->set_module_name("sample");
        auto& text = obj->add_section(
            ".text",
            bfd::section_flags::alloc
                | bfd::section_flags::load
                | bfd::section_flags::code,
            0);
        text.data = {0xC9};
        text.size = 1;
        obj->add_symbol("_main",
                        bfd::symbol_flags::global | bfd::symbol_flags::function,
                        0,
                        ".text");
        if (with_debug_section) {
            auto& dbg = obj->add_section(
                ".debug_info",
                bfd::section_flags::debugging,
                0);
            dbg.data = {1, 2, 3, 4};
            dbg.size = 4;
        }
        obj->close();
    }

    static void write_simple_elf_executable(const std::filesystem::path& path,
                                            bool with_debug_section)
    {
        auto obj = bfd::bfd::open_w(path, bfd::flavour::elf);
        obj->object().format = xbfd::obj_format::executable;
        obj->object().entry = 0x0100;
        obj->set_module_name("linked");
        auto& text = obj->add_section(
            ".text",
            bfd::section_flags::alloc
                | bfd::section_flags::load
                | bfd::section_flags::code,
            0x0100);
        text.data = {0xC9};
        text.size = 1;
        obj->add_symbol("_start",
                        bfd::symbol_flags::global
                            | bfd::symbol_flags::function,
                        0x0100,
                        ".text");
        obj->add_symbol("_weak_hook",
                        bfd::symbol_flags::global
                            | bfd::symbol_flags::weak
                            | bfd::symbol_flags::function,
                        0x0100,
                        ".text");
        if (with_debug_section) {
            auto& dbg = obj->add_section(
                ".debug_info",
                bfd::section_flags::debugging,
                0);
            dbg.data = {1, 2, 3, 4};
            dbg.size = 4;
        }
        obj->close();
    }

} // namespace

TEST(operations_convert_rel_to_elf_object) {
    xobjcopy_temp_dir temp;
    const auto in = temp.path / "sample.rel";
    const auto out = temp.path / "sample.o";

    write_text(in,
        "XL2\n"
        "H 1 areas 2 global symbols\n"
        "M sample\n"
        "A _CODE size 0001 flags 00\n"
        "S _main Def0000\n"
        "S _helper Ref0000\n"
        "T 00 00 C9\n"
        "R 00 00 00 00\n");

    xobjcopy::cli_options opts;
    opts.input_file = in;
    opts.output_file = out;
    opts.input_target = xobjcopy::target_kind::rel;
    opts.output_target = xobjcopy::target_kind::elf;
    xobjcopy::run(opts);

    auto obj = bfd::bfd::open_r(out);
    ASSERT(obj->check_format(bfd::format::object));
    ASSERT_EQ(static_cast<int>(obj->get_flavour()),
              static_cast<int>(bfd::flavour::elf));
    auto* text = obj->find_section("_CODE");
    ASSERT(text != nullptr);
    ASSERT_EQ(text->size, 1u);
    ASSERT_EQ(text->data[0], 0xC9);
}

TEST(operations_strip_debug_removes_debug_sections_from_elf) {
    xobjcopy_temp_dir temp;
    const auto in = temp.path / "debug.o";
    const auto out = temp.path / "stripped.o";

    write_simple_elf_object(in, true);

    xobjcopy::cli_options opts;
    opts.input_file = in;
    opts.output_file = out;
    opts.input_target = xobjcopy::target_kind::elf;
    opts.output_target = xobjcopy::target_kind::elf;
    opts.strip_debug = true;
    xobjcopy::run(opts);

    auto obj = bfd::bfd::open_r(out);
    ASSERT(obj->check_format(bfd::format::object));
    ASSERT(obj->find_section(".text") != nullptr);
    ASSERT(obj->find_section(".debug_info") == nullptr);
}

TEST(operations_copy_preserves_elf_executable_entry_and_weak_symbols) {
    xobjcopy_temp_dir temp;
    const auto in = temp.path / "linked.elf";
    const auto out = temp.path / "copied.elf";

    write_simple_elf_executable(in, false);

    xobjcopy::cli_options opts;
    opts.input_file = in;
    opts.output_file = out;
    opts.input_target = xobjcopy::target_kind::elf;
    opts.output_target = xobjcopy::target_kind::elf;
    xobjcopy::run(opts);

    auto obj = bfd::bfd::open_r(out);
    ASSERT(obj->check_format(bfd::format::executable));
    ASSERT_EQ(obj->object().entry, 0x0100u);

    bool saw_weak = false;
    for (const auto& sym : obj->symbols()) {
        if (sym.name == "_weak_hook") {
            saw_weak = true;
            ASSERT(sym.is_global());
            ASSERT(sym.is_weak());
        }
    }
    ASSERT(saw_weak);
}

TEST(operations_strip_debug_preserves_elf_executable_format) {
    xobjcopy_temp_dir temp;
    const auto in = temp.path / "debug-linked.elf";
    const auto out = temp.path / "stripped-linked.elf";

    write_simple_elf_executable(in, true);

    xobjcopy::cli_options opts;
    opts.input_file = in;
    opts.output_file = out;
    opts.input_target = xobjcopy::target_kind::elf;
    opts.output_target = xobjcopy::target_kind::elf;
    opts.strip_debug = true;
    xobjcopy::run(opts);

    auto obj = bfd::bfd::open_r(out);
    ASSERT(obj->check_format(bfd::format::executable));
    ASSERT_EQ(obj->object().entry, 0x0100u);
    ASSERT(obj->find_section(".text") != nullptr);
    ASSERT(obj->find_section(".debug_info") == nullptr);
}

TEST(operations_convert_text_library_to_ar_archive) {
    xobjcopy_temp_dir temp;
    const auto member = temp.path / "member.rel";
    const auto in = temp.path / "sample.lib";
    const auto out = temp.path / "sample.a";

    write_text(member,
        "XL2\n"
        "H 1 areas 1 global symbols\n"
        "M member\n"
        "A _CODE size 0001 flags 00\n"
        "S _member Def0000\n"
        "T 00 00 C9\n"
        "R 00 00 00 00\n");
    write_text(in, "# library\nmember.rel\n");

    xobjcopy::cli_options opts;
    opts.input_file = in;
    opts.output_file = out;
    opts.input_target = xobjcopy::target_kind::ar_text;
    opts.output_target = xobjcopy::target_kind::ar_binary;
    xobjcopy::run(opts);

    auto obj = bfd::bfd::open_r(out);
    ASSERT(obj->check_format(bfd::format::archive));
    ASSERT_EQ(static_cast<int>(obj->get_flavour()),
              static_cast<int>(bfd::flavour::ar_binary));
    ASSERT_EQ(obj->members().size(), 1u);
    ASSERT_EQ(obj->members()[0].name, "member.rel");
}
