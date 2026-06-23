        .module cnd_wait
        .optsdcc -mz80 sdcccall(1)
        .globl  _cnd_wait
        .globl  __threads_cnd_wait_core
        .area   _CODE
_cnd_wait::
        jp      __threads_cnd_wait_core
