//
// xobjcopy errors
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
//
#ifndef XOBJCOPY_ERRORS_HPP
#define XOBJCOPY_ERRORS_HPP

#include <stdexcept>
#include <string>

namespace xobjcopy {

    class error : public std::runtime_error {
    public:
        using std::runtime_error::runtime_error;
    };

    class usage_error : public error {
    public:
        using error::error;
    };

} // namespace xobjcopy

#endif // XOBJCOPY_ERRORS_HPP
