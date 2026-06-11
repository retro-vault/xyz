#
# SDCC Z80 setup for the C23 compatibility suite.
#
# This setup uses `sdcc -mz80` for compilation and `ucsim_z80` for
# execution. Runtime output is captured through the simulator interface
# (`simif`) mapped onto Z80 output port `0xff`.
#
# MIT License (see: LICENSE)
# Copyright (C) 2026 tomaz stih
#

# SDCC Z80 Setup

Use this setup with:

```sh
make matrix PROFILE=setups/sdcc-z80/profile-ucsim.json
```

Compile only:

```sh
make matrix PROFILE=setups/sdcc-z80/profile-ucsim.json RUN_MODE=never
```

## Files

- `profile-ucsim.json`
  Ready-to-use compiler profile for `sdcc -mz80` plus `ucsim_z80`.
- `bin/sdcc_z80_driver.sh`
  Two-step SDCC driver that compiles the suite case and the support
  shim separately, then links them.
- `bin/run_ucsim_z80.sh`
  Runs the linked `.ihx` image in `ucsim_z80`, captures simulator
  output, and forwards only program output to the suite runner.
- `support/simif_stdio.c`
  Minimal `putchar()` and `puts()` implementation that writes through
  `simif`.

## Notes

- This is a freestanding cross-compilation setup, so many hosted C23
  library tests will legitimately fail to compile or link.
- The runtime path is mainly intended for language and preprocessor
  cases that only need simple `puts()`-style output.
- The suite output for a passing runnable test is still the same
  `OK feature_name` line expected by the main runner.
