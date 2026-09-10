        ; Remove the head node from an intrusive list.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module list_remove_first
        .optsdcc -mz80 sdcccall(1)

        .globl  _list_remove_first

        .area   _CODE

        ; list_remove_first, sdcccall(1)
        ; inputs: HL = head address
        ; outputs: DE = original first element, possibly null
        ; clobbers: af, bc, de, hl
_list_remove_first:
        ld      e, (hl)
        inc     hl
        ld      d, (hl)
        ld      a, d
        or      e
        ret     z
        push    de
        ex      de, hl
        ld      c, (hl)
        inc     hl
        ld      b, (hl)
        ld      a, b
        ld      (de), a
        dec     de
        ld      a, c
        ld      (de), a
        pop     de
        ret
