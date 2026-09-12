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
- RST 10 is an immediate `RET`, used by esxDOS boot text.
- RST 18 jumps to the writable YOS vector table and provides `query_service` to RAM processes.
- RST 20, RST 28, and RST 30 jump to their writable YOS vector slots.
- RST 38 is the exact esxDOS-compatible IM1 return sequence `PUSH AF; POP AF; EI; RETI`.
- NMI at `0x0066` preserves the firmware's leading `PUSH AF`; YOS does not install an NMI handler.
- `0x007B` contains `LD A,(HL); RET`, which lets mapped esxDOS read an inline RST 08 service selector from the base ROM.

RST 08 and NMI are firmware-owned. Their remaining bytes stay zero-filled. The linker script additionally reserves `0x04C6`, `0x0562` (divIDE automatic paging entry points) and `0x3D00-0x3DFF` (the Interface 1 trigger) so no kernel code is ever fetched from those addresses.

## Writable restart table

The Z80 restart instructions jump to fixed ROM addresses, and ROM cannot be changed. YOS therefore makes the RST 18-30 entries jump into `__sys_vec_tbl`, a 24-byte block in RAM holding eight three-byte `JP nn` instructions. `__startup_init` copies the immutable image `__sys_vectors_start` from `_CONST` into it; the default image points every slot at `__sys_reti` (the NMI slot at `__sys_retn`).

The public `get_interrupt_handler` and `set_interrupt_handler` entries of the `yos_t` table (kernel routines `_sys_vec_get` and `_sys_vec_set`) read and patch bytes 1 and 2 of one entry. The slot numbers are the `enum yos_vector` values in `yos.h`:

| Slot | Vector | Use |
|---:|---|---|
| 2 | `YOS_VECTOR_RST18` | named-service lookup; installed by `main` |
| 3 | `YOS_VECTOR_RST20` | free for applications |
| 4 | `YOS_VECTOR_RST28` | free for applications |
| 5 | `YOS_VECTOR_RST30` | free for applications |
| 6 | `YOS_VECTOR_RST38` | reserved; the scheduler does not use it |

Slots 0, 1 and 7 (RST 08, RST 10, NMI) exist in the table for uniformity but nothing in ROM jumps through them.

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
2. `tmr_install(__clock_tick, 0, NONE)` and `tmr_install(__kbd_scan, 0, NONE)` — both fire on every tick.
3. `svc_register("yos", __yos)` and `svc_register("gpx", __gpx_service)`.
4. `boot_shell`, which calls `process_load("shell.sys")` to load, validate, relocate and start the shell as an XPRG process on the current esxDOS drive.
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
        ;; 2. copy the restart-vector image into RAM
        ld      hl, #__sys_vectors_start
        ld      de, #__sys_vec_tbl
        ld      bc, #24                 ; eight three-byte JP vectors
        ldir
        ;; 3. copy initialised variables from ROM to RAM
        ld      de, #s__INITIALIZED
        ld      hl, #s__INITIALIZER
        ld      bc, #l__INITIALIZER
        ...
        ldir
        ;; 4. build the public service table
        jp      __syscall_table_init
```

`__syscall_table_init` copies the 96-byte `.yos_template` (the ordered list of kernel entry points that matches `yos_t` in `yos.h`) into `__yos` in BSS. Applications never see the template; they receive `__yos` from `query_service("yos")`.

The esxDOS RAM gates (`fs/_esxdos_gates.s`) are also `_INITIALIZED` data: nineteen three-byte `RST 08; <selector>; RET` stubs (57 bytes at `0x5B37`) that the filesystem calls so the inline selector byte is fetched from RAM while esxDOS is paged over the ROM. This is why YOS must not touch its writable data until esxDOS has finished its own cold boot and returned through `0x0001`.

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

```
0x0000 ┌────────────────────────────────┐
       │ ROM header (256 bytes)         │  reset, RST 08-38, NMI, 0x007B
0x0100 ├────────────────────────────────┤
       │ ROM: kernel, drivers, fs, gpx  │  _CODE, _CONST, _INITIALIZER
       │                                │  must end below 0x4000
0x4000 ├────────────────────────────────┤
       │ screen bitmap + attributes     │
0x5B00 ├────────────────────────────────┤
       │ _INITIALIZED (copied from ROM) │  clock counters, keyboard state,
       │                                │  esxDOS gates at 0x5B37
0x5B70 │ _BSS                           │
       │   fd/error/timer state         │
0x5B94 │   __yos            96 bytes    │  public ABI 1 service table
       │   kernel stack    512 bytes    │  grows down to here from 0x5DF4
0x5DF4 │   __sys_vec_tbl    24 bytes    │  writable RST table
       │   list roots, mouse state, ...   │
0x5EFF │ __im2_vector        2 bytes    │  IM2 handler address
0x5F01 ├────────────────────────────────┤
       │ __sys_heap       1024 bytes    │  kernel objects
0x6301 ├────────────────────────────────┤
       │ __heap                         │  processes, thread stacks,
       │                                │  application data
0xFFFF └────────────────────────────────┘
```

The kernel stack is only used before the scheduler starts and inside the idle loop; every thread runs on a stack allocated from `__heap` (see [Threads](THREADS.md)).
