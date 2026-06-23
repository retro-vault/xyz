        .module mtx_destroy
        .optsdcc -mz80 sdcccall(1)
        .globl  _mtx_destroy
        .globl  __threads_mtx_destroy_core
        .area   _CODE
_mtx_destroy::
        jp      __threads_mtx_destroy_core
