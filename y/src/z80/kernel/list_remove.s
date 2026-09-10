        ; Remove a specified node from an intrusive list.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module list_remove
        .optsdcc -mz80 sdcccall(1)

        .globl  _list_remove

        .area   _CODE

        ; list_remove, sdcccall(1)
        ; inputs: HL = head address, DE = element
        ; outputs: DE = removed element or null
        ; clobbers: af, bc, de, hl
        ; The comparison is list_match_eq inlined. Each stack word is
        ; the address of the link that points at the current candidate.
        ; Keep list_find's limit of 256 unsuccessful comparisons.
_list_remove:
        ld      b, #0
.remove_loop:
        push    hl
        call    .next
        ld      a, h
        or      l
        jr      z, .remove_missing
        ld      a, l
        cp      e
        jr      nz, .remove_next
        ld      a, h
        cp      d
        jr      z, .remove_found
.remove_next:
        pop     af
        djnz    .remove_loop
        ld      de, #0
        ret
.remove_missing:
        pop     af
        ld      de, #0
        ret
.remove_found:
        pop     bc
        call    .next
        ld      a, l
        ld      (bc), a
        inc     bc
        ld      a, h
        ld      (bc), a
        ret

        ; Dereference an ordinary next link.
.next:
        ld      a, (hl)
        inc     hl
        ld      h, (hl)
        ld      l, a
        ret
