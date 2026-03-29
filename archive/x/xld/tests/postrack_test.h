#ifndef POSTRACK_TEST_H
#define POSTRACK_TEST_H

// test_postrack.cpp
#include <gtest/gtest.h>

#include "convention.h"

// Fixture class for testing postrack
class postrack_test : public ::testing::test {
protected:
    void setup() override {
        // Set up code to run before each test
        tracker = new postrack();
    }

    void teardown() override {
        // Clean up code to run after each test
        delete tracker;
    }

    postrack* tracker;
};

#endif