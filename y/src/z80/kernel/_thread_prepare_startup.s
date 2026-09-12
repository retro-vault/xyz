        ; Emit the per-thread startup and exit stub.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module thread_prepare_startup
        .optsdcc -mz80 sdcccall(1)
        .globl  _thread_prepare_startup
        .globl  _thread_exit
        .area   _CODE

        ; inputs: hl = thread, de = entry
        ; outputs: stub and initial return PC installed
        ; clobbers: af, bc, de, hl; preserves ix and iy
_thread_prepare_startup::
        ld      b, h
        ld      c, l
        push    hl
        inc     hl
        inc     hl
        inc     hl
        inc     hl
        ld      a, (hl)
        inc     hl
        ld      h, (hl)
        ld      l, a
        push    de
        ld      de, #20
        add     hl, de
        ex      de, hl
        ld      hl, #6
        add     hl, bc
        ld      a, l
        ld      (de), a
        inc     de
        ld      a, h
        ld      (de), a
        pop     de
        ld      (hl), #0xcd
        inc     hl
        ld      (hl), e
        inc     hl
        ld      (hl), d
        inc     hl
        ld      (hl), #0x21
        inc     hl
        ld      (hl), c
        inc     hl
        ld      (hl), b
        inc     hl
        ld      (hl), #0xc3
        inc     hl
        ld      de, #_thread_exit
        ld      (hl), e
        inc     hl
        ld      (hl), d
        inc     hl
        ld      (hl), #0
        pop     hl
        ret
