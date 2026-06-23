        .module cnd_timedwait
        .optsdcc -mz80 sdcccall(1)
        .globl  _cnd_timedwait
        .globl  __threads_cnd_timedwait_core
        .area   _CODE
_cnd_timedwait::
        jp      __threads_cnd_timedwait_core
