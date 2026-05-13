# libxdbg

`libxdbg` is the host-side debug information library for the `xdbg`
toolchain.

It has two jobs:

- read and write linked `.xdbg` debug databases
- provide a pluggable disassembly interface, with a Z80 backend and an
  SDCC-style formatter today

The library is intended for tools such as:

- `xlink`, when it starts emitting linked debug sidecars
- `xdbg`, when it loads symbols and disassembles target memory
- a future UDAP adapter or IDE integration

At the moment the build produces a static archive:

- `build/lib/xdbg/libxdbg.a`

## Public Headers

Public headers live in [include/xdbg](/home/tstih/data/retro-vault/xyz/include/xdbg):

- [xdbg.hpp](/home/tstih/data/retro-vault/xyz/include/xdbg/xdbg.hpp)
- [model.hpp](/home/tstih/data/retro-vault/xyz/include/xdbg/model.hpp)
- [io.hpp](/home/tstih/data/retro-vault/xyz/include/xdbg/io.hpp)
- [error.hpp](/home/tstih/data/retro-vault/xyz/include/xdbg/error.hpp)
- [disassembler.hpp](/home/tstih/data/retro-vault/xyz/include/xdbg/disassembler.hpp)

Use the umbrella include when you want the full surface:

```cpp
#include <xdbg/xdbg.hpp>
```

## What The Model Represents

The top-level type is `xdbg::document`. It contains:

- `files`: source file table
- `symbols`: globals, labels, sections, constants, objects, function symbols
- `functions`: named address ranges
- `lines`: address-to-source mappings
- `variables`: locals, parameters, and other scoped values

Important enums:

- `xdbg::language_kind`: `unknown`, `c`, `cxx`, `assembly`
- `xdbg::symbol_kind`: `function`, `global`, `local`, `parameter`, `label`, `section`, `type`, `constant`, `object`
- `xdbg::storage_kind`: `address`, `stack`, `register_name`, `register_pair`, `frame_relative`

This is meant to cover both C and assembly debug data in one linked
database.

## Reading And Writing `.xdbg`

The text format is line-oriented and versioned.

Minimal example:

```text
xdbg 1
image path="yos.rom"
entry address=0x100
file id=1 path="src/yos/kernel/syscall.c" language=c
symbol name="_main" kind=function address=0x100 file=1 line=12 language=c
function name="_main" start=0x100 end=0x140 file=1 line=12 return_type="int" language=c
line address=0x100 file=1 line=12 column=1
variable name="code" kind=parameter parent="_main" storage=register_pair register="hl" type="int" start=0x100 end=0x140 file=1 line=12 language=c
```

Supported record kinds today:

- `xdbg`
- `image`
- `entry`
- `file`
- `symbol`
- `function`
- `line`
- `variable`

Read from a file:

```cpp
#include <iostream>
#include <xdbg/xdbg.hpp>

int main() {
    xdbg::document doc = xdbg::read_file("yos.xdbg");
    for (const auto& function : doc.functions) {
        std::cout << function.name
                  << " @ 0x" << std::hex << function.start_address << "\n";
    }
}
```

Write a file:

```cpp
#include <xdbg/xdbg.hpp>

int main() {
    xdbg::document doc;
    doc.image_path = "yos.rom";
    doc.entry_address = 0x100;

    doc.files.push_back({
        .id = 1,
        .path = "src/yos/kernel/syscall.c",
        .language = xdbg::language_kind::c
    });

    doc.functions.push_back({
        .name = "_main",
        .start_address = 0x100,
        .end_address = 0x140,
        .file_id = 1,
        .line = 12,
        .column = 1,
        .return_type = "int",
        .language = xdbg::language_kind::c
    });

    xdbg::write_file("yos.xdbg", doc);
}
```

## Disassembly API

The disassembly layer is intentionally open-ended.

Core interfaces:

- `xdbg::memory_reader`
- `xdbg::disassembler`
- `xdbg::syntax_formatter`

Current factories:

- `xdbg::make_z80_disassembler()`
- `xdbg::make_native_formatter()`
- `xdbg::make_sdcc_z80_formatter()`

This lets you separate:

- how bytes are fetched
- how instructions are decoded
- how instructions are rendered

Sample:

```cpp
#include <iostream>
#include <vector>
#include <xdbg/xdbg.hpp>

int main() {
    std::vector<uint8_t> bytes = {0x3E, 0x42, 0xDD, 0x7E, 0x05, 0xC9};
    xdbg::vector_memory_reader memory(bytes);

    auto disassembler = xdbg::make_z80_disassembler();
    auto formatter = xdbg::make_sdcc_z80_formatter();

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

Errors derive from `xdbg::error`.

- `xdbg::error`: generic library/runtime failure
- `xdbg::parse_error`: malformed `.xdbg` input, including line number context

## Build

Build the library:

```sh
make -C lib/xdbg all
```

Run tests:

```sh
make -C lib/xdbg test
```

## Current Limits

- The library currently builds as a static archive, not `libxdbg.so`.
- The only built-in decoder today is Z80.
- The only built-in alternate syntax formatter today is SDCC-flavored Z80.

Those limits are deliberate. The API is meant to stay stable while more
CPU backends and emitters get added later.
