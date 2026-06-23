        ; isspace.s
        ;
        ; libc isspace implementation for the xcc Z80 libc.
        ; Accepts ASCII horizontal/vertical whitespace plus space.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module isspace
        .optsdcc -mz80 sdcccall(1)


        .globl  _isspace
        .globl  __ctype_return_false
        .globl  __ctype_return_flag
        .globl  __ctype_test_interval

        .area   _CODE

        ;; _isspace
        ;; C whitespace is the control run 0x09..0x0d plus the ordinary space.
_isspace::
        ld      a,h
        or      a
        jp      nz,__ctype_return_false
        ld      a,l
        ld      de,#0x0d09
        call    __ctype_test_interval
        jr      z,isspace_done
        cp      #0x20
isspace_done:
        jp      __ctype_return_flag
