/*
 * Implements the MI debug protocol frontend and adapts debugger
 * session events and commands to GDB/MI-style I/O.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */

#include "frontends.hpp"

#include <cctype>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace {

    struct mi_command {
        std::string token;
        std::string name;
        std::vector<std::string> args;
    };

    std::string trim(const std::string& value) {
        const auto start = value.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) {
            return "";
        }
        const auto end = value.find_last_not_of(" \t\r\n");
        return value.substr(start, end - start + 1);
    }

    std::vector<std::string> split_command(const std::string& line) {
        std::vector<std::string> tokens;
        std::size_t pos = 0;
        while (pos < line.size()) {
            while (pos < line.size() &&
                   std::isspace(static_cast<unsigned char>(line[pos]))) {
                ++pos;
            }
            if (pos >= line.size()) {
                break;
            }

            if (line[pos] == '"') {
                ++pos;
                std::string token;
                bool escaped = false;
                while (pos < line.size()) {
                    const char ch = line[pos++];
                    if (escaped) {
                        token.push_back(ch);
                        escaped = false;
                    } else if (ch == '\\') {
                        escaped = true;
                    } else if (ch == '"') {
                        break;
                    } else {
                        token.push_back(ch);
                    }
                }
                tokens.push_back(token);
                continue;
            }

            const std::size_t start = pos;
            while (pos < line.size() &&
                   !std::isspace(static_cast<unsigned char>(line[pos]))) {
                ++pos;
            }
            tokens.push_back(line.substr(start, pos - start));
        }
        return tokens;
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

    std::string escape_c_string(const std::string& value) {
        std::string result;
        for (char ch : value) {
            switch (ch) {
            case '\\': result += "\\\\"; break;
            case '"': result += "\\\""; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default: result.push_back(ch); break;
            }
        }
        return result;
    }

    std::string mi_string(const std::string& value) {
        return "\"" + escape_c_string(value) + "\"";
    }

    std::string mi_tuple(const std::vector<std::pair<std::string, std::string>>& fields) {
        std::ostringstream out;
        out << "{";
        for (std::size_t i = 0; i < fields.size(); ++i) {
            if (i != 0) {
                out << ",";
            }
            out << fields[i].first << "=" << fields[i].second;
        }
        out << "}";
        return out.str();
    }

    std::string mi_list(const std::vector<std::string>& items) {
        std::ostringstream out;
        out << "[";
        for (std::size_t i = 0; i < items.size(); ++i) {
            if (i != 0) {
                out << ",";
            }
            out << items[i];
        }
        out << "]";
        return out.str();
    }

    std::string frame_tuple(const stop_snapshot& snapshot) {
        std::vector<std::pair<std::string, std::string>> fields = {
            {"level", mi_string("0")},
            {"addr", mi_string(hex_u32(snapshot.pc))}
        };
        if (snapshot.source.has_value()) {
            if (snapshot.source->function_name.has_value()) {
                fields.push_back({"func", mi_string(snapshot.source->function_name.value())});
            }
            fields.push_back({"file", mi_string(std::filesystem::path(snapshot.source->file_path).filename().string())});
            fields.push_back({"fullname", mi_string(snapshot.source->file_path)});
            fields.push_back({"line", mi_string(std::to_string(snapshot.source->line))});
        }
        return mi_tuple(fields);
    }

    std::string mi_stop_reason(xdbgstub::stop_reason reason) {
        switch (reason) {
        case xdbgstub::stop_reason::breakpoint: return "breakpoint-hit";
        case xdbgstub::stop_reason::step: return "end-stepping-range";
        case xdbgstub::stop_reason::pause: return "signal-received";
        case xdbgstub::stop_reason::halted: return "signal-received";
        case xdbgstub::stop_reason::exited: return "exited-normally";
        case xdbgstub::stop_reason::signal: return "signal-received";
        case xdbgstub::stop_reason::none: return "end-stepping-range";
        }
        return "signal-received";
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

    mi_command parse_mi_command(const std::string& line) {
        mi_command result;
        std::size_t pos = 0;
        while (pos < line.size() &&
               std::isdigit(static_cast<unsigned char>(line[pos]))) {
            result.token.push_back(line[pos++]);
        }

        while (pos < line.size() &&
               std::isspace(static_cast<unsigned char>(line[pos]))) {
            ++pos;
        }

        if (pos < line.size() && line[pos] == '-') {
            ++pos;
        }

        const std::size_t name_start = pos;
        while (pos < line.size() &&
               !std::isspace(static_cast<unsigned char>(line[pos]))) {
            ++pos;
        }
        result.name = line.substr(name_start, pos - name_start);
        result.args = split_command(line.substr(pos));
        return result;
    }

} // namespace

mi_frontend::mi_frontend(debugger_host& host)
    : debugger_(host)
{
    debugger_.set_event_sink(this);
}

int mi_frontend::run() {
    if (debugger_.session().is_connected()) {
        std::cout << "=thread-group-added,id=" << mi_string("i1") << "\n";
        std::cout << "=thread-group-started,id=" << mi_string("i1")
                  << ",pid=" << mi_string("1") << "\n";
        std::cout << "=thread-created,id=" << mi_string("1")
                  << ",group-id=" << mi_string("i1") << "\n";
        std::cout << "=thread-selected,id=" << mi_string("1") << "\n";
    }
    std::cout << "(gdb)\n" << std::flush;

    std::string line;
    while (std::getline(std::cin, line)) {
        std::string token;
        for (char ch : line) {
            if (!std::isdigit(static_cast<unsigned char>(ch))) {
                break;
            }
            token.push_back(ch);
        }
        try {
            if (!handle_line(line)) {
                break;
            }
        } catch (const std::exception& e) {
            std::cout << token << "^error,msg=" << mi_string(e.what()) << "\n";
            std::cout << "(gdb)\n" << std::flush;
        }
    }
    return 0;
}

bool mi_frontend::handle_line(const std::string& line) {
    const auto command = parse_mi_command(trim(line));
    const std::string token = command.token;

    if (command.name.empty()) {
        std::cout << token << "^done\n(gdb)\n" << std::flush;
        return true;
    }

    if (command.name == "gdb-version") {
        std::cout << token << "^done,version=" << mi_string("xdbg 0.1") << "\n";
        std::cout << "(gdb)\n" << std::flush;
        return true;
    }

    if (command.name == "list-features") {
        std::cout << token << "^done,features=[]\n";
        std::cout << "(gdb)\n" << std::flush;
        return true;
    }

    if (command.name == "enable-pretty-printing") {
        std::cout << token << "^done\n";
        std::cout << "(gdb)\n" << std::flush;
        return true;
    }

    if (command.name == "gdb-set") {
        std::cout << token << "^done\n";
        std::cout << "(gdb)\n" << std::flush;
        return true;
    }

    if (command.name == "gdb-show") {
        std::string value = "off";
        if (!command.args.empty()) {
            const auto& key = command.args[0];
            if (key == "auto-solib-add") {
                value = "on";
            } else if (key == "language") {
                value = "c";
            } else if (key == "new-console") {
                value = "off";
            } else if (key == "solib-search-path") {
                value = "";
            } else if (key == "stop-on-solib-events") {
                value = "0";
            }
        }
        std::cout << token << "^done,value=" << mi_string(value) << "\n";
        std::cout << "(gdb)\n" << std::flush;
        return true;
    }

    if (command.name == "environment-cd") {
        if (command.args.empty()) {
            throw std::runtime_error("environment-cd requires a path");
        }
        std::filesystem::current_path(command.args[0]);
        std::cout << token << "^done\n";
        std::cout << "(gdb)\n" << std::flush;
        return true;
    }

    if (command.name == "file-exec-and-symbols") {
        if (command.args.empty()) {
            throw std::runtime_error("file-exec-and-symbols requires a path");
        }
        debugger_.set_exec_path(command.args[0]);
        debugger_.maybe_load_default_symbols();
        std::cout << token << "^done\n";
        std::cout << "(gdb)\n" << std::flush;
        return true;
    }

    if (command.name == "target-select") {
        if (command.args.size() < 2 || command.args[0] != "remote") {
            throw std::runtime_error("target-select currently supports only remote HOST:PORT");
        }
        debugger_.connect_remote(command.args[1]);
        std::cout << "=thread-group-added,id=" << mi_string("i1") << "\n";
        std::cout << "=thread-group-started,id=" << mi_string("i1")
                  << ",pid=" << mi_string("1") << "\n";
        std::cout << "=thread-created,id=" << mi_string("1")
                  << ",group-id=" << mi_string("i1") << "\n";
        std::cout << "=thread-selected,id=" << mi_string("1") << "\n";
        std::cout << token << "^connected\n";
        std::cout << "(gdb)\n" << std::flush;
        return true;
    }

    if (command.name == "break-insert") {
        std::string location;
        for (const auto& arg : command.args) {
            if (!arg.empty() && arg[0] != '-') {
                location = arg;
            }
        }
        if (location.empty()) {
            throw std::runtime_error("break-insert requires a location");
        }
        const auto breakpoint = debugger_.add_breakpoint(location);
        std::cout << token << "^done,bkpt="
                  << mi_tuple({
                      {"number", mi_string(std::to_string(breakpoint.id))},
                      {"type", mi_string("breakpoint")},
                      {"disp", mi_string("keep")},
                      {"enabled", mi_string("y")},
                      {"addr", mi_string(hex_u32(breakpoint.address))},
                      {"func", mi_string(location)}
                  }) << "\n";
        std::cout << "(gdb)\n" << std::flush;
        return true;
    }

    if (command.name == "break-delete") {
        for (const auto& arg : command.args) {
            if (!arg.empty() && arg[0] != '-') {
                debugger_.delete_breakpoint(static_cast<int>(parse_u32(arg)));
            }
        }
        std::cout << token << "^done\n";
        std::cout << "(gdb)\n" << std::flush;
        return true;
    }

    if (command.name == "exec-run" || command.name == "exec-continue") {
        std::cout << token << "^running\n";
        std::cout << "*running,thread-id=" << mi_string("all") << "\n";
        stop_snapshot stop;
        if (command.name == "exec-run") {
            stop = debugger_.status();
            if (!stop.source.has_value()) {
                stop = debugger_.step_instruction();
            }
        } else {
            stop = debugger_.continue_execution();
        }
        std::cout << "*stopped,reason=" << mi_string(mi_stop_reason(stop.reason))
                  << ",thread-id=" << mi_string("1")
                  << ",current-thread-id=" << mi_string("1")
                  << ",stopped-threads=" << mi_string("all")
                  << ",frame=" << frame_tuple(stop) << "\n";
        std::cout << "(gdb)\n" << std::flush;
        return true;
    }

    if (command.name == "exec-step-instruction" ||
        command.name == "exec-next-instruction") {
        std::cout << token << "^running\n";
        std::cout << "*running,thread-id=" << mi_string("all") << "\n";
        const auto stop = debugger_.step_instruction();
        std::cout << "*stopped,reason=" << mi_string(mi_stop_reason(stop.reason))
                  << ",thread-id=" << mi_string("1")
                  << ",current-thread-id=" << mi_string("1")
                  << ",stopped-threads=" << mi_string("all")
                  << ",frame=" << frame_tuple(stop) << "\n";
        std::cout << "(gdb)\n" << std::flush;
        return true;
    }

    if (command.name == "exec-interrupt") {
        const auto stop = debugger_.pause_execution();
        std::cout << token << "^done\n";
        std::cout << "*stopped,reason=" << mi_string(mi_stop_reason(stop.reason))
                  << ",thread-id=" << mi_string("1")
                  << ",current-thread-id=" << mi_string("1")
                  << ",stopped-threads=" << mi_string("all")
                  << ",frame=" << frame_tuple(stop) << "\n";
        std::cout << "(gdb)\n" << std::flush;
        return true;
    }

    if (command.name == "stack-info-depth") {
        std::cout << token << "^done,depth=" << mi_string("1") << "\n";
        std::cout << "(gdb)\n" << std::flush;
        return true;
    }

    if (command.name == "stack-list-frames") {
        const auto stop = debugger_.status();
        std::cout << token << "^done,stack="
                  << mi_list({frame_tuple(stop)}) << "\n";
        std::cout << "(gdb)\n" << std::flush;
        return true;
    }

    if (command.name == "stack-list-variables") {
        const auto regs = debugger_.session().read_registers();
        const auto vars = debugger_.session().visible_variables(regs.pc);
        std::vector<std::string> items;
        for (const auto* var : vars) {
            std::vector<std::pair<std::string, std::string>> fields = {
                {"name", mi_string(var->name)}
            };
            if (var->type_name.has_value()) {
                fields.push_back({"type", mi_string(var->type_name.value())});
            }
            if (const auto value = stringify_storage(*var); value.has_value()) {
                fields.push_back({"value", mi_string(value.value())});
            }
            items.push_back(mi_tuple(fields));
        }
        std::cout << token << "^done,variables=" << mi_list(items) << "\n";
        std::cout << "(gdb)\n" << std::flush;
        return true;
    }

    if (command.name == "stack-select-frame") {
        std::cout << token << "^done\n";
        std::cout << "(gdb)\n" << std::flush;
        return true;
    }

    if (command.name == "data-list-register-values") {
        std::vector<std::string> items;
        const auto registers = debugger_.session().register_values();
        for (std::size_t i = 0; i < registers.size(); ++i) {
            items.push_back(mi_tuple({
                {"number", mi_string(std::to_string(i))},
                {"value", mi_string(hex_u32(registers[i].second))}
            }));
        }
        std::cout << token << "^done,register-values=" << mi_list(items) << "\n";
        std::cout << "(gdb)\n" << std::flush;
        return true;
    }

    if (command.name == "data-read-memory-bytes") {
        if (command.args.size() < 2) {
            throw std::runtime_error("data-read-memory-bytes requires ADDRESS COUNT");
        }
        const auto address = debugger_.session().resolve_address_expression(command.args[0]);
        const auto count = static_cast<std::size_t>(parse_u32(command.args[1]));
        const auto data = debugger_.session().read_memory(address, count);
        std::ostringstream bytes;
        for (uint8_t byte : data) {
            bytes << std::setw(2) << std::setfill('0')
                  << std::hex << std::nouppercase
                  << static_cast<int>(byte);
        }
        std::cout << token << "^done,memory=[{begin=" << mi_string(hex_u32(address))
                  << ",offset=" << mi_string("0")
                  << ",end=" << mi_string(hex_u32(address + static_cast<uint32_t>(count)))
                  << ",contents=" << mi_string(bytes.str()) << "}]\n";
        std::cout << "(gdb)\n" << std::flush;
        return true;
    }

    if (command.name == "data-evaluate-expression") {
        if (command.args.empty()) {
            throw std::runtime_error("data-evaluate-expression requires an expression");
        }
        const auto value = debugger_.session().resolve_address_expression(command.args[0]);
        std::cout << token << "^done,value=" << mi_string(hex_u32(value)) << "\n";
        std::cout << "(gdb)\n" << std::flush;
        return true;
    }

    if (command.name == "thread-info") {
        const auto stop = debugger_.status();
        std::cout << token << "^done,threads="
                  << mi_list({mi_tuple({
                      {"id", mi_string("1")},
                      {"target-id", mi_string("Thread 1")},
                      {"state", mi_string("stopped")},
                      {"frame", frame_tuple(stop)}
                  })})
                  << ",current-thread-id=" << mi_string("1") << "\n";
        std::cout << "(gdb)\n" << std::flush;
        return true;
    }

    if (command.name == "list-thread-groups") {
        std::cout << token << "^done,groups="
                  << mi_list({mi_tuple({
                      {"id", mi_string("i1")},
                      {"type", mi_string("process")},
                      {"pid", mi_string("1")},
                      {"executable", mi_string(debugger_.session().exec_path().has_value()
                          ? debugger_.session().exec_path()->string()
                          : "")}
                  })}) << "\n";
        std::cout << "(gdb)\n" << std::flush;
        return true;
    }

    if (command.name == "interpreter-exec") {
        if (command.args.size() < 2 || command.args[0] != "console") {
            throw std::runtime_error("interpreter-exec currently supports only console");
        }
        std::ostringstream console_output;
        std::ostringstream console_error;
        cli_frontend console(debugger_.session(), std::cin, console_output, console_error, true, false);
        console.execute_command(command.args[1]);

        std::istringstream out_lines(console_output.str());
        std::string text;
        while (std::getline(out_lines, text)) {
            std::cout << "~" << mi_string(text + "\n") << "\n";
        }

        std::istringstream err_lines(console_error.str());
        while (std::getline(err_lines, text)) {
            std::cout << "&" << mi_string(text + "\n") << "\n";
        }

        std::cout << token << "^done\n";
        std::cout << "(gdb)\n" << std::flush;
        return true;
    }

    if (command.name == "gdb-exit") {
        std::cout << token << "^exit\n";
        return false;
    }

    throw std::runtime_error("unsupported mi command: " + command.name);
}

std::unique_ptr<debug_protocol> make_mi_protocol(debugger_host& host) {
    return std::make_unique<mi_frontend>(host);
}
