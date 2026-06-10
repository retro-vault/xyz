        ;; fsetpos.s
        ;;
        ;; Public fsetpos() entry point. The current FILE model is byte-
        ;; addressed and unbuffered, so an fpos_t is simply the underlying long
        ;; file offset used by fseek().
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module fsetpos
        .optsdcc -mz80 sdcccall(1)

        .globl  _fsetpos
        .globl  _fseek

        SEEK_SET_V      .equ 0x0000

        .area   _DATA
__stdio_fsetpos_lo:
        .dw     0
__stdio_fsetpos_hi:
        .dw     0
__stdio_fsetpos_stream:
        .dw     0

        .area   _CODE

        ;; _fsetpos
        ;; sdcccall(1) ABI:
        ;;   HL = FILE *stream
        ;;   DE = const fpos_t *pos
        ;;
        ;; Return 0 on success, -1 on error.
_fsetpos::
        ld      (__stdio_fsetpos_stream),hl
        ld      a,d
        or      e
        jr      z,__stdio_fsetpos_fail ; Null source position is treated as failure.
        ex      de,hl
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        inc     hl
        ld      (__stdio_fsetpos_lo),de
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        ld      (__stdio_fsetpos_hi),de

        ld      bc,#SEEK_SET_V
        push    bc
        ld      de,(__stdio_fsetpos_hi)
        push    de
        ld      de,(__stdio_fsetpos_lo)
        push    de
        ld      hl,(__stdio_fsetpos_stream)
        call    _fseek
        pop     bc
        pop     bc
        pop     bc
        ret

__stdio_fsetpos_fail:
        ld      hl,#0xffff
        push    hl
        pop     de
        ret
