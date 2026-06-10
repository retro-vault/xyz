        .module cnd_signal
        .optsdcc -mz80 sdcccall(1)
        .globl  _cnd_signal
        .globl  __threads_cnd_signal_core
        .area   _CODE
_cnd_signal::
        jp      __threads_cnd_signal_core
