# YOS on Spectrum Next in ZEsarUX

This runner verifies the production universal YOS ROM as a ZEsarUX `TBBlue`
machine with exact Next machine ID 10. It uses fast 48-ROM compatibility mode
so the YOS replacement ROM owns `0x0000-0x3FFF`, while NextRegs and the MMU
remain available.

From the repository root, run the automated headless check:

```sh
python3 y/tests/zesarux-next/run.py --headless
```

The 128K phase uses ZEsarUX's installed `128.rom`: its editor ROM is retained
in slot 0 and YOS replaces the 48 BASIC slot that ESXIDE boots. The YOS 128K
mapper preserves that slot on every `0x7FFD` paging write. If the ROM is not
installed in ZEsarUX's normal data directory, pass its path with `--rom128`.

Headless mode first runs the libxz80 Next backend test, which initializes and
checks all 126 user heaps. It then starts the exact ROM in ZEsarUX, stops at
`_boot_shell` before disk I/O, and runs a fixed-RAM hardware probe through the
YOS mapper and RST20 far-call gate from logical bank 0 to bank 125. Finally it
cold-boots separate real 48K and 128K machines through divIDE and the ESXIDE
firmware. Both must load `shell.sys` and render the shell; the
128K run must additionally identify model 128 and expose all six user banks.

The runner creates a private 128 MiB FAT16 HDF below
`build/yos-zesarux-next/`, copies the vendored esxDOS `SYS`, `BIN`, and `TMP`
directories plus `shell.sys`, and starts ZEsarUX without
loading or saving the user's configuration. It also clones the HDF container
to the raw `.ide` form used by the ZEsarUX IDE-card runs. IDE writes are
nonpersistent. Logs, ZRCP transcripts, both media formats, and `result.json`
remain in the printed run directory.

For a visible window, omit `--headless`:

```sh
python3 y/tests/zesarux-next/run.py
```

Visible mode launches the TBBlue banking configuration. The automated
`--headless` mode is required for the separate 48K and 128K real-divIDE
assertions. `--prepare-only` creates both media formats and prints the full
TBBlue launch command without starting ZEsarUX. `--esxdos DIR` overrides the
vendored runtime when testing another release.
