        ; ctype_common.s
        ;
        ; Shared helper routines for the libc ctype implementation.
        ; The public ctype entry points all funnel their common return-value
        ; handling and ASCII interval checks through these helpers.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module ctype_common
        .optsdcc -mz80 sdcccall(1)


        .globl  __ctype_return_false
        .globl  __ctype_return_true
        .globl  __ctype_return_flag
        .globl  __ctype_return_hl
        .globl  __ctype_test_interval

        .area   _CODE

        ; __ctype_return_false
        ; outputs: DE = 0
__ctype_return_false::
        ld      de,#0x0000
        ret

        ; __ctype_return_true
        ; outputs: DE = 1
__ctype_return_true::
        ld      de,#0x0001
        ret

        ; __ctype_return_flag
        ; inputs:  flags from the prior classification test
        ; outputs: DE = 1 when Z is set, else 0
__ctype_return_flag::
        ld      de,#0x0000
        ret     nz
        inc     e
        ret

        ; __ctype_return_hl
        ; inputs:  HL = promoted int result
        ; outputs: DE = HL
__ctype_return_hl::
        ex      de,hl
        ret

        ; __ctype_test_interval
        ; inputs:
        ;   A = value to test
        ;   D = inclusive upper bound
        ;   E = inclusive lower bound
        ; outputs:
        ;   Z = 1 when E <= A <= D
        ;   Z = 0 otherwise
        ; clobbers: BC, flags
        ; notes:
        ;   The original input byte is parked in C so A can be reused for the
        ;   compare sequence without losing the caller-visible value.
__ctype_test_interval::
        push    bc
        ld      c,a                      ; preserve the tested byte
        cp      e
        jr      c,__ctype_test_interval_outside
        cp      d
        jr      z,__ctype_test_interval_inside
        jr      c,__ctype_test_interval_inside
__ctype_test_interval_outside:
        xor     a                        ; force Z = 0 on exit
        inc     a
        ld      a,c
        pop     bc
        ret
__ctype_test_interval_inside:
        xor     a                        ; force Z = 1 on exit
        ld      a,c
        pop     bc
        ret
