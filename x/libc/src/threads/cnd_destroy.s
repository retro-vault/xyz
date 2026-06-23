        .module cnd_destroy
        .optsdcc -mz80 sdcccall(1)
        .globl  _cnd_destroy
        .globl  __threads_cnd_destroy_core
        .area   _CODE
_cnd_destroy::
        jp      __threads_cnd_destroy_core
