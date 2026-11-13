        .module next_vectors
        .globl __sys_vec_tbl, __sys_vectors_start, __sys_vectors_end
        .globl __sys_reti, __sys_retn
        .area _CODE
__sys_vectors_start::
        .db 0xc3
        .dw __sys_reti
        .db 0xc3
        .dw __sys_reti
        .db 0xc3
        .dw __sys_reti
        .db 0xc3
        .dw __sys_reti
        .db 0xc3
        .dw __sys_reti
        .db 0xc3
        .dw __sys_reti
        .db 0xc3
        .dw __sys_reti
        .db 0xc3
        .dw __sys_retn
__sys_vectors_end::
        .area _BSS
__sys_vec_tbl::
        .ds 24
