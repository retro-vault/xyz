        ; ffsll.s
        ;
        ; libc ffsll implementation for the xcc Z80 libc.
        ; Returns the one-based index of the least-significant set bit of a
        ; 64-bit long long, or 0 if zero (GNU).
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module ffsll
        .optsdcc -mz80 sdcccall(1)


        .globl  _ffsll
        .globl  __ffs_scan_byte

        .area   _CODE

        ; _ffsll
        ; inputs:  DE:HL:DE':HL' = long long (DE = lsw .. HL' = msw)
        ; outputs: DE = 1-based index of the lowest set bit, or 0
        ; clobbers: AF, BC, DE', HL'
_ffsll::
        ld      a,e
        or      a
        ld      c,#0
        jr      nz,ffsll_scan
        ld      a,d
        or      a
        ld      c,#8
        jr      nz,ffsll_scan
        ld      a,l
        or      a
        ld      c,#16
        jr      nz,ffsll_scan
        ld      a,h
        or      a
        ld      c,#24
        jr      nz,ffsll_scan
        exx
        ld      a,e
        exx
        or      a
        ld      c,#32
        jr      nz,ffsll_scan_alt
        exx
        ld      a,d
        exx
        or      a
        ld      c,#40
        jr      nz,ffsll_scan_alt
        exx
        ld      a,l
        exx
        or      a
        ld      c,#48
        jr      nz,ffsll_scan_alt
        exx
        ld      a,h
        exx
        or      a
        jr      z,ffsll_none
        ld      c,#56
ffsll_scan_alt:
        ; A already holds the byte under test
ffsll_scan:
        call    __ffs_scan_byte
        ld      a,c
        add     a,b
        ld      e,a
        ld      d,#0
        ret
ffsll_none:
        ld      de,#0x0000
        ret
