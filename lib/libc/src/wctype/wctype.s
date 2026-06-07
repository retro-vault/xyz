        ; wctype.s
        ;
        ; libc wctype() for the xcc Z80 libc.  Maps a class name to its
        ; __WCTYPE_* id (1..12), or 0 for NULL/unknown.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module wctype
        .optsdcc -mz80 sdcccall(1)
        .globl  _wctype
        .globl  _strcmp
        .area   _CODE

        ; _wctype
        ; inputs:  HL = name
        ; outputs: DE = class id (1..12) or 0
_wctype::
        ld      a,h
        or      l
        jr      z,wct_none
        push    hl                      ; [name]
        ld      bc,#__wct_table
wct_loop:
        ld      a,(bc)
        ld      e,a
        inc     bc
        ld      a,(bc)
        ld      d,a                     ; DE = entry name pointer
        inc     bc
        ld      a,d
        or      e
        jr      z,wct_none_pop          ; sentinel -> not found
        pop     hl
        push    hl                      ; HL = name (kept on stack)
        push    bc
        call    _strcmp                 ; HL=name, DE=entry
        ld      a,d
        or      e
        pop     bc
        jr      z,wct_hit
        inc     bc                      ; skip the id, advance to next entry
        inc     bc
        jr      wct_loop
wct_hit:
        ld      a,(bc)
        ld      e,a
        inc     bc
        ld      a,(bc)
        ld      d,a                     ; DE = id
        pop     hl                      ; discard [name]
        ret
wct_none_pop:
        pop     hl
wct_none:
        ld      de,#0
        ret

__wct_n_alnum:  .asciz "alnum"
__wct_n_alpha:  .asciz "alpha"
__wct_n_blank:  .asciz "blank"
__wct_n_cntrl:  .asciz "cntrl"
__wct_n_digit:  .asciz "digit"
__wct_n_graph:  .asciz "graph"
__wct_n_lower:  .asciz "lower"
__wct_n_print:  .asciz "print"
__wct_n_punct:  .asciz "punct"
__wct_n_space:  .asciz "space"
__wct_n_upper:  .asciz "upper"
__wct_n_xdigit: .asciz "xdigit"
__wct_table:
        .dw __wct_n_alnum,  1
        .dw __wct_n_alpha,  2
        .dw __wct_n_blank,  3
        .dw __wct_n_cntrl,  4
        .dw __wct_n_digit,  5
        .dw __wct_n_graph,  6
        .dw __wct_n_lower,  7
        .dw __wct_n_print,  8
        .dw __wct_n_punct,  9
        .dw __wct_n_space,  10
        .dw __wct_n_upper,  11
        .dw __wct_n_xdigit, 12
        .dw 0, 0
