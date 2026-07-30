        ; div.s
        ;
        ; libc div implementation for the xcc Z80 libc.
        ; Computes the signed 16-bit quotient and remainder in one call.
        ; The SDCC ABI passes a hidden pointer to caller-owned div_t storage
        ; in the stack word nearest the return address.
        ;
        ; Delegates to the shared signed-divide core: __divsint produces the
        ; quotient (DE) and an unsigned remainder (HL), and __get_remainder
        ; corrects the remainder sign to match the dividend (C truncation),
        ; preserving the quotient in DE.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module div
        .optsdcc -mz80 sdcccall(1)


        .globl  _div
        .globl  __divsint
        .globl  __get_remainder

        .area   _CODE

        ; _div
        ; inputs:  HL = numerator, DE = denominator (both signed int)
        ; hidden:  result pointer at 4(ix)
        ; clobbers: AF, BC, DE, HL
_div::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __divsint               ; DE = quot, HL = unsigned rem
        call    __get_remainder         ; HL = signed rem, DE = quot

        push    hl                      ; preserve remainder
        push    de                      ; preserve quotient
        ld      l,4(ix)
        ld      h,5(ix)                 ; HL = caller result
        pop     de
        ld      (hl),e                  ; quot
        inc     hl
        ld      (hl),d
        inc     hl
        pop     bc
        ld      (hl),c                  ; rem
        inc     hl
        ld      (hl),b

        pop     ix
        ret
