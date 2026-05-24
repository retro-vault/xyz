# Emulator Debugger Integration

This project already splits debugger support into two parts:

- `xdbg` is the debugger frontend
- `xdbgstub` is the TCP transport between `xdbg` and your emulator

That means your emulator usually should not implement DAP directly.
Instead, it should expose a remote debug target over TCP, and let
`xdbg --interpreter=dap` speak DAP to VS Code over `stdin/stdout`.

## Architecture

The intended data flow is:

```text
VS Code
  -> DAP over stdio
xdbg --interpreter=dap
  -> xdbgstub client over TCP
your emulator
```

So there are two APIs:

- `xdbg` frontend API: CLI / MI / DAP
- `xdbgstub` target API: remote emulator integration

One important consequence:

- source lookup and source-vs-disassembly presentation live on the
  `xdbg` side
- your emulator-side stub does not send source files or formatted
  assembly listings
- it only needs to provide correct memory, register, breakpoint, and
  execution state behavior so `xdbg` can build source or disassembly
  views on top

## What To Implement In Your Emulator

Your emulator should:

1. derive from `xdbgstub::target`
2. implement the required debug operations
3. create an `xdbgstub::server`
4. listen on a TCP port
5. call `server.serve(your_target)`

Use:

```cpp
#include <xdbgstub/xdbgstub.hpp>
```

Relevant public headers:

- `include/xdbgstub/model.hpp`
- `include/xdbgstub/server.hpp`
- `include/xdbgstub/xdbgstub.hpp`

## Required Target Methods

Your derived class must implement:

- `status()`
- `read_registers()`
- `write_registers(...)`
- `read_memory(...)`
- `write_memory(...)`
- `continue_execution()`
- `step_instruction()`
- `pause_execution()`
- `set_breakpoint(...)`
- `clear_breakpoint(...)`
- `detach()`

These methods are the seam between your emulator core and the debugger.

## Minimal Skeleton

```cpp
#include <array>
#include <cstdint>
#include <vector>

#include <xdbgstub/xdbgstub.hpp>

class emulator_target final : public xdbgstub::target {
public:
    xdbgstub::target_status status() override {
        xdbgstub::target_status s;
        s.state = running_ ? xdbgstub::execution_state::running
                           : xdbgstub::execution_state::stopped;
        s.reason = last_reason_;
        s.pc = regs_.pc;
        s.registers = regs_;
        return s;
    }

    xdbgstub::cpu_state read_registers() override {
        return regs_;
    }

    void write_registers(const xdbgstub::cpu_state& state) override {
        regs_ = state;
    }

    std::vector<uint8_t> read_memory(uint32_t address, std::size_t length) override {
        std::vector<uint8_t> data;
        data.reserve(length);
        for (std::size_t i = 0; i < length; ++i) {
            data.push_back(memory_[(address + i) & 0xFFFF]);
        }
        return data;
    }

    void write_memory(uint32_t address, const std::vector<uint8_t>& data) override {
        for (std::size_t i = 0; i < data.size(); ++i) {
            memory_[(address + i) & 0xFFFF] = data[i];
        }
    }

    xdbgstub::target_status continue_execution() override {
        running_ = true;
        // Run your emulator until:
        // - a breakpoint is hit
        // - the CPU halts/exits
        // - pause is requested
        // Then update regs_, last_reason_, running_ and return status().
        return status();
    }

    xdbgstub::target_status step_instruction() override {
        running_ = false;
        // Execute exactly one instruction here.
        last_reason_ = xdbgstub::stop_reason::step;
        return status();
    }

    xdbgstub::target_status pause_execution() override {
        running_ = false;
        last_reason_ = xdbgstub::stop_reason::pause;
        return status();
    }

    void set_breakpoint(uint32_t address) override {
        breakpoints_.push_back(static_cast<uint16_t>(address));
    }

    void clear_breakpoint(uint32_t address) override {
        const auto value = static_cast<uint16_t>(address);
        breakpoints_.erase(
            std::remove(breakpoints_.begin(), breakpoints_.end(), value),
            breakpoints_.end());
    }

    void detach() override {
        running_ = false;
    }

private:
    xdbgstub::cpu_state regs_ {};
    std::array<uint8_t, 65536> memory_ {};
    std::vector<uint16_t> breakpoints_;
    xdbgstub::stop_reason last_reason_ = xdbgstub::stop_reason::none;
    bool running_ = false;
};

int main() {
    emulator_target target;
    xdbgstub::server server;
    server.listen("127.0.0.1", 9000);
    while (server.is_listening()) {
        server.serve(target);
    }
}
```

If your emulator needs to shut the stub down from another thread, call
`server.close()`. That will wake a blocking `serve()` call even if it is
waiting in `accept()` or on an already connected client.

## What The Emulator Must Keep Updated

Your debugger target should reflect real emulator state:

- registers
- current PC
- memory reads and writes
- running vs stopped state
- why execution stopped
- breakpoint state

In practice this means your emulator core should already have:

- a CPU state structure
- memory access helpers
- an execution loop
- a breakpoint list or hook

If those already exist, the debugger layer is usually thin glue code.

## Breakpoints

The simplest implementation is software-side breakpoint checking:

1. store breakpoint addresses in a container
2. before executing an instruction, compare current PC against that list
3. if matched, stop execution and return `stop_reason::breakpoint`

That is exactly the style used by the reference target in:

- `src/xc/xdbg/src/xdbg_z80.cpp`

## Running Your Emulator With xdbg

Start your emulator server first:

```sh
./your-emulator --listen 127.0.0.1:9000
```

Then connect with `xdbg`:

```sh
xdbg --exec program.bin --symbols program.xdbg --remote 127.0.0.1:9000
```

Or use DAP mode for VS Code:

```sh
xdbg --interpreter=dap --quiet --exec program.bin --symbols program.xdbg --remote 127.0.0.1:9000
```

## VS Code Setup

The VS Code side talks to `xdbg`, not directly to your emulator.

So your launch config should point to:

- debugger executable: `xdbg`
- remote target: `127.0.0.1:9000`
- program: your binary image
- symbols: your `.xdbg` file

## Important Clarification

`debug_protocol` and the DAP frontend are not the emulator-side API.

They are for debugger frontend implementations and they run over
`stdin/stdout`.

For emulator integration, the API you want is `xdbgstub::target`
plus `xdbgstub::server`.

## Recommended Reference Files

Read these first:

- `include/xdbgstub/server.hpp`
- `include/xdbgstub/model.hpp`
- `lib/xdbgstub/README.md`
- `src/xc/xdbg/src/xdbg_z80.cpp`
- `src/xc/xdbg/README.md`
