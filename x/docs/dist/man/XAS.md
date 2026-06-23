# xas — assembler

Z80 assembler. Accepts SDCC `sdasz80` syntax (default) or GNU `gas`
syntax, producing SDCC `.rel` or ELF32 objects respectively.

## Synopsis

```bash
xas [options] <input.s>
```

## Common usage

```bash
# Assemble to a .rel object (SDCC dialect, default)
xas --mode=sdcc startup.s -o startup.rel

# Assemble GNU-syntax source to ELF32
xas --mode=gnu startup.s -o startup.o

# Assemble with debug information
xas --mode=sdcc -g main.s -o main.rel
```

## Options

| Option | Meaning |
|---|---|
| `--mode=sdcc` | SDCC sdasz80 directives, `.rel` output (default) |
| `--mode=gnu` | GNU gas directives, ELF32 output |
| `--format=sdcc` | Pretty-print / emit SDCC-style assembly text |
| `--format=gnu` | Pretty-print / emit GNU-style assembly text |
| `-o <file>` | Output file |
| `-g` | Emit debug information |
| `-I <dir>` | Add include directory |
| `-D <sym[=v]>` | Define preprocessor symbol |

You rarely need to call xas directly: `xcc` invokes it automatically.
Use it directly when assembling hand-written `.s` files outside a C
build, or when converting between the two assembly dialects with
`--format=`.
