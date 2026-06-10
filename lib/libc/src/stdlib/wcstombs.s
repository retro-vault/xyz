        ;; wcstombs.s
        ;;
        ;; Collapse a wchar_t string into the one-byte execution charset.
        ;; Any code point outside the unsigned-byte range reports EILSEQ.

        .module wcstombs
        .optsdcc -mz80 sdcccall(1)

        .globl  _wcstombs
        .globl  __errno_value

        .area   _CODE

_wcstombs::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      c,4(ix)
        ld      b,5(ix)                 ; BC = destination byte limit
        push    bc                      ; save original limit for the return value
        ld      a,b
        or      c
        jr      z,wcstombs_done
        ex      de,hl                   ; HL = src wide string, DE = dst bytes
wcstombs_loop:
        ld      a,b
        or      c
        jr      z,wcstombs_done_pop
        push    bc
        ld      c,(hl)
        inc     hl
        ld      a,(hl)
        inc     hl
        or      a
        jr      nz,wcstombs_bad
        ld      a,c
        pop     bc
        or      a
        jr      z,wcstombs_term
        ex      de,hl                   ; HL = dst, DE = advanced src
        ld      (hl),a
        inc     hl
        ex      de,hl                   ; restore HL = src, DE = dst + 1
        dec     bc
        jr      wcstombs_loop
wcstombs_term:
        ex      de,hl                   ; HL = dst
        xor     a
        ld      (hl),a
        ex      de,hl
wcstombs_done_pop:
        pop     hl
        or      a
        sbc     hl,bc
        ex      de,hl
        pop     ix
        ret
wcstombs_bad:
        pop     bc
        pop     hl
        ld      hl,#84
        ld      (__errno_value),hl
        ld      de,#0xffff
        pop     ix
        ret
wcstombs_done:
        pop     hl
        ld      de,#0x0000
        pop     ix
        ret
