        .module mtx_trylock
        .optsdcc -mz80 sdcccall(1)
        .globl  _mtx_trylock
        .globl  __threads_mtx_trylock_core
        .area   _CODE
_mtx_trylock::
        jp      __threads_mtx_trylock_core
