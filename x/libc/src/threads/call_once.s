        .module call_once
        .optsdcc -mz80 sdcccall(1)
        .globl  _call_once
        .globl  __threads_call_once_core
        .area   _CODE
_call_once::
        jp      __threads_call_once_core
