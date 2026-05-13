/*
 * Implements the DAP debug protocol frontend and adapts debugger
 * session events and commands to Debug Adapter Protocol I/O.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */

#include "frontends.hpp"

#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <regex>
#include <set>
#include <stdexcept>
#include <string>

namespace {

    std::string trim(const std::string& value) {
        const auto start = value.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) {
            return "";
        }
        const auto end = value.find_last_not_of(" \t\r\n");
        return value.substr(start, end - start + 1);
    }

    uint32_t parse_u32(const std::string& value) {
        char* end = nullptr;
        const unsigned long parsed = std::strtoul(value.c_str(), &end, 0);
        if (end == value.c_str() || *end != '\0') {
            throw std::runtime_error("invalid number: " + value);
        }
        return static_cast<uint32_t>(parsed);
    }

    std::string hex_u32(uint32_t value) {
        std::ostringstream out;
        out << "0x" << std::hex << std::uppercase << value << std::dec;
        return out.str();
    }

    std::optional<std::string> stringify_storage(const xdbg::variable& variable) {
        switch (variable.storage) {
        case xdbg::storage_kind::address:
            if (variable.address.has_value()) {
                return hex_u32(variable.address.value());
            }
            break;
        case xdbg::storage_kind::stack:
        case xdbg::storage_kind::frame_relative:
            if (variable.offset.has_value()) {
                return std::to_string(variable.offset.value());
            }
            break;
        case xdbg::storage_kind::register_name:
        case xdbg::storage_kind::register_pair:
            if (variable.register_name.has_value()) {
                return variable.register_name.value();
            }
            break;
        case xdbg::storage_kind::unknown:
            break;
        }
        return std::nullopt;
    }

    std::string normalize_c_symbol_name(const std::string& name) {
        if (!name.empty() && name.front() == '_') {
            return name.substr(1);
        }
        return name;
    }

    std::size_t infer_type_size(const std::optional<std::string>& type_name) {
        if (!type_name.has_value()) {
            return 1;
        }

        const std::string& type = type_name.value();
        if (type.find("char") != std::string::npos) {
            return 1;
        }
        if (type.find("long") != std::string::npos ||
            type.find("float") != std::string::npos) {
            return 4;
        }
        if (type.find("int") != std::string::npos ||
            type.find("short") != std::string::npos) {
            return 2;
        }
        return 1;
    }

    std::string format_scalar_value(uint32_t value, std::size_t size) {
        std::ostringstream out;
        if (size == 1) {
            out << static_cast<unsigned>(value & 0xffu) << " (" << hex_u32(value & 0xffu) << ")";
        } else if (size == 2) {
            out << static_cast<unsigned>(value & 0xffffu) << " (" << hex_u32(value & 0xffffu) << ")";
        } else {
            out << value << " (" << hex_u32(value) << ")";
        }
        return out.str();
    }

    std::optional<uint32_t> read_register_value(
        const xdbgstub::cpu_state& regs,
        const std::string& register_name)
    {
        if (register_name == "a") return regs.af >> 8;
        if (register_name == "f") return regs.af & 0xffu;
        if (register_name == "b") return regs.bc >> 8;
        if (register_name == "c") return regs.bc & 0xffu;
        if (register_name == "d") return regs.de >> 8;
        if (register_name == "e") return regs.de & 0xffu;
        if (register_name == "h") return regs.hl >> 8;
        if (register_name == "l") return regs.hl & 0xffu;
        if (register_name == "i") return regs.i;
        if (register_name == "r") return regs.r;
        if (register_name == "af") return regs.af;
        if (register_name == "bc") return regs.bc;
        if (register_name == "de") return regs.de;
        if (register_name == "hl") return regs.hl;
        if (register_name == "ix") return regs.ix;
        if (register_name == "iy") return regs.iy;
        if (register_name == "sp") return regs.sp;
        if (register_name == "pc") return regs.pc;
        return std::nullopt;
    }

    std::optional<uint32_t> read_memory_scalar(
        debugger_host& host,
        uint32_t address,
        std::size_t size)
    {
        if (size == 0 || size > 4) {
            return std::nullopt;
        }

        const auto bytes = host.read_memory(address, size);
        if (bytes.size() < size) {
            return std::nullopt;
        }

        uint32_t value = 0;
        for (std::size_t i = 0; i < size; ++i) {
            value |= static_cast<uint32_t>(bytes[i]) << (8 * i);
        }
        return value;
    }

    std::optional<std::string> read_variable_value(
        debugger_host& host,
        const xdbg::variable& variable)
    {
        const std::size_t size = infer_type_size(variable.type_name);
        const auto regs = host.read_registers();

        switch (variable.storage) {
        case xdbg::storage_kind::address:
            if (variable.address.has_value()) {
                if (const auto value = read_memory_scalar(host, variable.address.value(), size);
                    value.has_value()) {
                    return format_scalar_value(value.value(), size);
                }
            }
            break;
        case xdbg::storage_kind::register_name:
        case xdbg::storage_kind::register_pair:
            if (variable.register_name.has_value()) {
                if (const auto value = read_register_value(regs, variable.register_name.value());
                    value.has_value()) {
                    return format_scalar_value(value.value(), size);
                }
            }
            break;
        case xdbg::storage_kind::stack:
        case xdbg::storage_kind::frame_relative:
        case xdbg::storage_kind::unknown:
            break;
        }

        return stringify_storage(variable);
    }

    const xdbg::symbol* find_global_symbol(
        const xdbg::document* symbols,
        const std::string& expression)
    {
        if (symbols == nullptr) {
            return nullptr;
        }

        for (const auto& symbol : symbols->symbols) {
            if (symbol.kind != xdbg::symbol_kind::object &&
                symbol.kind != xdbg::symbol_kind::global) {
                continue;
            }
            if (symbol.name == expression ||
                normalize_c_symbol_name(symbol.name) == expression) {
                return &symbol;
            }
        }
        return nullptr;
    }

    std::string describe_global_symbol(const xdbg::symbol& symbol) {
        const std::string display_name = normalize_c_symbol_name(symbol.name);
        if (symbol.size.has_value() && symbol.size.value() > 4) {
            std::ostringstream out;
            out << "array[" << symbol.size.value() << "] @ " << hex_u32(symbol.address);
            return out.str();
        }

        const std::size_t size = symbol.size.has_value()
            ? static_cast<std::size_t>(symbol.size.value())
            : 1;
        return format_scalar_value(symbol.address, size);
    }

    std::optional<std::string> read_global_symbol_value(
        debugger_host& host,
        const xdbg::symbol& symbol)
    {
        if (symbol.size.has_value() && symbol.size.value() > 4) {
            std::ostringstream out;
            out << "array[" << symbol.size.value() << "] @ " << hex_u32(symbol.address);
            return out.str();
        }

        const std::size_t size = symbol.size.has_value()
            ? static_cast<std::size_t>(symbol.size.value())
            : 1;
        if (const auto value = read_memory_scalar(host, symbol.address, size); value.has_value()) {
            return format_scalar_value(value.value(), size);
        }
        return std::nullopt;
    }

    struct expression_value {
        std::string result;
        std::optional<std::string> type_name;
    };

    std::optional<expression_value> evaluate_simple_expression(
        debugger_host& host,
        const std::string& raw_expression)
    {
        const std::string expression = trim(raw_expression);
        const auto stop = host.status();

        for (const auto* variable : host.visible_variables(stop.pc)) {
            if (variable->name == expression) {
                if (const auto value = read_variable_value(host, *variable); value.has_value()) {
                    return expression_value{value.value(), variable->type_name};
                }
            }
        }

        if (const auto* symbols = host.symbols(); symbols != nullptr) {
            static const std::regex indexed_name(
                R"(^([A-Za-z_][A-Za-z0-9_]*)\[(0x[0-9A-Fa-f]+|[0-9]+)\]$)");
            std::smatch match;
            if (std::regex_match(expression, match, indexed_name)) {
                const auto* symbol = find_global_symbol(symbols, match[1].str());
                if (symbol != nullptr) {
                    const uint32_t index = parse_u32(match[2].str());
                    if (const auto value = read_memory_scalar(host, symbol->address + index, 1);
                        value.has_value()) {
                        return expression_value{
                            format_scalar_value(value.value(), 1),
                            std::optional<std::string>("unsigned char")};
                    }
                }
            }

            if (const auto* symbol = find_global_symbol(symbols, expression); symbol != nullptr) {
                if (const auto value = read_global_symbol_value(host, *symbol); value.has_value()) {
                    return expression_value{value.value(), symbol->type_name};
                }
            }
        }

        return std::nullopt;
    }

} // namespace

dap_frontend::dap_frontend(debugger_host& host)
    : debugger_(host)
{
    debugger_.set_event_sink(this);
}

int dap_frontend::run() {
    json_value body;
    while (read_message(body)) {
        try {
            handle_request(body);
        } catch (const std::exception& e) {
            int seq = 0;
            std::string command;
            if (body.is_object()) {
                seq = optional_int(body, "seq").value_or(0);
                command = optional_string(body, "command").value_or("");
            }
            send_response(seq, command, false, json_value(json_value::object_type{}), e.what());
        }
    }
    return 0;
}

void dap_frontend::on_stop(const stop_snapshot& stop) {
    if (suppress_stop_events_) {
        return;
    }

    if (stop.reason == xdbgstub::stop_reason::exited) {
        emit_exited_event(0);
        emit_terminated_event();
        return;
    }

    json_value::object_type body = {
        {"reason", stop_reason(stop)},
        {"threadId", int64_t{1}},
        {"allThreadsStopped", true}
    };
    if (stop.source.has_value() && stop.source->function_name.has_value()) {
        body["description"] = stop.source->function_name.value();
    }
    send_event("stopped", body);
}

void dap_frontend::on_remote_connected(const std::string& target) {
    connected_ = true;
    emit_process_event(target);
    emit_thread_event("started");
}

void dap_frontend::on_detached() {
    connected_ = false;
    emit_thread_event("exited");
    emit_terminated_event();
}

bool dap_frontend::read_message(json_value& body) {
    std::string line;
    std::size_t content_length = 0;
    bool saw_header = false;

    while (std::getline(std::cin, line)) {
        saw_header = true;
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty()) {
            break;
        }

        const auto colon = line.find(':');
        if (colon == std::string::npos) {
            throw std::runtime_error("invalid DAP header");
        }

        const auto name = trim(line.substr(0, colon));
        const auto value = trim(line.substr(colon + 1));
        if (name == "Content-Length") {
            content_length = static_cast<std::size_t>(parse_u32(value));
        }
    }

    if (!saw_header) {
        return false;
    }
    if (content_length == 0) {
        throw std::runtime_error("missing Content-Length header");
    }

    std::string payload(content_length, '\0');
    std::cin.read(payload.data(), static_cast<std::streamsize>(content_length));
    if (std::cin.gcount() != static_cast<std::streamsize>(content_length)) {
        throw std::runtime_error("truncated DAP payload");
    }
    body = json_value::parse(payload);
    return true;
}

void dap_frontend::write_json_message(const json_value& body) {
    const auto text = body.stringify();
    std::cout << "Content-Length: " << text.size() << "\r\n\r\n" << text << std::flush;
}

void dap_frontend::send_response(
    int seq,
    const std::string& command,
    bool success,
    json_value body,
    const std::string& message)
{
    json_value::object_type response = {
        {"seq", int64_t{next_seq_++}},
        {"type", "response"},
        {"request_seq", int64_t{seq}},
        {"success", success},
        {"command", command},
        {"body", std::move(body)}
    };
    if (!message.empty()) {
        response["message"] = message;
    }
    write_json_message(json_value(std::move(response)));
}

void dap_frontend::send_event(const std::string& event, json_value body) {
    write_json_message(json_value(json_value::object_type{
        {"seq", int64_t{next_seq_++}},
        {"type", "event"},
        {"event", event},
        {"body", std::move(body)}
    }));
}

dap_frontend::request_envelope dap_frontend::parse_request(const json_value& body) const {
    if (!body.is_object()) {
        throw std::runtime_error("DAP payload must be an object");
    }
    if (require_string(body, "type") != "request") {
        throw std::runtime_error("DAP payload is not a request");
    }

    request_envelope request;
    request.seq = require_int(body, "seq");
    request.command = require_string(body, "command");
    if (const auto* arguments = optional_object(body, "arguments"); arguments != nullptr) {
        request.arguments = *arguments;
    } else {
        request.arguments = json_value(json_value::object_type{});
    }
    return request;
}

void dap_frontend::handle_request(const json_value& body) {
    const auto request = parse_request(body);

    if (request.command == "initialize") {
        handle_initialize(request.seq);
    } else if (request.command == "launch" || request.command == "attach") {
        handle_launch_or_attach(request.seq, request.command, request.arguments);
    } else if (request.command == "configurationDone") {
        handle_configuration_done(request.seq);
    } else if (request.command == "setBreakpoints") {
        handle_set_breakpoints(request.seq, request.arguments);
    } else if (request.command == "setFunctionBreakpoints") {
        handle_set_function_breakpoints(request.seq, request.arguments);
    } else if (request.command == "setExceptionBreakpoints") {
        handle_set_exception_breakpoints(request.seq);
    } else if (request.command == "threads") {
        handle_threads(request.seq);
    } else if (request.command == "stackTrace") {
        handle_stack_trace(request.seq, request.arguments);
    } else if (request.command == "scopes") {
        handle_scopes(request.seq, request.arguments);
    } else if (request.command == "variables") {
        handle_variables(request.seq, request.arguments);
    } else if (request.command == "continue") {
        handle_continue(request.seq);
    } else if (request.command == "next" || request.command == "stepIn" ||
               request.command == "stepOut") {
        handle_next(request.seq, request.command);
    } else if (request.command == "pause") {
        handle_pause(request.seq);
    } else if (request.command == "disconnect") {
        handle_disconnect(request.seq);
    } else if (request.command == "source") {
        handle_source(request.seq, request.arguments);
    } else if (request.command == "evaluate") {
        handle_evaluate(request.seq, request.arguments);
    } else if (request.command == "loadedSources") {
        handle_loaded_sources(request.seq);
    } else {
        send_response(request.seq, request.command, false,
            json_value(json_value::object_type{}),
            "unsupported DAP command");
    }
}

void dap_frontend::handle_initialize(int seq) {
    initialized_ = true;
    send_response(seq, "initialize", true, json_value(json_value::object_type{
        {"supportsConfigurationDoneRequest", true},
        {"supportsFunctionBreakpoints", true},
        {"supportsEvaluateForHovers", true},
        {"supportsLoadedSourcesRequest", true},
        {"supportsPauseRequest", true},
        {"supportsStepBack", false},
        {"supportsDisassembleRequest", false},
        {"exceptionBreakpointFilters", json_value(json_value::array_type{})}
    }));
    send_event("initialized");
}

void dap_frontend::handle_launch_or_attach(
    int seq, const std::string& command, const json_value& arguments)
{
    if (const auto program = optional_string(arguments, "program"); program.has_value()) {
        debugger_.set_exec_path(program.value());
    }
    if (const auto symbols = optional_string(arguments, "symbols"); symbols.has_value()) {
        debugger_.load_symbols_file(symbols.value());
    } else {
        debugger_.maybe_load_default_symbols();
    }
    if (const auto remote = optional_string(arguments, "remoteTarget"); remote.has_value()) {
        debugger_.connect_remote(remote.value());
    }
    send_response(seq, command, true);

    if (json_get_boolean(arguments, "stopOnEntry").value_or(false) && connected_) {
        on_stop(debugger_.session().status());
    }
}

void dap_frontend::handle_set_breakpoints(int seq, const json_value& arguments) {
    const auto* source = optional_object(arguments, "source");
    if (source == nullptr) {
        throw std::runtime_error("setBreakpoints requires source");
    }
    const auto source_path = source_path_from_value(*source);

    clear_source_breakpoints(source_path);
    json_value::array_type result_breakpoints;
    for (const auto& line_value : collect_breakpoint_lines(arguments)) {
        const int requested_line = static_cast<int>(line_value.as_number());
        const auto location = source_path + ":" + std::to_string(requested_line);
        const auto resolved = debugger_.session().info_line_argument(location);
        const auto resolved_location =
            resolved.location.file_path + ":" + std::to_string(resolved.location.line);
        const auto breakpoint = debugger_.add_breakpoint(resolved_location);
        source_breakpoint_ids_[source_path].push_back(breakpoint.id);
        result_breakpoints.push_back(
            make_breakpoint_object(breakpoint, static_cast<int>(resolved.location.line)));
    }

    send_response(seq, "setBreakpoints", true, json_value(json_value::object_type{
        {"breakpoints", json_value(std::move(result_breakpoints))}
    }));
}

void dap_frontend::handle_set_function_breakpoints(int seq, const json_value& arguments) {
    for (int id : function_breakpoint_ids_) {
        debugger_.delete_breakpoint(id);
    }
    function_breakpoint_ids_.clear();

    json_value::array_type result_breakpoints;
    if (const auto* breakpoints = optional_array(arguments, "breakpoints"); breakpoints != nullptr) {
        for (const auto& item : *breakpoints) {
            if (!item.is_object()) {
                continue;
            }
            const auto breakpoint = debugger_.add_breakpoint(require_string(item, "name"));
            function_breakpoint_ids_.push_back(breakpoint.id);
            result_breakpoints.push_back(make_breakpoint_object(breakpoint, 0));
        }
    }

    send_response(seq, "setFunctionBreakpoints", true, json_value(json_value::object_type{
        {"breakpoints", json_value(std::move(result_breakpoints))}
    }));
}

void dap_frontend::handle_set_exception_breakpoints(int seq) {
    send_response(seq, "setExceptionBreakpoints", true, json_value(json_value::object_type{
        {"breakpoints", json_value(json_value::array_type{})}
    }));
}

void dap_frontend::handle_threads(int seq) {
    send_response(seq, "threads", true, json_value(json_value::object_type{
        {"threads", json_value(json_value::array_type{
            json_value(json_value::object_type{
                {"id", int64_t{1}},
                {"name", "main"}
            })
        })}
    }));
}

void dap_frontend::handle_stack_trace(int seq, const json_value& arguments) {
    const int start_frame = optional_int(arguments, "startFrame").value_or(0);
    const int levels = optional_int(arguments, "levels").value_or(1);
    json_value::array_type frames;
    if (start_frame == 0 && levels != 0) {
        frames.push_back(make_stack_frame_object(debugger_.session().status(), 1));
    }

    send_response(seq, "stackTrace", true, json_value(json_value::object_type{
        {"stackFrames", json_value(std::move(frames))},
        {"totalFrames", int64_t{1}}
    }));
}

void dap_frontend::handle_scopes(int seq, const json_value& arguments) {
    static_cast<void>(require_int(arguments, "frameId"));
    const auto stop = debugger_.session().status();
    const auto locals_ref = new_variable_reference({variable_reference::kind::locals, stop.pc});
    const auto globals_ref = new_variable_reference({variable_reference::kind::globals, stop.pc});
    const auto registers_ref = new_variable_reference({variable_reference::kind::registers, stop.pc});
    send_response(seq, "scopes", true, json_value(json_value::object_type{
        {"scopes", json_value(json_value::array_type{
            make_scope_object("Locals", locals_ref, "locals"),
            make_scope_object("Globals", globals_ref, "globals"),
            make_scope_object("Registers", registers_ref, "registers")
        })}
    }));
}

void dap_frontend::handle_variables(int seq, const json_value& arguments) {
    const auto ref = require_int(arguments, "variablesReference");
    const auto it = variable_references_.find(ref);
    if (it == variable_references_.end()) {
        throw std::runtime_error("unknown variables reference");
    }

    json_value::array_type items;
    if (it->second.type == variable_reference::kind::locals) {
        for (const auto* var : debugger_.session().visible_variables(it->second.pc)) {
            items.push_back(make_variable_object(
                var->name,
                read_variable_value(debugger_.session(), *var).value_or("<unavailable>"),
                var->type_name));
        }
    } else if (it->second.type == variable_reference::kind::globals) {
        if (const auto* symbols = debugger_.session().symbols(); symbols != nullptr) {
            for (const auto& symbol : symbols->symbols) {
                if (symbol.kind != xdbg::symbol_kind::object &&
                    symbol.kind != xdbg::symbol_kind::global) {
                    continue;
                }
                items.push_back(make_variable_object(
                    normalize_c_symbol_name(symbol.name),
                    read_global_symbol_value(debugger_.session(), symbol)
                        .value_or(hex_u32(symbol.address)),
                    symbol.type_name));
            }
        }
    } else {
        for (const auto& reg : debugger_.session().register_values()) {
            items.push_back(make_variable_object(reg.first, hex_u32(reg.second)));
        }
    }

    send_response(seq, "variables", true, json_value(json_value::object_type{
        {"variables", json_value(std::move(items))}
    }));
}

void dap_frontend::handle_continue(int seq) {
    emit_continued_event();
    suppress_stop_events_ = true;
    auto current = debugger_.session().status();
    if (current.reason == xdbgstub::stop_reason::breakpoint &&
        has_breakpoint_at(current.pc)) {
        current = debugger_.step_instruction();
        if (current.reason == xdbgstub::stop_reason::exited) {
            suppress_stop_events_ = false;
            send_response(seq, "continue", true, json_value(json_value::object_type{
                {"allThreadsContinued", true}
            }));
            on_stop(current);
            return;
        }
    }

    const auto stop = debugger_.continue_execution();
    suppress_stop_events_ = false;
    send_response(seq, "continue", true, json_value(json_value::object_type{
        {"allThreadsContinued", true}
    }));
    on_stop(stop);
}

void dap_frontend::handle_next(int seq, const std::string& command) {
    emit_continued_event();
    suppress_stop_events_ = true;
    const auto stop = step_to_next_source_stop(command);
    suppress_stop_events_ = false;
    send_response(seq, command, true);
    on_stop(stop);
}

void dap_frontend::handle_pause(int seq) {
    suppress_stop_events_ = true;
    const auto stop = debugger_.pause_execution();
    suppress_stop_events_ = false;
    send_response(seq, "pause", true);
    on_stop(stop);
}

void dap_frontend::handle_disconnect(int seq) {
    if (connected_ || debugger_.session().is_connected()) {
        debugger_.detach();
    }
    send_response(seq, "disconnect", true);
}

void dap_frontend::handle_configuration_done(int seq) {
    send_response(seq, "configurationDone", true);
    if (connected_) {
        on_stop(debugger_.session().status());
    }
}

void dap_frontend::handle_source(int seq, const json_value& arguments) {
    const auto* source = optional_object(arguments, "source");
    if (source == nullptr) {
        throw std::runtime_error("source request requires source");
    }
    const auto path = source_path_from_value(*source);

    std::ifstream input(path);
    if (!input.is_open()) {
        throw std::runtime_error("cannot open source file");
    }
    std::ostringstream content;
    content << input.rdbuf();
    send_response(seq, "source", true, json_value(json_value::object_type{
        {"content", content.str()},
        {"mimeType", "text/plain"}
    }));
}

void dap_frontend::handle_evaluate(int seq, const json_value& arguments) {
    const auto expr = require_string(arguments, "expression");
    if (const auto value = evaluate_simple_expression(debugger_.session(), expr);
        value.has_value()) {
        json_value::object_type body = {
            {"result", value->result},
            {"variablesReference", int64_t{0}}
        };
        if (value->type_name.has_value()) {
            body["type"] = value->type_name.value();
        }
        send_response(seq, "evaluate", true, json_value(std::move(body)));
        return;
    }

    const auto address = debugger_.session().resolve_address_expression(expr);
    send_response(seq, "evaluate", true, json_value(json_value::object_type{
        {"result", hex_u32(address)},
        {"variablesReference", int64_t{0}}
    }));
}

void dap_frontend::handle_loaded_sources(int seq) {
    json_value::array_type sources;
    if (const auto* symbols = debugger_.session().symbols(); symbols != nullptr) {
        std::set<std::string> seen;
        for (const auto& file : symbols->files) {
            if (seen.insert(file.path).second) {
                sources.push_back(make_source_object(file.path));
            }
        }
    }

    send_response(seq, "loadedSources", true, json_value(json_value::object_type{
        {"sources", json_value(std::move(sources))}
    }));
}

std::string dap_frontend::require_string(const json_value& object, const std::string& key) const {
    if (!object.is_object()) {
        throw std::runtime_error("expected JSON object");
    }
    return object.at(key).as_string();
}

int dap_frontend::require_int(const json_value& object, const std::string& key) const {
    if (!object.is_object()) {
        throw std::runtime_error("expected JSON object");
    }
    return static_cast<int>(object.at(key).as_number());
}

bool dap_frontend::require_bool(const json_value& object, const std::string& key) const {
    if (!object.is_object()) {
        throw std::runtime_error("expected JSON object");
    }
    return object.at(key).as_boolean();
}

std::optional<std::string> dap_frontend::optional_string(
    const json_value& object, const std::string& key) const
{
    return json_get_string(object, key);
}

std::optional<int> dap_frontend::optional_int(
    const json_value& object, const std::string& key) const
{
    if (const auto value = json_get_number(object, key); value.has_value()) {
        return static_cast<int>(value.value());
    }
    return std::nullopt;
}

const json_value* dap_frontend::optional_object(
    const json_value& object, const std::string& key) const
{
    if (const auto* value = object.find(key); value != nullptr && value->is_object()) {
        return value;
    }
    return nullptr;
}

const json_value::array_type* dap_frontend::optional_array(
    const json_value& object, const std::string& key) const
{
    if (const auto* value = object.find(key); value != nullptr && value->is_array()) {
        return &value->as_array();
    }
    return nullptr;
}

std::string dap_frontend::source_path_from_value(const json_value& source) const {
    if (const auto path = optional_string(source, "path"); path.has_value()) {
        return path.value();
    }
    if (const auto name = optional_string(source, "name"); name.has_value()) {
        return name.value();
    }
    throw std::runtime_error("source object is missing path/name");
}

json_value dap_frontend::make_source_object(const source_location& source) const {
    return make_source_object(source.file_path);
}

json_value dap_frontend::make_source_object(const std::string& path) const {
    return json_value(json_value::object_type{
        {"name", std::filesystem::path(path).filename().string()},
        {"path", path}
    });
}

json_value dap_frontend::make_breakpoint_object(
    const breakpoint_entry& breakpoint,
    int requested_line) const
{
    json_value::object_type object = {
        {"id", int64_t{breakpoint.id}},
        {"verified", true}
    };
    if (requested_line > 0) {
        object["line"] = int64_t{requested_line};
    }
    return json_value(std::move(object));
}

json_value dap_frontend::make_stack_frame_object(
    const stop_snapshot& stop,
    int frame_id) const
{
    json_value::object_type object = {
        {"id", int64_t{frame_id}},
        {"name", stop.source.has_value()
            ? stop.source->function_name.value_or("<unknown>")
            : std::string("<unknown>")},
        {"line", int64_t{stop.source.has_value() ? stop.source->line : 1}},
        {"column", int64_t{stop.source.has_value() ? stop.source->column : 1}},
        {"instructionPointerReference", hex_u32(stop.pc)}
    };
    if (stop.source.has_value()) {
        object["source"] = make_source_object(stop.source.value());
    }
    return json_value(std::move(object));
}

json_value dap_frontend::make_scope_object(
    const std::string& name,
    int variables_reference,
    const std::string& presentation_hint) const
{
    return json_value(json_value::object_type{
        {"name", name},
        {"variablesReference", int64_t{variables_reference}},
        {"presentationHint", presentation_hint},
        {"expensive", false}
    });
}

json_value dap_frontend::make_variable_object(
    const std::string& name,
    const std::string& value,
    const std::optional<std::string>& type_name) const
{
    json_value::object_type object = {
        {"name", name},
        {"value", value},
        {"variablesReference", int64_t{0}}
    };
    if (type_name.has_value() && !type_name->empty()) {
        object["type"] = type_name.value();
    }
    return json_value(std::move(object));
}

stop_snapshot dap_frontend::step_to_next_source_stop(const std::string& command) {
    const auto start = debugger_.session().status();
    const auto start_regs = debugger_.session().read_registers();
    constexpr int max_instruction_steps = 100000;

    auto same_source = [](const std::optional<source_location>& lhs,
                          const std::optional<source_location>& rhs) {
        if (!lhs.has_value() || !rhs.has_value()) {
            return false;
        }
        return lhs->file_path == rhs->file_path &&
               lhs->line == rhs->line &&
               lhs->column == rhs->column &&
               lhs->function_name == rhs->function_name;
    };

    auto same_function = [](const std::optional<source_location>& lhs,
                            const std::optional<source_location>& rhs) {
        if (!lhs.has_value() || !rhs.has_value()) {
            return false;
        }
        return lhs->function_name == rhs->function_name;
    };

    if (!start.source.has_value()) {
        return debugger_.step_instruction();
    }

    stop_snapshot stop = start;
    for (int i = 0; i < max_instruction_steps; ++i) {
        stop = debugger_.step_instruction();
        if (has_breakpoint_at(stop.pc) && stop.pc != start.pc) {
            stop.reason = xdbgstub::stop_reason::breakpoint;
            return stop;
        }
        if (stop.reason == xdbgstub::stop_reason::breakpoint ||
            stop.reason == xdbgstub::stop_reason::exited ||
            stop.reason == xdbgstub::stop_reason::halted) {
            return stop;
        }

        const auto regs = debugger_.session().read_registers();

        if (command == "stepOut") {
            if (regs.sp > start_regs.sp) {
                return stop;
            }
            if (!same_function(stop.source, start.source)) {
                return stop;
            }
            continue;
        }

        if (command == "next" && regs.sp < start_regs.sp) {
            continue;
        }

        if (!same_source(stop.source, start.source)) {
            return stop;
        }
    }

    return stop;
}

bool dap_frontend::has_breakpoint_at(uint32_t pc) const {
    for (const auto& breakpoint : debugger_.session().breakpoints()) {
        if (breakpoint.address == pc) {
            return true;
        }
    }
    return false;
}

std::string dap_frontend::stop_reason(const stop_snapshot& stop) const {
    switch (stop.reason) {
    case xdbgstub::stop_reason::breakpoint: return "breakpoint";
    case xdbgstub::stop_reason::step: return "step";
    case xdbgstub::stop_reason::pause: return "pause";
    case xdbgstub::stop_reason::halted: return "pause";
    case xdbgstub::stop_reason::exited: return "exited";
    case xdbgstub::stop_reason::signal: return "exception";
    case xdbgstub::stop_reason::none: return "pause";
    }
    return "pause";
}

int dap_frontend::new_variable_reference(variable_reference ref) {
    const int id = next_variable_reference_++;
    variable_references_[id] = ref;
    return id;
}

void dap_frontend::clear_source_breakpoints(const std::string& source_path) {
    const auto it = source_breakpoint_ids_.find(source_path);
    if (it == source_breakpoint_ids_.end()) {
        return;
    }
    for (int id : it->second) {
        debugger_.delete_breakpoint(id);
    }
    source_breakpoint_ids_.erase(it);
}

json_value::array_type dap_frontend::collect_breakpoint_lines(
    const json_value& arguments) const
{
    json_value::array_type result;
    if (const auto* breakpoints = optional_array(arguments, "breakpoints"); breakpoints != nullptr) {
        for (const auto& item : *breakpoints) {
            if (item.is_object()) {
                if (const auto line = optional_int(item, "line"); line.has_value()) {
                    result.push_back(json_value(int64_t{line.value()}));
                }
            }
        }
    }
    return result;
}

void dap_frontend::emit_process_event(const std::string& target) {
    send_event("process", json_value(json_value::object_type{
        {"name", target},
        {"systemProcessId", int64_t{1}},
        {"isLocalProcess", false},
        {"startMethod", "attach"}
    }));
}

void dap_frontend::emit_thread_event(const std::string& reason) {
    send_event("thread", json_value(json_value::object_type{
        {"reason", reason},
        {"threadId", int64_t{1}}
    }));
}

void dap_frontend::emit_continued_event() {
    send_event("continued", json_value(json_value::object_type{
        {"threadId", int64_t{1}},
        {"allThreadsContinued", true}
    }));
}

void dap_frontend::emit_exited_event(int exit_code) {
    send_event("exited", json_value(json_value::object_type{
        {"exitCode", int64_t{exit_code}}
    }));
}

void dap_frontend::emit_terminated_event() {
    send_event("terminated");
}

std::unique_ptr<debug_protocol> make_dap_protocol(debugger_host& host) {
    return std::make_unique<dap_frontend>(host);
}
