# libxgdb

`libxgdb` is the host-side debug information library for the `xgdb`
toolchain.

It has two jobs:

- define the internal `xgdb::document` model used to exchange debug metadata between tools
- provide a pluggable disassembly interface, with a Z80 backend and an
  SDCC-style formatter today

Source-level debug symbols are loaded elsewhere in the toolchain from linked
ELF files or SDCC `.cdb`/`.map` sidecars; this library only provides the
in-memory model and disassembly surface.

The library is intended for tools such as:

- `xgdb`, when it disassembles target memory and works with the debug model

The library currently builds and stages a static archive at:

- `bin/lib/libxgdb.a`

## Public Headers

Public headers live in [lib/xgdb/include/xgdb](/home/tstih/data/retro-vault/xyz/lib/xgdb/include/xgdb):

- [xgdb.h](/home/tstih/data/retro-vault/xyz/lib/xgdb/include/xgdb/xgdb.h)
- [model.h](/home/tstih/data/retro-vault/xyz/lib/xgdb/include/xgdb/model.h)
- [error.h](/home/tstih/data/retro-vault/xyz/lib/xgdb/include/xgdb/error.h)
- [disassembler.h](/home/tstih/data/retro-vault/xyz/lib/xgdb/include/xgdb/disassembler.h)

Use the umbrella include when you want the full surface:

```cpp
#include <xgdb/xgdb.h>
```

## What The Model Represents

The top-level type is `xgdb::document`. It contains:

- `files`: source file table
- `symbols`: globals, labels, sections, constants, objects, function symbols
- `functions`: named address ranges
- `lines`: address-to-source mappings
- `variables`: locals, parameters, and other scoped values

Important enums:

- `xgdb::language_kind`: `unknown`, `c`, `cxx`, `assembly`
- `xgdb::symbol_kind`: `function`, `global`, `local`, `parameter`, `label`, `section`, `type`, `constant`, `object`
- `xgdb::storage_kind`: `address`, `stack`, `register_name`, `register_pair`, `frame_relative`

This covers both C and assembly debug data.  The document is populated by
`debugger_session` via translation from linked ELF files or SDCC `.cdb`/`.map`
sidecars; `libxgdb` itself provides no file I/O.

## Disassembly API

The disassembly layer is intentionally open-ended.

Core interfaces:

- `xgdb::memory_reader`
- `xgdb::disassembler`
- `xgdb::syntax_formatter`

Current factories:

- `xgdb::make_z80_disassembler()`
- `xgdb::make_native_formatter()`
- `xgdb::make_sdcc_z80_formatter()`

This lets you separate:

- how bytes are fetched
- how instructions are decoded
- how instructions are rendered

Sample:

```cpp
#include <iostream>
#include <vector>
#include <xgdb/xgdb.h>

int main() {
    std::vector<uint8_t> bytes = {0x3E, 0x42, 0xDD, 0x7E, 0x05, 0xC9};
    xgdb::vector_memory_reader memory(bytes);

    auto disassembler = xgdb::make_z80_disassembler();
    auto formatter = xgdb::make_sdcc_z80_formatter();

    uint32_t pc = 0;
    for (int i = 0; i < 3; ++i) {
        const auto instruction = disassembler->disassemble_one(pc, memory);
        std::cout << formatter->format(instruction) << "\n";
        pc += static_cast<uint32_t>(instruction.bytes.size());
    }
}
```

Typical SDCC-style output:

```text
ld	a, #0x42
ld	a, 5(ix)
ret
```

## Errors

Errors derive from `xgdb::error`.

- `xgdb::error`: generic library/runtime failure

## Build

Build the library:

```sh
make -C lib/xgdb all
```

Run tests:

```sh
make -C lib/xgdb test
```

## Current Limits

- The library currently builds as a static archive, not `libxgdb.so`.
- The only built-in decoder today is Z80.
- The only built-in alternate syntax formatter today is SDCC-flavored Z80.

Those limits are deliberate. The API is meant to stay stable while more
CPU backends and emitters get added later.
