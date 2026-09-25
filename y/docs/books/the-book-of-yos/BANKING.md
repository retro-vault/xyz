# Banked Processes and Libraries

YOS keeps the complete kernel and all shared runtime state below `0xC000`.
The upper 16 KiB, `0xC000-0xFFFF`, is a logical executable-bank window.
Process and library XPRG images and public user allocations are packed into
independent user-heap arenas behind that window. Every OS object—including
thread stacks, service records and bound call tables—remains in the single
fixed OS heap below `0xC000`.

## Detection and configured capacity

Every ROM contains all three mappers. At boot YOS reads NextReg `0x00` first
and recognizes the exact machine IDs `0x08`, `0x0A`, and `0xFA`. If that is
not a Next, it performs a reversible page-0/page-1 test through port `0x7FFD`;
failure to observe distinct pages selects 48K. The detected model byte is
kept in fixed RAM and exposed as `yos->rom_model()`.

`YOS_BANK_COUNT` sets the compiled capacity and defaults to 126, the maximum
safe Next count. `YOS_BANK_BACKEND` selects the hardware model expected by the
emulator test fixture; it does not remove the other mappers from the ROM:

```sh
make -C y/src/z80 YOS_BANK_BACKEND=48 YOS_BANK_COUNT=1
make -C y/src/z80 YOS_BANK_BACKEND=128 YOS_BANK_COUNT=6
make -C y/src/z80 YOS_BANK_BACKEND=next YOS_BANK_COUNT=126
```

The detected 48K model always uses one logical bank. It runs the complete
bank-aware allocator and call ABI but its mapper only records bank zero. The
128K model uses at most six configured banks and maps logical banks to
physical pages `0,1,3,4,6,7` through `0x7FFD`; pages 5 and 2 remain fixed
below the window. ESXIDE boots YOS from the 48 BASIC ROM slot, so YOS keeps
`0x7FFD` ROM-select bit 4 set while leaving the screen and paging-disable bits
clear. The Next maps
each logical bank to an adjacent MMU-page pair in slots 6 and 7 using NextRegs
`0x56` and `0x57`. It skips physical 16 KiB pages 2 and 5 because those back
the fixed RAM below `0xC000`. The paired selector/data writes are protected
by a nestable critical section.

Bank identifiers are seven bits because bit 7 is the inline far-jump flag.
That gives 1 bank on 48K, up to 6 usable banks on 128K, and up to 126 safe
logical banks on a 2 MiB Next.

## Bank arenas and ownership

Boot calls `__bank_detect`, installs the selected mapper as a three-byte
`JP nn` trampoline in fixed RAM, then calls `__bank_init`. Initialization
maps every detected bank and creates a normal YOS heap at `0xC000` with size
`0x4000`. `__bank_allocate` scans
the arenas in logical-bank order and uses their existing first-fit allocator.
Several images can therefore share one physical bank. Split, coalescing,
shrinking, owner transfer, rollback, and final owner release use the same
seven-byte block format as the fixed OS heap.

The shared image loader reads and relocates the XL payload in the selected
bank. A process or library object records the selected bank at byte 15.
Process reaping scans every bank and releases all blocks owned by that object.
An image must fit one free extent and cannot cross the 16 KiB window.

Library export tables do not live beside banked code. The loader creates a
stable fixed-memory table of packed three-byte entries:

```text
bank, address-low, address-high
```

Each entry names the final relocated address in `0xC000-0xFFFF`. The
library's existing reference count pins both this table and its banked image.

## Far calls

RST 20h is the assembly immediate gate:

```asm
        rst     0x20
        .db     bank                    ; bit 7 clear: CALL
        .dw     relocated_address
```

Setting bit 7 requests a non-returning far jump; the remaining seven bits are
the bank. A same-bank call pushes the descriptor continuation and jumps
directly to the target. A cross-bank call records the caller bank and
continuation in fixed memory, maps the target, and gives the target an
ordinary return address immediately followed by its normal stack arguments.
The common return gate preserves result registers, restores the caller bank,
and jumps to the saved continuation.

Every thread has four checked bank-call frames. Each frame is three bytes
(`caller bank, continuation-low, continuation-high`). A fifth nested
cross-bank call is not entered; the gate resumes its continuation with carry
set. Kernel context has an equivalent four-frame fallback. Same-bank calls
consume no frame.

XCC's `--platform=yos` backend uses RST 28h for calls through
`[[xcc::far]]` function pointers. It saves the argument registers, passes
the runtime `{bank,address}`, and enters the same call core. This preserves
sdcccall(1), including callee-cleaned stack arguments. Other XCC targets do
not emit the YOS gate.

## Scheduling

A thread records the exact logical bank mapped when it is preempted, which can
be a library bank rather than its process's home bank. The scheduler maps that
saved bank before restoring the register context. Far-call frames are already
complete in fixed RAM before a target is entered, so an interrupt and context
switch at any instruction in banked code is safe. Kernel threads use bank
`0xFF`, meaning no bank mapping is required.

## Pointer rules

Near pointers below `0xC000` are stable across bank switches. A near pointer
in the upper window is meaningful only while its owning bank is mapped.
Cross-bank interfaces must therefore use scalars, fixed-memory pointers,
handles, bounded copies, or packed far pointers.

XCC far pointers are three bytes in the same `bank,address-low,address-high`
order. Arithmetic adjusts only the 16-bit address and preserves the bank;
crossing the pointed-to object or the end of the window is undefined.
Functions return a far pointer in HL (address) and E (bank).

The public `allocate_memory` call returns this far-pointer type and searches
all configured banks. Standard C `malloc` must return a 16-bit near pointer,
so the YOS libc backend deliberately allocates it only from the bank currently
executing the process or library; it never silently spills a near pointer into
another bank. Use `yos_user_ptr_t` and the raw table calls when allocation must
span the full bank set.

RST 30h supplies byte loads/stores for XCC far-data dereferences while
temporarily mapping the requested bank and restoring the execution bank. The
public timer and print-hook callback APIs still carry near pointers and must
only be given fixed-memory callbacks.
