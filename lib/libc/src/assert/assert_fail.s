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

        .globl  __assert_fail
        .globl  __assert_expr
        .globl  __assert_file
        .globl  __assert_line
        .globl  __assert_func

        .area   _CODE

        ; __assert_fail
        ; inputs:
        ;   HL           = pointer to failing expression text
        ;   DE           = pointer to source filename
        ;   4(ix)..5(ix) = source line number
        ;   6(ix)..7(ix) = pointer to function name string
        ; outputs:
        ;   does not return
        ; clobbers: AF, HL, DE, IX
        ; notes:
        ;   The standard wants a diagnostic on stderr followed by abort().
        ;   Until stdio and abort() exist in this libc, we preserve the failure
        ;   context in globals and stop the CPU in place.
__assert_fail:
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

        .area   _DATA

__assert_expr:
        .dw     0

__assert_file:
        .dw     0

__assert_line:
        .dw     0

__assert_func:
        .dw     0
