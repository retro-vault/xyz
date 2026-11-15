        ;; CP/M user-area namespace. Public paths use configured folder names;
        ;; numeric aliases always select the same physical user areas.
        ;; MIT License (see LICENSE), Copyright (C) 2026 Tomaz Stih.
        .module partner_yos_path
        .globl __partner_path, __cpm_directory_config, __cpm_path
        .globl __cpm_resolve, __cpm_alias_set
        .area _BSS
__cpm_path:: .ds 32
        .area _CODE
        ;; HL=caller path -> HL=common normalized path, A=0/FF.
        ;; Copy before switching banks; retain BC/DE/IX/IY. No retained input.
__partner_path::
        push ix
        push bc
        push de
        ld ix,#0
        add ix,sp
        ex de,hl
        ld hl,#-128
        add hl,sp
        ld sp,hl
        push hl
        ex de,hl
        ld b,#128
.copy_path:
        ld a,(hl)
        inc hl
        ld (de),a
        inc de
        or a
        jr z,.resolve
        djnz .copy_path
        pop hl
        ld a,#255
        jr .restore
.resolve:
        pop hl
        call __cpm_resolve
.restore:
        ld sp,ix
        pop de
        pop bc
        pop ix
        ld hl,#__cpm_path
        ret
        ;; Named service setter: HL=name, E=area; A=0 or FF.
        ;; Common snapshot permits names residing in a banked application.
__cpm_directory_config::
        push ix
        push de
        ld ix,#0
        add ix,sp
        ex de,hl
        ld hl,#-13
        add hl,sp
        ld sp,hl
        push hl
        ex de,hl
        ld b,#13
.copy_alias:
        ld a,(hl)
        inc hl
        ld (de),a
        inc de
        or a
        jr z,.set_alias
        djnz .copy_alias
        pop hl
        ld a,#255
        jr .config_done
.set_alias:
        pop hl
        ld e,0(ix)
        call __cpm_alias_set
.config_done:
        ld sp,ix
        pop de
        pop ix
        ret
