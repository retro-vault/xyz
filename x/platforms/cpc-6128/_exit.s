        ; Return a CPC firmware-hosted program to its BASIC caller.

        .module _exit
        .optsdcc -mz80 sdcccall(1)
        .globl  __exit
        .globl  _cpc_exit_status

CPC_SAVED_SP    .equ    0xa6fa

        .area   _CODE
__exit::
        ld      (_cpc_exit_status),hl
        ld      sp,#CPC_SAVED_SP
        pop     de
        ex      de,hl
        ld      sp,hl
        ret

        .area   _BSS
_cpc_exit_status::
        .ds     2
