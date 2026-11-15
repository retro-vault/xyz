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
        jp nc,.error
        ld hl,#0xc050
        rst 0x18
        .dw .direct_far                   ; CAS IN DIRECT
        jp nc,.error
        rst 0x18
        .dw .close_far
        ld hl,#.kernel
        ld b,#10
        call .open
        jp nc,.error
        ld hl,#0x0100
        rst 0x18
        .dw .direct_far
        jp nc,.error
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
        ;; Stage the packed common image away from its output before expanding.
        ;; Kernel input ends below 8000, and both streams reuse one decoder.
        ld hl,#0xc050
        ld de,#0x8000
        ld bc,#COMMON_PACKED_SIZE
        ldir
        ld hl,#0x8000+COMMON_PACKED_SIZE-1
        ld de,#COMMON_LAST
        ld ix,#0xc04f
        call 0xff00+.unpack-.decompress
        ld hl,#PACKED_LAST
        ld de,#KERNEL_LAST
        ld ix,#0x00ff
        call 0xff00+.unpack-.decompress
        jp 0x4000
.unpack:
        ld a,(hl)                       ; eight-token flag byte
        dec hl
        ld iyl,a
        ld iyh,#8
.token:
        ld a,iyl
        srl a
        ld iyl,a
        jr c,.match
        ld a,(hl)
        dec hl
        ld (de),a
        dec de
        jr .next
.match:
        ld c,(hl)
        dec hl
        ld b,(hl)
        dec hl
        ld a,b
        and #0xf0
        rrca
        rrca
        rrca
        rrca
        add a,#3                        ; match length 3..18
        push hl                         ; compressed input cursor
        push af
        ld a,b
        and #0x0f
        ld b,a
        inc bc                          ; encoded distance minus one
        ld h,d
        ld l,e
        add hl,bc
        pop af
        ld c,a
        ld b,#0
        lddr
        pop hl
.next:
        ld a,d
        cp ixh
        jr nz,.more
        ld a,e
        cp ixl
        ret z
.more:
        ld a,iyh
        dec a
        ld iyh,a
        jr nz,.token
        jr .unpack
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
