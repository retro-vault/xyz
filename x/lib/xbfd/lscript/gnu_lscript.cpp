//
// gnu_lscript.cpp — parse a useful GNU ld linker-script subset.
//
// Supported constructs:
//   ENTRY(symbol)
//   OUTPUT_FORMAT(binary|xl|elf|ihx)
//   RESERVE(lo-hi) / RESERVE(lo, hi)
//   BINARY_RANGE(lo-hi) / BINARY_RANGE(lo, hi)
//   MEMORY { ROM (rx) : ORIGIN = 0x0100, LENGTH = 0x1000 }
//   SECTIONS { .text : { *(.text .text.*) *(.rodata .rodata.*) } > ROM }
//   ASSERT(expr, "message")
//   symbol assignments like _end = .;
//   section load attributes like AT(0x1234) / AT>ROM
//   /DISCARD/ output sections
//
// This is intentionally not a full GNU ld parser. The goal is to accept
// practical real-world scripts and preserve the placement semantics xld
// actually needs: entry, output format, memory regions, reserved ranges,
// output section ordering, and section-to-region anchoring.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//
#include <xbfd/lscript.h>

#include <cctype>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace xbfd {
namespace {

struct gnu_token {
    enum class kind { ident, number, string_lit, symbol, end };
    kind type = kind::end;
    std::string text;
    int line = 1;
};

class gnu_lexer {
public:
    explicit gnu_lexer(std::string source) : source_(std::move(source)) {}

    std::vector<gnu_token> lex() {
        std::vector<gnu_token> out;
        while (true) {
            skip_ignored();
            if (pos_ >= source_.size())
                break;

            const char ch = source_[pos_];
            if (ch == '"') {
                out.push_back(read_string());
                continue;
            }
            if (is_ident_start(ch)) {
                out.push_back(read_ident());
                continue;
            }
            if (std::isdigit(static_cast<unsigned char>(ch))) {
                out.push_back(read_number());
                continue;
            }

            gnu_token tok;
            tok.type = gnu_token::kind::symbol;
            tok.text.assign(1, ch);
            tok.line = line_;
            ++pos_;
            out.push_back(std::move(tok));
        }

        gnu_token end;
        end.type = gnu_token::kind::end;
        end.line = line_;
        out.push_back(std::move(end));
        return out;
    }

private:
    static bool is_ident_start(char ch) {
        return std::isalpha(static_cast<unsigned char>(ch))
            || ch == '_' || ch == '.' || ch == '$'
            || ch == '*' || ch == '?';
    }

    static bool is_ident_continue(char ch) {
        return std::isalnum(static_cast<unsigned char>(ch))
            || ch == '_' || ch == '.' || ch == '$'
            || ch == '*' || ch == '?';
    }

    void skip_ignored() {
        while (pos_ < source_.size()) {
            if (std::isspace(static_cast<unsigned char>(source_[pos_]))) {
                if (source_[pos_] == '\n')
                    ++line_;
                ++pos_;
                continue;
            }
            if (source_[pos_] == '#') {
                while (pos_ < source_.size() && source_[pos_] != '\n')
                    ++pos_;
                continue;
            }
            if (source_[pos_] == '/' && pos_ + 1 < source_.size()) {
                if (source_[pos_ + 1] == '/') {
                    pos_ += 2;
                    while (pos_ < source_.size() && source_[pos_] != '\n')
                        ++pos_;
                    continue;
                }
                if (source_[pos_ + 1] == '*') {
                    pos_ += 2;
                    while (pos_ + 1 < source_.size()
                           && !(source_[pos_] == '*' && source_[pos_ + 1] == '/')) {
                        if (source_[pos_] == '\n')
                            ++line_;
                        ++pos_;
                    }
                    if (pos_ + 1 < source_.size())
                        pos_ += 2;
                    continue;
                }
            }
            break;
        }
    }

    gnu_token read_ident() {
        gnu_token tok;
        tok.type = gnu_token::kind::ident;
        tok.line = line_;
        const std::size_t start = pos_;
        while (pos_ < source_.size() && is_ident_continue(source_[pos_]))
            ++pos_;
        tok.text = source_.substr(start, pos_ - start);
        return tok;
    }

    gnu_token read_number() {
        gnu_token tok;
        tok.type = gnu_token::kind::number;
        tok.line = line_;
        const std::size_t start = pos_;
        while (pos_ < source_.size()) {
            const char ch = source_[pos_];
            if (std::isalnum(static_cast<unsigned char>(ch))
                || ch == 'x' || ch == 'X')
                ++pos_;
            else
                break;
        }
        tok.text = source_.substr(start, pos_ - start);
        return tok;
    }

    gnu_token read_string() {
        gnu_token tok;
        tok.type = gnu_token::kind::string_lit;
        tok.line = line_;
        ++pos_; // opening quote
        const std::size_t start = pos_;
        while (pos_ < source_.size() && source_[pos_] != '"') {
            if (source_[pos_] == '\\' && pos_ + 1 < source_.size())
                pos_ += 2;
            else {
                if (source_[pos_] == '\n')
                    ++line_;
                ++pos_;
            }
        }
        tok.text = source_.substr(start, pos_ - start);
        if (pos_ < source_.size() && source_[pos_] == '"')
            ++pos_;
        return tok;
    }

    std::string source_;
    std::size_t pos_ = 0;
    int line_ = 1;
};

static std::string read_text_file(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input.is_open())
        throw lscript_error("cannot open linker script '" + path.string() + "'");
    std::ostringstream text;
    text << input.rdbuf();
    return text.str();
}

static uint16_t parse_u16_gnu(const std::string& text,
                              const std::filesystem::path& path,
                              int line)
{
    try {
        unsigned long value = 0;
        if (text.size() > 2 && text[0] == '0'
            && (text[1] == 'x' || text[1] == 'X')) {
            value = std::stoul(text, nullptr, 16);
        } else {
            value = std::stoul(text, nullptr, 10);
        }
        if (value > 0xFFFFu)
            throw lscript_error(path.string() + ":" + std::to_string(line)
                                + ": address out of 16-bit range");
        return static_cast<uint16_t>(value);
    } catch (const std::exception&) {
        throw lscript_error(path.string() + ":" + std::to_string(line)
                            + ": invalid number '" + text + "'");
    }
}

static lscript_output_format parse_output_format_name(
    const std::string& text,
    const std::filesystem::path& path,
    int line)
{
    if (text == "binary" || text == "bin")
        return lscript_output_format::bin;
    if (text == "xl")
        return lscript_output_format::xl;
    if (text == "elf")
        return lscript_output_format::elf;
    if (text == "ihx")
        return lscript_output_format::ihx;
    throw lscript_error(path.string() + ":" + std::to_string(line)
                        + ": unsupported OUTPUT_FORMAT '" + text + "'");
}

class gnu_parser {
public:
    gnu_parser(const std::filesystem::path& path,
               std::vector<gnu_token> tokens,
               gnu_lscript& out)
        : path_(path), tokens_(std::move(tokens)), out_(out) {}

    void parse() {
        while (!is_end()) {
            if (match_ident("ENTRY")) {
                parse_entry();
            } else if (match_ident("OUTPUT_FORMAT")) {
                parse_output_format();
            } else if (match_ident("RESERVE")) {
                auto range = parse_range_call("RESERVE");
                out_.add_reserved_range(range);
            } else if (match_ident("BINARY_RANGE")) {
                out_.set_output_range(parse_range_call("BINARY_RANGE"));
            } else if (match_ident("MEMORY")) {
                parse_memory();
            } else if (match_ident("SECTIONS")) {
                parse_sections();
            } else if (match_ident("ASSERT")) {
                skip_parenthesized_block("ASSERT");
                match_symbol(';');
            } else if (is_assignment_start()) {
                skip_assignment();
            } else {
                advance();
            }
        }
    }

private:
    bool is_end() const { return peek().type == gnu_token::kind::end; }

    const gnu_token& peek(std::size_t lookahead = 0) const {
        const std::size_t idx = std::min(pos_ + lookahead, tokens_.size() - 1);
        return tokens_[idx];
    }

    gnu_token advance() {
        if (!is_end())
            return tokens_[pos_++];
        return tokens_.back();
    }

    bool is_symbol(char ch, std::size_t lookahead = 0) const {
        return peek(lookahead).type == gnu_token::kind::symbol
            && peek(lookahead).text == std::string(1, ch);
    }

    bool match_symbol(char ch) {
        if (is_symbol(ch)) {
            advance();
            return true;
        }
        return false;
    }

    bool is_ident_token(std::size_t lookahead = 0) const {
        return peek(lookahead).type == gnu_token::kind::ident;
    }

    bool match_ident(const char* text) {
        if (is_ident_token() && peek().text == text) {
            advance();
            return true;
        }
        return false;
    }

    void expect_symbol(char ch, const char* context) {
        if (!match_symbol(ch)) {
            throw lscript_error(path_.string() + ":" + std::to_string(peek().line)
                                + ": expected '" + std::string(1, ch)
                                + "' after " + context);
        }
    }

    std::string expect_ident(const char* context) {
        if (!is_ident_token()) {
            throw lscript_error(path_.string() + ":" + std::to_string(peek().line)
                                + ": expected identifier for " + context);
        }
        return advance().text;
    }

    uint16_t expect_number(const char* context) {
        if (peek().type != gnu_token::kind::number) {
            throw lscript_error(path_.string() + ":" + std::to_string(peek().line)
                                + ": expected number for " + context);
        }
        auto tok = advance();
        return parse_u16_gnu(tok.text, path_, tok.line);
    }

    void parse_entry() {
        expect_symbol('(', "ENTRY");
        out_.set_entry_symbol(expect_ident("ENTRY"));
        expect_symbol(')', "ENTRY");
        match_symbol(';');
    }

    void parse_output_format() {
        expect_symbol('(', "OUTPUT_FORMAT");
        if (peek().type != gnu_token::kind::ident
            && peek().type != gnu_token::kind::string_lit) {
            throw lscript_error(path_.string() + ":" + std::to_string(peek().line)
                                + ": expected format name for OUTPUT_FORMAT");
        }
        auto tok = advance();
        out_.set_output_format(parse_output_format_name(tok.text, path_, tok.line));
        while (!is_end() && !match_symbol(')'))
            advance();
        match_symbol(';');
    }

    lscript_address_range parse_range_call(const char* name) {
        expect_symbol('(', name);
        const auto start = parse_range_bound();
        if (!start.has_value()) {
            throw lscript_error(path_.string() + ":" + std::to_string(peek().line)
                                + ": expected start address in " + std::string(name));
        }
        if (!(match_symbol('-') || match_symbol(','))) {
            throw lscript_error(path_.string() + ":" + std::to_string(peek().line)
                                + ": expected '-' or ',' in " + std::string(name));
        }
        const auto end = parse_range_bound();
        if (!end.has_value()) {
            throw lscript_error(path_.string() + ":" + std::to_string(peek().line)
                                + ": expected end address in " + std::string(name));
        }
        expect_symbol(')', name);
        match_symbol(';');
        return {*start, *end};
    }

    void parse_memory() {
        expect_symbol('{', "MEMORY");
        while (!is_end() && !match_symbol('}')) {
            if (!is_ident_token()) {
                advance();
                continue;
            }

            const std::string region_name = advance().text;
            if (match_symbol('(')) {
                int depth = 1;
                while (!is_end() && depth > 0) {
                    if (match_symbol('(')) ++depth;
                    else if (match_symbol(')')) --depth;
                    else advance();
                }
            }
            expect_symbol(':', "MEMORY region");

            std::optional<uint16_t> origin;
            std::optional<uint16_t> length;

            if (is_ident_token() && peek().text == "ORIGIN") {
                while (!is_end()) {
                    if (match_ident("ORIGIN")) {
                        expect_symbol('=', "ORIGIN");
                        origin = parse_scalar_expr();
                        continue;
                    }
                    if (match_ident("LENGTH")) {
                        expect_symbol('=', "LENGTH");
                        length = parse_scalar_expr();
                        continue;
                    }
                    if (match_symbol(',') || match_symbol(';'))
                        continue;
                    break;
                }
            } else {
                origin = parse_scalar_expr();
                if (match_symbol(','))
                    length = parse_scalar_expr();
            }

            if (origin.has_value())
                memory_origins_[region_name] = *origin;
            if (length.has_value())
                memory_lengths_[region_name] = *length;
        }
    }

    void parse_sections() {
        expect_symbol('{', "SECTIONS");
        std::optional<std::string> current_region;

        while (!is_end() && !match_symbol('}')) {
            if (match_ident("ASSERT")) {
                skip_parenthesized_block("ASSERT");
                match_symbol(';');
                continue;
            }
            if (is_assignment_start()) {
                skip_assignment();
                continue;
            }
            if (!looks_like_output_section()) {
                advance();
                continue;
            }

            const std::string output_name = parse_output_section_name();
            std::optional<uint16_t> explicit_base;
            std::optional<std::string> region_name;

            if (!is_symbol(':'))
                explicit_base = parse_scalar_expr();

            if (!match_symbol(':')) {
                skip_statement();
                continue;
            }

            if (match_ident("AT")) {
                if (match_symbol('('))
                    skip_parenthesized_content();
                else if (match_symbol('>'))
                    (void)expect_ident("AT load region");
            }

            if (match_symbol('('))
                skip_parenthesized_content();

            if (!match_symbol('{')) {
                skip_statement();
                continue;
            }

            auto input_areas = parse_section_block();

            bool scanning_attrs = true;
            while (scanning_attrs && !is_end()) {
                if (match_symbol('>')) {
                    region_name = expect_ident("memory region");
                } else if (match_ident("AT")) {
                    if (match_symbol('>'))
                        (void)expect_ident("AT load region");
                    else if (match_symbol('('))
                        skip_parenthesized_content();
                } else if (match_symbol(':')) {
                    if (is_ident_token())
                        advance();
                } else {
                    scanning_attrs = false;
                }
            }

            if (output_name != "/DISCARD/") {
                if (input_areas.empty() && output_name != ".")
                    input_areas.push_back(output_name);

                for (const auto& area_name : input_areas)
                    out_.add_area_order(area_name);

                if (!input_areas.empty()) {
                    const auto& anchor_area = input_areas.front();
                    if (explicit_base.has_value()) {
                        out_.set_area_base(anchor_area, *explicit_base);
                    } else if (region_name.has_value()) {
                        auto region_it = memory_origins_.find(*region_name);
                        if (region_it != memory_origins_.end()
                            && (!current_region.has_value()
                                || *current_region != *region_name)) {
                            out_.set_area_base(anchor_area, region_it->second);
                        }
                        current_region = region_name;
                    }
                }
            }

            match_symbol(';');
        }
    }

    bool looks_like_output_section() const {
        return is_ident_token()
            || (is_symbol('/') && is_ident_token(1) && peek(1).text == "DISCARD"
                && is_symbol('/', 2));
    }

    std::string parse_output_section_name() {
        if (is_symbol('/') && is_ident_token(1) && peek(1).text == "DISCARD"
            && is_symbol('/', 2)) {
            advance();
            advance();
            advance();
            return "/DISCARD/";
        }
        return expect_ident("output section");
    }

    bool is_assignment_start() const {
        if (!is_ident_token())
            return false;
        if (is_symbol('=', 1))
            return true;
        return (is_symbol('+', 1) || is_symbol('-', 1)
                || is_symbol('*', 1) || is_symbol('/', 1)
                || is_symbol('%', 1) || is_symbol('&', 1)
                || is_symbol('|', 1) || is_symbol('^', 1))
            && is_symbol('=', 2);
    }

    void skip_assignment() {
        while (!is_end()) {
            if (match_symbol(';'))
                return;
            if (is_symbol('}'))
                return;
            if (match_symbol('('))
                skip_parenthesized_content();
            else
                advance();
        }
    }

    void skip_statement() {
        int brace_depth = 0;
        while (!is_end()) {
            if (match_symbol('{')) {
                ++brace_depth;
                continue;
            }
            if (match_symbol('}')) {
                if (brace_depth == 0) {
                    if (pos_ > 0)
                        --pos_;
                    return;
                }
                --brace_depth;
                continue;
            }
            if (brace_depth == 0 && match_symbol(';'))
                return;
            if (match_symbol('('))
                skip_parenthesized_content();
            else
                advance();
        }
    }

    void skip_parenthesized_block(const char* context) {
        expect_symbol('(', context);
        skip_parenthesized_content();
    }

    void skip_parenthesized_content() {
        int depth = 1;
        while (!is_end() && depth > 0) {
            if (match_symbol('(')) {
                ++depth;
            } else if (match_symbol(')')) {
                --depth;
            } else {
                advance();
            }
        }
    }

    std::vector<std::string> parse_section_block() {
        std::vector<std::string> areas;
        int depth = 1;

        while (!is_end() && depth > 0) {
            if (match_symbol('{')) {
                ++depth;
                continue;
            }
            if (match_symbol('}')) {
                --depth;
                continue;
            }

            if (depth == 1) {
                if (match_ident("ASSERT")) {
                    skip_parenthesized_block("ASSERT");
                    match_symbol(';');
                    continue;
                }
                if (is_assignment_start()) {
                    skip_assignment();
                    continue;
                }
                if (is_ident_token() && is_symbol('(', 1)) {
                    const std::string opener = advance().text;
                    if (opener == "KEEP"
                        || opener.rfind("SORT_", 0) == 0
                        || opener == "EXCLUDE_FILE"
                        || opener == "BYTE"
                        || opener == "SHORT"
                        || opener == "LONG"
                        || opener == "QUAD"
                        || opener == "FILL") {
                        parse_input_section_pattern_list(areas);
                        continue;
                    }
                    parse_input_section_pattern_list(areas);
                    continue;
                }
            }

            advance();
        }

        return areas;
    }

    void parse_input_section_pattern_list(std::vector<std::string>& areas) {
        expect_symbol('(', "section pattern list");
        int depth = 1;
        while (!is_end() && depth > 0) {
            if (match_symbol('(')) {
                ++depth;
                continue;
            }
            if (match_symbol(')')) {
                --depth;
                continue;
            }

            if (depth == 1) {
                if (match_ident("EXCLUDE_FILE")) {
                    skip_parenthesized_block("EXCLUDE_FILE");
                    continue;
                }
                if (is_ident_token() && is_symbol('(', 1)) {
                    const std::string nested = advance().text;
                    if (nested == "KEEP"
                        || nested.rfind("SORT_", 0) == 0
                        || nested == "EXCLUDE_FILE"
                        || nested == "*"
                        || nested.find('.') != std::string::npos) {
                        parse_input_section_pattern_list(areas);
                        continue;
                    }
                    parse_input_section_pattern_list(areas);
                    continue;
                }
                if (is_ident_token()) {
                    const auto area = canonical_input_area_name(advance().text);
                    if (area.has_value())
                        push_unique(areas, *area);
                    continue;
                }
            }

            advance();
        }
    }

    static void push_unique(std::vector<std::string>& areas,
                            const std::string& area_name)
    {
        for (const auto& existing : areas) {
            if (existing == area_name)
                return;
        }
        areas.push_back(area_name);
    }

    static std::optional<std::string> canonical_input_area_name(
        const std::string& pattern)
    {
        if (pattern.empty() || pattern == "COMMON")
            return std::nullopt;
        if (pattern[0] != '.')
            return std::nullopt;

        auto end = pattern.find_first_of("*?");
        std::string base = pattern.substr(0, end);
        while (!base.empty() && base.back() == '.')
            base.pop_back();
        if (base.empty() || base == ".")
            return std::nullopt;
        return base;
    }

    std::optional<uint16_t> parse_scalar_expr() {
        auto value = parse_additive_expr();
        if (!value.has_value())
            return std::nullopt;
        return static_cast<uint16_t>(*value & 0xFFFF);
    }

    std::optional<uint16_t> parse_range_bound() {
        auto value = parse_primary_expr();
        if (!value.has_value())
            return std::nullopt;
        return static_cast<uint16_t>(*value & 0xFFFF);
    }

    std::optional<int32_t> parse_additive_expr() {
        auto lhs = parse_unary_expr();
        if (!lhs.has_value())
            return std::nullopt;

        while (!is_end()) {
            if (match_symbol('+')) {
                auto rhs = parse_unary_expr();
                if (!rhs.has_value())
                    return std::nullopt;
                *lhs += *rhs;
            } else if (match_symbol('-')) {
                auto rhs = parse_unary_expr();
                if (!rhs.has_value())
                    return std::nullopt;
                *lhs -= *rhs;
            } else {
                break;
            }
        }

        return lhs;
    }

    std::optional<int32_t> parse_unary_expr() {
        if (match_symbol('+'))
            return parse_unary_expr();
        if (match_symbol('-')) {
            auto value = parse_unary_expr();
            if (!value.has_value())
                return std::nullopt;
            return -*value;
        }
        return parse_primary_expr();
    }

    std::optional<int32_t> parse_primary_expr() {
        if (peek().type == gnu_token::kind::number) {
            auto tok = advance();
            return static_cast<int32_t>(parse_u16_gnu(tok.text, path_, tok.line));
        }

        if (match_symbol('(')) {
            auto value = parse_additive_expr();
            expect_symbol(')', "expression");
            return value;
        }

        if (!is_ident_token())
            return std::nullopt;

        const auto ident = advance().text;
        if (ident == "ORIGIN") {
            expect_symbol('(', "ORIGIN");
            const auto name = expect_ident("ORIGIN");
            expect_symbol(')', "ORIGIN");
            auto it = memory_origins_.find(name);
            if (it == memory_origins_.end())
                return std::nullopt;
            return static_cast<int32_t>(it->second);
        }
        if (ident == "LENGTH") {
            expect_symbol('(', "LENGTH");
            const auto name = expect_ident("LENGTH");
            expect_symbol(')', "LENGTH");
            auto it = memory_lengths_.find(name);
            if (it == memory_lengths_.end())
                return std::nullopt;
            return static_cast<int32_t>(it->second);
        }
        if (ident == "ALIGN") {
            expect_symbol('(', "ALIGN");
            auto value = parse_additive_expr();
            expect_symbol(')', "ALIGN");
            return value;
        }

        return std::nullopt;
    }

    std::filesystem::path path_;
    std::vector<gnu_token> tokens_;
    std::size_t pos_ = 0;
    std::unordered_map<std::string, uint16_t> memory_origins_;
    std::unordered_map<std::string, uint16_t> memory_lengths_;
    gnu_lscript& out_;
};

} // namespace

std::unique_ptr<gnu_lscript> gnu_lscript::read(
    const std::filesystem::path& path)
{
    auto script = std::make_unique<gnu_lscript>();
    gnu_lexer lexer(read_text_file(path));
    gnu_parser parser(path, lexer.lex(), *script);
    parser.parse();
    return script;
}

} // namespace xbfd
