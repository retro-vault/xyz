// errors.hpp
//
// exception hierarchy for xlink
//
// MIT License (see: LICENSE)
// copyright (C) 2021 tomaz stih
//
// 2021-07-28   tstih
#ifndef XLINK_ERRORS_HPP
#define XLINK_ERRORS_HPP

#include <stdexcept>
#include <string>

namespace xlink {

    class xlink_error : public std::runtime_error {
    public:
        using std::runtime_error::runtime_error;
    };

    class parse_error : public xlink_error {
    public:
        parse_error(const std::string& file, int line, const std::string& msg)
            : xlink_error(file + ":" + std::to_string(line) + ": " + msg),
              file_(file), line_(line) {}

        parse_error(const std::string& msg)
            : xlink_error(msg), line_(0) {}

        const std::string& file() const { return file_; }
        int line() const { return line_; }

    private:
        std::string file_;
        int line_;
    };

    class symbol_error : public xlink_error {
    public:
        using xlink_error::xlink_error;
    };

    class placement_error : public xlink_error {
    public:
        using xlink_error::xlink_error;
    };

    class reloc_error : public xlink_error {
    public:
        using xlink_error::xlink_error;
    };

} // namespace xlink

#endif // XLINK_ERRORS_HPP
