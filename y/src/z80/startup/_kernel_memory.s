        ; Fixed system stack and heap reservations for the kernel.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module _kernel_memory
        .optsdcc -mz80 sdcccall(1)

        .globl  __sys_stack
        .globl  __sys_heap
        .globl  __heap

        .area   _BSS
        .ds     512
__sys_stack::

        .area   _HEAP
__sys_heap::
        .ds     1024
__heap::
