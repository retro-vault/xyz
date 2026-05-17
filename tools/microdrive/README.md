# microdrive

Host-side tool for creating, listing, and editing ZX Spectrum Microdrive (`.mdr`)
images.

## Build

From the repository root:

```bash
make -C tools/microdrive
```

The binaries are placed at:

- `bin/bin/tools/microdrive/microdrive`

## Usage

```bash
microdrive <command> [args]
```

Common commands:

- `create <image.mdr> <label>`: create a new image
- `dir <image.mdr>`: list directory
- `put <image.mdr> <file>`: add a file
- `get <image.mdr> <file>`: extract a file

Run `microdrive --help` for full usage details.
