        ; atoi.s — compact decimal conversion for the 16-bit int model.
        ;
        ; Keep this implementation independent of strtol(): model S omits the
        ; long text-conversion family, but atoi() remains part of its core
        ; stdlib surface.  Overflow has undefined behaviour for atoi(), so the
        ; natural modulo-16-bit accumulation is sufficient here.
        ; MIT License (see: LICENSE)  Copyright (C) 2026 tomaz stih
        .module atoi
        .optsdcc -mz80 sdcccall(1)
        .globl  _atoi
        .area   _CODE
_atoi::
        ld      de,#0                   ; 16-bit accumulator
        ld      b,#0                    ; nonzero for a leading '-'

atoi_space:
        ld      a,(hl)
        cp      #0x20                   ; ordinary space
        jr      z,atoi_space_next
        cp      #0x09                   ; \t, \n, \v, \f, \r
        jr      c,atoi_sign
        cp      #0x0e
        jr      nc,atoi_sign
atoi_space_next:
        inc     hl
        jr      atoi_space

atoi_sign:
        cp      #'-'
        jr      nz,atoi_plus
        inc     b
        inc     hl
        jr      atoi_digits
atoi_plus:
        cp      #'+'
        jr      nz,atoi_digits
        inc     hl

atoi_digits:
        ld      a,(hl)
        sub     #'0'
        cp      #10
        jr      nc,atoi_done
        ld      c,a                     ; preserve the new digit

        ; DE = DE * 10 + C.  HL holds the input cursor, so preserve it while
        ; using HL/DE for the shift-add sequence (10*x = 8*x + 2*x).
        push    hl
        ld      h,d
        ld      l,e
        add     hl,hl                   ; 2*x
        push    hl
        add     hl,hl                   ; 4*x
        add     hl,hl                   ; 8*x
        pop     de                     ; DE = 2*x
        add     hl,de                   ; HL = 10*x
        ld      e,c
        ld      d,#0
        add     hl,de                   ; HL = 10*x + digit
        ex      de,hl                   ; accumulator back in DE
        pop     hl                     ; restore input cursor
        inc     hl
        jr      atoi_digits

atoi_done:
        ld      a,b
        or      a
        ret     z
        xor     a                       ; DE = -DE
        sub     e
        ld      e,a
        ld      a,#0
        sbc     a,d
        ld      d,a
        ret
