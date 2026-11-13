        .module next_video
        .globl __next_video_init, _gpx_clrscr, _next_hw_init
        .globl _enter_critical_section, _leave_critical_section
        .area _GPX
__next_video_init::
        ld d,#0x12
        ld e,#8
        call .reg
        ld d,#0x70
        ld e,#0x10              ; 320x256x8, column-major
        call .reg
        ld d,#0x43
        ld e,#0x10
        call .reg
        ld d,#0x40
        ld e,#0
        call .reg
        ld d,#0x41
        ld e,#0
        call .reg
        ld d,#0x40
        ld e,#255
        call .reg
        ld d,#0x41
        ld e,#255
        call .reg
        ld d,#0x68
        ld e,#0x80
        call .reg
        ;; Sprites: black = index 0, white = index 255, transparent = E3.
        ld d,#0x43
        ld e,#0x20
        call .reg
        ld d,#0x40
        ld e,#0
        call .reg
        ld d,#0x41
        call .reg
        ld d,#0x40
        ld e,#255
        call .reg
        ld d,#0x41
        call .reg
        ld d,#0x43
        ld e,#0x10
        call .reg
        ld d,#0x4b
        ld e,#0xe3
        call .reg
        ld d,#0x15
        ld e,#3                 ; sprites visible over the border, SLU order
        call .reg
        call _next_hw_init
        ;; Reset Layer 2 clipping and scrolling left by the launcher.
        ld d,#0x1c
        ld e,#3
        call .reg
        ld d,#0x19              ; sprite clip window: whole display
        ld e,#0
        call .reg
        ld e,#255
        call .reg
        ld e,#0
        call .reg
        ld e,#255
        call .reg
        ld d,#0x18
        ld e,#0
        call .reg
        ld e,#255               ; the x clip units cover x=639 too
        call .reg
        ld e,#0
        call .reg
        ld e,#255
        call .reg
        ld d,#0x16
        ld e,#0
        call .reg
        ld d,#0x17
        call .reg
        ld d,#0x71
        call .reg
        jp _gpx_clrscr
.reg:
        ld bc,#0x243b
        out (c),d
        inc b
        out (c),e
        ret
_gpx_clrscr::
        call _enter_critical_section
        ld a,#0
.page:
        push af
        ld e,a
        ld bc,#0x123b
        ld a,#7
        out (c),a
        ld a,e
        or #0x10
        out (c),a
        ;; The whole 16K page is white. PUSH writes two pixels in 11
        ;; T-states; restore the real stack before paging or making a call.
        ld (.restore_stack+1),sp
        ld de,#0xffff
        ld sp,#0x4000
        ld c,#4
.clear_quarter:
        ld b,#0                   ; 256 * 16 pixels = one quarter-page
.clear_words:
        push de
        push de
        push de
        push de
        push de
        push de
        push de
        push de
        djnz .clear_words
        dec c
        jr nz,.clear_quarter
.restore_stack:
        ld sp,#0                  ; patched while the real stack is mapped
        pop af
        inc a
        cp #5
        jr c,.page
        ld bc,#0x123b
        ld a,#2
        out (c),a
        jp _leave_critical_section
