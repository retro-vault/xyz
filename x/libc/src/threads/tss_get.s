        .module tss_get
        .optsdcc -mz80 sdcccall(1)
        .globl  _tss_get
        .globl  __threads_tss_get_core
        .area   _CODE
_tss_get::
        jp      __threads_tss_get_core
