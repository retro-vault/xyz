/*
 * Implements the protocol-neutral debugger core that wraps the
 * shared session layer and emits frontend-facing debug events.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */

#include "debugger.h"

void debugger::set_event_sink(debugger_event_sink* sink) {
    sink_ = sink;
}

void debugger::set_exec_path(const std::filesystem::path& path) {
    host_.set_exec_path(path);
    if (sink_ != nullptr) {
        sink_->on_exec_file_set(path);
    }
}

void debugger::load_cdb_file(const std::filesystem::path& path) {
    host_.load_cdb_file(path);
    if (sink_ != nullptr) {
        sink_->on_symbols_loaded(path);
    }
}

void debugger::load_map_file(const std::filesystem::path& path) {
    host_.load_map_file(path);
}

void debugger::maybe_load_default_symbols() {
    const auto before = host_.cdb_path();
    host_.maybe_load_default_symbols();
    const auto after = host_.cdb_path();
    if (sink_ != nullptr && after.has_value() &&
        (!before.has_value() || before.value() != after.value())) {
        sink_->on_symbols_loaded(after.value());
    }
}

void debugger::connect_remote(const std::string& target) {
    host_.connect_remote(target);
    if (sink_ != nullptr) {
        sink_->on_remote_connected(target);
    }
}

breakpoint_entry debugger::add_breakpoint(const std::string& expression) {
    const auto breakpoint = host_.add_breakpoint(expression);
    if (sink_ != nullptr) {
        sink_->on_breakpoint_added(breakpoint);
    }
    return breakpoint;
}

void debugger::delete_breakpoint(int id) {
    host_.delete_breakpoint(id);
    if (sink_ != nullptr) {
        sink_->on_breakpoint_deleted(id);
    }
}

void debugger::delete_all_breakpoints() {
    host_.delete_all_breakpoints();
    if (sink_ != nullptr) {
        sink_->on_breakpoints_cleared();
    }
}

stop_snapshot debugger::status() {
    const auto stop = host_.status();
    if (sink_ != nullptr) {
        sink_->on_stop(stop);
    }
    return stop;
}

stop_snapshot debugger::continue_execution() {
    const auto stop = host_.continue_execution();
    if (sink_ != nullptr) {
        sink_->on_stop(stop);
    }
    return stop;
}

stop_snapshot debugger::step_instruction() {
    const auto stop = host_.step_instruction();
    if (sink_ != nullptr) {
        sink_->on_stop(stop);
    }
    return stop;
}

stop_snapshot debugger::pause_execution() {
    const auto stop = host_.pause_execution();
    if (sink_ != nullptr) {
        sink_->on_stop(stop);
    }
    return stop;
}

void debugger::detach() {
    host_.detach();
    if (sink_ != nullptr) {
        sink_->on_detached();
    }
}

debugger_host& debugger::session() {
    return host_;
}

const debugger_host& debugger::session() const {
    return host_;
}
