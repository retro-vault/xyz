        .module tss_set
        .optsdcc -mz80 sdcccall(1)
        .globl  _tss_set
        .globl  __threads_tss_set_core
        .area   _CODE
_tss_set::
        jp      __threads_tss_set_core
