        .module _exit
        .optsdcc -mz80 sdcccall(1)
        .globl  __exit
        .globl  _zx_exit_status
        .area   _CODE
__exit::
        ld      (_zx_exit_status),hl
        di
.zx_exit_halt:
        halt
        jr      .zx_exit_halt
        .area   _BSS
_zx_exit_status::
        .ds     2
