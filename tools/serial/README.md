# serial

A Linux host tool for transferring binary files to and from a ZX Spectrum over a serial (RS-232) connection.

## Protocol

The ZX Spectrum side must be running a compatible receiver/sender program.
Communication uses **2400 baud, 8N1, hardware RTS/CTS flow control**.

Each transfer starts with a fixed header:

| Field      | Size   | Direction  | Description                        |
|------------|--------|------------|------------------------------------|
| `type`     | 1 byte | both       | Block type. `3` = CODE block.      |
| `data_len` | 2 bytes| both       | Payload length in bytes.           |
| `par1`     | 2 bytes| both       | Load address (CODE blocks).        |
| `par2`     | 2 bytes| both       | `0xFFFF` for CODE blocks.          |
| `reserved` | 2 bytes| **send only** | Always `0`. Required by the ZX receiver. |

The payload follows immediately after the header.

The `reserved` field is present only in the **put** direction (PC → Spectrum).
When **receiving** (Spectrum → PC) the header is 7 bytes (no `reserved` word).

## Build

From the repository root:

```
make
```

The binary is placed at `bin/bin/tools/serial/serial`.

## Usage

### Send a file to the Spectrum

```
serial put <device> <file> <addr>
```

| Argument  | Description                                    |
|-----------|------------------------------------------------|
| `device`  | Serial port, e.g. `/dev/ttyUSB0`               |
| `file`    | Binary file to send                            |
| `addr`    | Load address in decimal, e.g. `32768` (0x8000) |

Example:

```
serial put /dev/ttyUSB0 yos.rom 0
```

### Receive a file from the Spectrum

```
serial get <device> <file>
```

| Argument  | Description                              |
|-----------|------------------------------------------|
| `device`  | Serial port, e.g. `/dev/ttyUSB0`         |
| `file`    | Output file to write the received data   |

Example:

```
serial get /dev/ttyUSB0 snapshot.bin
```

## Notes

- The baud rate is fixed at **2400**. The ZX Spectrum's serial interface is slow; higher rates are unreliable.
- RTS/CTS hardware handshaking is enabled. Make sure your cable and adapter support it.
- The tool blocks until the full transfer completes.
