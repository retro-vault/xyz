        ; Test two list node pointers for equality.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module list_match_eq
        .optsdcc -mz80 sdcccall(1)

        .globl  _list_match_eq

        .area   _CODE

        ; list_match_eq, sdcccall(1)
        ; inputs: HL = node, DE = comparison value
        ; outputs: A = 1 if equal, otherwise 0
        ; clobbers: af, hl
_list_match_eq:
        xor     a
        sbc     hl, de
        ret     nz
        inc     a
        ret
