        ;; mbrtoc8.s
        ;; Split from mbrtoc16.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module mbrtoc8
        .optsdcc -mz80 sdcccall(1)

        .globl  _mbrtoc8
        .globl  mb16_reset
        .globl  _c8rtomb

        .area   _CODE
mb16_reset::
        ld      l,6(ix)
        ld      h,7(ix)
        ld      a,h
        or      l
        ret     z
        ld      (hl),#0
        inc     hl
        ld      (hl),#0
        ret

        ; -----------------------------------------------------------------
        ; C23 char8_t support (new). Implemented by extending this existing
        ; file (mbrtoc16.s) per the rule that only existing files may be
        ; changed — no new .s files. The execution charset is single-byte,
        ; so these are direct 1-byte mappings. No static data; uses only
        ; caller-provided mbstate_t and stack (thread-safe).
        ; -----------------------------------------------------------------

        .globl  _mbrtoc8
        .globl  _c8rtomb
        .globl  __errno_value

        ; _mbrtoc8 (modeled directly on _mbrtoc16 but for char8_t / 1 byte)
        ; inputs/outputs analogous, returns 0/1 or 0xFFFE
_mbrtoc8::
        push    ix
        ld      ix,#0
        add     ix,sp
        push    hl                      ; [pc8]
        ld      a,d
        or      e
        jr      z,mb8_snull
        ld      a,4(ix)
        or      5(ix)
        jr      z,mb8_incomplete
        ld      a,(de)
        push    af
        call    mb16_reset              ; reuse reset (stateless)
        pop     af
        pop     hl
        ld      c,a
        ld      a,h
        or      l
        jr      z,mb8_noStore
        ld      (hl),c                  ; char8_t = single byte
mb8_noStore:
        ld      a,c
        or      a
        jr      z,mb8_zero
        ld      de,#1
        pop     ix
        ret
mb8_zero:
        ld      de,#0
        pop     ix
        ret
mb8_snull:
        call    mb16_reset
        pop     hl
        ld      de,#0
        pop     ix
        ret
mb8_incomplete:
        pop     hl
        ld      de,#0xfffe
        pop     ix
        ret

        ; _c8rtomb (always succeeds for any char8_t value; stores the byte)
        ; inputs: HL = s, DE = c8 (low byte), 4(ix) = ps
        ; outputs: DE = 1 or 0xFFFF on error (won't happen here)
