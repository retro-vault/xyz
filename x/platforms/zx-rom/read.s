        .module read
        .optsdcc -mz80 sdcccall(1)
        .globl  _read
        .globl  _getchar
        .area   _CODE
_read::
        ld      a,h
        or      l
        jr      nz,.zx_read_fail
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      c,4(ix)
        ld      b,5(ix)
        push    bc
        ex      de,hl
.zx_read_loop:
        ld      a,b
        or      c
        jr      z,.zx_read_done
        push    bc
        push    hl
        call    _getchar
        ld      a,e
        pop     hl
        pop     bc
        ld      (hl),a
        inc     hl
        dec     bc
        jr      .zx_read_loop
.zx_read_done:
        pop     de
        pop     ix
        ret
.zx_read_fail:
        ld      de,#0xffff
        ret
