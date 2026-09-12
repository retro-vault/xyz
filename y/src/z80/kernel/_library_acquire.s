        ; Attach one owned reference to a resident library.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module _library_acquire
        .optsdcc -mz80 sdcccall(1)
        .globl  __library_acquire
        .globl  __library_refs
        .globl  _so_create
        .equ    LIBRARY_SERVICE,   5
        .equ    LIBRARY_REFS,     13
        .equ    REFERENCE_LIBRARY, 4
        .equ    SERVICE_INTERFACE, 20
        .area   _CODE

        ; inputs: hl = library, de = client; critical section held
        ; outputs: de = interface or zero, a = 2 on allocation failure
        ; clobbers: af, bc, de, hl; preserves ix and iy
        ; The 1 KiB system heap bounds the count far below 65535.
__library_acquire::
        push    ix
        push    hl
        pop     ix
        push    de
        ld      hl, #__library_refs
        ld      de, #6
        call    _so_create
        ld      a, d
        or      e
        ld      a, #2
        jr      z, .done
        ld      hl, #REFERENCE_LIBRARY
        add     hl, de
        push    ix
        pop     de
        ld      (hl), e
        inc     hl
        ld      (hl), d
        inc     LIBRARY_REFS(ix)
        jr      nz, .table
        inc     LIBRARY_REFS+1(ix)
.table:
        ld      l, LIBRARY_SERVICE(ix)
        ld      h, LIBRARY_SERVICE+1(ix)
        ld      de, #SERVICE_INTERFACE
        add     hl, de
        ld      e, (hl)
        inc     hl
        ld      d, (hl)
.done:
        pop     ix
        ret
