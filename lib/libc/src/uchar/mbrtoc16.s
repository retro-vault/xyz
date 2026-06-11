        ; mbrtoc16.s
        ;
        ; libc mbrtoc16() for the xcc Z80 libc.  The execution character set is
        ; a stateless single-byte encoding, so each byte maps directly to one
        ; UTF-16 code unit.
        ;
        ; Also contains the C23 char8_t functions (mbrtoc8/c8rtomb) — added
        ; to this existing file only (no new .s created).
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module mbrtoc16
        .optsdcc -mz80 sdcccall(1)
        .globl  _mbrtoc16
        .area   _CODE

        ; _mbrtoc16
        ; inputs:  HL = pc16, DE = s, 4(ix) = n, 6(ix) = ps
        ; outputs: DE = 0 (NUL or s==NULL), 1 (one byte), or 0xFFFE (n==0)
_mbrtoc16::
        push    ix
        ld      ix,#0
        add     ix,sp
        push    hl                      ; [pc16]
        ld      a,d
        or      e
        jr      z,mb16_snull            ; s == NULL
        ld      a,4(ix)
        or      5(ix)
        jr      z,mb16_incomplete       ; n == 0
        ld      a,(de)                  ; byte
        push    af
        call    mb16_reset
        pop     af
        pop     hl                      ; [pc16]
        ld      c,a                     ; C = byte
        ld      a,h
        or      l
        jr      z,mb16_noStore          ; pc16 == NULL
        ld      (hl),c
        inc     hl
        ld      (hl),#0                 ; char16_t high byte
mb16_noStore:
        ld      a,c
        or      a
        jr      z,mb16_zero
        ld      de,#1
        pop     ix
        ret
mb16_zero:
        ld      de,#0
        pop     ix
        ret
mb16_snull:
        call    mb16_reset
        pop     hl
        ld      de,#0
        pop     ix
        ret
mb16_incomplete:
        pop     hl
        ld      de,#0xfffe
        pop     ix
        ret
        ; mb16_reset: zero *ps (mbstate_t, 2 bytes) when ps (6(ix)) != NULL
mb16_reset:
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
_c8rtomb::
        push    ix
        ld      ix,#0
        add     ix,sp
        push    hl                      ; [s]
        push    de                      ; [c8]
        ld      l,4(ix)
        ld      h,5(ix)
        ld      a,h
        or      l
        jr      z,c8_nr
        ld      (hl),#0
        inc     hl
        ld      (hl),#0
c8_nr:
        pop     de                      ; [c8] — E = value, D should be 0 but ignore for char8
        pop     hl                      ; [s]
        ld      a,h
        or      l
        jr      z,c8_ret1
        ld      (hl),e
c8_ret1:
        ld      de,#1
        pop     ix
        ret
