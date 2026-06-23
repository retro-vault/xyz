        ; wcscmp.s — compare two wide strings by wchar_t value (-1/0/1).
        ; wchar_t is 16-bit, so each element is compared high byte first.
        ; MIT License (see: LICENSE)  Copyright (C) 2026 tomaz stih
        .module wcscmp
        .optsdcc -mz80 sdcccall(1)
        .globl  _wcscmp
        .area   _CODE
        ;; _wcscmp
        ;; Compare wchar_t values numerically. High bytes decide first so the
        ;; signed 16-bit ordering matches ordinary integer comparison.
_wcscmp::
wcc_loop:
        ld      a,(hl)                  ; lhs low -> C
        ld      c,a
        inc     hl
        ld      a,(hl)                  ; lhs high -> B
        dec     hl
        ld      b,a
        ld      a,(de)                  ; rhs low
        push    af
        inc     de
        ld      a,(de)                  ; rhs high
        dec     de
        cp      b                       ; rhs_high vs lhs_high
        jr      c,wcc_gt_pop            ; rhs_high < lhs_high -> lhs > rhs
        jr      nz,wcc_lt_pop
        pop     af                      ; rhs low
        cp      c                       ; rhs_low vs lhs_low
        jr      c,wcc_gt
        jr      nz,wcc_lt
        ld      a,b                     ; Equal element: stop only on the wide NUL.
        or      c
        jr      z,wcc_equal
        inc     hl
        inc     hl
        inc     de
        inc     de
        jr      wcc_loop
wcc_gt_pop:
        pop     af
wcc_gt:
        ld      de,#1
        ret
wcc_lt_pop:
        pop     af
wcc_lt:
        ld      de,#0xffff
        ret
wcc_equal:
        ld      de,#0
        ret
