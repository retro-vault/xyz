// mmap_istream.cpp
#include "mmap_istream.h"

mmap_istream::mmap_istream(const char* filename, bool utf8_mode)
    : std::istream(&buffer_), buffer_(filename, utf8_mode) {}

int mmap_istream::line() const {
    return buffer_.tracker().line();
}

int mmap_istream::column() const {
    return buffer_.tracker().column();
}
