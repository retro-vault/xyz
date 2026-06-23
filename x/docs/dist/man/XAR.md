# xar — archiver

Creates and maintains static libraries of Z80 objects, in SDCC
text-index `.lib` format (default) or GNU `ar` format.

## Synopsis

```bash
xar [--mode=sdcc|gnu] <operation>[modifiers] <archive> [members...]
```

## Common usage

```bash
# Create (or update) a library from objects
xar rcs libfoo.a foo.rel bar.rel

# List the members of a library
xar t libfoo.a

# Extract all members
xar x libfoo.a

# Delete a member
xar d libfoo.a bar.rel
```

## Operations and modifiers

| Letter | Meaning |
|---|---|
| `r` | Add or replace members |
| `t` | List archive contents |
| `x` | Extract members (all if none specified) |
| `d` | Delete members |
| `c` | (modifier) Create archive, suppress warning |
| `v` | (modifier) Verbose |
| `s` | (modifier) Write symbol index (reserved, no-op) |

## Options

| Option | Meaning |
|---|---|
| `--mode=sdcc` | Text-index `.lib` format (default) |
| `--mode=gnu` | GNU `ar` binary format |

Libraries built with xar can be passed to `xld` (or to `xcc` on the
command line) like any object file; only the members needed to resolve
symbols are linked in.
