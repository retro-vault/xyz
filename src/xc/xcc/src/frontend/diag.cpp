//
// diag.cpp — shared diagnostic engine implementation.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//
#include "frontend/diag.h"
#include <cstdio>

namespace xcc {

void diag_engine::emit(const char *sev, const char *file, int line, int col,
                       const char *fmt, va_list ap) {
    char msg[512];
    vsnprintf(msg, sizeof(msg), fmt, ap);
    if (col > 0)
        fprintf(stderr, "%s:%d:%d: %s: %s\n", file, line, col, sev, msg);
    else
        fprintf(stderr, "%s:%d: %s: %s\n", file, line, sev, msg);
}

void diag_engine::emit(const char *sev, const char *fmt, va_list ap) {
    char msg[512];
    vsnprintf(msg, sizeof(msg), fmt, ap);
    fprintf(stderr, "xcc: %s: %s\n", sev, msg);
}

// ----- source_loc overloads ------------------------------------------

void diag_engine::note(const source_loc &loc, const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    emit("note", loc.file, loc.line, loc.col, fmt, ap);
    va_end(ap);
}

void diag_engine::warning(const source_loc &loc, const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    emit("warning", loc.file, loc.line, loc.col, fmt, ap);
    va_end(ap);
}

void diag_engine::error(const source_loc &loc, const char *fmt, ...) {
    ++errors_;
    va_list ap; va_start(ap, fmt);
    emit("error", loc.file, loc.line, loc.col, fmt, ap);
    va_end(ap);
}

void diag_engine::fatal(const source_loc &loc, const char *fmt, ...) {
    ++errors_;
    va_list ap; va_start(ap, fmt);
    emit("fatal error", loc.file, loc.line, loc.col, fmt, ap);
    va_end(ap);
    throw fatal_error{};
}

// ----- file/line overloads -------------------------------------------

void diag_engine::note(const char *file, int line, const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    emit("note", file, line, 0, fmt, ap);
    va_end(ap);
}

void diag_engine::warning(const char *file, int line, const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    emit("warning", file, line, 0, fmt, ap);
    va_end(ap);
}

void diag_engine::error(const char *file, int line, const char *fmt, ...) {
    ++errors_;
    va_list ap; va_start(ap, fmt);
    emit("error", file, line, 0, fmt, ap);
    va_end(ap);
}

void diag_engine::fatal(const char *file, int line, const char *fmt, ...) {
    ++errors_;
    va_list ap; va_start(ap, fmt);
    emit("fatal error", file, line, 0, fmt, ap);
    va_end(ap);
    throw fatal_error{};
}

// ----- location-free overloads ---------------------------------------

void diag_engine::error(const char *fmt, ...) {
    ++errors_;
    va_list ap; va_start(ap, fmt);
    emit("error", fmt, ap);
    va_end(ap);
}

void diag_engine::fatal(const char *fmt, ...) {
    ++errors_;
    va_list ap; va_start(ap, fmt);
    emit("fatal error", fmt, ap);
    va_end(ap);
    throw fatal_error{};
}

} // namespace xcc
