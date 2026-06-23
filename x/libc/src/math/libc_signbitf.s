        ;; libc_signbitf.s
        ;;
        ;; libc signbit helper for the xcc Z80 libc.
        ;; Returns DE = 1 when the float sign bit is set, else DE = 0.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih
        .module libc_signbitf
        .optsdcc -mz80 sdcccall(1)
        .globl  __libc_signbitf
        .globl  ___libc_signbitf
        .area   _CODE
        ;; The sign bit lives in bit 7 of H, the top byte of HL:DE.
__libc_signbitf:
___libc_signbitf::
        ld      de,#0                   ; default false
        bit     7,h                     ; inspect the sign bit directly
        ret     z                       ; positive numbers and +0 return 0
        inc     de                      ; negative numbers return 1
        ret
