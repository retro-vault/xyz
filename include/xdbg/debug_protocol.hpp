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
    virtual ~debug_protocol() = default;
    virtual int run() = 0;
};

std::unique_ptr<debug_protocol> make_cli_protocol(
    debugger_host& host,
    std::istream& input,
    std::ostream& output,
    std::ostream& error,
    bool quiet,
    bool show_prompt = true);

std::unique_ptr<debug_protocol> make_mi_protocol(debugger_host& host);

std::unique_ptr<debug_protocol> make_dap_protocol(debugger_host& host);

#endif
