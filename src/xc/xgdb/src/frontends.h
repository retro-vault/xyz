// Declares the CLI and MI frontend adapters that expose the shared
// debugger core through different user and editor protocols.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih

#ifndef XGDB_FRONTENDS_HPP
#define XGDB_FRONTENDS_HPP

#include <fstream>
#include <istream>
#include <memory>
#include <ostream>
#include <streambuf>
#include <string>
#include <vector>

#include <xgdb/debug_protocol.h>

#include "debugger.h"

// Streambuf that writes every byte to both a primary sink and a log file.
// Used to tee stdout/stderr to the log when --log is active.
class tee_streambuf final : public std::streambuf {
public:
    tee_streambuf(std::streambuf* primary, std::streambuf* log)
        : primary_(primary), log_(log) {}
protected:
    int overflow(int c) override {
        if (c == EOF) return EOF;
        if (primary_->sputc(static_cast<char>(c)) == EOF) return EOF;
        log_->sputc(static_cast<char>(c));
        return c;
    }
    std::streamsize xsputn(const char* s, std::streamsize n) override {
        const auto written = primary_->sputn(s, n);
        log_->sputn(s, n);
        log_->pubsync();   // flush log immediately so nothing is lost if blocked
        return written;
    }
    int sync() override {
        log_->pubsync();
        return primary_->pubsync();
    }
private:
    std::streambuf* primary_;
    std::streambuf* log_;
};

// Command-line interactive frontend for the debugger core.
class cli_frontend final : public debug_protocol, public debugger_event_sink {
public:
    // Construct a CLI frontend over explicit IO streams.
    cli_frontend(
        debugger_host& host,
        std::istream& input,
        std::ostream& output,
        std::ostream& error,
        bool show_prompt = true);

    // Queue commands to execute before entering the interactive loop.
    void set_execute_commands(std::vector<std::string> commands);
    // Run the frontend until quit or EOF.
    int run() override;
    // Execute one CLI command line.
    bool execute_command(const std::string& line);
    // Enable protocol logging to a file (logs [IN]/[OUT] lines).
    void set_log(std::ostream* log);
    // Emit a GDB-style source annotation for DDD and similar frontends.
    void emit_source_annotation(const source_location& location);

private:
    debugger debugger_;
    std::istream& input_;
    std::ostream& output_;
    std::ostream& error_;
    std::vector<std::string> execute_commands_;
    bool show_prompt_  = true;
    bool should_quit_  = false;
    int  annotate_     = 0;         // GDB annotation level (0=off, 1=on)
    std::ostream* log_ = nullptr;   // optional protocol log
};

// Minimal machine-interface style frontend.
class mi_frontend final : public debug_protocol, public debugger_event_sink {
public:
    explicit mi_frontend(debugger_host& host);
    int run() override;
    void set_log(std::ostream* log);

private:
    debugger debugger_;
    bool handle_line(const std::string& line);
    std::ostream* log_ = nullptr;
};

#endif
