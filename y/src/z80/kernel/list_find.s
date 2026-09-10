        ; Find an intrusive list node through a callback.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module list_find
        .optsdcc -mz80 sdcccall(1)

        .globl  _list_find

        .area   _CODE

        ; list_find, sdcccall(1)
        ; inputs: HL = first, DE = previous-node output address
        ;         stack words = match callback, argument
        ; outputs: DE = matching node or null; removes both stack words
        ; clobbers: af, bc, de, hl; preserves ix and iy
        ; Callback receives HL = node, DE = argument and returns A.
        ; Frame: -2 saved iy, -4 previous output, -6 first, -7 guard;
        ;         +4 callback, +6 argument.
_list_find:
        push    ix
        ld      ix, #0
        add     ix, sp
        push    iy
        push    de
        push    hl
        xor     a
        push    af
        ld      (de), a
        inc     de
        ld      (de), a
.find_loop:
        ld      l, -6(ix)
        ld      h, -5(ix)
        ld      a, h
        or      l
        jr      z, .find_result
        ld      c, 4(ix)
        ld      b, 5(ix)
        ld      e, 6(ix)
        ld      d, 7(ix)
        call    .call_bc
        or      a
        jr      nz, .find_result
        ld      l, -4(ix)
        ld      h, -3(ix)
        ld      e, -6(ix)
        ld      d, -5(ix)
        ld      (hl), e
        inc     hl
        ld      (hl), d
        ex      de, hl
        call    .next
        ld      -6(ix), l
        ld      -5(ix), h
        inc     -7(ix)
        jr      nz, .find_loop
        ld      de, #0
        jr      .find_done
.find_result:
        ld      e, -6(ix)
        ld      d, -5(ix)
.find_done:
        ld      sp, ix
        dec     sp
        dec     sp
        pop     iy
        pop     ix
        pop     hl
        pop     bc
        pop     bc
        jp      (hl)

        ; Dereference an ordinary next link, optionally testing null.
        ; Preserve bc and de for traversal counters and return values.
.next_nonnull:
        ld      a, h
        or      l
        ret     z
.next:
        ld      a, (hl)
        inc     hl
        ld      h, (hl)
        ld      l, a
        ret

        ; Indirect callback; neither arguments nor return A are changed.
.call_bc:
        push    bc
        ret
