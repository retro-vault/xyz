        .module mtx_lock
        .optsdcc -mz80 sdcccall(1)
        .globl  _mtx_lock
        .globl  __threads_mtx_lock_core
        .area   _CODE
_mtx_lock::
        jp      __threads_mtx_lock_core
