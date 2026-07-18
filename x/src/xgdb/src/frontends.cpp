#include "frontends.h"
#include "debugger_session.h"

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

    void print_no_executable(std::ostream& output) {
        output << "No executable file specified.\n";
        output << "Use the \"file\" command.\n";
    }

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

    std::string trim_trailing_separators(const std::string& value) {
        std::size_t end = value.size();
        while (end > 0) {
            const char ch = value[end - 1];
            if (ch == ',' || std::isspace(static_cast<unsigned char>(ch))) {
                --end;
                continue;
            }
            break;
        }
        return value.substr(0, end);
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
        const std::string normalized = trim_trailing_separators(trim(value));
        char* end = nullptr;
        const unsigned long parsed = std::strtoul(normalized.c_str(), &end, 0);
        if (end == normalized.c_str() || *end != '\0') {
            throw std::runtime_error("invalid number: " + value);
        }
        return static_cast<uint32_t>(parsed);
    }

    std::string hex_u32(uint32_t value) {
        std::ostringstream out;
        out << "0x" << std::hex << std::uppercase << value << std::dec;
        return out.str();
    }

    std::string format_reason(xgdb::stop_reason reason) {
        switch (reason) {
        case xgdb::stop_reason::breakpoint: return "breakpoint";
        case xgdb::stop_reason::step: return "step";
        case xgdb::stop_reason::pause: return "pause";
        case xgdb::stop_reason::halted: return "halted";
        case xgdb::stop_reason::exited: return "exited";
        case xgdb::stop_reason::signal: return "signal";
        case xgdb::stop_reason::none: return "none";
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

    std::string mi_stop_reason(xgdb::stop_reason reason) {
        switch (reason) {
        case xgdb::stop_reason::breakpoint: return "breakpoint-hit";
        case xgdb::stop_reason::step: return "end-stepping-range";
        case xgdb::stop_reason::pause: return "signal-received";
        case xgdb::stop_reason::halted: return "signal-received";
        case xgdb::stop_reason::exited: return "exited-normally";
        case xgdb::stop_reason::signal: return "signal-received";
        case xgdb::stop_reason::none: return "end-stepping-range";
        }
        return "signal-received";
    }

    std::optional<std::string> stringify_storage(const xgdb::variable& variable) {
        switch (variable.storage) {
        case xgdb::storage_kind::address:
            if (variable.address.has_value()) {
                return hex_u32(variable.address.value());
            }
            break;
        case xgdb::storage_kind::stack:
        case xgdb::storage_kind::frame_relative:
            if (variable.offset.has_value()) {
                return std::to_string(variable.offset.value());
            }
            break;
        case xgdb::storage_kind::register_name:
        case xgdb::storage_kind::register_pair:
            if (variable.register_name.has_value()) {
                return variable.register_name.value();
            }
            break;
        case xgdb::storage_kind::unknown:
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
    bool show_prompt)
    : debugger_(host),
      input_(input),
      output_(output),
      error_(error),
      show_prompt_(show_prompt)
{
    debugger_.set_event_sink(this);
}

void cli_frontend::set_execute_commands(std::vector<std::string> commands) {
    execute_commands_ = std::move(commands);
}

void cli_frontend::set_log(std::ostream* log) { log_ = log; }

void cli_frontend::emit_source_annotation(const source_location& /*location*/) {
    // No annotation emitted.  \032\032 causes "□□" box chars in the console
    // (DDD's tty_full_name writes it back to the readline terminal when
    // app_data.annotate == 1) and a double (gdb)(gdb) prompt.
    // DDD's text parser picks up "func () at file:line" from normal stop
    // output and updates the source pane without needing explicit annotation.
}

int cli_frontend::run() {
    // Output the same volume of text as real GDB (~650 bytes).
    // DDD sets promptPosition based on the length of our startup output:
    // too little and (gdb) lands at position 0, overwriting the copyright.
    output_ << "\nGNU gdb (xgdb) 0.1\n"
            << "Copyright (C) 2026 tomaz stih. MIT License.\n"
            << "Target: Z80 via GDB Remote Serial Protocol.\n"
            << "For bug reports: https://github.com/retro-vault/xyz\n"
            << "\n"
            << "For help, type \"help\".\n"
            << "Type \"target remote HOST:PORT\" to connect to a Z80 gdbserver.\n";
    if (debugger_.session().symbol_path().has_value()) {
        output_ << "Symbols loaded from \""
                << debugger_.session().symbol_path().value() << "\".\n";
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
            if (annotate_ >= 1) output_ << "\032\032prompt\n";
            output_ << "(gdb) " << std::flush;
        }
        if (!std::getline(input_, line)) {
            break;
        }
        if (log_) *log_ << "[IN]  " << line << "\n" << std::flush;

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

    if (head == "#") {
        return true;
    }

    if (head == "help") {
        output_
            << "commands:\n"
            << "  help\n"
            << "  quit | q\n"
            << "  target remote HOST:PORT\n"
            << "  cdb-file FILE\n"
            << "  map-file FILE\n"
            << "  directory DIR\n"
            << "  load FILE ADDR [PC]  upload binary to target memory\n"
            << "  file FILE\n"
            << "  break EXPR | b EXPR\n"
            << "  delete [ID]\n"
            << "  continue | c\n"
            << "  step | s | next | n | stepi | si | nexti | ni\n"
            << "  finish\n"
            << "  backtrace | bt | where\n"
            << "  up | down | frame\n"
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
            << "  set ...\n";
        return true;
    }

    if (head == "source") {
        if (tokens.size() < 2) {
            throw std::runtime_error("usage: source FILE");
        }

        std::ifstream commands(tokens[1]);
        if (!commands.is_open()) {
            throw std::runtime_error("cannot open command file: " + tokens[1]);
        }

        std::string sourced_line;
        while (std::getline(commands, sourced_line)) {
            try {
                execute_command(sourced_line);
            } catch (const std::exception& e) {
                error_ << "error: " << e.what() << "\n";
            }
            if (should_quit_) {
                break;
            }
        }
        return true;
    }

    if (head == "quit" || head == "q") {
        should_quit_ = true;
        return false;
    }

    if (head == "show" && tokens.size() >= 2) {
        if (tokens[1] == "version") {
            output_ << "GNU gdb (xgdb) 0.1\n";
        } else if (tokens[1] == "language") {
            output_ << "The current source language is \"auto; currently c\".\n";
        } else if (tokens[1] == "prompt") {
            output_ << "Gdb's prompt is \"(gdb) \".\n";
        } else if (tokens[1] == "print" || tokens[1] == "output-radix"
                   || tokens[1] == "input-radix" || tokens[1] == "endian"
                   || tokens[1] == "architecture") {
            // Accept common GDB frontend startup probes without trying to
            // emulate another debugger's CLI semantics.
        }
        return true;
    }

    if (head == "set") {
        if (tokens.size() >= 3 && tokens[1] == "annotate") {
            const int level = std::atoi(tokens[2].c_str());
            if (level >= 2) {
                throw std::runtime_error("annotation level 2+ not supported");
            }
            annotate_ = level;
        }
        if (tokens.size() >= 3 && tokens[1] == "prompt") {
            // DDD sends "set prompt (gdb) " to normalise the prompt string.
            // We already use (gdb) so just accept it.
        }
        // Accept common GDB frontend startup probes silently.
        return true;
    }

    if (head == "define" || head == "end" || head == "document") {
        return true;
    }

    // GDB's `output EXPR` prints EXPR without a newline or type tag.
    // DDD sends `output "cookie"` during init to verify GDB is responding;
    // we echo the argument so DDD's config_output check passes.
    if (head == "output") {
        if (tokens.size() >= 2) {
            std::string val = tokens[1];
            // Strip surrounding quotes if present.
            if (val.size() >= 2 && val.front() == '"' && val.back() == '"')
                val = val.substr(1, val.size() - 2);
            output_ << val;   // no newline — matches GDB's output command
        }
        return true;
    }

    if (head == "pwd") {
        output_ << std::filesystem::current_path() << "\n";
        return true;
    }

    if (head == "output") {
        if (tokens.size() >= 2) {
            for (std::size_t i = 1; i < tokens.size(); ++i) {
                if (i != 1) {
                    output_ << " ";
                }
                output_ << tokens[i];
            }
        }
        return true;
    }

    if (head == "display") {
        return true;
    }

    if (head == "target") {
        if (tokens.size() < 3 || tokens[1] != "remote") {
            throw std::runtime_error("usage: target remote HOST:PORT");
        }
        debugger_.connect_remote(tokens[2]);
        output_ << "Remote debugging using " << tokens[2] << ".\n";
        return true;
    }

    if (head == "directory" || head == "dir") {
        if (tokens.size() < 2) {
            throw std::runtime_error("usage: directory DIR");
        }
        static_cast<debugger_session&>(debugger_.session()).add_source_dir(tokens[1]);
        output_ << "Source directory added: " << tokens[1] << "\n";
        return true;
    }

    if (head == "cdb-file") {
        if (tokens.size() < 2) {
            throw std::runtime_error("usage: cdb-file FILE");
        }
        debugger_.load_cdb_file(tokens[1]);
        output_ << "Loaded CDB symbols from " << tokens[1] << "\n";
        return true;
    }

    if (head == "map-file") {
        if (tokens.size() < 2) {
            throw std::runtime_error("usage: map-file FILE");
        }
        debugger_.load_map_file(tokens[1]);
        output_ << "Loaded MAP symbols from " << tokens[1] << "\n";
        return true;
    }

    if (head == "load") {
        if (tokens.size() < 3) {
            throw std::runtime_error("usage: load FILE ADDR [PC]");
        }
        const std::string& load_path = tokens[1];
        const uint32_t load_addr = parse_u32(tokens[2]);
        const uint32_t load_pc   = tokens.size() >= 4
            ? parse_u32(tokens[3]) : load_addr;

        std::ifstream bin(load_path, std::ios::binary);
        if (!bin.is_open()) {
            throw std::runtime_error("cannot open file: " + load_path);
        }
        const std::vector<uint8_t> bytes(
            std::istreambuf_iterator<char>(bin),
            std::istreambuf_iterator<char>{});

        debugger_.session().write_memory(load_addr, bytes);

        auto regs = debugger_.session().read_registers();
        regs.pc = static_cast<uint16_t>(load_pc);
        debugger_.session().write_registers(regs);

        output_ << "Loaded " << bytes.size() << " bytes at "
                << hex_u32(load_addr) << ", PC set to "
                << hex_u32(load_pc) << "\n";
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
            emit_source_annotation(stop.source.value());
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

    // stepi / nexti — single machine instruction
    // step / next / s / n — source-level step (maps to stepi on embedded targets)
    // finish — run to end of function (maps to continue for now)
    if (head == "stepi" || head == "si" || head == "nexti" || head == "ni"
        || head == "step" || head == "s" || head == "next" || head == "n") {
        const auto stop = debugger_.step_instruction();
        if (stop.source.has_value()) {
            emit_source_annotation(stop.source.value());
            output_ << stop.source->function_name.value_or("<unknown>")
                    << " () at " << stop.source->file_path
                    << ":" << stop.source->line << "\n";
            if (stop.source_text.has_value()) {
                output_ << stop.source->line << "\t" << stop.source_text.value() << "\n";
            }
        } else {
            output_ << "Stopped at pc=" << hex_u32(stop.pc) << "\n";
        }
        return true;
    }

    if (head == "finish") {
        const auto stop = debugger_.continue_execution();
        if (stop.source.has_value()) {
            emit_source_annotation(stop.source.value());
            output_ << stop.source->function_name.value_or("<unknown>")
                    << " () at " << stop.source->file_path
                    << ":" << stop.source->line << "\n";
        } else {
            output_ << "Stopped at pc=" << hex_u32(stop.pc) << "\n";
        }
        return true;
    }

    // Stack frame commands — single-frame target, always frame 0.
    if (head == "backtrace" || head == "bt" || head == "where") {
        const auto stop = debugger_.status();
        output_ << "#0  ";
        if (stop.source.has_value()) {
            emit_source_annotation(stop.source.value());
            output_ << stop.source->function_name.value_or("??")
                    << " () at " << stop.source->file_path
                    << ":" << stop.source->line;
        } else {
            output_ << "?? () at 0x" << std::hex << stop.pc << std::dec;
        }
        output_ << "\n";
        return true;
    }

    if (head == "up" || head == "down") {
        output_ << "No stack frames.\n";
        return true;
    }

    if (head == "frame" || head == "f") {
        const auto stop = debugger_.status();
        output_ << "#0  ";
        if (stop.source.has_value()) {
            output_ << stop.source->function_name.value_or("??")
                    << " () at " << stop.source->file_path
                    << ":" << stop.source->line;
        } else {
            output_ << "?? () at 0x" << std::hex << stop.pc << std::dec;
        }
        output_ << "\n";
        return true;
    }

    if (head == "status") {
        const auto stop = debugger_.status();
        if (stop.source.has_value()) {
            emit_source_annotation(stop.source.value());
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
            if (!debugger_.session().has_symbols()) {
                print_no_executable(output_);
                return true;
            }
            for (const auto& function : debugger_.session().symbols()->functions) {
                output_ << hex_u32(function.start_address)
                        << " " << function.name << "\n";
            }
            return true;
        }
        if (tokens[1] == "files") {
            if (!debugger_.session().has_symbols()) {
                print_no_executable(output_);
                return true;
            }
            for (const auto& file : debugger_.session().symbols()->files) {
                output_ << file.id << " " << file.path << "\n";
            }
            return true;
        }
        if (tokens[1] == "display") {
            output_ << "There are no auto-display expressions now.\n";
            return true;
        }
        if (tokens[1] == "program") {
            if (!debugger_.session().is_connected()) {
                output_ << "The program being debugged is not being run.\n";
            } else {
                output_ << "Program stopped.\n";
            }
            return true;
        }
        if (tokens[1] == "source" || tokens[1] == "sources") {
            if (!debugger_.session().has_symbols()) {
                print_no_executable(output_);
                return true;
            }
            if (tokens[1] == "source") {
                if (!debugger_.session().is_connected()) {
                    output_ << "No current source file.\n";
                    return true;
                }
                if (const auto location = debugger_.session().current_source_location();
                    location.has_value()) {
                    output_ << "Current source file is " << location->file_path
                            << ":" << location->line << "\n";
                } else {
                    output_ << "No current source file.\n";
                }
            } else {
                for (const auto& file : debugger_.session().symbols()->files) {
                    output_ << file.path << "\n";
                }
            }
            return true;
        }
        if (tokens[1] == "line") {
            if (tokens.size() < 3) {
                if (!debugger_.session().is_connected() || !debugger_.session().has_symbols()) {
                    print_no_executable(output_);
                    return true;
                }
            } else if (!debugger_.session().has_symbols()) {
                print_no_executable(output_);
                return true;
            }
            // Output position in stop-format ("func () at file:line") so
            // DDD's text parser updates the source pane without showing
            // the "Debugger Message" dialog that "Line N of..." triggers.
            try {
                const auto result = tokens.size() >= 3
                    ? debugger_.session().info_line_argument(tokens[2])
                    : debugger_.session().info_line_current();
                output_ << result.location.function_name.value_or("??")
                        << " () at " << result.location.file_path
                        << ":" << result.location.line << "\n";
            } catch (...) {}
            return true;
            const auto result = tokens.size() >= 3
                ? debugger_.session().info_line_argument(tokens[2])
                : debugger_.session().info_line_current();
            output_ << "Line " << result.location.line
                    << " of \"" << result.location.file_path << "\" starts at address "
                    << hex_u32(result.location.address);
            if (result.location.function_name.has_value()) {
                output_ << " <" << result.location.function_name.value() << ">";
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
        return true;
    }

    if (head == "list" || head == "l") {
        if (!debugger_.session().has_symbols()) {
            print_no_executable(output_);
            return true;
        }
        if (tokens.size() < 2 && !debugger_.session().is_connected()) {
            print_no_executable(output_);
            return true;
        }
        if (annotate_ >= 1) {
            if (tokens.size() < 2) {
                if (const auto location = debugger_.session().current_source_location();
                    location.has_value()) {
                    emit_source_annotation(location.value());
                }
            } else {
                std::string anchor = tokens[1];
                if (const auto comma = anchor.find(','); comma != std::string::npos) {
                    anchor = anchor.substr(0, comma);
                    if (anchor.find(':') == std::string::npos) {
                        if (const auto location = debugger_.session().current_source_location();
                            location.has_value()) {
                            anchor = location->file_path + ":" + anchor;
                        }
                    }
                }
                try {
                    emit_source_annotation(
                        debugger_.session().info_line_argument(anchor).location);
                } catch (const std::exception&) {
                    // Keep listing functional even if the annotation anchor
                    // cannot be resolved exactly like GDB would.
                }
            }
        }
        const auto lines = debugger_.session().list_source(tokens.size() >= 2
            ? std::optional<std::string>(tokens[1])
            : std::nullopt);
        for (const auto& line : lines) {
            if (annotate_ >= 1) {
                output_ << line.line << "\t" << line.text << "\n";
            } else {
                output_ << (line.is_current ? "=> " : "   ")
                        << std::setw(5) << line.line
                        << " " << line.text << "\n";
            }
        }
        return true;
    }

    if (head == "disassemble" || head == "disas") {
        uint32_t address = debugger_.session().read_registers().pc;
        std::optional<uint32_t> end_address;
        std::size_t count = 8;

        // Strip GDB disassembly mode flags: /m (mixed), /s (source), /r (raw)
        auto dtokens = tokens;
        while (dtokens.size() >= 2 && !dtokens[1].empty() && dtokens[1][0] == '/')
            dtokens.erase(dtokens.begin() + 1);
        const auto& tokens = dtokens; // shadow for the rest of this block

        if (tokens.size() >= 2) {
            std::string start_expr = tokens[1];
            std::string end_expr;

            if (const auto comma = start_expr.find(','); comma != std::string::npos) {
                end_expr = trim(start_expr.substr(comma + 1));
                start_expr = trim(start_expr.substr(0, comma));
            }
            if (end_expr.empty() && !tokens[1].empty() && tokens[1].back() == ',' &&
                tokens.size() >= 3) {
                end_expr = tokens[2];
            }

            try {
                address = debugger_.session().resolve_address_expression(start_expr);
            } catch (...) { /* keep current PC */ }

            if (!end_expr.empty()) {
                try {
                    end_address = debugger_.session().resolve_address_expression(end_expr);
                } catch (...) {}
            } else if (tokens.size() >= 3) {
                try { count = static_cast<std::size_t>(parse_u32(tokens[2])); }
                catch (...) {}
            }
        }

        std::vector<disassembly_line> lines;
        if (end_address.has_value()) {
            uint32_t pc = address;
            std::size_t emitted = 0;
            while (pc < end_address.value() && emitted < 1024) {
                const auto chunk = debugger_.session().disassemble(pc, 1);
                if (chunk.empty()) {
                    break;
                }
                lines.push_back(chunk.front());
                const auto step = chunk.front().bytes.empty()
                    ? 1u
                    : static_cast<uint32_t>(chunk.front().bytes.size());
                pc += step;
                ++emitted;
            }
        } else {
            lines = debugger_.session().disassemble(address, count);
        }

        for (const auto& instruction : lines) {
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

    // Always silently ignore unrecognised commands.  DDD feeds its own
    // copyright banner to the debugger stdin (via XmTextSetString triggering
    // gdbChangeCB before the first GDB prompt).  Any error output for those
    // lines comes back to DDD and gets displayed again, causing the banner
    // to appear twice.  Returning silently keeps the console clean.
    return true;
}

std::unique_ptr<debug_protocol> make_cli_protocol(
    debugger_host& host,
    std::istream& input,
    std::ostream& output,
    std::ostream& error,
    bool show_prompt)
{
    return std::make_unique<cli_frontend>(
        host, input, output, error, show_prompt);
}
