# The Boot Process

## Reset and esxDOS handoff

The kernel starts at `0x0000` in `y/src/z80/startup/crt0rom.s`. Its first five bytes are `DI`, `XOR A`, and `JP .init`. esxDOS resumes at `0x0001` during cold boot, so the `XOR A` and jump must stay at those exact addresses. The normal YOS entry begins at `0x0100`, after the complete firmware-compatible header.

```asm
        .org    0x0000
        di
        xor     a
        jp      .init
```

`.init` first uses `0xFFFF` as temporary stack space and calls `__startup_init` (see below). It then selects the kernel stack `__sys_stack`, sets IM1 with interrupts still disabled, and calls `_main`. If `_main` ever returns the CPU parks in a `HALT` loop.

## Fixed restart and NMI entries

The divIDE hardware maps esxDOS on instruction fetches at several fixed addresses. The replacement ROM therefore preserves the entry bytes required by the firmware:

- RST 08 begins with `LD HL,(0x5C5D)` and belongs to esxDOS file calls.
- RST 10 preserves HL around the guarded character sink. The fixed 48K ROM print entry at `0x09F4` jumps to the same wrapper. Output is ignored until RAM gates and the sink are initialized.
- RST 18 jumps to the writable YOS vector table and provides `query_service` to RAM processes.
- RST 20, RST 28, and RST 30 jump to their writable YOS vector slots.
- RST 38 is the exact esxDOS-compatible IM1 return sequence `PUSH AF; POP AF; EI; RETI`.
- NMI at `0x0066` preserves the firmware's leading `PUSH AF`; YOS does not install an NMI handler.
- `0x007B` contains `LD A,(HL); RET`, which lets mapped esxDOS read an inline RST 08 service selector from the base ROM.

RST 08 and NMI are firmware-owned. Their remaining bytes stay zero-filled. The linker script additionally reserves `0x04C6`, `0x0562` (divIDE automatic paging entry points) and `0x3D00-0x3DFF` (the Interface 1 trigger) so no kernel code is ever fetched from those addresses.

## Writable restart table

The Z80 restart instructions jump to fixed ROM addresses, and ROM cannot be changed. YOS therefore makes the RST 18-30 entries jump into `__sys_vec_tbl`, a 24-byte block in RAM holding eight three-byte `JP nn` instructions. `__startup_init` copies the immutable image `__sys_vectors_start` from its fixed ROM slot into it; the default image points every slot at `__sys_reti` (the NMI slot at `__sys_retn`).

The public `get_interrupt_handler` and `set_interrupt_handler` entries of the `yos_t` table (kernel routines `_sys_vec_get` and `_sys_vec_set`) read and patch bytes 1 and 2 of one entry. The slot numbers are the `enum yos_vector` values in `yos.h`:

| Slot | Vector | Use |
|---:|---|---|
| 2 | `YOS_VECTOR_RST18` | named-service lookup; installed by `main` |
| 3 | `YOS_VECTOR_RST20` | free for applications |
| 4 | `YOS_VECTOR_RST28` | free for applications |
| 5 | `YOS_VECTOR_RST30` | free for applications |
| 6 | `YOS_VECTOR_RST38` | reserved; the scheduler does not use it |

Slots 0, 1 and 7 (RST 08, RST 10, NMI) exist in the table for uniformity but nothing in ROM jumps through them. RST 10 uses its fixed print wrapper.

```c
#include <yos.h>

yos_t *yos = (yos_t *)query_service("yos");

/* redirect the application-owned RST 0x20 slot to my_handler */
yos->set_interrupt_handler(my_handler, YOS_VECTOR_RST20);

/* read back the current handler address */
yos_handler_t h = yos->get_interrupt_handler(YOS_VECTOR_RST20);
```

Internally `_sys_vec_set` computes `__sys_vec_tbl + 3 * vector + 1` and stores
the little-endian address there. Both routines run inside a critical section
so a half-written address can never be executed. The compact setter also
removes its one-byte stack argument before tail-calling the matching leave:

```asm
_sys_vec_set::
        call    _enter_critical_section
        ex      de, hl                   ; handler
        pop     bc                       ; return address
        pop     hl                       ; vector and untouched caller byte
        dec     sp                       ; consume only the vector byte
        push    bc
        ld      c, l
        ld      b, #0
        ld      hl, #__sys_vec_tbl+1
        add     hl, bc
        add     hl, bc
        add     hl, bc                   ; base + 1 + 3*vector
        ld      (hl), e                 ; low byte of handler
        inc     hl
        ld      (hl), d                 ; high byte of handler
        jp      _leave_critical_section
```

## 50 Hz scheduling with IM2

The physical RST 38 entry must remain compatible with divIDE, so YOS does not put its scheduler there. Once disk loading and process creation are complete, `__im2_init` writes the address of `__thread_robin` into the two-byte `__im2_vector` at `0x5EFF`, loads `I=0x5E`, and selects interrupt mode 2. The Spectrum ULA supplies `0xFF` on the data bus during interrupt acknowledge, so the CPU reads the handler address from `0x5E00 + 0xFF = 0x5EFF`.

This costs two fixed RAM bytes (the `_IM2` area in `linker.lk`) rather than a 257-byte vector table. `_HEAP` begins immediately after it at `0x5F01`.

Every esxDOS call in `fs/_esxdos_calls.s` enters a nestable critical section
at the common gate dispatcher and leaves it only after firmware returns and
divIDE restores the YOS ROM. While firmware is mapped, the ROM address of
`__thread_robin` contains unrelated firmware code: IM2 must not run then.

Descriptor-backed calls hold an outer critical section from descriptor
validation or reservation through native I/O and the final state update.
Thus `open`, `close`, `read`, `write`, `lseek`, `fstat`, `fsync`, and directory
stream operations cannot race their own kernel records; append seek plus write
is one transaction. The inner firmware gate simply nests. Path-only calls have
no descriptor transaction, but their native gate is still protected. Loader
validation, CRC, relocation, and library initialization remain preemptible;
the whole-load try-lock is state, not one long interrupt mask.

## Kernel entry

`main.s` starts with interrupts disabled and performs, in order:

1. `mem_init(__sys_heap, 1024)` and `mem_init(__heap, 0xFFFF - __heap)`.
2. `tmr_install` registers `__clock_tick`, `__kbd_scan`, and `__mouse_scan`
   as kernel-owned callbacks with period zero, so all three fire on every tick.
3. `svc_register("yos", __yos)` and `svc_register("gpx", __gpx_service)`.
4. `boot_shell`, which calls `process_load("op.sys")` to load, validate, relocate and start the shell as an XPRG process on the current esxDOS drive.
5. `sys_vec_set(_svc_query_rst18, YOS_VECTOR_RST18)`.
6. `__im2_init`, then `EI`.
7. The idle `HALT` loop. From now on every frame interrupt runs the scheduler.

Steps 4 and 5 happen before IM2 preemption is armed. Critical-section exits
preserve the disabled interrupt state throughout boot; only the explicit
`EI` in step 6 enables scheduling.

## RAM initialisation (`__startup_init`)

Because *yos* runs from ROM, every writable variable must be copied to RAM before it is used. The linker keeps two segments for this:

| Segment | Location | Purpose |
|---|---|---|
| `_INITIALIZER` | ROM | initial values (the source) |
| `_INITIALIZED` | RAM at `0x5B00` | live variables (the destination) |

`__startup_init` (in `startup/_startup_init.s`) does four things:

```asm
__startup_init::
        ;; 1. zero BSS
        ld      bc, #l__BSS
        ...
        ldir
        ;; 2. initialize nonzero keyboard/clock state and RAM esxDOS gates
        ...
        ;; 3. copy the restart-vector image into RAM
        ld      hl, #__sys_vectors_start
        ld      de, #__sys_vec_tbl
        ld      bc, #24                 ; eight three-byte JP vectors
        ldir
        ret
```

The immutable 104-byte `__yos` table is published directly from ROM. Its order
matches `yos_t` in `yos.h`; applications receive it from `query_service("yos")`.

Startup generates twenty three-byte `RST 08; <selector>; RET` RAM gates from
a 20-byte selector table. Filesystem code calls these gates so the inline
selector is fetched from RAM while esxDOS is paged over the ROM. YOS still
waits until esxDOS finishes cold boot and returns through `0x0001` before it
touches writable data.

The syscall table and gate selectors occupy the otherwise unused replacement
ROM header range `0x0080..0x00ff`. The conventional executable entry remains
at `0x0100`; linked content ends at `0x4000` (exclusive), filling the 16 KiB ROM.

## Reference-counted critical sections

The Z80 `di` and `ei` instructions are a blunt instrument. A common pitfall arises when subroutines call each other:

```asm
subroutine_a:
        di
        call    subroutine_b            ; subroutine_b will re-enable interrupts!
        ;; ← interrupts are NOW ENABLED here, even though we called di above
        ei
        ret

subroutine_b:
        di
        ;; protected code ...
        ei                              ; this ei affects subroutine_a too
        ret
```

`subroutine_b` does not know it was called from inside a `di` block. When it executes `ei` on return, the code in `subroutine_a` that follows the call is no longer protected.

YOS records nesting and the original interrupt state in one byte,
`__interrupt_refcount`: bits 0–6 are the depth (maximum 127), and bit 7
remembers whether the outermost caller had interrupts enabled. Entry samples
IFF2 with `LD A,I`, disables interrupts and increments the depth. The final
matching leave executes `EI` only if that saved bit was set. An unmatched
leave is a no-op. Both routines preserve every register, including flags.

Consequently, nested syscalls are safe, and callbacks already running with
interrupts disabled stay disabled after a protected operation. Do not call
blocking operations or explicitly execute `EI` inside a critical section.

```c
yos->enter_critical_section();   /* depth 1; remember caller's IFF */
yos->enter_critical_section();   /* depth 2 */
shared_value = 7;
yos->leave_critical_section();   /* depth 1; still disabled */
yos->leave_critical_section();   /* depth 0; restore caller's IFF */
```

The ROM-saving `__critical_call` trampoline guards routines whose arguments
are entirely in registers. It arranges for the body's ordinary return to
leave the section and return to its caller. It adds one return word, preserves
primary argument registers, and clobbers alternate DE/HL. Never use it with
an unadjusted caller-stack argument layout or live alternate-register state.

## Memory layout at boot

After `_main` has armed the scheduler the address space looks like this (addresses from the current link map):

| Address | Region | Notes |
|---|---|---|
| `0x0000` | ROM header (256 bytes) | Reset, RST 08–38, NMI, `0x007B`; immutable 104-byte `__yos` table at `0x0080` |
| `0x0100` | ROM: kernel, drivers, fs, gpx | `_CODE`, `_CONST`, `_INITIALIZER`; must end below `0x4000` |
| `0x4000` | Screen bitmap and attributes | ULA |
| `0x5B00` | Zero-filled `_BSS` | File descriptors, errno, timer root |
| `0x5B24` | Kernel stack (512 bytes) | Grows down from `__sys_stack = 0x5D24` |
| `0x5D24` | `__sys_vec_tbl` (24 bytes), clock, keyboard, mouse and list roots | Writable RST table; esxDOS gates start at `0x5D78` |
| `0x5EFF` | `__im2_vector` (2 bytes) | IM2 handler address |
| `0x5F01` | `__sys_heap` (1024 bytes) | Kernel objects |
| `0x6301` | `__heap` | Processes, thread stacks, application data |
| `0xFFFF` | End of RAM | |

The kernel stack is only used before the scheduler starts and inside the idle loop; every thread runs on a stack allocated from `__heap` (see [Threads](THREADS.md)).

## NMOS IFF sample race

An interrupt accepted immediately after `LD A,I` can clear P/V on an NMOS
Z80 (also emulated by Fuse). At the critical-entry sample this incorrectly
looks like an interrupt-disabled caller; the eventual leave would keep IM2
disabled, stopping the clock, keyboard and mouse timers.

The scheduler calls `__critical_iff_repair` after saving AF and HL and before
switching threads. If the interrupted PC is exactly `__critical_iff_sampled`
(the `DI` following the sample), accepted IRQ proves that IFF was enabled.
The helper sets only P/V in that thread's saved AF. It preserves other flags,
registers and return addresses; ordinary disabled callers and nested sections
retain their previous behavior. No application-side interrupt workaround is
needed. The helper uses a full word address comparison, avoiding byte-address
relocation expressions. The kernel suite checks matching and neighboring PCs;
ALTO's YOS integration test also forces this IRQ boundary during live input.

The ROM build places the small critical entries before the `0x0562` paging
trap and orders the small object reaper before the process/thread query to
use the gap below `0x3D00`. The reserved ranges remain unchanged.


### Fixed print entry and final ROM checksum

ABI 3 reserves `0x09F4..0x09F6` for the esxDOS PRINT-OUT jump. The ROM
finishing script (`y/scripts/patch_rom.py`) validates the link map and unused
bytes before placing this jump and the small immutable tables in linker gaps:
`RETI` at `0x09F0`, `RETN` at `0x09F2`, the `gpx` name at `0x3CE1`, and
the 24-byte default vector image at `0x3CE5`. The timer adapter immediately
before the first gap returns, so its unused linker bridge can be replaced;
the live bridge at `0x3CFD` remains intact. Unexpected layout changes fail
the build rather than overwriting code.

Short relative branches and shared lookup exits recover the required space.
The finishing script hashes the final, patched 16384-byte image and writes
`yos-kernel.rom.sha256` alongside it. ABI 3's table has 52 entries; the new
`exec_command` and `set_print_hook` entries are at byte offsets 100 and 102.
