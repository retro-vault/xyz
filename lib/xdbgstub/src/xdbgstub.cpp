/*
 * Implements the hosted xdbgstub client and server transport,
 * message parsing, and target command forwarding for remote
 * debugging sessions.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <charconv>
#include <cstdint>
#include <cstring>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <xdbgstub/client.hpp>
#include <xdbgstub/error.hpp>
#include <xdbgstub/server.hpp>

namespace xdbgstub {

    namespace {

        using field_map = std::map<std::string, std::string>;

        struct message {
            std::string kind;
            field_map fields;
        };

        std::string quote(const std::string& value) {
            std::string result = "\"";
            for (char ch : value) {
                if (ch == '"' || ch == '\\') {
                    result.push_back('\\');
                }
                result.push_back(ch);
            }
            result.push_back('"');
            return result;
        }

        std::string unquote(const std::string& value) {
            if (value.size() < 2 || value.front() != '"' || value.back() != '"') {
                return value;
            }

            std::string result;
            bool escaped = false;
            for (std::size_t i = 1; i + 1 < value.size(); ++i) {
                const char ch = value[i];
                if (escaped) {
                    result.push_back(ch);
                    escaped = false;
                } else if (ch == '\\') {
                    escaped = true;
                } else {
                    result.push_back(ch);
                }
            }
            return result;
        }

        std::string trim(const std::string& value) {
            const auto start = value.find_first_not_of(" \t\r\n");
            if (start == std::string::npos) {
                return "";
            }
            const auto end = value.find_last_not_of(" \t\r\n");
            return value.substr(start, end - start + 1);
        }

        bool is_identifier_char(char ch) {
            const unsigned char uch = static_cast<unsigned char>(ch);
            return std::isalnum(uch) || ch == '_' || ch == '-';
        }

        std::string read_token(const std::string& line, std::size_t& pos) {
            while (pos < line.size() &&
                   std::isspace(static_cast<unsigned char>(line[pos]))) {
                ++pos;
            }
            if (pos >= line.size()) {
                return "";
            }

            if (line[pos] == '"') {
                std::string result;
                result.push_back(line[pos++]);
                bool escaped = false;
                while (pos < line.size()) {
                    const char ch = line[pos++];
                    result.push_back(ch);
                    if (escaped) {
                        escaped = false;
                    } else if (ch == '\\') {
                        escaped = true;
                    } else if (ch == '"') {
                        return result;
                    }
                }
                throw protocol_error("unterminated quoted token");
            }

            const std::size_t start = pos;
            while (pos < line.size() &&
                   !std::isspace(static_cast<unsigned char>(line[pos]))) {
                ++pos;
            }
            return line.substr(start, pos - start);
        }

        field_map parse_fields(const std::string& line) {
            field_map fields;
            std::size_t pos = 0;

            while (true) {
                while (pos < line.size() &&
                       std::isspace(static_cast<unsigned char>(line[pos]))) {
                    ++pos;
                }
                if (pos >= line.size()) {
                    break;
                }

                const std::size_t key_start = pos;
                while (pos < line.size() && is_identifier_char(line[pos])) {
                    ++pos;
                }
                if (key_start == pos || pos >= line.size() || line[pos] != '=') {
                    throw protocol_error("expected key=value field");
                }

                const std::string key = line.substr(key_start, pos - key_start);
                ++pos;
                const std::string value = read_token(line, pos);
                fields[key] = unquote(value);
            }

            return fields;
        }

        message parse_message_line(const std::string& raw_line) {
            const std::string line = trim(raw_line);
            if (line.empty()) {
                throw protocol_error("empty protocol line");
            }

            std::size_t pos = 0;
            const std::string kind = read_token(line, pos);
            message result;
            result.kind = kind;
            result.fields = parse_fields(line.substr(pos));
            return result;
        }

        std::string format_message(const std::string& kind, const field_map& fields) {
            std::ostringstream out;
            out << kind;
            for (const auto& [key, value] : fields) {
                out << ' ' << key << '=' << quote(value);
            }
            out << '\n';
            return out.str();
        }

        std::string hex_u32(uint32_t value) {
            std::ostringstream out;
            out << "0x" << std::hex << value << std::dec;
            return out.str();
        }

        std::string hex_u16(uint16_t value) {
            return hex_u32(value);
        }

        std::string hex_u8(uint8_t value) {
            return hex_u32(value);
        }

        uint32_t parse_u32(const std::string& value) {
            char* end = nullptr;
            const unsigned long parsed = std::strtoul(value.c_str(), &end, 0);
            if (end == value.c_str() || *end != '\0') {
                throw protocol_error("invalid numeric value '" + value + "'");
            }
            return static_cast<uint32_t>(parsed);
        }

        bool parse_bool(const std::string& value) {
            if (value == "0" || value == "false") return false;
            if (value == "1" || value == "true") return true;
            throw protocol_error("invalid boolean value '" + value + "'");
        }

        std::string bool_string(bool value) {
            return value ? "1" : "0";
        }

        std::string bytes_to_hex(const std::vector<uint8_t>& data) {
            static const char* digits = "0123456789ABCDEF";
            std::string result;
            result.reserve(data.size() * 2);
            for (uint8_t byte : data) {
                result.push_back(digits[(byte >> 4) & 0x0F]);
                result.push_back(digits[byte & 0x0F]);
            }
            return result;
        }

        std::vector<uint8_t> hex_to_bytes(const std::string& value) {
            if (value.size() % 2 != 0) {
                throw protocol_error("hex data must have even length");
            }

            std::vector<uint8_t> data;
            data.reserve(value.size() / 2);
            for (std::size_t i = 0; i < value.size(); i += 2) {
                const std::string piece = value.substr(i, 2);
                data.push_back(static_cast<uint8_t>(parse_u32("0x" + piece)));
            }
            return data;
        }

        const char* to_string(execution_state state) {
            switch (state) {
            case execution_state::stopped: return "stopped";
            case execution_state::running: return "running";
            case execution_state::terminated: return "terminated";
            }
            return "stopped";
        }

        execution_state parse_execution_state(const std::string& value) {
            if (value == "stopped") return execution_state::stopped;
            if (value == "running") return execution_state::running;
            if (value == "terminated") return execution_state::terminated;
            throw protocol_error("invalid execution_state '" + value + "'");
        }

        const char* to_string(stop_reason reason) {
            switch (reason) {
            case stop_reason::none: return "none";
            case stop_reason::breakpoint: return "breakpoint";
            case stop_reason::step: return "step";
            case stop_reason::pause: return "pause";
            case stop_reason::halted: return "halted";
            case stop_reason::exited: return "exited";
            case stop_reason::signal: return "signal";
            }
            return "none";
        }

        stop_reason parse_stop_reason(const std::string& value) {
            if (value == "none") return stop_reason::none;
            if (value == "breakpoint") return stop_reason::breakpoint;
            if (value == "step") return stop_reason::step;
            if (value == "pause") return stop_reason::pause;
            if (value == "halted") return stop_reason::halted;
            if (value == "exited") return stop_reason::exited;
            if (value == "signal") return stop_reason::signal;
            throw protocol_error("invalid stop_reason '" + value + "'");
        }

        field_map cpu_state_to_fields(const cpu_state& state) {
            return {
                {"af", hex_u16(state.af)},
                {"bc", hex_u16(state.bc)},
                {"de", hex_u16(state.de)},
                {"hl", hex_u16(state.hl)},
                {"ix", hex_u16(state.ix)},
                {"iy", hex_u16(state.iy)},
                {"sp", hex_u16(state.sp)},
                {"pc", hex_u16(state.pc)},
                {"i", hex_u8(state.i)},
                {"r", hex_u8(state.r)},
                {"iff1", bool_string(state.iff1)},
                {"iff2", bool_string(state.iff2)},
                {"halted", bool_string(state.halted)}
            };
        }

        cpu_state cpu_state_from_fields(const field_map& fields) {
            cpu_state state;
            state.af = static_cast<uint16_t>(parse_u32(fields.at("af")));
            state.bc = static_cast<uint16_t>(parse_u32(fields.at("bc")));
            state.de = static_cast<uint16_t>(parse_u32(fields.at("de")));
            state.hl = static_cast<uint16_t>(parse_u32(fields.at("hl")));
            state.ix = static_cast<uint16_t>(parse_u32(fields.at("ix")));
            state.iy = static_cast<uint16_t>(parse_u32(fields.at("iy")));
            state.sp = static_cast<uint16_t>(parse_u32(fields.at("sp")));
            state.pc = static_cast<uint16_t>(parse_u32(fields.at("pc")));
            state.i = static_cast<uint8_t>(parse_u32(fields.at("i")));
            state.r = static_cast<uint8_t>(parse_u32(fields.at("r")));
            state.iff1 = parse_bool(fields.at("iff1"));
            state.iff2 = parse_bool(fields.at("iff2"));
            state.halted = parse_bool(fields.at("halted"));
            return state;
        }

        field_map target_status_to_fields(const target_status& status) {
            field_map fields;
            fields["state"] = to_string(status.state);
            fields["reason"] = to_string(status.reason);
            fields["pc"] = hex_u32(status.pc);
            if (status.exit_code.has_value()) {
                fields["exit_code"] = hex_u32(status.exit_code.value());
            }
            if (status.registers.has_value()) {
                for (const auto& [key, value] : cpu_state_to_fields(status.registers.value())) {
                    fields["reg_" + key] = value;
                }
            }
            return fields;
        }

        target_status target_status_from_fields(const field_map& fields) {
            target_status status;
            status.state = parse_execution_state(fields.at("state"));
            status.reason = parse_stop_reason(fields.at("reason"));
            status.pc = parse_u32(fields.at("pc"));

            const auto exit_it = fields.find("exit_code");
            if (exit_it != fields.end()) {
                status.exit_code = parse_u32(exit_it->second);
            }

            field_map reg_fields;
            for (const auto& [key, value] : fields) {
                if (key.rfind("reg_", 0) == 0) {
                    reg_fields[key.substr(4)] = value;
                }
            }
            if (!reg_fields.empty()) {
                status.registers = cpu_state_from_fields(reg_fields);
            }

            return status;
        }

        void close_fd(int& fd) {
            if (fd >= 0) {
                ::close(fd);
                fd = -1;
            }
        }

        void throw_errno(const std::string& prefix) {
            throw error(prefix + ": " + std::strerror(errno));
        }

        void send_all(int fd, const std::string& text) {
            std::size_t sent = 0;
            while (sent < text.size()) {
                const ssize_t rc = ::send(fd, text.data() + sent,
                                          text.size() - sent, 0);
                if (rc < 0) {
                    throw_errno("send failed");
                }
                if (rc == 0) {
                    throw error("connection closed while sending");
                }
                sent += static_cast<std::size_t>(rc);
            }
        }

        std::string recv_line(int fd) {
            std::string line;
            char ch = 0;
            while (true) {
                const ssize_t rc = ::recv(fd, &ch, 1, 0);
                if (rc < 0) {
                    throw_errno("recv failed");
                }
                if (rc == 0) {
                    if (line.empty()) {
                        throw error("connection closed");
                    }
                    return line;
                }
                if (ch == '\n') {
                    return line;
                }
                line.push_back(ch);
            }
        }

        std::string require_field(const field_map& fields, const std::string& key) {
            const auto it = fields.find(key);
            if (it == fields.end()) {
                throw protocol_error("missing field '" + key + "'");
            }
            return it->second;
        }

        message transact(int fd, const field_map& request_fields) {
            send_all(fd, format_message("request", request_fields));
            const message response = parse_message_line(recv_line(fd));
            if (response.kind != "response") {
                throw protocol_error("expected response message");
            }

            const auto status_it = response.fields.find("status");
            if (status_it == response.fields.end()) {
                throw protocol_error("response missing status field");
            }
            if (status_it->second != "ok") {
                const auto msg_it = response.fields.find("message");
                throw error(msg_it != response.fields.end()
                    ? msg_it->second
                    : "remote command failed");
            }
            return response;
        }

        int connect_socket(const std::string& host, uint16_t port) {
            struct addrinfo hints {};
            hints.ai_family = AF_UNSPEC;
            hints.ai_socktype = SOCK_STREAM;

            struct addrinfo* info = nullptr;
            const std::string port_text = std::to_string(port);
            const int rc = ::getaddrinfo(host.c_str(), port_text.c_str(),
                                         &hints, &info);
            if (rc != 0) {
                throw error("getaddrinfo failed: " + std::string(gai_strerror(rc)));
            }

            int fd = -1;
            for (auto* cursor = info; cursor != nullptr; cursor = cursor->ai_next) {
                fd = ::socket(cursor->ai_family, cursor->ai_socktype, cursor->ai_protocol);
                if (fd < 0) {
                    continue;
                }
                if (::connect(fd, cursor->ai_addr, cursor->ai_addrlen) == 0) {
                    break;
                }
                close_fd(fd);
            }
            ::freeaddrinfo(info);

            if (fd < 0) {
                throw error("unable to connect to remote target");
            }

            return fd;
        }

        int create_listen_socket(const std::string& bind_host, uint16_t port) {
            struct addrinfo hints {};
            hints.ai_family = AF_UNSPEC;
            hints.ai_socktype = SOCK_STREAM;
            hints.ai_flags = AI_PASSIVE;

            struct addrinfo* info = nullptr;
            const std::string port_text = std::to_string(port);
            const char* host_ptr = bind_host.empty() ? nullptr : bind_host.c_str();
            const int rc = ::getaddrinfo(host_ptr, port_text.c_str(), &hints, &info);
            if (rc != 0) {
                throw error("getaddrinfo failed: " + std::string(gai_strerror(rc)));
            }

            int fd = -1;
            for (auto* cursor = info; cursor != nullptr; cursor = cursor->ai_next) {
                fd = ::socket(cursor->ai_family, cursor->ai_socktype, cursor->ai_protocol);
                if (fd < 0) {
                    continue;
                }

                int enable = 1;
                ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));

                if (::bind(fd, cursor->ai_addr, cursor->ai_addrlen) == 0 &&
                    ::listen(fd, 1) == 0) {
                    break;
                }

                close_fd(fd);
            }
            ::freeaddrinfo(info);

            if (fd < 0) {
                throw error("unable to bind listen socket");
            }
            return fd;
        }

        void check_response_id(const message& response, uint32_t expected_id) {
            if (parse_u32(require_field(response.fields, "id")) != expected_id) {
                throw protocol_error("mismatched response id");
            }
        }

        field_map base_response_fields(uint32_t id) {
            return {{"id", std::to_string(id)}, {"status", "ok"}};
        }

        void add_fields(field_map& dst, const field_map& src) {
            for (const auto& [key, value] : src) {
                dst[key] = value;
            }
        }

    } // namespace

    client::~client() {
        close();
    }

    client::client(client&& other) noexcept
        : socket_fd_(other.socket_fd_),
          next_request_id_(other.next_request_id_) {
        other.socket_fd_ = -1;
        other.next_request_id_ = 1;
    }

    client& client::operator=(client&& other) noexcept {
        if (this != &other) {
            close();
            socket_fd_ = other.socket_fd_;
            next_request_id_ = other.next_request_id_;
            other.socket_fd_ = -1;
            other.next_request_id_ = 1;
        }
        return *this;
    }

    void client::connect(const std::string& host, uint16_t port) {
        close();
        socket_fd_ = connect_socket(host, port);
    }

    void client::close() {
        close_fd(socket_fd_);
    }

    bool client::is_connected() const {
        return socket_fd_ >= 0;
    }

    void client::ping() {
        const uint32_t request_id = next_request_id_++;
        field_map request_fields = {
            {"id", std::to_string(request_id)},
            {"command", "ping"}
        };
        const message response = transact(socket_fd_, request_fields);
        check_response_id(response, request_id);
    }

    target_status client::status() {
        const uint32_t request_id = next_request_id_++;
        field_map request_fields = {
            {"id", std::to_string(request_id)},
            {"command", "status"}
        };
        const message response = transact(socket_fd_, request_fields);
        check_response_id(response, request_id);
        return target_status_from_fields(response.fields);
    }

    cpu_state client::read_registers() {
        const uint32_t request_id = next_request_id_++;
        field_map request_fields = {
            {"id", std::to_string(request_id)},
            {"command", "read_registers"}
        };
        const message response = transact(socket_fd_, request_fields);
        check_response_id(response, request_id);
        return cpu_state_from_fields(response.fields);
    }

    void client::write_registers(const cpu_state& state) {
        const uint32_t request_id = next_request_id_++;
        field_map request_fields = {
            {"id", std::to_string(request_id)},
            {"command", "write_registers"}
        };
        add_fields(request_fields, cpu_state_to_fields(state));
        const message response = transact(socket_fd_, request_fields);
        check_response_id(response, request_id);
    }

    std::vector<uint8_t> client::read_memory(uint32_t address, std::size_t length) {
        const uint32_t request_id = next_request_id_++;
        field_map request_fields = {
            {"id", std::to_string(request_id)},
            {"command", "read_memory"},
            {"address", hex_u32(address)},
            {"length", std::to_string(length)}
        };
        const message response = transact(socket_fd_, request_fields);
        check_response_id(response, request_id);
        return hex_to_bytes(require_field(response.fields, "data"));
    }

    void client::write_memory(uint32_t address, const std::vector<uint8_t>& data) {
        const uint32_t request_id = next_request_id_++;
        field_map request_fields = {
            {"id", std::to_string(request_id)},
            {"command", "write_memory"},
            {"address", hex_u32(address)},
            {"data", bytes_to_hex(data)}
        };
        const message response = transact(socket_fd_, request_fields);
        check_response_id(response, request_id);
    }

    target_status client::continue_execution() {
        const uint32_t request_id = next_request_id_++;
        field_map request_fields = {
            {"id", std::to_string(request_id)},
            {"command", "continue"}
        };
        const message response = transact(socket_fd_, request_fields);
        check_response_id(response, request_id);
        return target_status_from_fields(response.fields);
    }

    target_status client::step_instruction() {
        const uint32_t request_id = next_request_id_++;
        field_map request_fields = {
            {"id", std::to_string(request_id)},
            {"command", "step_instruction"}
        };
        const message response = transact(socket_fd_, request_fields);
        check_response_id(response, request_id);
        return target_status_from_fields(response.fields);
    }

    target_status client::pause_execution() {
        const uint32_t request_id = next_request_id_++;
        field_map request_fields = {
            {"id", std::to_string(request_id)},
            {"command", "pause"}
        };
        const message response = transact(socket_fd_, request_fields);
        check_response_id(response, request_id);
        return target_status_from_fields(response.fields);
    }

    void client::set_breakpoint(uint32_t address) {
        const uint32_t request_id = next_request_id_++;
        field_map request_fields = {
            {"id", std::to_string(request_id)},
            {"command", "set_breakpoint"},
            {"address", hex_u32(address)}
        };
        const message response = transact(socket_fd_, request_fields);
        check_response_id(response, request_id);
    }

    void client::clear_breakpoint(uint32_t address) {
        const uint32_t request_id = next_request_id_++;
        field_map request_fields = {
            {"id", std::to_string(request_id)},
            {"command", "clear_breakpoint"},
            {"address", hex_u32(address)}
        };
        const message response = transact(socket_fd_, request_fields);
        check_response_id(response, request_id);
    }

    void client::detach() {
        const uint32_t request_id = next_request_id_++;
        field_map request_fields = {
            {"id", std::to_string(request_id)},
            {"command", "detach"}
        };
        const message response = transact(socket_fd_, request_fields);
        check_response_id(response, request_id);
    }

    server::~server() {
        close();
    }

    server::server(server&& other) noexcept
        : listen_fd_(other.listen_fd_) {
        other.listen_fd_ = -1;
    }

    server& server::operator=(server&& other) noexcept {
        if (this != &other) {
            close();
            listen_fd_ = other.listen_fd_;
            other.listen_fd_ = -1;
        }
        return *this;
    }

    void server::listen(const std::string& bind_host, uint16_t port) {
        close();
        listen_fd_ = create_listen_socket(bind_host, port);
    }

    void server::close() {
        close_fd(listen_fd_);
    }

    bool server::is_listening() const {
        return listen_fd_ >= 0;
    }

    void server::serve(target& debug_target) {
        if (listen_fd_ < 0) {
            throw error("server is not listening");
        }

        struct sockaddr_storage client_addr {};
        socklen_t client_len = sizeof(client_addr);
        int client_fd = ::accept(listen_fd_,
                                 reinterpret_cast<struct sockaddr*>(&client_addr),
                                 &client_len);
        if (client_fd < 0) {
            throw_errno("accept failed");
        }

        try {
            while (true) {
                const message request = parse_message_line(recv_line(client_fd));
                if (request.kind != "request") {
                    throw protocol_error("expected request message");
                }

                const uint32_t request_id = parse_u32(
                    require_field(request.fields, "id"));
                const std::string command = require_field(request.fields, "command");

                field_map response_fields = base_response_fields(request_id);

                if (command == "ping") {
                    response_fields["message"] = "pong";
                } else if (command == "status") {
                    add_fields(response_fields,
                               target_status_to_fields(debug_target.status()));
                } else if (command == "read_registers") {
                    add_fields(response_fields,
                               cpu_state_to_fields(debug_target.read_registers()));
                } else if (command == "write_registers") {
                    debug_target.write_registers(cpu_state_from_fields(request.fields));
                } else if (command == "read_memory") {
                    const uint32_t address = parse_u32(
                        require_field(request.fields, "address"));
                    const std::size_t length = static_cast<std::size_t>(parse_u32(
                        require_field(request.fields, "length")));
                    response_fields["data"] = bytes_to_hex(
                        debug_target.read_memory(address, length));
                } else if (command == "write_memory") {
                    const uint32_t address = parse_u32(
                        require_field(request.fields, "address"));
                    debug_target.write_memory(address,
                        hex_to_bytes(require_field(request.fields, "data")));
                } else if (command == "continue") {
                    add_fields(response_fields,
                               target_status_to_fields(debug_target.continue_execution()));
                } else if (command == "step_instruction") {
                    add_fields(response_fields,
                               target_status_to_fields(debug_target.step_instruction()));
                } else if (command == "pause") {
                    add_fields(response_fields,
                               target_status_to_fields(debug_target.pause_execution()));
                } else if (command == "set_breakpoint") {
                    debug_target.set_breakpoint(
                        parse_u32(require_field(request.fields, "address")));
                } else if (command == "clear_breakpoint") {
                    debug_target.clear_breakpoint(
                        parse_u32(require_field(request.fields, "address")));
                } else if (command == "detach") {
                    debug_target.detach();
                    send_all(client_fd, format_message("response", response_fields));
                    break;
                } else {
                    throw protocol_error("unknown command '" + command + "'");
                }

                send_all(client_fd, format_message("response", response_fields));
            }
        } catch (...) {
            close_fd(client_fd);
            throw;
        }

        close_fd(client_fd);
    }

} // namespace xdbgstub
