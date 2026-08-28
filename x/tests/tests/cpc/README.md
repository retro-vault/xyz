# Amstrad CPC real-ROM validation

`run_mcp.py` builds `smoke.c` for `cpc-464`, `cpc-664`, and `cpc-6128`,
packages the binaries as their real delivery media, and boots them with
`amstrad-cpc-mcp` and model-correct ROMs.

```sh
python3 x/tests/tests/cpc/run_mcp.py \
  --mcp /path/to/amstrad-cpc-mcp \
  --roms /path/to/roms
```

The ROM directory must contain `cpc464-os.rom`, `cpc464-basic.rom`, the
equivalent 664 and 6128 pairs, and `amsdos.rom`. With the MCP repository next
to this repository, its staged executable and ROM directory are found by
default.

The 464 run loads a generated CDT. The 664 and 6128 runs load independent
writable DSK images and exercise raw and stdio AMSDOS operations in addition
to the shared libc, clock, and console checks. Success produces three `PASS`
lines ending in marker `0xA5`.

This is an explicit platform/release validation and is not part of the
manifest-driven regression pack. Direct `run_mcp.py` invocation is strict and
reports missing assets as an error.
