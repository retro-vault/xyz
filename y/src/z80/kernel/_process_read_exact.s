        ; Read an exact byte count through the YOS POSIX descriptor layer.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module _process_read_exact
        .optsdcc -mz80 sdcccall(1)

        .globl  __process_read_exact
        .globl  _read

        .area   _CODE

        ; input: HL = fd, DE = buffer, BC = length.
        ; output: carry clear after an exact read; carry set on error/EOF.
        ; preserves IX and IY.
__process_read_exact::
        push    ix
        ld      ix,#0
        add     ix,sp
        push    hl                      ; IX-2: fd
        push    de                      ; IX-4: next byte
        push    bc                      ; IX-6: remaining
.loop:
        ld      c,-6(ix)
        ld      b,-5(ix)
        ld      a,b
        or      c
        jr      z,.ok
        push    bc
        ld      l,-2(ix)
        ld      h,-1(ix)
        ld      e,-4(ix)
        ld      d,-3(ix)
        call    _read
        pop     bc
        ld      a,d
        and     e
        inc     a
        jr      z,.bad
        ld      a,d
        or      e
        jr      z,.bad
        ld      l,-6(ix)
        ld      h,-5(ix)
        or      a
        sbc     hl,de
        jr      c,.bad
        ld      -6(ix),l
        ld      -5(ix),h
        ld      l,-4(ix)
        ld      h,-3(ix)
        add     hl,de
        jr      c,.bad
        ld      -4(ix),l
        ld      -3(ix),h
        jr      .loop
.ok:
        or      a
        jr      .return
.bad:
        scf
.return:
        ld      sp,ix
        pop     ix
        ret
