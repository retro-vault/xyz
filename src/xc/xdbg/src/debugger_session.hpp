// Declares the concrete debugger session that binds xdbg symbol handling,
// source lookup, breakpoint tracking, and xdbgstub transport together.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih

#ifndef XDBG_DEBUGGER_SESSION_HPP
#define XDBG_DEBUGGER_SESSION_HPP

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include <xdbg/debugger_host.hpp>
#include <xdbg/xdbg.hpp>
#include <xdbgstub/xdbgstub.hpp>

// Shared debugger session implementation used by all xdbg frontends.
class debugger_session : public debugger_host {
public:
    // Set the executable image path.
    void set_exec_path(const std::filesystem::path& path) override;
    // Return the current executable image path when set.
    const std::optional<std::filesystem::path>& exec_path() const override;

    // Load one explicit symbols file.
    void load_symbols_file(const std::filesystem::path& path) override;
    // Load the default sidecar symbols file when available.
    void maybe_load_default_symbols() override;
    // Return the current symbols file path when set.
    const std::optional<std::filesystem::path>& symbols_path() const override;
    // Check whether symbols are currently loaded.
    bool has_symbols() const override;
    // Return the loaded xdbg document when available.
    const xdbg::document* symbols() const override;

    // Connect to a remote target identified by `host:port`.
    void connect_remote(const std::string& target) override;
    // Check whether the remote session is active.
    bool is_connected() const override;

    // Add one breakpoint expression and return its resolved record.
    breakpoint_entry add_breakpoint(const std::string& expression) override;
    // Delete one breakpoint by debugger-visible identifier.
    void delete_breakpoint(int id) override;
    // Delete all installed breakpoints.
    void delete_all_breakpoints() override;
    // Return the current breakpoint table.
    const std::vector<breakpoint_entry>& breakpoints() const override;

    // Read the target register set.
    xdbgstub::cpu_state read_registers() override;
    // Write the target register set.
    void write_registers(const xdbgstub::cpu_state& state) override;
    // Return the raw target status from the transport layer.
    xdbgstub::target_status raw_status() override;
    // Return a normalized stop snapshot.
    stop_snapshot status() override;
    // Resume execution until the next stop.
    stop_snapshot continue_execution() override;
    // Execute a single instruction when supported.
    stop_snapshot step_instruction() override;
    // Pause target execution.
    stop_snapshot pause_execution() override;
    // Detach from the remote target.
    void detach() override;

    // Resolve one CLI or frontend address expression to an absolute address.
    uint32_t resolve_address_expression(const std::string& expression) override;

    // Return the current source location, if known.
    std::optional<source_location> current_source_location() override;
    // Return the source location that corresponds to one machine address.
    std::optional<source_location> source_location_for_address(uint32_t address) const override;
    // Return the starting source location of one named function.
    std::optional<source_location> source_location_for_function(
        const std::string& function_name) const override;
    // Return `info line` output for the current stop point.
    line_info_result info_line_current() override;
    // Return `info line` output for an explicit source argument.
    line_info_result info_line_argument(const std::string& argument) override;
    // Return a source listing window around one resolved location.
    std::vector<source_line_view> list_source(
        const std::optional<std::string>& argument,
        uint32_t context_before = 5,
        uint32_t context_after = 5) override;

    // Disassemble a range of instructions from target memory.
    std::vector<disassembly_line> disassemble(
        uint32_t address, std::size_t count) override;
    // Read a block of target memory.
    std::vector<uint8_t> read_memory(uint32_t address, std::size_t length) override;

    // Return variables visible at the supplied program counter.
    std::vector<const xdbg::variable*> visible_variables(uint32_t pc) const override;
    // Return a formatted snapshot of the visible register values.
    std::vector<std::pair<std::string, uint32_t>> register_values() override;

private:
    // Convert raw target status into the richer xdbg stop model.
    stop_snapshot make_stop_snapshot(const xdbgstub::target_status& status);
    // Read one source line from disk when available.
    std::optional<std::string> read_source_line(
        const std::filesystem::path& path, uint32_t target_line) const;
    // Return a window of source lines around a target line number.
    std::vector<source_line_view> source_window(
        const std::filesystem::path& path,
        uint32_t target_line,
        uint32_t context_before,
        uint32_t context_after) const;

    // Look up a source-file record by file identifier.
    const xdbg::source_file* find_file(uint32_t file_id) const;
    // Look up a function record by function name.
    const xdbg::function* find_function_by_name(const std::string& name) const;
    // Look up a symbol record by symbol name.
    const xdbg::symbol* find_symbol_by_name(const std::string& name) const;
    // Find the function that covers the supplied program counter.
    const xdbg::function* find_function_for_pc(uint32_t pc) const;
    // Find the best line entry for the supplied program counter.
    const xdbg::line_entry* find_line_for_pc(uint32_t pc) const;
    // Find the first line entry that belongs to a function.
    const xdbg::line_entry* find_line_for_function(
        const xdbg::function& function) const;
    // Find one line entry by source file and line number.
    const xdbg::line_entry* find_line_by_file_and_line(
        uint32_t file_id, uint32_t line_number) const;
    // Estimate the end address of one source line range.
    std::optional<uint32_t> find_line_end_address(
        const xdbg::line_entry& line) const;

    std::optional<std::filesystem::path> exec_path_;
    std::optional<xdbg::document> symbols_;
    std::optional<std::filesystem::path> symbols_path_;
    xdbgstub::client remote_;
    std::vector<breakpoint_entry> breakpoints_;
    int next_breakpoint_id_ = 1;
};

#endif
