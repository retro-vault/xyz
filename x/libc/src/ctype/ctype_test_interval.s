        ;; ctype_test_interval.s
        ;; Split from ctype_common.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module ctype_test_interval
        .optsdcc -mz80 sdcccall(1)

        .globl  __ctype_test_interval

        .area   _CODE
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
