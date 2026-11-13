        ;; Track process/library allocations separately from resident UI state.
        ;; MIT License (see LICENSE), Copyright (C) 2026 Tomaz Stih.
        .module partner_memory
        .optsdcc -mz80 sdcccall(1)
        .globl __native_alloc, __native_free, __native_memory_reap
        .globl _malloc, _free
        .area _BSS
.allocations: .ds 512                ; 128 {pointer, owner}
        .area _CODE
        ;; HL=size, DE=owner -> DE=pointer. Preserves IX/IY.
__native_alloc::
        push ix
        push hl
        push de
        ld ix,#.allocations
        ld b,#128
.find:
        ld a,0(ix)
        or 1(ix)
        jr z,.slot
        ld de,#4
        add ix,de
        djnz .find
        pop de
        pop hl
        pop ix
        ld de,#0
        ret
.slot:
        pop de
        ld 2(ix),e
        ld 3(ix),d
        pop hl
        call _malloc
        ld 0(ix),e
        ld 1(ix),d
        pop ix
        ret
__native_free::
        push ix
        ld ix,#.allocations
        ld b,#128
.free_next:
        ld a,0(ix)
        cp l
        jr nz,.free_skip
        ld a,1(ix)
        cp h
        jr z,.free_found
.free_skip:
        ld de,#4
        add ix,de
        djnz .free_next
        jr .free_done
.free_found:
        ld 0(ix),#0
        ld 1(ix),#0
.free_done:
        pop ix
        jp _free
        ;; IX=owner; all live child allocations are returned to the arena.
__native_memory_reap::
        push ix
        push iy
        push ix
        pop iy
        ld ix,#.allocations
        ld b,#128
.reap_next:
        push bc
        ld a,0(ix)
        or 1(ix)
        jr z,.reap_skip
        push iy
        pop de
        ld a,2(ix)
        cp e
        jr nz,.reap_skip
        ld a,3(ix)
        cp d
        jr nz,.reap_skip
        ld l,0(ix)
        ld h,1(ix)
        ld 0(ix),#0
        ld 1(ix),#0
        call _free
.reap_skip:
        ld de,#4
        add ix,de
        pop bc
        djnz .reap_next
        pop iy
        pop ix
        ret
