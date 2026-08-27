        ; CPC 464 console write hook; this target has no disk files.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module write
        .optsdcc -mz80 sdcccall(1)

        .globl  _write
        .globl  __cpc_putchar_a

        .area   _CODE

        ; _write
        ; inputs: HL = fd, DE = buffer, length at 4(ix)
        ; outputs: DE = bytes written, or -1
        ; clobbers: af, bc, de, hl, ix

_write::
        ld      a,h
        or      a
        jr      nz,.cpc_write_fail
        ld      a,l
        dec     a
        cp      #2
        jr      nc,.cpc_write_fail
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      c,4(ix)
        ld      b,5(ix)
        push    bc
        ex      de,hl
.cpc_write_loop:
        ld      a,b
        or      c
        jr      z,.cpc_write_done
        push    bc
        push    hl
        ld      a,(hl)
        call    __cpc_putchar_a
        pop     hl
        pop     bc
        inc     hl
        dec     bc
        jr      .cpc_write_loop
.cpc_write_done:
        pop     de
        pop     ix
        ret
.cpc_write_fail:
        ld      de,#0xffff
        ret
