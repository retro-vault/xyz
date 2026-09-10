        ; Validate an opaque YOS directory object and recover its handle.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module _directory_validate
        .optsdcc -mz80 sdcccall(1)

        .globl  __directory_validate

        .equ    DIRECTORY_MAGIC, 0xd1
        .equ    DIRECTORY_SIZE,  47

        .area   _CODE

        ; input: HL = DIR pointer.
        ; output: A = native handle and carry clear, or A = EBADF and carry set.
        ; preserves BC, DE, HL, IX and IY.
__directory_validate::
        ld      a,h
        cp      #0x40
        jr      c,.bad
        push    hl
        push    de
        ld      de,#DIRECTORY_SIZE-1
        add     hl,de
        pop     de
        jr      c,.bad_pop
        pop     hl
        push    hl
        inc     hl
        ld      a,(hl)
        cp      #DIRECTORY_MAGIC
        pop     hl
        jr      nz,.bad
        ld      a,(hl)
        or      a
        ret
.bad_pop:
        pop     hl
.bad:
        ld      a,#9                    ; EBADF
        scf
        ret
