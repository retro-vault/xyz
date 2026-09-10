# The Boot Process

## Reset and esxDOS handoff

The current assembly kernel starts at `0x0000` in `y/src/z80/startup/crt0rom.s`. Its first five bytes are `DI`, `XOR A`, and `JP .init`. esxDOS resumes at `0x0001` during cold boot, so the `XOR A` and jump must stay at those exact addresses. The normal YOS entry begins at `0x0100`, after the complete firmware-compatible header.

```asm
        .org    0x0000
        di
        xor     a
        jp      .init
```

Startup first uses `0xFFFF` as temporary stack space, clears BSS, copies initialized data and the RAM vector table, then selects the kernel stack. Interrupts remain disabled while the kernel registers services and loads `shell.sys` from the current esxDOS drive.

## Fixed restart and NMI entries

The divIDE hardware maps esxDOS on instruction fetches at several fixed addresses. The replacement ROM therefore preserves the entry bytes required by the firmware:

- RST 08 begins with `LD HL,(0x5C5D)` and belongs to esxDOS file calls.
- RST 10 is an immediate `RET`, used by esxDOS boot text.
- RST 18 redirects to the writable YOS vector table and provides `query_service` to RAM processes.
- RST 20, RST 28, and RST 30 redirect to their writable YOS vector slots.
- RST 38 is the exact esxDOS-compatible IM1 return sequence `PUSH AF; POP AF; EI; RETI`.
- NMI at `0x0066` preserves the firmware's leading `PUSH AF`; YOS does not install an NMI handler.
- `0x007B` contains `LD A,(HL); RET`, which lets mapped esxDOS read an inline RST08 service selector from the base ROM.

RST 08 and NMI are firmware-owned. Their remaining bytes stay zero-filled.

## Writable restart table

`__startup_init` copies eight three-byte `JP` entries from ROM to `__sys_vec_tbl` in RAM. Public `get_interrupt_handler` and `set_interrupt_handler` operate on this table. The named-service bridge is installed in slot 2, corresponding to RST 18. RST 10 itself is fixed and is not exposed as a writable public vector.

## 50 Hz scheduling with IM2

The physical RST38 entry must remain compatible with divIDE, so YOS does not put its scheduler there. Once disk loading and process creation are complete, `__im2_init` writes `__thread_robin` to the two-byte vector at `0x5EFF`, loads `I=0x5E`, selects interrupt mode 2, and enables interrupts. The Spectrum ULA supplies `0xFF` on the interrupt bus, making the CPU fetch the scheduler address from `0x5EFF`.

This costs two fixed RAM bytes. `_HEAP` begins at `0x5F01`; the gap below the IM2 word is ordinary linker alignment after kernel BSS, rather than a 257-byte vector table.

## Kernel entry

The kernel entry sequence is:

1. Initialize BSS, writable data, syscall tables, and the kernel and process heaps.
2. Install the clock and keyboard timers.
3. Register the `yos` and `gpx` services.
4. Load, validate, relocate, and start `shell.sys` as an XPRG process.
5. Install the RST18 service bridge.
6. Select IM2 and enable interrupts.
7. Remain in the `HALT` idle loop while the scheduler runs processes.

## RAM Initialisation (`gsinit`)

Because *yos* runs from ROM, every global C variable must be *copied* to RAM at startup — they cannot be modified if they live in ROM. SDCC uses two special linker segments for this:

| Segment | Location | Purpose |
|---|---|---|
| `_INITIALIZER` | ROM | Initial values (the source) |
| `_INITIALIZED` | RAM | Live variables (the destination) |

The `gsinit` routine copies `_INITIALIZER` → `_INITIALIZED` and also installs the default RST vector table. The routine is placed in the `_GSINIT` segment (by SDCC convention) and ends in `_GSFINAL`:

```asm
        .area   _GSINIT
gsinit:
        ;; copy vector table from ROM to RAM
        ld      hl,#start_vectors       ; source in ROM
        ld      de,#__sys_vec_tbl       ; destination in RAM
        ld      bc,#end_vectors - #start_vectors
        ldir                            ; block copy

        ;; copy initialized global variables from ROM to RAM
        ld      de, #s__INITIALIZED     ; destination
        ld      hl, #s__INITIALIZER     ; source
        ld      bc, #l__INITIALIZER     ; byte count
        ld      a, b
        or      a, c
        jr      z, gsinit_none          ; skip if no initialised data
        ldir
gsinit_none:
        .area   _GSFINAL
        ret
```

## Reference-Counted Critical Sections

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

*Yos* solves this with a reference counter. `enter_critical_section()` increments the counter and executes `di`. `leave_critical_section()` decrements the counter, and only executes `ei` when the counter reaches zero:

```asm
_enter_critical_section::
        di
        push    hl
        ld      hl,#ir_refcount
        inc     (hl)                    ; one more nested disable
        pop     hl
        ret

_leave_critical_section::
        push    af
        ld      a,(#ir_refcount)
        or      a
        jr      z,do_ei                 ; already at zero: just enable
        dec     a
        ld      (#ir_refcount),a        ; decrement the counter
        or      a
        jr      nz,dont_ei              ; still nested: stay disabled
do_ei:
        ei                              ; safe to enable now
dont_ei:
        pop     af
        ret
```

With this scheme, the earlier example becomes safe:

```c
enter_critical_section();   // refcount = 1, di executed
enter_critical_section();   // refcount = 2, di again (no-op)
// ... protected code ...
leave_critical_section();   // refcount = 1, still disabled
leave_critical_section();   // refcount = 0, ei executed
```

The exported C prototypes are `void enter_critical_section()` and `void leave_critical_section()`.

## Installing and Reading RST Vector Handlers

The `sys_vec_set()` and `sys_vec_get()` functions read and write the RAM vector table. Vector numbers are defined as constants in `vectors.h` (`RST08` = 0 through `NMI` = 7).

```c
#include <yos.h>

/* redirect the application-owned RST 0x20 slot to my_handler */
set_interrupt_handler(my_handler, YOS_VECTOR_RST20);

/* read back the current handler address */
yos_handler_t h = get_interrupt_handler(YOS_VECTOR_RST20);
```

Internally, each entry in `__sys_vec_tbl` is a 3-byte `jp nn` instruction. `sys_vec_set` patches bytes 1 and 2 of the relevant entry with the new address (little-endian), leaving the `jp` opcode (byte 0) untouched. Both functions disable interrupts during the update to prevent a half-written address from being executed.

```asm
_sys_vec_set::
        call    _enter_critical_section
        ;; ... pop handler address into BC, vector number into E ...
        ld      d,#0x00
        ld      hl,#__sys_vec_tbl
        add     hl,de
        add     hl,de
        add     hl,de                   ; HL = base + 3*vector_number
        inc     hl                      ; skip the jp opcode byte
        ld      (hl),c                  ; write low byte of handler
        inc     hl
        ld      (hl),b                  ; write high byte of handler
        call    _leave_critical_section
        ret
```

## Memory Layout at Boot Time

After `init` completes and `main()` begins, the address space looks like this:

```
0x0000 ┌────────────────────────────────┐
       │  ROM: yos code                 │  read-only
       │  (crt0rom.s, kernel, drivers)  │
       ├────────────────────────────────┤
       │  RAM: BSS segment              │
       │    __sys_vec_tbl  (24 bytes)   │  RST vector table (RAM copy)
       │    __sys_stack    (512 bytes)  │  OS kernel stack
       │    __sys_heap     (1024 bytes) │  OS heap
       │    __heap → ...                │  user heap (rest of RAM)
       ├────────────────────────────────┤
       │  RAM: INITIALIZED segment      │  global C variables
       └────────────────────────────────┘ 0xFFFF
```

The kernel heap starts at `0x5F01` after the fixed IM2 word and reserves its first 1024 bytes for kernel allocations. The process heap follows it and extends to the top of RAM. Each user thread gets its own declared stack allocation from that heap.
