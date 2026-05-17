/*
 * Defines the xdbgstub protocol model shared by debugger clients,
 * servers, and target implementations during hosted debug sessions.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */

#ifndef XDBGSTUB_MODEL_HPP
#define XDBGSTUB_MODEL_HPP

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace xdbgstub {

    // High-level execution state reported by a debug target.
    enum class execution_state {
        stopped,     // Target is stopped and available for inspection.
        running,     // Target is currently running.
        terminated   // Target has terminated and will not execute further.
    };

    // Reason why target execution stopped.
    enum class stop_reason {
        none,         // No specific stop reason is available.
        breakpoint,   // Execution stopped at a breakpoint.
        step,         // Execution stopped after a step request.
        pause,        // Execution stopped because a pause was requested.
        halted,       // Execution stopped because the CPU entered a halted state.
        exited,       // Execution stopped because the target program exited.
        signal        // Execution stopped because of a signal or external trap.
    };

    /*
     * Complete CPU register snapshot for the emulated Z80 target.
     */
    struct cpu_state {
        uint16_t af = 0;       /* Combined accumulator and flags register. */
        uint16_t bc = 0;       /* General-purpose BC register pair. */
        uint16_t de = 0;       /* General-purpose DE register pair. */
        uint16_t hl = 0;       /* General-purpose HL register pair. */
        uint16_t ix = 0;       /* Index register IX. */
        uint16_t iy = 0;       /* Index register IY. */
        uint16_t sp = 0;       /* Stack pointer register. */
        uint16_t pc = 0;       /* Program counter register. */
        uint8_t i = 0;         /* Interrupt vector register. */
        uint8_t r = 0;         /* Memory refresh register. */
        bool iff1 = false;     /* Primary interrupt enable flip-flop. */
        bool iff2 = false;     /* Secondary interrupt enable flip-flop. */
        bool halted = false;   /* CPU halted state flag. */
    };

    /*
     * Snapshot of the target's current execution status.
     */
    struct target_status {
        execution_state state = execution_state::stopped; /* Current execution state. */
        stop_reason reason = stop_reason::none;           /* Reason for the current stop, when applicable. */
        uint32_t pc = 0;                                  /* Program counter at the stop point. */
        std::optional<uint32_t> exit_code;                /* Exit code when the target has terminated. */
        std::optional<cpu_state> registers;               /* Optional register snapshot captured with the status. */
    };

    /*
     * Breakpoint definition stored by a debugger client or target.
     */
    struct breakpoint {
        uint32_t address = 0;   /* Address where execution should stop. */
        bool enabled = true;    /* Breakpoint enable flag. */
    };

    /*
     * Abstract debug target served over the xdbgstub protocol.
     */
    class target {
    public:
        /*
         * Destroy the target implementation through the interface.
         */
        virtual ~target() = default;

        /*
         * Query the current execution status.
         *
         * Returns:
         *      Current target status snapshot.
         */
        virtual target_status status() = 0;

        /*
         * Read the target CPU register set.
         *
         * Returns:
         *      Current CPU register snapshot.
         */
        virtual cpu_state read_registers() = 0;

        /*
         * Replace the target CPU register set.
         *
         * Parameters:
         *      state       - Register values to write.
         */
        virtual void write_registers(const cpu_state& state) = 0;

        /*
         * Read a block of target memory.
         *
         * Parameters:
         *      address     - Start address to read from.
         *      length      - Number of bytes to read.
         *
         * Returns:
         *      Bytes read from the requested address range.
         */
        virtual std::vector<uint8_t> read_memory(
            uint32_t address, std::size_t length) = 0;

        /*
         * Write a block of bytes into target memory.
         *
         * Parameters:
         *      address     - Start address to write to.
         *      data        - Bytes to store in memory.
         */
        virtual void write_memory(
            uint32_t address, const std::vector<uint8_t>& data) = 0;

        /*
         * Resume target execution.
         *
         * Returns:
         *      Target status after the run completes or stops.
         */
        virtual target_status continue_execution() = 0;

        /*
         * Execute one instruction when supported by the target.
         *
         * Returns:
         *      Target status after the step completes.
         */
        virtual target_status step_instruction() = 0;

        /*
         * Request that target execution pause.
         *
         * Returns:
         *      Target status after the pause request completes.
         */
        virtual target_status pause_execution() = 0;

        /*
         * Install a breakpoint at the specified address.
         *
         * Parameters:
         *      address     - Address where execution should stop.
         */
        virtual void set_breakpoint(uint32_t address) = 0;

        /*
         * Remove a breakpoint at the specified address.
         *
         * Parameters:
         *      address     - Address to clear.
         */
        virtual void clear_breakpoint(uint32_t address) = 0;

        /*
         * Detach debugger control from the target.
         */
        virtual void detach() = 0;
    };

} // namespace xdbgstub

#endif // XDBGSTUB_MODEL_HPP
