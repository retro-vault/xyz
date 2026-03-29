// bookmark.h
#ifndef BOOKMARK_H
#define BOOKMARK_H

#include <cstddef>

class bookmark {
public:
    bookmark(std::size_t pos, int line, int column);

    std::size_t position() const;
    int line() const;
    int column() const;

private:
    std::size_t pos_;
    int line_;
    int column_;
};

#endif // BOOKMARK_H
