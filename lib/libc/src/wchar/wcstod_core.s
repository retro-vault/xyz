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

        .area   _DATA
__wcsfp_nptr:      .ds 2
__wcsfp_endp:      .ds 2
__wcsfp_buf:       .ds 2
__wcsfp_char_end:  .ds 2
__wcsfp_len:       .ds 2
__wcsfp_result:    .ds 8

        .area   _CODE

        ;; __wcstod_core
        ;; inputs:
        ;;   HL = nptr
        ;;   DE = endptr (wchar_t **, may be NULL)
        ;; outputs:
        ;;   DE:HL:DE':HL' = parsed double
        ;;   *endptr updated to the first unparsed wchar_t when endptr != NULL
__wcstod_core::
        ld      (__wcsfp_nptr),hl
        ld      (__wcsfp_endp),de

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
        ld      (__wcsfp_len),a
        ld      a,b
        ld      (__wcsfp_len + 1),a
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
        jp      __db_zero

wcstod_have_buf:
        push    de
        pop     hl
        ld      (__wcsfp_buf),hl

        ;; Transcode the counted byte-range prefix into the temporary narrow
        ;; buffer and append a terminating NUL for strtod().
        ld      hl,(__wcsfp_nptr)
        ld      de,(__wcsfp_buf)
        ld      bc,(__wcsfp_len)
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
        ld      hl,(__wcsfp_buf)
        ld      de,#__wcsfp_char_end
        call    _strtod
        ld      (__wcsfp_result),de
        ld      (__wcsfp_result + 2),hl
        exx
        ld      (__wcsfp_result + 4),de
        ld      (__wcsfp_result + 6),hl
        exx

        ;; Map the returned byte end pointer back to the original wide source.
        ld      hl,(__wcsfp_char_end)
        ld      de,(__wcsfp_buf)
        or      a
        sbc     hl,de                   ; HL = consumed narrow bytes
        add     hl,hl                   ; widen byte count -> wchar_t bytes
        ld      de,(__wcsfp_nptr)
        add     hl,de                   ; HL = resulting wchar_t *
        call    wcstod_store_hl_end

        ;; Free the temporary buffer, then restore the parsed double result.
        ld      hl,(__wcsfp_buf)
        call    _free
        ld      de,(__wcsfp_result)
        ld      hl,(__wcsfp_result + 2)
        exx
        ld      de,(__wcsfp_result + 4)
        ld      hl,(__wcsfp_result + 6)
        exx
        ret

wcstod_store_nptr_end:
        ld      hl,(__wcsfp_nptr)
        ;; fall through

wcstod_store_hl_end:
        ld      de,(__wcsfp_endp)
        ld      a,d
        or      e
        ret     z
        ld      a,l
        ld      (de),a
        inc     de
        ld      a,h
        ld      (de),a
        ret
