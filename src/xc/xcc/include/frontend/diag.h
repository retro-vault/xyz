//
// diag.h — shared diagnostic engine for the xcc compiler.
//
// All compiler stages (preprocessor, lexer, parser, sema) report
// diagnostics through a single diag_engine instance created by the
// driver.  This gives consistent formatting and lets the driver
// decide whether to keep going after errors.
//
// Severity levels:
//   note    — informational, no counter increment
//   warning — printed, no counter increment
//   error   — printed, increments error_count()
//   fatal   — printed, increments error_count(), throws fatal_error
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//

#pragma once
#include "frontend/token.h"  // source_loc
#include <cstdarg>
#include <stdexcept>

namespace xcc {

// Thrown by diag_engine::fatal() so the driver can catch it without exit(1).
struct fatal_error : std::runtime_error {
    explicit fatal_error() : std::runtime_error("fatal compilation error") {}
};

class diag_engine {
public:
    void note   (const source_loc &loc, const char *fmt, ...);
    void warning(const source_loc &loc, const char *fmt, ...);
    void error  (const source_loc &loc, const char *fmt, ...);
    void fatal  (const source_loc &loc, const char *fmt, ...);  // exits

    // For callers that have file/line but no column (e.g. preprocessor).
    void note   (const char *file, int line, const char *fmt, ...);
    void warning(const char *file, int line, const char *fmt, ...);
    void error  (const char *file, int line, const char *fmt, ...);
    void fatal  (const char *file, int line, const char *fmt, ...);

    // For driver-level messages with no location.
    void error(const char *fmt, ...);
    void fatal(const char *fmt, ...);

    int  error_count() const { return errors_; }
    bool has_errors()  const { return errors_ > 0; }

private:
    int errors_ = 0;

    void emit(const char *sev, const char *file, int line, int col,
              const char *fmt, va_list ap);
    void emit(const char *sev, const char *fmt, va_list ap);
};

} // namespace xcc
