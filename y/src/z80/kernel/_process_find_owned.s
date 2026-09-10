        ; Shared lookup for system objects owned by a process.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module _process_find_owned
        .optsdcc -mz80 sdcccall(1)

        .globl  __process_find_owned

        .area   _CODE

        ; Find the first system object owned by de.
        ; inputs: hl = first, de = owner
        ; outputs: de = matching object or zero
        ; clobbers: af, bc, hl
        ; A wrapping eight-bit guard permits 256 examined objects.
__process_find_owned::
        ld      c, #0
.find_loop:
        ld      a, h
        or      l
        jr      z, .find_missing
        push    hl
        inc     hl
        inc     hl
        ld      a, (hl)
        cp      e
        jr      nz, .find_next
        inc     hl
        ld      a, (hl)
        cp      d
        jr      z, .find_found
.find_next:
        pop     hl
        inc     c
        jr      z, .find_missing
        ld      a, (hl)
        inc     hl
        ld      h, (hl)
        ld      l, a
        jr      .find_loop
.find_found:
        pop     de
        ret
.find_missing:
        ld      de, #0
        ret
