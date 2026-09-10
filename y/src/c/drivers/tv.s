        ;; tv.s
        ;;
        ;; Video RAM calculations for ZX Spectrum
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2021 Tomaz Stih
        ;;
        ;; 2021-06-16   tstih

        .module	tv

        .area	_CODE

        .equ	VMEMBEG, 0x4000         ; video RAM start
        .equ	VMEMSZE, 0x1800         ; raster size
        .equ	ATTRSZE, 0x02ff         ; attr size
        .equ	BDRPORT, 0xfe           ; border port

        ;; tv_rowaddr
        ;; param:  B = Y coordinate (0-191)
        ;; return: HL = VRAM address, A = L
        ;; affects: AF, HL
        ;; notes:   converts Y coordinate to video RAM row address
tv_rowaddr::
        ld	a,b                     ; get y0-y2 to acc
        and	#0x07                   ; mask out 00000111
        or	#0x40                   ; VRAM addr
        ld	h,a                     ; to h
        ld	a,b                     ; y to acc again
        rrca
        rrca
        rrca
        and	#0x18                   ; y6,y7 to correct pos
        or	h                       ; h or a
        ld	h,a                     ; store to h
        ld	a,b                     ; y back to a
        rla                             ; move y3-y5 to position
        rla
        and	#0xe0                   ; mask out 11100000
        ld	l,a                     ; to l
        ret

        ;; tv_nextrow
        ;; param:  HL = current row address
        ;; return: HL = next row address
        ;; affects: AF, HL
        ;; notes:   advances HL to next video RAM row address
tv_nextrow::
        inc	h
        ld	a,h
        and	#7
        jr	nz,.tv_nextrow_done
        ld	a,l
        add	a,#32
        ld	l,a
        jr	c,.tv_nextrow_done
        ld	a,h
        sub	#8
        ld	h,a
.tv_nextrow_done:
        ret

        ;; extern void tv_cls(uint8_t bg, uint8_t fg);
        ;; param:  A = background color
        ;;         B = foreground color
        ;; return: (none)
        ;; affects: AF, HL, DE, BC
        ;; notes:   clears entire screen with attribute colors
tv_cls::
        out	(#BDRPORT),a            ; set border
        ;; prepare attr
        rlca                            ; bits 3-5
        rlca
        rlca
        or	b                       ; ink color to bits 0-2
        ;; first clear VRAM
        ld	hl,#VMEMBEG             ; VRAM addr
        ld	bc,#VMEMSZE             ; size
        ld	(hl),l                  ; l=0
        ld	d,h
        ld	e,#1
        ldir                            ; clear
        ;; then clear attributes
        ld	(hl),a                  ; attr
        ld	bc,#ATTRSZE             ; size of attr
        ldir
        ret
