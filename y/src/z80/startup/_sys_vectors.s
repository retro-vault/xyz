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

        .area   _CONST
__sys_vectors_start::
        jp      __sys_reti
        jp      __sys_reti
        jp      __sys_reti
        jp      __sys_reti
        jp      __sys_reti
        jp      __sys_reti
        jp      __sys_reti
        jp      __sys_retn
__sys_vectors_end::

        .area   _BSS
__sys_vec_tbl::
        .ds     24
