# Banked Processes and Libraries

YOS keeps the complete kernel and all shared runtime state below `0xC000`.
The upper 16 KiB, `0xC000`–`0xFFFF`, is a logical executable-bank window.
Process and library XPRG images, along with public user allocations, are
packed into independent user-heap arenas behind that window. Every OS
object — including thread stacks, service records, and bound call tables —
stays in the single fixed OS heap below `0xC000`.

## Detection and configured capacity

Every ROM contains all three mappers. At boot, YOS reads NextReg `0x00`
first and recognizes the exact machine IDs `0x08`, `0x0A`, and `0xFA`. If
the machine is not a Next, it runs a reversible page-0/page-1 test through
port `0x7FFD`; failing to observe distinct pages selects 48K. The detected
model byte is kept in fixed RAM and exposed as `yos->rom_model()`.

`YOS_BANK_COUNT` sets the compiled capacity and defaults to 126, the
maximum safe Next count. `YOS_BANK_BACKEND` selects the hardware model the
emulator test fixture expects; it does not remove the other mappers from
the ROM:

```sh
make -C y/src/z80 YOS_BANK_BACKEND=48 YOS_BANK_COUNT=1
make -C y/src/z80 YOS_BANK_BACKEND=128 YOS_BANK_COUNT=6
make -C y/src/z80 YOS_BANK_BACKEND=next YOS_BANK_COUNT=126
```

The detected 48K model always uses one logical bank. It runs the complete
bank-aware allocator and call ABI, but its mapper only ever records bank
zero. The 128K model uses at most six configured banks and maps logical
banks to physical pages `0,1,3,4,6,7` through `0x7FFD`; pages 5 and 2 stay
fixed below the window. Because ESXIDE boots YOS from the 48 BASIC ROM
slot, YOS keeps `0x7FFD` ROM-select bit 4 set while leaving the screen and
paging-disable bits clear. The Next maps each logical bank to an adjacent
MMU-page pair in slots 6 and 7 using NextRegs `0x56` and `0x57`, skipping
physical 16 KiB pages 2 and 5 because those back the fixed RAM below
`0xC000`. The paired selector/data writes are protected by a nestable
critical section.

Bank identifiers are seven bits, because bit 7 is reserved as the inline
far-jump flag. That leaves 1 bank on 48K, up to 6 usable banks on 128K, and
up to 126 safe logical banks on a 2 MiB Next.

## Bank arenas and ownership

Boot calls `__bank_detect`, installs the selected mapper as a three-byte
`JP nn` trampoline in fixed RAM, and then calls `__bank_init`.
Initialization maps every detected bank and creates a normal YOS heap at
`0xC000` with size `0x4000`. `__bank_allocate` scans the arenas in
logical-bank order and uses their existing first-fit allocator, so several
images can share one physical bank. Split, coalesce, shrink, owner
transfer, rollback, and final owner release all use the same seven-byte
block format as the fixed OS heap.

The shared image loader reads and relocates the XL payload in the selected
bank. A process or library object records its selected bank at byte 15.
Process reaping scans every bank and releases all blocks owned by that
object. An image must fit within one free extent — it can never cross the
16 KiB window boundary.

Library export tables do not live beside the banked code they describe.
The loader instead creates a stable fixed-memory table of packed
three-byte entries:

```text
bank, address-low, address-high
```

Each entry names the final relocated address in `0xC000`–`0xFFFF`. The
library's existing reference count pins both this table and its banked
image together.

## Far calls

RST 20h is the assembly immediate gate:

```asm
        rst     0x20
        .db     bank                    ; bit 7 clear: CALL
        .dw     relocated_address
```

Setting bit 7 requests a non-returning far jump; the remaining seven bits
give the bank. A same-bank call simply pushes the descriptor continuation
and jumps directly to the target. A cross-bank call records the caller
bank and continuation in fixed memory, maps the target bank in, and hands
the target an ordinary return address immediately followed by its normal
stack arguments. The common return gate then preserves the result
registers, restores the caller's bank, and jumps to the saved
continuation.

Every thread carries four checked bank-call frames, each three bytes
(`caller bank, continuation-low, continuation-high`). A fifth nested
cross-bank call is refused rather than entered — the gate resumes its
continuation with carry set instead. Kernel context has an equivalent
four-frame fallback. Same-bank calls consume no frame at all.

XCC's `--platform=yos` backend uses RST 28h for calls through
`[[xcc::far]]` function pointers. It saves the argument registers, passes
the runtime `{bank,address}` pair, and enters the same call core described
above — preserving `sdcccall(1)` semantics, including callee-cleaned stack
arguments. No other XCC target emits this gate.

## Scheduling

A thread records the exact logical bank that was mapped when it was
preempted, which can be a library bank rather than its process's home
bank. The scheduler remaps that saved bank before restoring the register
context. Because far-call frames are already complete in fixed RAM before
a target is ever entered, an interrupt and context switch at any
instruction in banked code is safe. Kernel threads use bank `0xFF`,
meaning no bank mapping is required for them at all.

## Pointer rules

Near pointers below `0xC000` stay stable across bank switches. A near
pointer inside the upper window, by contrast, is only meaningful while its
owning bank is mapped. Cross-bank interfaces must therefore rely on
scalars, fixed-memory pointers, handles, bounded copies, or packed far
pointers — never a bare near pointer into the banked window.

XCC far pointers are three bytes, in the same
`bank, address-low, address-high` order. Arithmetic adjusts only the
16-bit address and preserves the bank; crossing the pointed-to object or
the end of the window is undefined behavior. Functions return a far
pointer in HL (address) and E (bank).

The public `allocate_memory` call returns this far-pointer type and
searches every configured bank. Standard C `malloc`, however, must return
an ordinary 16-bit near pointer, so the YOS libc backend deliberately
allocates only from the bank currently executing the process or library —
it never silently spills a near pointer into another bank. Use
`yos_user_ptr_t` and the raw table calls instead whenever an allocation
needs to span the full bank set.

RST 30h supplies byte loads and stores for XCC far-data dereferences,
temporarily mapping the requested bank and restoring the execution bank
afterward. The public timer and print-hook callback APIs still carry near
pointers, so they must only ever be given fixed-memory callbacks.
