        ; Release the tail of a kernel-heap block for the public YOS table.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module _yos_shrink
        .optsdcc -mz80 sdcccall(1)

        .globl  __yos_shrink
        .globl  __mem_split
        .globl  __yos_free
        .globl  _enter_critical_section
        .globl  _leave_critical_section

        .area   _CODE

        ; __yos_shrink, internal service-table adapter
        ; inputs: hl = payload of a live allocation, de = bytes to keep
        ; outputs: de = payload, or zero when the header is not allocated
        ; clobbers: af, bc, de, hl; preserves ix and iy
        ; The block never moves. The bytes past DE are split off and freed
        ; when they can form a heap block; otherwise, or when DE is not
        ; smaller than the block, the payload keeps its current size.
__yos_shrink::
        push    ix
        push    hl                      ; payload, returned on success
        ld      bc, #-7
        add     hl, bc
        push    hl
        pop     ix                      ; block header
        call    _enter_critical_section
        bit     0, 4(ix)
        jr      z, .failed              ; free or foreign header
        call    __mem_split             ; carry: nothing to release
        call    nc, __yos_free          ; coalesce the released tail
        pop     de
.leave:
        pop     ix
        jp      _leave_critical_section
.failed:
        pop     hl
        ld      de, #0
        jr      .leave
