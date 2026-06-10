        ;; mblen.s
        ;;
        ;; Stateless single-byte execution charset:
        ;; - NULL pointer resets/queries state and returns 0
        ;; - n == 0 cannot supply a complete character and returns -1
        ;; - every non-NUL byte is a valid one-byte character

        .module mblen
        .optsdcc -mz80 sdcccall(1)

        .globl  _mblen

        .area   _CODE

_mblen::
        ld      a,h
        or      l
        jr      z,mblen_zero
        ld      a,d
        or      e
        jr      z,mblen_fail
        ld      a,(hl)
        or      a
        jr      z,mblen_zero
        ld      de,#0x0001
        ret
mblen_zero:
        ld      de,#0x0000
        ret
mblen_fail:
        ld      de,#0xffff
        ret
