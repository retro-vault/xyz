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

// Breakpoint record tracked by the debugger session.
struct breakpoint_entry {
    int id = 0;                  // Stable debugger-visible breakpoint identifier.
    uint32_t address = 0;        // Resolved machine address.
    std::string expression;      // Original user expression used to create it.
};

// Source location resolved from debug metadata.
struct source_location {
    uint32_t address = 0;                        // Machine address of the location.
    std::string file_path;                       // Source file path.
    uint32_t line = 0;                           // One-based source line number.
    uint32_t column = 0;                         // One-based source column number.
    std::optional<std::string> function_name;   // Optional containing function name.
};

// Result of an `info line` style query.
struct line_info_result {
    source_location location;                // Start of the resolved source range.
    std::optional<uint32_t> end_address;     // Optional end address of the same range.
};

// One line of source text prepared for display.
struct source_line_view {
    uint32_t line = 0;        // One-based source line number.
    std::string text;         // Source text content.
    bool is_current = false;  // True when this is the current execution line.
};

// One rendered disassembly line.
struct disassembly_line {
    uint32_t address = 0;         // Instruction address.
    std::vector<uint8_t> bytes;   // Raw instruction bytes.
    std::string text;             // Human-readable disassembly text.
};

// Normalized execution-stop snapshot shared with frontends.
struct stop_snapshot {
    xdbgstub::stop_reason reason = xdbgstub::stop_reason::none;  // Reason the target stopped.
    uint32_t pc = 0;                                             // Program counter at the stop point.
    std::optional<source_location> source;                       // Optional resolved source location.
    std::optional<std::string> source_text;                      // Optional current source line text.
};

// Abstract debugger backend consumed by CLI, MI, and DAP frontends.
class debugger_host {
public:
    // Destroy one debugger host through the abstract base.
    //
    // Parameters:
    //      None.
    //
    // Returns:
    //      Nothing.
    virtual ~debugger_host() = default;

    // Set the executable image path.
    //
    // Parameters:
    //      path        - Binary or image path associated with the session.
    //
    // Returns:
    //      Nothing.
    virtual void set_exec_path(const std::filesystem::path& path) = 0;

    // Return the current executable image path.
    //
    // Parameters:
    //      None.
    //
    // Returns:
    //      Reference to the optional executable path.
    virtual const std::optional<std::filesystem::path>& exec_path() const = 0;

    // Load one explicit symbols file.
    //
    // Parameters:
    //      path        - Path to the `.xdbg` symbols file.
    //
    // Returns:
    //      Nothing.
    virtual void load_symbols_file(const std::filesystem::path& path) = 0;

    // Load the default sidecar symbols file when available.
    //
    // Parameters:
    //      None.
    //
    // Returns:
    //      Nothing.
    //
    // Notes:
    //      Implementations typically derive the default symbol path from
    //      the current executable image path.
    virtual void maybe_load_default_symbols() = 0;

    // Return the current symbols file path.
    //
    // Parameters:
    //      None.
    //
    // Returns:
    //      Reference to the optional symbols path.
    virtual const std::optional<std::filesystem::path>& symbols_path() const = 0;

    // Check whether symbols are currently loaded.
    //
    // Parameters:
    //      None.
    //
    // Returns:
    //      True when symbol data is available, otherwise false.
    virtual bool has_symbols() const = 0;

    // Return the loaded debug document.
    //
    // Parameters:
    //      None.
    //
    // Returns:
    //      Pointer to the loaded document, or `nullptr` when unavailable.
    virtual const xdbg::document* symbols() const = 0;

    // Connect to a remote target identified by `host:port`.
    //
    // Parameters:
    //      target      - Remote endpoint in host-and-port form.
    //
    // Returns:
    //      Nothing.
    virtual void connect_remote(const std::string& target) = 0;

    // Check whether the remote target is currently connected.
    //
    // Parameters:
    //      None.
    //
    // Returns:
    //      True when the transport is connected, otherwise false.
    virtual bool is_connected() const = 0;

    // Add one breakpoint expression and return the created record.
    //
    // Parameters:
    //      expression  - User-facing expression that resolves to an address.
    //
    // Returns:
    //      Fully populated breakpoint record.
    virtual breakpoint_entry add_breakpoint(const std::string& expression) = 0;

    // Delete one breakpoint by debugger-visible identifier.
    //
    // Parameters:
    //      id          - Breakpoint id previously returned by `add_breakpoint()`.
    //
    // Returns:
    //      Nothing.
    virtual void delete_breakpoint(int id) = 0;

    // Delete all installed breakpoints.
    //
    // Parameters:
    //      None.
    //
    // Returns:
    //      Nothing.
    virtual void delete_all_breakpoints() = 0;

    // Return the current breakpoint table.
    //
    // Parameters:
    //      None.
    //
    // Returns:
    //      Reference to the internal breakpoint list.
    virtual const std::vector<breakpoint_entry>& breakpoints() const = 0;

    // Read the target register set.
    //
    // Parameters:
    //      None.
    //
    // Returns:
    //      Current target CPU state snapshot.
    virtual xdbgstub::cpu_state read_registers() = 0;

    // Write the target register set.
    //
    // Parameters:
    //      state       - Register values to write into the target.
    //
    // Returns:
    //      Nothing.
    virtual void write_registers(const xdbgstub::cpu_state& state) = 0;

    // Return the raw target transport status.
    //
    // Parameters:
    //      None.
    //
    // Returns:
    //      Raw status structure reported by the target transport.
    virtual xdbgstub::target_status raw_status() = 0;

    // Return a normalized stop snapshot.
    //
    // Parameters:
    //      None.
    //
    // Returns:
    //      Frontend-friendly stop snapshot.
    virtual stop_snapshot status() = 0;

    // Resume execution until the next stop.
    //
    // Parameters:
    //      None.
    //
    // Returns:
    //      Stop snapshot captured after execution stops again.
    virtual stop_snapshot continue_execution() = 0;

    // Execute a single instruction when supported.
    //
    // Parameters:
    //      None.
    //
    // Returns:
    //      Stop snapshot captured after the step completes.
    virtual stop_snapshot step_instruction() = 0;

    // Pause target execution.
    //
    // Parameters:
    //      None.
    //
    // Returns:
    //      Stop snapshot captured after the pause completes.
    virtual stop_snapshot pause_execution() = 0;

    // Detach from the remote target.
    //
    // Parameters:
    //      None.
    //
    // Returns:
    //      Nothing.
    virtual void detach() = 0;

    // Resolve one user-facing address expression to a machine address.
    //
    // Parameters:
    //      expression  - Symbol, number, or other supported address expression.
    //
    // Returns:
    //      Resolved machine address.
    virtual uint32_t resolve_address_expression(const std::string& expression) = 0;

    // Return the current source location when known.
    //
    // Parameters:
    //      None.
    //
    // Returns:
    //      Resolved source location for the current stop point, or no value.
    virtual std::optional<source_location> current_source_location() = 0;

    // Return the source location for one machine address.
    //
    // Parameters:
    //      address     - Machine address to map back to source.
    //
    // Returns:
    //      Matching source location, or no value when unavailable.
    virtual std::optional<source_location> source_location_for_address(uint32_t address) const = 0;

    // Return the source location for one function name.
    //
    // Parameters:
    //      function_name - Function symbol name to resolve.
    //
    // Returns:
    //      Matching source location, or no value when unavailable.
    virtual std::optional<source_location> source_location_for_function(
        const std::string& function_name) const = 0;

    // Return `info line` output for the current execution point.
    //
    // Parameters:
    //      None.
    //
    // Returns:
    //      Source range summary for the current stop point.
    virtual line_info_result info_line_current() = 0;

    // Return `info line` output for one explicit argument.
    //
    // Parameters:
    //      argument    - Source-or-symbol argument accepted by the frontend.
    //
    // Returns:
    //      Source range summary for the resolved argument.
    virtual line_info_result info_line_argument(const std::string& argument) = 0;

    // Return a source listing window around the resolved location.
    //
    // Parameters:
    //      argument       - Optional explicit source argument; current location when omitted.
    //      context_before - Number of lines to include before the focus line.
    //      context_after  - Number of lines to include after the focus line.
    //
    // Returns:
    //      Ordered source lines prepared for display.
    virtual std::vector<source_line_view> list_source(
        const std::optional<std::string>& argument,
        uint32_t context_before = 5,
        uint32_t context_after = 5) = 0;

    // Disassemble a range of target instructions.
    //
    // Parameters:
    //      address     - Start address to disassemble.
    //      count       - Maximum instruction count to decode.
    //
    // Returns:
    //      Decoded disassembly lines.
    virtual std::vector<disassembly_line> disassemble(
        uint32_t address, std::size_t count) = 0;

    // Read a block of target memory.
    //
    // Parameters:
    //      address     - Start address to read from.
    //      length      - Number of bytes to read.
    //
    // Returns:
    //      Raw bytes read from target memory.
    virtual std::vector<uint8_t> read_memory(uint32_t address, std::size_t length) = 0;

    // Return variables visible at the supplied program counter.
    //
    // Parameters:
    //      pc          - Program counter used to select active scopes.
    //
    // Returns:
    //      Pointers to visible variable metadata records.
    virtual std::vector<const xdbg::variable*> visible_variables(uint32_t pc) const = 0;

    // Return a formatted register snapshot.
    //
    // Parameters:
    //      None.
    //
    // Returns:
    //      Ordered register name/value pairs.
    virtual std::vector<std::pair<std::string, uint32_t>> register_values() = 0;
};

#endif
