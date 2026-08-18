        .module write
        .optsdcc -mz80 sdcccall(1)
        .globl  _write
        .globl  _zx_console_putc_a
        .area   _CODE
_write::
        ld      a,h
        or      a
        jr      nz,.zx_write_fail
        ld      a,l
        cp      #1
        jr      z,.zx_write_console
        cp      #2
        jr      nz,.zx_write_fail
.zx_write_console:
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      c,4(ix)
        ld      b,5(ix)
        push    bc
        ex      de,hl
.zx_write_loop:
        ld      a,b
        or      c
        jr      z,.zx_write_done
        push    bc
        push    hl
        ld      a,(hl)
        call    _zx_console_putc_a
        pop     hl
        pop     bc
        inc     hl
        dec     bc
        jr      .zx_write_loop
.zx_write_done:
        pop     de
        pop     ix
        ret
.zx_write_fail:
        ld      de,#0xffff
        ret
