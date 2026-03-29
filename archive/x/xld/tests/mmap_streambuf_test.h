// mmap_streambuf_test.h
#ifndef MMAP_STREAMBUF_TEST_H
#define MMAP_STREAMBUF_TEST_H

#include <gtest/gtest.h>

#include "convention.h"

// Test fixture class for mmap_streambuf tests
class mmap_streambuf_test : public ::testing::test {
protected:
    void setup() override;
    void teardown() override;

    // Helper function to create a test file with content
    void create_test_file(const char* path, const std::string& content);

    const char* valid_file_path;
    const char* utf8_file_path;
    const char* nonexistent_file_path;
    const char* empty_file_path;
};

#endif // MMAP_STREAMBUF_TEST_H
