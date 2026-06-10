        ;; ungetwc.s
        ;;
        ;; The stdio pushback slot is one byte wide, so only code units that
        ;; round-trip through wctob can be pushed back.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module ungetwc
        .optsdcc -mz80 sdcccall(1)

        .globl  _ungetwc
        .globl  _wctob
        .globl  _ungetc

        .area   _CODE

_ungetwc::
        push    de
        call    _wctob
        ld      a,d
        cp      #0xff
        jr      nz,__ungetwc_have_byte
        ld      a,e
        cp      #0xff
        jr      z,__ungetwc_fail
__ungetwc_have_byte:
        pop     bc
        ex      de,hl
        ld      e,c
        ld      d,b
        jp      _ungetc
__ungetwc_fail:
        pop     bc
        ret
