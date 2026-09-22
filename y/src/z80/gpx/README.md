# YOS ZX Spectrum libgpx

This directory vendors the ZX Spectrum and portable assembly modules from
upstream `main` snapshot, commit
`0ef6f070be0aa35054e6799cae56157b9f7890c5` (`v1.1.0-1-g0ef6f07`):

https://github.com/retro-vault/libgpx

The sources are included in the YOS ROM archive so the ROM is self-contained
and reproducible. `_gpx_name.s` and `_gpx_service.s` are YOS integration
modules. YOS adaptations allocate/free independent, process-owned six-byte
contexts, report the constant Spectrum dimensions without a global context,
and protect framebuffer read/modify/write spans, rows and sprite operations
with the kernel's IFF-preserving critical sections. Creation does not clear
the shared display. Compound drawings can interleave between primitives;
callers coordinate overlapping regions and sprite lifetimes. The vendored
library remains GPL-2.0 licensed; see `LICENSE.libgpx`.

The YOS copy also shares the kernel's IX-frame entry routine to reduce ROM
size. Frame offsets and argument cleanup are unchanged; entry trades extra
cycles and two transient stack bytes for smaller call sites.
Outline and filled circles additionally share initialization and midpoint
X/Y updates, retaining their separate pixel/span emission and frame layouts.
The additional helper calls use two transient stack bytes. Font and cursor
payloads are unchanged.
