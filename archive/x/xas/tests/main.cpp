#include <gtest/gtest.h> // <-- Required for ::testing::InitGoogleTest and RUN_ALL_TESTS
#include <filesystem>
std::filesystem::path g_test_binary_dir;

int main(int argc, char **argv)
{
    g_test_binary_dir = std::filesystem::path(argv[0]).parent_path();
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
