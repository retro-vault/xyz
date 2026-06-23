        ;; putwchar.s
        ;;
        ;; putwchar narrows one wide code unit through wctob and then reuses
        ;; the existing stdout byte path. Unrepresentable values fail with WEOF.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module putwchar
        .optsdcc -mz80 sdcccall(1)

        .globl  _putwchar
        .globl  _wctob
        .globl  _putchar

        .area   _CODE

_putwchar::
        ld      b,h
        ld      c,l
        call    _wctob
        ld      a,d
        cp      #0xff
        jr      nz,__putwchar_emit
        ld      a,e
        cp      #0xff
        ret     z
__putwchar_emit:
        ex      de,hl
        call    _putchar
        ld      a,d
        cp      #0xff
        jr      nz,__putwchar_ok
        ld      a,e
        cp      #0xff
        ret     z
__putwchar_ok:
        ld      e,c
        ld      d,b
        ret
