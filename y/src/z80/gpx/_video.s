        ;; _video.s
        ;;
        ;; ZX Spectrum VRAM row helpers shared by line/pixel routines.
        ;;
        ;; GPL2 License (see: LICENSE)
        ;; Copyright (C) 2026 Tomaz Stih
        ;;
        ;; 2026-08-25   TS

        .module _video
        .optsdcc -mz80 sdcccall(1)

        .globl  __vid_rowaddr
        .globl  __vid_nextrow
        .globl  __vid_prevrow
        .globl  __vid_nextrow_carry
        .globl  __vid_prevrow_carry

        .area   _CODE

        ;; ------------------------------------------------------------
        ;; __vid_rowaddr
        ;; Base address of a pixel row in ZX display memory. The Spectrum
        ;; interleaves rows, so this is a bit shuffle rather than a
        ;; multiply.
        ;;
        ;; Arguments:
        ;;   B = y (0..191)
        ;;
        ;; Return:
        ;;   HL = row base in pixel VRAM (0x4000..0x57E0)
        ;;
        ;; Clobbers:
        ;;   AF, HL
__vid_rowaddr::
        ld      a,b
        and     #0x07
        or      #0x40
        ld      h,a

        ld      a,b
        rrca
        rrca
        rrca
        and     #0x18
        or      h
        ld      h,a

        ld      a,b
        rla
        rla
        and     #0xE0
        ld      l,a
        ret

        ;; ------------------------------------------------------------
        ;; __vid_nextrow
        ;; Step a VRAM pointer down one pixel row. Works on pointers that
        ;; already carry an x offset, because bits 0..4 of L are untouched.
        ;;
        ;; Arguments:
        ;;   HL = current row base, or a row pointer with an x offset
        ;;
        ;; Return:
        ;;   HL = the same pointer one pixel row further down
        ;;
        ;; Clobbers:
        ;;   AF
__vid_nextrow::
        inc     h
        ld      a,h
        and     #0x07
        ret     nz                      ; seven rows in eight end here

        ;; ------------------------------------------------------------
        ;; __vid_nextrow_carry
        ;; The character-cell crossing half of __vid_nextrow. Hot loops
        ;; inline the common three instructions and enter here with
        ;; `call z`, paying a call only on the one row in eight that
        ;; leaves the current cell.
        ;;
        ;; Arguments:
        ;;   HL = row pointer whose low three row bits have just wrapped
        ;;
        ;; Return:
        ;;   HL = pointer to the first row of the next character cell
        ;;
        ;; Clobbers:
        ;;   AF
__vid_nextrow_carry::
        ld      a,l
        add     a,#0x20
        ld      l,a
        ret     c

        ld      a,h
        sub     #0x08
        ld      h,a
        ret

        ;; ------------------------------------------------------------
        ;; __vid_prevrow
        ;; Step a VRAM pointer up one pixel row; the exact inverse of
        ;; __vid_nextrow, and equally safe on x-offset pointers.
        ;;
        ;; Arguments:
        ;;   HL = current row base, or a row pointer with an x offset
        ;;
        ;; Return:
        ;;   HL = the same pointer one pixel row further up
        ;;
        ;; Clobbers:
        ;;   AF
__vid_prevrow::
        dec     h
        ld      a,h
        and     #0x07
        cp      #0x07
        ret     nz                      ; seven rows in eight end here

        ;; ------------------------------------------------------------
        ;; __vid_prevrow_carry
        ;; The character-cell crossing half of __vid_prevrow, entered the
        ;; same way __vid_nextrow_carry is.
        ;;
        ;; Arguments:
        ;;   HL = row pointer whose low three row bits have just wrapped
        ;;
        ;; Return:
        ;;   HL = pointer to the last row of the previous character cell
        ;;
        ;; Clobbers:
        ;;   AF
__vid_prevrow_carry::
        ld      a,l
        sub     #0x20
        ld      l,a
        ret     c

        ld      a,h
        add     a,#0x08
        ld      h,a
        ret
