        .module tss_create
        .optsdcc -mz80 sdcccall(1)
        .globl  _tss_create
        .globl  __threads_tss_create_core
        .area   _CODE
_tss_create::
        jp      __threads_tss_create_core
