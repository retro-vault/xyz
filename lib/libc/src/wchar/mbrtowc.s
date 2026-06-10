        ;; mbrtowc.s
        ;;
        ;; Restartable multibyte-to-wide conversion for the target's stateless
        ;; single-byte execution charset.
        ;;
        ;; The existing mbtowc() core already handles the actual widening
        ;; rules. This wrapper only needs to add the restartable semantics:
        ;;   - s == NULL reports the initial shift state (always 0 here)
        ;;   - s != NULL and n == 0 reports an incomplete sequence with -2
        ;;   - otherwise mbtowc() already implements the right conversion

        .module mbrtowc
        .optsdcc -mz80 sdcccall(1)

        .globl  _mbrtowc
        .globl  _mbtowc

        .area   _CODE

_mbrtowc::
        ;; A NULL source pointer queries the current shift state. The target
        ;; encoding is stateless, so this always reports success with length 0.
        ld      a,d
        or      e
        jr      z,mbrtowc_zero

        ;; The third argument (n) lives at 4(ix) under sdcccall(1). When no
        ;; bytes are available from a non-NULL source the standard restartable
        ;; APIs report an incomplete multibyte character via (size_t)-2.
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      c,4(ix)
        ld      b,5(ix)
        pop     ix
        ld      a,b
        or      c
        jr      z,mbrtowc_incomplete

        jp      _mbtowc

mbrtowc_zero:
        ld      de,#0x0000
        ret

mbrtowc_incomplete:
        ld      de,#0xfffe
        ret
