        ; isalnum.s
        ;
        ; libc isalnum implementation for the xcc Z80 libc.
        ; Accepts ASCII decimal digits and ASCII alphabetic characters.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module isalnum
        .optsdcc -mz80 sdcccall(1)


        .globl  _isalnum
        .globl  __ctype_return_false
        .globl  __ctype_return_true
        .globl  __ctype_test_interval

        .area   _CODE

        ;; _isalnum
        ;; Reject values outside the single-byte execution charset, then test the
        ;; three ASCII bands that make up the alphanumeric class.
_isalnum::
        ld      a,h
        or      a
        jp      nz,__ctype_return_false
        ld      a,l
        ld      de,#0x3930
        call    __ctype_test_interval
        jp      z,__ctype_return_true
        ld      de,#0x5a41
        call    __ctype_test_interval
        jp      z,__ctype_return_true
        ld      de,#0x7a61
        call    __ctype_test_interval
        jp      z,__ctype_return_true
        jp      __ctype_return_false
