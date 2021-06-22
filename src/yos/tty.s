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
        .globl  _tty_putc
        .globl  _tty_puts

        ;; variables
        .globl  _tty_x
        .globl  _tty_y
        
        ;; constants
        BLACK   =   0x00
        GREEN   =   0x04

        CWIDTH  =   0x06
        CHEIGHT =   0x06
        CXMAX   =   41
        CYMAX   =   31

        BDRPORT =   0xfe
        VMEMBEG =   0x4000
        ATTRSZE =   0x02ff
        VMEMSZE =   0x1800
        SCRROW6 =   0x4600              ; screen row 6
        BYTSROW =   32                  ; bytes per screen row

        FASCII  =   32
        LF      =   0x0A
        CR      =   0x0D

        .area   _CODE


        ;; ----------------------
        ;; extern void tty_cls();
        ;; ----------------------
        ;; clears the screen using black background 
        ;; color and green foreground color
        ;; affects: af, hl, de, bc
_tty_cls::
        ld      a,#BLACK
        ld      b,#GREEN
        call    vid_cls
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
        ld      a,#186
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


        ;; ----------------------------
        ;; extern void tty_putc(int c);
        ;; ----------------------------
        ;; draw a character at current x,y
        ;; this function does not manage location i.e.
        ;; it leaves x,y untouched. it just draws the
        ;; character.
        ;; affects: af, bc, de, hl
_tty_putc::
        pop     hl                      ; get return address
        pop     de                      ; get char
        ;; restore stack after obtaining parameters
        push    de
        push    hl
putc_raw:
        ;; make hl point to correct character
        ld      a,e                     ; char ascii to a
        sub     #FASCII                 ; minus first ascii
        ld      h,#0                    ; high=0
        ld      l,a                     ; char
        push    hl                      ; store hl
        add     hl,hl                   ; hl=hl*2
        add     hl,hl                   ; hl=hl*4
        pop     de                      ; de=original hl
        add     hl,de                   ; hl=hl*5
        ex      de,hl                   ; de=hl
        ld      hl,#_tty_font           ; font start
        add     hl,de                   ; hl points to correct char
        ex      de,hl                   ; de=start of char
        ;; calculate correct row
        ld      a,(#_tty_y)             ; get y, low res.
        sla     a                       ; *2
        ld      b,a                     ; store
        sla     a                       ; *4
        add     b                       ; *6
        ld      b,a                     ; hires y to b
        call    vid_rowaddr             ; hl=vmem addr.
        ;; calculate required shifts
        ;; we need to multiply by 6, add 2 (offset)
        ;; and then divide by 8 to get the correct
        ;; byte
        ld      a,(#_tty_x)             ; x into a
        sla     a                       ; a=a*2
        ld      b,a
        sla     a                       ; a=a*4
        add     b                       ; a=a*6
        add     #2                      ; add offset
        ;; now get the correct byte and shifts
        ld      c,a                     ; store a
        and     #0x07                   ; shifts to a
        srl     c                       ; /2
        srl     c                       ; /4
        srl     c                       ; /8 
        ld      b,#0                    ; bc=byte
        add     hl,bc                   ; add to vmem...
        ;; at this point
        ;; de=char start
        ;; hl=vmem start
        ;; a=shifts
        ;; if shifts <=2 we'll clip second byte
        ex      af,af'
        xor     a                       ; assume no clip
        ex      af,af'
        cp      #3                      ; a-3
        jr      nc,no_clip              ; a<=2...
        ex      af,af'
        inc     a                       ; a=1, clip!
        ex      af,af'
no_clip:
        ld      c,a                     ; num shift
        push    bc
        ex      de,hl                   ; easier to calculate
        ;; scan line 0
        ld      a,(hl)                  ; data
        call    pch_to_screen
        ;; scan line 1
        ld      a,(hl)                  ; data
        inc     hl
        ld      b,(hl)                  ; data byte 2
        srl     a
        rr      b
        srl     a
        rr      b
        ld      a,b                     ; a=data
        call    pch_to_screen
        ;; scan line 2
        ld      a,(hl)                  ; data
        inc     hl
        ld      b,(hl)                  ; data
        sla     b                       ; shift 4xleft
        rl      a
        sla     b
        rl      a
        sla     b
        rl      a
        sla     b
        rl      a                       ; a=data
        call    pch_to_screen
        ;; scan line 3
        ld      a,(hl)                  ; data
        inc     hl                      ; next char byte
        sla     a                       ; 2x left
        sla     a                       ; a=data
        call    pch_to_screen
        ;; scan line 4
        ld      a,(hl)                  ; data
        call    pch_to_screen
        ;; scan line 5
        ld      a,(hl)                  ; data
        inc     hl
        ld      b,(hl)                  ; data byte 2
        srl     a
        rr      b
        srl     a
        rr      b
        ld      a,b                     ; a=data
        call    pch_to_screen
        pop     bc
        ret
pch_to_screen:
        ;; a=data
        ;; de=vmem address
        exx                             ; temp
        pop     hl                      ; ret address
        exx
        pop     bc                      ; get num shifts into c
        push    bc                      ; restore stack
        exx
        push    hl                      ; ret address back
        exx
        ex      de,hl                   ; hl=vmem address
        push    de                      ; ...will need it later
        ld      de,#0x03ff              ; mask
        and     #0xfc                   ; cut data
        ld      b,a                     ; store data to b
        ld      a,c                     ; a=num shifts
        cp      #0                      ; no shifts?
        jr      z,pch_sh_done           ; shift is done
        ld      c,#0x00                 ; c=0
pch_shift:
        srl     b                       ; shift data
        rr      c                       ; 16 bits
        scf                             ; carry
        rr      d                       ; shift mask
        rr      e                       ; and data
        dec     a                       ; a=a-1
        jr      nz,pch_shift            ; shift on
pch_sh_done:
        ld      a,d                     ; first mask to a
        and     (hl)                    ; mask AND screen
        or      b                       ; OR data
        ld      (hl),a                  ; back to screen
        ;; TODO: we only need second byte
        ;; if shifts > 2
        ex      af,af'
        or      a                       ; clip?
        jr      nz, pch_skip2           ; skip 2nd byte
        ex      af,af'
        inc     hl                      ; next byte
        ld      a,e                     ; second mask
        and     (hl)                    ; mask AND screen
        or      c                       ; and data
        ld      (hl),a                  ; to screen
        dec     hl                      ; vmem pointer back
        jr      pch_no_alt
pch_skip2:
        ex      af,af'
pch_no_alt:
        call    vid_nextrow             ; next row
        pop     de                      ; restore de
        ex      de,hl                   ; toggle de and hl
        ret


        ;; -------------------------------
        ;; extern void tty_puts(string s);
        ;; -------------------------------
        ;; draws a string, this function updates 
        ;; cursor position and respects escape sequences:
        ;; \n, \r
_tty_puts::
        ;; get ptr to string to hl
        pop     de                      ; ret address
        pop     hl                      ; ptr to string
        ;; restore stack
        push    hl
        push    de
puts_loop:
        ld      a,(hl)                  ; a = ascii
        cp      #0                      ; end of string?
        ret     z                       ; return if done...
        push    hl                      ; store current ptr.
        cp      #LF                     ; line feed?
        jr      z, linefeed
        ld      e,a                     ; ascii to e
        call    putc_raw                ; to screen
        ld      a,(#_tty_x)             ; a=x
        cp      #CXMAX                  ; x==max x?
        jr      z,linefeed              ; linefeed
        inc     a                       ; increase x
        ld      (#_tty_x),a             ; and store it
nextch:
        pop     hl                      ; pointer to next char
        inc     hl                      ; prepare for next char
        jr      puts_loop               ; and next
linefeed:
        call    newline
        jr      nextch
newline:
        ld      a,(#_tty_y)             ; get y coord.
        cp      #CYMAX                  ; last row?
        jr      nz,nl_add
        ;; we need to scroll
        push    af                      ; store y
        call    _tty_scroll
        pop     af
        jr      nl_update
nl_add:
        inc     a                       ; y++
nl_update:
        ld      (#_tty_y),a             ; store new y
        xor     a                       ; a=0
        ld      (#_tty_x),a             ; and new x
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