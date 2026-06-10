        ; isxdigit.s
        ;
        ; libc isxdigit implementation for the xcc Z80 libc.
        ; Accepts ASCII decimal digits plus hexadecimal A-F / a-f.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module isxdigit
        .optsdcc -mz80 sdcccall(1)


        .globl  _isxdigit
        .globl  __ctype_return_false
        .globl  __ctype_return_true
        .globl  __ctype_test_interval

        .area   _CODE

        ;; _isxdigit
        ;; Hex digits are the decimal band plus the upper- and lowercase A-F
        ;; islands.
_isxdigit::
        ld      a,h
        or      a
        jp      nz,__ctype_return_false
        ld      a,l
        ld      de,#0x3930
        call    __ctype_test_interval
        jp      z,__ctype_return_true
        ld      de,#0x4641
        call    __ctype_test_interval
        jp      z,__ctype_return_true
        ld      de,#0x6661
        call    __ctype_test_interval
        jp      z,__ctype_return_true
        jp      __ctype_return_false
