//
// adb_parser unit tests
//
// MIT License (see: LICENSE)
//
#include <filesystem>
#include <fstream>

#include <xld/adb_parser.h>

namespace {

    struct adb_temp_dir {
        std::filesystem::path path;

        adb_temp_dir() {
            path = std::filesystem::temp_directory_path()
                 / ("xlink_adb_test_" + std::to_string(std::rand()));
            std::filesystem::create_directories(path);
        }

        ~adb_temp_dir() {
            std::error_code ec;
            std::filesystem::remove_all(path, ec);
        }
    };

    static void write_adb_text(const std::filesystem::path& path,
                               const std::string& text)
    {
        std::ofstream out(path);
        out << text;
    }

} // namespace

TEST(adb_parser_reads_calling_convention_extension) {
    adb_temp_dir temp;
    auto adb_path = temp.path / "abi_main.adb";

    write_adb_text(adb_path,
        "M:abi_main\n"
        "F:G$helper$0_0$0({2}DF,SI:S),C,0,0,0,0,0,ABI=sdcccall(0)\n"
        "S:G$helper$0_0$0({2}DF,SI:S),C,0,0\n");

    auto doc = xld::adb_parser::parse(adb_path);
    ASSERT_EQ(doc.functions.size(), 1u);
    ASSERT_EQ(doc.functions.front().display_name, "_helper");
    ASSERT_EQ(doc.functions.front().calling_convention,
              xbfd::calling_convention::xcc_sdcccall0);
}
