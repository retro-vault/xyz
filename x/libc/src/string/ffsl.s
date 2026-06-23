        ; ffsl.s
        ;
        ; libc ffsl implementation for the xcc Z80 libc.
        ; Returns the one-based index of the least-significant set bit of a
        ; 32-bit long, or 0 if the value is zero (POSIX/GNU).
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module ffsl
        .optsdcc -mz80 sdcccall(1)


        .globl  _ffsl
        .globl  __ffs_scan_byte

        .area   _CODE

        ; _ffsl
        ; inputs:  DE = low 16 bits, HL = high 16 bits of long
        ; outputs: DE = 1-based index of the lowest set bit, or 0
        ; clobbers: AF, BC
_ffsl::
        ld      a,e
        or      a
        ld      c,#0
        jr      nz,ffsl_scan
        ld      a,d
        or      a
        ld      c,#8
        jr      nz,ffsl_scan
        ld      a,l
        or      a
        ld      c,#16
        jr      nz,ffsl_scan
        ld      a,h
        or      a
        jr      z,ffsl_none
        ld      c,#24
ffsl_scan:
        call    __ffs_scan_byte
        ld      a,c
        add     a,b
        ld      e,a
        ld      d,#0
        ret
ffsl_none:
        ld      de,#0x0000
        ret
