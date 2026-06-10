        ;; fputwc.s
        ;;
        ;; The target only supports a single-byte execution charset, so wide
        ;; output is accepted when wctob can narrow the code unit losslessly.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module fputwc
        .optsdcc -mz80 sdcccall(1)

        .globl  _fputwc
        .globl  _wctob
        .globl  _fputc

        .area   _CODE

_fputwc::
        push    hl                      ; Save the original wide result value.
        push    de                      ; Save the stream pointer.
        call    _wctob
        ld      a,d
        cp      #0xff
        jr      nz,__fputwc_have_byte
        ld      a,e
        cp      #0xff
        jr      z,__fputwc_fail_narrow
__fputwc_have_byte:
        pop     bc
        ex      de,hl                   ; HL = narrowed byte as promoted int.
        ld      e,c
        ld      d,b
        call    _fputc
        pop     bc
        ld      a,d
        cp      #0xff
        jr      nz,__fputwc_ok
        ld      a,e
        cp      #0xff
        ret     z
__fputwc_ok:
        ld      e,c
        ld      d,b
        ret
__fputwc_fail_narrow:
        pop     bc
        pop     bc
        ret
