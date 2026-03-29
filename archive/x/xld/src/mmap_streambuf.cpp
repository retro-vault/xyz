// mmap_streambuf.cpp
#include "mmap_streambuf.h"
#include <codecvt>
#include <locale>
#include <iostream>

mmap_streambuf::mmap_streambuf(const char* filename, bool utf8_mode)
    : file_(filename), utf8_mode_(utf8_mode) {
    if (file_.is_open()) {
        setg(const_cast<char*>(file_.data()), const_cast<char*>(file_.data()), const_cast<char*>(file_.data()) + file_.size());
    }
}

mmap_streambuf::~mmap_streambuf() {}

std::streambuf::int_type mmap_streambuf::underflow() {
    if (gptr() == egptr()) {
        return traits_type::eof();
    }

    const char* start = gptr();
    std::size_t remaining = egptr() - gptr();
    if (utf8_mode_) {
        decode_utf8(start, remaining);
    } else {
        tracker_.update_position(*gptr());
        gbump(1);
    }

    return traits_type::to_int_type(*gptr());
}

std::streambuf::int_type mmap_streambuf::sputbackc(char_type c) {
    // Handle putback by moving the pointer back and updating tracking
    if (gptr() > eback()) {
        gbump(-1);
        if (*gptr() == c) {
            tracker_.adjust_position_on_putback(c);
            return traits_type::to_int_type(c);
        }
    }
    return traits_type::eof();
}

void mmap_streambuf::decode_utf8(const char* start, std::size_t remaining) {
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;
    try {
        std::u32string decoded = converter.from_bytes(start, start + remaining);
        tracker_.update_position(decoded[0]);
    } catch (const std::range_error&) {
        throw std::ios_base::failure("Error decoding UTF-8 sequence.");
    }
}
