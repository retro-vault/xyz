        ; setjmp.s
        ;
        ; libc setjmp implementation for the xcc Z80 libc.
        ; Captures the minimum execution context required to re-enter xcc
        ; generated code later via longjmp.
        ;
        ; Private jmp_buf layout used by this implementation:
        ;   0..1 = caller stack pointer after the original setjmp returned
        ;   2..3 = saved return address
        ;   4..5 = saved IX frame pointer
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module setjmp
        .optsdcc -mz80 sdcccall(1)

        .globl  __setjmp

        .area   _CODE

        ; __setjmp
        ; inputs:
        ;   HL = pointer to jmp_buf storage
        ; outputs:
        ;   DE = 0 on the initial return
        ; clobbers: AF, BC, HL, IX
        ; notes:
        ;   The saved stack pointer is the caller-visible SP after RET, not the
        ;   callee entry SP. That lets longjmp recreate the exact post-call
        ;   stack layout before returning to the saved PC.
__setjmp:
        ld      c, l
        ld      b, h                    ; BC = jmp_buf pointer

        push    ix
        ld      ix, #0
        add     ix, sp

        ld      hl, #4
        add     hl, sp                  ; HL = caller SP after RET
        ld      a, l
        ld      (bc), a
        inc     bc
        ld      a, h
        ld      (bc), a
        inc     bc

        ld      a, 2(ix)                ; saved return address low byte
        ld      (bc), a
        inc     bc
        ld      a, 3(ix)                ; saved return address high byte
        ld      (bc), a
        inc     bc

        ld      a, 0(ix)                ; caller IX low byte
        ld      (bc), a
        inc     bc
        ld      a, 1(ix)                ; caller IX high byte
        ld      (bc), a

        ld      de, #0
        pop     ix
        ret
