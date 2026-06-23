        ;; mbrlen.s
        ;;
        ;; Restartable multibyte length probe for the target's stateless
        ;; single-byte execution charset.
        ;;
        ;; Behaviour differs from mblen() in one important standard detail:
        ;; when s is non-NULL but n == 0, mbrlen() reports an incomplete
        ;; multibyte character with (size_t)-2, not -1.

        .module mbrlen
        .optsdcc -mz80 sdcccall(1)

        .globl  _mbrlen

        .area   _CODE

_mbrlen::
        ;; NULL input probes or resets the initial shift state. This target is
        ;; always in the initial state, so the result is 0.
        ld      a,h
        or      l
        jr      z,mbrlen_zero

        ;; No bytes available from a non-NULL source means "incomplete
        ;; multibyte character", which the restartable APIs report as -2.
        ld      a,d
        or      e
        jr      z,mbrlen_incomplete

        ;; A NUL byte is a complete multibyte sequence of length 0; every
        ;; other byte in the execution charset consumes exactly one byte.
        ld      a,(hl)
        or      a
        jr      z,mbrlen_zero
        ld      de,#0x0001
        ret

mbrlen_zero:
        ld      de,#0x0000
        ret

mbrlen_incomplete:
        ld      de,#0xfffe
        ret
