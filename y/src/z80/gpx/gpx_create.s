        ;; Allocate an independent, process-owned ZX Spectrum GPX context.
        ;; GPL2 License (see: LICENSE)
        ;; Copyright (C) 2026 Tomaz Stih

        .module gpx_create
        .optsdcc -mz80 sdcccall(1)
        .globl  _gpx_create
        .globl  __os_malloc
        .area   _CODE

        ;; A = mode (ignored); DE = fresh context or NULL on exhaustion.
        ;; Clobbers AF, BC, DE, HL; preserves IX/IY. Does not clear the screen.
        ;; Each context belongs to the caller's process/library for cleanup.
_gpx_create::
        ld      hl,#6
        call    __os_malloc
        ld      a,d
        or      e
        ret     z
        push    de
        ld      hl,#.defaults
        ld      bc,#6
        ldir
        pop     de
        ret
.defaults:
        .dw     256,192
        .db     1,0                     ; one page, opaque text background
