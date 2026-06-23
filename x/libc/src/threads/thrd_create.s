        .module thrd_create
        .optsdcc -mz80 sdcccall(1)
        .globl  _thrd_create
        .globl  __threads_thrd_create_core
        .area   _CODE
_thrd_create::
        jp      __threads_thrd_create_core
