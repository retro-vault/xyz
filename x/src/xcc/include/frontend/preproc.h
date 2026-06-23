//
// preproc.h — built-in C preprocessor for xcc.
//
// Handles the common subset needed for bare-metal Z80 programs:
//   #define / #undef   object-like and function-like macros
//   #include            both "file" and <file> forms
//   #ifdef / #ifndef / #if / #elif / #else / #endif
//   #error              emits a fatal diagnostic
//   #pragma once        include-once header guard
//   #pragma GCC diagnostic ...
//   Predefined:         __FILE__, __LINE__, __DATE__, __TIME__
//
// The preprocessor runs as a text-to-text pass before the lexer.
// Output contains standard "# linenum \"filename\"" markers so the
// lexer, parser, and DWARF emitter all report correct source locations.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//

#pragma once
#include "frontend/diag.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace xcc {

struct macro_def {
    bool                     is_function_like = false;
    bool                     is_variadic      = false;
    std::vector<std::string> params;
    std::string              body;
};

class preprocessor {
public:
    //
    // include_paths: directories searched for <file> includes (and "file" if
    // not found relative to the including file).
    // cmdline_defines: NAME or NAME=VALUE strings from -D flags.
    //
    preprocessor(diag_engine                   &diag,
                 const std::vector<std::string> &include_paths,
                 const std::vector<std::string> &cmdline_defines);

    //
    // Preprocess source text from filename and return the fully expanded text
    // with "# line" markers.  Calls exit(1) on #error or missing includes.
    //
    std::string process(const std::string &source,
                        const std::string &filename);

private:
    diag_engine                                     &diag_;
    std::vector<std::string>                         include_paths_;
    std::unordered_map<std::string, macro_def>       macros_;
    std::unordered_set<std::string>                  pragma_once_files_;

    // Set by process_text() before each expand() call so that __LINE__ and
    // __FILE__ inside macro bodies resolve to the call-site location.
    mutable int         expand_lineno_ = 0;
    mutable std::string expand_file_;
    mutable int         counter_       = 0; // __COUNTER__ value

    // Recursive worker: processes source, appending to out.
    void process_text(const std::string &source,
                      const std::string &filename,
                      std::string       &out,
                      int                depth);

    // Macro expansion on a single line of text.
    std::string expand(const std::string &text,
                       std::vector<std::string> &guard) const;

    // Parse the argument list of a function-like macro call.
    // pos must point at '('; advanced past the closing ')'.
    static std::vector<std::string> parse_args(const std::string &text,
                                               size_t &pos);

    // Evaluate a #if / #elif expression (returns 0 or non-zero).
    long long eval_if(const std::string &expr,
                      const std::string &current_dir) const;

    // Find an include file; returns "" if not found.
    std::string find_include(const std::string &name,
                             bool               system_include,
                             const std::string &current_dir) const;

    static std::string read_file(const std::string &path);
    static std::string pp_trim(const std::string &s);
    static bool        is_id_start(char c);
    static bool        is_id_cont(char c);
    static std::string read_ident(const std::string &s, size_t pos);
};

} // namespace xcc
