        ;; string_return_hl.s
        ;; Split from string_common.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module string_return_hl
        .optsdcc -mz80 sdcccall(1)

        .globl  __string_return_hl

        .area   _CODE
__string_return_hl::
        ex      de,hl
        ret

        ; __string_return_bc
        ; inputs: BC = pointer / size result
        ; outputs: DE = BC
