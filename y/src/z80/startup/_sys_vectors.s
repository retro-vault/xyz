        ; RAM restart table and its immutable ROM initialization image.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module _sys_vectors
        .optsdcc -mz80 sdcccall(1)

        .globl  __sys_vec_tbl
        .globl  __sys_vectors_start
        .globl  __sys_vectors_end
        .globl  __sys_reti
        .globl  __sys_retn

        ; The ROM image installs these 24 template bytes in the free space
        ; immediately before the fixed 3D00h divIDE region. See patch_rom.py.
__sys_vectors_start = 0x3ce5
__sys_vectors_end = 0x3cfd
__sys_reti = 0x09f0
__sys_retn = 0x09f2

        .area   _BSS
__sys_vec_tbl::
        .ds     24
