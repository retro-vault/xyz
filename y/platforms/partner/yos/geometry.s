        .ifndef YOS_CPC_AMSDOS
        .equ YOS_CPC_AMSDOS,0
        .endif
        ;; CP/M media geometry. All scratch is in common RAM, serialized by YOS.
        ;; MIT License (see LICENSE), Copyright (C) 2026 Tomaz Stih.
        .module partner_geometry
        .include "cpmfs.inc"
        .globl __cpmfs_geometry, __cpmfs_geometry_iy
        .globl __cpmfs_block_valid, __cpmfs_extent_base
        .globl __cpmfs_record_count
        .area _CODE
__cpmfs_geometry_iy::
        push af
        ld a,FILE_DRIVE(iy)
        call __cpmfs_geometry
        pop af
        ret
        ;; A=device. Preserves all registers, including flags.
__cpmfs_geometry::
        push af
        push hl
        .if YOS_CPC_AMSDOS
        ld hl,#0
        .else
        cp #4
        jr nc,.hd
        ld hl,#36
        .endif
        ld (CPMFS_G_DIR),hl
        .if YOS_CPC_AMSDOS
        ld hl,#8
        .else
        ld hl,#52
        .endif
        ld (CPMFS_G_END),hl
        .if YOS_CPC_AMSDOS
        ld hl,#180
        .else
        ld hl,#342
        .endif
        ld (CPMFS_G_BLOCKS),hl
        .if YOS_CPC_AMSDOS
        ld a,#2
        .else
        ld a,#3
        .endif
        ld (CPMFS_G_SHIFT),a
        .if YOS_CPC_AMSDOS
        .else
        ld a,#2
        .endif
        ld (CPMFS_G_RESERVED),a
        xor a
        .if YOS_CPC_AMSDOS
        .else
        jr .exm
.hd:
        ld hl,#32
        ld (CPMFS_G_DIR),hl
        ld hl,#160
        ld (CPMFS_G_END),hl
        ld hl,#2446                    ; standard Partner 10 MB image
        ld (CPMFS_G_BLOCKS),hl
        ld a,#4
        ld (CPMFS_G_SHIFT),a
        ld a,#8
        ld (CPMFS_G_RESERVED),a
        ld a,#1
        .endif
.exm:
        ld (CPMFS_G_EXM),a
        pop hl
        pop af
        ret
        ;; DE=allocation block; CF when outside the medium. Preserve HL/DE.
__cpmfs_block_valid::
        push hl
        ld hl,(CPMFS_G_BLOCKS)
        or a
        sbc hl,de
        jr z,.invalid
        jr c,.invalid
        or a
        pop hl
        ret
.invalid:
        scf
        pop hl
        ret
        ;; Round logical DE extent down to its physical entry's first extent.
__cpmfs_extent_base::
        ld a,(CPMFS_G_EXM)
        cpl
        and e
        ld e,a
        ret
        ;; DE=records left in this physical extent; HL=directory entry.
        ;; Clamp to 128 (FD) / 256 (HD), then encode EX low bit and RC.
        ;; BC and HL preserved. Caller supplies extent-aligned EX/S2.
__cpmfs_record_count::
        push bc
        push hl
        ld a,(CPMFS_G_EXM)
        or a
        jr z,.fd_count
        ld bc,#12
        add hl,bc
        res 0,(hl)
        ld a,d
        or a
        jr nz,.hd_full
        ld a,e
        cp #129
        jr c,.store
        set 0,(hl)
        sub #128
        jr .store
.hd_full:
        set 0,(hl)
        ld a,#128
        jr .store
.fd_count:
        ld bc,#12
        add hl,bc
        ld a,d
        or a
        jr nz,.fd_full
        ld a,e
        cp #129
        jr c,.store
.fd_full:
        ld a,#128
.store:
        inc hl
        inc hl
        inc hl
        ld (hl),a
        pop hl
        pop bc
        ret
