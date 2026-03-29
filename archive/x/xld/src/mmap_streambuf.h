// mmap_streambuf.h
#ifndef MMAP_STREAMBUF_H
#define MMAP_STREAMBUF_H

#include <streambuf>
#include "mmap_file.h"
#include "postrack.h"

class mmap_streambuf : public std::streambuf {
public:
    mmap_streambuf(const char* filename, bool utf8_mode = false);
    ~mmap_streambuf();

protected:
    int_type underflow() override;  // Correct function to handle reading
    int_type sputbackc(char_type c);  // Not overriding, just handling putback directly

private:
    mmap_file file_;
    postrack tracker_;
    bool utf8_mode_;

    void decode_utf8(const char* start, std::size_t remaining);
};

#endif // MMAP_STREAMBUF_H
