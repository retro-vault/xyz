// Declares the CLI, MI, and DAP frontend adapters that expose the shared
// debugger core through different user and editor protocols.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih

#ifndef XDBG_FRONTENDS_HPP
#define XDBG_FRONTENDS_HPP

#include <istream>
#include <map>
#include <ostream>
#include <optional>
#include <string>
#include <cstdint>
#include <set>
#include <vector>

#include <xdbg/debug_protocol.hpp>

#include "debugger.hpp"
#include "dap_json.hpp"

// Command-line interactive frontend for the debugger core.
class cli_frontend final : public debug_protocol, public debugger_event_sink {
public:
    // Construct a CLI frontend over explicit IO streams.
    cli_frontend(
        debugger_host& host,
        std::istream& input,
        std::ostream& output,
        std::ostream& error,
        bool quiet,
        bool show_prompt = true);

    // Queue commands to execute before entering the interactive loop.
    void set_execute_commands(std::vector<std::string> commands);
    // Run the frontend until quit or EOF.
    int run() override;
    // Execute one CLI command line.
    bool execute_command(const std::string& line);

private:
    debugger debugger_;
    std::istream& input_;
    std::ostream& output_;
    std::ostream& error_;
    std::vector<std::string> execute_commands_;
    bool quiet_ = false;
    bool show_prompt_ = true;
    bool should_quit_ = false;
};

// Minimal machine-interface style frontend.
class mi_frontend final : public debug_protocol, public debugger_event_sink {
public:
    // Construct an MI frontend over one debugger host.
    explicit mi_frontend(debugger_host& host);
    // Run the MI request loop.
    int run() override;

private:
    debugger debugger_;
    // Handle one MI command line.
    bool handle_line(const std::string& line);
};

// Debug Adapter Protocol frontend used by the VS Code extension.
class dap_frontend final : public debug_protocol, public debugger_event_sink {
public:
    // Construct a DAP frontend over one debugger host.
    explicit dap_frontend(debugger_host& host);
    // Run the DAP message loop.
    int run() override;
    // Publish a stop event to the DAP client.
    void on_stop(const stop_snapshot& stop) override;
    // Publish a connection event to the DAP client.
    void on_remote_connected(const std::string& target) override;
    // Publish a detach event to the DAP client.
    void on_detached() override;

private:
    debugger debugger_;
    // Stored mapping from DAP variable references to debugger-side scopes.
    struct variable_reference {
        // Kind of variable scope represented by the DAP handle.
        enum class kind {
            locals,     // Local variables for the current stop point.
            globals,    // Global symbols or variables.
            registers   // CPU register snapshot.
        };

        kind type = kind::locals;
        uint32_t pc = 0;
    };

    // Parsed DAP request envelope.
    struct request_envelope {
        int seq = 0;
        std::string command;
        json_value arguments = json_value(json_value::object_type{});
    };

    // Read one framed DAP JSON message.
    bool read_message(json_value& body);
    // Write one framed DAP JSON message.
    void write_json_message(const json_value& body);
    // Send a DAP response message.
    void send_response(
        int seq,
        const std::string& command,
        bool success,
        json_value body = json_value(json_value::object_type{}),
        const std::string& message = "");
    // Send a DAP event message.
    void send_event(
        const std::string& event,
        json_value body = json_value(json_value::object_type{}));
    // Parse one request envelope from decoded JSON.
    request_envelope parse_request(const json_value& body) const;
    // Dispatch one decoded DAP request.
    void handle_request(const json_value& body);
    // Handle the DAP initialize request.
    void handle_initialize(int seq);
    // Handle launch and attach requests.
    void handle_launch_or_attach(
        int seq, const std::string& command, const json_value& arguments);
    // Handle source-address breakpoint updates.
    void handle_set_breakpoints(int seq, const json_value& arguments);
    // Handle function breakpoint updates.
    void handle_set_function_breakpoints(int seq, const json_value& arguments);
    // Handle exception breakpoint configuration.
    void handle_set_exception_breakpoints(int seq);
    // Handle the threads request.
    void handle_threads(int seq);
    // Handle the stackTrace request.
    void handle_stack_trace(int seq, const json_value& arguments);
    // Handle the scopes request.
    void handle_scopes(int seq, const json_value& arguments);
    // Handle the variables request.
    void handle_variables(int seq, const json_value& arguments);
    // Handle the continue request.
    void handle_continue(int seq);
    // Handle next and stepIn style requests.
    void handle_next(int seq, const std::string& command);
    // Handle the pause request.
    void handle_pause(int seq);
    // Handle the disconnect request.
    void handle_disconnect(int seq);
    // Handle the configurationDone request.
    void handle_configuration_done(int seq);
    // Handle source content retrieval.
    void handle_source(int seq, const json_value& arguments);
    // Handle disassembly retrieval.
    void handle_disassemble(int seq, const json_value& arguments);
    // Handle evaluate expressions.
    void handle_evaluate(int seq, const json_value& arguments);
    // Handle loadedSources enumeration.
    void handle_loaded_sources(int seq);

    // Extract and validate a required string field.
    std::string require_string(const json_value& object, const std::string& key) const;
    // Extract and validate a required integer field.
    int require_int(const json_value& object, const std::string& key) const;
    // Extract and validate a required boolean field.
    bool require_bool(const json_value& object, const std::string& key) const;
    // Extract an optional string field.
    std::optional<std::string> optional_string(
        const json_value& object, const std::string& key) const;
    // Extract an optional integer field.
    std::optional<int> optional_int(
        const json_value& object, const std::string& key) const;
    // Extract an optional object field.
    const json_value* optional_object(
        const json_value& object, const std::string& key) const;
    const json_value::array_type* optional_array(
        const json_value& object, const std::string& key) const;
    std::string source_path_from_value(const json_value& source) const;
    json_value make_source_object(const source_location& source) const;
    json_value make_source_object(const std::string& path) const;
    json_value make_breakpoint_object(
        const breakpoint_entry& breakpoint,
        int requested_line) const;
    json_value make_stack_frame_object(
        const stop_snapshot& stop,
        int frame_id) const;
    json_value make_scope_object(
        const std::string& name,
        int variables_reference,
        const std::string& presentation_hint) const;
    json_value make_variable_object(
        const std::string& name,
        const std::string& value,
        const std::optional<std::string>& type_name = std::nullopt) const;
    std::optional<uint32_t> stack_return_address(uint32_t sp);
    stop_snapshot continue_to_temporary_breakpoint(uint32_t address);
    bool has_breakpoint_at(uint32_t pc) const;
    stop_snapshot step_to_next_source_stop(const std::string& command);
    std::string stop_reason(const stop_snapshot& stop) const;
    int new_variable_reference(variable_reference ref);
    void clear_source_breakpoints(const std::string& source_path);
    json_value::array_type collect_breakpoint_lines(
        const json_value& arguments) const;
    void emit_process_event(const std::string& target);
    void emit_thread_event(const std::string& reason);
    void emit_continued_event();
    void emit_exited_event(int exit_code);
    void emit_terminated_event();

    int next_seq_ = 1;
    int next_variable_reference_ = 1;
    std::map<int, variable_reference> variable_references_;
    std::map<std::string, std::vector<int>> source_breakpoint_ids_;
    std::vector<int> function_breakpoint_ids_;
    bool suppress_stop_events_ = false;
    bool initialized_ = false;
    bool connected_ = false;
};

#endif
