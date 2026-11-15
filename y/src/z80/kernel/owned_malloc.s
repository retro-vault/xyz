        ; Allocate near data with an explicit process/library owner. Retained
        ; service data must outlive a calling process, so it uses the library's
        ; descriptor rather than the caller's ordinary allocation ownership.
        .module owned_malloc
        .optsdcc -mz80 sdcccall(1)
        .globl _yos_owned_malloc, _yos_owned_common_malloc
        .globl __sys_heap, __bank_current, _mem_allocate
        .globl __critical_call
        .area _ROM_ARGS
        ; HL=size, DE=live owner -> HL=address, E=bank, D=zero.
        ; Uses the existing allocator and owner-based reclamation unchanged.
_yos_owned_malloc::
        .if YOS_UNIFIED_48K
        ;; Classic 48K has no switchable arena. Its physical bank marker is
        ;; still zero, but all owned allocations belong to the unified heap.
        .globl __bank_count
        ld a,(__bank_count)
        or a
        ld a,#0xff
        jr z,.owned
        .endif
        ld a,(__bank_current)
        jr .owned
_yos_owned_common_malloc::
        ld a,#0xff
.owned:
        call __critical_call
        push af                        ; remember the allocation's address space
        push de
        ex de,hl
        ld hl,#__sys_heap
        inc a
        jr z,.allocate
        ld hl,#YOS_BANK_BASE
.allocate:
        call _mem_allocate
        pop bc
        ex de,hl
        ld e,b
        ld d,#0
        ret
