        .module mtx_init
        .optsdcc -mz80 sdcccall(1)
        .globl  _mtx_init
        .globl  __threads_mtx_init_core
        .area   _CODE
_mtx_init::
        jp      __threads_mtx_init_core
