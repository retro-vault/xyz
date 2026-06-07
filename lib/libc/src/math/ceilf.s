        ; ceilf.s
        ;
        ; libc ceilf implementation for the xcc Z80 libc.
        ; Rounds toward positive infinity.  double / long double are 32-bit
        ; on this target, so ceil / ceill share the implementation.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module ceilf
        .optsdcc -mz80 sdcccall(1)


        .globl  _ceilf
        .globl  _ceil
        .globl  _ceill
        .globl  _truncf
        .globl  __float_incmag

        .area   _CODE

        ; _ceilf / _ceil / _ceill
        ; inputs:  HL:DE = float x
        ; outputs: HL:DE = ceil(x)
        ; clobbers: AF, BC, IX
_ceil::
_ceill::
_ceilf::
        push    de
        push    hl
        call    _truncf                 ; HL:DE = trunc(x)
        pop     bc
        ld      a,h
        cp      b
        jr      nz,ceilf_frac_hl
        ld      a,l
        cp      c
        jr      nz,ceilf_frac_hl
        pop     bc
        ld      a,d
        cp      b
        jr      nz,ceilf_frac
        ld      a,e
        cp      c
        jr      nz,ceilf_frac
        ret                             ; no fraction: ceil == trunc
ceilf_frac_hl:
        pop     bc
ceilf_frac:
        bit     7,h                     ; sign of trunc(x)
        ret     nz                      ; x < 0: ceil == trunc
        jp      __float_incmag          ; x > 0 with fraction: round up
