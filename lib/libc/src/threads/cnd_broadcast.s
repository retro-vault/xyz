        .module cnd_broadcast
        .optsdcc -mz80 sdcccall(1)
        .globl  _cnd_broadcast
        .globl  __threads_cnd_broadcast_core
        .area   _CODE
_cnd_broadcast::
        jp      __threads_cnd_broadcast_core
