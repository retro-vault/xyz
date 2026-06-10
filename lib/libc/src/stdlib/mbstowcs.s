        ;; mbstowcs.s
        ;;
        ;; Expand a narrow string into wchar_t elements under the target's
        ;; one-byte execution charset. Each source byte becomes one 16-bit
        ;; code unit with a zero high byte.

        .module mbstowcs
        .optsdcc -mz80 sdcccall(1)

        .globl  _mbstowcs

        .area   _CODE

_mbstowcs::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      c,4(ix)
        ld      b,5(ix)                 ; BC = destination element limit
        push    bc                      ; save original limit for the return value
        ld      a,b
        or      c
        jr      z,mbstowcs_done
        ex      de,hl                   ; HL = src, DE = dst
mbstowcs_loop:
        ld      a,b
        or      c
        jr      z,mbstowcs_done_pop
        ld      a,(hl)
        or      a
        jr      z,mbstowcs_term
        ld      (de),a
        inc     de
        xor     a
        ld      (de),a
        inc     de
        inc     hl
        dec     bc
        jr      mbstowcs_loop
mbstowcs_term:
        xor     a
        ld      (de),a
        inc     de
        ld      (de),a
mbstowcs_done_pop:
        pop     hl
        or      a
        sbc     hl,bc
        ex      de,hl
        pop     ix
        ret
mbstowcs_done:
        pop     hl
        ld      de,#0x0000
        pop     ix
        ret
