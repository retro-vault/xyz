        ;; wcsxfrm.s
        ;;
        ;; Locale-neutral wide transform. The transformed form is the source
        ;; string itself, and the reported length is the source length in wide
        ;; characters excluding the terminator.

        .module wcsxfrm
        .optsdcc -mz80 sdcccall(1)

        .globl  _wcsxfrm

        .area   _CODE

_wcsxfrm::
        push    ix
        ld      ix,#0
        add     ix,sp
        push    hl                      ; save destination at ix-2..-1
        push    de                      ; save source at ix-4..-3

        ;; Count the full source length first so the return value is stable
        ;; even when the destination buffer truncates.
        ld      e,-4(ix)
        ld      d,-3(ix)
        ld      bc,#0x0000
wcsxfrm_scan:
        ld      a,(de)
        inc     de
        ld      l,a
        ld      a,(de)
        ld      h,a
        inc     de
        ld      a,h
        or      l
        jr      z,wcsxfrm_scan_done
        inc     bc
        jr      wcsxfrm_scan
wcsxfrm_scan_done:
        push    bc                      ; preserve reported length

        ;; n == 0 means "report only".
        ld      a,4(ix)
        or      5(ix)
        jr      z,wcsxfrm_ret

        ;; Copy at most n-1 transformed wide elements, then append a wide NUL.
        ld      c,4(ix)
        ld      b,5(ix)
        dec     bc
        ld      l,-2(ix)
        ld      h,-1(ix)
        ld      e,-4(ix)
        ld      d,-3(ix)
wcsxfrm_copy:
        ld      a,b
        or      c
        jr      z,wcsxfrm_term
        ld      a,(de)
        ld      (hl),a
        push    af
        inc     de
        inc     hl
        ld      a,(de)
        ld      (hl),a
        inc     de
        inc     hl
        dec     hl
        pop     af
        or      (hl)
        inc     hl
        jr      z,wcsxfrm_term_back
        dec     bc
        jr      wcsxfrm_copy
wcsxfrm_term_back:
        dec     hl
        dec     hl
wcsxfrm_term:
        xor     a
        ld      (hl),a
        inc     hl
        ld      (hl),a
wcsxfrm_ret:
        pop     de                      ; reported length
        pop     bc                      ; discard saved source
        pop     bc                      ; discard saved destination
        pop     ix
        ret
