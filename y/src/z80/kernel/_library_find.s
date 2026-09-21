        ; Find a ready shared library by full name and image ABI.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module _library_find
        .optsdcc -mz80 sdcccall(1)
        .globl  __library_find
        .globl  _process_first
        .globl  __string_compare
        .equ    LIBRARY_FLAGS,   4
        .equ    LIBRARY_SERVICE, 5
        .equ    LIBRARY_ABI,     7
        .area   _CODE

        ; inputs: ix = loader frame; caller holds critical section
        ; outputs: de = library or zero
        ; clobbers: af, bc, de, hl; preserves ix and iy
__library_find::
        push    iy
        ld      iy, (_process_first)
.loop:
        push    iy
        pop     de
        ld      a, d
        or      e
        jr      z, .done
        ld      a, LIBRARY_FLAGS(iy)
        cp      #3
        jr      nz, .next
        ld      a, LIBRARY_ABI(iy)
        cp      6(ix)
        jr      nz, .next
        ld      l, LIBRARY_SERVICE(iy)
        ld      h, LIBRARY_SERVICE+1(iy)
        ld      de, #4
        add     hl, de
        ex      de, hl
        push    ix
        pop     hl
        ld      bc, #40
        add     hl, bc
        call    __string_compare
        ld      a, d
        or      e
        jr      nz, .next
        push    iy
        pop     de
.done:
        pop     iy
        ret
.next:
        ld      l, 0(iy)
        ld      h, 1(iy)
        push    hl
        pop     iy
        jr      .loop
