/*
 * Implements the Debug Adapter Protocol frontend for xgdb.
 *
 * This adapts VS Code / DAP requests to the shared debugger_session.  The
 * target-facing side remains RSP, so the same DAP frontend works with xemu
 * and with future hardware targets that speak GDB Remote Serial Protocol.
 *
 * Portions of the DAP framing and request shape are based on the udap project:
 * https://github.com/retro-vault/udap
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */

#include "frontends.h"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace {

    namespace json {

        struct object;
        struct array;

        class value {
        public:
            using storage_type = std::variant<
                std::nullptr_t, bool, int64_t, std::string,
                std::shared_ptr<object>, std::shared_ptr<array>>;

            value() : data_(nullptr) {}
            value(std::nullptr_t) : data_(nullptr) {}
            value(bool v) : data_(v) {}
            value(int v) : data_(static_cast<int64_t>(v)) {}
            value(unsigned int v) : data_(static_cast<int64_t>(v)) {}
            value(int64_t v) : data_(v) {}
            value(const char* v) : data_(std::string(v ? v : "")) {}
            value(const std::string& v) : data_(v) {}
            value(std::string&& v) : data_(std::move(v)) {}
            value(const object& v);
            value(object&& v);
            value(const array& v);
            value(array&& v);
            value(std::initializer_list<std::pair<const char*, value>> init);

            bool is_object() const {
                return std::holds_alternative<std::shared_ptr<object>>(data_);
            }
            bool is_array() const {
                return std::holds_alternative<std::shared_ptr<array>>(data_);
            }
            bool is_string() const {
                return std::holds_alternative<std::string>(data_);
            }
            bool is_bool() const { return std::holds_alternative<bool>(data_); }
            bool is_int64() const { return std::holds_alternative<int64_t>(data_); }
            bool is_uint64() const {
                return is_int64() && std::get<int64_t>(data_) >= 0;
            }
            const object& as_object() const {
                return *std::get<std::shared_ptr<object>>(data_);
            }
            object& as_object() {
                return *std::get<std::shared_ptr<object>>(data_);
            }
            const array& as_array() const {
                return *std::get<std::shared_ptr<array>>(data_);
            }
            array& as_array() {
                return *std::get<std::shared_ptr<array>>(data_);
            }
            const std::string& as_string() const {
                return std::get<std::string>(data_);
            }
            bool as_bool() const { return std::get<bool>(data_); }
            int64_t as_int64() const { return std::get<int64_t>(data_); }
            uint64_t as_uint64() const {
                return static_cast<uint64_t>(std::get<int64_t>(data_));
            }

            const storage_type& data() const { return data_; }

        private:
            storage_type data_;
        };

        struct object : std::map<std::string, value> {
            using std::map<std::string, value>::map;
        };

        struct array : std::vector<value> {
            using std::vector<value>::vector;
        };

        value::value(const object& v) : data_(std::make_shared<object>(v)) {}
        value::value(object&& v)
            : data_(std::make_shared<object>(std::move(v))) {}
        value::value(const array& v) : data_(std::make_shared<array>(v)) {}
        value::value(array&& v)
            : data_(std::make_shared<array>(std::move(v))) {}
        value::value(std::initializer_list<std::pair<const char*, value>> init)
            : data_(std::make_shared<object>()) {
            auto& obj = *std::get<std::shared_ptr<object>>(data_);
            for (const auto& item : init)
                obj.emplace(item.first, item.second);
        }

        class parser {
        public:
            explicit parser(std::string input) : input_(std::move(input)) {}

            value parse() {
                auto v = parse_value();
                skip_ws();
                if (pos_ != input_.size())
                    throw std::runtime_error("trailing data after JSON value");
                return v;
            }

        private:
            void skip_ws() {
                while (pos_ < input_.size()
                       && std::isspace(static_cast<unsigned char>(input_[pos_])))
                    ++pos_;
            }

            char peek() {
                skip_ws();
                if (pos_ >= input_.size())
                    throw std::runtime_error("unexpected end of JSON input");
                return input_[pos_];
            }

            bool consume(char c) {
                skip_ws();
                if (pos_ < input_.size() && input_[pos_] == c) {
                    ++pos_;
                    return true;
                }
                return false;
            }

            void expect(char c) {
                if (!consume(c)) {
                    std::string msg = "expected JSON character '";
                    msg.push_back(c);
                    msg += "'";
                    throw std::runtime_error(msg);
                }
            }

            value parse_value() {
                const char c = peek();
                if (c == '{') return parse_object();
                if (c == '[') return parse_array();
                if (c == '"') return parse_string();
                if (c == '-' || std::isdigit(static_cast<unsigned char>(c)))
                    return parse_number();
                if (input_.compare(pos_, 4, "true") == 0) {
                    pos_ += 4;
                    return true;
                }
                if (input_.compare(pos_, 5, "false") == 0) {
                    pos_ += 5;
                    return false;
                }
                if (input_.compare(pos_, 4, "null") == 0) {
                    pos_ += 4;
                    return nullptr;
                }
                throw std::runtime_error("invalid JSON value");
            }

            object parse_object() {
                expect('{');
                object obj;
                if (consume('}')) return obj;
                for (;;) {
                    auto key = parse_string().as_string();
                    expect(':');
                    obj[key] = parse_value();
                    if (consume('}')) break;
                    expect(',');
                }
                return obj;
            }

            array parse_array() {
                expect('[');
                array arr;
                if (consume(']')) return arr;
                for (;;) {
                    arr.push_back(parse_value());
                    if (consume(']')) break;
                    expect(',');
                }
                return arr;
            }

            value parse_string() {
                expect('"');
                std::string out;
                while (pos_ < input_.size()) {
                    char c = input_[pos_++];
                    if (c == '"') return out;
                    if (c != '\\') {
                        out.push_back(c);
                        continue;
                    }
                    if (pos_ >= input_.size())
                        throw std::runtime_error("unterminated JSON escape");
                    c = input_[pos_++];
                    switch (c) {
                    case '"': out.push_back('"'); break;
                    case '\\': out.push_back('\\'); break;
                    case '/': out.push_back('/'); break;
                    case 'b': out.push_back('\b'); break;
                    case 'f': out.push_back('\f'); break;
                    case 'n': out.push_back('\n'); break;
                    case 'r': out.push_back('\r'); break;
                    case 't': out.push_back('\t'); break;
                    case 'u': {
                        uint32_t codepoint = parse_hex4();
                        if (codepoint >= 0xd800 && codepoint <= 0xdbff) {
                            if (pos_ + 6 > input_.size()
                                || input_[pos_] != '\\'
                                || input_[pos_ + 1] != 'u') {
                                throw std::runtime_error("missing JSON surrogate pair");
                            }
                            pos_ += 2;
                            const uint32_t low = parse_hex4();
                            if (low < 0xdc00 || low > 0xdfff)
                                throw std::runtime_error("invalid JSON surrogate pair");
                            codepoint = 0x10000
                                + ((codepoint - 0xd800) << 10)
                                + (low - 0xdc00);
                        } else if (codepoint >= 0xdc00 && codepoint <= 0xdfff) {
                            throw std::runtime_error("unexpected JSON low surrogate");
                        }
                        append_utf8(out, codepoint);
                        break;
                    }
                    default:
                        throw std::runtime_error("invalid JSON escape");
                    }
                }
                throw std::runtime_error("unterminated JSON string");
            }

            uint32_t parse_hex4() {
                if (pos_ + 4 > input_.size())
                    throw std::runtime_error("short JSON unicode escape");
                uint32_t value = 0;
                for (int i = 0; i < 4; ++i) {
                    const char c = input_[pos_++];
                    value <<= 4;
                    if (c >= '0' && c <= '9') value |= static_cast<uint32_t>(c - '0');
                    else if (c >= 'a' && c <= 'f')
                        value |= static_cast<uint32_t>(c - 'a' + 10);
                    else if (c >= 'A' && c <= 'F')
                        value |= static_cast<uint32_t>(c - 'A' + 10);
                    else
                        throw std::runtime_error("invalid JSON unicode escape");
                }
                return value;
            }

            static void append_utf8(std::string& out, uint32_t codepoint) {
                if (codepoint <= 0x7f) {
                    out.push_back(static_cast<char>(codepoint));
                } else if (codepoint <= 0x7ff) {
                    out.push_back(static_cast<char>(0xc0 | (codepoint >> 6)));
                    out.push_back(static_cast<char>(0x80 | (codepoint & 0x3f)));
                } else if (codepoint <= 0xffff) {
                    out.push_back(static_cast<char>(0xe0 | (codepoint >> 12)));
                    out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3f)));
                    out.push_back(static_cast<char>(0x80 | (codepoint & 0x3f)));
                } else if (codepoint <= 0x10ffff) {
                    out.push_back(static_cast<char>(0xf0 | (codepoint >> 18)));
                    out.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3f)));
                    out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3f)));
                    out.push_back(static_cast<char>(0x80 | (codepoint & 0x3f)));
                } else {
                    throw std::runtime_error("JSON unicode codepoint out of range");
                }
            }

            value parse_number() {
                const auto start = pos_;
                if (input_[pos_] == '-') ++pos_;
                while (pos_ < input_.size()
                       && std::isdigit(static_cast<unsigned char>(input_[pos_])))
                    ++pos_;
                if (pos_ < input_.size()
                    && (input_[pos_] == '.' || input_[pos_] == 'e'
                        || input_[pos_] == 'E')) {
                    throw std::runtime_error("floating point JSON numbers are unsupported");
                }
                return static_cast<int64_t>(
                    std::stoll(input_.substr(start, pos_ - start)));
            }

            std::string input_;
            std::size_t pos_ = 0;
        };

        std::string escape_string(const std::string& s) {
            std::ostringstream out;
            out << '"';
            for (unsigned char c : s) {
                switch (c) {
                case '"': out << "\\\""; break;
                case '\\': out << "\\\\"; break;
                case '\b': out << "\\b"; break;
                case '\f': out << "\\f"; break;
                case '\n': out << "\\n"; break;
                case '\r': out << "\\r"; break;
                case '\t': out << "\\t"; break;
                default:
                    if (c < 0x20) {
                        out << "\\u" << std::hex << std::setw(4)
                            << std::setfill('0') << static_cast<int>(c)
                            << std::dec << std::setfill(' ');
                    } else {
                        out << static_cast<char>(c);
                    }
                    break;
                }
            }
            out << '"';
            return out.str();
        }

        std::string serialize(const value& v);

        std::string serialize(const object& obj) {
            std::ostringstream out;
            out << '{';
            bool first = true;
            for (const auto& [key, item] : obj) {
                if (!first) out << ',';
                first = false;
                out << escape_string(key) << ':' << serialize(item);
            }
            out << '}';
            return out.str();
        }

        std::string serialize(const array& arr) {
            std::ostringstream out;
            out << '[';
            for (std::size_t i = 0; i < arr.size(); ++i) {
                if (i) out << ',';
                out << serialize(arr[i]);
            }
            out << ']';
            return out.str();
        }

        std::string serialize(const value& v) {
            const auto& data = v.data();
            if (std::holds_alternative<std::nullptr_t>(data)) return "null";
            if (std::holds_alternative<bool>(data))
                return std::get<bool>(data) ? "true" : "false";
            if (std::holds_alternative<int64_t>(data))
                return std::to_string(std::get<int64_t>(data));
            if (std::holds_alternative<std::string>(data))
                return escape_string(std::get<std::string>(data));
            if (std::holds_alternative<std::shared_ptr<object>>(data))
                return serialize(*std::get<std::shared_ptr<object>>(data));
            return serialize(*std::get<std::shared_ptr<array>>(data));
        }

        value parse(const std::string& input) {
            return parser(input).parse();
        }

    } // namespace json

    constexpr int THREAD_ID = 1;
    constexpr int FRAME_ID = 1;
    constexpr int REGISTERS_REF = 1;
    constexpr int LOCALS_REF = 2;
    constexpr int GLOBALS_REF = 3;

    std::string trim(const std::string& value) {
        const auto start = value.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) return "";
        const auto end = value.find_last_not_of(" \t\r\n");
        return value.substr(start, end - start + 1);
    }

    std::string json_string(const json::value& value, const char* key,
                            const std::string& fallback = {}) {
        if (!value.is_object()) return fallback;
        const auto& obj = value.as_object();
        auto it = obj.find(key);
        if (it == obj.end() || !it->second.is_string()) return fallback;
        return std::string(it->second.as_string());
    }

    bool json_bool(const json::value& value, const char* key, bool fallback = false) {
        if (!value.is_object()) return fallback;
        const auto& obj = value.as_object();
        auto it = obj.find(key);
        if (it == obj.end() || !it->second.is_bool()) return fallback;
        return it->second.as_bool();
    }

    int json_int(const json::value& value, const char* key, int fallback = 0) {
        if (!value.is_object()) return fallback;
        const auto& obj = value.as_object();
        auto it = obj.find(key);
        if (it == obj.end()) return fallback;
        if (it->second.is_int64()) return static_cast<int>(it->second.as_int64());
        if (it->second.is_uint64()) return static_cast<int>(it->second.as_uint64());
        return fallback;
    }

    std::optional<uint32_t> json_u32_any(
        const json::value& value,
        const char* key) {
        if (!value.is_object()) return std::nullopt;
        const auto& obj = value.as_object();
        auto it = obj.find(key);
        if (it == obj.end()) return std::nullopt;
        if (it->second.is_int64())
            return static_cast<uint32_t>(it->second.as_int64());
        if (it->second.is_uint64())
            return static_cast<uint32_t>(it->second.as_uint64());
        if (it->second.is_string())
            return static_cast<uint32_t>(
                std::strtoul(std::string(it->second.as_string()).c_str(),
                             nullptr, 0));
        return std::nullopt;
    }

    std::optional<json::value> json_member(const json::value& value, const char* key) {
        if (!value.is_object()) return std::nullopt;
        const auto& obj = value.as_object();
        auto it = obj.find(key);
        if (it == obj.end()) return std::nullopt;
        return it->second;
    }

    uint32_t parse_u32(const std::string& value) {
        char* end = nullptr;
        const unsigned long parsed = std::strtoul(value.c_str(), &end, 0);
        if (end == value.c_str() || *end != '\0')
            throw std::runtime_error("invalid number: " + value);
        return static_cast<uint32_t>(parsed);
    }

    std::string hex_u32(uint32_t value) {
        std::ostringstream out;
        out << "0x" << std::hex << std::uppercase << value << std::dec;
        return out.str();
    }

    std::string hex_u16(uint32_t value) {
        std::ostringstream out;
        out << "0x" << std::hex << std::uppercase
            << std::setw(4) << std::setfill('0') << (value & 0xffffu)
            << std::dec;
        return out.str();
    }

    std::string bytes_hex(const std::vector<uint8_t>& bytes) {
        std::ostringstream out;
        for (uint8_t byte : bytes) {
            out << std::setw(2) << std::setfill('0')
                << std::hex << std::nouppercase << static_cast<int>(byte);
        }
        return out.str();
    }

    std::string base64_encode(const std::vector<uint8_t>& bytes) {
        static constexpr char B64[] =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::string out;
        out.reserve(((bytes.size() + 2) / 3) * 4);
        for (std::size_t i = 0; i < bytes.size(); i += 3) {
            uint32_t n = static_cast<uint32_t>(bytes[i]) << 16;
            if (i + 1 < bytes.size()) n |= static_cast<uint32_t>(bytes[i + 1]) << 8;
            if (i + 2 < bytes.size()) n |= static_cast<uint32_t>(bytes[i + 2]);
            out.push_back(B64[(n >> 18) & 0x3f]);
            out.push_back(B64[(n >> 12) & 0x3f]);
            out.push_back(i + 1 < bytes.size() ? B64[(n >> 6) & 0x3f] : '=');
            out.push_back(i + 2 < bytes.size() ? B64[n & 0x3f] : '=');
        }
        return out;
    }

    std::string mime_for_path(const std::string& path) {
        const auto ext = std::filesystem::path(path).extension().string();
        if (ext == ".c" || ext == ".h") return "text/x-c";
        if (ext == ".s" || ext == ".S" || ext == ".asm" || ext == ".lst")
            return "text/x-asm";
        return "text/plain";
    }

    std::string read_all_text(const std::filesystem::path& path) {
        std::ifstream in(path);
        if (!in) throw std::runtime_error("cannot read source: " + path.string());
        std::ostringstream out;
        out << in.rdbuf();
        return out.str();
    }

    std::optional<uint32_t> resolve_line(debugger_host& session,
                                         const std::string& path,
                                         int line) {
        try {
            const auto result = session.info_line_argument(
                path + ":" + std::to_string(line));
            return result.location.address;
        } catch (const std::exception&) {
            try {
                const auto result = session.info_line_argument(
                    std::filesystem::path(path).filename().string()
                    + ":" + std::to_string(line));
                return result.location.address;
            } catch (const std::exception&) {
                return std::nullopt;
            }
        }
    }

    std::string variable_storage_value(const xgdb::variable& variable,
                                       debugger_host& session) {
        try {
            if (variable.storage == xgdb::storage_kind::address
                && variable.address.has_value()) {
                const auto bytes = session.read_memory(variable.address.value(), 1);
                if (!bytes.empty()) {
                    return hex_u32(bytes[0]);
                }
                return hex_u32(variable.address.value());
            }
        } catch (const std::exception&) {
        }
        if (variable.storage == xgdb::storage_kind::frame_relative
            || variable.storage == xgdb::storage_kind::stack) {
            if (variable.offset.has_value()) return std::to_string(variable.offset.value());
        }
        if (variable.storage == xgdb::storage_kind::register_name
            || variable.storage == xgdb::storage_kind::register_pair) {
            if (variable.register_name.has_value()) return variable.register_name.value();
        }
        return "";
    }

    std::string stop_reason(xgdb::stop_reason reason) {
        switch (reason) {
        case xgdb::stop_reason::breakpoint: return "breakpoint";
        case xgdb::stop_reason::step: return "step";
        case xgdb::stop_reason::pause: return "pause";
        case xgdb::stop_reason::exited: return "exited";
        case xgdb::stop_reason::halted: return "exception";
        case xgdb::stop_reason::signal: return "exception";
        case xgdb::stop_reason::none: return "step";
        }
        return "exception";
    }

    class dap_io {
    public:
        dap_io(std::istream& input, std::ostream& output, std::ostream* log)
            : input_(input), output_(output), log_(log) {}

        std::optional<json::object> read_request() {
            int content_length = -1;
            std::string line;
            while (std::getline(input_, line)) {
                if (!line.empty() && line.back() == '\r') line.pop_back();
                if (line.empty()) break;
                if (line.rfind("Content-Length:", 0) == 0) {
                    content_length = std::stoi(trim(line.substr(15)));
                }
            }
            if (!input_ || content_length < 0) return std::nullopt;
            std::string payload(static_cast<std::size_t>(content_length), '\0');
            input_.read(payload.data(), content_length);
            if (input_.gcount() != content_length) return std::nullopt;
            if (log_) *log_ << "[DAP IN] " << payload << "\n" << std::flush;
            auto value = json::parse(payload);
            if (!value.is_object()) return std::nullopt;
            return value.as_object();
        }

        void send(const json::object& message) {
            const std::string payload = json::serialize(message);
            if (log_) *log_ << "[DAP OUT] " << payload << "\n" << std::flush;
            std::lock_guard lock(output_mutex_);
            output_ << "Content-Length: " << payload.size()
                    << "\r\n\r\n" << payload << std::flush;
        }

    private:
        std::istream& input_;
        std::ostream& output_;
        std::ostream* log_;
        std::mutex output_mutex_;
    };

    class dap_session {
    public:
        dap_session(debugger& dbg, dap_io& io)
            : debugger_(dbg), io_(io) {}

        bool handle(const json::object& request) {
            const int request_seq = json_int(request, "seq", 0);
            const std::string command = json_string(request, "command");
            const json::value arguments = json_member(request, "arguments")
                .value_or(json::object{});

            try {
                if (command == "initialize") return initialize(request_seq, command);
                if (command == "launch") return launch(request_seq, command, arguments);
                if (command == "attach") return attach(request_seq, command, arguments);
                if (command == "configurationDone") return configuration_done(request_seq, command);
                if (command == "disconnect" || command == "terminate")
                    return disconnect(request_seq, command);
                if (command == "threads") return threads(request_seq, command);
                if (command == "stackTrace") return stack_trace(request_seq, command);
                if (command == "scopes") return scopes(request_seq, command);
                if (command == "variables") return variables(request_seq, command, arguments);
                if (command == "setBreakpoints")
                    return set_breakpoints(request_seq, command, arguments);
                if (command == "setFunctionBreakpoints")
                    return set_function_breakpoints(request_seq, command, arguments);
                if (command == "setInstructionBreakpoints")
                    return set_instruction_breakpoints(request_seq, command, arguments);
                if (command == "setExceptionBreakpoints")
                    return respond(request_seq, command, true, json::object{});
                if (command == "breakpointLocations")
                    return breakpoint_locations(request_seq, command, arguments);
                if (command == "continue")
                    return continue_execution(request_seq, command);
                if (command == "next" || command == "stepIn" || command == "stepOut")
                    return step(request_seq, command);
                if (command == "pause") return pause(request_seq, command);
                if (command == "source") return source(request_seq, command, arguments);
                if (command == "readMemory") return read_memory(request_seq, command, arguments);
                if (command == "disassemble") return disassemble(request_seq, command, arguments);
                if (command == "evaluate") return evaluate(request_seq, command, arguments);
                if (command == "loadedSources") return loaded_sources(request_seq, command);
                return respond_error(request_seq, command, "unsupported dap command: " + command);
            } catch (const std::exception& e) {
                return respond_error(request_seq, command, e.what());
            }
        }

        void join_worker() {
            if (worker_.joinable()) worker_.join();
        }

    private:
        bool respond(int request_seq, const std::string& command, bool success,
                     const json::object& body = {}) {
            json::object response;
            response["seq"] = next_seq_++;
            response["type"] = "response";
            response["request_seq"] = request_seq;
            response["success"] = success;
            response["command"] = command;
            response["body"] = body;
            io_.send(response);
            return command != "disconnect" && command != "terminate";
        }

        bool respond_error(int request_seq, const std::string& command,
                           const std::string& message) {
            json::object response;
            response["seq"] = next_seq_++;
            response["type"] = "response";
            response["request_seq"] = request_seq;
            response["success"] = false;
            response["command"] = command;
            response["message"] = message;
            io_.send(response);
            return true;
        }

        void event(const std::string& name, const json::object& body = {}) {
            json::object ev;
            ev["seq"] = next_seq_++;
            ev["type"] = "event";
            ev["event"] = name;
            ev["body"] = body;
            io_.send(ev);
        }

        void stopped_event(const stop_snapshot& stop, const std::string& override_reason = {}) {
            json::object body;
            body["reason"] = override_reason.empty()
                ? stop_reason(stop.reason)
                : override_reason;
            body["threadId"] = THREAD_ID;
            body["allThreadsStopped"] = true;
            event("stopped", body);
        }

        bool initialize(int request_seq, const std::string& command) {
            json::object body;
            body["supportsConfigurationDoneRequest"] = true;
            body["supportsFunctionBreakpoints"] = true;
            body["supportsInstructionBreakpoints"] = true;
            body["supportsBreakpointLocationsRequest"] = true;
            body["supportsLoadedSourcesRequest"] = true;
            body["supportsDisassembleRequest"] = true;
            body["supportsEvaluateForHovers"] = true;
            body["supportsMemoryReferences"] = true;
            body["supportsReadMemoryRequest"] = true;
            body["supportsSetVariable"] = false;
            body["supportsRestartFrame"] = false;
            body["supportsTerminateDebuggee"] = false;
            respond(request_seq, command, true, body);
            event("initialized");
            return true;
        }

        bool launch(int request_seq, const std::string& command,
                    const json::value& arguments) {
            const auto program = json_string(arguments, "program");
            const auto cdb = json_string(arguments, "cdbFile");
            const auto map = json_string(arguments, "mapFile");
            const auto symbols = json_string(arguments, "symbols");
            const auto remote = json_string(arguments, "remoteTarget");
            const bool stop_on_entry = json_bool(arguments, "stopOnEntry", true);
            const bool no_load = json_bool(arguments, "noLoad", false);

            if (!program.empty()) {
                debugger_.set_exec_path(program);
                if (!symbols.empty()
                    && std::filesystem::path(symbols).extension() == ".elf") {
                    debugger_.load_elf_file(symbols);
                } else if (!cdb.empty()) {
                    debugger_.load_cdb_file(cdb);
                } else {
                    debugger_.maybe_load_default_symbols();
                }
            }
            if (!map.empty()) debugger_.load_map_file(map);
            debugger_.session().set_download_enabled(!no_load);
            if (auto origin = json_u32_any(arguments, "origin"); origin.has_value())
                debugger_.session().set_download_origin(*origin);
            auto pc = json_u32_any(arguments, "pc");
            if (!pc.has_value())
                pc = json_u32_any(arguments, "startAddress");
            debugger_.session().set_download_pc(pc);
            if (!remote.empty()) debugger_.connect_remote(remote);

            launched_ = true;
            stop_on_entry_ = stop_on_entry;
            return respond(request_seq, command, true, json::object{});
        }

        bool attach(int request_seq, const std::string& command,
                    const json::value& arguments) {
            const auto program = json_string(arguments, "program");
            const auto cdb = json_string(arguments, "cdbFile");
            const auto map = json_string(arguments, "mapFile");
            const auto remote = json_string(arguments, "remoteTarget");
            const bool load = json_bool(arguments, "load", false);
            if (!program.empty()) {
                debugger_.set_exec_path(program);
                if (!cdb.empty()) debugger_.load_cdb_file(cdb);
                else debugger_.maybe_load_default_symbols();
            }
            if (!map.empty()) debugger_.load_map_file(map);
            debugger_.session().set_download_enabled(load);
            if (auto origin = json_u32_any(arguments, "origin"); origin.has_value())
                debugger_.session().set_download_origin(*origin);
            auto pc = json_u32_any(arguments, "pc");
            if (!pc.has_value())
                pc = json_u32_any(arguments, "startAddress");
            debugger_.session().set_download_pc(pc);
            if (!remote.empty()) debugger_.connect_remote(remote);
            launched_ = true;
            return respond(request_seq, command, true, json::object{});
        }

        bool configuration_done(int request_seq, const std::string& command) {
            respond(request_seq, command, true, json::object{});
            if (launched_ && stop_on_entry_ && debugger_.session().is_connected()) {
                stopped_event(debugger_.status(), "entry");
            }
            return true;
        }

        bool disconnect(int request_seq, const std::string& command) {
            join_worker();
            if (debugger_.session().is_connected()) {
                try { debugger_.detach(); } catch (const std::exception&) {}
            }
            respond(request_seq, command, true, json::object{});
            event("terminated");
            return false;
        }

        bool threads(int request_seq, const std::string& command) {
            json::array threads;
            threads.push_back({{"id", THREAD_ID}, {"name", "main"}});
            return respond(request_seq, command, true, {{"threads", threads}});
        }

        json::object stack_frame() {
            const auto stop = debugger_.status();
            json::object frame;
            frame["id"] = FRAME_ID;
            frame["name"] = stop.function_name.value_or(
                stop.source && stop.source->function_name
                    ? *stop.source->function_name
                    : "0x" + hex_u32(stop.pc).substr(2));
            frame["line"] = 0;
            frame["column"] = 1;
            frame["instructionPointerReference"] = hex_u16(stop.pc);
            frame["memoryReference"] = hex_u16(stop.pc);
            if (stop.source.has_value()) {
                frame["line"] = stop.source->line;
                frame["column"] = std::max<uint32_t>(stop.source->column, 1);
                json::object source;
                source["name"] = std::filesystem::path(
                    stop.source->file_path).filename().string();
                source["path"] = stop.source->file_path;
                source["sourceReference"] = source_reference(stop.source->file_path);
                frame["source"] = source;
            }
            return frame;
        }

        bool stack_trace(int request_seq, const std::string& command) {
            json::array frames;
            frames.push_back(stack_frame());
            return respond(request_seq, command, true,
                           {{"stackFrames", frames}, {"totalFrames", 1}});
        }

        bool scopes(int request_seq, const std::string& command) {
            json::array scopes;
            scopes.push_back({{"name", "Registers"},
                              {"variablesReference", REGISTERS_REF},
                              {"presentationHint", "registers"},
                              {"expensive", false}});
            scopes.push_back({{"name", "Locals"},
                              {"variablesReference", LOCALS_REF},
                              {"presentationHint", "locals"},
                              {"expensive", false}});
            scopes.push_back({{"name", "Globals"},
                              {"variablesReference", GLOBALS_REF},
                              {"presentationHint", "globals"},
                              {"expensive", false}});
            return respond(request_seq, command, true, {{"scopes", scopes}});
        }

        bool variables(int request_seq, const std::string& command,
                       const json::value& arguments) {
            const int ref = json_int(arguments, "variablesReference");
            json::array vars;
            if (ref == REGISTERS_REF) {
                for (const auto& reg : debugger_.session().register_values()) {
                    vars.push_back({{"name", reg.first},
                                    {"value", hex_u32(reg.second)},
                                    {"variablesReference", 0}});
                }
            } else {
                const auto pc = debugger_.session().read_registers().pc;
                for (const auto* var : debugger_.session().visible_variables(pc)) {
                    const bool is_global = var->kind == xgdb::symbol_kind::global;
                    if ((ref == GLOBALS_REF) != is_global) continue;
                    json::object item;
                    item["name"] = var->name;
                    item["value"] = variable_storage_value(*var, debugger_.session());
                    item["variablesReference"] = 0;
                    if (var->type_name.has_value()) item["type"] = *var->type_name;
                    if (var->address.has_value())
                        item["memoryReference"] = hex_u16(var->address.value());
                    vars.push_back(item);
                }
            }
            return respond(request_seq, command, true, {{"variables", vars}});
        }

        bool set_breakpoints(int request_seq, const std::string& command,
                             const json::value& arguments) {
            std::string path;
            if (auto source = json_member(arguments, "source"); source.has_value()) {
                path = json_string(*source, "path");
                if (path.empty()) path = json_string(*source, "name");
            }

            for (int id : source_breakpoints_[path]) {
                try { debugger_.delete_breakpoint(id); } catch (const std::exception&) {}
            }
            source_breakpoints_[path].clear();

            json::array out;
            if (auto bps = json_member(arguments, "breakpoints");
                bps.has_value() && bps->is_array()) {
                for (const auto& bp : bps->as_array()) {
                    const int line = json_int(bp, "line", 1);
                    json::object item;
                    item["line"] = line;
                    if (auto addr = resolve_line(debugger_.session(), path, line);
                        addr.has_value()) {
                        const auto record = debugger_.add_breakpoint(
                            "*" + std::to_string(*addr));
                        source_breakpoints_[path].push_back(record.id);
                        item["verified"] = true;
                        item["instructionReference"] = hex_u16(record.address);
                    } else {
                        item["verified"] = false;
                        item["message"] = "no code generated for this source line";
                    }
                    out.push_back(item);
                }
            }
            return respond(request_seq, command, true, {{"breakpoints", out}});
        }

        bool set_function_breakpoints(int request_seq, const std::string& command,
                                      const json::value& arguments) {
            for (int id : function_breakpoints_) {
                try { debugger_.delete_breakpoint(id); } catch (const std::exception&) {}
            }
            function_breakpoints_.clear();
            json::array out;
            if (auto bps = json_member(arguments, "breakpoints");
                bps.has_value() && bps->is_array()) {
                for (const auto& bp : bps->as_array()) {
                    const auto name = json_string(bp, "name");
                    json::object item;
                    try {
                        const auto record = debugger_.add_breakpoint(name);
                        function_breakpoints_.push_back(record.id);
                        item["verified"] = true;
                        item["instructionReference"] = hex_u16(record.address);
                    } catch (const std::exception& e) {
                        item["verified"] = false;
                        item["message"] = e.what();
                    }
                    out.push_back(item);
                }
            }
            return respond(request_seq, command, true, {{"breakpoints", out}});
        }

        bool set_instruction_breakpoints(int request_seq, const std::string& command,
                                         const json::value& arguments) {
            for (int id : instruction_breakpoints_) {
                try { debugger_.delete_breakpoint(id); } catch (const std::exception&) {}
            }
            instruction_breakpoints_.clear();
            json::array out;
            if (auto bps = json_member(arguments, "breakpoints");
                bps.has_value() && bps->is_array()) {
                for (const auto& bp : bps->as_array()) {
                    const auto ref = json_string(bp, "instructionReference");
                    json::object item;
                    try {
                        const auto record = debugger_.add_breakpoint(ref);
                        instruction_breakpoints_.push_back(record.id);
                        item["verified"] = true;
                        item["instructionReference"] = hex_u16(record.address);
                    } catch (const std::exception& e) {
                        item["verified"] = false;
                        item["message"] = e.what();
                    }
                    out.push_back(item);
                }
            }
            return respond(request_seq, command, true, {{"breakpoints", out}});
        }

        bool breakpoint_locations(int request_seq, const std::string& command,
                                  const json::value& arguments) {
            std::string path;
            if (auto source = json_member(arguments, "source"); source.has_value()) {
                path = json_string(*source, "path");
            }
            const int start = json_int(arguments, "line", 1);
            int end = json_int(arguments, "endLine", start);
            if (end < start) end = start;
            json::array locs;
            for (int line = start; line <= end; ++line) {
                if (resolve_line(debugger_.session(), path, line).has_value()) {
                    json::object loc;
                    loc["line"] = line;
                    locs.push_back(loc);
                }
            }
            return respond(request_seq, command, true, {{"breakpoints", locs}});
        }

        bool continue_execution(int request_seq, const std::string& command) {
            respond(request_seq, command, true, {{"allThreadsContinued", true}});
            join_worker();
            worker_ = std::thread([this]() {
                try {
                    const auto stop = debugger_.continue_execution();
                    stopped_event(stop);
                } catch (const std::exception& e) {
                    json::object body;
                    body["output"] = std::string(e.what()) + "\n";
                    body["category"] = "stderr";
                    event("output", body);
                }
            });
            return true;
        }

        bool step(int request_seq, const std::string& command) {
            const auto stop = debugger_.step_instruction();
            respond(request_seq, command, true, {{"allThreadsContinued", false}});
            stopped_event(stop, "step");
            return true;
        }

        bool pause(int request_seq, const std::string& command) {
            const auto stop = debugger_.pause_execution();
            respond(request_seq, command, true, json::object{});
            stopped_event(stop, "pause");
            return true;
        }

        bool source(int request_seq, const std::string& command,
                    const json::value& arguments) {
            std::string path;
            const int ref = json_int(arguments, "sourceReference", 0);
            if (ref > 0) {
                auto it = ref_to_path_.find(ref);
                if (it != ref_to_path_.end()) path = it->second;
            }
            if (path.empty()) {
                if (auto source = json_member(arguments, "source"); source.has_value())
                    path = json_string(*source, "path");
            }
            if (path.empty())
                return respond_error(request_seq, command, "unknown source");
            return respond(request_seq, command, true,
                           {{"content", read_all_text(path)},
                            {"mimeType", mime_for_path(path)}});
        }

        bool read_memory(int request_seq, const std::string& command,
                         const json::value& arguments) {
            const auto mem_ref = json_string(arguments, "memoryReference", "0");
            const int offset = json_int(arguments, "offset", 0);
            const int count = std::max(0, json_int(arguments, "count", 0));
            const uint32_t address = (parse_u32(mem_ref) + offset) & 0xffffu;
            const auto bytes = debugger_.session().read_memory(address, count);
            return respond(request_seq, command, true,
                           {{"address", hex_u16(address)},
                            {"data", base64_encode(bytes)},
                            {"unreadableBytes", 0}});
        }

        bool disassemble(int request_seq, const std::string& command,
                         const json::value& arguments) {
            const auto mem_ref = json_string(arguments, "memoryReference", "0");
            const int offset = json_int(arguments, "offset", 0)
                + json_int(arguments, "instructionOffset", 0);
            const int count = std::max(0, json_int(arguments, "instructionCount", 0));
            const uint32_t address = (parse_u32(mem_ref) + offset) & 0xffffu;
            json::array instructions;
            for (const auto& line : debugger_.session().disassemble(address, count)) {
                json::object item;
                item["address"] = hex_u16(line.address);
                item["instructionBytes"] = bytes_hex(line.bytes);
                item["instruction"] = line.text;
                if (auto loc = debugger_.session().source_location_for_address(line.address);
                    loc.has_value()) {
                    item["line"] = loc->line;
                    item["location"] = {{"path", loc->file_path},
                                        {"sourceReference", source_reference(loc->file_path)}};
                }
                instructions.push_back(item);
            }
            return respond(request_seq, command, true,
                           {{"instructions", instructions}});
        }

        bool evaluate(int request_seq, const std::string& command,
                      const json::value& arguments) {
            const auto expr = json_string(arguments, "expression");
            try {
                const auto value = debugger_.session().resolve_address_expression(expr);
                return respond(request_seq, command, true,
                               {{"result", hex_u32(value)},
                                {"variablesReference", 0},
                                {"memoryReference", hex_u16(value)}});
            } catch (const std::exception& e) {
                return respond_error(request_seq, command, e.what());
            }
        }

        bool loaded_sources(int request_seq, const std::string& command) {
            json::array sources;
            if (const auto* doc = debugger_.session().symbols()) {
                for (const auto& file : doc->files) {
                    sources.push_back({{"name", std::filesystem::path(file.path)
                                            .filename().string()},
                                       {"path", file.path},
                                       {"sourceReference", source_reference(file.path)}});
                }
            }
            return respond(request_seq, command, true, {{"sources", sources}});
        }

        int source_reference(const std::string& path) {
            auto it = path_to_ref_.find(path);
            if (it != path_to_ref_.end()) return it->second;
            const int ref = next_source_ref_++;
            path_to_ref_[path] = ref;
            ref_to_path_[ref] = path;
            return ref;
        }

        debugger& debugger_;
        dap_io& io_;
        std::atomic<int> next_seq_{1};
        bool launched_ = false;
        bool stop_on_entry_ = true;
        int next_source_ref_ = 1000;
        std::unordered_map<std::string, int> path_to_ref_;
        std::unordered_map<int, std::string> ref_to_path_;
        std::map<std::string, std::vector<int>> source_breakpoints_;
        std::vector<int> function_breakpoints_;
        std::vector<int> instruction_breakpoints_;
        std::thread worker_;
    };

} // namespace

dap_frontend::dap_frontend(debugger_host& host)
    : debugger_(host) {
    debugger_.set_event_sink(this);
}

void dap_frontend::set_log(std::ostream* log) { log_ = log; }

int dap_frontend::run() {
    dap_io io(std::cin, std::cout, log_);
    dap_session session(debugger_, io);

    while (auto request = io.read_request()) {
        if (!session.handle(*request)) break;
    }

    session.join_worker();
    return 0;
}

std::unique_ptr<debug_protocol> make_dap_protocol(debugger_host& host) {
    return std::make_unique<dap_frontend>(host);
}
