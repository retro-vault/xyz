/*
 * Defines the xdbgstub exception hierarchy used to report transport
 * and protocol failures to hosted debugging clients and servers.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */

#ifndef XDBGSTUB_ERROR_HPP
#define XDBGSTUB_ERROR_HPP

#include <stdexcept>
#include <string>

namespace xdbgstub {

    // Base exception for xdbgstub runtime failures.
    class error : public std::runtime_error {
    public:
        // Inherit the standard runtime error constructors.
        using std::runtime_error::runtime_error;
    };

    // Exception thrown when a protocol message is malformed or invalid.
    class protocol_error : public error {
    public:
        // Inherit the base xdbgstub error constructors.
        using error::error;
    };

} // namespace xdbgstub

#endif // XDBGSTUB_ERROR_HPP
