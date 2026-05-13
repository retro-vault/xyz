/*
 * Declares the abstract debugger host interface that protocol
 * frontends use to drive execution, inspect state, and expose
 * CLI, MI, or DAP services.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */

#ifndef XDBG_DEBUGGER_HOST_HPP
#define XDBG_DEBUGGER_HOST_HPP

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <xdbg/model.hpp>
#include <xdbgstub/xdbgstub.hpp>

struct breakpoint_entry {
    int id = 0;
    uint32_t address = 0;
    std::string expression;
};

struct source_location {
    uint32_t address = 0;
    std::string file_path;
    uint32_t line = 0;
    uint32_t column = 0;
    std::optional<std::string> function_name;
};

struct line_info_result {
    source_location location;
    std::optional<uint32_t> end_address;
};

struct source_line_view {
    uint32_t line = 0;
    std::string text;
    bool is_current = false;
};

struct disassembly_line {
    uint32_t address = 0;
    std::vector<uint8_t> bytes;
    std::string text;
};

struct stop_snapshot {
    xdbgstub::stop_reason reason = xdbgstub::stop_reason::none;
    uint32_t pc = 0;
    std::optional<source_location> source;
    std::optional<std::string> source_text;
};

class debugger_host {
public:
    virtual ~debugger_host() = default;

    virtual void set_exec_path(const std::filesystem::path& path) = 0;
    virtual const std::optional<std::filesystem::path>& exec_path() const = 0;

    virtual void load_symbols_file(const std::filesystem::path& path) = 0;
    virtual void maybe_load_default_symbols() = 0;
    virtual const std::optional<std::filesystem::path>& symbols_path() const = 0;
    virtual bool has_symbols() const = 0;
    virtual const xdbg::document* symbols() const = 0;

    virtual void connect_remote(const std::string& target) = 0;
    virtual bool is_connected() const = 0;

    virtual breakpoint_entry add_breakpoint(const std::string& expression) = 0;
    virtual void delete_breakpoint(int id) = 0;
    virtual void delete_all_breakpoints() = 0;
    virtual const std::vector<breakpoint_entry>& breakpoints() const = 0;

    virtual xdbgstub::cpu_state read_registers() = 0;
    virtual void write_registers(const xdbgstub::cpu_state& state) = 0;
    virtual xdbgstub::target_status raw_status() = 0;
    virtual stop_snapshot status() = 0;
    virtual stop_snapshot continue_execution() = 0;
    virtual stop_snapshot step_instruction() = 0;
    virtual stop_snapshot pause_execution() = 0;
    virtual void detach() = 0;

    virtual uint32_t resolve_address_expression(const std::string& expression) const = 0;

    virtual std::optional<source_location> current_source_location() = 0;
    virtual std::optional<source_location> source_location_for_address(uint32_t address) const = 0;
    virtual std::optional<source_location> source_location_for_function(
        const std::string& function_name) const = 0;
    virtual line_info_result info_line_current() = 0;
    virtual line_info_result info_line_argument(const std::string& argument) = 0;
    virtual std::vector<source_line_view> list_source(
        const std::optional<std::string>& argument,
        uint32_t context_before = 5,
        uint32_t context_after = 5) = 0;

    virtual std::vector<disassembly_line> disassemble(
        uint32_t address, std::size_t count) = 0;
    virtual std::vector<uint8_t> read_memory(uint32_t address, std::size_t length) = 0;

    virtual std::vector<const xdbg::variable*> visible_variables(uint32_t pc) const = 0;
    virtual std::vector<std::pair<std::string, uint32_t>> register_values() = 0;
};

#endif
