# Application Images (`.app`)

This chapter describes how `yos` treats application files on disk today, and defines the next-step `.app` container for importing **legacy ZX Spectrum binaries**.

## Current State

Today, the `yos` process loader in [kernel/process.c](/home/tstih/data/retro-vault/xyz/src/yos/kernel/process.c:1) supports:

- relocatable **XL** images directly
- legacy `YAPZ` tape-code apps with absolute load and jump addresses

In other words, `.app` on disk may now be either:

- a native `XL` image copied with an `.app` extension
- a legacy Spectrum `YAPZ` container produced by `appmake`

## Why Extend The Format

We also want `.app` to carry **legacy ZX Spectrum applications** that were not linked for `yos` and therefore cannot be relocated by the XL loader.

Those legacy imports need:

- a fixed absolute load address
- a fixed jump address
- a way to identify that the payload is a legacy Spectrum program
- an optional saved CPU state for snapshot-style launches

## Container Overview

The long-term `.app` container uses a small common header and then stores the payload bytes directly.

Two signatures are reserved:

- `YAPP` - native `yos` application container
- `YAPZ` - legacy ZX Spectrum application container

The legacy form intentionally uses a **slightly different signature** so the loader can reject or route it early, even before it looks at flags.

All multi-byte fields are little-endian.

## Common Header

### Header Layout - 24 bytes

| Offset | Size | Field | Meaning |
|---|---:|---|---|
| 0 | 4 | `magic` | `YAPP` for native apps, `YAPZ` for legacy ZX apps |
| 4 | 1 | `version` | format version, currently `0x01` |
| 5 | 1 | `kind` | payload kind, see below |
| 6 | 1 | `flags` | capability bits, see below |
| 7 | 1 | `header_size` | currently `24` |
| 8 | 2 | `load_addr` | absolute address where payload bytes must be copied |
| 10 | 2 | `entry_addr` | absolute address where execution starts |
| 12 | 2 | `payload_size` | payload bytes that follow the optional state block |
| 14 | 2 | `state_size` | optional machine-state block size |
| 16 | 2 | `stack_ptr` | initial SP for snapshot-style launches, else `0` |
| 18 | 2 | reserved | must be `0` |
| 20 | 4 | reserved | must be `0` |

The file layout is:

```text
+-------------------+
| app header        |
+-------------------+
| state block       |  state_size bytes, optional
+-------------------+
| payload           |  payload_size bytes
+-------------------+
```

## Flags

| Bit | Name | Meaning |
|---|---|---|
| `0x01` | `APP_FLAG_RELOCATABLE` | payload is relocatable and not tied to `load_addr` |
| `0x02` | `APP_FLAG_LEGACY_ZX` | payload is a legacy ZX Spectrum application |
| `0x04` | `APP_FLAG_HAS_STATE` | a machine-state block is present |
| `0x08` | `APP_FLAG_ABSOLUTE_LOAD` | payload must be copied to `load_addr` exactly |

For the new legacy imports:

- `APP_FLAG_LEGACY_ZX` must be set
- `APP_FLAG_ABSOLUTE_LOAD` must be set
- `APP_FLAG_RELOCATABLE` must be clear

## Payload Kinds

| Value | Name | Meaning |
|---:|---|---|
| `0` | `APP_KIND_NATIVE_XL` | native `yos` payload, typically XL-derived |
| `1` | `APP_KIND_ZX_TAPE_CODE` | legacy Spectrum tape `CODE` block |
| `2` | `APP_KIND_ZX_SNAPSHOT_48K` | trimmed 48K snapshot with register state |

Kinds `1` and `2` are the first legacy formats defined here.

## Legacy Tape App

This is the simpler import path.

The source is a legacy tape image such as a `.tap` file containing a `CODE` block. The converter extracts the machine-code bytes and writes them as a non-relocatable `.app`.

### Rules

- `magic = YAPZ`
- `kind = APP_KIND_ZX_TAPE_CODE`
- `flags = APP_FLAG_LEGACY_ZX | APP_FLAG_ABSOLUTE_LOAD`
- `load_addr` is the absolute destination address
- `entry_addr` is the absolute jump target
- `state_size = 0`
- `stack_ptr` may optionally carry the Spectrum stack pointer derived from BASIC `CLEAR`

The loader copies `payload_size` bytes to `load_addr`, optionally restores `SP` from `stack_ptr`, and jumps to `entry_addr`.

This format is appropriate when:

- the tape contains a single machine-code block
- execution can start from a known absolute address
- no full CPU register restore is required

## Legacy Snapshot App

This is the richer import path for a previously running ZX Spectrum program.

The source is a snapshot image. The initial implementation targets **48K `.sna`** files because they provide a plain 48 KB RAM dump plus register state.

Unlike a full emulator snapshot, the `yos` `.app` form is intentionally **shortened**:

- ROM is never included
- low RAM that conflicts with `yos` data/BSS is omitted
- only the high-memory tail beginning at `load_addr` is stored

### Rules

- `magic = YAPZ`
- `kind = APP_KIND_ZX_SNAPSHOT_48K`
- `flags = APP_FLAG_LEGACY_ZX | APP_FLAG_HAS_STATE | APP_FLAG_ABSOLUTE_LOAD`
- `load_addr` is the first preserved RAM address
- `entry_addr` is the restored PC / jump address
- `stack_ptr` is the initial SP to restore before jumping
- `state_size` is the size of the register block

The loader copies the payload back to `[load_addr, load_addr + payload_size)`, restores CPU state from the state block, restores `SP`, and then jumps to `entry_addr`.

### Important Limitation

Because the snapshot is trimmed from the bottom, the preserved code and stack must already live high enough in RAM to avoid overlapping `yos` runtime memory.

In practice this means:

- `load_addr` must be chosen above OS-owned RAM
- `entry_addr` should point into the preserved payload area
- `stack_ptr` should also remain in preserved RAM

If any of those still point into omitted low memory, the imported app is not safely resumable.

## Snapshot State Block

For `APP_KIND_ZX_SNAPSHOT_48K`, the state block is currently a fixed 32-byte register record:

| Offset | Size | Field |
|---|---:|---|
| 0 | 2 | `af` |
| 2 | 2 | `bc` |
| 4 | 2 | `de` |
| 6 | 2 | `hl` |
| 8 | 2 | `af_alt` |
| 10 | 2 | `bc_alt` |
| 12 | 2 | `de_alt` |
| 14 | 2 | `hl_alt` |
| 16 | 2 | `ix` |
| 18 | 2 | `iy` |
| 20 | 2 | `sp` |
| 22 | 1 | `i` |
| 23 | 1 | `r` |
| 24 | 1 | `iff2` |
| 25 | 1 | `im` |
| 26 | 1 | `border` |
| 27 | 5 | reserved |

`entry_addr` stores the restored PC separately, so the state block does not duplicate it.

## Suggested Loader Strategy

When `yos` grows support for this container, the loader should:

1. read the first 4 bytes
2. if the signature is `XL`, keep the current relocatable path
3. if the signature is `YAPZ`, use the legacy absolute-load path
4. reject any `load_addr` range that collides with OS memory, data, BSS, or critical resident services

The current implementation already does this for `APP_KIND_ZX_TAPE_CODE`. Snapshot-style state restore remains future work.

## Host Tool

The host-side converter for the legacy forms lives in:

- [`pkg/appmake`](../../pkg/appmake)

It currently supports:

- `.tap` `CODE` block to `APP_KIND_ZX_TAPE_CODE`
- 48K `.sna` snapshot to `APP_KIND_ZX_SNAPSHOT_48K`

## Status

This chapter defines the on-disk format and the current loader behavior.

Today:

- `XL` apps are loadable as before
- legacy `YAPZ` tape-code apps are loadable from `ysh`
- snapshot-style `YAPZ` apps are still only a host-side format contract
