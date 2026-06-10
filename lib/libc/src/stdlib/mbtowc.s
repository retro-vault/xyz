        ;; mbtowc.s
        ;;
        ;; Convert one byte from the execution charset into a 16-bit wchar_t.
        ;; This target uses a stateless single-byte encoding, so any non-NUL
        ;; byte converts directly and consumes one input byte.

        .module mbtowc
        .optsdcc -mz80 sdcccall(1)

        .globl  _mbtowc

        .area   _CODE

_mbtowc::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      a,d
        or      e
        jr      z,mbtowc_zero
        ld      c,4(ix)
        ld      b,5(ix)
        ld      a,b
        or      c
        jr      z,mbtowc_fail
        ld      a,(de)
        ld      c,a
        ld      a,h
        or      l
        jr      z,mbtowc_nostore
        ld      (hl),c
        inc     hl
        xor     a
        ld      (hl),a
mbtowc_nostore:
        ld      a,c
        or      a
        jr      z,mbtowc_zero_pop
        ld      de,#0x0001
        pop     ix
        ret
mbtowc_zero:
        ld      a,h
        or      l
        jr      z,mbtowc_zero_pop
        xor     a
        ld      (hl),a
        inc     hl
        ld      (hl),a
mbtowc_zero_pop:
        ld      de,#0x0000
        pop     ix
        ret
mbtowc_fail:
        ld      de,#0xffff
        pop     ix
        ret
