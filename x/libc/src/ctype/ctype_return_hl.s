        ;; ctype_return_hl.s
        ;; Split from ctype_common.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module ctype_return_hl
        .optsdcc -mz80 sdcccall(1)

        .globl  __ctype_return_hl

        .area   _CODE
__ctype_return_hl::
        ex      de,hl
        ret

        ;; __ctype_test_interval
        ;; Test the ASCII byte in A against the closed interval [E, D].
        ;; The helper preserves the original byte in A so callers can chain
        ;; several range checks without reloading L each time.
