# XCC - Z80 C Compiler

Use **SDCC** for XYZ compilation.

SDCC is the Small Device C Compiler, which provides Z80/8085/6502 and many other processor targets.

- Official site: http://sdcc.sourceforge.net/
- Version required: 4.5 or later (uses register-based calling convention)

## Building with SDCC

Compilation for XYZ is handled by the build system. Use the standard `make` command from the project root.

```bash
make        # builds entire project including SDCC compilation
make clean  # removes build artifacts
```

The SDCC compiler is invoked inside a Docker container to ensure consistent toolchain versions across platforms.
