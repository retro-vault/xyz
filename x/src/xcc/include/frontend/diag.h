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
#include <array>
#include <cstdarg>
#include <stdexcept>
#include <string>
#include <vector>

namespace xcc {

enum class warning_group {
    GENERAL = 0,
    CPP,
    UNKNOWN_PRAGMAS,
    UNKNOWN_WARNING_OPTION,
    IMPLICIT_FUNCTION_DECLARATION,
    DEPRECATED_DECLARATIONS,
    UNUSED_RESULT,
    ATTRIBUTES,
    OLD_STYLE_DEFINITION,
    C23_EXTENSIONS,
    ABI,
    CONSTEXPR_NOT_CONSTANT,
    BITINT_WIDTH,
    COUNT
};

const char *warning_group_name(warning_group group);
bool warning_group_from_name(const std::string &name, warning_group &group);

struct diagnostic_options {
    std::array<bool, static_cast<size_t>(warning_group::COUNT)> enabled{};
    std::array<bool, static_cast<size_t>(warning_group::COUNT)> as_error{};
    bool all_warnings_as_errors = false;

    diagnostic_options();

    void set_defaults();
    void disable_all();
    void enable_all();
    void enable_wall();
    void enable_wextra();
    void enable_pedantic();
    void set_group(warning_group group, bool value);
    void set_group_error(warning_group group, bool value);
    bool group_enabled(warning_group group) const;
    bool group_as_error(warning_group group) const;
};

// Thrown by diag_engine::fatal() so the driver can catch it without exit(1).
struct fatal_error : std::runtime_error {
    explicit fatal_error() : std::runtime_error("fatal compilation error") {}
};

class diag_engine {
public:
    void set_options(const diagnostic_options &opts);
    const diagnostic_options &options() const { return opts_; }

    void push_diagnostics();
    void pop_diagnostics(const char *file, int line);
    void set_diagnostic(warning_group group, const char *state);
    void handle_pragma(const char *file, int line, const std::string &text);

    void note   (const source_loc &loc, const char *fmt, ...);
    void warning(const source_loc &loc, const char *fmt, ...);
    void warning(warning_group group, const source_loc &loc, const char *fmt, ...);
    void error  (const source_loc &loc, const char *fmt, ...);
    void fatal  (const source_loc &loc, const char *fmt, ...);  // exits

    // For callers that have file/line but no column (e.g. preprocessor).
    void note   (const char *file, int line, const char *fmt, ...);
    void warning(const char *file, int line, const char *fmt, ...);
    void warning(warning_group group, const char *file, int line, const char *fmt, ...);
    void error  (const char *file, int line, const char *fmt, ...);
    void fatal  (const char *file, int line, const char *fmt, ...);

    // For driver-level messages with no location.
    void error(const char *fmt, ...);
    void fatal(const char *fmt, ...);

    int  error_count() const { return errors_; }
    bool has_errors()  const { return errors_ > 0; }

private:
    struct diagnostic_snapshot {
        std::string file;
        int line = 1;
        diagnostic_options opts;
    };

    int errors_ = 0;
    diagnostic_options base_opts_;
    diagnostic_options opts_;
    std::vector<diagnostic_options> option_stack_;
    std::vector<diagnostic_snapshot> snapshots_;

    void emit(const char *sev, const char *file, int line, int col,
              const char *fmt, va_list ap);
    void emit(const char *sev, const char *fmt, va_list ap);
    void emit_warning(warning_group group, const char *file, int line, int col,
                      const char *fmt, va_list ap);
    void record_snapshot(const char *file, int line);
    diagnostic_options options_at(const char *file, int line) const;
};

} // namespace xcc
