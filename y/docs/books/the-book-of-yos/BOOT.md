# The Boot Process

## Reset and esxDOS handoff

The kernel starts at `0x0000` in `y/src/z80/startup/crt0rom.s`. Its first
five bytes are `DI`, `XOR A`, and `JP .init`. esxDOS resumes at `0x0001`
during cold boot, so the `XOR A` and the jump must stay at those exact
addresses. Ordinary YOS execution begins at `0x0100`, after the complete
firmware-compatible header.

```asm
        .org    0x0000
        di
        xor     a
        jp      .init
```

`.init` first uses `0xFFFF` as temporary stack space and calls
`__startup_init` (described below). It then selects the kernel stack
`__sys_stack`, sets IM1 with interrupts still disabled, and calls `_main`.
If `_main` ever returns, the CPU parks in a `HALT` loop.

## Fixed restart and NMI entries

The divIDE hardware maps esxDOS in on instruction fetches at several fixed
addresses, so the replacement ROM must preserve the entry bytes the firmware
expects:

- RST 08 begins with `LD HL,(0x5C5D)` and belongs to esxDOS file calls.
- RST 10 preserves HL around the guarded character sink. The fixed 48K ROM
  print entry at `0x09F4` jumps to the same wrapper. Output is ignored until
  RAM gates and the sink are initialized.
- RST 18 jumps to the writable YOS vector table and provides
  `query_service` to RAM processes.
- RST 20 is the inline far-call/jump gate, RST 28 is XCC's dynamic far-call
  gate, and RST 30 remains available through its writable vector slot.
- RST 38 is the exact esxDOS-compatible IM1 return sequence
  `PUSH AF; POP AF; EI; RETI`.
- NMI at `0x0066` preserves the firmware's leading `PUSH AF`; YOS installs
  no NMI handler of its own.
- `0x007B` contains `LD A,(HL); RET`, which lets mapped esxDOS read an
  inline RST 08 service selector from the base ROM.

RST 08 and NMI are firmware-owned, so their remaining bytes stay
zero-filled. The linker script additionally reserves `0x04C6` and `0x0562`
(the divIDE automatic paging entry points) and `0x3D00`–`0x3DFF` (the
Interface 1 trigger), so kernel code is never fetched from any of those
addresses.

The linker also reserves patch-owned bytes explicitly: `0x09F0`–`0x09F6`
holds RETI, RETN, and PRINT-OUT; `0x3CE1`–`0x3CF8` holds the restart-vector
image, inside the reservation that ends at `0x3DFF`. `patch_rom.py` checks
these slots before installing their contents — they are not free space, and
neither are the linker's bridges around the reserved regions.

The current universal ROM ends at `s__GSFINAL = 0x3FDF`, leaving 33
contiguous zero-filled bytes through `0x3FFF`. The build prints the current
tail size and rejects any content that crosses the 16 KiB ROM boundary. It
also checks every linked ROM area against the fixed reservations, because
zero-valued live code or data is not free space either — use the occupied
end, not `l__CODE` or a run of zeros, when measuring how much ROM is
actually available. Shared frame helpers trade entry/exit cycles for size
without changing IX-relative locals or the public ABI.

## Writable restart table

The Z80 restart instructions jump to fixed ROM addresses, and ROM cannot be
changed. YOS works around this by making the RST 18–30 entries jump into
`__sys_vec_tbl`, a 24-byte block in RAM holding eight three-byte `JP nn`
instructions. `__startup_init` copies the immutable image
`__sys_vectors_start` from its fixed ROM slot into that table; the default
image points every slot at `__sys_reti` (the NMI slot points at
`__sys_retn`).

The public `get_interrupt_handler` and `set_interrupt_handler` entries of
the `yos_t` table — backed by kernel routines `_sys_vec_get` and
`_sys_vec_set` — read and patch bytes 1 and 2 of one entry. The slot
numbers are the `enum yos_vector` values in `yos.h`:

| Slot | Vector | Use |
|---:|---|---|
| 2 | `YOS_VECTOR_RST18` | named-service lookup; installed by `main` |
| 3 | `YOS_VECTOR_RST20` | inline banked call/jump; installed by `main` |
| 4 | `YOS_VECTOR_RST28` | dynamic XCC far call; installed by `main` |
| 5 | `YOS_VECTOR_RST30` | far-data byte access; installed by `main` |
| 6 | `YOS_VECTOR_RST38` | reserved; the scheduler does not use it |

Slots 0, 1, and 7 (RST 08, RST 10, NMI) exist in the table for uniformity,
but nothing in ROM jumps through them — RST 10 uses its fixed print wrapper
instead.

```c
#include <yos.h>

yos_t *yos = (yos_t *)query_service("yos");

/* redirect the application-owned RST 0x20 slot to my_handler */
yos->set_interrupt_handler(my_handler, YOS_VECTOR_RST20);

/* read back the current handler address */
yos_handler_t h = yos->get_interrupt_handler(YOS_VECTOR_RST20);
```

Internally, `_sys_vec_set` computes `__sys_vec_tbl + 3 * vector + 1` and
stores the little-endian address there. Both routines run inside a critical
section, so a half-written address can never be executed. The compact
setter also removes its one-byte stack argument before tail-calling the
matching leave:

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

The physical RST 38 entry has to remain compatible with divIDE, so YOS does
not put its scheduler there. Once disk loading and process creation are
complete, `__im2_init` writes the address of `__thread_robin` into the
two-byte `__im2_vector` at `0x5EFF`, loads `I=0x5E`, and selects interrupt
mode 2. The Spectrum ULA supplies `0xFF` on the data bus during interrupt
acknowledge, so the CPU reads the handler address from
`0x5E00 + 0xFF = 0x5EFF`.

This trick costs two fixed RAM bytes — the `_IM2` area in `linker.lk` —
rather than a 257-byte vector table. `_HEAP` begins immediately after it,
at `0x5F01`.

Every esxDOS call in `fs/_esxdos_calls.s` enters a nestable critical section
at the common gate dispatcher and leaves it only after firmware returns and
divIDE restores the YOS ROM. While firmware is mapped, the ROM address of
`__thread_robin` contains unrelated firmware code, so IM2 must not run
during that window.

Descriptor-backed calls hold an outer critical section from descriptor
validation or reservation through native I/O and the final state update.
That means `open`, `close`, `read`, `write`, `lseek`, `fstat`, `fsync`, and
the directory stream operations can never race their own kernel records —
an append seek plus write is one transaction. The inner firmware gate
simply nests inside it. Path-only calls have no descriptor transaction, but
their native gate is still protected. Loader validation, CRC, relocation,
and library initialization all remain preemptible; the whole-load try-lock
is ordinary state, not one long interrupt mask.

## Kernel entry

`main.s` starts with interrupts disabled and performs, in order:

1. Initialize the aliased `__sys_heap`/`__heap` OS arena through `0xBFFF`,
   and every configured bank arena at `0xC000`–`0xFFFF`; expand the packed
   font into an OS-owned common allocation.
2. Call `tmr_install` to register `__clock_tick`, `__kbd_scan`, and
   `__mouse_scan` as kernel-owned callbacks with period zero, so all three
   fire on every tick.
3. Call `svc_register("yos", __yos)` to publish the single OS interface,
   including graphics.
4. Call `boot_shell`, which calls `process_load("shell.sys")` to load,
   validate, relocate, and start the shell as an XPRG process on the
   current esxDOS drive.
5. Install the RST18 service query and the RST20/RST28 far-call gates.
6. Call `__im2_init`, then `EI`.
7. Enter the idle `HALT` loop. From this point on, every frame interrupt
   runs the scheduler.

Steps 4 and 5 happen before IM2 preemption is armed. Critical-section exits
preserve the disabled interrupt state throughout boot; only the explicit
`EI` in step 6 enables scheduling.

## RAM initialisation (`__startup_init`)

Because *yos* runs from ROM, every writable variable has to be copied to
RAM before it can be used. The linker keeps two segments for this:

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

The immutable 152-byte `__yos` table is published directly from ROM. Its
order matches ABI 1's `yos_t` in `yos.h` and the offsets in `yos.inc`;
applications receive it from `query_service("yos")`.

Startup also generates twenty three-byte `RST 08; <selector>; RET` RAM
gates from a 20-byte selector table. Filesystem code calls these gates so
that the inline selector is fetched from RAM while esxDOS is paged in over
the ROM. YOS still waits for esxDOS to finish cold boot and return through
`0x0001` before it touches writable data at all.

Once the fixed OS heap is initialized, `_main` calls `__bank_detect`. The
probe first recognizes the exact NextReg `0x00` IDs `0x08`, `0x0A`, and
`0xFA`; failing that, it temporarily distinguishes 7FFD pages 0 and 1,
restores the one modified byte, and leaves page 0 mapped. On a 128K
machine, every probe write retains the 48 BASIC ROM slot that ESXIDE uses
to boot YOS. The probe stores the resulting 48K, 128K, or Next model byte
and patches a three-byte `JP nn` mapper trampoline into fixed RAM before
the bank heaps are initialized.

The syscall table and gate selectors occupy the otherwise unused
replacement-ROM header range `0x0080`–`0x00FF`. The conventional executable
entry remains at `0x0100`; linked content ends at `0x4000` (exclusive),
filling the full 16 KiB ROM.

## Reference-counted critical sections

The Z80 `DI` and `EI` instructions are a blunt instrument. A common pitfall
arises when subroutines call each other:

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

`subroutine_b` has no way of knowing it was called from inside a `DI`
block. When it executes `EI` on return, the code in `subroutine_a` that
follows the call is no longer protected.

YOS records nesting depth and the original interrupt state in one byte,
`__interrupt_refcount`: bits 0–6 hold the depth (maximum 127), and bit 7
remembers whether the outermost caller had interrupts enabled. Entry
samples IFF2 with `LD A,I`, disables interrupts, and increments the depth.
The final matching leave executes `EI` only if that saved bit was set; an
unmatched leave is a no-op. Both routines preserve every register,
including flags.

As a result, nested syscalls are safe, and callbacks that are already
running with interrupts disabled stay disabled after a protected operation
returns. Do not call blocking operations, and never execute a bare `EI`,
inside a critical section.

```c
yos->enter_critical_section();   /* depth 1; remember caller's IFF */
yos->enter_critical_section();   /* depth 2 */
shared_value = 7;
yos->leave_critical_section();   /* depth 1; still disabled */
yos->leave_critical_section();   /* depth 0; restore caller's IFF */
```

The ROM-saving `__critical_call` trampoline guards routines whose arguments
live entirely in registers. It arranges for the body's ordinary return to
leave the section and return to its caller, at the cost of one extra return
word; it preserves the primary argument registers but clobbers the
alternate DE/HL pair. Never use it with an unadjusted caller-stack argument
layout or with live state in the alternate registers.

## Memory layout at boot

After `_main` has armed the scheduler, the address space looks like this
(addresses taken from the current link map):

| Address | Region | Notes |
|---|---|---|
| `0x0000` | ROM header (256 bytes) | Reset, RST 08–38, NMI, `0x007B`; the unified table is linked in `_CONST` |
| `0x0100` | ROM: kernel, drivers, fs, gpx | `_CODE`, `_CONST`, `_INITIALIZER`; must end below `0x4000` |
| `0x4000` | Screen bitmap and attributes | ULA |
| `0x5B00` | Zero-filled `_BSS` | File descriptors, errno, timer root |
| `0x5B30` | Bank-map trampoline (3 bytes) | Patched once with `JP` to the detected model's mapper |
| `0x5B37` | Kernel stack (512 bytes) | Grows down from `__sys_stack = 0x5D37` |
| `0x5D37` | `__sys_vec_tbl` (24 bytes), clock, keyboard, mouse and list roots | Writable RST table |
| `0x5EFF` | `__im2_vector` (2 bytes) | IM2 handler address |
| `0x5F01` | `__sys_heap` / `__heap` | All fixed OS objects and stacks through `0xBFFF` |
| `0xC000` | Selected user-bank arena | Process/library images and user allocations through `0xFFFF` |

The kernel stack is used only before the scheduler starts and inside the
idle loop; every thread runs on an OS-heap stack allocated from the same
fixed `__sys_heap`/`__heap` arena (see [Threads](THREADS.md)).

## NMOS IFF sample race

An interrupt accepted immediately after `LD A,I` can clear P/V on an NMOS
Z80 — a quirk Fuse also emulates. At the critical-entry sample, that looks
exactly like an interrupt-disabled caller, and the eventual leave would
then keep IM2 disabled, silently stopping the clock, keyboard, and mouse
timers.

The scheduler calls `__critical_iff_repair` after saving AF and HL and
before switching threads. If the interrupted PC is exactly
`__critical_iff_sampled` (the `DI` following the sample), an accepted IRQ
proves that IFF was actually enabled. The helper sets only P/V in that
thread's saved AF, preserving every other flag, register, and return
address — ordinary disabled callers and nested sections keep their
previous behavior unchanged, and no application-side interrupt workaround
is needed. The helper compares full-word addresses, avoiding
byte-address relocation expressions. The kernel test suite checks matching
and neighboring PCs, and the Alto YOS integration test also forces this IRQ
boundary during live input.

The ROM build places the small critical entries before the `0x0562` paging
trap and orders the small object reaper before the process/thread query, so
that both fit in the gap below `0x3D00`. The reserved ranges themselves
remain unchanged.

### Fixed print entry and final ROM checksum

ABI 4 reserves `0x09F4`–`0x09F6` for the esxDOS PRINT-OUT jump. The ROM
finishing script (`y/scripts/patch_rom.py`) validates the link map and the
unused bytes before placing this jump and the small immutable tables in the
linker's gaps: `RETI` at `0x09F0`, `RETN` at `0x09F2`, and the 24-byte
default vector image at `0x3CE1`. The timer adapter immediately before the
first gap returns, so its unused linker bridge can be replaced, while the
live bridge at `0x3CF9` remains intact. Any unexpected layout change fails
the build rather than silently overwriting code.

Short relative branches and shared lookup exits recover the space this
needs. The finishing script hashes the final, patched 16384-byte image and
writes `yos-kernel.rom.sha256` alongside it. ABI 1's table has 76 grouped
entries and occupies 152 bytes: `version`, `rom_model`, `get_sys_info`, and
`set_print_hook` sit at byte offsets 0, 2, 4, and 6; `exec_command` is at byte
offset 106, and the 22 graphics entries occupy byte offsets 108 through 150.
