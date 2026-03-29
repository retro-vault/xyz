// mmap_streambuf_test.cpp
#include <fstream>
#include <cstring> // For std::memcmp
#include <cstdio>  // For std::remove

#include "convention.h"

#include "mmap_file.h"
#include "mmap_streambuf.h"

#include "mmap_streambuf_test.h"

// SetUp method: Initialize test paths and create necessary test files
void mmap_streambuf_test::setup() {
    // Set the paths for the test files
    valid_file_path = "bin/data/test_file.txt";
    utf8_file_path = "bin/data/utf8_test_file.txt";
    empty_file_path = "bin/data/empty_file.txt";
    nonexistent_file_path = "bin/data/nonexistent.txt";

    // Create test files with content
    create_test_file(valid_file_path, "Hello, this is a test file.\nThis file contains multiple lines.\n1234567890\nEnd of the file.\n");
    create_test_file(utf8_file_path, "test");
    create_test_file(empty_file_path, "");
}

// TearDown method: Clean up the test files
void mmap_streambuf_test::teardown() {
    // Remove test files after tests
    std::remove(valid_file_path);
    std::remove(utf8_file_path);
    std::remove(empty_file_path);
}

// Helper function to create a test file with the given content
void mmap_streambuf_test::create_test_file(const char* path, const std::string& content) {
    std::ofstream file(path);
    file << content;
    file.close();
}

// Test reading from a valid file using mmap_streambuf
TEST_F(mmap_streambuf_test, read_valid_file) {
    mmap_streambuf buffer(valid_file_path, false); // ASCII mode
    std::istream input(&buffer);
    std::string line;
    std::getline(input, line);

    EXPECT_EQ(line, "Hello, this is a test file.") << "Failed to read the first line correctly.";
    EXPECT_TRUE(input.good()) << "Stream state should be good after reading.";
}

// Test handling of an empty file
TEST_F(mmap_streambuf_test, read_empty_file) {
    mmap_streambuf buffer(empty_file_path, false);
    std::istream input(&buffer);
    std::string line;
    std::getline(input, line);

    EXPECT_EQ(line, "") << "Empty file should result in an empty string.";
    EXPECT_TRUE(input.eof()) << "Stream should reach EOF immediately with an empty file.";
}

// Test handling of putback operations
TEST_F(mmap_streambuf_test, putback_character) {
    mmap_streambuf buffer(valid_file_path, false);
    std::istream input(&buffer);
    char c;

    input.get(c);
    EXPECT_EQ(c, 'H') << "Expected first character to be 'H'.";
    input.putback(c);

    input.get(c);
    EXPECT_EQ(c, 'H') << "After putback, expected to read 'H' again.";
}

// Test attempting to open a non-existent file
TEST_F(mmap_streambuf_test, open_nonexistent_file) {
    EXPECT_THROW({
        mmap_streambuf buffer(nonexistent_file_path, false);
    }, std::ios_base::failure) << "Expected exception when opening a nonexistent file.";
}