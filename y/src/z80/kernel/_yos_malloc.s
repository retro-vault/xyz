        ; Kernel-owned malloc adapter for the public YOS service table.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module _yos_malloc
        .optsdcc -mz80 sdcccall(1)

        .globl  __yos_malloc
        .globl  __heap
        .globl  _mem_allocate
        .globl  _thread_current
        .globl  _enter_critical_section
        .globl  _leave_critical_section

        .equ    THREAD_PROCESS, 22

        .area   _CODE

        ; __yos_malloc, internal service-table adapter
        ; inputs: hl = requested byte count
        ; outputs: de = payload or zero
        ; clobbers: af, bc, de, hl; preserves ix and iy
__yos_malloc::
        ex      de, hl
        call    _enter_critical_section
        ld      bc, #0
        ld      hl, (_thread_current)
        ld      a, h
        or      l
        jr      z, .owner_ready
        push    de
        ld      de, #THREAD_PROCESS
        add     hl, de
        ld      c, (hl)
        inc     hl
        ld      b, (hl)
        pop     de
.owner_ready:
        push    bc
        ld      hl, #__heap
        call    _mem_allocate
        call    _leave_critical_section
        ret
