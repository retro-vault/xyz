# libxdbgstub

`libxdbgstub` is the transport layer between `xdbg` and a remote target.

It gives you:

- a client API for debugger-side tools
- a server API for emulator-side stubs
- a small abstract `target` interface that an emulator implements
- a simple line-based protocol over TCP

The build currently produces a static archive:

- `build/lib/xdbgstub/libxdbgstub.a`

## Public Headers

Public headers live in [include/xdbgstub](/home/tstih/data/retro-vault/xyz/include/xdbgstub):

- [xdbgstub.hpp](/home/tstih/data/retro-vault/xyz/include/xdbgstub/xdbgstub.hpp)
- [model.hpp](/home/tstih/data/retro-vault/xyz/include/xdbgstub/model.hpp)
- [client.hpp](/home/tstih/data/retro-vault/xyz/include/xdbgstub/client.hpp)
- [server.hpp](/home/tstih/data/retro-vault/xyz/include/xdbgstub/server.hpp)
- [error.hpp](/home/tstih/data/retro-vault/xyz/include/xdbgstub/error.hpp)

Use the umbrella include when you want the full interface:

```cpp
#include <xdbgstub/xdbgstub.hpp>
```

## The Execution Model

The emulator side implements `xdbgstub::target`.

That interface exposes:

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

The debugger side uses `xdbgstub::client`, which mirrors those
operations.

`xdbgstub::server` accepts a TCP client and dispatches requests to a
`target`.

This layer does not carry source files or pre-rendered assembly text.
Those views are built on the debugger side by `xdbg`, using:

- `.xdbg` symbol data for source mappings when available
- live target memory from `xdbgstub` for disassembly

## Data Types

The central state types are:

- `xdbgstub::cpu_state`
- `xdbgstub::target_status`
- `xdbgstub::execution_state`
- `xdbgstub::stop_reason`

`cpu_state` currently models the Z80 register set used by `xdbg-z80`:

- `af`, `bc`, `de`, `hl`
- `ix`, `iy`
- `sp`, `pc`
- `i`, `r`
- `iff1`, `iff2`
- `halted`

That is enough for the first remote debugger loop. If you later want a
CPU-neutral register file, this layer can be expanded.

## Minimal Client Example

```cpp
#include <iostream>
#include <xdbgstub/xdbgstub.hpp>

int main() {
    xdbgstub::client remote;
    remote.connect("127.0.0.1", 9000);

    const auto status = remote.status();
    std::cout << "pc=0x" << std::hex << status.pc << "\n";

    const auto regs = remote.read_registers();
    std::cout << "af=0x" << std::hex << regs.af << "\n";

    remote.set_breakpoint(0x0100);
    remote.continue_execution();
    remote.detach();
}
```

## Minimal Emulator Stub Example

```cpp
#include <xdbgstub/xdbgstub.hpp>

class toy_target final : public xdbgstub::target {
public:
    xdbgstub::target_status status() override { return status_; }
    xdbgstub::cpu_state read_registers() override { return regs_; }
    void write_registers(const xdbgstub::cpu_state& state) override { regs_ = state; }

    std::vector<uint8_t> read_memory(uint32_t address, std::size_t length) override {
        return std::vector<uint8_t>(length, memory_[address & 0xFFFF]);
    }

    void write_memory(uint32_t address, const std::vector<uint8_t>& data) override {
        for (std::size_t i = 0; i < data.size(); ++i) {
            memory_[(address + i) & 0xFFFF] = data[i];
        }
    }

    xdbgstub::target_status continue_execution() override { return status_; }
    xdbgstub::target_status step_instruction() override { return status_; }
    xdbgstub::target_status pause_execution() override { return status_; }
    void set_breakpoint(uint32_t) override {}
    void clear_breakpoint(uint32_t) override {}
    void detach() override {}

private:
    xdbgstub::cpu_state regs_;
    xdbgstub::target_status status_;
    std::array<uint8_t, 65536> memory_ {};
};

int main() {
    toy_target target;
    xdbgstub::server server;
    server.listen("127.0.0.1", 9000);
    while (server.is_listening()) {
        server.serve(target);
    }
}
```

To stop a running server from another thread during emulator shutdown,
call `server.close()`. That now closes both the listening socket and any
active client connection so a blocking `serve()` call can return cleanly.

## Protocol Shape

The wire protocol is intentionally simple:

- one message per line
- a record kind first
- `key=value` pairs after that
- quoted strings when needed

Example request:

```text
request command="read_memory" id="3" address="0x100" length="16"
```

Example response:

```text
response id="3" status="ok" data="3E42DD7E05C9"
```

Commands currently implemented:

- `ping`
- `status`
- `read_registers`
- `write_registers`
- `read_memory`
- `write_memory`
- `continue`
- `step_instruction`
- `pause`
- `set_breakpoint`
- `clear_breakpoint`
- `detach`

## Build

Build the library:

```sh
make -C lib/xdbgstub all
```

Run tests:

```sh
make -C lib/xdbgstub test
```

## Intended Use

This library is the clean seam between:

- `xdbg`, which behaves like a debugger front-end
- an emulator, which provides machine behavior

That keeps your emulator-specific code small: implement `target`, start
`server`, and the rest of the command transport is already done.
