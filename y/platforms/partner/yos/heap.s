        ;; Near UI objects live in common RAM; executable images in either
        ;; private heap. The baremetal allocator supplies both policies.
        .module partner_yos_heap
        .globl _malloc, _free, __alloc_zero, __heap_init, __heap_first
        .globl _smalloc, _sfree, _memory_init
        .globl __partner_image_alloc, __partner_process_image_alloc, __partner_image_free
        .globl _balloc
        .globl __partner_far_malloc, __partner_far_free
        .globl __partner_bank_select, __partner_image_bank
        .equ __heap_first,0xc018
        .area _BSS
__partner_image_bank:: .ds 1
        .area _CODE
__heap_init::
        jp _memory_init
_malloc::
        jp _smalloc
_free::
        jp _sfree
__alloc_zero::
        push hl
        call _smalloc
        pop bc
        ld a,d
        or e
        ret z
        push de
        ex de,hl
        xor a
.zero:
        ld (hl),a
        inc hl
        dec bc
        ld a,b
        or c
        jr z,.zero_done
        xor a
        jr .zero
.zero_done:
        pop de
        ret
        ;; HL=size -> DE=image and logical image bank; keep the kernel mapped.
__partner_image_alloc::
        call __partner_far_malloc
.image_result:
        ld a,h
        or l
        jr z,.image_none
        ld a,e
        dec a                          ; YOS banks are zero-based, hardware 1/2
        ld (__partner_image_bank),a
        ex de,hl
        ret
.image_none:
        ld de,#0
        ret
        ;; HL=image bytes -> DE=image, logical bank in __partner_image_bank.
        ;; Prefer an arena with 4352 bytes of startup space for external fonts and
        ;; near data. Probe/free/reallocate is atomic in this cooperative loader;
        ;; the extra space is never retained. If neither bank has that room,
        ;; fall back to the ordinary packed allocation policy.
__partner_process_image_alloc::
        push hl
        ld bc,#4352
        add hl,bc
        jr c,.process_fallback
        call __partner_image_alloc
        ld a,d
        or e
        jr z,.process_fallback
        ld a,(__partner_image_bank)
        push af
        ex de,hl
        call __partner_image_free
        pop af
        inc a                          ; balloc uses physical bank 1/2
        pop hl
        push af
        inc sp
        call _balloc
        inc sp
        jr .image_result
.process_fallback:
        pop hl
        jp __partner_image_alloc

        ;; A=bank, HL=allocation. The far allocator restores the current bank.
__partner_image_free::
        inc a                          ; logical 0/1 -> physical 1/2
        push af
        inc sp
        push hl
        call __partner_far_free
        ret

        .globl __partner_image_get, __partner_image_put
        .globl __partner_image_in, __partner_image_out
        .globl __far_getb, __far_putb
        ;; Relocation arithmetic relies on carry across successive bytes.
__partner_image_get::
        push bc
        push af
        ld a,(__partner_image_bank)
        inc a
        ld c,a
        call __far_getb
        pop bc
        ld b,a
        push bc
        pop af
        pop bc
        ret
__partner_image_put::
        push bc
        push af
        ld c,a
        ld a,(__partner_image_bank)
        inc a
        ld b,a
        ld a,c
        ld c,b
        call __far_putb
        pop af
        pop bc
        ret
__partner_image_in::
        ld a,b
        or c
        ret z
        ld a,(hl)
        ex de,hl
        call __partner_image_put
        ex de,hl
        inc hl
        inc de
        dec bc
        jr __partner_image_in
__partner_image_out::
        ld a,b
        or c
        ret z
        call __partner_image_get
        ld (de),a
        inc hl
        inc de
        dec bc
        jr __partner_image_out
