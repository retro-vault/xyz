        ; Attach one owned reference to a resident library.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module _library_acquire
        .optsdcc -mz80 sdcccall(1)
        .globl  __library_acquire
        .globl  __library_refs
        .globl  _so_create
        .equ    LIBRARY_SERVICE,   6
        .equ    LIBRARY_REFS,     14
        .equ    REFERENCE_LIBRARY, 5
        .equ    SERVICE_INTERFACE, 21
        .area   _CODE

        ; inputs: hl = library, de = client; critical section held
        ; outputs: de = interface or zero, a = 2 on allocation failure
        ; clobbers: af, bc, de, hl; preserves ix and iy
        ; The 1 KiB system heap bounds the count far below 65535.
__library_acquire::
        push    ix
        push    hl
        pop     ix
        ; One owned reference per client/library: repeated load_library does
        ; not consume more OS heap. There is no public individual release API.
        push    de
        ld      hl, (__library_refs)
.find:
        ld      a, h
        or      l
        jr      z, .new_reference
        push    hl
        inc     hl
        inc     hl
        inc     hl                      ; owner address, after packed bank
        ld      a, (hl)
        cp      e
        jr      nz, .next_reference
        inc     hl
        ld      a, (hl)
        cp      d
        jr      nz, .next_reference
        inc     hl
        ld      a, (hl)
        cp      ixl
        jr      nz, .next_reference
        inc     hl
        ld      a, (hl)
        cp      ixh
        jr      nz, .next_reference
        pop     hl
        pop     de
        jr      .table
.next_reference:
        pop     hl
        ld      a, (hl)
        inc     hl
        ld      h, (hl)
        ld      l, a
        jr      .find
.new_reference:
        ; The client owner is already on the stack for so_create.
        ld      hl, #__library_refs
        ld      de, #7
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
