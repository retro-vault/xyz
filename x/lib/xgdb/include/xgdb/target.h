/*
 * Defines the Z80 target model types shared between the debugger
 * host and the remote debug target.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */

#ifndef XGDB_TARGET_H
#define XGDB_TARGET_H

#include <cstdint>
#include <optional>
#include <vector>

namespace xgdb {

    enum class execution_state {
        stopped,
        running,
        terminated
    };

    enum class stop_reason {
        none,
        breakpoint,
        step,
        pause,
        halted,
        exited,
        signal
    };

    struct cpu_state {
        uint16_t af = 0;
        uint16_t bc = 0;
        uint16_t de = 0;
        uint16_t hl = 0;
        uint16_t ix = 0;
        uint16_t iy = 0;
        uint16_t sp = 0;
        uint16_t pc = 0;
        uint8_t  i  = 0;
        uint8_t  r  = 0;
        bool iff1   = false;
        bool iff2   = false;
        bool halted = false;
    };

    struct target_status {
        execution_state state  = execution_state::stopped;
        stop_reason     reason = stop_reason::none;
        uint32_t        pc     = 0;
        std::optional<uint32_t> exit_code;
        std::optional<cpu_state> registers;
    };

} // namespace xgdb

#endif // XGDB_TARGET_H
