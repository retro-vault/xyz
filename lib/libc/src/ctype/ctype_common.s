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

        .area   _CODE
__ctype_return_false::
        ld      de,#0x0000
        ret

        ;; __ctype_return_true
        ;; Return the canonical true value used by the narrow ctype entry points.
