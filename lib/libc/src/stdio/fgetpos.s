        ;; fgetpos.s
        ;;
        ;; Public fgetpos() entry point. The fd-backed FILE layer already knows
        ;; how to report the current byte offset through ftell(), so this wrapper
        ;; simply forwards to ftell() and stores the resulting long into *pos
        ;; using the target's little-endian 32-bit layout (low word first).
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module fgetpos
        .optsdcc -mz80 sdcccall(1)

        .globl  _fgetpos
        .globl  _ftell

        .area   _CODE

        ;; _fgetpos
        ;; sdcccall(1) ABI:
        ;;   HL = FILE *stream
        ;;   DE = fpos_t *pos
        ;;
        ;; Return 0 on success, -1 on error.
_fgetpos::
        push    de
        ld      a,d
        or      e
        jr      z,__stdio_fgetpos_fail ; Null destination is treated as failure.

        call    _ftell
        ld      a,h
        cp      #0xff
        jr      nz,__stdio_fgetpos_store
        ld      a,l
        cp      #0xff
        jr      nz,__stdio_fgetpos_store
        ld      a,d
        cp      #0xff
        jr      nz,__stdio_fgetpos_store
        ld      a,e
        cp      #0xff
        jr      z,__stdio_fgetpos_fail ; Propagate ftell() error.

__stdio_fgetpos_store:
        push    hl
        push    de
        pop     bc
        pop     de
        pop     hl
        ld      (hl),c
        inc     hl
        ld      (hl),b
        inc     hl
        ld      (hl),e
        inc     hl
        ld      (hl),d
        ld      hl,#0x0000
        push    hl
        pop     de
        ret

__stdio_fgetpos_fail:
        pop     de
        ld      hl,#0xffff
        push    hl
        pop     de
        ret
