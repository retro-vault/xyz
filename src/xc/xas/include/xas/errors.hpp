// errors.hpp
//
// Exception hierarchy for xas.  All exceptions derive from asm_error.
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
//
#ifndef XAS_ERRORS_HPP
#define XAS_ERRORS_HPP

#include <stdexcept>
#include <string>

namespace xas {

    class asm_error : public std::runtime_error {
    public:
        using std::runtime_error::runtime_error;
    };

    class lex_error : public asm_error {
    public:
        lex_error(const std::string& file, int line, const std::string& msg)
            : asm_error(file + ":" + std::to_string(line) + ": error: " + msg),
              file_(file), line_(line) {}

        const std::string& file() const { return file_; }
        int line() const { return line_; }
    private:
        std::string file_;
        int line_;
    };

    class parse_error : public asm_error {
    public:
        parse_error(const std::string& file, int line, const std::string& msg)
            : asm_error(file + ":" + std::to_string(line) + ": error: " + msg),
              file_(file), line_(line) {}

        const std::string& file() const { return file_; }
        int line() const { return line_; }
    private:
        std::string file_;
        int line_;
    };

    class codegen_error : public asm_error {
    public:
        codegen_error(const std::string& file, int line, const std::string& msg)
            : asm_error(file + ":" + std::to_string(line) + ": error: " + msg),
              file_(file), line_(line) {}

        const std::string& file() const { return file_; }
        int line() const { return line_; }
    private:
        std::string file_;
        int line_;
    };

} // namespace xas

#endif // XAS_ERRORS_HPP
