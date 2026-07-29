        ; longjmp.s
        ;
        ; libc longjmp implementation for the xcc Z80 libc.
        ; Restores the execution context captured by setjmp and resumes
        ; execution at the saved return site with the requested return value.
        ;
        ; Private jmp_buf layout used by this implementation:
        ;   0..1 = caller stack pointer after the original setjmp returned
        ;   2..3 = saved return address
        ;   4..5 = saved IX frame pointer
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module longjmp
        .optsdcc -mz80 sdcccall(1)

        .globl  ___longjmp

        .area   _CODE

        ; ___longjmp (C identifier __longjmp)
        ; inputs:
        ;   HL = pointer to jmp_buf
        ;   DE = value that setjmp should appear to return
        ; outputs:
        ;   does not return normally; resumes at the saved setjmp call site
        ; clobbers: AF, BC, DE, HL, IX, IY
        ; notes:
        ;   The saved program counter is pushed onto the restored stack so a
        ;   plain RET re-enters the original caller with the right stack shape.
___longjmp:
        push    de                      ; preserve requested setjmp return value

        ld      c, (hl)
        inc     hl
        ld      b, (hl)
        inc     hl                      ; BC = restored SP

        ld      e, (hl)
        inc     hl
        ld      d, (hl)
        inc     hl                      ; DE = saved return address
        push    de
        pop     iy                      ; IY = saved return address

        ld      e, (hl)
        inc     hl
        ld      d, (hl)                 ; DE = saved caller IX
        push    de
        pop     ix

        pop     de                      ; DE = requested return value
        ld      a, d
        or      e
        jr      nz, longjmp_have_value
        ld      de, #1                  ; longjmp(..., 0) makes setjmp return 1
longjmp_have_value:
        ld      h, b
        ld      l, c
        ld      sp, hl                  ; restore caller stack
        push    iy                      ; recreate the saved return address
        ret
