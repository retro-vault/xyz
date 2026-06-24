# Emulator Debugger Integration

This guide reflects the current debugger stack in this repository.

## Current Architecture

The live split is:

- `xgdb`
  the debugger frontend
- `librsp`
  the transport layer between the debugger and a target
- `xemu`
  the reference Z80 emulator and remote target built on that transport

In other words, an emulator should not implement a debugger frontend
protocol directly first. The normal integration point is the remote target
side of the GDB Remote Serial Protocol (RSP).

The data flow today is:

```text
xgdb
  -> librsp client over TCP
your emulator implementing rsp::target
  <- librsp server over TCP
```

`xgdb` handles symbol loading, source lookup, disassembly presentation,
breakpoint bookkeeping, and user-facing protocols. Your emulator-side
integration only needs to expose correct target state and execution control.

## Which API To Implement

For emulator integration, implement `rsp::target` from:

```cpp
#include <rsp/rsp.h>
```

Relevant public headers:

- `lib/rsp/include/rsp/rsp.h`
- `lib/xgdb/include/xgdb/target.h`
- `lib/xgdb/include/xgdb/model.h`
- `lib/xgdb/include/xgdb/debugger_host.h`
- `lib/xgdb/include/xgdb/debug_protocol.h`

In practice, most emulator authors only need `rsp/rsp.h`.

## Required `rsp::target` Methods

Your derived class must implement:

- `read_registers()`
- `write_registers(...)`
- `read_memory(...)`
- `write_memory(...)`
- `cont()`
- `step()`
- `stop_reason()`
- `insert_breakpoint(...)`
- `remove_breakpoint(...)`
- `detach()`

These methods form the seam between your emulator core and the debugger
stack.

## Minimal Skeleton

```cpp
#include <algorithm>
#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include <rsp/rsp.h>

class emulator_target final : public rsp::target {
public:
    std::vector<uint8_t> read_registers() override {
        return register_bytes_;
    }

    void write_registers(const std::vector<uint8_t>& data) override {
        register_bytes_ = data;
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

    std::string cont() override {
        running_ = true;
        // Run until stop, then return an RSP stop reply such as "S05".
        running_ = false;
        return last_stop_;
    }

    std::string step() override {
        running_ = false;
        // Execute one instruction here.
        last_stop_ = "S05";
        return last_stop_;
    }

    std::string stop_reason() override {
        return last_stop_;
    }

    void insert_breakpoint(uint32_t address) override {
        breakpoints_.push_back(static_cast<uint16_t>(address));
    }

    void remove_breakpoint(uint32_t address) override {
        const auto value = static_cast<uint16_t>(address);
        breakpoints_.erase(
            std::remove(breakpoints_.begin(), breakpoints_.end(), value),
            breakpoints_.end());
    }

    void detach() override {
        running_ = false;
    }

private:
    std::vector<uint8_t> register_bytes_ {};
    std::array<uint8_t, 65536> memory_ {};
    std::vector<uint16_t> breakpoints_ {};
    std::string last_stop_ = "S05";
    bool running_ = false;
};

int main() {
    emulator_target target;
    rsp::server server;
    server.listen("127.0.0.1", 9000);
    while (server.is_listening()) {
        server.serve(target);
    }
}
```

If the emulator needs to shut the debug server down from another thread,
call `server.close()`. That wakes a blocking `serve()` call even when it is
waiting in `accept()` or on an already connected client.

## What the Emulator Must Keep Accurate

Your target implementation must reflect real emulator state:

- registers
- current PC
- memory contents
- running vs stopped state
- reason for the last stop
- breakpoint state

If your emulator already has a CPU state structure, memory access helpers,
an execution loop, and some breakpoint hook point, the debugger layer is
usually thin glue.

## Breakpoint Strategy

The simplest approach is software-side breakpoint checking:

1. store breakpoint addresses in a container
2. compare the current PC against that list before instruction execution
3. stop and return an RSP reply such as `"S05"` when matched

The reference target follows that pattern in:

- `x/lib/xemu/src/xemu.cpp`
- `x/src/xemu/src/main.cpp`

## Running Against `xgdb`

Start the emulator-side RSP server first:

```sh
./your-emulator --listen 127.0.0.1:9000
```

Then connect with `xgdb`:

```sh
bin/x/bin/xgdb --exec program.bin --cdb program.cdb --map program.map --remote 127.0.0.1:9000
```

Useful notes:

- `--cdb` loads SDCC debug information
- `--map` loads linker MAP data and supplements the CDB view
- if `--cdb` is omitted, `xgdb` also tries sidecar files derived from `--exec`

## Frontend vs Target Responsibilities

Keep this split in mind:

- `xgdb` frontend side:
  symbols, source mapping, disassembly views, CLI/MI/DAP protocol handling
- emulator target side:
  registers, memory, execution control, breakpoint behavior

So the emulator does not send source files, line tables, or formatted
disassembly. It exposes machine state, and `xgdb` builds the user-facing
debug view on top of that.

## Recommended Reference Files

Read these first:

- `lib/rsp/include/rsp/rsp.h`
- `lib/xgdb/include/xgdb/target.h`
- `x/lib/xemu/include/xemu/xemu.h`
- `x/src/xgdb/README.md`
- `x/src/xemu/README.md`
