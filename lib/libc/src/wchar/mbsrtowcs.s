        ;; mbsrtowcs.s
        ;;
        ;; Convert a narrow string referenced through *src into wchar_t
        ;; elements. The target's execution charset is stateless and
        ;; single-byte, so each input byte expands to one 16-bit code unit
        ;; with a zero high byte.
        ;;
        ;; Standard behaviour implemented here:
        ;; - dst == NULL counts the converted wchar_t elements without storing
        ;; - on seeing the terminating NUL, *src becomes NULL and the count of
        ;;   produced wide characters (excluding the terminator) is returned
        ;; - when the destination limit is reached first, *src is updated to
        ;;   the first unconverted narrow byte

        .module mbsrtowcs
        .optsdcc -mz80 sdcccall(1)

        .globl  _mbsrtowcs

        .area   _CODE

_mbsrtowcs::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      c,4(ix)                 ; BC = destination element limit
        ld      b,5(ix)
        push    hl                      ; preserve the incoming dst pointer
        ld      hl,#-10
        add     hl,sp
        ld      sp,hl

        ;; locals:
        ;;   -4(ix)..-3(ix) current dst
        ;;   -6(ix)..-5(ix) current src
        ;;   -8(ix)..-7(ix) src double-pointer
        ;;   -10(ix)..-9(ix) original dst element limit
        ;;   -12(ix)..-11(ix) converted count for dst == NULL mode
        ld      a,-2(ix)
        ld      -4(ix),a               ; current dst low
        ld      a,-1(ix)
        ld      -3(ix),a               ; current dst high
        ld      -8(ix),e
        ld      -7(ix),d
        ld      a,c
        ld      -10(ix),a
        ld      a,b
        ld      -9(ix),a
        xor     a
        ld      -12(ix),a
        ld      -11(ix),a

        ;; NULL src double-pointer behaves like an empty conversion.
        ld      a,d
        or      e
        jr      nz,mbsrtowcs_have_srcpp
        ld      de,#0x0000
        jp      mbsrtowcs_ret
mbsrtowcs_have_srcpp:
        ex      de,hl
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        ld      -6(ix),e
        ld      -5(ix),d
        ld      a,d
        or      e
        jr      nz,mbsrtowcs_loop
        ld      de,#0x0000
        jp      mbsrtowcs_ret

mbsrtowcs_loop:
        ;; In store mode the count bound limits produced wchar_t elements.
        ld      a,-3(ix)
        or      -4(ix)
        jr      z,mbsrtowcs_fetch
        ld      a,b
        or      c
        jr      nz,mbsrtowcs_fetch
        jr      mbsrtowcs_partial

mbsrtowcs_fetch:
        ld      l,-6(ix)
        ld      h,-5(ix)
        ld      a,(hl)
        or      a
        jr      z,mbsrtowcs_done

        ;; Store the widened code unit when a destination buffer was supplied.
        ld      d,h
        ld      e,l                      ; preserve the current src pointer
        push    af                       ; keep the narrow byte across tests
        ld      a,-3(ix)
        or      -4(ix)
        jr      z,mbsrtowcs_count_only
        ld      l,-4(ix)
        ld      h,-3(ix)
        pop     af
        ld      (hl),a
        inc     hl
        xor     a
        ld      (hl),a
        inc     hl
        ld      -4(ix),l
        ld      -3(ix),h
        dec     bc
        jr      mbsrtowcs_advance
mbsrtowcs_count_only:
        pop     af
        ld      l,-12(ix)
        ld      h,-11(ix)
        inc     hl
        ld      -12(ix),l
        ld      -11(ix),h
mbsrtowcs_advance:
        inc     de
        ld      -6(ix),e
        ld      -5(ix),d
        jr      mbsrtowcs_loop

mbsrtowcs_partial:
        ;; Destination filled before the source terminator. Publish the first
        ;; unconverted narrow-byte address back through *src.
        ld      l,-8(ix)
        ld      h,-7(ix)
        ld      a,-6(ix)
        ld      (hl),a
        inc     hl
        ld      a,-5(ix)
        ld      (hl),a
        jr      mbsrtowcs_count

mbsrtowcs_done:
        ;; Conversion consumed the terminating NUL. Emit a wide terminator when
        ;; the caller supplied a destination buffer with space remaining, then
        ;; publish *src = NULL.
        ld      a,-3(ix)
        or      -4(ix)
        jr      z,mbsrtowcs_done_store_null
        ld      a,b
        or      c
        jr      z,mbsrtowcs_done_store_null
        ld      l,-4(ix)
        ld      h,-3(ix)
        xor     a
        ld      (hl),a
        inc     hl
        ld      (hl),a
mbsrtowcs_done_store_null:
        ld      l,-8(ix)
        ld      h,-7(ix)
        xor     a
        ld      (hl),a
        inc     hl
        ld      (hl),a

mbsrtowcs_count:
        ld      a,-3(ix)
        or      -4(ix)
        jr      z,mbsrtowcs_count_only_ret
        ld      l,-10(ix)
        ld      h,-9(ix)
        or      a
        sbc     hl,bc
        ex      de,hl
        jr      mbsrtowcs_ret
mbsrtowcs_count_only_ret:
        ld      e,-12(ix)
        ld      d,-11(ix)
mbsrtowcs_ret:
        ld      sp,ix
        pop     ix
        ret
