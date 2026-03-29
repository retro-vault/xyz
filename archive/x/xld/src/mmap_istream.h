// mmap_istream.h
#ifndef MMAP_ISTREAM_H
#define MMAP_ISTREAM_H

#include <istream>
#include "mmap_streambuf.h"

class mmap_istream : public std::istream {
public:
    mmap_istream(const char* filename, bool utf8_mode = false);

    int line() const;
    int column() const;

private:
    mmap_streambuf buffer_;
};

#endif // MMAP_ISTREAM_H
