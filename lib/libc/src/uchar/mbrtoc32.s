        ; mbrtoc32.s
        ;
        ; libc mbrtoc32() for the xcc Z80 libc.  One execution byte maps
        ; directly to one UTF-32 code unit.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module mbrtoc32
        .optsdcc -mz80 sdcccall(1)
        .globl  _mbrtoc32
        .area   _CODE

        ; _mbrtoc32
        ; inputs:  HL = pc32, DE = s, 4(ix) = n, 6(ix) = ps
        ; outputs: DE = 0, 1, or 0xFFFE (n == 0)
_mbrtoc32::
        push    ix
        ld      ix,#0
        add     ix,sp
        push    hl                      ; [pc32]
        ld      a,d
        or      e
        jr      z,mb32_snull
        ld      a,4(ix)
        or      5(ix)
        jr      z,mb32_incomplete
        ld      a,(de)
        push    af
        call    mb32_reset
        pop     af
        pop     hl                      ; [pc32]
        ld      c,a
        ld      a,h
        or      l
        jr      z,mb32_noStore
        ld      (hl),c
        inc     hl
        ld      (hl),#0
        inc     hl
        ld      (hl),#0
        inc     hl
        ld      (hl),#0                 ; char32_t upper bytes
mb32_noStore:
        ld      a,c
        or      a
        jr      z,mb32_zero
        ld      de,#1
        pop     ix
        ret
mb32_zero:
        ld      de,#0
        pop     ix
        ret
mb32_snull:
        call    mb32_reset
        pop     hl
        ld      de,#0
        pop     ix
        ret
mb32_incomplete:
        pop     hl
        ld      de,#0xfffe
        pop     ix
        ret
mb32_reset:
        ld      l,6(ix)
        ld      h,7(ix)
        ld      a,h
        or      l
        ret     z
        ld      (hl),#0
        inc     hl
        ld      (hl),#0
        ret
