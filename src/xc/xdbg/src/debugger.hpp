/*
 * Declares the protocol-neutral debugger core and event sink used
 * to drive CLI, MI, and DAP frontends from a shared session layer.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */

#ifndef XDBG_DEBUGGER_HPP
#define XDBG_DEBUGGER_HPP

#include <optional>
#include <string>

#include <xdbg/debugger_host.hpp>

class debugger_event_sink {
public:
    virtual ~debugger_event_sink() = default;

    virtual void on_exec_file_set(const std::filesystem::path& path) {}
    virtual void on_symbols_loaded(const std::filesystem::path& path) {}
    virtual void on_remote_connected(const std::string& target) {}
    virtual void on_breakpoint_added(const breakpoint_entry& breakpoint) {}
    virtual void on_breakpoint_deleted(int id) {}
    virtual void on_breakpoints_cleared() {}
    virtual void on_stop(const stop_snapshot& stop) {}
    virtual void on_detached() {}
};

class debugger {
public:
    explicit debugger(debugger_host& host)
        : host_(host) {}

    void set_event_sink(debugger_event_sink* sink);

    void set_exec_path(const std::filesystem::path& path);
    void load_symbols_file(const std::filesystem::path& path);
    void maybe_load_default_symbols();
    void connect_remote(const std::string& target);

    breakpoint_entry add_breakpoint(const std::string& expression);
    void delete_breakpoint(int id);
    void delete_all_breakpoints();

    stop_snapshot status();
    stop_snapshot continue_execution();
    stop_snapshot step_instruction();
    stop_snapshot pause_execution();
    void detach();

    debugger_host& session();
    const debugger_host& session() const;

private:
    debugger_host& host_;
    debugger_event_sink* sink_ = nullptr;
};

#endif
