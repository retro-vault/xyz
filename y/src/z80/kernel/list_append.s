        ; Append a node to an intrusive list.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module list_append
        .optsdcc -mz80 sdcccall(1)

        .globl  _list_append

        .area   _CODE

        ; list_append, sdcccall(1)
        ; inputs: HL = head address, DE = element
        ; outputs: DE = element, or null after 256 traversed links
        ; clobbers: af, bc, de, hl
        ; Clear element->next before reading the head, including when
        ; the element is already reachable from that head.
_list_append:
        xor     a
        ld      (de), a
        inc     de
        ld      (de), a
        dec     de
        ld      c, (hl)
        inc     hl
        ld      b, (hl)
        ld      a, b
        or      c
        jr      z, .append_tail
        ld      h, b
        ld      l, c
        ld      b, #0
.append_loop:
        ld      c, (hl)
        inc     hl
        ld      a, (hl)
        or      c
        jr      z, .append_tail
        ld      h, (hl)
        ld      l, c
        djnz    .append_loop
        ld      de, #0
        ret
.append_tail:
        ld      (hl), d
        dec     hl
        ld      (hl), e
        ret
