        ; Commit image ownership and disarm loader rollback.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module _image_transfer
        .optsdcc -mz80 sdcccall(1)
        .globl  __image_transfer
        .area   _CODE

        ; inputs: ix = loader frame, de = new image owner
        ; outputs: HL = resident image, frame image pointer cleared
        ; clobbers: af, bc, hl; preserves de, ix and iy
        ; Caller holds a critical section until the new owner is ready.
__image_transfer::
        ld      l, 66(ix)
        ld      h, 67(ix)
        push    hl                      ; return resident pointer to caller
        ld      bc, #-5
        add     hl, bc
        ld      (hl), e
        inc     hl
        ld      (hl), d
        xor     a
        ld      66(ix), a
        ld      67(ix), a
        pop     hl
        ret
