        ; isblank.s
        ;
        ; libc isblank implementation for the xcc Z80 libc.
        ; Matches horizontal tab and space in the ASCII range.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module isblank
        .optsdcc -mz80 sdcccall(1)


        .globl  _isblank
        .globl  __ctype_return_false
        .globl  __ctype_return_flag

        .area   _CODE

        ;; _isblank
        ;; isblank only accepts horizontal tab and space, so direct compares are
        ;; smaller than routing through the interval helper twice.
_isblank::
        ld      a,h
        or      a
        jp      nz,__ctype_return_false
        ld      a,l
        cp      #0x09
        jr      z,isblank_done
        cp      #0x20
isblank_done:
        jp      __ctype_return_flag
