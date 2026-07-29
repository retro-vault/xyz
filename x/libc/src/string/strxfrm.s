        ; strxfrm.s
        ;
        ; libc strxfrm implementation for the xcc Z80 libc.
        ; With no locale collation tables yet, the transformed form is just the
        ; source string itself. The helper still reports the full transformed
        ; length, even if the destination buffer is shorter.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module strxfrm
        .optsdcc -mz80 sdcccall(1)


        .globl  _strxfrm
        .globl  __string_scan_nul

        .area   _CODE

        ; _strxfrm
        ; inputs:
        ;   HL         = destination buffer
        ;   DE         = source string
        ;   4(ix)..5(ix) = destination buffer size
        ; outputs:
        ;   DE = transformed string length
        ; clobbers: AF, BC, HL, IX
        ; notes:
        ;   The original destination pointer is parked on the stack while HL is
        ;   reused to compute the source length.
_strxfrm::
        push    ix
        ld      ix,#0
        add     ix,sp
        push    hl                      ; preserve destination pointer
        push    de                      ; preserve source pointer
        ex      de,hl                   ; HL = source for the scan helper
        call    __string_scan_nul
        pop     de
        or      a                       ; clear carry before subtracting
        sbc     hl,de                   ; HL = transformed length
        ex      (sp),hl                 ; stash length, recover destination
        ld      a,4(ix)
        or      5(ix)
        jr      z,strxfrm_return_len
        ld      c,4(ix)
        ld      b,5(ix)
strxfrm_copy:
        ld      a,b
        or      c
        jr      z,strxfrm_return_len
        ld      a,(de)
        ld      (hl),a                  ; copy the identity transform byte
        inc     de
        inc     hl
        dec     bc
        or      a
        jr      z,strxfrm_return_len
        jr      strxfrm_copy
strxfrm_return_len:
        pop     de
        pop     ix
        ret
