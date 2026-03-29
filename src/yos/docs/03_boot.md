# The Boot Process

## Z80 Power-Up

When the Z80 microprocessor is powered up or reset, it unconditionally begins executing code at address `0x0000`. On the ZX Spectrum this address is in ROM, which is where *yos* lives. The entry point is defined in [crt0rom.s](https://github.com/tstih/xyz/blob/main/src/yos/startup/crt0rom.s).

The very first thing the operating system does is disable interrupts, then immediately jump to the `init` subroutine. The two instructions are followed by four reserved bytes (intended for a future version field). Together these form exactly 8 bytes — which is important because, as we will see below, the Z80 restart handlers are spaced exactly 8 bytes apart.

```asm
        .org    0x0000
        di                              ; disable interrupts
        jp      init                    ; jump to init subroutine
        .db     0,0,0,0                 ; reserved (version placeholder)
```

Interrupts are disabled immediately because the system is not yet initialised. If a 50 Hz timer interrupt were to fire before the stack is set up, the CPU would push the return address to an undefined location and almost certainly crash.

## RST Vectors

The Z80 has eight single-byte *restart* instructions: `RST 0x00`, `RST 0x08`, `RST 0x10`, `RST 0x18`, `RST 0x20`, `RST 0x28`, `RST 0x30`, and `RST 0x38`. Each one behaves like a fast `CALL` — it pushes the current program counter onto the stack and jumps to a fixed address determined by the operand. Because the opcode is only one byte, the upper byte of the destination address is always `0x00`, so `RST 0x18` jumps to `0x0018`.

Restarts are useful for frequently-called routines because a single-byte `RST` instruction is much more compact and faster than a three-byte `CALL nn`.

Since the restart destinations are at addresses `0x0000`, `0x0008`, `0x0010`, ..., `0x0038`, and the first slot is already taken by the boot code, *yos* fits one handler per 8-byte slot starting at `0x0008`.

```asm
        ;; rst 0x08 at address 0x0008
        jp      rst8                    ; redirect to RAM handler
rst8ret:
        reti                            ; return from interrupt
        .db     0,0,0                   ; padding to fill 8 bytes

        ;; rst 0x10 at address 0x0010
        jp      rst10
rst10ret:
        reti
        .db     0,0,0

        ;; rst 0x18 at address 0x0018
        jp      rst18
rst18ret:
        reti
        .db     0,0,0
        ;; ... pattern repeats through 0x0038 ...
```

Each 8-byte slot consists of:
- a 3-byte `jp` instruction that redirects execution to a RAM address,
- a 2-byte `reti` as the default return path, and
- 3 bytes of padding.

The critical thing to notice is that each `jp` goes to a label in **RAM**, not ROM. This is what makes the vector table reconfigurable at runtime.

## RST Vectors Jump to RAM

*Yos* runs from ROM, which cannot be modified. Yet many features — timers, keyboard drivers, multitasking — need to hook into interrupt and restart vectors. The solution is a small *vector table* in RAM.

At startup, *yos* copies a default jump table from ROM into a reserved area of RAM labelled `__sys_vec_tbl`. Each entry is exactly 3 bytes — the size of a `jp nn` instruction:

```asm
__sys_vec_tbl::
rst8:   .ds     3               ; 3 bytes for "jp <handler>"
rst10:  .ds     3
rst18:  .ds     3
rst20:  .ds     3
rst28:  .ds     3
rst30:  .ds     3
rst38:  .ds     3
nmi:    .ds     3
```

The default values copied into these slots are jump-back instructions that return immediately to the ROM `reti` for each vector:

```asm
start_vectors:
        jp      rst8ret         ; default: do nothing for RST 0x08
        jp      rst10ret
        jp      rst18ret
        jp      rst20ret
        jp      rst28ret
        jp      rst30ret
        jp      rst38ret
        jp      nmiret
end_vectors:
```

So when `RST 0x08` fires, the chain of events is:

```
RST 0x08
  → ROM 0x0008: jp rst8        (in ROM, goes to RAM)
  → RAM rst8:   jp rst8ret     (in RAM, default: goes back to ROM)
  → ROM rst8ret: reti          (returns from the call)
```

Because `rst8` in RAM is a variable jump instruction, you can change its target address at any time to intercept `RST 0x08`. This is exactly how the thread scheduler is installed: it replaces the default `rst38` handler with `jp __thread_robin`.

## Special Hardware Interrupts

### The 50 Hz Screen Refresh (RST 0x38)

The ZX Spectrum's ULA chip generates a hardware interrupt 50 times per second (once per video frame) when it redraws the screen. In interrupt mode 1 (IM 1), which *yos* uses, this interrupt always calls address `0x0038` — the `RST 0x38` slot.

*Yos* uses this 50 Hz tick heavily: it drives the timer subsystem, keyboard scanning, cursor blinking, and the preemptive thread scheduler.

> **Warning:** Do not hook `RST38` directly unless you understand the consequences. The thread scheduler and timers depend on it. If your program needs periodic callbacks, use the **timer API** (`tmr_install`) instead — it is designed exactly for this purpose.

### The NMI Interrupt

The Non-Maskable Interrupt (NMI) fires at address `0x0066` and, unlike ordinary interrupts, cannot be masked by the `di` instruction. It uses `retn` instead of `reti` to return. The standard ZX Spectrum hardware does not generate NMIs, but some expansion hardware does. *Yos* installs a default NMI handler that immediately returns.

## The `init` Subroutine

After the boot jump at `0x0000`, execution arrives at `init`. This routine performs all low-level initialisation before handing control to the C `main()` function:

1. **Set up the OS stack.** The stack pointer `SP` is loaded with `__sys_stack`, a 512-byte region in BSS. This ensures any subsequent `CALL` or interrupt has a valid stack to use.
2. **Copy RAM initialisation data** (`gsinit`). Global C variables with initial values, and the RST vector table, are copied from ROM to RAM.
3. **Enter interrupt mode 1** and **enable interrupts**. From this point the 50 Hz timer interrupt is live.
4. **Call `_main()`** — the C entry point of the operating system.
5. **Tarpit.** If `main()` ever returns (which it normally does, after setting up the scheduler), execution falls into an infinite `halt` loop. The processor will keep waking on interrupts but never advance beyond this loop — it becomes the idle task.

```asm
init:
        ld      sp,#__sys_stack         ; point SP at the OS stack
        call    gsinit                  ; copy ROM→RAM data

        im      1                       ; interrupt mode 1 (50 Hz)
        ei                              ; enable interrupts

        call    _main                   ; run the OS

tarpit:
        halt                            ; wait for next interrupt
        jr      tarpit                  ; loop forever
```

The tarpit is not just filler — it is the *idle state* of the system. Once `main()` installs the thread scheduler into `RST38` and returns, the tarpit's `halt` becomes the point where execution lands between scheduler ticks. Each 50 Hz interrupt wakes the processor, saves the idle context, and switches to a runnable thread. When no thread is ready, execution falls back to the tarpit.

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

## Reference-Counted Interrupt Enable/Disable

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

*Yos* solves this with a reference counter. `ir_disable()` increments the counter and executes `di`. `ir_enable()` decrements the counter, and only executes `ei` when the counter reaches zero:

```asm
_ir_disable::
        di
        push    hl
        ld      hl,#ir_refcount
        inc     (hl)                    ; one more nested disable
        pop     hl
        ret

_ir_enable::
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
ir_disable();           // refcount = 1, di executed
ir_disable();           // refcount = 2, di again (no-op)
// ... protected code ...
ir_enable();            // refcount = 1, still disabled
ir_enable();            // refcount = 0, ei executed
```

The exported C prototypes are `void ir_disable()` and `void ir_enable()`.

## Installing and Reading RST Vector Handlers

The `sys_vec_set()` and `sys_vec_get()` functions read and write the RAM vector table. Vector numbers are defined as constants in `vectors.h` (`RST08` = 0 through `NMI` = 7).

```c
#include <kernel/vectors.h>

/* redirect RST 0x10 to my_handler */
sys_vec_set(my_handler, RST10);

/* read back the current handler address */
void (*h)() = sys_vec_get(RST10);
```

Internally, each entry in `__sys_vec_tbl` is a 3-byte `jp nn` instruction. `sys_vec_set` patches bytes 1 and 2 of the relevant entry with the new address (little-endian), leaving the `jp` opcode (byte 0) untouched. Both functions disable interrupts during the update to prevent a half-written address from being executed.

```asm
_sys_vec_set::
        call    _ir_disable
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
        call    _ir_enable
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

The OS stack and OS heap are fixed-size areas declared in `crt0rom.s`. The user heap (`__heap`) extends from the end of the OS heap all the way to the top of RAM. Each user thread gets its own stack allocated from the user heap.
