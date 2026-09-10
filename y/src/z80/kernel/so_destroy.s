        ; Unlink and free a system object.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module so_destroy
        .optsdcc -mz80 sdcccall(1)
        .globl  _so_destroy
        .globl  __sys_heap
        .globl  _mem_free
        .globl  _list_remove
        .area   _CODE

_so_destroy::
        call    _list_remove
        ld      a, d
        or      e
        ret     z
        ld      hl, #__sys_heap
        jp      _mem_free
