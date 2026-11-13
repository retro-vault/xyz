        .module _video_next_l2
        .optsdcc -mz80 sdcccall(1)
        .include "_zxnext-l2-defs.inc"
        .globl __nxt_map_page
        .globl __nxt_unmap
        .globl __nxt_offset
        .globl __vid_rowaddr
        .globl __vid_nextrow
        .globl __vid_prevrow
        .area _CODE

;; Select the 16 KiB Layer 2 window.  The control write enables the ROM
;; overlay read/write paths; the second write selects one of the five banks.
__nxt_map_page:
        and #7
        ld d,a
        ld bc,#NXT_L2_PORT
        ld a,#NXT_L2_MAP
        out (c),a
        ld a,d
        or #NXT_L2_OFFSET
        out (c),a
        ret

__nxt_unmap:
        ld bc,#NXT_L2_PORT
        ld a,#NXT_L2_IDLE
        out (c),a
        ret

;; Set the active 16 KiB offset (A=0..4), leaving the read/write overlay on.
__nxt_offset:
        and #7
        or #NXT_L2_OFFSET
        ld d,a
        ld bc,#NXT_L2_PORT
        ld a,d
        out (c),a
        ret

;; B=y. Return the logical byte offset of the row in DE (y*320).
;; This helper is intentionally linear; callers map the bank before use.
__vid_rowaddr:
        ld a,b
        ld l,a
        ld h,#0
        ld d,h
        ld e,a
        add hl,hl                 ; *2
        add hl,hl                 ; *4
        add hl,hl                 ; *8
        add hl,hl                 ; *16
        add hl,hl                 ; *32
        add hl,hl                 ; *64
        ld a,h
        ld h,e
        ld l,#0                   ; y*256
        add hl,de                 ; y*256 + y*64
        ret

__vid_nextrow:
        ld de,#NXT_WIDTH
        add hl,de
        ret
__vid_prevrow:
        ld de,#NXT_WIDTH
        or a
        sbc hl,de
        ret
