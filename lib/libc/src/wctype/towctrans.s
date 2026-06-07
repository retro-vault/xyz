        ; towctrans.s
        ;
        ; libc towctrans() for the xcc Z80 libc.  1 -> towlower, 2 -> towupper,
        ; anything else returns wc unchanged.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module towctrans
        .optsdcc -mz80 sdcccall(1)
        .globl  _towctrans
        .globl  _towlower
        .globl  _towupper
        .area   _CODE

        ; _towctrans: HL = wc, DE = desc -> DE = mapped wc
_towctrans::
        ld      a,d
        or      a
        jr      nz,twct_id
        ld      a,e
        cp      #1
        jr      z,twct_lower
        cp      #2
        jr      z,twct_upper
twct_id:
        ex      de,hl
        ret
twct_lower:
        jp      _towlower
twct_upper:
        jp      _towupper
