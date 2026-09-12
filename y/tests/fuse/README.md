# YOS in Fuse

From the repository root, with an extracted esxDOS 0.8.9 distribution:

```sh
python3 y/tests/fuse/run.py --esxdos build/yos-fuse/esxdos089
```

Requires the staged X toolchain, Fuse with a graphical display, hdfmonkey,
a host C compiler and the libspectrum development headers/library. Firmware
is user-supplied, not bundled. The distribution must contain `ESXIDE.BIN`
and its matching `SYS`, `BIN` and `TMP` directories; keep `AutoBoot=0` in
the stock configuration.

The runner builds the current ROM, shell and test library, then creates a
new FAT16 HDF and SZX under `build/yos-fuse/run-*/`. Existing emulator media
and Fuse settings are not overwritten. `--prepare-only` prepares these
artifacts without opening the window.

The snapshot only supplies the ROM/EPROM and pristine machine state: CPU
PC starts at zero, interrupts are disabled, and Spectrum/divIDE RAM is
zeroed. It contains no preloaded process, successful screen or saved BASIC
workspace. Actual esxDOS boots, opens both files, and supplies disk reads
to the production kernel. Fuse 1.6.0 has no separate divIDE EPROM path flag,
which is why the runner uses this cold snapshot.

Within a few seconds expect the centred greeting
`Alto (c) 2026 Wischner Labs Ltd.` and `Library OK`. This confirms that the
scheduled shell loaded the library, ran its relocated initializer,
queried/called its registered interface and rendered the returned string.
It is the current shell smoke screen, not an interactive command prompt.

The kernel must disable preemption for each firmware call: divIDE pages
out the ROM containing the IM2 scheduler. Leaving IM2 enabled during a
library disk read previously jumped into firmware instead of the scheduler
and restarted YOS before it could draw. The kernel regression suite checks
that gates reject interrupts and preserve enclosing critical sections.
It also runs the exact production ROM with per-thread error checks, forced
concurrent library loads, descriptor/append transaction audits and independent
GPX contexts. The visible smoke screen remains unchanged by synchronization.
