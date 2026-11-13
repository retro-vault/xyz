        ;; AMSDOS bootstrap. Firmware is used only to load the two RAM images.
        ;; No firmware workspace is retained after entering the desktop.
        .module cpc_desktop_boot
        .area _CODE
        .globl _boot
_boot::
        ld bc,#0x7fc1
        out (c),c
        rst 0x18
        .dw .close_far                   ; close RUN's input
        ld hl,#.common
        ld b,#10
        call .open
        jr nc,.error
        ld hl,#0xc050
        rst 0x18
        .dw .direct_far                   ; CAS IN DIRECT
        jr nc,.error
        rst 0x18
        .dw .close_far
        ld hl,#.kernel
        ld b,#10
        call .open
        jr nc,.error
        ld hl,#0x0100
        rst 0x18
        .dw .direct_far
        jr nc,.error
        rst 0x18
        .dw .close_far
        di
        ld bc,#0x7f8d
        out (c),c
        ld hl,#.decompress
        ld de,#0xff00
        ld bc,#.decompress_end-.decompress
        ldir
        ld sp,#0xfe00
        jp 0xff00
.decompress:
        ld hl,#PACKED_LAST
        ld de,#KERNEL_LAST
.unpack:
        ld a,(hl)
        dec hl
        bit 7,a
        jr nz,.match
        inc a
        ld b,a
.literal:
        ld a,(hl)
        dec hl
        ld (de),a
        dec de
        djnz .literal
        jr .next
.match:
        and #127
        add a,#3
        push af
        ld c,(hl)
        dec hl
        ld b,(hl)
        dec hl
        push hl
        ld h,d
        ld l,e
        add hl,bc
        pop bc                         ; compressed input pointer
        pop af
        push bc
        ld c,a
        ld b,#0
        lddr
        pop hl
.next:
        ld a,h
        or a
        jr nz,.unpack
        jp 0x4000
.decompress_end:
.open:
        ld de,#0x8000                 ; AMSDOS 2K input buffer
        rst 0x18
        .dw .open_far
        ret
.error:
        ld a,#69
        call 0xbb5a
        jr .error
.common: .ascii "COMMON.BIN"
.kernel: .ascii "KERNEL.BIN"

.in_open:
        ld iy,(0xbe7d)
        jp 0xcd4c
.in_close:
        ld iy,(0xbe7d)
        jp 0xcd4f
.in_direct:
        ld iy,(0xbe7d)
        jp 0xcd58
.open_far: .dw .in_open
        .db 7
.close_far: .dw .in_close
        .db 7
.direct_far: .dw .in_direct
        .db 7
