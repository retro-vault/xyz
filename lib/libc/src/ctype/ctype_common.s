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

        ;; __ctype_return_false
        ;; Return the canonical false value used by the narrow ctype entry points.
__ctype_return_false::
        ld      de,#0x0000
        ret

        ;; __ctype_return_true
        ;; Return the canonical true value used by the narrow ctype entry points.
__ctype_return_true::
        ld      de,#0x0001
        ret

        ;; __ctype_return_flag
        ;; Convert a prior equality or range test into libc's 0/1 integer result.
        ;; Callers arrange for Z to mean "classification matched".
__ctype_return_flag::
        ld      de,#0x0000
        ret     nz
        inc     e
        ret

        ;; __ctype_return_hl
        ;; Return an unchanged promoted int through DE without rebuilding it bytewise.
__ctype_return_hl::
        ex      de,hl
        ret

        ;; __ctype_test_interval
        ;; Test the ASCII byte in A against the closed interval [E, D].
        ;; The helper preserves the original byte in A so callers can chain
        ;; several range checks without reloading L each time.
__ctype_test_interval::
        push    bc
        ld      c,a                      ; Keep the candidate byte available to callers.
        cp      e
        jr      c,__ctype_test_interval_outside
        cp      d
        jr      z,__ctype_test_interval_inside
        jr      c,__ctype_test_interval_inside
__ctype_test_interval_outside:
        xor     a                        ; Return NZ for "outside the interval".
        inc     a
        ld      a,c
        pop     bc
        ret
__ctype_test_interval_inside:
        xor     a                        ; Return Z for "inside the interval".
        ld      a,c
        pop     bc
        ret
