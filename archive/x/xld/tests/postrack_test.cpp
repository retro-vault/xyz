// test_postrack.cpp
#include <gtest/gtest.h>

#include "postrack.h"

#include "convention.h"
#include "postrack_test.h"

// Test updating position with a normal character (non-newline)
TEST_F(postrack_test, update_position_with_normal_character) {
    tracker->update_position('a');
    EXPECT_EQ(tracker->line(), 1);  // Expect line number to be 1
    EXPECT_EQ(tracker->column(), 1); // Expect column to increment
}

// Test updating position with a newline character
TEST_F(postrack_test, update_position_with_newline_character) {
    tracker->update_position('\n');
    EXPECT_EQ(tracker->line(), 2);  // Expect line number to increment
    EXPECT_EQ(tracker->column(), 0); // Expect column to reset
    EXPECT_EQ(tracker->newline_positions().size(), 1); // Expect newline position to be tracked
}

// Test adjusting position on putback of a normal character
TEST_F(postrack_test, adjust_position_on_putback_normal_character) {
    tracker->update_position('a');
    tracker->adjust_position_on_putback('a');
    EXPECT_EQ(tracker->line(), 1);  // Line should remain unchanged
    EXPECT_EQ(tracker->column(), 0); // Column should decrement
}

// Test adjusting position on putback of a newline character
TEST_F(postrack_test, adjust_position_on_putback_newline_character) {
    tracker->update_position('\n');
    tracker->adjust_position_on_putback('\n');
    EXPECT_EQ(tracker->line(), 1);  // Expect line to decrement back
    EXPECT_EQ(tracker->column(), 0); // Column adjustment based on newline map
    EXPECT_EQ(tracker->newline_positions().size(), 1); // Newline positions should still contain the newline
}

// Test setting a specific position
TEST_F(postrack_test, set_position) {
    tracker->set_position(5);
    EXPECT_EQ(tracker->line(), 1);  // Line should not change with set_position
    EXPECT_EQ(tracker->column(), 0); // Column does not update without update_position call
}

// Test adding a bookmark and then resetting the position
TEST_F(postrack_test, add_bookmark_and_reset_position) {
    // Simulate updating positions up to position 10
    for (int i = 0; i < 9; ++i) {
        tracker->update_position('a'); // Update tracker naturally to position 10
    }
    tracker->update_position('\n'); // Simulate reaching position 10 with a newline

    tracker->add_bookmark(10);        // Add a bookmark at position 10
    tracker->set_position(10);        // Reset to the bookmarked position
    EXPECT_EQ(tracker->line(), 2);    // Expect to be on line 2 since a newline was at position 10
    EXPECT_EQ(tracker->column(), 0);  // Expect column to be 0 at the start of a new line
}

