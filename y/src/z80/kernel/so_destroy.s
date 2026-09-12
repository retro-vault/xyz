        ; Unlink and free a system object.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module so_destroy
        .optsdcc -mz80 sdcccall(1)
        .globl  _so_destroy
        .globl  __critical_call
        .globl  __sys_heap
        .globl  _mem_free
        .globl  _list_remove
        .area   _CODE

        ; inputs: HL = list head address, DE = object; no stack arguments.
        ; outputs: DE = freed/merged payload or zero if absent.
        ; clobbers: AF, BC, DE, HL and alternate DE/HL; preserves IX/IY.
        ; Protect unlink and free together, including calls from the scheduler.
_so_destroy::
        call    __critical_call
        call    _list_remove
        ld      a, d
        or      e
        ret     z
        ld      hl, #__sys_heap
        jp      _mem_free
