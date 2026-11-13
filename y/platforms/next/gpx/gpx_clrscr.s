        .module gpx_clrscr_next_l2
        .optsdcc -mz80 sdcccall(1)
        .include "_zxnext-l2-defs.inc"
        .globl _gpx_clrscr
        .globl __nxt_map_page
        .globl __nxt_unmap
        .area _CODE
_gpx_clrscr::
        ld c,#0
.page:
        ld a,c
        call __nxt_map_page
        ld hl,#0
        ld de,#0x4000
        ld a,#0xff
.fill:
        ld (hl),a
        inc hl
        dec de
        ld a,d
        or e
        jr nz,.fill
        inc c
        ld a,c
        cp #NXT_PAGES
        jr c,.page
        jp __nxt_unmap
