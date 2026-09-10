# Alto heap comparison and XCC calloc improvements

The supplied Alto routines are smaller, and they are faster in the measured
allocation trace. The calloc technique is directly useful to XCC. The complete
heap is a different allocation contract, so its size advantage should not be
presented as a feature-equivalent replacement.

XCC retains its existing allocator and its multiple-heap, ownership and aligned
allocation features. Only the calloc technique is adopted from Alto. The other
XCC heap edits are boundary and overflow corrections within the existing
implementation; the whole-allocator figures below are a comparison only.

## Changes applied to XCC

`x/libc/src/stdlib/calloc.s` now checks the product with shift/add arithmetic,
then clears the allocation with one seed store and LDIR. It no longer imports
multiply or divide helpers. The requested byte count survives malloc's allowed
register clobbers, IX/IY are preserved, allocation failure clears no memory,
and zero-size requests retain XCC's malloc(0) policy.

The supplied minimal arithmetic/clear combination is 57 bytes when inlined.
XCC uses a 67-byte version with early zero-argument handling: this is still
53 bytes smaller than the previous 120-byte routine, and the zero paths are
also faster. The shared libc implementation serves both optimization profiles
and every platform that uses this allocator.

Measured T-states below replace malloc with the same 10-T-state return stub in
both versions, isolating product validation and clearing. They do not include
the actual allocation search or heap initialization.

| Call | Previous | Revised |
|---|---:|---:|
| calloc(0, 65535) | 50 | 28 |
| calloc(65535, 0) | 65 | 63 |
| calloc(1, 1) | 1,653 | 249 |
| calloc(1, 32) | 2,967 | 904 |
| calloc(16, 16) | 12,874 | 5,892 |
| calloc(100, 7) | 31,852 | 15,384 |
| calloc(1, 1024) | 45,623 | 21,736 |

The comparison also exposed two existing XCC defects. Rounding a 65,535-byte
request to even size could wrap to zero, allowing an undersized malloc result
or a destructive realloc shrink. Allocation now rejects this overflow, and a
failed realloc preserves the original block. Default-heap setup now rounds an
odd platform base up to two-byte alignment. Explicit custom arenas retain their
original exact-span contract; their caller supplies the required alignment.
Arena initialization creates an empty heap for invalid, wrapping or too-small
intervals before writing a block header.

These correctness guards add 29 bytes across the allocator modules. Together
with the calloc reduction, the five changed modules shrink by 24 bytes. When
the former arithmetic helpers were otherwise unused, another 92 linked bytes
disappear, giving a combined 116-byte saving. Actual application savings depend
on which archive members were already needed.

## Whole allocator comparison

| Property | Supplied Alto | Revised XCC |
|---|---|---|
| Allocated-block header | 2 bytes | 8 bytes |
| Free-list organization | Only free blocks, address ordered | All blocks, with free flag and owner |
| Heap ownership | One arena | Explicit multiple heaps |
| Odd-size request alignment | No rounding | Rounds size to even; default heap has an even base |
| Zero-size allocation | Minimum two-byte payload | Null |
| malloc/free/init code and dependencies | 264 bytes | 567 bytes |

The code measurement excludes Alto's separate 24-byte zero-allocation helper
and XCC's nine-byte test/platform stub. XCC's linked dependencies include its
owning-heap and aligned-pointer support. This is a comparison of the actual
implementations, not equal feature sets. Alto's free payload holds its next
pointer, and address ordering limits coalescing to the neighbouring free
blocks after insertion. XCC's larger representation and its coalescing walk
remain opportunities for a future allocator redesign.

In a fresh arena [0x4000, 0xc000), the trace allocates two three-byte objects,
frees them in allocation order, then allocates 256 bytes:

| Operation | Alto T-states | Revised XCC T-states |
|---|---:|---:|
| Initialization plus first allocation | 714 | 1,919 |
| Second allocation | 589 | 1,320 |
| Free first object | 458 | 994 |
| Free second object | 981 | 1,773 |
| Allocate 256 bytes after coalescing | 571 | 1,175 |

Alto initializes explicitly in 125 T-states; XCC initializes lazily inside the
first malloc. Alto returns 0x4002 and then 0x4007, demonstrating the odd-address
alignment difference. XCC returns 0x4008 and 0x4014. This small trace establishes
a real advantage for Alto on these operations; it does not establish a universal
speed advantage for arbitrary fragmentation patterns.

## Evidence

The focused validation passed 1,870,971 product, overflow and exact-clearing
checks, plus 12,584 checks against the actual allocator for each of even and odd
default platform bases, covering reuse, ownership, alignment, invalid arenas and
realloc preservation. The compiled allocation matrix passed 306/306 lanes across
S/M/L models, both calling conventions and the configured optimization profiles.
Existing custom-heap regression fixtures remain unchanged.
Durable regressions are `xcc_core_t302_heap_allocation_boundaries` and
`xcc_exec_int_t281_heap_allocation_boundaries`; the independent emulator probe
lives in `x/tests/tests/libc/heap_calloc_probe.cpp`.

Exact before/after source hashes, routine sizes and cycle samples are retained
in `build/sccz80-campaign/framework/heap-calloc/final-results.json`, with the final
custom/default-heap contract checks in
`build/sccz80-campaign/final-validation-v4/diagnose-heap/final-results.json`. The supplied
Alto image changes only the platform heap-limit include to the test arena limit;
its image and map remain in `build/sccz80-campaign/framework/heap-calloc/alto.*`.
The final-source XCC image, map and cycle trace are in
`build/sccz80-campaign/final-validation-v4/diagnose-heap/allocator-comparison/`.
Full combined compiler validation is reported separately by the optimization
campaign.
