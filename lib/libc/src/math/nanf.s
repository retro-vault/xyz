        ; nanf.s
        ;
        ; libc nanf implementation for the xcc Z80 libc.
        ; Returns a quiet NaN.  The tag-string argument is ignored.
        ; double / long double are 32-bit on this target, so nan / nanl
        ; share the implementation.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module nanf
        .optsdcc -mz80 sdcccall(1)


        .globl  _nanf
        .globl  _nan
        .globl  _nanl

        .area   _CODE

        ; _nanf / _nan / _nanl
        ; inputs:  HL = const char *tag (ignored)
        ; outputs: HL:DE = quiet NaN (0x7FC00000)
        ; clobbers: AF
_nan::
_nanl::
_nanf::
        ld      h,#0x7f
        ld      l,#0xc0
        ld      d,#0x00
        ld      e,#0x00
        ret
