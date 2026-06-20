//
// diag.cpp — shared diagnostic engine implementation.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//
#include "frontend/diag.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>

namespace xcc {

namespace {

static size_t group_index(warning_group group) {
    return static_cast<size_t>(group);
}

static std::string trim_copy(const std::string &s) {
    size_t first = 0;
    while (first < s.size() && std::isspace((unsigned char)s[first]))
        ++first;
    size_t last = s.size();
    while (last > first && std::isspace((unsigned char)s[last - 1]))
        --last;
    return s.substr(first, last - first);
}

static std::string lower_copy(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return (char)std::tolower(c); });
    return s;
}

static bool take_word(std::string &s, std::string &word) {
    s = trim_copy(s);
    if (s.empty())
        return false;
    size_t end = 0;
    while (end < s.size() && !std::isspace((unsigned char)s[end]))
        ++end;
    word = s.substr(0, end);
    s = s.substr(end);
    return true;
}

static bool take_quoted_warning_name(const std::string &s, std::string &name) {
    std::string t = trim_copy(s);
    if (t.size() < 2 || t[0] != '"')
        return false;
    size_t end = t.find('"', 1);
    if (end == std::string::npos)
        return false;
    name = t.substr(1, end - 1);
    return true;
}

} // namespace

const char *warning_group_name(warning_group group) {
    switch (group) {
    case warning_group::GENERAL: return "general";
    case warning_group::CPP: return "cpp";
    case warning_group::UNKNOWN_PRAGMAS: return "unknown-pragmas";
    case warning_group::UNKNOWN_WARNING_OPTION: return "unknown-warning-option";
    case warning_group::IMPLICIT_FUNCTION_DECLARATION: return "implicit-function-declaration";
    case warning_group::DEPRECATED_DECLARATIONS: return "deprecated-declarations";
    case warning_group::UNUSED_RESULT: return "unused-result";
    case warning_group::ATTRIBUTES: return "attributes";
    case warning_group::OLD_STYLE_DEFINITION: return "old-style-definition";
    case warning_group::C23_EXTENSIONS: return "c23-extensions";
    case warning_group::ABI: return "abi";
    case warning_group::CONSTEXPR_NOT_CONSTANT: return "constexpr-not-constant";
    case warning_group::BITINT_WIDTH: return "bitint-width";
    case warning_group::COUNT: break;
    }
    return "unknown";
}

bool warning_group_from_name(const std::string &raw_name, warning_group &group) {
    std::string name = lower_copy(trim_copy(raw_name));
    if (name.rfind("-wno-", 0) == 0)
        name = name.substr(5);
    else if (name.rfind("-w", 0) == 0)
        name = name.substr(2);

    struct binding {
        const char *name;
        warning_group group;
    };
    static constexpr binding bindings[] = {
        {"general", warning_group::GENERAL},
        {"cpp", warning_group::CPP},
        {"unknown-pragmas", warning_group::UNKNOWN_PRAGMAS},
        {"unknown-warning-option", warning_group::UNKNOWN_WARNING_OPTION},
        {"implicit-function-declaration", warning_group::IMPLICIT_FUNCTION_DECLARATION},
        {"deprecated-declarations", warning_group::DEPRECATED_DECLARATIONS},
        {"deprecated", warning_group::DEPRECATED_DECLARATIONS},
        {"unused-result", warning_group::UNUSED_RESULT},
        {"nodiscard", warning_group::UNUSED_RESULT},
        {"attributes", warning_group::ATTRIBUTES},
        {"old-style-definition", warning_group::OLD_STYLE_DEFINITION},
        {"c23-extensions", warning_group::C23_EXTENSIONS},
        {"abi", warning_group::ABI},
        {"constexpr-not-constant", warning_group::CONSTEXPR_NOT_CONSTANT},
        {"bitint-width", warning_group::BITINT_WIDTH},
    };
    for (const auto &binding : bindings) {
        if (name == binding.name) {
            group = binding.group;
            return true;
        }
    }
    return false;
}

diagnostic_options::diagnostic_options() {
    set_defaults();
}

void diagnostic_options::set_defaults() {
    enabled.fill(true);
    as_error.fill(false);
    all_warnings_as_errors = false;
    enabled[group_index(warning_group::UNKNOWN_PRAGMAS)] = false;
}

void diagnostic_options::disable_all() {
    enabled.fill(false);
}

void diagnostic_options::enable_all() {
    enabled.fill(true);
}

void diagnostic_options::enable_wall() {
    enable_all();
}

void diagnostic_options::enable_wextra() {
    set_group(warning_group::OLD_STYLE_DEFINITION, true);
    set_group(warning_group::ABI, true);
    set_group(warning_group::CONSTEXPR_NOT_CONSTANT, true);
    set_group(warning_group::BITINT_WIDTH, true);
}

void diagnostic_options::enable_pedantic() {
    set_group(warning_group::C23_EXTENSIONS, true);
    set_group(warning_group::BITINT_WIDTH, true);
}

void diagnostic_options::set_group(warning_group group, bool value) {
    enabled[group_index(group)] = value;
}

void diagnostic_options::set_group_error(warning_group group, bool value) {
    as_error[group_index(group)] = value;
    if (value)
        enabled[group_index(group)] = true;
}

bool diagnostic_options::group_enabled(warning_group group) const {
    return enabled[group_index(group)];
}

bool diagnostic_options::group_as_error(warning_group group) const {
    return all_warnings_as_errors || as_error[group_index(group)];
}

void diag_engine::set_options(const diagnostic_options &opts) {
    base_opts_ = opts;
    opts_ = opts;
    option_stack_.clear();
    snapshots_.clear();
}

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

void diag_engine::emit_warning(warning_group group, const char *file, int line,
                               int col, const char *fmt, va_list ap) {
    const diagnostic_options active_opts = options_at(file, line);
    if (!active_opts.group_enabled(group))
        return;

    char msg[512];
    vsnprintf(msg, sizeof(msg), fmt, ap);

    const bool promoted = active_opts.group_as_error(group);
    const char *group_name = warning_group_name(group);
    char with_group[640];
    snprintf(with_group, sizeof(with_group), "%s [-W%s%s]",
             msg, promoted ? "error=" : "", group_name);

    if (promoted)
        ++errors_;

    if (col > 0)
        fprintf(stderr, "%s:%d:%d: %s: %s\n", file, line, col,
                promoted ? "error" : "warning", with_group);
    else
        fprintf(stderr, "%s:%d: %s: %s\n", file, line,
                promoted ? "error" : "warning", with_group);
}

void diag_engine::push_diagnostics() {
    option_stack_.push_back(opts_);
}

void diag_engine::pop_diagnostics(const char *file, int line) {
    if (option_stack_.empty()) {
        warning(warning_group::UNKNOWN_PRAGMAS, file, line,
                "#pragma GCC diagnostic pop without matching push");
        return;
    }
    opts_ = option_stack_.back();
    option_stack_.pop_back();
    record_snapshot(file, line + 1);
}

void diag_engine::set_diagnostic(warning_group group, const char *state) {
    if (strcmp(state, "ignored") == 0) {
        opts_.set_group(group, false);
        opts_.set_group_error(group, false);
    } else if (strcmp(state, "warning") == 0) {
        opts_.set_group(group, true);
        opts_.set_group_error(group, false);
    } else if (strcmp(state, "error") == 0) {
        opts_.set_group(group, true);
        opts_.set_group_error(group, true);
    }
}

void diag_engine::record_snapshot(const char *file, int line) {
    diagnostic_snapshot snapshot;
    snapshot.file = file ? file : "<unknown>";
    snapshot.line = line;
    snapshot.opts = opts_;
    snapshots_.push_back(snapshot);
}

diagnostic_options diag_engine::options_at(const char *file, int line) const {
    diagnostic_options result = base_opts_;
    const std::string file_name = file ? file : "<unknown>";
    for (const auto &snapshot : snapshots_) {
        if (snapshot.file == file_name && snapshot.line <= line)
            result = snapshot.opts;
    }
    return result;
}

void diag_engine::handle_pragma(const char *file, int line, const std::string &text) {
    std::string rest = text;
    std::string word;
    if (!take_word(rest, word)) {
        warning(warning_group::UNKNOWN_PRAGMAS, file, line, "empty #pragma ignored");
        return;
    }

    if (word == "GCC") {
        if (!take_word(rest, word) || word != "diagnostic") {
            warning(warning_group::UNKNOWN_PRAGMAS, file, line,
                    "unknown #pragma GCC %s ignored", word.c_str());
            return;
        }
    } else if (word != "diagnostic") {
        warning(warning_group::UNKNOWN_PRAGMAS, file, line,
                "unknown #pragma %s ignored", word.c_str());
        return;
    }

    std::string action;
    if (!take_word(rest, action)) {
        warning(warning_group::UNKNOWN_PRAGMAS, file, line,
                "malformed #pragma GCC diagnostic ignored");
        return;
    }

    if (action == "push") {
        push_diagnostics();
        return;
    }
    if (action == "pop") {
        pop_diagnostics(file, line);
        return;
    }
    if (action != "ignored" && action != "warning" && action != "error") {
        warning(warning_group::UNKNOWN_PRAGMAS, file, line,
                "unknown #pragma GCC diagnostic action '%s'", action.c_str());
        return;
    }

    std::string option;
    if (!take_quoted_warning_name(rest, option)) {
        warning(warning_group::UNKNOWN_PRAGMAS, file, line,
                "malformed #pragma GCC diagnostic %s", action.c_str());
        return;
    }

    warning_group group;
    if (!warning_group_from_name(option, group)) {
        warning(warning_group::UNKNOWN_WARNING_OPTION, file, line,
                "unknown warning option '%s'", option.c_str());
        return;
    }

    set_diagnostic(group, action.c_str());
    record_snapshot(file, line + 1);
}

// ----- source_loc overloads ------------------------------------------

void diag_engine::note(const source_loc &loc, const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    emit("note", loc.file, loc.line, loc.col, fmt, ap);
    va_end(ap);
}

void diag_engine::warning(const source_loc &loc, const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    emit_warning(warning_group::GENERAL, loc.file, loc.line, loc.col, fmt, ap);
    va_end(ap);
}

void diag_engine::warning(warning_group group, const source_loc &loc, const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    emit_warning(group, loc.file, loc.line, loc.col, fmt, ap);
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
    emit_warning(warning_group::GENERAL, file, line, 0, fmt, ap);
    va_end(ap);
}

void diag_engine::warning(warning_group group, const char *file, int line,
                          const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    emit_warning(group, file, line, 0, fmt, ap);
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
