        ; Insert a node at the head of an intrusive list.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module list_insert
        .optsdcc -mz80 sdcccall(1)

        .globl  _list_insert

        .area   _CODE

        ; list_insert, sdcccall(1)
        ; inputs: HL = head address, DE = element
        ; outputs: DE = element
        ; clobbers: bc, hl
_list_insert:
        ld      c, (hl)
        inc     hl
        ld      b, (hl)
        ex      de, hl
        ld      (hl), c
        inc     hl
        ld      (hl), b
        dec     hl
        ex      de, hl
        ld      (hl), d
        dec     hl
        ld      (hl), e
        ret
