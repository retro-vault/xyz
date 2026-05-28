// errors.hpp
//
// Exception hierarchy for libbfd.  All exceptions derive from bfd_error
// so callers can catch them with a single handler.
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
//
#ifndef XBFD_ERRORS_HPP
#define XBFD_ERRORS_HPP

#include <stdexcept>
#include <string>

namespace bfd {

    // Base for all libbfd exceptions.
    class bfd_error : public std::runtime_error {
    public:
        using std::runtime_error::runtime_error;
    };

    // File could not be opened or read.
    class io_error : public bfd_error {
    public:
        using bfd_error::bfd_error;
    };

    // File format is not recognised or is malformed.
    class format_error : public bfd_error {
    public:
        format_error(const std::string& file, int line, const std::string& msg)
            : bfd_error(file + ":" + std::to_string(line) + ": " + msg),
              file_(file), line_(line) {}

        explicit format_error(const std::string& msg)
            : bfd_error(msg), line_(0) {}

        const std::string& file() const { return file_; }
        int line() const { return line_; }

    private:
        std::string file_;
        int line_;
    };

    // A symbol required for an operation could not be resolved.
    class symbol_error : public bfd_error {
    public:
        using bfd_error::bfd_error;
    };

    // A relocation could not be applied.
    class reloc_error : public bfd_error {
    public:
        using bfd_error::bfd_error;
    };

} // namespace bfd

#endif // XBFD_ERRORS_HPP
