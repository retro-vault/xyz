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

class cli_frontend final : public debug_protocol, public debugger_event_sink {
public:
    cli_frontend(
        debugger_host& host,
        std::istream& input,
        std::ostream& output,
        std::ostream& error,
        bool quiet,
        bool show_prompt = true);

    void set_execute_commands(std::vector<std::string> commands);
    int run() override;
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

class mi_frontend final : public debug_protocol, public debugger_event_sink {
public:
    explicit mi_frontend(debugger_host& host);
    int run() override;

private:
    debugger debugger_;
    bool handle_line(const std::string& line);
};

class dap_frontend final : public debug_protocol, public debugger_event_sink {
public:
    explicit dap_frontend(debugger_host& host);
    int run() override;
    void on_stop(const stop_snapshot& stop) override;
    void on_remote_connected(const std::string& target) override;
    void on_detached() override;

private:
    debugger debugger_;
    struct variable_reference {
        enum class kind {
            locals,
            globals,
            registers
        };

        kind type = kind::locals;
        uint32_t pc = 0;
    };

    struct request_envelope {
        int seq = 0;
        std::string command;
        json_value arguments = json_value(json_value::object_type{});
    };

    bool read_message(json_value& body);
    void write_json_message(const json_value& body);
    void send_response(
        int seq,
        const std::string& command,
        bool success,
        json_value body = json_value(json_value::object_type{}),
        const std::string& message = "");
    void send_event(
        const std::string& event,
        json_value body = json_value(json_value::object_type{}));
    request_envelope parse_request(const json_value& body) const;
    void handle_request(const json_value& body);
    void handle_initialize(int seq);
    void handle_launch_or_attach(
        int seq, const std::string& command, const json_value& arguments);
    void handle_set_breakpoints(int seq, const json_value& arguments);
    void handle_set_function_breakpoints(int seq, const json_value& arguments);
    void handle_set_exception_breakpoints(int seq);
    void handle_threads(int seq);
    void handle_stack_trace(int seq, const json_value& arguments);
    void handle_scopes(int seq, const json_value& arguments);
    void handle_variables(int seq, const json_value& arguments);
    void handle_continue(int seq);
    void handle_next(int seq, const std::string& command);
    void handle_pause(int seq);
    void handle_disconnect(int seq);
    void handle_configuration_done(int seq);
    void handle_source(int seq, const json_value& arguments);
    void handle_evaluate(int seq, const json_value& arguments);
    void handle_loaded_sources(int seq);

    std::string require_string(const json_value& object, const std::string& key) const;
    int require_int(const json_value& object, const std::string& key) const;
    bool require_bool(const json_value& object, const std::string& key) const;
    std::optional<std::string> optional_string(
        const json_value& object, const std::string& key) const;
    std::optional<int> optional_int(
        const json_value& object, const std::string& key) const;
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
