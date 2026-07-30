        ; assert_fail.s
        ;
        ; libc assert failure sink for the xcc Z80 libc.
        ; Saves the failing expression text and source location into globals
        ; that a debugger can inspect, then halts forever.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih



        .module assert_fail
        .optsdcc -mz80 sdcccall(1)

        .globl  ___assert_fail
        .globl  __assert_fail
        .globl  __assert_expr
        .globl  __assert_file
        .globl  __assert_func
        .globl  __assert_line

        .area   _CODE
        ; C external names receive the normal leading underscore.  Keep the
        ; older assembler-facing spelling as an alias for debugger scripts.
___assert_fail::
__assert_fail::
        push    ix
        ld      ix, #0
        add     ix, sp

        ld      (__assert_expr), hl
        ld      (__assert_file), de
        ld      l, 4(ix)
        ld      h, 5(ix)
        ld      (__assert_line), hl
        ld      l, 6(ix)
        ld      h, 7(ix)
        ld      (__assert_func), hl

        pop     ix

__assert_fail_halt:
        halt
        jp      __assert_fail_halt
