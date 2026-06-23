        ;; wcstod_core.s
        ;;
        ;; Shared wide-string to floating-point parser for wcstof/wcstod/wcstold.
        ;;
        ;; The target's execution charset is single-byte, so the wide wrappers
        ;; can transcode the byte-range prefix of the source wchar_t string into
        ;; a temporary narrow buffer, reuse the existing strtod core, and then
        ;; map the returned narrow end pointer back to the original wide source.
        ;;
        ;; Any wchar_t with a non-zero high byte terminates the transcode the
        ;; same way a non-byte character would terminate a narrow parse.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module wcstod_core
        .optsdcc -mz80 sdcccall(1)

        .globl  __wcstod_core
        .globl  _malloc
        .globl  _free
        .globl  _strtod
        .globl  __db_zero

        .area   _CODE

        ;; __wcstod_core
        ;; inputs:
        ;;   HL = nptr
        ;;   DE = endptr (wchar_t **, may be NULL)
        ;; outputs:
        ;;   DE:HL:DE':HL' = parsed double
        ;;   *endptr updated to the first unparsed wchar_t when endptr != NULL
__wcstod_core::
        ld      b,h
        ld      c,l
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      hl,#-18
        add     hl,sp
        ld      sp,hl
        ld      -18(ix),c
        ld      -17(ix),b
        ld      -16(ix),e
        ld      -15(ix),d

        ;; Count the contiguous byte-range wchar_t prefix. The parser only
        ;; needs a narrow mirror up to the first non-byte code unit or NUL.
        ld      bc,#0
wcstod_count:
        ld      a,(hl)
        ld      e,a
        inc     hl
        ld      a,(hl)
        dec     hl
        or      a
        jr      nz,wcstod_alloc
        ld      a,e
        or      a
        jr      z,wcstod_alloc
        inc     bc
        inc     hl
        inc     hl
        jr      wcstod_count

wcstod_alloc:
        ld      a,c
        ld      -10(ix),a
        ld      a,b
        ld      -9(ix),a
        ld      h,b
        ld      l,c
        inc     hl                      ; include terminating narrow NUL
        call    _malloc
        ld      a,d
        or      e
        jr      nz,wcstod_have_buf

        ;; Out-of-memory: surface an empty parse result and leave endptr at
        ;; the original source pointer.
        call    wcstod_store_nptr_end
        call    __db_zero
        jp      wcstod_ret

wcstod_have_buf:
        push    de
        pop     hl
        ld      -14(ix),l
        ld      -13(ix),h

        ;; Transcode the counted byte-range prefix into the temporary narrow
        ;; buffer and append a terminating NUL for strtod().
        ld      l,-18(ix)
        ld      h,-17(ix)
        ld      e,-14(ix)
        ld      d,-13(ix)
        ld      c,-10(ix)
        ld      b,-9(ix)
        ld      a,b
        or      c
        jr      z,wcstod_term
wcstod_copy:
        ld      a,(hl)
        ld      (de),a
        inc     de
        inc     hl
        inc     hl
        dec     bc
        ld      a,b
        or      c
        jr      nz,wcstod_copy
wcstod_term:
        xor     a
        ld      (de),a

        ;; Run the proven narrow parser and preserve its double result across
        ;; the temporary buffer teardown.
        ld      l,-14(ix)
        ld      h,-13(ix)
        push    ix
        pop     de
        ld      bc,#-12
        add     ix,bc
        push    ix
        pop     de
        ld      bc,#12
        add     ix,bc
        call    _strtod
        ld      -8(ix),e
        ld      -7(ix),d
        ld      -6(ix),l
        ld      -5(ix),h
        exx
        ld      -4(ix),e
        ld      -3(ix),d
        ld      -2(ix),l
        ld      -1(ix),h
        exx

        ;; Map the returned byte end pointer back to the original wide source.
        ld      l,-12(ix)
        ld      h,-11(ix)
        ld      e,-14(ix)
        ld      d,-13(ix)
        or      a
        sbc     hl,de                   ; HL = consumed narrow bytes
        add     hl,hl                   ; widen byte count -> wchar_t bytes
        ld      e,-18(ix)
        ld      d,-17(ix)
        add     hl,de                   ; HL = resulting wchar_t *
        call    wcstod_store_hl_end

        ;; Free the temporary buffer, then restore the parsed double result.
        ld      l,-14(ix)
        ld      h,-13(ix)
        call    _free
        ld      e,-8(ix)
        ld      d,-7(ix)
        ld      l,-6(ix)
        ld      h,-5(ix)
        exx
        ld      e,-4(ix)
        ld      d,-3(ix)
        ld      l,-2(ix)
        ld      h,-1(ix)
        exx
        jr      wcstod_ret

wcstod_store_nptr_end:
        ld      l,-18(ix)
        ld      h,-17(ix)
        ;; fall through

wcstod_store_hl_end:
        ld      e,-16(ix)
        ld      d,-15(ix)
        ld      a,d
        or      e
        ret     z
        ld      a,l
        ld      (de),a
        inc     de
        ld      a,h
        ld      (de),a
        ret
wcstod_ret:
        ld      sp,ix
        pop     ix
        ret
