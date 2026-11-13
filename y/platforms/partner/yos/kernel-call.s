        .ifndef YOS_CPC_AMSDOS
        .equ YOS_CPC_AMSDOS,0
        .endif
        ;; Single-copy resident kernel: five-byte shared entry veneers.
        ;; The inline word names code in hardware bank 1. Preserve all input
        ;; registers, including IX/IY, and leave packed stack arguments in place.
        .module partner_kernel_call
        .globl __partner_kernel_call, __bank_call_iy
        .area _FIXED_CODE
__partner_kernel_call::
        ex af,af'
        exx
        pop hl                         ; inline target word
        ld c,(hl)
        inc hl
        ld b,(hl)
        pop de                         ; actual API caller continuation
        ld a,(0xc002)
        cp #1
        jr nz,.map_kernel
        push de
        push bc
        exx
        ex af,af'
        ret
.map_kernel:
        push iy
        pop hl                         ; saved by the bank-call frame
        push bc
        pop iy
        xor a                          ; public bank 0 = resident bank
        jp __bank_call_iy

        ;; Far bytes preserve registers and IFF and never expose a transient
        ;; mapping to an ISR. Common addresses require no bank transition.
        .globl __far_getb, __far_putb, __partner_bank_select
__far_getb::
        .if YOS_CPC_AMSDOS
        push af
        ld a,h
        cp #0xc0
        jr nc,.cpc_getb_direct
        ld a,c
        or a
        jr z,.cpc_getb_direct
        ld a,(0xc002)
        cp c
        jr nz,.cpc_getb_mapped
.cpc_getb_direct:
        pop af
        ld a,(hl)
        ret
.cpc_getb_mapped:
        pop af

        .else
        .endif
        push bc
        push de
        ld b,#0
        jr .byte
__far_putb::
        .if YOS_CPC_AMSDOS
        push af
        ld a,h
        cp #0xc0
        jr nc,.cpc_putb_direct
        ld a,c
        or a
        jr z,.cpc_putb_direct
        ld a,(0xc002)
        cp c
        jr nz,.cpc_putb_mapped
.cpc_putb_direct:
        pop af
        ld (hl),a
        ret
.cpc_putb_mapped:
        pop af

        .else
        .endif
        push bc
        push de
        ld d,a
        ld b,#1
.byte:
        ld a,i
        push af
        di
        ld a,(0xc002)
        push af
        ld a,c
        or a
        jr z,.mapped
        call __partner_bank_select
.mapped:
        ld a,b
        or a
        jr nz,.put
        ld d,(hl)
        jr .restore
.put:
        ld (hl),d
.restore:
        pop af
        call __partner_bank_select
        pop af
        jp po,.disabled
        ei
.disabled:
        ld a,d
        pop de
        pop bc
        ret
