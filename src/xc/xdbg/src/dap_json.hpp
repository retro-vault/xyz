/*
 * Provides a small self-contained JSON value, parser, and writer
 * implementation for the xdbg DAP frontend.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */

#ifndef XDBG_DAP_JSON_HPP
#define XDBG_DAP_JSON_HPP

#include <cctype>
#include <cstdint>
#include <map>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

class json_value {
public:
    enum class kind {
        null_value,
        boolean,
        number,
        string,
        array,
        object
    };

    using array_type = std::vector<json_value>;
    using object_type = std::map<std::string, json_value>;

    json_value() = default;
    json_value(std::nullptr_t) {}
    json_value(bool value)
        : kind_(kind::boolean), boolean_(value) {}
    json_value(int64_t value)
        : kind_(kind::number), number_(value) {}
    json_value(uint64_t value)
        : kind_(kind::number), number_(static_cast<int64_t>(value)) {}
    json_value(const char* value)
        : kind_(kind::string), string_(value) {}
    json_value(std::string value)
        : kind_(kind::string), string_(std::move(value)) {}
    json_value(array_type value)
        : kind_(kind::array), array_(std::move(value)) {}
    json_value(object_type value)
        : kind_(kind::object), object_(std::move(value)) {}

    kind type() const { return kind_; }

    bool is_null() const { return kind_ == kind::null_value; }
    bool is_boolean() const { return kind_ == kind::boolean; }
    bool is_number() const { return kind_ == kind::number; }
    bool is_string() const { return kind_ == kind::string; }
    bool is_array() const { return kind_ == kind::array; }
    bool is_object() const { return kind_ == kind::object; }

    bool as_boolean() const {
        require(kind::boolean, "boolean");
        return boolean_;
    }

    int64_t as_number() const {
        require(kind::number, "number");
        return number_;
    }

    const std::string& as_string() const {
        require(kind::string, "string");
        return string_;
    }

    const array_type& as_array() const {
        require(kind::array, "array");
        return array_;
    }

    const object_type& as_object() const {
        require(kind::object, "object");
        return object_;
    }

    array_type& as_array() {
        require(kind::array, "array");
        return array_;
    }

    object_type& as_object() {
        require(kind::object, "object");
        return object_;
    }

    const json_value* find(const std::string& key) const {
        if (!is_object()) {
            return nullptr;
        }
        const auto it = object_.find(key);
        return it == object_.end() ? nullptr : &it->second;
    }

    const json_value& at(const std::string& key) const {
        const auto* value = find(key);
        if (value == nullptr) {
            throw std::runtime_error("missing object field: " + key);
        }
        return *value;
    }

    static json_value parse(const std::string& text) {
        parser state(text);
        auto value = state.parse_value();
        state.skip_whitespace();
        if (!state.at_end()) {
            throw std::runtime_error("unexpected trailing JSON data");
        }
        return value;
    }

    std::string stringify() const {
        std::ostringstream out;
        write_to(out);
        return out.str();
    }

private:
    class parser {
    public:
        explicit parser(const std::string& input)
            : input_(input) {}

        json_value parse_value() {
            skip_whitespace();
            if (at_end()) {
                throw std::runtime_error("unexpected end of JSON input");
            }

            const char ch = input_[pos_];
            if (ch == 'n') {
                expect_literal("null");
                return json_value();
            }
            if (ch == 't') {
                expect_literal("true");
                return json_value(true);
            }
            if (ch == 'f') {
                expect_literal("false");
                return json_value(false);
            }
            if (ch == '"') {
                return json_value(parse_string());
            }
            if (ch == '[') {
                return json_value(parse_array());
            }
            if (ch == '{') {
                return json_value(parse_object());
            }
            if (ch == '-' || std::isdigit(static_cast<unsigned char>(ch))) {
                return json_value(parse_number());
            }

            throw std::runtime_error("invalid JSON value");
        }

        void skip_whitespace() {
            while (!at_end() &&
                   std::isspace(static_cast<unsigned char>(input_[pos_]))) {
                ++pos_;
            }
        }

        bool at_end() const {
            return pos_ >= input_.size();
        }

    private:
        std::string parse_string() {
            expect('"');
            std::string result;
            while (!at_end()) {
                const char ch = input_[pos_++];
                if (ch == '"') {
                    return result;
                }
                if (ch != '\\') {
                    result.push_back(ch);
                    continue;
                }
                if (at_end()) {
                    throw std::runtime_error("unterminated JSON escape");
                }
                const char esc = input_[pos_++];
                switch (esc) {
                case '"': result.push_back('"'); break;
                case '\\': result.push_back('\\'); break;
                case '/': result.push_back('/'); break;
                case 'b': result.push_back('\b'); break;
                case 'f': result.push_back('\f'); break;
                case 'n': result.push_back('\n'); break;
                case 'r': result.push_back('\r'); break;
                case 't': result.push_back('\t'); break;
                case 'u':
                    throw std::runtime_error("unicode escapes are not supported");
                default:
                    throw std::runtime_error("invalid JSON escape");
                }
            }
            throw std::runtime_error("unterminated JSON string");
        }

        array_type parse_array() {
            expect('[');
            array_type values;
            skip_whitespace();
            if (consume(']')) {
                return values;
            }
            while (true) {
                values.push_back(parse_value());
                skip_whitespace();
                if (consume(']')) {
                    return values;
                }
                expect(',');
            }
        }

        object_type parse_object() {
            expect('{');
            object_type values;
            skip_whitespace();
            if (consume('}')) {
                return values;
            }
            while (true) {
                skip_whitespace();
                if (at_end() || input_[pos_] != '"') {
                    throw std::runtime_error("expected JSON object key");
                }
                const auto key = parse_string();
                skip_whitespace();
                expect(':');
                values.emplace(key, parse_value());
                skip_whitespace();
                if (consume('}')) {
                    return values;
                }
                expect(',');
            }
        }

        int64_t parse_number() {
            const std::size_t start = pos_;
            if (input_[pos_] == '-') {
                ++pos_;
            }
            if (at_end() || !std::isdigit(static_cast<unsigned char>(input_[pos_]))) {
                throw std::runtime_error("invalid JSON number");
            }
            while (!at_end() &&
                   std::isdigit(static_cast<unsigned char>(input_[pos_]))) {
                ++pos_;
            }
            if (!at_end() && (input_[pos_] == '.' || input_[pos_] == 'e' || input_[pos_] == 'E')) {
                throw std::runtime_error("floating-point JSON numbers are not supported");
            }
            return std::stoll(input_.substr(start, pos_ - start));
        }

        void expect(char ch) {
            skip_whitespace();
            if (at_end() || input_[pos_] != ch) {
                throw std::runtime_error(std::string("expected '") + ch + "'");
            }
            ++pos_;
        }

        bool consume(char ch) {
            skip_whitespace();
            if (at_end() || input_[pos_] != ch) {
                return false;
            }
            ++pos_;
            return true;
        }

        void expect_literal(const char* literal) {
            while (*literal != '\0') {
                if (at_end() || input_[pos_] != *literal) {
                    throw std::runtime_error("invalid JSON literal");
                }
                ++pos_;
                ++literal;
            }
        }

        const std::string& input_;
        std::size_t pos_ = 0;
    };

    void require(kind expected, const char* name) const {
        if (kind_ != expected) {
            throw std::runtime_error(std::string("JSON value is not a ") + name);
        }
    }

    void write_to(std::ostream& out) const {
        switch (kind_) {
        case kind::null_value:
            out << "null";
            break;
        case kind::boolean:
            out << (boolean_ ? "true" : "false");
            break;
        case kind::number:
            out << number_;
            break;
        case kind::string:
            out << "\"" << escape_string(string_) << "\"";
            break;
        case kind::array:
            out << "[";
            for (std::size_t i = 0; i < array_.size(); ++i) {
                if (i != 0) {
                    out << ",";
                }
                array_[i].write_to(out);
            }
            out << "]";
            break;
        case kind::object:
            out << "{";
            for (auto it = object_.begin(); it != object_.end(); ++it) {
                if (it != object_.begin()) {
                    out << ",";
                }
                out << "\"" << escape_string(it->first) << "\":";
                it->second.write_to(out);
            }
            out << "}";
            break;
        }
    }

    static std::string escape_string(const std::string& value) {
        std::string result;
        for (char ch : value) {
            switch (ch) {
            case '\\': result += "\\\\"; break;
            case '"': result += "\\\""; break;
            case '\b': result += "\\b"; break;
            case '\f': result += "\\f"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default: result.push_back(ch); break;
            }
        }
        return result;
    }

    kind kind_ = kind::null_value;
    bool boolean_ = false;
    int64_t number_ = 0;
    std::string string_;
    array_type array_;
    object_type object_;
};

inline std::optional<std::string> json_get_string(
    const json_value& object, const std::string& key)
{
    if (const auto* value = object.find(key); value != nullptr && value->is_string()) {
        return value->as_string();
    }
    return std::nullopt;
}

inline std::optional<int64_t> json_get_number(
    const json_value& object, const std::string& key)
{
    if (const auto* value = object.find(key); value != nullptr && value->is_number()) {
        return value->as_number();
    }
    return std::nullopt;
}

inline std::optional<bool> json_get_boolean(
    const json_value& object, const std::string& key)
{
    if (const auto* value = object.find(key); value != nullptr && value->is_boolean()) {
        return value->as_boolean();
    }
    return std::nullopt;
}

#endif
