// mmap_file_test.cpp
#include <fstream>
#include <cstring> // For std::memcmp
#include <cstdio>  // For remove

#include "mmap_file.h"

#include "mmap_file_test.h"

// SetUp method: Initialize test paths and create necessary test files
void mmap_file_test::setup() {
    // Set the paths for the test files
    valid_file_path = "bin/data/test_file.txt";
    empty_file_path = "bin/data/empty_file.txt";
    nonexistent_file_path = "bin/data/nonexistent.txt";

    // Create the test files
    create_test_file(valid_file_path);
    create_empty_file(empty_file_path);
}

// TearDown method: Clean up the test files
void mmap_file_test::teardown() {
    // Remove test files after tests
    std::remove(valid_file_path);
    std::remove(empty_file_path);
}

// Create a sample test file with content
void mmap_file_test::create_test_file(const char* path) {
    std::ofstream file(path);
    file << "Hello, this is a test file.\n"
         << "This file contains multiple lines.\n"
         << "1234567890\n"
         << "End of the file.\n";
    file.close();
}

// Create an empty test file
void mmap_file_test::create_empty_file(const char* path) {
    std::ofstream file(path);
    file.close();
}

// Test opening a valid file
TEST_F(mmap_file_test, open_valid_file) {
    mmap_file file(valid_file_path);
    EXPECT_TRUE(file.is_open()) << "Failed to open a valid file.";
}

// Test reading contents from a valid file
TEST_F(mmap_file_test, read_file_contents) {
    mmap_file file(valid_file_path);
    ASSERT_TRUE(file.is_open()) << "Failed to open a valid file.";

    // Expected data from the test file
    const char* expected_data = "Hello, this is a test file.\n"
                                "This file contains multiple lines.\n"
                                "1234567890\n"
                                "End of the file.\n";

    EXPECT_EQ(file.size(), std::strlen(expected_data)) << "File size mismatch.";
    EXPECT_EQ(std::memcmp(file.data(), expected_data, file.size()), 0)
        << "File contents do not match expected data.";
}

// Test trying to open a non-existent file
TEST_F(mmap_file_test, open_nonexistent_file) {
    EXPECT_THROW({
        mmap_file file(nonexistent_file_path);
    }, std::ios_base::failure) << "Expected exception when opening a nonexistent file.";
}

// Test handling of an empty file
TEST_F(mmap_file_test, open_empty_file) {
    mmap_file file(empty_file_path);
    ASSERT_TRUE(file.is_open()) << "Failed to open an empty file.";
    EXPECT_EQ(file.size(), 0) << "Empty file size should be zero.";
    EXPECT_EQ(file.data(), nullptr) << "Data pointer should be nullptr for an empty file.";
}