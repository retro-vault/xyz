        ; Firmware switches the 2000 window to its own SRAM. Bounce reads
        ; through 128 fixed bytes while the existing filesystem lock is held.
        ; A=handle, HL=banked destination, BC=count. Return native CF/error or
        ; BC=actual length; preserve IX/IY. No application stack buffer needed.
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih
        ;
        .module divmmc_read
        .optsdcc -mz80 sdcccall(1)
        .globl __divmmc_read, __zx_esx_f_read
        .if YOS_DIVMMC
        .area _DIVMMC_STATE
        .else
        .area _BSS
        .endif
.buffer: .ds 128
        .area _CODE
__divmmc_read::
        push ix
        ld ix,#0
        add ix,sp
        push af                        ; -2 handle
        push hl                        ; -4 destination
        push bc                        ; -6 remaining
        ld hl,#0
        push hl                        ; -8 total
        push hl                        ; -10 requested chunk
.loop:
        ld c,-6(ix)
        ld b,-5(ix)
        ld a,b
        or c
        jr z,.success
        ld a,b
        or a
        jr nz,.full
        ld a,c
        cp #129
        jr c,.size
.full:
        ld bc,#128
.size:
        ld -10(ix),c
        ld -9(ix),b
        ld a,-1(ix)
        ld hl,#.buffer
        call __zx_esx_f_read
        jr c,.return
        ld a,b
        or c
        jr z,.success
        ld l,-8(ix)
        ld h,-7(ix)
        add hl,bc
        ld -8(ix),l
        ld -7(ix),h
        ld l,-6(ix)
        ld h,-5(ix)
        or a
        sbc hl,bc
        ld -6(ix),l
        ld -5(ix),h
        push bc
        ld e,-4(ix)
        ld d,-3(ix)
        ld hl,#.buffer
        ldir
        ld -4(ix),e
        ld -3(ix),d
        pop bc
        ld l,-10(ix)
        ld h,-9(ix)
        or a
        sbc hl,bc
        jr z,.loop
.success:
        ld c,-8(ix)
        ld b,-7(ix)
        or a                           ; EOF/short read is not an error
.return:
        ld sp,ix
        pop ix
        ret
