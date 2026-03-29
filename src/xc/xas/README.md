# XAS - Z80 Assembler

Use **SDCC** for XYZ assembly language compilation.

SDCC includes the `as` assembler which processes `.s` assembly files into object code (`.rel` files).

- Official site: http://sdcc.sourceforge.net/
- Version required: 4.5 or later

## Assembly Language Support

The SDCC assembler (`as`) is a Z80-compatible assembler that handles:
- `.module` directives for section naming
- `.globl` directives for public symbols
- Area definitions (`.area _CODE`, `.area _DATA`, etc.)
- Instruction syntax compatible with Z80 ISA
- Macro support via SDCC preprocessor

## Building Assembly

Assembly files are compiled and linked as part of the standard XYZ build process:

```bash
make        # builds entire project including assembly files
make clean  # removes build artifacts
```

Compilation occurs inside a Docker container to ensure consistent SDCC versions.
