// postrack.cpp
#include "postrack.h"

postrack::postrack()
    : line_(1), column_(0), current_pos_(0) {}

void postrack::update_position(int ch) {
    if (ch == '\n') {
        newline_positions_.insert(current_pos_);
        ++line_;
        column_ = 0;
    } else {
        ++column_;
    }
    ++current_pos_;
}

void postrack::adjust_position_on_putback(char c) {
    if (c == '\n') {
        --line_;
        column_ = current_pos_ - *newline_positions_.lower_bound(current_pos_);
    } else {
        --column_;
    }
    --current_pos_;
}

void postrack::add_bookmark(std::size_t pos) {
    // Store the current line and column along with the position in bookmarks
    bookmarks_[pos] = {line_, column_};
}

void postrack::set_position(std::size_t pos) {
    current_pos_ = pos;

    // Check if the position is bookmarked
    if (bookmarks_.count(pos)) {
        line_ = bookmarks_[pos].first;
        column_ = bookmarks_[pos].second;
    } else {
        // Recalculate line and column for non-bookmarked positions
        line_ = 1;
        column_ = 0;
        for (auto newline_pos : newline_positions_) {
            if (newline_pos < pos) {
                ++line_;
                column_ = pos - newline_pos - 1;  // Calculate the column correctly
            } else {
                break;
            }
        }
    }
}

int postrack::line() const {
    return line_;
}

int postrack::column() const {
    return column_;
}

const std::set<std::size_t>& postrack::newline_positions() const {
    return newline_positions_;
}
