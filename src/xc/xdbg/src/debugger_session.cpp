#include "debugger_session.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <memory>
#include <sstream>
#include <stdexcept>

namespace {

    class remote_memory_reader final : public xdbg::memory_reader {
    public:
        explicit remote_memory_reader(xdbgstub::client& remote)
            : remote_(remote) {}

        uint8_t read8(uint32_t address) const override {
            const auto data = remote_.read_memory(address, 1);
            return data.empty() ? 0x00 : data[0];
        }

    private:
        xdbgstub::client& remote_;
    };

    uint32_t parse_u32(const std::string& value) {
        char* end = nullptr;
        const unsigned long parsed = std::strtoul(value.c_str(), &end, 0);
        if (end == value.c_str() || *end != '\0') {
            throw std::runtime_error("invalid number: " + value);
        }
        return static_cast<uint32_t>(parsed);
    }

    std::filesystem::path default_symbols_path(
        const std::filesystem::path& exec_path)
    {
        auto replaced = exec_path;
        replaced.replace_extension(".xdbg");
        if (std::filesystem::exists(replaced)) {
            return replaced;
        }

        auto appended = exec_path;
        appended += ".xdbg";
        return appended;
    }

    std::pair<std::string, uint16_t> split_host_port(const std::string& value) {
        const auto colon = value.rfind(':');
        if (colon == std::string::npos) {
            throw std::runtime_error("expected host:port");
        }
        return {
            value.substr(0, colon),
            static_cast<uint16_t>(parse_u32(value.substr(colon + 1)))
        };
    }

} // namespace

void debugger_session::set_exec_path(const std::filesystem::path& path) {
    exec_path_ = path;
}

const std::optional<std::filesystem::path>& debugger_session::exec_path() const {
    return exec_path_;
}

void debugger_session::load_symbols_file(const std::filesystem::path& path) {
    symbols_ = xdbg::read_file(path);
    symbols_path_ = path;
}

void debugger_session::maybe_load_default_symbols() {
    if (!exec_path_.has_value()) {
        return;
    }

    const auto guess = default_symbols_path(exec_path_.value());
    if (std::filesystem::exists(guess)) {
        load_symbols_file(guess);
    }
}

const std::optional<std::filesystem::path>& debugger_session::symbols_path() const {
    return symbols_path_;
}

bool debugger_session::has_symbols() const {
    return symbols_.has_value();
}

const xdbg::document* debugger_session::symbols() const {
    return symbols_.has_value() ? &symbols_.value() : nullptr;
}

void debugger_session::connect_remote(const std::string& target) {
    const auto [host, port] = split_host_port(target);
    remote_.connect(host, port);
}

bool debugger_session::is_connected() const {
    return remote_.is_connected();
}

breakpoint_entry debugger_session::add_breakpoint(const std::string& expression) {
    const uint32_t address = resolve_address_expression(expression);
    remote_.set_breakpoint(address);
    breakpoints_.push_back({next_breakpoint_id_++, address, expression});
    return breakpoints_.back();
}

void debugger_session::delete_breakpoint(int id) {
    auto it = std::find_if(
        breakpoints_.begin(),
        breakpoints_.end(),
        [id](const breakpoint_entry& entry) { return entry.id == id; });
    if (it == breakpoints_.end()) {
        throw std::runtime_error("breakpoint id not found");
    }
    remote_.clear_breakpoint(it->address);
    breakpoints_.erase(it);
}

void debugger_session::delete_all_breakpoints() {
    for (const auto& breakpoint : breakpoints_) {
        remote_.clear_breakpoint(breakpoint.address);
    }
    breakpoints_.clear();
}

const std::vector<breakpoint_entry>& debugger_session::breakpoints() const {
    return breakpoints_;
}

xdbgstub::cpu_state debugger_session::read_registers() {
    return remote_.read_registers();
}

void debugger_session::write_registers(const xdbgstub::cpu_state& state) {
    remote_.write_registers(state);
}

xdbgstub::target_status debugger_session::raw_status() {
    return remote_.status();
}

stop_snapshot debugger_session::status() {
    return make_stop_snapshot(remote_.status());
}

stop_snapshot debugger_session::continue_execution() {
    return make_stop_snapshot(remote_.continue_execution());
}

stop_snapshot debugger_session::step_instruction() {
    return make_stop_snapshot(remote_.step_instruction());
}

stop_snapshot debugger_session::pause_execution() {
    return make_stop_snapshot(remote_.pause_execution());
}

void debugger_session::detach() {
    remote_.detach();
    remote_.close();
}

uint32_t debugger_session::resolve_address_expression(
    const std::string& expression)
{
    if (expression.empty()) {
        throw std::runtime_error("missing address expression");
    }

    if (expression[0] == '*') {
        return parse_u32(expression.substr(1));
    }

    if (std::isdigit(static_cast<unsigned char>(expression[0]))) {
        return parse_u32(expression);
    }

    if (expression.find(':') != std::string::npos) {
        const auto line_info = info_line_argument(expression);
        return line_info.location.address;
    }

    if (const auto* function = find_function_by_name(expression)) {
        return function->start_address;
    }

    if (const auto* symbol = find_symbol_by_name(expression)) {
        return symbol->address;
    }

    throw std::runtime_error("unknown symbol or address: " + expression);
}

std::optional<source_location> debugger_session::current_source_location() {
    return source_location_for_address(read_registers().pc);
}

std::optional<source_location> debugger_session::source_location_for_address(
    uint32_t address) const
{
    const auto* line = find_line_for_pc(address);
    if (line == nullptr) {
        return std::nullopt;
    }

    const auto* file = find_file(line->file_id);
    if (file == nullptr) {
        return std::nullopt;
    }

    source_location location;
    location.address = address;
    location.file_path = file->path;
    location.line = line->line;
    location.column = line->column;
    if (const auto* function = find_function_for_pc(address)) {
        location.function_name = function->name;
    }
    return location;
}

std::optional<source_location> debugger_session::source_location_for_function(
    const std::string& function_name) const
{
    const auto* function = find_function_by_name(function_name);
    if (function == nullptr) {
        return std::nullopt;
    }

    const auto* line = find_line_for_function(*function);
    if (line == nullptr) {
        return std::nullopt;
    }

    const auto* file = find_file(line->file_id);
    if (file == nullptr) {
        return std::nullopt;
    }

    source_location location;
    location.address = line->address;
    location.file_path = file->path;
    location.line = line->line;
    location.column = line->column;
    location.function_name = function->name;
    return location;
}

line_info_result debugger_session::info_line_current() {
    const auto registers = read_registers();
    return info_line_argument("*" + std::to_string(registers.pc));
}

line_info_result debugger_session::info_line_argument(const std::string& argument) {
    const xdbg::line_entry* line = nullptr;
    const xdbg::function* function = nullptr;
    const xdbg::source_file* file = nullptr;
    uint32_t address = 0;

    if (!argument.empty() && argument[0] == '*') {
        address = resolve_address_expression(argument);
        line = find_line_for_pc(address);
        function = find_function_for_pc(address);
    } else if (const auto* found_function = find_function_by_name(argument)) {
        function = found_function;
        line = find_line_for_function(*found_function);
        address = found_function->start_address;
    } else {
        const auto colon = argument.rfind(':');
        if (colon == std::string::npos) {
            throw std::runtime_error("cannot resolve info line argument");
        }

        const std::string file_name = argument.substr(0, colon);
        const auto line_number = parse_u32(argument.substr(colon + 1));

        if (!symbols_.has_value()) {
            throw std::runtime_error("no symbols loaded");
        }

        for (const auto& candidate : symbols_->files) {
            if (candidate.path == file_name ||
                std::filesystem::path(candidate.path).filename() == file_name) {
                file = &candidate;
                break;
            }
        }

        if (file == nullptr) {
            throw std::runtime_error("source file not found");
        }

        line = find_line_by_file_and_line(file->id, line_number);
        if (line != nullptr) {
            address = line->address;
            function = find_function_for_pc(address);
        }
    }

    if (line == nullptr) {
        throw std::runtime_error("no source line found");
    }

    if (file == nullptr) {
        file = find_file(line->file_id);
    }
    if (file == nullptr) {
        throw std::runtime_error("source file id not found");
    }

    line_info_result result;
    result.location.address = address == 0 ? line->address : address;
    result.location.file_path = file->path;
    result.location.line = line->line;
    result.location.column = line->column;
    if (function != nullptr) {
        result.location.function_name = function->name;
    }
    result.end_address = find_line_end_address(*line);
    return result;
}

std::vector<source_line_view> debugger_session::list_source(
    const std::optional<std::string>& argument,
    uint32_t context_before,
    uint32_t context_after)
{
    if (!symbols_.has_value()) {
        throw std::runtime_error("no symbols loaded");
    }

    std::filesystem::path path;
    uint32_t line_number = 0;

    if (!argument.has_value()) {
        const auto registers = read_registers();
        const auto* line = find_line_for_pc(registers.pc);
        if (line == nullptr) {
            throw std::runtime_error("no source line for current pc");
        }
        const auto* file = find_file(line->file_id);
        if (file == nullptr) {
            throw std::runtime_error("source file id not found");
        }
        path = file->path;
        line_number = line->line;
    } else if (const auto* function = find_function_by_name(argument.value())) {
        if (!function->file_id.has_value() || !function->line.has_value()) {
            throw std::runtime_error("function has no source location");
        }
        const auto* file = find_file(function->file_id.value());
        if (file == nullptr) {
            throw std::runtime_error("source file id not found");
        }
        path = file->path;
        line_number = function->line.value();
    } else {
        const auto colon = argument->rfind(':');
        if (colon == std::string::npos) {
            throw std::runtime_error("cannot resolve list argument");
        }
        path = argument->substr(0, colon);
        line_number = parse_u32(argument->substr(colon + 1));
    }

    return source_window(path, line_number, context_before, context_after);
}

std::vector<disassembly_line> debugger_session::disassemble(
    uint32_t address, std::size_t count)
{
    remote_memory_reader memory(remote_);
    auto disassembler = xdbg::make_z80_disassembler();
    auto formatter = xdbg::make_sdcc_z80_formatter();

    std::vector<disassembly_line> result;
    uint32_t pc = address;
    for (std::size_t i = 0; i < count; ++i) {
        const auto instruction = disassembler->disassemble_one(pc, memory);
        result.push_back({pc, instruction.bytes, formatter->format(instruction)});
        pc += static_cast<uint32_t>(instruction.bytes.size());
    }
    return result;
}

std::vector<uint8_t> debugger_session::read_memory(
    uint32_t address, std::size_t length)
{
    return remote_.read_memory(address, length);
}

std::vector<const xdbg::variable*> debugger_session::visible_variables(
    uint32_t pc) const
{
    std::vector<const xdbg::variable*> result;
    if (!symbols_.has_value()) {
        return result;
    }

    const xdbg::function* function = find_function_for_pc(pc);
    for (const auto& variable : symbols_->variables) {
        if (variable.start_address.has_value() &&
            variable.end_address.has_value() &&
            (pc < variable.start_address.value() ||
             pc >= variable.end_address.value())) {
            continue;
        }

        if (function != nullptr && variable.parent_name.has_value() &&
            variable.parent_name.value() != function->name) {
            continue;
        }
        result.push_back(&variable);
    }
    return result;
}

std::vector<std::pair<std::string, uint32_t>> debugger_session::register_values() {
    const auto regs = read_registers();
    return {
        {"af", regs.af},
        {"bc", regs.bc},
        {"de", regs.de},
        {"hl", regs.hl},
        {"ix", regs.ix},
        {"iy", regs.iy},
        {"sp", regs.sp},
        {"pc", regs.pc},
        {"i", regs.i},
        {"r", regs.r}
    };
}

stop_snapshot debugger_session::make_stop_snapshot(
    const xdbgstub::target_status& status)
{
    stop_snapshot snapshot;
    snapshot.reason = status.reason;
    snapshot.pc = status.pc;
    snapshot.source = source_location_for_address(status.pc);
    if (snapshot.source.has_value()) {
        snapshot.source_text = read_source_line(
            snapshot.source->file_path, snapshot.source->line);
    }
    return snapshot;
}

std::optional<std::string> debugger_session::read_source_line(
    const std::filesystem::path& path, uint32_t target_line) const
{
    std::ifstream input(path);
    if (!input.is_open()) {
        return std::nullopt;
    }

    std::string line;
    uint32_t current_line = 0;
    while (std::getline(input, line)) {
        ++current_line;
        if (current_line == target_line) {
            return line;
        }
    }
    return std::nullopt;
}

std::vector<source_line_view> debugger_session::source_window(
    const std::filesystem::path& path,
    uint32_t target_line,
    uint32_t context_before,
    uint32_t context_after) const
{
    std::ifstream input(path);
    if (!input.is_open()) {
        throw std::runtime_error("cannot open source file: " + path.string());
    }

    std::vector<source_line_view> result;
    const uint32_t start_line = target_line > context_before
        ? target_line - context_before
        : 1;
    const uint32_t end_line = target_line + context_after;

    std::string line;
    uint32_t current_line = 0;
    while (std::getline(input, line)) {
        ++current_line;
        if (current_line < start_line) {
            continue;
        }
        if (current_line > end_line) {
            break;
        }
        result.push_back({current_line, line, current_line == target_line});
    }
    return result;
}

const xdbg::source_file* debugger_session::find_file(uint32_t file_id) const {
    if (!symbols_.has_value()) {
        return nullptr;
    }
    for (const auto& file : symbols_->files) {
        if (file.id == file_id) {
            return &file;
        }
    }
    return nullptr;
}

const xdbg::function* debugger_session::find_function_by_name(
    const std::string& name) const
{
    if (!symbols_.has_value()) {
        return nullptr;
    }
    for (const auto& function : symbols_->functions) {
        if (function.name == name) {
            return &function;
        }
    }
    return nullptr;
}

const xdbg::symbol* debugger_session::find_symbol_by_name(
    const std::string& name) const
{
    if (!symbols_.has_value()) {
        return nullptr;
    }
    for (const auto& symbol : symbols_->symbols) {
        if (symbol.name == name) {
            return &symbol;
        }
    }
    return nullptr;
}

const xdbg::function* debugger_session::find_function_for_pc(uint32_t pc) const {
    if (!symbols_.has_value()) {
        return nullptr;
    }
    for (const auto& function : symbols_->functions) {
        if (pc >= function.start_address && pc < function.end_address) {
            return &function;
        }
    }
    return nullptr;
}

const xdbg::line_entry* debugger_session::find_line_for_pc(uint32_t pc) const {
    if (!symbols_.has_value()) {
        return nullptr;
    }

    const xdbg::line_entry* best = nullptr;
    for (const auto& line : symbols_->lines) {
        if (line.address <= pc) {
            if (best == nullptr || line.address > best->address) {
                best = &line;
            }
        }
    }
    return best;
}

const xdbg::line_entry* debugger_session::find_line_for_function(
    const xdbg::function& function) const
{
    if (!symbols_.has_value()) {
        return nullptr;
    }

    if (function.file_id.has_value() && function.line.has_value()) {
        for (const auto& line : symbols_->lines) {
            if (line.file_id == function.file_id.value() &&
                line.line == function.line.value()) {
                return &line;
            }
        }
    }
    return find_line_for_pc(function.start_address);
}

const xdbg::line_entry* debugger_session::find_line_by_file_and_line(
    uint32_t file_id, uint32_t line_number) const
{
    if (!symbols_.has_value()) {
        return nullptr;
    }

    const xdbg::line_entry* best = nullptr;
    for (const auto& line : symbols_->lines) {
        if (line.file_id != file_id || line.line < line_number) {
            continue;
        }

        if (best == nullptr ||
            line.line < best->line ||
            (line.line == best->line && line.address < best->address)) {
            best = &line;
        }
    }
    return best;
}

std::optional<uint32_t> debugger_session::find_line_end_address(
    const xdbg::line_entry& target) const
{
    if (!symbols_.has_value()) {
        return std::nullopt;
    }

    uint32_t best = 0;
    bool found = false;
    for (const auto& line : symbols_->lines) {
        if (line.address > target.address) {
            if (!found || line.address < best) {
                best = line.address;
                found = true;
            }
        }
    }

    if (found) {
        return best;
    }

    if (const auto* function = find_function_for_pc(target.address)) {
        return function->end_address;
    }
    return std::nullopt;
}
