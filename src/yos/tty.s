        ;; tty.s
        ;;
        ;; core tty functions
        ;;
        ;; TODO:
        ;;  tty_putc and tty_getc
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2021 Tomaz Stih
        ;;
		;; 2021-06-16   tstih
        .module tty

        ;; functions
        .globl  _tty_cls
        .globl  _tty_scroll
        .globl  _tty_xy

        ;; variables
        .globl  _tty_x
        .globl  _tty_y
        
        ;; constants
        BLACK   =   0x00
        GREEN   =   0x04

        CWIDTH  =   0x06
        CHEIGHT =   0x06
        XMAX    =   246
        YMAX    =   186

        BDRPORT =   0xfe
        VMEMBEG =   0x4000
        ATTRSZE =   0x02ff
        VMEMSZE =   0x1800
        SCRROW6 =   0x4600              ; screen row 6
        BYTSROW =   32                  ; bytes per screen row

        FASCII  =   32

        .area   _CODE



        ;; ----------------------
        ;; extern void tty_cls();
        ;; ----------------------
        ;; clears the screen using black background 
        ;; color and green foreground color
        ;; affects: af, hl, de, bc
_tty_cls::
        ld      a,#BLACK
        out     (#BDRPORT),a            ; set border to black
        ;; prepare attr
        rlca                            ; bits 3-5
        rlca
        rlca
        or      #GREEN                  ; ink color to bits 0-2
        ;; first vmem
        ld      hl,#VMEMBEG             ; vmem
        ld      bc,#VMEMSZE             ; size
        ld      (hl),l                  ; l=0
        ld      d,h
        ld      e,#1
        ldir                            ; cls
        ld      (hl),a                  ; attr
        ld      bc,#ATTRSZE             ; size of attr
        ldir
        ret


        ;; ------------------------
        ;; extern void tty_scroll()
        ;; ------------------------
        ;; scrolls up one row
        ;; affects: af, bc, de, hl
_tty_scroll::
        ld      de,#VMEMBEG             ; scan line 1
        ld      hl,#SCRROW6             ; scan line 6
        ld      bc,#BYTSROW             ; bytes to transfer
        ;; move 186 lines (192 - 6)
        ld      a,#YMAX
ls_loop:
        push    af
        push    bc
        push    de
        push    hl
        ldir                            ; move scan line
        pop     hl
        pop     de
        call    vid_nextrow
        ex      de,hl
        call    vid_nextrow
        ex      de,hl
        pop     bc
        pop     af
        dec     a
        jr      nz,ls_loop
        ;; fill last line with zeroes
        ;; note: de already points to correct line
        ld      a,#CHEIGHT              ; 6 lines
        ex      de,hl                   ; correct line to hl
ls_clr_loop:
        push    hl                      ; store hl
        ld      b,#BYTSROW              ; bytes
ls_clrlne_loop:
        ld      (hl),#0
        inc     hl
        djnz    ls_clrlne_loop
        pop     hl
        push    af
        call    vid_nextrow
        pop     af
        dec     a
        jr      nz,ls_clr_loop
        ret


        ;; -----------------------------------------
        ;; extern void tty_xy(uint8_t x, uint8_t y);
        ;; -----------------------------------------
        ;; go to xy
        ;; affects: af, bc, de, hl
_tty_xy::
        ;; grab parameters from stack
        pop     de                      ; ret address
        pop     bc                      ; c=x, b=y
        ;; restore stack
        push    bc
        push    de
        ;; write to memory
        ld      a,b
        ld      (#_tty_y),a
        ld      a,c
        ld      (#_tty_x),a
        ;; and done
        ret


        .area _INITIALIZED
_tty_x:: 
        .ds     1
_tty_y:: 
        .ds     1


        .area _INITIALIZER
init_tty_x:
        .byte   0
init_tty_y:
        .byte   0

        