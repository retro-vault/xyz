// postrack.h
#ifndef POSTRACK_H
#define POSTRACK_H

#include <set>
#include <map>  // To store bookmarks with positions

class postrack {
public:
    postrack();

    void update_position(int ch);
    void adjust_position_on_putback(char c);
    void add_bookmark(std::size_t pos);
    void set_position(std::size_t pos);

    int line() const;
    int column() const;
    const std::set<std::size_t>& newline_positions() const;

private:
    int line_;
    int column_;
    std::size_t current_pos_;
    std::set<std::size_t> newline_positions_;
    std::map<std::size_t, std::pair<int, int>> bookmarks_; // Stores bookmarks with line and column
};

#endif // POSTRACK_H
