        ;; Allocate an independent, process-owned ZX Spectrum GPX context.
        ;; GPL2 License (see: LICENSE)
        ;; Copyright (C) 2026 Tomaz Stih

        .module gpx_create
        .optsdcc -mz80 sdcccall(1)
        .globl  _gpx_create
        .globl  __near_malloc
        .area   _CODE

        ;; A = mode (ignored); DE = fresh context or NULL on exhaustion.
        ;; Clobbers AF, BC, DE, HL; preserves IX/IY. Does not clear the screen.
        ;; Each context belongs to the caller's process/library for cleanup.
_gpx_create::
        ld      hl,#6
        call    __near_malloc
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
        .dw     320,256
        .db     5,0                     ; five 16K framebuffer banks, opaque text background
