        ; ffs.s
        ;
        ; libc ffs implementation for the xcc Z80 libc.
        ; Returns the one-based index of the least-significant set bit of a
        ; 16-bit int, or 0 if the value is zero (POSIX).
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module ffs
        .optsdcc -mz80 sdcccall(1)


        .globl  _ffs
        .globl  __ffs_scan_byte

        .area   _CODE

        ; _ffs
        ; inputs:  HL = int value
        ; outputs: DE = 1-based index of the lowest set bit, or 0
        ; clobbers: AF, BC
_ffs::
        ld      a,l
        or      a
        jr      nz,ffs_low
        ld      a,h
        or      a
        jr      z,ffs_none
        ld      c,#8                    ; bit lives in the high byte
        jr      ffs_scan
ffs_low:
        ld      c,#0
ffs_scan:
        call    __ffs_scan_byte         ; B = 1-based bit index within A
        ld      a,c
        add     a,b
        ld      e,a
        ld      d,#0
        ret
ffs_none:
        ld      de,#0x0000
        ret

        ; __ffs_scan_byte
        ; inputs:  A = non-zero byte
        ; outputs: B = 1-based index of the lowest set bit (1..8)
        ; clobbers: AF
__ffs_scan_byte::
        ld      b,#1
ffs_scan_byte_loop:
        rra
        jr      c,ffs_scan_byte_done
        inc     b
        jr      ffs_scan_byte_loop
ffs_scan_byte_done:
        ret
