        ; Iterate an intrusive list with cycle guards.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module list_iterate
        .optsdcc -mz80 sdcccall(1)

        .globl  _list_iterate

        .area   _CODE

        ; list_iterate, sdcccall(1)
        ; inputs: HL = first, DE = callback, stack word = argument
        ; outputs: none; removes the argument word
        ; clobbers: af, bc, de, hl; preserves ix and iy
        ; Callback receives HL = node and DE = argument. Persistent
        ; traversal state survives arbitrary caller-saved clobbers.
        ; Frame: -2 saved iy, -4 callback, -6 first, -8 slow,
        ;         -10 fast, -11 guard, +4 argument.
        ; The callback may change any next link.
_list_iterate:
        push    ix
        ld      ix, #0
        add     ix, sp
        push    iy
        push    de
        push    hl
        push    hl
        push    hl
        xor     a
        push    af
.iterate_loop:
        ld      l, -6(ix)
        ld      h, -5(ix)
        ld      a, h
        or      l
        jr      z, .iterate_done
        ld      c, -4(ix)
        ld      b, -3(ix)
        ld      e, 4(ix)
        ld      d, 5(ix)
        call    .call_bc

        ; Read first->next after the callback, before updating Floyd's
        ; cursors. A wrapping guard exits before either cursor is read.
        ld      l, -6(ix)
        ld      h, -5(ix)
        call    .next
        ld      -6(ix), l
        ld      -5(ix), h
        inc     -11(ix)
        jr      z, .iterate_done
        ld      l, -8(ix)
        ld      h, -7(ix)
        call    .next_nonnull
        ld      -8(ix), l
        ld      -7(ix), h
        ld      l, -10(ix)
        ld      h, -9(ix)
        call    .next_nonnull
        call    .next_nonnull
        ld      -10(ix), l
        ld      -9(ix), h
        ld      a, h
        or      l
        jr      z, .iterate_loop
        ld      e, -8(ix)
        ld      d, -7(ix)
        ld      a, d
        or      e
        jr      z, .iterate_loop
        or      a
        sbc     hl, de
        jr      nz, .iterate_loop
.iterate_done:
        ld      sp, ix
        dec     sp
        dec     sp
        pop     iy
        pop     ix
        pop     hl
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
