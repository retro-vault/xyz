        ; tolower.s
        ;
        ; libc tolower implementation for the xcc Z80 libc.
        ; Converts ASCII uppercase letters to lowercase and leaves everything
        ; else unchanged.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module tolower
        .optsdcc -mz80 sdcccall(1)


        .globl  _tolower
        .globl  __ctype_return_hl
        .globl  __ctype_test_interval

        .area   _CODE

        ;; _tolower
        ;; Only 'A'..'Z' are rewritten. Every other promoted int, including EOF,
        ;; must pass through unchanged.
_tolower::
        ld      a,h
        or      a
        jp      nz,__ctype_return_hl
        ld      a,l
        ld      de,#0x5a41
        call    __ctype_test_interval
        jr      nz,tolower_return_a
        add     a,#0x20
tolower_return_a:
        ld      e,a
        ld      d,#0x00
        ret
