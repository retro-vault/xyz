        ;; string_return_bc.s
        ;; Split from string_common.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module string_return_bc
        .optsdcc -mz80 sdcccall(1)

        .globl  __string_return_bc

        .area   _CODE
__string_return_bc::
        ld      d,b
        ld      e,c
        ret

        ; __string_compare_result
        ; inputs: flags from a prior CP/compare
        ; outputs: DE = -1, 0, or 1
        ; notes:
        ;   The helper assumes the caller preserved the compare flags and wants
        ;   a normal C strcmp-style tri-state result.
