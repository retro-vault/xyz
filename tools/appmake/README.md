# `appmake`

`appmake` is a small command-style helper for ZX Spectrum tape files and legacy `yos` `.app` images.

## Commands

- `list`: list the contents of a `.tap` or `.tzx` tape image
- `analyze`: start from the BASIC `USR` entry and print loaded blocks, reachable code ranges, and annotated ROM/SYSVAR dependencies
- `make`: extract the main tape `CODE` block, wrap it as a legacy `yos` `.app`, and write it straight to a microdrive image
- `tap`: extract a ROM-style `CODE` block from a `.tap` file and emit a legacy `.app`
- `sna`: import a 48K `.sna` snapshot as a trimmed high-memory legacy `.app`

## Output

The tool writes a `.app` file with:

- legacy signature `YAPZ`
- `APP_FLAG_LEGACY_ZX`
- absolute load address
- absolute entry address
- optional register state for snapshot imports

The exact format is documented in:

- [`src/yos/docs/11_app_format.md`](../../src/yos/docs/11_app_format.md)

## Usage

### List tape contents

```bash
bin/bin/tools/appmake/appmake list tests/tapes/manic.tap
bin/bin/tools/appmake/appmake list tests/tapes/fist.tzx
```

### Analyze program start and dependencies

```bash
bin/bin/tools/appmake/appmake analyze tests/tapes/manic.tap
```

### Build and write a runnable `.app` to microdrive

```bash
bin/bin/tools/appmake/appmake make tests/tapes/manic.tap tests/microdrives/hello.mdr --app manic.app
bin/bin/tools/appmake/appmake make tests/tapes/fist.tzx tests/microdrives/hello.mdr --app fist.app
```

Defaults:

- `--app`: defaults to a 6-character lowercase stem plus `.app`, so it can be launched from `ysh`
- `--start`: defaults to BASIC `USR` when present, otherwise the code block load address
- `--sp`: defaults to BASIC `CLEAR` when present, otherwise `0`
- `--name`: picks a specific `CODE` block if the tape contains more than one
- `--cart`: only used when creating a new `.mdr` image

### Convert tape CODE block

```bash
bin/bin/tools/appmake/appmake tap input.tap output.app
bin/bin/tools/appmake/appmake tap input.tap output.app --name LOADER
bin/bin/tools/appmake/appmake tap input.tap output.app --load 0x8000 --start 0x8000
```

Defaults:

- `load_addr`: taken from the TAP header
- `entry_addr`: defaults to `load_addr`

### Convert 48K snapshot

```bash
bin/bin/tools/appmake/appmake sna input.sna output.app --load 0x8000
bin/bin/tools/appmake/appmake sna input.sna output.app --load 0x9000 --start 0x9200
```

For `.sna`:

- `--load` is required
- `--start` defaults to the restored PC recovered from the snapshot stack
- `--sp` defaults to the restored SP after the saved PC has been popped

## Notes

- `list` supports `.tap` and `.tzx` containers.
- `analyze` currently supports `.tap` files.
- `make` supports ROM-style `CODE` blocks from both `.tap` and `.tzx`.
- TZX listing currently understands the common Spectrum block types needed to inspect tape structure and file contents.
- `analyze` uses a vendored single-header Z80 core from `kosarev/z80` under MIT license.
- `analyze` annotates ROM routine and system variable references using the Spectrum ROM reference maps from SkoolKid.
- `make` carries the BASIC-derived entry point and stack pointer into the legacy app header so `yos` can start the imported program with Spectrum-style `USR` / `CLEAR` semantics.
- Only standard ROM-style TAP `CODE` headers are supported for `tap` conversion.
- If a TAP file contains multiple `CODE` blocks, the first one is used unless `--name` selects a specific block.
- Only **48K** `.sna` files are supported right now.
- The tool does not know the exact in-RAM `yos` layout, so you must choose a safe high-memory `--load` address for snapshot imports.
