        .module cnd_init
        .optsdcc -mz80 sdcccall(1)
        .globl  _cnd_init
        .globl  __threads_cnd_init_core
        .area   _CODE
_cnd_init::
        jp      __threads_cnd_init_core
