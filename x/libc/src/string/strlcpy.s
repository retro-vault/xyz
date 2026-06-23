        ; strlcpy.s
        ;
        ; libc strlcpy implementation for the xcc Z80 libc.
        ; Copies src into a dest buffer of the given size, always NUL
        ; terminating (unless size==0), and returns strlen(src) (BSD).
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module strlcpy
        .optsdcc -mz80 sdcccall(1)


        .globl  _strlcpy

        .area   _CODE

        ; _strlcpy
        ; inputs:  HL = destination, DE = source, 4(ix)..5(ix) = dest size
        ; outputs: DE = strlen(source)
        ; clobbers: AF, BC, HL, IX
_strlcpy::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      c,4(ix)
        ld      b,5(ix)                 ; BC = size
        push    de                      ; remember src to measure later
        ; copy while size-1 > 0 and src not NUL
        ld      a,b
        or      c
        jr      z,strlcpy_measure       ; size == 0: copy nothing
strlcpy_copy:
        dec     bc                      ; reserve room for the NUL
        ld      a,b
        or      c
        jr      z,strlcpy_term          ; only room for the terminator left
        ld      a,(de)
        ld      (hl),a
        or      a
        jr      z,strlcpy_measure_pop   ; copied the NUL: done copying
        inc     hl
        inc     de
        jr      strlcpy_copy
strlcpy_term:
        ld      (hl),#0x00              ; force terminate truncated copy
strlcpy_measure_pop:
strlcpy_measure:
        pop     de                      ; DE = original source
        ; return strlen(source)
        ld      h,d
        ld      l,e                     ; HL = source
        xor     a
        ld      bc,#0xffff
        cpir                            ; scan to NUL
        ; length = (0xFFFF - BC) - 1
        ld      a,#0xff
        sub     c
        ld      e,a
        ld      a,#0xff
        sbc     a,b
        ld      d,a
        dec     de                      ; exclude the NUL
        pop     ix
        ret
