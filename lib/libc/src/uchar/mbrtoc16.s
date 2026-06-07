        ; mbrtoc16.s
        ;
        ; libc mbrtoc16() for the xcc Z80 libc.  The execution character set is
        ; a stateless single-byte encoding, so each byte maps directly to one
        ; UTF-16 code unit.
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
