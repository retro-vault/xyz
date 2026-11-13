.ifndef YOS_RESIDENT_DESKTOP
.equ YOS_RESIDENT_DESKTOP,0
.endif
        ;; Low-level gateway for the standalone duplicated storage fixture.
        ;; The single-copy desktop retains only bank_select in common memory;
        ;; kernel-call.s and far.s supply its schedulable gates.
        ;; MIT License (see LICENSE), Copyright (C) 2026 Tomaz Stih.
        .module partner_bank
        .globl __partner_bank_init, __partner_bank_select
        .if YOS_RESIDENT_DESKTOP
        .else
        .globl __partner_bank_call, __partner_bank_gateway_end
        .endif
        .equ BANK_GATE,0xfe10
        .equ CURRENT_BANK,0xc002
        .equ CALL_DEPTH,0xc240
        .equ CALL_FRAMES,0xc241
        .equ CALL_LIMIT,32
        .area _CODE
__partner_bank_init::
        .if YOS_RESIDENT_DESKTOP
        ret
        .else
        ld hl,#.gateway
        ld de,#BANK_GATE
        ld bc,#.gateway_end-.gateway
        ldir
        xor a
        ld (CALL_DEPTH),a
        ret
        .endif
        ;; Persistent selection (shared in the desktop build). Preserve all
        ;; registers and IFF. FF means retain the current mapping.
__partner_bank_select::
        push af
        cp #0xff
        jr z,.return_af
        push bc
        ld b,a
        ld a,i
        di
        push af
        ld a,b
        cp #1
        jr z,.select1
        cp #2
        jr nz,.selected
        out (0x90),a
        jr .store
.select1:
        out (0x88),a
.store:
        ld (CURRENT_BANK),a
.selected:
        pop af
        jp po,.disabled
        ei
.disabled:
        pop bc
.return_af:
        pop af
        ret
        ;; A=1/2/FF, BC=target, HL/DE and ordinary stack args reach target.
        ;; Result main registers preserved, alternates scratch. Call frames
        ;; are outside the argument stack, so callee-clean APIs work too.
        ;; This low-level gate is synchronous; the scheduler must supply its
        ;; own per-thread call frames before allowing yields inside a call.
        .if YOS_RESIDENT_DESKTOP
        .else
__partner_bank_call::
        jp BANK_GATE
.gateway:
        exx
        ld e,a
        pop hl                         ; caller continuation
        push ix
        push hl
        ld a,(CALL_DEPTH)
        cp #CALL_LIMIT
        jr nc,.overflow
        ld c,a
        ld b,#0
        inc a
        ld (CALL_DEPTH),a
        ld ix,#CALL_FRAMES
        add ix,bc
        add ix,bc
        add ix,bc
        pop hl
        ld a,(CURRENT_BANK)
        ld 0(ix),a
        ld 1(ix),l
        ld 2(ix),h
        ld a,e
        call __partner_bank_select
        pop ix
        ld hl,#BANK_GATE+(.return-.gateway)
        push hl
        exx
        push bc
        ret
.overflow:
        pop hl
        pop ix
        push hl
        exx
        ld de,#0
        scf
        ret
.return:
        exx
        ex af,af'
        push ix
        ld a,(CALL_DEPTH)
        dec a
        ld (CALL_DEPTH),a
        ld c,a
        ld b,#0
        ld ix,#CALL_FRAMES
        add ix,bc
        add ix,bc
        add ix,bc
        ld a,0(ix)
        ld l,1(ix)
        ld h,2(ix)
        call __partner_bank_select
        pop ix
        push hl
        ex af,af'
        exx
        ret
.gateway_end:
__partner_bank_gateway_end::
        .endif
