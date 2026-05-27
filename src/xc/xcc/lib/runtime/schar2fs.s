        ; signed char to float (ieee-754 single) for the runtime
        ; converts an 8-bit signed value (-128..127) to 32-bit single-
        ; precision.
        ; implemented by sign-extending to 16-bit and tail-calling
        ; ___sint2fs.
        ;
        ; gpl-2.0-or-later (see: LICENSE)
        ; copyright (c) 2025 tomaz stih

        .module schar2fs
        .optsdcc -mz80 sdcccall(1)

        .area   _CODE

        .globl  ___schar2fs
        .globl  ___sint2fs

        ; ___schar2fs
        ; inputs:  a = value (signed char)
        ; outputs: hl:de = (float)a
        ; clobbers: af, bc, de, hl
___schar2fs:
        ; sign-extend A into HL
        ld      l,a
        add     a,a                     ; sign bit -> carry
        sbc     a,a                     ; a = 0x00 or 0xFF
        ld      h,a
        jp      ___sint2fs              ; tail-call
