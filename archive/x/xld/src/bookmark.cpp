// bookmark.cpp
#include "bookmark.h"

bookmark::bookmark(std::size_t pos, int line, int column)
    : pos_(pos), line_(line), column_(column) {}

std::size_t bookmark::position() const {
    return pos_;
}

int bookmark::line() const {
    return line_;
}

int bookmark::column() const {
    return column_;
}
