        ; CPC 464 console read hook; this target has no disk files.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module read
        .optsdcc -mz80 sdcccall(1)

        .globl  _read
        .globl  _getchar

        .area   _CODE

        ; _read
        ; inputs: HL = fd, DE = buffer, length at 4(ix)
        ; outputs: DE = bytes read, or -1
        ; clobbers: af, bc, de, hl, ix

_read::
        ld      a,h
        or      l
        jr      nz,.cpc_read_fail
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      c,4(ix)
        ld      b,5(ix)
        push    bc
        ex      de,hl
.cpc_read_loop:
        ld      a,b
        or      c
        jr      z,.cpc_read_done
        push    bc
        push    hl
        call    _getchar
        ld      a,e
        pop     hl
        pop     bc
        ld      (hl),a
        inc     hl
        dec     bc
        jr      .cpc_read_loop
.cpc_read_done:
        pop     de
        pop     ix
        ret
.cpc_read_fail:
        ld      de,#0xffff
        ret
