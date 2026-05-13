#include "frontends.hpp"

#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
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

    std::string format_reason(xdbgstub::stop_reason reason) {
        switch (reason) {
        case xdbgstub::stop_reason::breakpoint: return "breakpoint";
        case xdbgstub::stop_reason::step: return "step";
        case xdbgstub::stop_reason::pause: return "pause";
        case xdbgstub::stop_reason::halted: return "halted";
        case xdbgstub::stop_reason::exited: return "exited";
        case xdbgstub::stop_reason::signal: return "signal";
        case xdbgstub::stop_reason::none: return "none";
        }
        return "none";
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

cli_frontend::cli_frontend(
    debugger_host& host,
    std::istream& input,
    std::ostream& output,
    std::ostream& error,
    bool quiet,
    bool show_prompt)
    : debugger_(host),
      input_(input),
      output_(output),
      error_(error),
      quiet_(quiet),
      show_prompt_(show_prompt)
{
    debugger_.set_event_sink(this);
}

void cli_frontend::set_execute_commands(std::vector<std::string> commands) {
    execute_commands_ = std::move(commands);
}

int cli_frontend::run() {
    if (!quiet_) {
        output_ << "xdbg 0.1\n";
        if (debugger_.session().symbols_path().has_value()) {
            output_ << "Loaded symbols from " << debugger_.session().symbols_path().value() << "\n";
        }
        if (debugger_.session().exec_path().has_value()) {
            output_ << "Using executable: " << debugger_.session().exec_path().value() << "\n";
        }
    }

    for (const auto& command : execute_commands_) {
        execute_command(command);
        if (should_quit_) {
            return 0;
        }
    }

    std::string line;
    while (!should_quit_) {
        if (show_prompt_) {
            output_ << "(xdbg) " << std::flush;
        }
        if (!std::getline(input_, line)) {
            break;
        }

        try {
            execute_command(line);
        } catch (const std::exception& e) {
            error_ << "error: " << e.what() << "\n";
        }
    }
    return 0;
}

bool cli_frontend::execute_command(const std::string& line) {
    const std::string trimmed = trim(line);
    if (trimmed.empty()) {
        return true;
    }

    const auto tokens = split_command(trimmed);
    if (tokens.empty()) {
        return true;
    }

    const std::string& head = tokens[0];

    if (head == "help") {
        output_
            << "commands:\n"
            << "  help\n"
            << "  quit | q\n"
            << "  target remote HOST:PORT\n"
            << "  symbol-file FILE\n"
            << "  file FILE\n"
            << "  break EXPR | b EXPR\n"
            << "  delete [ID]\n"
            << "  continue | c\n"
            << "  stepi | si | nexti | ni\n"
            << "  info registers\n"
            << "  info breakpoints\n"
            << "  info functions\n"
            << "  info files\n"
            << "  info line [LOC]\n"
            << "  info locals\n"
            << "  list [SYMBOL|FILE:LINE]\n"
            << "  disassemble [EXPR] [COUNT]\n"
            << "  x/Nxb ADDR\n"
            << "  x/Ni ADDR\n"
            << "  status\n"
            << "  detach\n"
            << "  show version\n"
            << "  set pagination off\n";
        return true;
    }

    if (head == "quit" || head == "q") {
        should_quit_ = true;
        return false;
    }

    if (head == "show" && tokens.size() >= 2 && tokens[1] == "version") {
        output_ << "xdbg 0.1\n";
        return true;
    }

    if (head == "set" && tokens.size() >= 3 &&
        tokens[1] == "pagination" && tokens[2] == "off") {
        return true;
    }

    if (head == "pwd") {
        output_ << std::filesystem::current_path() << "\n";
        return true;
    }

    if (head == "target") {
        if (tokens.size() < 3 || tokens[1] != "remote") {
            throw std::runtime_error("usage: target remote HOST:PORT");
        }
        debugger_.connect_remote(tokens[2]);
        output_ << "Connected to " << tokens[2] << "\n";
        return true;
    }

    if (head == "symbol-file") {
        if (tokens.size() < 2) {
            throw std::runtime_error("usage: symbol-file FILE");
        }
        debugger_.load_symbols_file(tokens[1]);
        output_ << "Loaded symbols from " << tokens[1] << "\n";
        return true;
    }

    if (head == "file") {
        if (tokens.size() < 2) {
            throw std::runtime_error("usage: file FILE");
        }
        debugger_.set_exec_path(tokens[1]);
        output_ << "Executable set to " << tokens[1] << "\n";
        return true;
    }

    if (head == "break" || head == "b") {
        if (tokens.size() < 2) {
            throw std::runtime_error("usage: break EXPR");
        }
        const auto breakpoint = debugger_.add_breakpoint(tokens[1]);
        output_ << "Breakpoint " << breakpoint.id
                << " at " << hex_u32(breakpoint.address) << "\n";
        return true;
    }

    if (head == "delete") {
        if (tokens.size() == 1) {
            if (debugger_.session().breakpoints().empty()) {
                output_ << "No breakpoints.\n";
            } else {
                debugger_.delete_all_breakpoints();
                output_ << "Deleted all breakpoints.\n";
            }
            return true;
        }
        debugger_.delete_breakpoint(static_cast<int>(parse_u32(tokens[1])));
        output_ << "Deleted breakpoint " << tokens[1] << "\n";
        return true;
    }

    if (head == "continue" || head == "c" || head == "run") {
        const auto stop = debugger_.continue_execution();
        if (stop.source.has_value()) {
            output_ << stop.source->function_name.value_or("<unknown>")
                    << " () at " << stop.source->file_path
                    << ":" << stop.source->line << "\n";
            if (stop.source_text.has_value()) {
                output_ << stop.source->line << "\t" << stop.source_text.value() << "\n";
            }
        } else {
            output_ << "Stopped: reason=" << format_reason(stop.reason)
                    << " pc=" << hex_u32(stop.pc) << "\n";
        }
        return true;
    }

    if (head == "stepi" || head == "si" || head == "nexti" || head == "ni") {
        const auto stop = debugger_.step_instruction();
        if (stop.source.has_value()) {
            output_ << stop.source->function_name.value_or("<unknown>")
                    << " () at " << stop.source->file_path
                    << ":" << stop.source->line << "\n";
            if (stop.source_text.has_value()) {
                output_ << stop.source->line << "\t" << stop.source_text.value() << "\n";
            }
        } else {
            output_ << "Stopped: reason=" << format_reason(stop.reason)
                    << " pc=" << hex_u32(stop.pc) << "\n";
        }
        return true;
    }

    if (head == "status") {
        const auto stop = debugger_.status();
        if (stop.source.has_value()) {
            output_ << stop.source->function_name.value_or("<unknown>")
                    << " () at " << stop.source->file_path
                    << ":" << stop.source->line << "\n";
        } else {
            output_ << "Stopped: reason=" << format_reason(stop.reason)
                    << " pc=" << hex_u32(stop.pc) << "\n";
        }
        return true;
    }

    if (head == "detach") {
        debugger_.detach();
        output_ << "Detached.\n";
        return true;
    }

    if (head == "info") {
        if (tokens.size() < 2) {
            throw std::runtime_error("usage: info SUBCOMMAND");
        }
        if (tokens[1] == "registers") {
            const auto regs = debugger_.session().read_registers();
            output_
                << "af " << hex_u32(regs.af) << "\n"
                << "bc " << hex_u32(regs.bc) << "\n"
                << "de " << hex_u32(regs.de) << "\n"
                << "hl " << hex_u32(regs.hl) << "\n"
                << "ix " << hex_u32(regs.ix) << "\n"
                << "iy " << hex_u32(regs.iy) << "\n"
                << "sp " << hex_u32(regs.sp) << "\n"
                << "pc " << hex_u32(regs.pc) << "\n"
                << "i  " << hex_u32(regs.i) << "\n"
                << "r  " << hex_u32(regs.r) << "\n"
                << "iff1 " << regs.iff1 << "\n"
                << "iff2 " << regs.iff2 << "\n"
                << "halted " << regs.halted << "\n";
            return true;
        }
        if (tokens[1] == "breakpoints") {
            if (debugger_.session().breakpoints().empty()) {
                output_ << "No breakpoints.\n";
            } else {
                for (const auto& breakpoint : debugger_.session().breakpoints()) {
                    output_ << breakpoint.id
                            << "  " << hex_u32(breakpoint.address)
                            << "  " << breakpoint.expression << "\n";
                }
            }
            return true;
        }
        if (tokens[1] == "functions") {
            if (!debugger_.session().symbols()) {
                throw std::runtime_error("no symbols loaded");
            }
            for (const auto& function : debugger_.session().symbols()->functions) {
                output_ << hex_u32(function.start_address)
                        << " " << function.name << "\n";
            }
            return true;
        }
        if (tokens[1] == "files") {
            if (!debugger_.session().symbols()) {
                throw std::runtime_error("no symbols loaded");
            }
            for (const auto& file : debugger_.session().symbols()->files) {
                output_ << file.id << " " << file.path << "\n";
            }
            return true;
        }
        if (tokens[1] == "line") {
            const auto result = tokens.size() >= 3
                ? debugger_.session().info_line_argument(tokens[2])
                : debugger_.session().info_line_current();
            output_ << "Line " << result.location.line
                    << " of \"" << result.location.file_path << "\" starts at address "
                    << hex_u32(result.location.address);
            if (result.location.function_name.has_value()) {
                output_ << " <" << result.location.function_name.value() << ">";
            }
            if (result.end_address.has_value()) {
                output_ << " and ends at " << hex_u32(result.end_address.value());
            }
            output_ << ".\n";
            return true;
        }
        if (tokens[1] == "locals") {
            const auto regs = debugger_.session().read_registers();
            const auto vars = debugger_.session().visible_variables(regs.pc);
            if (vars.empty()) {
                output_ << "No visible locals.\n";
            } else {
                for (const auto* var : vars) {
                    output_ << var->name;
                    if (var->type_name.has_value()) {
                        output_ << " : " << var->type_name.value();
                    }
                    if (const auto value = stringify_storage(*var); value.has_value()) {
                        output_ << "  " << value.value();
                    }
                    output_ << "\n";
                }
            }
            return true;
        }
        throw std::runtime_error("unknown info subcommand");
    }

    if (head == "list") {
        const auto lines = debugger_.session().list_source(tokens.size() >= 2
            ? std::optional<std::string>(tokens[1])
            : std::nullopt);
        for (const auto& line : lines) {
            output_ << (line.is_current ? "=> " : "   ")
                    << std::setw(5) << line.line
                    << " " << line.text << "\n";
        }
        return true;
    }

    if (head == "disassemble" || head == "disas") {
        uint32_t address = debugger_.session().read_registers().pc;
        std::size_t count = 8;
        if (tokens.size() >= 2) {
            address = debugger_.session().resolve_address_expression(tokens[1]);
        }
        if (tokens.size() >= 3) {
            count = static_cast<std::size_t>(parse_u32(tokens[2]));
        }
        for (const auto& instruction : debugger_.session().disassemble(address, count)) {
            output_ << hex_u32(instruction.address) << ": ";
            for (uint8_t byte : instruction.bytes) {
                output_ << std::setw(2) << std::setfill('0')
                        << std::hex << std::uppercase
                        << static_cast<int>(byte) << " "
                        << std::nouppercase << std::dec << std::setfill(' ');
            }
            output_ << "\t" << instruction.text << "\n";
        }
        return true;
    }

    if (head == "x" || (head.size() >= 2 && head[0] == 'x' && head[1] == '/')) {
        if (tokens.size() < 2) {
            throw std::runtime_error("usage: x/Nxb ADDR");
        }

        std::size_t count = 16;
        bool instruction_mode = false;
        if (head.size() > 2 && head[1] == '/') {
            std::size_t pos = 2;
            while (pos < head.size() &&
                   std::isdigit(static_cast<unsigned char>(head[pos]))) {
                ++pos;
            }
            if (pos > 2) {
                count = static_cast<std::size_t>(parse_u32(head.substr(2, pos - 2)));
            }
            if (pos < head.size() && (head[pos] == 'i' || head[pos] == 'I')) {
                instruction_mode = true;
            }
        }

        const uint32_t address = debugger_.session().resolve_address_expression(tokens[1]);
        if (instruction_mode) {
            for (const auto& instruction : debugger_.session().disassemble(address, count)) {
                output_ << hex_u32(instruction.address) << ": ";
                for (uint8_t byte : instruction.bytes) {
                    output_ << std::setw(2) << std::setfill('0')
                            << std::hex << std::uppercase
                            << static_cast<int>(byte) << " "
                            << std::nouppercase << std::dec << std::setfill(' ');
                }
                output_ << "\t" << instruction.text << "\n";
            }
            return true;
        }

        const auto data = debugger_.session().read_memory(address, count);
        for (std::size_t i = 0; i < data.size(); ++i) {
            if (i % 16 == 0) {
                output_ << hex_u32(address + static_cast<uint32_t>(i)) << ": ";
            }
            output_ << std::setw(2) << std::setfill('0')
                    << std::hex << std::uppercase
                    << static_cast<int>(data[i]) << " "
                    << std::nouppercase << std::dec << std::setfill(' ');
            if ((i % 16) == 15 || i + 1 == data.size()) {
                output_ << "\n";
            }
        }
        return true;
    }

    throw std::runtime_error("unknown command: " + head);
}

std::unique_ptr<debug_protocol> make_cli_protocol(
    debugger_host& host,
    std::istream& input,
    std::ostream& output,
    std::ostream& error,
    bool quiet,
    bool show_prompt)
{
    return std::make_unique<cli_frontend>(
        host, input, output, error, quiet, show_prompt);
}
