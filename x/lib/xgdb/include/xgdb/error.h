/*
 * Defines the xgdb exception hierarchy used to report parsing
 * and document processing failures to hosted tooling.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */

#ifndef XGDB_ERROR_H
#define XGDB_ERROR_H

#include <stdexcept>
#include <string>

namespace xgdb {

    // Base exception for xgdb runtime and parsing failures.
    class error : public std::runtime_error {
    public:
        // Inherit the standard runtime error constructors.
        using std::runtime_error::runtime_error;
    };

    // Exception thrown when an xgdb document cannot be parsed.
    class parse_error : public error {
    public:
        // Construct a parse error tied to a specific input line.
        //
        // Parameters:
        //      line_number - One-based line number of the error.
        //      message     - Human-readable error description.
        //
        // Returns:
        //      New exception object.
        parse_error(std::size_t line_number, const std::string& message)
            : error("line " + std::to_string(line_number) + ": " + message) {}
    };

} // namespace xgdb

#endif // XGDB_ERROR_H
