# X Compiler Suite Features

The X Compiler Suite is a Z80 development suite: C compiler, assembler, linker, optimizer,
archiver, object converter, debugger, runtime, libc, and platform libraries.
The tools live under `x/src/`; target headers and libraries live under
`x/libc/`, `x/runtime/`, `x/platforms/`, and the host-side support libraries in
`x/lib/`.

Together they give you a **C23-oriented** toolchain for bare metal, CP/M,
Spectrum-style targets, or your own board. The compiler defaults to ISO C23 and
ships a matching standard library in hand-tuned Z80 assembly.

This document is a programmer's guide. Each section is meant to stand on its
own — read the one that matches what you are trying to do. **§1–§24** cover
toolchain mechanics; **§25–§42** cover C23 language and library features;
**§43 onward** cover advanced emulator, SDK, optimization, and integration
features. **§34–§42** are the `[[attribute]]` catalogue split into smaller
daily-sized feature entries with generated-assembly examples.

---

<div class="article-toc">

<p class="article-toc__title">Contents</p>

<p class="article-toc__part">Compiler Fundamentals</p>

| Section | What it covers |
|---|---|
| [§1. Standard C23](#1-standard-c23) | Build normal C23 programs with the full `xcc` → `xas` → `xld` pipeline. |
| [§2. Predefined Compiler Symbols](#2-predefined-compiler-symbols) | See the `__XCC__`, `__STDC_VERSION__`, integer model, and float-format macros. |
| [§3. Compiler Pragmas](#3-compiler-pragmas) | Use `#pragma once` and GNU-style diagnostic control. |

<p class="article-toc__part">Objects, Linking And Debug</p>

| Section | What it covers |
|---|---|
| [§4. One Toolchain, Two Object Worlds](#4-one-toolchain-two-object-worlds) | SDCC `.rel` / `.lib` and GNU ELF / `.a` modes in one suite. |
| [§5. Assembly Dialect Translation](#5-assembly-dialect-translation) | Convert SDCC-style assembly to GNU style, and back again. |
| [§6. Dual Debug Model](#6-dual-debug-model) | CDB for SDCC-style builds and DWARF2 for ELF builds. |
| [§7. Integrated Z80 Debugger](#7-integrated-z80-debugger) | `xgdb`, `xemu`, source stepping, breakpoints, and emulator attach. |
| [§8. Relocatable XL Output](#8-relocatable-xl-output) | Native XL images with relocation tables for loaders and OSes. |
| [§9. Reserved Address Ranges In The Linker](#9-reserved-address-ranges-in-the-linker) | Keep memory holes, MMIO, ROM, stacks, and firmware areas out of allocations. |
| [§10. Multiple Final Image Formats](#10-multiple-final-image-formats) | Emit XL, raw binary, ELF, Intel HEX, and other deployment formats. |

<p class="article-toc__part">Optimization, Runtime, And Platforms</p>

| Section | What it covers |
|---|---|
| [§11. Standalone Z80 Optimizer](#11-standalone-z80-optimizer) | Run `xopt` independently on one or many assembly files. |
| [§12. Stable Speed And Manual Whole-Module Optimization](#12-stable-speed-and-manual-whole-module-optimization) | `xcc -Of` is stable and `-O3` is its empty experimental alias; aggressive `xopt --cross-file` use stays explicit. |
| [§13. Selectable Float Representation](#13-selectable-float-representation) | Choose IEEE `float` or fixed 8.8, 16.16, and 24.8 formats. |
| [§14. 64-Bit Integer And Double Runtime Support](#14-64-bit-integer-and-double-runtime-support) | `long long`, software `double`, and S/M/L libc feature switches. |
| [§15. Retargetable Platform Model](#15-retargetable-platform-model) | Add new boards, ROMs, CP/M-like targets, or bare-metal platforms. |
| [§16. Self-Contained Relocatable Installation](#16-self-contained-relocatable-installation) | Install once; tools find headers, libraries, and support files by prefix. |

<p class="article-toc__part">Build, Link, And Distribution</p>

| Section | What it covers |
|---|---|
| [§17. GNU-Style Driver Behavior](#17-gnu-style-driver-behavior) | Familiar GCC-style switches for build systems and muscle memory. |
| [§18. Classic CMake And Make Integration](#18-classic-cmake-and-make-integration) | Use the X Compiler Suite from ordinary Makefiles and CMake projects. |
| [§19. Object And Archive Conversion](#19-object-and-archive-conversion) | Convert object formats when moving between SDCC and GNU ecosystems. |
| [§20. Archive Compatibility](#20-archive-compatibility) | Work with `.lib` and `.a` archives. |
| [§21. Linker-Script Compatibility](#21-linker-script-compatibility) | Use familiar script constructs for memory layout. |
| [§22. Automatic Runtime And Library Discovery](#22-automatic-runtime-and-library-discovery) | Let the driver locate `crt0`, libc, runtime, and platform libraries. |
| [§23. Useful Bare-Metal Defaults](#23-useful-bare-metal-defaults) | Sensible defaults for tiny systems and freestanding builds. |
| [§24. One Suite For The Whole Pipeline](#24-one-suite-for-the-whole-pipeline) | Compile, optimize, assemble, link, and debug with one coherent toolset. |

<p class="article-toc__part">C23 Language And Library</p>

| Section | What it covers |
|---|---|
| [§25. C23 Language Support In The Compiler](#25-c23-language-support-in-the-compiler) | Modern C syntax, attributes, `_BitInt`, `nullptr`, `typeof`, and more. |
| [§26. C23 Standard Library In Hand-Written Assembler](#26-c23-standard-library-in-hand-written-assembler) | Small Z80-native libc surface with C23 headers. |
| [§27. Atomics On Z80](#27-atomics-on-z80-stdatomich) | `<stdatomic.h>` on an 8-bit CPU. |
| [§28. `stdbit.h` And `stdckdint.h`](#28-stdbith-and-stdckdinth) | Bit utilities and checked integer arithmetic. |
| [§29. UTF-8 `char8_t` And The Unicode Layer](#29-utf-8-char8_t-and-the-unicode-layer) | C23 text types and conversion support. |
| [§30. C23 Math And IEC 60559 Extras](#30-c23-math-and-iec-60559-extras) | New math entry points, classification, ordering, and payload helpers. |
| [§31. Register Calling Conventions](#31-register-calling-conventions) | SDCC-compatible register ABI and stack ABI interop. |
| [§32. `_Complex` Numbers And Inline Assembly](#32-_complex-numbers-and-inline-assembly) | Complex arithmetic plus inline Z80 assembly. |
| [§33. Aggressive Compile-Time Evaluation](#33-aggressive-compile-time-evaluation--o2-and-up) | Constant folding and function evaluation at `-O2` and above. |
| [§34. `[[attributes]]` Overview And Standard C23 Attributes](#34-attributes-overview-and-standard-c23-attributes) | Syntax, unknown-attribute behavior, and standard C23 attributes such as `[[noreturn]]`. |
| [§35. `[[sdcc::sdccall(n)]]` ABI Selection](#35-sdccsdcccalln-abi-selection) | Pick stack or register ABI explicitly for one function. |
| [§36. `[[sdcc::naked]]`, `[[sdcc::interrupt]]`, And `[[sdcc::critical]]`](#36-sdccnaked-sdccinterrupt-and-sdcccritical) | Raw entry stubs, full ISRs, and interrupt-masked functions. |
| [§37. `[[sdcc::at(addr)]]` Absolute Variables](#37-sdccataddr-absolute-variables) | Bind a C object name to a fixed memory address. |
| [§38. `[[sdcc::sfr(port)]]` Port-Mapped Variables](#38-sdccsfrport-port-mapped-variables) | Map C reads and writes onto Z80 `in` / `out`. |
| [§39. `[[xcc::far]]` 24-Bit Pointers](#39-xccfar-24-bit-pointers) | Banked pointers with 16-bit address plus 8-bit bank byte. |
| [§40. `[[xcc::bank(n)]]` Banked Code And Data Placement](#40-xccbankn-banked-code-and-data-placement) | Put functions and static-storage objects into `_CODE_BANK_n` / `_DATA_BANK_n`. |
| [§41. `[[z88dk::…]]` Alternate Call Conventions](#41-z88dk-alternate-call-conventions) | z88dk-compatible fastcall and callee-cleanup interop. |
| [§42. Attribute Rules And Interactions](#42-attribute-rules-and-interactions) | Variadics, imported ABIs, atomics, and other cross-attribute rules. |

<p class="article-toc__part">Advanced Tooling And Integration</p>

| Section | What it covers |
|---|---|
| [§43. Bank-Switched Memory Emulation](#43-bank-switched-memory-emulation) | `xemu` stores/selectors/windows/port-rules for paged memory and far pointers. |
| [§44. Headless Emulator And Console Port Binding](#44-headless-emulator-and-console-port-binding) | Run binaries directly, wire Z80 ports to host I/O, or expose `xemu` as an RSP target. |
| [§45. Embeddable Host SDK Libraries](#45-embeddable-host-sdk-libraries) | Reuse `libxemu`, `libxgdb`, `libxopt`, `libxz80`, `librsp`, and `libxbfd` in your own tools. |
| [§46. Fine-Grained Optimization Switches](#46-fine-grained-optimization-switches) | Override individual optimizer passes with `-f...` and `-fno-...`. |
| [§47. GNU Compatibility Extras](#47-gnu-compatibility-extras) | Statement expressions, `typeof_unqual`, GNU builtins, and compatibility parsing. |
| [§48. GCC-Style Warning Surface](#48-gcc-style-warning-surface) | Command-line warning control that matches the pragma groups in `xcc`. |

</div>

# Compiler Fundamentals

## 1. Standard C23

**What:** A normal C toolchain for Z80. `xcc` defaults to **ISO C23**
(`__STDC_VERSION__` is `202311L`). You write modern C; the driver runs the full
pipeline.

**When:** Any time you want a complete program without hand-wiring every object
file. This is the default way to use the suite.

**How it works:** `xcc` preprocesses and compiles C to assembly, calls `xas` to
assemble, then calls `xld` to link. Unless you pass `-nostdlib`, the linker
pulls in `crt0`, libc, compiler runtime, and the platform library for
`--platform` (default `none`). Output defaults to a relocatable XL image.

```
# One-step build
xcc hello.c -o hello.xl

# Several translation units, mixed C and assembly
xcc main.c util.c startup.s -o app.xl

# Stop after compile+assemble (Make/CMake pipelines)
xcc -c main.c              # → main.rel
xcc -S util.c -o util.s    # → assembly only

# Release build
xcc -Os main.c util.c -o app.xl
```

**You get:** Common headers from `<prefix>/z80/include`, selected-platform
headers from `<prefix>/z80/include/<platform>`, and libraries from
`<prefix>/z80/lib`. The driver finds these relative to the install prefix — no
wrapper scripts required.

**Skip the runtime** when you are writing a freestanding loader or test harness:

```
xcc -nostdlib -nostartfiles boot.s -o boot.bin --oformat=binary
```

---

## 2. Predefined Compiler Symbols

**What:** `xcc` predefines C23, Z80 data-model, compiler identity, GNU
compatibility, and selected floating-point ABI macros so portable code can test
the compilation environment without custom build scripts.

**When:** Use these in headers, portability shims, low-level libraries, and
tests that need to know the compiler, integer widths, pointer size, or selected
`float` representation.

**How it works:** The preprocessor always defines the core symbols below.
The driver additionally injects `__XCC_FLOAT_*` macros according to
`--float-format=`. User `-Dname=value` macros are applied afterwards and can
add project-specific switches.

| Group | Symbols |
|---|---|
| Standard C | `__STDC__=1`, `__STDC_HOSTED__=1`, `__STDC_VERSION__=202311L` |
| Compiler identity | `__XCC__=1`, `__xcc__=1`, `__VERSION__="xcc"` |
| GNU compatibility face | `__GNUC__=4`, `__GNUC_MINOR__=2`, `__GNUC_PATCHLEVEL__=1` |
| Source location | `__FILE__`, `__LINE__`, `__DATE__`, `__TIME__`, `__COUNTER__` |
| C23 preprocessor predicates | `__has_include(...)`, `__has_c_attribute(...)` |
| Character model | `__CHAR_BIT__=8`, `__CHAR_WIDTH__=8`, `__SCHAR_MAX__=127`, `__UCHAR_MAX__=255` |
| Integer widths | `__SHRT_WIDTH__=16`, `__INT_WIDTH__=16`, `__LONG_WIDTH__=32`, `__LONG_LONG_WIDTH__=64`, `__LLONG_WIDTH__=64`, `__INTMAX_WIDTH__=64` |
| Integer maximums | `__SHRT_MAX__=32767`, `__INT_MAX__=32767`, `__LONG_MAX__=2147483647L`, `__LONG_LONG_MAX__=9223372036854775807LL` |
| Fixed-width maximums | `__INT8_MAX__=127`, `__INT16_MAX__=32767`, `__INT32_MAX__=2147483647L`, `__INT64_MAX__=9223372036854775807LL` |
| Unsigned maximums | `__UINT8_MAX__=255`, `__UINT16_MAX__=65535U`, `__UINT32_MAX__=4294967295UL`, `__UINT64_MAX__=18446744073709551615ULL` |
| C max types | `__INTMAX_MAX__=9223372036854775807LL`, `__UINTMAX_MAX__=18446744073709551615ULL` |
| Type sizes | `__SIZEOF_SHORT__=2`, `__SIZEOF_INT__=2`, `__SIZEOF_LONG__=4`, `__SIZEOF_LONG_LONG__=8`, `__SIZEOF_POINTER__=2` |
| C ABI typedef spellings | `__SIZE_TYPE__=unsigned int`, `__PTRDIFF_TYPE__=int`, `__WCHAR_TYPE__=int` |
| ABI maximums | `__UINT_MAX__=65535U`, `__ULONG_MAX__=4294967295UL`, `__SIZE_MAX__=65535U`, `__PTRDIFF_MAX__=32767`, `__WCHAR_MAX__=32767` |
| Unicode capability | `__STDC_UTF_16__=1`, `__STDC_UTF_32__=1` |

Floating-point format macros:

| `--float-format=` | Symbols |
|---|---|
| `ieee32` | `__XCC_FLOAT_FORMAT_IEEE32=1`, `__XCC_FLOAT_FORMAT__=0`, `__XCC_FLOAT_SIZE__=4`, `__XCC_FLOAT_FRACTION_BITS__=0` |
| `fixed8_8` | `__XCC_FLOAT_FORMAT_FIXED8_8=1`, `__XCC_FLOAT_FORMAT__=1`, `__XCC_FLOAT_SIZE__=2`, `__XCC_FLOAT_FRACTION_BITS__=8` |
| `fixed16_16` | `__XCC_FLOAT_FORMAT_FIXED16_16=1`, `__XCC_FLOAT_FORMAT__=2`, `__XCC_FLOAT_SIZE__=4`, `__XCC_FLOAT_FRACTION_BITS__=16` |
| `fixed24_8` | `__XCC_FLOAT_FORMAT_FIXED24_8=1`, `__XCC_FLOAT_FORMAT__=3`, `__XCC_FLOAT_SIZE__=4`, `__XCC_FLOAT_FRACTION_BITS__=8` |
| `ieee16` | `__XCC_FLOAT_FORMAT_IEEE16=1`, `__XCC_FLOAT_FORMAT__=4`, `__XCC_FLOAT_SIZE__=2`, `__XCC_FLOAT_FRACTION_BITS__=0` |

```
#include <math.h>

#if defined(__XCC__) && __STDC_VERSION__ >= 202311L
#define HAVE_XCC_C23 1
#endif

#if __SIZEOF_POINTER__ == 2 && __INT_WIDTH__ == 16
#define Z80_SMALL_POINTER_MODEL 1
#endif

#if defined(__XCC_FLOAT_FORMAT_FIXED8_8)
#define FAST_FLOAT_UI_LABEL "8.8 fixed"
#elif defined(__XCC_FLOAT_FORMAT_FIXED16_16)
#define FAST_FLOAT_UI_LABEL "16.16 fixed"
#elif defined(__XCC_FLOAT_FORMAT_FIXED24_8)
#define FAST_FLOAT_UI_LABEL "24.8 fixed"
#else
#define FAST_FLOAT_UI_LABEL "IEEE single"
#endif

float root(float x) {
    return sqrtf(x);      /* compiler lowers this for the selected float ABI */
}
```

**Rule:** `--platform=` currently selects linker/platform support; it does not
predefine a platform symbol. If a project wants one, pass it explicitly, for
example `-D__XCC_PLATFORM_CPM3__=1`.

---

## 3. Compiler Pragmas

**What:** `xcc` supports practical source-level pragmas for include guards and
warning control, using the GNU diagnostic spelling that many portable projects
already use.

**When:** Use pragmas when a header should be included only once, or when a
specific compatibility shim needs to silence or promote one warning without
turning off diagnostics for the whole build.

**How it works:** `#pragma once` is handled by the preprocessor using the
canonical include path. Diagnostic pragmas are applied from the following line
onward, can be stacked with `push` / `pop`, and use the same warning group names
as `-W...` command-line switches.

| Pragma | Status | Meaning |
|---|---|---|
| `#pragma once` | Supported | Mark the current header as include-once, even if it is reached through another relative path. |
| `#pragma GCC diagnostic push` | Supported | Save the current warning state. |
| `#pragma GCC diagnostic pop` | Supported | Restore the last pushed warning state. |
| `#pragma GCC diagnostic ignored "-Wname"` | Supported | Disable one warning group from the next line onward. |
| `#pragma GCC diagnostic warning "-Wname"` | Supported | Enable one warning group and make it a warning. |
| `#pragma GCC diagnostic error "-Wname"` | Supported | Enable one warning group and promote it to an error. |
| `#pragma diagnostic ...` | Supported | Accepted as a short alias for `#pragma GCC diagnostic ...`. |
| `_Pragma("...")` | Parsed only | Accepted as a C pragma operator statement, currently discarded as a no-op. |
| Unknown pragmas | Ignored by default | Warn with `-Wunknown-pragmas`, or silence with the diagnostic pragma. |

Recognized warning group names:

| Group | Aliases |
|---|---|
| `general` | |
| `cpp` | |
| `unknown-pragmas` | |
| `unknown-warning-option` | |
| `implicit-function-declaration` | |
| `deprecated-declarations` | `deprecated` |
| `unused-result` | `nodiscard` |
| `attributes` | |
| `old-style-definition` | |
| `c23-extensions` | |
| `abi` | |
| `constexpr-not-constant` | |
| `bitint-width` | |

Header guard example:

```c
#pragma once

int device_open(const char *name);
int device_close(int fd);
```

Local warning suppression:

```c
[[deprecated("use new_api")]] void old_api(void);
void new_api(void);

void compatibility_entry(void) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    old_api();
#pragma GCC diagnostic pop

    new_api();
}
```

Make one warning fatal in a narrow region:

```c
#pragma GCC diagnostic push
#pragma GCC diagnostic error "-Wimplicit-function-declaration"

void strict_region(void) {
    must_be_declared_first();
}

#pragma GCC diagnostic pop
```

Silence vendor pragmas from imported headers:

```c
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma clang fp eval_method(double)
```

**Rule:** Prefer `#pragma GCC diagnostic ...` for real diagnostic control.
`_Pragma("...")` is accepted for parser compatibility today, but it does not
yet execute the pragma payload.

---

# Objects, Linking And Debug

## 4. One Toolchain, Two Object Worlds

**What:** One install speaks both SDCC/ASxxxx and GNU object formats.

**When:** You have legacy `.rel` / `.lib` code, or you want ELF / `ar` for a
GNU-style pipeline — both from a single install.

**How it works:** `--mode=sdcc` (default) reads SDCC assembly dialect and
writes `.rel` objects. `--mode=gnu` reads GNU `gas` dialect and writes ELF32
objects. The linker mode must match the objects you pass it. You cannot mix
`.rel` and `.o` in one link step without converting first (see §19).

| Mode | Assembly dialect | Object output | Archive format |
|---|---|---|---|
| `sdcc` | `sdasz80` | `.rel` | `.lib` (SDCC text index) |
| `gnu` | `gas` | `.o` (ELF32) | `.a` (GNU ar) |

```
# SDCC pipeline (default) → XL image
xas --mode=sdcc startup.s -o startup.rel
xld --mode=sdcc startup.rel main.rel -o app.xl

# GNU pipeline → ELF image
xas --mode=gnu startup.s -o startup.o
xld --mode=gnu startup.o main.o --oformat=elf -o app.elf
```

**Rule:** Pick one mode per link. Convert objects with `xobjcopy` if you need
to cross over.

---

## 5. Assembly Dialect Translation

**What:** `xas --format=` converts assembly *source* between SDCC and GNU syntax
without assembling. Parsed once into a common form, then printed in the other
dialect.

**When:** You have assembly in one dialect but need the other for your link
pipeline, or you want to review what `xcc -masm=gnuas` would look like on disk.

**How it works:** Instructions are shared between dialects; what changes is
directives, immediates, addressing order, sections, and macro syntax. Well-known
SDCC **areas** map to GNU **sections**:

| SDCC | GNU | Purpose |
|---|---|---|
| `.area _CODE` | `.text` | Executable code |
| `.area _DATA` | `.data` | Initialized writable data |
| `.area _CONST` | `.rodata` | Read-only constants |
| `.area _BSS` | `.bss` | Zero-init data |
| `.area _TLS` | `.tdata` | Thread-local storage |

Other spelling changes: `.globl` ↔ `.global`, `.dw` ↔ `.word`, `#42` ↔ `42`,
`5(ix)` ↔ `(ix+5)`. If the source has no section directive before the first
instruction, output goes into `_CODE` / `.text`.

**Macros:** Simple macros usually survive as native target-dialect macros.
GNU `\param` becomes bare `param` in SDCC; `.endm` ↔ `.endr` on repeat blocks.
If a macro uses dialect-only tricks (`\@` local labels, `'` string concat,
nested repeats inside a macro), `xas` expands it at the call site instead. The
bytes stay the same; the macro definition disappears from the output.

```
xas --mode=sdcc --format=gnu legacy.s -o legacy-gnu.s   # text only
xas --mode=gnu  --format=sdcc modern.s -o modern-sdcc.s
```

SDCC → GNU (sections and operands):

```
; --- input ---
.globl _start
.area _CODE
_start:
    ld a,#42
    ld 5(ix),a
    .dw _start

; --- output ---
.global _start
.text
_start:
    ld a, 42
    ld (ix+5), a
    .word _start
```

GNU → SDCC (macro kept):

```
; --- input ---
        .section .text
        .macro PUSH2 r1 r2
        push \r1
        push \r2
        .endm
        PUSH2 bc, de

; --- output ---
        .area _CODE
        .macro PUSH2 r1, r2
        push r1
        push r2
        .endm
        PUSH2 bc, de
```

GNU → SDCC (macro expanded — `\@` has no SDCC equivalent):

```
; --- input ---
        .macro skip
        jr .L\@
        nop
.L\@:
        .endm
        skip
        skip

; --- output ---
        jr .L1
        nop
.L1:
        jr .L2
        nop
.L2:
```

**Rule:** `--format=` emits text; add a normal `xas` pass (without `--format`)
to produce an object file.

---

## 6. Dual Debug Model

**What:** Source-level debug info in either SDCC style (CDB) or GNU style
(DWARF inside ELF).

**When:** You want breakpoints on C functions, source-line stepping, or variable
printing in `xgdb`.

**How it works:** Pass `-g` to `xcc` (forwarded to `xld`). In the default SDCC
pipeline the linker writes a sidecar `.cdb` text database mapping addresses to
source lines, symbols, and scopes. In the GNU pipeline (`-masm=gnuas
--oformat=elf`) the same switch embeds DWARF2 sections in the ELF file.

| Artifact | Pipeline | Contains |
|---|---|---|
| `app.cdb` | SDCC / XL | Functions, locals, source file:line ↔ address |
| `app.map` | Any (`-Map=` / `-Wl,-Map=`) | Final addresses of sections and symbols |
| DWARF sections | GNU / ELF | Same role as CDB, inside the object |

```
# SDCC path — CDB sidecar next to the image
xcc -g main.c -o app.xl
# → app.xl, app.cdb, often app.map

# GNU path — debug inside ELF
xcc -masm=gnuas -g --oformat=elf main.c -o app.elf
```

**Rule:** `xgdb` needs the executable *and* the debug database. For XL builds
that means `--cdb app.cdb`. The `.map` file is optional but useful when an
address looks wrong.

---

## 7. Integrated Z80 Debugger

**What:** `xgdb` (GDB-style front end) + `xemu` (standalone Z80 emulator
speaking GDB remote protocol over TCP).

**When:** Debug on your workstation without hardware, or attach `xgdb` to any
remote target that speaks the same protocol.

**How it works:** Build with `-g`. Start `xemu` as the "machine" — it loads
your binary at `--origin` and sets PC/SP. Start `xgdb`, point it at the same
image plus the CDB file, and connect with `--remote`. Then use normal commands:
`break main`, `run`, `next`, `step`, `print x`, `continue`.

```
# Terminal 1 — emulator target
xemu --listen 127.0.0.1:9000 \
         --load-bin app.bin --origin 0x0100 --pc 0x0100

# Terminal 2 — debugger
xgdb --exec app.bin --cdb app.cdb --map app.map --remote 127.0.0.1:9000
```

```
# XL workflow (CDB name follows the -o basename)
xcc -g main.c -o app.xl
xemu --listen 127.0.0.1:9000 &
xgdb --exec app.xl --cdb app.cdb --remote 127.0.0.1:9000
```

**Key options:**

| Tool | Flag | Meaning |
|---|---|---|
| `xemu` | `--origin ADDR` | Where raw binary is loaded in RAM |
| `xemu` | `--pc ADDR` | Initial program counter (often = origin) |
| `xgdb` | `--cdb FILE` | SDCC debug database from `xld -g` |
| `xgdb` | `-d DIR` | Extra source search path |

---

## 8. Relocatable XL Output

**What:** XL is the default executable format — a loadable Z80 program with a
header and relocation table.

**When:** Your loader or OS picks the run address at runtime (YOS, monitors,
banked RAM). Contrast with `--oformat=binary`, which is fixed to `-Ttext`.

**How it works:** The linker assigns tentative addresses and emits the program
image. Every location that still contains an absolute address (jump target,
data pointer, etc.) is recorded in the relocation table in the XL header. A
loader copies the image to wherever RAM is free, walks the table, patches those
words, then jumps to the entry point. Same file, different address — that is
what *relocatable* means for XL.

```
xcc main.c -o app.xl
xld main.rel util.rel -o app.xl

# Tentative base is still just a link-time assumption
xld -Ttext=0x4000 main.rel -o app.xl   # remains relocatable
```

**Rule:** Need a fixed address for an emulator EPROM load? Use
`--oformat=binary -Ttext=0x8000`. Need a loader-friendly image? Stay with XL.

---

## 9. Reserved Address Ranges In The Linker

**What:** Mark address holes the linker must not place sections into.

**When:** Vectors at `0x0000`, firmware workspace, or memory-mapped I/O in the
middle of the map. Without reserves, the linker only sees "free" as everything
not yet allocated and may pack code on top of hardware.

**How it works:** Each `--reserve=lo-hi` removes a range from the placement
pool before section layout. Repeat the flag for multiple holes. Same effect in
linker scripts via `RESERVE(lo, hi)` (GNU `.ld`) or by avoiding those ranges in
your layout (SDCC `.lk`).

```
xld --oformat=binary -Ttext=0x0100 \
    --reserve=0x0000-0x003f \
    --reserve=0xff00-0xffff \
    main.rel -o app.bin
```

```
xcc main.c -Wl,--reserve=0x0000-0x003f -o app.xl
```

**Rule:** Reserves affect *placement*, not emission. If you explicitly put a
section at `0x0000` via script, you can still override a reserve — reserves are
guardrails for automatic layout.

---

## 10. Multiple Final Image Formats

**What:** One linker, four common output containers.

**When:** Match what your deployment step expects — OS loader, raw emulator,
Intel HEX flasher, or ELF-aware tools.

| `--oformat` | File | Use when |
|---|---|---|
| `xl` (default) | `.xl` | Relocatable load via OS/monitor |
| `binary` | `.bin` | Fixed-address raw image |
| `ihx` | `.ihx` | EPROM programmers, serial bootloaders |
| `elf` | `.elf` | GNU tools, DWARF debug embedded |

```
xcc main.c -o app.xl
xcc main.c --oformat=binary -Ttext=0x8000 -o app.bin
xcc main.c --oformat=ihx -o app.ihx
xcc -masm=gnuas main.c --oformat=elf -o app.elf
```

**Rule:** `binary` and `ihx` need a known load address (`-Ttext` or linker
script). XL does not.

---

# Optimization, Runtime, And Platforms

## 11. Standalone Z80 Optimizer

**What:** `xopt` — the same optimizer `xcc` uses internally, runnable on `.s`
files directly.

**When:** Hand-written assembly, compiler output you want to squeeze further,
or measuring size/cycle savings before applying an assembly rewrite to
compiler output.

**How it works:** Peephole and cross-instruction rewrites at `-O2`/`-Os`.
`-O3`/`-Of` add speed-biased rules. `--cross-file` concatenates inputs and
optimizes across translation-unit boundaries (unique labels required).
`--stats` prints byte/cycle estimates without writing output.

```
xopt -O3 input.s -o output.s
xopt -O3 --out-dir optimized *.s
xopt --stats -O3 src/*.s              # measure only
xopt --reg-coverage hotpath.s         # register pressure report
```

**Typical pipeline:**

```
xcc -S -O2 main.c -o main.s
xopt -O3 main.s -o main.opt.s
xas main.opt.s -o main.rel
```

---

## 12. Stable And Experimental Speed Optimization

**What:** In `xcc`, `-Of` is the stable speed profile and `-O3` is currently a
distinct but empty alias reserved for the next experiment. In standalone
`xopt`, aggressive assembly optimization and `--cross-file` operation remain
explicit tools.

**When:** Use `-Of` for the validated compiler speed pipeline. Use `-O3` when
testing a newly staged speed experiment; until one is added it deliberately
produces the `-Of` result. Use `xopt --cross-file` only when you are willing to
inspect and test the combined assembly.

**How it works:** The compiler applies only stabilized IR, backend, and
assembly transformations. The former whole-function structural selector was
removed after the overfitting audit. `xopt --cross-file` can still optimize
separately compiled `.s` files as an explicit post-link-style experiment.

The stable backend includes data-flow-guarded register residence for loop
reductions, dispatch values, and immutable byte arguments; pair-pressure
scheduling for simple pointer walks; unsigned shift/mask folding; direct word
producer/store forwarding; and `HL`/`DE` scheduling of consecutive word-load
and add chains. These transformations are selected from IR shape, liveness,
CFG edges, and clobber information—not source names or whole-program
fingerprints. The locked z88dk comparison provides the current external audit:
both XCC profiles pass 24/24; `-Os` is strictly smallest on 24/24 programs,
`-Of` is smallest on 22/24 and fastest on 13/24 against the best valid current
zsdcc/80cc result. The linked-size comparison uses the generic
`z88dk-classic` runtime profile's literal-derived printf/scanf capabilities,
not precomputed benchmark masks.

```
xcc -O3 main.c util.c -o app.xl
```

```
xcc -S -O3 main.c util.c
xopt -O3 --cross-file main.s util.s -o combined.s
xas combined.s -o app.rel
```

**Rule:** Pure size policy belongs in `-Os`. New work starts in the empty
`-O3` lane and needs generic legality guards, tests, and independent-corpus
measurements before promotion to `-Of`; Pareto-safe size wins may also graduate
to `-Os`. Pointer induction must have a natural-loop reaching-definition proof,
and assembly peepholes must preserve register live-outs on every path. Pair
standalone `xopt` experiments with tests or `--stats`.

---

## 13. Selectable Float Representation

**What:** Choose how the C `float` type is implemented at compile time — IEEE
software float or fixed-point — without changing your source style.

**When:** IEEE-754 is correct but heavy on a small Z80. Fixed-point gives you
fast integer arithmetic with fractional precision you pick (8.8, 16.16, or
24.8).

**How it works:** `--float-format=` changes the `float` ABI. Source still uses
`float`, literals, operators, and `<math.h>`. The compiler emits calls to the
matching runtime (`fixed16_16_mul`, etc.) instead of IEEE helpers. Constant
folding and `-O3` can specialize further (divide by 2 → shift; multiply by
1.5 → dedicated helper).

| Format | Layout | Good for |
|---|---|---|
| `ieee32` (default) | 32-bit IEEE single | Full dynamic range, standards compliance |
| `fixed8_8` | 8 int + 8 frac bits | Very small code, limited range |
| `fixed16_16` | 16 + 16 | General embedded math |
| `fixed24_8` | 24 + 8 | Wider integer part, coarser fractions |

Fixed formats are a **complete `float` replacement** for typical embedded code.
The fixed runtime and `*f` math library (`sqrtf`, `fabsf`, `floorf`, `roundf`,
`fminf`/`fmaxf`, classification, `copysignf`, …) are in-tree and linked
automatically. In the default M distribution, `double` and `long double`
alias the selected `float` format; use L when a distinct binary64 type is
required.

```
#include <math.h>

float area(float w, float h) { return w * h; }
float bias(float v)  { return (v + 1.5f) / 3.0f; }
float filt(float s)  { return sqrtf(fabsf(s)); }
```

```
xcc -O3 --float-format=ieee32     math.c -o math-ieee.xl
xcc -O3 --float-format=fixed16_16 math.c -o math-16_16.xl
xcc -O3 --float-format=fixed8_8   math.c -o math-8_8.xl
xcc -O3 --float-format=fixed24_8  math.c -o math-24_8.xl
```

**Rule:** Pick one format per link. All translation units and linked libraries
must agree on the `float` ABI.

---

## 14. 64-Bit Integer And Double Runtime Support

**What:** `long long` and `double` on a 16-bit-target toolchain.

**When:** Your program uses 64-bit counters, timestamps, or double-precision
math beyond 16-bit `int`/`long`.

**How it works:** In model L, wide integer ops (`__mullong`, shifts, compares)
and the software double runtime live in `<prefix>/z80/lib` and link
automatically when the compiler emits references. `float` ABI is independent
(see §13), so L can use fixed `float` with software binary64 `double`. In M,
`double` and `long double` are source-compatible aliases of the selected
`float` ABI instead.

```
long long ticks_since_boot(void);

double seconds_since_boot(void) {
    return (double)ticks_since_boot() / 50.0;
}
```

```
xcc time.c -o time.xl
```

#### S model and libc feature switches

**What:** Build a smaller libc by omitting memory-hungry formatting and
conversion paths for `float`, `double`, `long`, and `long long`.

**When:** You are targeting small ROM/RAM machines and know your program does
not need wide numeric support in libc. This is especially useful for making
`printf`, `scanf`, `strto*`, and related helper surfaces much smaller.

**How it works:** These are **toolchain/library build switches**, not source
language switches. Rebuild or stage libc with an `X_MODEL`, then normal `xcc`
links against the smaller staged `libc.a`. The root makefile exports matching
feature macros (`__XCC_LIBC_NO_FLOAT`, `__XCC_LIBC_NO_DOUBLE`,
`__XCC_LIBC_NO_LONG`, `__XCC_LIBC_NO_LONGLONG`, and
`__XCC_LIBC_NO_STDIO_FLOAT`) so C and assembly sources can compile out the
heavy code paths.

An ordinary `make` or `make -C x` build defaults to the middle `M` model.
Select `X_MODEL=S` for the smallest library or `X_MODEL=L` for the complete
wide-numeric library; the explicit root targets `x-s`, `x-m`, and `x-l`
continue to stage all three distributions separately.

| Build switch | Effect |
|---|---|
| `X_MODEL=S` | Keep core integer ABI/runtime, file positioning, time, and rand; omit wide numeric formatting/conversion, floating math, and `long long` support |
| `X_MODEL=M` | Keep `float` and `long`; alias `double`/`long double` to `float`; omit binary64, `long long`, and stdio float conversions |
| `X_MODEL=L` | Full model: keep `float`, `double`, `long`, `long long`, and stdio float conversions |
| `LIBC_FLOAT=0` | Omit libc `float` formatting/conversion paths |
| `LIBC_DOUBLE=0` | Omit libc `double` formatting/conversion paths |
| `LIBC_LONG=0` | Omit libc `long` text/formatting APIs while retaining core 32-bit ABI/runtime services |
| `LIBC_LONGLONG=0` | Omit libc `long long` formatting/conversion paths |
| `LIBC_STDIO_FLOAT=0` | Keep `printf`/`scanf` integer-only even when `float` stays enabled elsewhere |

```
# Default distribution: the M model
make

# Smallest libc: the S model
make clean
X_MODEL=S make libc stage-xcc-support

# Middle libc: keep float and long, but keep stdio integer-only
make clean
X_MODEL=M make libc stage-xcc-support

# Selective custom build: keep long, drop float/double/long long
make clean
LIBC_FLOAT=0 LIBC_DOUBLE=0 LIBC_LONG=1 LIBC_LONGLONG=0 \
    make libc stage-xcc-support
```

Program source stays normal; unsupported heavy format/conversion cases are
compiled out of the library you staged:

```
/* S-model friendly: no float/double/long-long formatting needed. */
#include <stdio.h>

int main(void) {
    printf("score=%d\n", 42);
    return 0;
}
```

```
xcc --platform=cpm3 -Os hello.c -o hello.com
```

**Rule:** `X_MODEL=S` is now the canonical small-library switch. Use the
individual `LIBC_*` toggles when you want a custom middle ground.

---

## 15. Retargetable Platform Model

**What:** `--platform=<name>` selects startup code, linker script, and the
platform library that implements libc's machine hooks. Platform definitions are
**not hardcoded** in the linker — each platform is a source directory under
`x/platforms/<name>/`, and `xld` picks up the staged artifacts for that name at
link time.

**When:** Moving from bare metal to CP/M, Spectrum, or your own board. One
switch replaces hand-picking `crt0.rel` and syscall stubs.

**How name resolution works:** `xcc` forwards `--platform=<name>` to `xld`
(default `none`). The linker normalizes the name (it accepts both `cpm3` and
`z80-cpm3`), strips the `z80-` prefix when present, and looks for files named
after the short name:

| Artifact | Filename pattern |
|---|---|
| Startup | `crt0-<name>.rel` |
| Platform hooks | `lib<name>.a` |
| Linker script | `linker-<name>.lk` / `linker-<name>.ld` |

It probes `<prefix>/targets/z80-<name>/lib` first, then falls back to
`<prefix>/z80/lib` (the usual install layout). No fixed table of platform names
— if the files for `<name>` are staged, the platform works.

**Source layout:** Every platform is a directory `x/platforms/<name>/` with
`crt0.s`, `linker.lk`, `linker.ld`, and one file per hook (`write.s`,
`read.s`, `_exit.s`, …). The hook contract is declared in
`x/libc/include/sys.h`. Shipped examples: `none`, `cpm3`, `zx-rom`,
`zx-ram`. Target-only public headers remain inside that target directory; for
for example, CP/M's `sys/bdos.h` lives below `x/platforms/cpm3/include/` and is
visible only with `--platform=cpm3`. The ZX targets do not need private public
headers; none is a shared pseudo-platform.

The CP/M 3 startup converts the length-prefixed tail at `0x0080` into normal C
arguments. Since CP/M does not provide the transient program name,
`argv[0]` is an empty string; subsequent words begin at `argv[1]` and
`argv[argc]` is null. ASCII whitespace separates words and double quotes group
spaces without becoming part of the argument. Strings and the pointer table
are copied to an exact-sized stack allocation because CP/M file operations can
reuse `0x0080` as the default DMA buffer. Startup supplies the two arguments in
both conventions: HL/DE for `sdcccall(1)`, and right-to-left stack words for
`sdcccall(0)`. Its `<stdio.h>` `trygetchar()` uses BDOS function 11 and returns
immediately with zero or the console-ready status.

**ZX Spectrum 48K:** `zx-ram` emits fixed-address code beginning at `0x5CCB`,
the first byte after the Sinclair system variables. `xprog --tap` and
`--tzx` wrap that binary in a checksummed auto-start loader that can overwrite
its own BASIC program. `zx-rom` emits an exact 16 KiB replacement ROM and uses
the linker's `AT>rom` / `COPY _DATA` support to initialize writable state at
`0x5B00`.

Both ZX forms use hand-written assembly for startup and every platform hook.
Each self-contained target carries the YOS-derived bitmap console, proportional
6x12 Tamsyn font exported by snatch, and physical keyboard-matrix scanner.
The scanner is exported as level-triggered, non-blocking `<stdio.h>` `trygetchar()`;
blocking libc input loops around that same primitive. Console descriptors
work; filesystem and wall-clock hooks return their documented failure values.
See `x/docs/howtos/ZX-SPECTRUM-48K.md` for memory maps, Fuse commands, and the
four-mode MCP regression.

**Starting point — copy `none`:** `x/platforms/none/` is an empty-shell
template:
every required symbol exists but does the minimum (output discarded, files fail,
clock reads zero). Copy it when adding a board — do not start from scratch.
`x/platforms/none/README.md` has the full contract and reference empty C
implementations. See also `x/docs/howtos/RETARGET-LIBC.md`.

**How to add your own platform:**

```
cp -R x/platforms/none x/platforms/myboard
```

1. **`crt0.s`** — entry symbol `_entry`: set stack, zero `.bss`, copy `.data`,
   call `main`, call `exit` / `_exit`.
2. **`linker.lk` / `linker.ld`** — where `_CODE` / `.text` starts, entry symbol,
   reserved regions (see §21).
3. **Platform hooks** — implement the contract in `x/libc/include/sys.h`:

   | Symbol | Job |
   |---|---|
   | `_exit` | Stop the program |
   | `heap_region` | Tell malloc where RAM is (asm; returns HL/DE) |
   | `write` / `read` | Console and file I/O (fd 1/0 for stdout/stdin) |
   | `open` / `close` / `lseek` | Files (or return errors) |
   | `unlink` / `rename` | Filesystem ops |
   | `gettimeofday` | Wall clock for `time()` |

   Start with `write` if you only need `printf`. Filesystem hooks can stay as
   failing stubs.

4. **Build and stage** — the root Makefile assembles every `*.s` / `*.c` in
   your directory (except `crt0.s`, which stays standalone) into
   `libmyboard.a`, plus `crt0-myboard.rel` and the linker scripts:

```
PLATFORM=myboard make stage-xcc-support   # stage into the install prefix
# or: PLATFORM=myboard make -C x          # full toolchain rebuild
```

5. **Use it:**

```
xcc main.c --platform=myboard -o app.xl
```

---

## 16. Self-Contained Relocatable Installation

**What:** Copy the install prefix anywhere; tools still find headers and
libraries.

**When:** Packaging for CI, USB distribution, multiple versions side by side, or
`/opt/xtools` style deployment. Different meaning from XL relocatable (§8) —
this is about the *toolchain tree*, not your program binary.

**How it works:** Each tool discovers `<prefix>` from its own path. Common
headers sit in `<prefix>/z80/include`, target-private headers in
`<prefix>/z80/include/<platform>`, and target libs in `<prefix>/z80/lib`. No
`XTOP` env var required.

```
cp -R xtools /opt/xtools
/opt/xtools/bin/xcc main.c --platform=cpm3 -o app.xl
```

**Layout:**

```
bin/           xcc, xas, xld, xopt, xar, xobjcopy, xprog, xgdb, …
z80/include/   common C headers plus target-private subdirectories
z80/lib/       crt0, libc, runtime, libnone.a, libcpm3.a, …
share/doc/     tool manuals
```

---

# Build, Link, And Distribution

## 17. GNU-Style Driver Behavior

**What:** `xcc` and `xld` accept GCC/GNU ld switches you already know.

**When:** Integrating with existing flags, IDE settings, or documentation copied
from ARM/x86 cross-compile guides.

**Common flags:**

| Flag | Effect |
|---|---|
| `-Ipath` | Add include directory |
| `-DNAME=val` | Preprocessor define |
| `-Lpath` / `-lname` | Library search / link `libname.a` |
| `-c` | Compile+assemble only → `.rel` |
| `-S` | Compile only → `.s` |
| `-g` | Debug info |
| `-T file` / `-Ttext=addr` | Linker script / section base |
| `-e sym` | Entry symbol |
| `--oformat=fmt` | Output container (§10) |
| `-Wl,arg` | Forward arg to `xld` (repeat: `-Wl,-Map=f,-g`) |
| `-nostdlib` / `-nostartfiles` | Skip automatic crt0/libs |

```
xcc -Iinclude -DNDEBUG main.c util.s \
    -Llib -lfoo \
    -Wl,-Map=app.map,-g \
    --oformat=binary -Ttext=0x8000 \
    -o app.bin
```

---

## 18. Classic CMake And Make Integration

**What:** Point `CC` at `xcc`; no bespoke compiler wrapper.

**When:** You already have Make or CMake and want Z80 as another cross target.

```
CC=/opt/xtools/bin/xcc cmake -S . -B build -DCMAKE_SYSTEM_NAME=Generic
cmake --build build
```

```
CC      := xcc
CFLAGS  := -Os -Iinclude --platform=cpm3
LDFLAGS := -Wl,-Map=app.map

app.xl: main.c util.c
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@
```

**Tip:** Use `xcc -c` for separate compilation and `xcc -v` to see the exact
`xas`/`xld` commands when a rule misbehaves.

---

## 19. Object And Archive Conversion

**What:** `xobjcopy` translates objects and libraries between SDCC and GNU
formats.

**When:** One `.rel` file in a GNU link line, or shipping an SDCC `.lib` to a
team on ELF. Bridge formats; do not use it as a daily compile step.

```
xobjcopy -I rel -O elf main.rel main.o
xobjcopy -I elf -O rel main.o main.rel
xobjcopy -I lib -O ar libfoo.lib libfoo.a
xobjcopy -I ar -O lib libfoo.a libfoo.lib
```

**Rule:** Convert *before* linking. Symbol names and section boundaries are
preserved, but the two modes still need matching calling conventions.

---

## 20. Archive Compatibility

**What:** `xar` builds static libraries in either SDCC `.lib` or GNU `.a` form.

**When:** Packaging your own object collections for `-l` linking.

```
xar --mode=sdcc rcs libfoo.lib foo.rel bar.rel
xar --mode=gnu  rcs libfoo.a  foo.o  bar.o

xcc main.c -L. -lfoo -o app.xl
```

**Rule:** Library mode must match object mode. SDCC `.lib` with SDCC link;
GNU `.a` with GNU link.

---

## 21. Linker-Script Compatibility

**What:** Describe memory layout in a script instead of many CLI flags.

**When:** Platform defaults are close but not exact, or you want the map
documented in a file under version control. Each platform ships both dialects in
`x/platforms/<platform>/`.

**Legacy command file** (`linker.lk`) — compact SDCC-style layout:

```
; CP/M 3 — COM loads at 0x0100
-f binary
-b _CODE = 0x0100
ENTRY _entry
```

```
xld --mode=sdcc -T x/platforms/cpm3/linker.lk crt0.rel main.rel -o app.com
```

**GNU linker script** (`linker.ld`) — sections, ranges, reserves:

```
ENTRY(_entry)
OUTPUT_FORMAT(binary)
BINARY_RANGE(0x0100-0xFFFF)
RESERVE(0x0000, 0x003f)

SECTIONS {
  .text 0x0100 : { *(.text*) }
  .data        : { *(.data*) }
  .bss         : { *(.bss*) }
}
```

```
xld --mode=gnu -T x/platforms/cpm3/linker.ld crt0.o main.o -o app.com
```

**Rule:** `xcc --platform=cpm3` picks the platform script automatically. Override
with `-T your.ld` when needed. CLI flags like `-Ttext=` still win over script
defaults when both are present.

---

## 22. Automatic Runtime And Library Discovery

**What:** `xcc` / `xld` link a complete program from one `.c` file with no
manual `-l` list.

**When:** Always, unless you are doing freestanding work.

**What gets linked automatically (in order):**

1. `crt0` for `--platform`
2. Your objects
3. libc (`libc.a`)
4. Compiler runtime (integer helpers, and float runtime matching `--float-format`)
5. Platform library (`lib<name>.a` for the selected `--platform`)

```
xcc main.c --platform=cpm3 -o app.xl    # driver does everything
xld main.rel --platform=cpm3 -o app.xl  # same when linking by hand
```

**Opt out:**

```
xcc -nostdlib -nostartfiles minimal.s -o minimal.bin --oformat=binary
xld --no-default-runtime main.rel -o main.xl   # skip prefix probe only
```

---

## 23. Useful Bare-Metal Defaults

**What:** Omitting `--platform` is the same as `--platform=none`.

**When:** Bring-up before you have a real board package, freestanding tests, or
algorithms with no I/O yet.

**What `none` gives you:** Links and runs, but `printf` goes nowhere, files
fail, clock reads zero. The `x/platforms/none/` directory is both the default
backend and the **template to copy** when you add a real platform — every hook
is already declared as an empty shell you can fill in. Memory map starts
`.text` at `0x0000` unless you override.

```
xcc main.c -o app.xl              # platform=none implied
xcc main.c --platform=none -o app.xl
```

**Next step:** `cp -R x/platforms/none x/platforms/<your-target>/`, implement `write`
(and whatever else you need), build with `PLATFORM=<your-target>`, then use
`--platform=<your-target>` (§15).

---

## 24. One Suite For The Whole Pipeline

**What:** Compile → assemble → optimize → link → debug without format mismatches.

**When:** You want a single integrated pipeline from source to debug, or you are
documenting a workflow for your team.

**End-to-end example:**

```
# 1. Compile to assembly
xcc -S -O2 -g main.c -o main.s

# 2. Optimize assembly
xopt -O3 main.s -o main.opt.s

# 3. Assemble
xas main.opt.s -o main.rel

# 4. Link with debug info
xld main.rel -g -o main.xl

# 5. Debug
xemu --listen 127.0.0.1:9000 &
xgdb --exec main.xl --cdb main.cdb --remote 127.0.0.1:9000
```

**One-liner for daily work:**

```
xcc -Os -g main.c util.c --platform=cpm3 -o app.xl
```

Every tool in the chain shares the same install prefix, object modes, debug
formats, and platform layout — that is the point of the suite.

---

# C23 Language And Library

The sections below cover C23 language support, a full libc surface in
assembler, atomics, register calling conventions, and related library features.

---

## 25. C23 Language Support In The Compiler

**What:** `xcc` implements a broad slice of ISO C23 across the frontend,
preprocessor, and codegen.

**When:** You want modern C23 on Z80 — `bool`, `auto`, `constexpr`, `_BitInt`,
binary literals, and the rest of the features below.

**How it works:** The frontend, preprocessor, and codegen understand C23
constructs. The in-tree test suite under `x/src/xcc/tests/data/core/` has
dedicated cases for each feature below. A separate conformance matrix lives in
`x/tests/tests/c23/` for broader manifest-driven regression coverage.

**Language features implemented (representative):**

| Area | Examples |
|---|---|
| Types | `bool`/`true`/`false` keywords, `nullptr`, `char8_t`, `_BitInt(N)`, `typeof` / `typeof_unqual`, `auto` deduction |
| Constants | Binary literals (`0b1010`), digit separators (`1'000`), `constexpr` objects |
| Preprocessor | `#elifdef` / `#elifndef`, `#warning`, `__VA_OPT__`, `__has_include`, `__has_c_attribute` |
| Initialization | Empty `{}` init, designated initializers, compound literals with storage class |
| Enums | Typed enums (`enum e : unsigned char`) |
| Attributes | Standard C23 `[[noreturn]]`, `[[deprecated]]`, … plus Z80-specific `[[sdcc::…]]` and `[[z88dk::…]]` — full list in §34–§42 |
| Other | `_Generic`, `_Static_assert`, `_Complex` arithmetic (lowered by compiler), `__asm__` inline asm, `unreachable()` |

```
// C23 keywords — no <stdbool.h> required
bool ready = true;

// auto deduction
auto count = 0xFFFFu;

// _BitInt — width-precise integers
_BitInt(7) port_shadow = 0;

// Preprocessor
#define LOG(...) __VA_OPT__(void)__VA_ARGS__)

// Inline assembly
int pin(void) {
    int v = 0;
    __asm__("nop");
    return v;
}
```

```
xcc -std=c23 app.c -o app.xl    # default; explicit for build scripts
```

**Rule:** Not every C23 library function is linkable yet. Check §26 for libc
status; missing math transcendentals are declared in `<math.h>` but may not link
until implemented.

---

## 26. C23 Standard Library In Hand-Written Assembler

**What:** `x/libc/` is a C23 standard library implemented in Z80 assembly.

**When:** You need real `printf`, `malloc`, wide/UTF conversion, or IEC 60559
math on-target — and you care about code size and register usage on Z80.

**How it works:** Functions live in existing `.s` files following project style
(`.module`, `.area _CODE`, `sdcccall(1)` where appropriate, IX frames, EXX for
64-bit paths). Headers in `x/libc/include/` declare the full C23 surface.
`xcc` links `libc.a` automatically.

**Design constraint — thread-safe by default:** New libc code avoids writable
static data. State lives on the stack, in registers, or in explicitly passed
buffers. That makes the library usable in interrupt-heavy or cooperative
multitasking setups without surprise `_DATA` collisions.

**What is in good shape today:**

| Header / area | Status |
|---|---|
| `stdio.h` | `printf`/`scanf` family, float printing, C23 `%b` binary conversions |
| `stdlib.h` | `malloc`/`free`, `free_sized`, `free_aligned`, `strfromf`/`d`/`l` |
| `string.h` / `wchar.h` / `uchar.h` | Full string and wide/UTF-16/UTF-32 layer |
| `time.h` | `timespec_get`, `timespec_getres` |
| `stdbit.h` | Complete C23 bit utilities (header-inline `_Generic`) |
| `stdckdint.h` | `ckd_add` / `ckd_sub` / `ckd_mul` with real overflow detection |
| `stdatomic.h` | 8- and 16-bit atomics (see §27) |
| `math.h` | Broad C23 interface; fixed-point `*f` redirect with `--float-format=` |
| `complex.h` | `_Complex` accessors and core helpers |
| `tgmath.h` | Type-generic macros wired to implemented math/complex |

```
#include <stdio.h>
#include <stdlib.h>
#include <stdckdint.h>

int add_safe(int a, int b, int *out) {
    return ckd_add(out, a, b);   /* non-zero → overflow */
}

int demo(void) {
    char buf[32];
    strfromf(buf, sizeof buf, "%.3f", 1.25f);
    printf("pi bits: %b\n", 0b110100);
    free_sized(malloc(64), 64);
    return 0;
}
```

```
xcc demo.c -o demo.xl
```

**Rule:** Treat `<math.h>` transcendental families (trig, exp, log, …) as
declared-for-compatibility until you verify the symbol links. Classification,
roots, rounding, min/max, `fromfp*`, and fixed-point paths are the mature parts.

---

## 27. Atomics On Z80 (`<stdatomic.h>`)

**What:** C23 `_Atomic` types and `atomic_*` operations with runtime helpers.

**When:** Shared counters, flags, or hand-off buffers between your main code and
interrupt service routines on a single-core Z80.

**How it works:** The compiler lowers `atomic_*` calls to `__atomic_*` runtime
functions in `x/runtime/atomic/`. Each helper wraps the memory
operation in `di` / `ei` so a timer or serial ISR cannot observe a torn
read/write on 8- or 16-bit objects. Memory-order arguments are accepted for API
compatibility but collapse to this single-core behaviour.

**Supported widths today:** 1-byte and 2-byte objects (`char`, `unsigned char`,
`short`, `unsigned short`, and `int`/`unsigned int` via the 16-bit path).
Wider `atomic_long` and friends are declared; linking them calls a deliberate
trap helper until wider runtime ops exist.

```
#include <stdatomic.h>

static atomic_uchar rx_pending;
static atomic_ushort tick_count;

void isr_rx_byte(void) {
    atomic_fetch_add(&rx_pending, 1);
}

void main_loop(void) {
    unsigned char n = atomic_load(&rx_pending);
    if (n) {
        atomic_store(&rx_pending, 0);
        /* drain FIFO */
    }
}
```

**Rule:** Atomics serialize against interrupts, not against a second CPU. For
IRQ masking around a plain C function body, see `[[sdcc::critical]]` in §36.

---

## 28. `stdbit.h` And `stdckdint.h`

**What:** C23 bit-counting/scanning macros and checked integer arithmetic.

**When:** Protocol parsing, register bit maps, or safe arithmetic without
relying on signed-overflow UB.

**How it works:** `stdbit.h` is implemented as `_Generic` dispatch to inline
helpers — `stdc_count_ones`, `stdc_leading_zeros`, `stdc_bit_floor`,
`stdc_endian`, and the rest of the C23 `stdc_*` bit interface. `stdckdint.h`
routes `ckd_add`, `ckd_sub`, and `ckd_mul` through assembler helpers
(`__ckd_add_sint`, etc.) that compute the truncated result and return a
non-zero overflow flag per the standard.

```
#include <stdbit.h>
#include <stdckdint.h>

int parse_nibble(unsigned x) {
    return (int)stdc_count_ones(x & 0x0Fu);
}

int safe_scale(int value, int factor, int *result) {
    return ckd_mul(result, value, factor);
}
```

Both headers ship in the normal include tree and link against in-tree runtime
helpers.

---

## 29. UTF-8 `char8_t` And The Unicode Layer

**What:** C23 `char8_t` plus the full `uchar.h` / `wchar.h` conversion APIs.

**When:** Handling UTF-8 text on-target with the standard C23 conversion APIs.

**How it works:** `char8_t` is a distinct type from `char`. `mbrtoc8` /
`c8rtomb` convert between UTF-8 multibyte sequences and UTF-8 code units.
Alongside the existing `char16_t` / `char32_t` paths in `uchar.h` and the wide
stdio functions, you get one coherent C23 text stack in the same libc.

```
#include <uchar.h>

void put_utf8_char(char8_t c8) {
    char buf[8];
    mbstate_t st = {0};
    size_t n = c8rtomb(buf, c8, &st);
    if (n > 0) {
        /* write buf[0..n) to console */
    }
}
```

---

## 30. C23 Math And IEC 60559 Extras

**What:** Modern `<math.h>` beyond `sin`/`cos` — IEC 60559 manipulation functions,
payload/ordering helpers, and float formatting round-trip support.

**When:** Writing portable numeric code that uses C23 names (`fromfp`,
`fmaximum`, `roundeven`, `totalorder`, …) or formatting floats to string
without a home-grown `%f` implementation.

**Highlights:**

| Family | Examples | Notes |
|---|---|---|
| Rounding | `roundeven`, `fromfp`, `ufromfp`, `fromfpx` | IEC 60559 direction + width control |
| Min/max variants | `fmaximum`, `fminimum`, `fmaximum_num`, magnitude variants | Full C23 fmin/fmax superset |
| Ordering | `totalorder`, `totalordermag` | TotalOrder comparison |
| Payload | `getpayload`, `setpayload`, `setpayloadsig` | NaN payload manipulation |
| Formatting | `strfromf`, `strfromd`, `strfroml` | `%a`/`%f`-style formatting to caller buffer |
| Allocator | `free_sized`, `free_aligned` | C23 sized deallocation |

With `--float-format=fixed16_16` (§13), many `*f` forms redirect to
fixed-point helpers automatically. M's double spellings follow those same
helpers; L's double paths use the 64-bit software runtime.

```
#include <math.h>
#include <stdlib.h>

int compare_floats(float a, float b) {
    return totalorderf(&a, &b);   /* -1, 0, or 1 */
}

void report(float x) {
    char s[40];
    strfromf(s, sizeof s, "%.4e", x);
}
```

---

## 31. Register Calling Conventions

**What:** Multiple Z80 calling conventions in one compiler, selectable per
function via `[[sdcc::sdccall(N)]]` or `[[z88dk::…]]` (see §35–§41 for
prologue and epilogue codegen).

**When:** You want register-based argument passing for speed, or you are
linking objects that use a specific calling convention.

**How it works:** By default, C functions use the stack-based ABI. Attach
attributes to opt into register calling:

| Attribute | Arguments | Returns |
|---|---|---|
| `[[sdcc::sdccall(1)]]` | First 8-bit args in **L, B, C** (and wider rules for 16-bit) | 16-bit in **DE**, 32-bit in **DEHL** |
| `[[sdcc::sdccall(0)]]` | Stack only (used by variadic `printf`) | As above |
| `[[z88dk::fastcall]]` | First arg in **D** (8-bit) or **DE** (16-bit) | In first arg register |
| `[[z88dk::callee]]` | Callee pops stack arguments | Stack-based |

The platform hooks in `x/platforms/` and most of libc use `sdcccall(1)` so `printf`
and `read` do not shuffle every byte through the stack. `xcc -O2` can keep
incoming register arguments in registers across the function prologue when safe.

```
[[sdcc::sdccall(1)]]
int fast_add(int a, int b) {
    return a + b;
}

extern [[sdcc::sdccall(1)]] int foreign_mul(int a, int b);

int use_foreign(int x) {
    return foreign_mul(x, 3);   /* links against imported .rel / .o */
}
```

**Rule:** The convention on the declaration must match the object you link. The
import tests (`t080`–`t084` in the compiler suite) verify REL, ELF, `.lib`, and
`.a` interop when conventions align.

---

## 32. `_Complex` Numbers And Inline Assembly

**What:** C complex types with compiler-lowered arithmetic plus libc accessors.

**When:** DSP-ish code, phasors, or algorithms that use `float _Complex` as
first-class C.

**How it works:** `_Complex` is a keyword. `float _Complex` uses the target's
8-byte layout (two software-float components). `+`, `-`, `*` on complex values are
lowered in the compiler; `<complex.h>` supplies `creal`, `cimag`, `cabs`,
`conj`, `carg`, and the `CMPLXF` constructors. `tgmath.h` dispatches type-generic
calls via `_Generic`.

```
#include <complex.h>

float _Complex rotate(float _Complex z, float angle) {
    float _Complex t = CMPLXF(0.0f, angle);
    return z * t;
}
```

Inline assembly uses GCC-style `__asm__`:

```
void delay(void) {
    __asm__("nop");
    __asm__("nop");
}
```

---

## 33. Aggressive Compile-Time Evaluation (`-O2` And Up)

**What:** The compiler evaluates pure constant expressions and constant call
chains at compile time, not only for simple literals.

**When:** Lookup tables, size calculations, and bitmap masks without paying
runtime cost on Z80.

**How it works:** With `-O2` and above, `xcc` constant-folds arithmetic,
pointer differences, calls to `constexpr`-like pure helpers, and wide integer
operations when all inputs are known. The optimizer test suite (`t026`–`t045`,
`t028`–`t033`) locks this behaviour. `-Of` adds the validated speed-biased
pipeline on top; `-O3` currently aliases it and is ready for the next guarded
speed experiment.

```
/* With -O2, the loop folds to return 10; no runtime addition loop remains. */
int sum_1_to_4(void) {
    int s = 0;
    for (int i = 1; i <= 4; ++i)
        s += i;
    return s;
}
```

```
xcc -O2 sum.c -o sum.xl
xcc -S -O2 sum.c -o sum.s    # inspect: often just ld hl, #10 / ret
xcc -O3 hotpath.c -o hotpath.xl
```

**Rule:** `-O2` is the general default; use `-Of` for stable speed. `-O3` is
the empty experimental lane until new work is explicitly staged there.

---

## 34. `[[attributes]]` Overview And Standard C23 Attributes

**What:** C23 `[[attribute]]` syntax plus Z80-specific `[[sdcc::…]]` and
`[[z88dk::…]]` namespaces. Attributes change codegen, memory placement, or
diagnostics — they are not just hints.

**When:** ISRs, port-mapped I/O, absolute RAM mirrors, banked sections, fast
register calls, or standard C23 deprecation / nodiscard warnings.

**Syntax:** `[[ns::name]]`, `[[ns::name(arg)]]`, or several in sequence:
`[[noreturn]] [[sdcc::naked]]`. Unknown standard attributes warn and are
ignored; unknown `sdcc::` / `z88dk::` names warn. `__attribute__((…))` on a
declarator is accepted and skipped (use `[[…]]` for xcc effects).

Inspect what the compiler actually emits:

```
xcc -S app.c -o app.s    # read the assembly comments and instructions
```

Standard C23 attributes (no namespace):

| Attribute | Applies to | Effect |
|---|---|---|
| `[[noreturn]]` | Function | **No `ret` epilogue** — fall off the end or jump forever |
| `[[deprecated]]` / `[[deprecated("msg")]]` | Function, variable, type | **Warning** at every use site |
| `[[nodiscard]]` / `[[nodiscard("msg")]]` | Function return type | **Warning** if return value is discarded |
| `[[maybe_unused]]` | Variable, parameter, type, label | Suppresses unused-symbol warnings |
| `[[fallthrough]]` | Statement (`switch` case) | Marks intentional fall-through (no warning yet) |
| `[[unsequenced]]` | Function | Stored on symbol (informational) |
| `[[reproducible]]` | Function | Stored on symbol (informational) |

`[[noreturn]]` — epilogue omitted:

```
[[noreturn]] void halt_forever(void) {
    while (1) {}
}
```

```
_halt_forever:
    ; prologue …
__xcc_L0:
    jp  __xcc_L1
__xcc_L1:
    jp  __xcc_L0
__halt_forever_end:
    ; epilogue omitted: halt_forever is [[noreturn]]
```

---

## 35. `[[sdcc::sdccall(n)]]` ABI Selection

**What:** These attributes choose the function ABI explicitly. Use them when
one declaration must opt into stack ABI or the fast SDCC-style register ABI.

**Rules:** Only one ABI attribute per function. Variadic functions must use
`[[sdcc::sdccall(0)]]`.

`sdccall` is the canonical attribute name. XCC also accepts the historical
extra-`c` spelling `sdcccall` so existing sources continue to compile.

| Attribute | Argument passing | Return / epilogue | Use for |
|---|---|---|---|
| `[[sdcc::sdccall(0)]]` | Stack ABI with normal IX frame | `ret` | Variadic functions (`printf`), explicit stack interop |
| `[[sdcc::sdccall(1)]]` | First args in **L / BC / DE**, then stack | `ret`; 16-bit result in **DE** | Fast libc / platform hooks |

Example:

```
[[sdcc::sdccall(0)]] int stack_add(int a, int b);
[[sdcc::sdccall(1)]] int fast_add(int a, int b);
```

Generated assembly shows the chosen calling convention in the prologue comments:

```
_stack_add:
    ; sdcccall(0) prologue: stack_add
    ; receive (sdcccall(0)) param a at 4(ix)

_fast_add:
    ; sdcccall(1) prologue: fast_add
```

See §31 for the bigger calling-convention overview; this section is the
per-function override switch.

---

## 36. `[[sdcc::naked]]`, `[[sdcc::interrupt]]`, And `[[sdcc::critical]]`

These set CPU-entry / exit behavior rather than ordinary parameter passing.

| Attribute | Prologue | Body / epilogue | Use for |
|---|---|---|---|
| `[[sdcc::naked]]` | **Label only** — no `push ix`, no frame | **Nothing** — you supply all insns | Raw ISR stub, context switch, `halt` |
| `[[sdcc::interrupt]]` | `push af/bc/de/hl/iy` + IX frame | `reti` after full restore | Maskable interrupt service routines |
| `[[sdcc::critical]]` | **`di`** then IX frame | **`ei`** then `ret` | Short critical sections (not the same as `stdatomic`) |

`[[sdcc::naked]]` — you own the entire function body:

```
[[sdcc::naked]] void isr_stub(void) {
    __asm__("reti");
}
```

```
_isr_stub:
    ; naked: isr_stub
    reti
__isr_stub_end:
    ; naked epilogue: isr_stub
```

No stack frame, no register saves, no `ret` unless you write it. Taking
arguments in a naked function still uses the stack ABI internally if you
reference parameters — keep naked functions parameterless unless you know the
incoming register layout.

`[[sdcc::interrupt]]` — full context save and `reti`:

```
[[sdcc::interrupt]] void my_isr(void) {
    int x = 1;
}
```

```
_my_isr:
    ; interrupt prologue: my_isr
    push    af
    push    bc
    push    de
    push    hl
    push    iy
    push    ix
    ld      ix, #0
    add     ix, sp
    ; … function body …
__my_isr_end:
    ; interrupt epilogue: my_isr
    ld      sp, ix
    pop     ix
    pop     iy
    pop     hl
    pop     de
    pop     bc
    pop     af
    reti
```

`[[sdcc::critical]]` — interrupts masked for the whole function:

```
[[sdcc::critical]] void atomic_op(void) {
    int x = 42;
}
```

```
_atomic_op:
    ; critical prologue: atomic_op
    di
    push    ix
    ; … body …
__atomic_op_end:
    ld      sp, ix
    pop     ix
    ; critical epilogue: atomic_op
    ei
    ret
```

Combine with `[[noreturn]]` on naked shutdown paths:

```
[[noreturn]] [[sdcc::naked]] void raw_halt(void) {
    __asm__("halt");
    __asm__("jp 0");
}
```

---

## 37. `[[sdcc::at(addr)]]` Absolute Variables

**What:** Bind a C object name to a fixed machine address without allocating
space in `_DATA`.

```
[[sdcc::at(0x4000)]] int mapped_var;

int get(void) { return mapped_var; }
```

```
.globl _mapped_var
_mapped_var = 0x4000      ; 16384 — not allocated in _DATA

_get:
    ; …
    ld  hl, (_mapped_var)
    ; …
```

The linker does not reserve bytes in `_DATA`; the symbol is an absolute
address alias. You are responsible for that memory being valid RAM or MMIO.

Use this for shared RAM windows, firmware mailboxes, memory-mapped device
registers, or fixed screen buffers.

---

## 38. `[[sdcc::sfr(port)]]` Port-Mapped Variables

**What:** Map a C variable name onto a Z80 I/O port instead of a RAM cell.

`[[sdcc::sfr(0x3F)]]` — port number fits in the immediate `in` / `out` form:

```
[[sdcc::sfr(0x3F)]] unsigned char PORT_A;

void set_port(unsigned char v) { PORT_A = v; }
unsigned char get_port(void)   { return PORT_A; }
```

```
.globl _PORT_A
_PORT_A = 63              ; port number, not a RAM address

_set_port:
    ; … receive byte in A …
    out (#63), a

_get_port:
    in  a, (#63)
    ; … return via A / DE …
```

`[[sdcc::sfr(0x1234)]]` — 16-bit port address, using `BC` / `C` port I/O:

```
[[sdcc::sfr(0x1234)]] unsigned char PORT_WIDE;

void set_wide(unsigned char v) { PORT_WIDE = v; }
unsigned char get_wide(void)   { return PORT_WIDE; }
```

```
.globl _PORT_WIDE
_PORT_WIDE = 4660           ; 0x1234

_set_wide:
    ; … receive byte in A …
    push bc
    ld   bc, #4660
    out  (c), a
    pop  bc

_get_wide:
    push bc
    ld   bc, #4660
    in   a, (c)
    pop  bc
    ; … return via A / DE …
```

**SFR rules:**

- Port is a **Z80 I/O address** (`0`–`65535`). The symbol's numeric value *is*
  the port number. Values outside 16 bits are rejected.
- `xcc` decides at **compile time**: ports `0`–`255` use immediate `in/out`;
  ports `256`–`65535` load `BC` and use `in a,(c)` / `out (c),a`.
- Reads and writes go through the **`A` register** and Z80 `in` / `out`
  instructions — appropriate for Z80-style isolated I/O.
- There is no load/store to `(PORT_A)` as memory; `PORT_A = x` is always
  `out`, `y = PORT_A` is always `in`.
- Taking the address of an SFR variable gives you the port constant, not a
  memory pointer.

---

## 39. `[[xcc::far]]` 24-Bit Pointers

A `[[xcc::far]]` attribute placed **after the `*`** makes a pointer 24 bits
wide instead of 16: the low 16 bits are the address, the high 8 bits are a
**bank** selector. This is the standards-conforming attribute slot (C23
§6.7.6.1 puts an `attribute-specifier-sequence` right after the `*`, where it
appertains to the pointer — exactly like `const` in `char * const p`).

```
char * [[xcc::far]] p;                       // far pointer to char (3 bytes)
char * [[xcc::far]] * [[xcc::far]] pp;       // far pointer to far pointer
```

Only the C23 attribute spelling `[[xcc::far]]` is accepted for far pointers.

Placing the attribute *before* the `*` (`char [[xcc::far]] *p`) is diagnosed:
there it would appertain to the pointee type, not the pointer, so it does **not**
make a far pointer.

| Aspect | Behavior |
|---|---|
| `sizeof` | **3 bytes** (`addr16` + `bank`) vs 2 for a near pointer |
| Dereference | Routed through the per-target trampoline `__far_getb` / `__far_putb` (in `C` = bank, `HL` = address) |
| Arithmetic | Full **24-bit** add/sub: carrying/borrowing into the bank byte, so `p+n`, `p[i]`, `p++`, `p-n` behave like a flat pointer |
| `near → far` cast | Zero-extends: bank byte = 0 |
| `far → near` cast | Truncates to the low 16 bits |
| Return-by-value / args | Returned in `HL` (address) + `E` (bank); passed as 3 stack bytes |

A far pointer is a **distinct type** from a near pointer: assigning one to the
other requires a cast, and it has its own runtime, its own arithmetic, and its
own ABI footprint.

**Runtime trampoline.** Every far access calls a target hook:

```
__far_getb : in  C = bank, HL = address          -> out A = byte
__far_putb : in  C = bank, HL = address, A = byte -> stores the byte
```

Both **must preserve `BC`, `DE`, `HL`** (only `A`/flags change) so the compiler
can keep the pointer live across consecutive byte accesses. The default
implementations shipped for the `none` and `cpm3` targets ignore the bank and do
a plain `(hl)` access — so on an unbanked target a far pointer behaves exactly
like a near one. A banked target overrides `__far_getb` / `__far_putb` (linking
its own module ahead of the runtime library) to program the paging hardware from
the bank byte.

**Current limitations:** far−far pointer difference and far/far relational
comparisons compare only the low 16 bits (bank ignored); a full 24-bit
`memcpy`/`strcpy` family (`_fmemcpy`, …) and `intptr_t` widening to hold a far
pointer are not yet provided.

---

## 40. `[[xcc::bank(n)]]` Banked Code And Data Placement

A `[[xcc::bank(n)]]` attribute placed **before a function or variable declaration**
selects a numbered code/data bank `0..255`:

```
[[xcc::bank(3)]] int draw_sprite(void);
[[xcc::bank(7)]] unsigned char tile_cache[256];

void owner(void) {
    [[xcc::bank(5)]] static unsigned char scratch[32];
}
```

Only the C23 attribute spelling `[[xcc::bank(n)]]` is accepted for banked
placement.

`xcc` lowers this as a **placement attribute**:

- functions go to `_CODE_BANK_n`
- variables with static storage duration go to `_DATA_BANK_n`
- unannotated declarations continue to use the default `_CODE` / `_DATA`

This means the linker can order/place banked areas explicitly, and targets with
custom bank loaders can keep bank membership visible in object files.

**Rules:**

- `n` must be an integer in the range **`0..255`**
- applies to **functions** and **objects with static storage duration**
  (globals and `static` locals)
- rejected for automatic locals, `_Thread_local`, `[[sdcc::at(...)]]`, and
  `[[sdcc::sfr(...)]]`

**Current limitation:** this does **not yet** make calls automatically
cross-bank. `xcc` still emits ordinary near calls; `[[xcc::bank(n)]]` currently
controls section placement, not `__sdcc_banked_call` lowering.

---

## 41. `[[z88dk::…]]` Alternate Call Conventions

| Attribute | Argument passing | Callee cleanup |
|---|---|---|
| `[[z88dk::fastcall]]` | First 8-bit arg in **D**, 16-bit in **DE** | Caller pops nothing extra |
| `[[z88dk::callee]]` | Stack-based | **Callee** pops arguments |

Use when linking external objects that expect these register layouts:

```
[[z88dk::fastcall]] int z88_fast_inc(int x);
[[z88dk::callee]]   int z88_stack_add3(int a, int b, int c);
```

Caller of `z88_fast_inc` places the first `int` in **DE** before `call`.

---

## 42. Attribute Rules And Interactions

| Rule | Detail |
|---|---|
| Variadic functions | **Must** use `[[sdcc::sdccall(0)]]` — `printf` in `<stdio.h>` is annotated this way |
| `sdcccall(1)` + function pointers | Indirect calls fall back to stack ABI |
| `sdcccall(1)` + 4+ args | First three in registers; rest on stack |
| Imported libraries | `xld` can record ABI in `.rel`; mismatch with your declaration warns |
| `[[sdcc::naked]]` + locals | Frame is not set up — locals still work only if you add your own SP/IX setup |
| Memory-order atomics | Use `<stdatomic.h>` (§27) for `_Atomic` variables; `[[sdcc::critical]]` only masks IRQs for the function body |

Port-mapped I/O, absolute addresses, interrupt handlers, critical sections,
banked placement, and far pointers all use C23 `[[...]]` attribute syntax
instead of GNU-style `__attribute__((...))`.
The codegen is visible in `xcc -S` output with labelled prologue and epilogue
comments.

---

# Advanced Tooling And Integration

## 43. Bank-Switched Memory Emulation

**What:** `libxemu` can model flat RAM/ROM and selector/window-based banked
memory for paged machines, including port-driven bank selection. This is the
same emulator core used by the standalone `xemu` tool and by emulator-driven
tests.

**When:** Use it when a target swaps 16K/8K windows, overlays ROM and RAM,
routes far-pointer access through bank selectors, or needs paging behavior that
is difficult to validate on real hardware every time.

**How it works:** Build a `memory_map_config` from named **stores**,
**selectors**, **windows**, and **port rules**, then install it with
`machine::configure_memory_map()`. For the older fixed four-page model there
are still compatibility helpers: `configure_banked_memory()` and
`bind_bank_port()`.

| Building block | Meaning |
|---|---|
| `memory_store_config` | Backing storage object, bank count, bank size, and writability |
| `memory_selector_config` | Active bank register or latch |
| `memory_window_config` | CPU-visible address range mapped into one store |
| `memory_port_rule_config` | `out` side effect that updates a selector, with optional port masking |

```cpp
#include <xemu/xemu.h>

xemu::machine m;
xemu::memory_map_config map;

map.stores.push_back({"rom",   1, 0x10000, false});
map.stores.push_back({"banks", 8, 0x4000,  true});
map.selectors.push_back({"page", 0});

map.windows.push_back({0x0000, 0x3fff, "rom",   0, std::nullopt, 0});
map.windows.push_back({0x4000, 0x7fff, "banks", std::nullopt, std::string("page"), 0});
map.port_rules.push_back({0x7ffd, 0xffff, "page", 0x07, 0});

m.configure_memory_map(map);
```

This lets host-side tests exercise the same sort of paging that a real machine
would trigger via port writes, while still keeping breakpoints, run control,
and memory inspection available.

**Rule:** Use `configure_memory_map()` for arbitrary hardware. Keep
`configure_banked_memory()` and `bind_bank_port()` for quick compatibility with
the classic fixed 4x16K page-switching model.

---

## 44. Headless Emulator And Console Port Binding

**What:** `xemu` is not only a remote debugger target. It can also execute a
binary directly in headless mode and bind Z80 ports to host stdin/stdout.

**When:** Use this for CI smoke tests, golden-output execution tests, quick
bring-up of console-style programs, or any workflow where launching a debugger
would be unnecessary overhead.

**How it works:** `--listen` starts `xemu` as an RSP server for `xgdb`;
`--run` executes immediately. The CLI can bind one input port and one output
port, while the library also supports split status/data console input and a
`bind_emu_stdio()` helper for the default `platform=emu` console ABI.

| Mode | Typical use |
|---|---|
| `xemu --listen HOST:PORT` | Source-level debugging with `xgdb` or another RSP client |
| `xemu --run` | Headless execution for tests and quick manual checks |
| `--stdin-port` / `--stdout-port` | Simple console ABI on one input port and one output port |
| `bind_emu_stdio()` | Default split-console ABI: stdin status `0x00e2`, stdin data `0x00e3`, stdout `0x00e1` |

```bash
# Headless execution with simple port I/O
xemu --run --load-bin app.bin --origin 0x0100 --pc 0x0100 \
     --stdin-port 0 --stdout-port 1

# Debugger-target mode
xemu --listen 127.0.0.1:9000 --load-bin app.bin --origin 0x0100 --pc 0x0100
```

```cpp
xemu::machine m;
m.load_binary("app.bin", 0x0100);
m.set_pc(0x0100);
m.bind_emu_stdio(std::cin, std::cout);
auto stop = m.continue_execution(200000);
```

`--max-steps` is useful when a program should either finish or prove that it is
stuck within a predictable instruction budget.

**Rule:** For `platform=emu`, prefer the split-console ABI via
`bind_emu_stdio()` or ports `0x00e2` / `0x00e3` in and `0x00e1` out. Use plain
`--stdin-port` / `--stdout-port` only for simpler custom harnesses.

---

## 45. Embeddable Host SDK Libraries

**What:** The suite is also a host-side SDK. Its major internal engines are
shipped as reusable libraries with staged public headers, not just hidden
implementation details inside command-line tools.

**When:** Use these libraries when building your own debugger front end, ROM
inspector, object-file converter, emulator-driven test harness, or Z80 assembly
analysis tool on top of the X toolchain.

**How it works:** Host libraries stage into `<prefix>/include` and
`<prefix>/lib`, while the target C SDK stages separately into
`<prefix>/z80/include` and `<prefix>/z80/lib`.

| Library | Purpose |
|---|---|
| `libxemu` | In-process emulator, banked-memory helpers, breakpoint/run control, and RSP target adapter |
| `libxgdb` | Debugger-side symbol/source/disassembly model |
| `librsp` | GDB Remote Serial Protocol transport |
| `libxopt` | Shared Z80 optimization and analysis engine |
| `libxz80` | CPU core and disassembly support |
| `libxbfd` | Object, archive, binary, and debug-info reader/writer support |

Typical host-side integration:

```bash
c++ host_tool.cpp -I"$PREFIX/include" -L"$PREFIX/lib" \
   -lxemu -lxz80 -lrsp -o host_tool
```

Target-side C code still uses the separate cross SDK:

```bash
xcc app.c -o app.xl
# target headers from <prefix>/z80/include, target libs from <prefix>/z80/lib
```

**Rule:** Host SDK headers live in `include/`. Target program headers live in
`z80/include`. Treat them as two layers of the install tree, not one merged API
surface.

---

## 46. Fine-Grained Optimization Switches

**What:** `xcc` exposes named `-f...` and `-fno-...` toggles for individual
optimizer families, on top of the usual `-O0` / `-O1` / `-O2` / `-O3` /
`-Of` / `-Os` presets.

**When:** Use them to bisect regressions, pin one proven pass in CI, compare
generated code under controlled conditions, or build a project-specific tuning
profile without treating `-O3` as a black box.

**How it works:** An `-O` preset seeds the optimizer settings, then each
`-f...` or `-fno-...` override flips one named pass on or off.

| Area | Example switches |
|---|---|
| Late assembly | `peephole` |
| Module-level IR | `const-arg-prop`, `const-call-eval`, `function-const-eval`, `merge-identical-functions`, `inline-static-functions`, `internal-call-abi-promotion`, `internal-arg-packing` |
| Function IR | `scalar-local-promotion`, `reg-param-promotion`, `narrow-counted-byte-loops`, `loop-pointer-walk`, `duplicate-block-merge`, `merge-tails`, `local-frame-compaction` |
| Backend | `regalloc`, `compare-ifx-fusion`, `frame-omit`, `prealloc-temp-frame`, `switch-jump-tables`, `ctype-builtins` |

```bash
# Keep -O2, but turn off two backend choices
xcc -O2 -fno-regalloc -fno-frame-omit main.c -o main.xl

# Size profile with one explicit enable and one explicit disable
xcc -Os -fconst-call-eval -fno-inline-static-functions app.c -o app.xl

# Probe named shape-changing passes individually
xcc -Of -fno-duplicate-block-merge -fmerge-tails hot.c -o hot.xl
```

`-Of` enables `ctype-builtins` for the ASCII C locale supplied by X libc.
Calls are retained when the translation unit defines the named function, when
the declaration does not have the standard word argument/result shape, or
when `-fno-ctype-builtins` is specified. `internal-arg-packing` applies only to
private direct-call functions whose addresses do not escape; the callee and
every call site are rewritten as one module-level operation.

This is especially useful when a benchmark win is clear but you want to prove
which pass earned it, or when one transformation is good for a hot loop but
undesirable in the rest of a firmware image.

**Rule:** Start from `-O2` or `-Os` and override a few named passes. Building a
production profile from dozens of hand-picked flags at `-O0` is best treated as
an experiment, not a stable default.

---

## 47. GNU Compatibility Extras

**What:** Beyond core ISO C23, the frontend accepts several GNU-compatibility
constructs that make imported headers and macro-heavy codebases much easier to
port.

**When:** Use these when bringing over GCC-oriented utility headers, local
macro frameworks, or compatibility shims that assume GNU C spelling.

**How it works:** The parser supports GNU statement expressions `({ ... })`,
`typeof` / `typeof_unqual`, `__builtin_types_compatible_p`, and
`__builtin_bit_cast`. It also accepts `__extension__` as a no-op suppressor and
parses declarator `__attribute__((...))` for compatibility even when the real
effect should be spelled with C23 `[[...]]`.

| Feature | Use |
|---|---|
| `({ ... })` | Statement expressions for macro helpers that return a value |
| `typeof` / `typeof_unqual` | Derive types from expressions or strip qualifiers in portability code |
| `__builtin_types_compatible_p(T1, T2)` | Compile-time type dispatch and assertions |
| `__builtin_bit_cast(T, expr)` | Explicit same-size bit reinterpretation spellings |
| `__extension__` | Accepted as a compatibility prefix, ignored semantically |
| `__attribute__((...))` | Parsed on declarators for compatibility; use `[[...]]` for xcc-specific behavior |

```c
#define MAX_T(a, b) \
    ({ __typeof__(a) _a = (a); __typeof__(b) _b = (b); _a > _b ? _a : _b; })

_Static_assert(
    __builtin_types_compatible_p(typeof_unqual((const int)0), int),
    "expected unqualified int");
```

These features are particularly handy when importing code that was written to
straddle GCC, Clang, and SDCC-style environments without maintaining a special
`#ifdef XCC` fork.

**Rule:** Use GNU extras mainly in portability shims and macro infrastructure.
For ordinary application code, clean ISO C23 remains the best default surface.

---

## 48. GCC-Style Warning Surface

**What:** The command-line warning model follows familiar GCC spelling and
matches the warning-group names used by the pragma interface.

**When:** Use it when tightening CI, temporarily downgrading one noisy imported
warning, or promoting a specific correctness class such as ABI mismatches to an
error.

**How it works:** `xcc` supports whole-profile controls (`-Wall`, `-Wextra`,
`-Wpedantic`, `-Werror`, `-w`, `-W0`..`-W3`) plus per-group overrides
(`-Wname`, `-Wno-name`, `-Werror=name`, `-Wno-error=name`).

| Switch | Meaning |
|---|---|
| `-Wall` | Re-enable the full default warning surface |
| `-Wextra` | Add extra project checks such as `old-style-definition`, `abi`, `constexpr-not-constant`, and `bitint-width` |
| `-Wpedantic` | Enable pedantic language-surface checks such as `c23-extensions` |
| `-Werror` / `-Wno-error` | Promote or un-promote all warnings globally |
| `-Werror=name` | Promote one warning group only |
| `-Wno-name` | Disable one warning group without muting the rest |
| `-w` | Disable all warnings |

```bash
# Make ABI mismatches fatal, keep the rest as warnings
xcc -Wextra -Werror=abi main.c -o main.xl

# Silence just deprecation warnings while importing old APIs
xcc -Wno-deprecated-declarations compat.c -o compat.xl

# Strict CI lane
xcc -Wpedantic -Werror -c lib.c
```

The group names match the pragma controls from §3, so a warning you disable in
one file with `#pragma GCC diagnostic ignored "-Wattributes"` is the same group
you can enable globally with `-Wattributes`.

**Rule:** Prefer disabling one named group over `-w`, and prefer
`-Werror=<group>` over blanket `-Werror` when imported third-party code is still
being cleaned up.
