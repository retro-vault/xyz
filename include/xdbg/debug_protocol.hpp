/*
 * Declares the reusable protocol frontends and factory functions
 * that build CLI, MI, and DAP interfaces on top of a debugger host.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */

#ifndef XDBG_DEBUG_PROTOCOL_HPP
#define XDBG_DEBUG_PROTOCOL_HPP

#include <iosfwd>
#include <memory>

#include <xdbg/debugger_host.hpp>

class debug_protocol {
public:
    // Destroy one protocol instance through the abstract base.
    //
    // Parameters:
    //      None.
    //
    // Returns:
    //      Nothing.
    virtual ~debug_protocol() = default;

    // Run the protocol loop until completion.
    //
    // Parameters:
    //      None.
    //
    // Returns:
    //      Process exit code for the protocol session.
    //
    // Notes:
    //      Implementations usually block until the frontend disconnects,
    //      reaches EOF, or decides to terminate the debugger session.
    virtual int run() = 0;
};

// Build a CLI protocol instance over the supplied IO streams.
//
// Parameters:
//      host        - Debugger host used to service protocol requests.
//      input       - Stream used for command input.
//      output      - Stream used for normal command output.
//      error       - Stream used for diagnostics and errors.
//      quiet       - Suppress non-essential startup chatter when true.
//      show_prompt - Emit an interactive prompt when true.
//
// Returns:
//      Heap-allocated protocol instance.
std::unique_ptr<debug_protocol> make_cli_protocol(
    debugger_host& host,
    std::istream& input,
    std::ostream& output,
    std::ostream& error,
    bool quiet,
    bool show_prompt = true);

// Build an MI protocol instance.
//
// Parameters:
//      host        - Debugger host used to service MI requests.
//
// Returns:
//      Heap-allocated protocol instance.
std::unique_ptr<debug_protocol> make_mi_protocol(debugger_host& host);

// Build a DAP protocol instance.
//
// Parameters:
//      host        - Debugger host used to service DAP requests.
//
// Returns:
//      Heap-allocated protocol instance.
std::unique_ptr<debug_protocol> make_dap_protocol(debugger_host& host);

#endif
