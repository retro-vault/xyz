        .module tss_delete
        .optsdcc -mz80 sdcccall(1)
        .globl  _tss_delete
        .globl  __threads_tss_delete_core
        .area   _CODE
_tss_delete::
        jp      __threads_tss_delete_core
