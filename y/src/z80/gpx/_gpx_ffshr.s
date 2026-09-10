        ;; _gpx_ffshr.s
        ;;
        ;; Shared byte-span edge masks. A separate archive member lets
        ;; sprite restoration use this helper without the bitmap blitter.
        ;;
        ;; GPL2 License (see: LICENSE)
        ;; Copyright (C) 2026 Tomaz Stih
        ;;
        ;; 2026-09-06   TS

        .module _gpx_ffshr
        .optsdcc -mz80 sdcccall(1)

        .globl  __gpx_ffshr

        .area   _CODE

        ;; ------------------------------------------------------------
        ;; __gpx_ffshr
        ;; Right-edge mask for a byte span: 0xFF shifted right by A. A
        ;; table costs eight bytes and beats the shift loop it replaces at
        ;; every shift count.
        ;;
        ;; Arguments:
        ;;   A = shift count, 0..7
        ;;
        ;; Return:
        ;;   A = 0xFF >> count (a count of 0 gives 0xFF)
        ;;
        ;; Clobbers:
        ;;   AF, B
__gpx_ffshr::
        push    hl
        ld      hl,#.gb_ffshr_tab
        add     a,l
        ld      l,a
        jr      nc,.gb_ffshr_hi
        inc     h
.gb_ffshr_hi:
        ld      a,(hl)
        pop     hl
        ret

.gb_ffshr_tab:
        .db     0xff,0x7f,0x3f,0x1f,0x0f,0x07,0x03,0x01
