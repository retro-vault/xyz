        ; Allocate count * size bytes and clear the exact requested span.
        ; Checked shift/add multiplication needs no runtime arithmetic.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module calloc
        .optsdcc -mz80 sdcccall(1)
        .area   _CODE
        .globl  _calloc
        .globl  _malloc

        ; _calloc (sdcccall(1))
        ; inputs: HL = element count, DE = element size
        ; outputs: DE = zero-filled allocation, or zero on failure
        ; clobbers: AF, BC, DE, HL; preserves IX and IY
_calloc::
        ld      a,h
        or      l
        jp      z,_malloc
        ld      a,d
        or      e
        jr      nz,calloc_nonzero
        ld      hl,#0
        jp      _malloc
calloc_nonzero:
        ld      b,h
        ld      c,l
        ld      hl,#0
calloc_product:
        srl     b
        rr      c
        jr      nc,calloc_skip_add
        add     hl,de
        jr      c,calloc_overflow
calloc_skip_add:
        ld      a,b
        or      c
        jr      z,calloc_product_ready
        ; Only double while a nonzero multiplier remains.  A carry
        ; then proves that a required partial product cannot fit.
        sla     e
        rl      d
        jr      nc,calloc_product
calloc_overflow:
        ld      de,#0
        ret

calloc_product_ready:
        push    hl
        call    _malloc
        pop     bc
        ld      a,d
        or      e
        ret     z

        ; The saved byte count survives every permitted malloc clobber.
        ; Seed one zero and propagate it, avoiding BC=0's 65536 copies.
        push    de
        ex      de,hl
        ld      (hl),#0
        dec     bc
        ld      a,b
        or      c
        jr      z,calloc_zero_done
        ld      e,l
        ld      d,h
        inc     de
        ldir
calloc_zero_done:
        pop     de
        ret
