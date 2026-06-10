        ; isgraph.s
        ;
        ; libc isgraph implementation for the xcc Z80 libc.
        ; Accepts printable ASCII characters except space.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module isgraph
        .optsdcc -mz80 sdcccall(1)


        .globl  _isgraph
        .globl  __ctype_return_false
        .globl  __ctype_return_flag
        .globl  __ctype_test_interval

        .area   _CODE

        ;; _isgraph
        ;; Graphical characters occupy the printable ASCII range without space.
_isgraph::
        ld      a,h
        or      a
        jp      nz,__ctype_return_false
        ld      a,l
        ld      de,#0x7e21
        call    __ctype_test_interval
        jp      __ctype_return_flag
