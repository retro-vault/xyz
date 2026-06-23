        .module mtx_timedlock
        .optsdcc -mz80 sdcccall(1)
        .globl  _mtx_timedlock
        .globl  __threads_mtx_timedlock_core
        .area   _CODE
_mtx_timedlock::
        jp      __threads_mtx_timedlock_core
