        .module mtx_unlock
        .optsdcc -mz80 sdcccall(1)
        .globl  _mtx_unlock
        .globl  __threads_mtx_unlock_core
        .area   _CODE
_mtx_unlock::
        jp      __threads_mtx_unlock_core
