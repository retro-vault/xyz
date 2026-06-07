        ; atan2f.s
        ;
        ; libc atan2f for the xcc Z80 libc.  Rational approximation built on the
        ; soft-float runtime (one body serves atan2f/atan2/atan2l).  Matches the
        ; previous C implementation (0.28 Pade-style fit), including the
        ; quadrant corrections.  Constants:
        ;   PI = 0x40490FDB  0.5*PI = 0x3FC90FDB  0.28 = 0x3E8F5C29
        ;   1.0 = 0x3F800000
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module atan2f
        .optsdcc -mz80 sdcccall(1)
        .globl  _atan2f
        .globl  _atan2
        .globl  _atan2l
        .globl  ___libc_fpclassifyf
        .globl  ___fsadd
        .globl  ___fssub
        .globl  ___fsmul
        .globl  ___fsdiv
        .area   _DATA
__at_y: .ds 4
__at_x: .ds 4
__at_z: .ds 4
__at_t: .ds 4
__at_a: .ds 4
        .area   _CODE
        ; HL:DE = y, x at 4(ix)..7(ix) -> HL:DE = atan2(y, x)
_atan2::
_atan2l::
_atan2f::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      (__at_y),de
        ld      (__at_y + 2),hl
        ld      a,4(ix)
        ld      (__at_x),a
        ld      a,5(ix)
        ld      (__at_x + 1),a
        ld      a,6(ix)
        ld      (__at_x + 2),a
        ld      a,7(ix)
        ld      (__at_x + 3),a
        ; NaN check
        ld      hl,(__at_x + 2)
        ld      de,(__at_x)
        call    ___libc_fpclassifyf
        ld      a,d
        or      e
        jp      z,at_nan
        ld      hl,(__at_y + 2)
        ld      de,(__at_y)
        call    ___libc_fpclassifyf
        ld      a,d
        or      e
        jp      z,at_nan
        ; x == 0 ?
        ld      a,(__at_x + 3)
        and     #0x7f
        ld      c,a
        ld      a,(__at_x + 2)
        or      c
        ld      c,a
        ld      a,(__at_x + 1)
        or      c
        ld      c,a
        ld      a,(__at_x)
        or      c
        jp      nz,at_xnonzero
        ; y == 0 ?
        ld      a,(__at_y + 3)
        and     #0x7f
        ld      c,a
        ld      a,(__at_y + 2)
        or      c
        ld      c,a
        ld      a,(__at_y + 1)
        or      c
        ld      c,a
        ld      a,(__at_y)
        or      c
        jr      z,at_zero
        ld      a,(__at_y + 3)
        bit     7,a
        jr      nz,at_neg_hpi
        ld      hl,#0x3fc9              ; +0.5*PI
        ld      de,#0x0fdb
        jp      at_ret
at_neg_hpi:
        ld      hl,#0xbfc9              ; -0.5*PI
        ld      de,#0x0fdb
        jp      at_ret
at_zero:
        ld      hl,#0
        ld      de,#0
        jp      at_ret
at_nan:
        ld      hl,#0x7fc0
        ld      de,#0
        jp      at_ret
at_xnonzero:
        ; z = y / x
        ld      hl,(__at_x + 2)
        ld      bc,(__at_x)
        push    hl
        push    bc
        ld      de,(__at_y)
        ld      hl,(__at_y + 2)
        call    ___fsdiv
        pop     bc
        pop     bc
        ld      (__at_z),de
        ld      (__at_z + 2),hl
        ; abs_z < 1.0 ? e8(z) < 127
        ld      a,(__at_z + 3)
        and     #0x7f
        add     a,a
        ld      c,a
        ld      a,(__at_z + 2)
        rlca
        and     #1
        or      c
        cp      #127
        jp      nc,at_large
        ; small: atan = z / (1.0 + 0.28*z*z)
        ld      hl,(__at_z + 2)
        ld      bc,(__at_z)
        push    hl
        push    bc
        ld      de,(__at_z)
        ld      hl,(__at_z + 2)
        call    ___fsmul                ; z*z
        pop     bc
        pop     bc
        ld      (__at_t),de
        ld      (__at_t + 2),hl
        ld      hl,(__at_t + 2)
        ld      bc,(__at_t)
        push    hl
        push    bc
        ld      de,#0x5c29              ; 0.28
        ld      hl,#0x3e8f
        call    ___fsmul                ; 0.28*z*z
        pop     bc
        pop     bc
        ld      (__at_t),de
        ld      (__at_t + 2),hl
        ld      hl,(__at_t + 2)
        ld      bc,(__at_t)
        push    hl
        push    bc
        ld      de,#0x0000              ; 1.0
        ld      hl,#0x3f80
        call    ___fsadd                ; 1.0 + 0.28*z*z
        pop     bc
        pop     bc
        ld      (__at_t),de
        ld      (__at_t + 2),hl
        ld      hl,(__at_t + 2)
        ld      bc,(__at_t)
        push    hl
        push    bc
        ld      de,(__at_z)
        ld      hl,(__at_z + 2)
        call    ___fsdiv                ; z / denom
        pop     bc
        pop     bc
        ld      (__at_a),de
        ld      (__at_a + 2),hl
        ; x<0 correction
        ld      a,(__at_x + 3)
        bit     7,a
        jr      z,at_a_plain
        ld      a,(__at_y + 3)
        bit     7,a
        jr      nz,at_a_subpi
        ld      hl,#0x4049              ; atan += PI
        ld      bc,#0x0fdb
        push    hl
        push    bc
        ld      de,(__at_a)
        ld      hl,(__at_a + 2)
        call    ___fsadd
        pop     bc
        pop     bc
        jp      at_ret
at_a_subpi:
        ld      hl,#0x4049              ; atan -= PI
        ld      bc,#0x0fdb
        push    hl
        push    bc
        ld      de,(__at_a)
        ld      hl,(__at_a + 2)
        call    ___fssub
        pop     bc
        pop     bc
        jp      at_ret
at_a_plain:
        ld      hl,(__at_a + 2)
        ld      de,(__at_a)
        jp      at_ret
at_large:
        ; atan = 0.5*PI - z/(z*z + 0.28); if y<0 atan -= PI
        ld      hl,(__at_z + 2)
        ld      bc,(__at_z)
        push    hl
        push    bc
        ld      de,(__at_z)
        ld      hl,(__at_z + 2)
        call    ___fsmul                ; z*z
        pop     bc
        pop     bc
        ld      (__at_t),de
        ld      (__at_t + 2),hl
        ld      hl,#0x3e8f              ; + 0.28
        ld      bc,#0x5c29
        push    hl
        push    bc
        ld      de,(__at_t)
        ld      hl,(__at_t + 2)
        call    ___fsadd
        pop     bc
        pop     bc
        ld      (__at_t),de
        ld      (__at_t + 2),hl
        ld      hl,(__at_t + 2)
        ld      bc,(__at_t)
        push    hl
        push    bc
        ld      de,(__at_z)
        ld      hl,(__at_z + 2)
        call    ___fsdiv                ; z / (z*z+0.28)
        pop     bc
        pop     bc
        ld      (__at_t),de
        ld      (__at_t + 2),hl
        ld      hl,(__at_t + 2)
        ld      bc,(__at_t)
        push    hl
        push    bc
        ld      de,#0x0fdb              ; 0.5*PI
        ld      hl,#0x3fc9
        call    ___fssub                ; 0.5*PI - t
        pop     bc
        pop     bc
        ld      (__at_a),de
        ld      (__at_a + 2),hl
        ld      a,(__at_y + 3)
        bit     7,a
        jr      z,at_large_plain
        ld      hl,#0x4049              ; atan -= PI
        ld      bc,#0x0fdb
        push    hl
        push    bc
        ld      de,(__at_a)
        ld      hl,(__at_a + 2)
        call    ___fssub
        pop     bc
        pop     bc
        jp      at_ret
at_large_plain:
        ld      hl,(__at_a + 2)
        ld      de,(__at_a)
at_ret:
        pop     ix
        ret
