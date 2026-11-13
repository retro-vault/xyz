        ;; 48K kernel (C1), 48K applications (C2), 16K common RAM (bank 7).
        .module cpc_bank
        .globl __partner_bank_init, __partner_bank_select
        .area _FIXED_CODE
__partner_bank_init::
        ret
__partner_bank_select::
        push af
        cp #255
        jr z,.done
        push bc
        ld (0xc002),a
        or #0xc0
        ld bc,#0x7f00
        out (c),a
        pop bc
.done:
        pop af
        ret
        ;; GPX executes in bank 0 while C3 maps physical video RAM at 4000.
        ;; The renderer is synchronous and does not call application code.
        .globl __cpc_video_call
        .area _BSS
.video_return: .ds 2
        .area _FIXED_CODE
__cpc_video_call::
        ex af,af'
        exx
        pop hl
        ld e,(hl)
        inc hl
        ld d,(hl)
        pop hl
        ld (.video_return),hl
        ld bc,#0x7fc3
        out (c),c
        ld hl,#.video_done
        push hl
        push de
        exx
        ex af,af'
        ret
.video_done:
        ex af,af'
        exx
        ld bc,#0x7fc1
        out (c),c
        ld hl,(.video_return)
        push hl
        exx
        ex af,af'
        ret
