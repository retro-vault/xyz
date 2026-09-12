        ; Read an exact count through the YOS POSIX descriptor layer.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module _process_read_exact
        .optsdcc -mz80 sdcccall(1)
        .globl  __process_read_exact
        .globl  _read
        .area   _CODE

        ; inputs: hl = fd, de = buffer, bc = length
        ; outputs: carry clear on exact read, set on error/EOF
        ; clobbers: af, bc, de, hl; preserves ix and iy
        ; IX holds fd; stack saves buffer and remaining byte count.
__process_read_exact::
        push    ix
        push    hl
        pop     ix
.loop:
        ld      a, b
        or      c
        jr      z, .done
        push    de
        push    bc
        push    ix
        pop     hl
        call    _read
        pop     hl
        pop     bc
        bit     7, d
        jr      nz, .bad
        ld      a, d
        or      e
        jr      z, .bad
        or      a
        sbc     hl, de
        jr      c, .bad
        push    hl
        ld      h, b
        ld      l, c
        add     hl, de
        ex      de, hl
        pop     bc
        jr      nc, .loop
.bad:
        scf
.done:
        pop     ix
        ret
