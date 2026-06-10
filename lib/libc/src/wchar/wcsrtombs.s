        ;; wcsrtombs.s
        ;;
        ;; Convert a wide string referenced through *src back into the target's
        ;; one-byte execution charset. The target only supports byte-range code
        ;; points here, so any wchar_t with a non-zero high byte raises EILSEQ.
        ;;
        ;; Standard behaviour implemented here:
        ;; - dst == NULL counts the required bytes without storing
        ;; - on consuming the terminating wide NUL, *src becomes NULL and the
        ;;   byte count excluding the terminator is returned
        ;; - when the output limit is reached first, *src is updated to the
        ;;   first unconverted wide character

        .module wcsrtombs
        .optsdcc -mz80 sdcccall(1)

        .globl  _wcsrtombs
        .globl  __errno_value

        .area   _CODE

_wcsrtombs::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      c,4(ix)                 ; BC = destination byte limit
        ld      b,5(ix)
        push    hl                      ; preserve the incoming dst pointer
        ld      hl,#-10
        add     hl,sp
        ld      sp,hl

        ;; locals:
        ;;   -4(ix)..-3(ix) current dst
        ;;   -6(ix)..-5(ix) current src
        ;;   -8(ix)..-7(ix) src double-pointer
        ;;   -10(ix)..-9(ix) original dst byte limit
        ;;   -12(ix)..-11(ix) converted count for dst == NULL mode
        ld      a,-2(ix)
        ld      -4(ix),a
        ld      a,-1(ix)
        ld      -3(ix),a
        ld      -8(ix),e
        ld      -7(ix),d
        ld      a,c
        ld      -10(ix),a
        ld      a,b
        ld      -9(ix),a
        xor     a
        ld      -12(ix),a
        ld      -11(ix),a

        ld      a,d
        or      e
        jr      nz,wcsrtombs_have_srcpp
        ld      de,#0x0000
        jp      wcsrtombs_ret
wcsrtombs_have_srcpp:
        ex      de,hl
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        ld      -6(ix),e
        ld      -5(ix),d
        ld      a,d
        or      e
        jr      nz,wcsrtombs_loop
        ld      de,#0x0000
        jp      wcsrtombs_ret

wcsrtombs_loop:
        ;; In store mode the count bound limits produced bytes.
        ld      a,-3(ix)
        or      -4(ix)
        jr      z,wcsrtombs_fetch
        ld      a,b
        or      c
        jr      nz,wcsrtombs_fetch
        jr      wcsrtombs_partial

wcsrtombs_fetch:
        ld      l,-6(ix)
        ld      h,-5(ix)
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        inc     hl                      ; HL now points past the current wchar_t
        ld      a,d
        or      a
        jr      nz,wcsrtombs_bad
        ld      a,e
        or      a
        jr      z,wcsrtombs_done

        ;; Store the narrow byte when a destination buffer was supplied.
        ld      -6(ix),l
        ld      -5(ix),h
        ld      a,-3(ix)
        or      -4(ix)
        jr      z,wcsrtombs_count_only
        ld      l,-4(ix)
        ld      h,-3(ix)
        ld      a,e
        ld      (hl),a
        inc     hl
        ld      -4(ix),l
        ld      -3(ix),h
        dec     bc
        jr      wcsrtombs_advance
wcsrtombs_count_only:
        ld      l,-12(ix)
        ld      h,-11(ix)
        inc     hl
        ld      -12(ix),l
        ld      -11(ix),h
wcsrtombs_advance:
        jr      wcsrtombs_loop

wcsrtombs_partial:
        ld      l,-8(ix)
        ld      h,-7(ix)
        ld      a,-6(ix)
        ld      (hl),a
        inc     hl
        ld      a,-5(ix)
        ld      (hl),a
        jr      wcsrtombs_count

wcsrtombs_done:
        ;; The current wide element was NUL. Terminate the byte stream when
        ;; the caller supplied a buffer with space remaining, then publish
        ;; *src = NULL.
        ld      a,-3(ix)
        or      -4(ix)
        jr      z,wcsrtombs_done_store_null
        ld      a,b
        or      c
        jr      z,wcsrtombs_done_store_null
        ld      l,-4(ix)
        ld      h,-3(ix)
        xor     a
        ld      (hl),a
wcsrtombs_done_store_null:
        ld      l,-8(ix)
        ld      h,-7(ix)
        xor     a
        ld      (hl),a
        inc     hl
        ld      (hl),a
        jr      wcsrtombs_count

wcsrtombs_bad:
        ;; Leave *src at the offending wide character and surface EILSEQ.
        dec     hl
        dec     hl
        ld      -6(ix),l
        ld      -5(ix),h
        ld      l,-8(ix)
        ld      h,-7(ix)
        ld      a,-6(ix)
        ld      (hl),a
        inc     hl
        ld      a,-5(ix)
        ld      (hl),a
        ld      hl,#84
        ld      (__errno_value),hl
        ld      de,#0xffff
        jr      wcsrtombs_ret

wcsrtombs_count:
        ld      a,-3(ix)
        or      -4(ix)
        jr      z,wcsrtombs_count_only_ret
        ld      l,-10(ix)
        ld      h,-9(ix)
        or      a
        sbc     hl,bc
        ex      de,hl
        jr      wcsrtombs_ret
wcsrtombs_count_only_ret:
        ld      e,-12(ix)
        ld      d,-11(ix)
wcsrtombs_ret:
        ld      sp,ix
        pop     ix
        ret
