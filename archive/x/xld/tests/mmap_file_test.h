// mmap_file_test.h
#ifndef MMAP_FILE_TEST_H
#define MMAP_FILE_TEST_H

#include <fstream>
#include <cstring> // For std::memcmp
#include <cstdio>  // For std::remove

#include <gtest/gtest.h>

#include "convention.h"

// Test fixture class for mmap_file tests
class mmap_file_test : public ::testing::test {
protected:
    void setup() override;
    void teardown() override;

    // Helper functions to create test files
    void create_test_file(const char* path);
    void create_empty_file(const char* path);

    const char* valid_file_path;
    const char* empty_file_path;
    const char* nonexistent_file_path;
};

#endif // MMAP_FILE_TEST_H
