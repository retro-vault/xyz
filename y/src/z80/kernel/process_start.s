        ; Create a process and its initial thread.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module process_start
        .optsdcc -mz80 sdcccall(1)

        .globl  _process_start
        .globl  _process_first
        .globl  _so_create
        .globl  _so_destroy
        .globl  _thread_create
        .globl  _thread_resume
        .globl  __string_copy

        .equ    PROCESS_SIZE,           15
        .equ    PROCESS_FLAGS,           4
        .equ    PROCESS_NAME,            5
        .equ    PROCESS_MAIN_THREAD,    13
        .equ    THREAD_PROCESS,         22

        .area   _CODE

        ; _process_start, sdcccall(1)
        ; inputs: hl = name, de = entry, stack size at sp+2
        ; outputs: de = process or zero; removes stack-size argument
        ; clobbers: af, bc, de, hl; preserves ix and iy
        ; frame: name -2, entry -4, process -6, stack size +4
_process_start::
        push    ix
        ld      ix, #0
        add     ix, sp
        push    hl
        push    de
        ld      hl, #0
        push    hl
        ld      de, #PROCESS_SIZE
        ld      hl, #_process_first
        call    _so_create
        push    de
        ld      a, d
        or      e
        jr      z, .start_return

        ld      hl, #PROCESS_NAME
        add     hl, de
        ld      e, -2(ix)
        ld      d, -1(ix)
        call    __string_copy
        ld      l, -6(ix)
        ld      h, -5(ix)
        ld      de, #PROCESS_FLAGS
        add     hl, de
        ld      (hl), #0

        ld      l, -6(ix)
        ld      h, -5(ix)
        push    hl                      ; thread owner process
        ld      e, 4(ix)
        ld      d, 5(ix)
        ld      l, -4(ix)
        ld      h, -3(ix)
        call    _thread_create

        ld      l, -6(ix)
        ld      h, -5(ix)
        ld      bc, #PROCESS_MAIN_THREAD
        add     hl, bc
        ld      (hl), e
        inc     hl
        ld      (hl), d
        ld      a, d
        or      e
        jr      nz, .start_thread_ready

        ld      e, -6(ix)
        ld      d, -5(ix)
        ld      hl, #_process_first
        call    _so_destroy
        xor     a
        ld      -6(ix), a
        ld      -5(ix), a
        jr      .start_return

.start_thread_ready:
        push    de
        ex      de, hl
        ld      de, #THREAD_PROCESS
        add     hl, de
        ld      e, -6(ix)
        ld      d, -5(ix)
        ld      (hl), e
        inc     hl
        ld      (hl), d
        pop     hl
        call    _thread_resume

.start_return:
        ld      e, -6(ix)
        ld      d, -5(ix)
        ld      sp, ix
        pop     ix
        pop     hl                      ; return address
        pop     bc                      ; stack size
        push    hl
        ret
