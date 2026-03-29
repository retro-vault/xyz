// test_postrack.cpp

#include <gtest/gtest.h>

#include "convention.h"

int main(int argc, char **argv) {
    ::testing::init_google_test(&argc, argv);
    return RUN_ALL_TESTS();
}