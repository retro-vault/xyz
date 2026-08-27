# Amstrad CPC target guide

## Targets

| Platform | Medium | Files |
|---|---|---|
| `cpc-464` | CDT cassette | Console only |
| `cpc-664` | AMSDOS DSK | ROM-backed disk streams |
| `cpc-6128` | AMSDOS DSK | ROM-backed disk streams |

Programs link and load at `0x4000`, use the CPC firmware console, keyboard,
and 300 Hz clock, and return to BASIC after `main()`.

## Cassette

```sh
xcc -Os --platform=cpc-464 --oformat=binary main.c -o app.bin
xprog --cdt app.bin --name APP -o app.cdt
```

Insert the CDT and enter `RUN"!APP"`. CDT output uses CPC firmware binary
header/data records. The default load and entry address is `0x4000`.

## Disk

```sh
xcc -Os --platform=cpc-6128 --oformat=binary main.c -o app.bin
xprog --dsk app.bin --name APP.BIN -o app.dsk
```

Use `--platform=cpc-664` for a CPC 664. Insert the DSK and enter `RUN"APP"`.
The image is a standard 40-track single-sided CPCEMU/AMSDOS data disk.

The disk targets support `open`/`read`/`write`/`close`, input `lseek`,
`rename`/`remove`, and the corresponding stdio APIs through ROM operations.
The firmware permits one input descriptor (3) and one output descriptor (4).
AMSDOS output is sequential, so update/append modes and output seeking fail.
The cassette-only 464 omits the AMSDOS buffers and returns failure for file
operations while retaining the common archive-granular libc.
