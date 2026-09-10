        ;; gpx_clrscr.s
        ;;
        ;; ZX Spectrum clear-screen primitive.
        ;; Clears pixel VRAM, restores default attributes and black border.
        ;;
        ;; GPL2 License (see: LICENSE)
        ;; Copyright (C) 2026 Tomaz Stih
        ;;
        ;; 2026-08-25   TS

        .module gpx_clrscr
        .optsdcc -mz80 sdcccall(1)

        .globl  _gpx_clrscr

        .equ    VMEMBEG, 0x4000
        .equ    ATTRBEG, 0x5800
        .equ    BDRPORT, 0xfe

        .area   _CODE

        ;; ------------------------------------------------------------
        ;; _gpx_clrscr
        ;; Clear the screen: blank the 6K pixel area, set every attribute
        ;; cell to black ink on white paper, and match the border to it.
        ;;
        ;; Signature:
        ;;   void gpx_clrscr(void)
        ;;
        ;; Clobbers:
        ;;   AF, BC, HL
        ;;
        ;; References:
        ;;   .cls_pages
_gpx_clrscr::
        ;; clear pixel area (0x4000..0x57ff): 24 whole pages
        ld      hl,#VMEMBEG
        ld      b,#0x18
        xor     a
        call    .cls_pages

        ;; attributes (0x5800..0x5aff): 3 whole pages.
        ;; paper = white (light gray on composite displays), ink = black
        ld      b,#0x03
        ld      a,#0x38
        call    .cls_pages

        ;; border = white (same tone family as paper)
        ld      a,#0x07
        out     (#BDRPORT),a
        ret

        ;; ------------------------------------------------------------
        ;; .cls_pages
        ;;   HL = start, page aligned   B = whole 256-byte pages   A = value
        ;;
        ;; An unrolled store runs at ~13 T-states a byte against LDIR's 21,
        ;; and unlike a stack-based clear it needs no `di`, so the caller's
        ;; interrupt latency is untouched. L wraps to its start after 256
        ;; increments, which is why `inc l` is enough inside a page.
        ;; ------------------------------------------------------------
.cls_pages:
        ld      c,#0x20                 ; 32 x 8 bytes = one page
.cls_row:
        ld      (hl),a
        inc     l
        ld      (hl),a
        inc     l
        ld      (hl),a
        inc     l
        ld      (hl),a
        inc     l
        ld      (hl),a
        inc     l
        ld      (hl),a
        inc     l
        ld      (hl),a
        inc     l
        ld      (hl),a
        inc     l
        dec     c
        jr      nz,.cls_row
        inc     h
        djnz    .cls_pages
        ret
