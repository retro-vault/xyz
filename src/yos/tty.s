        ;; tty.s
        ;;
        ;; core tty functions
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
        .globl  _tty_attr
        .globl  _tty_outc
        .globl  _tty_putc
        .globl  _tty_getc
        .globl  _tty_puts

        .globl  __tty_tick_cursor
        .globl  _tty_show_cursor
        .globl  _tty_hide_cursor
        
        ;; constants
        .equ    BLACK, 0x00
        .equ    GREEN, 0x04

        .equ    CWIDTH, 0x06
        .equ    CHEIGHT, 0x06
        .equ    CXMAX, 41
        .equ    CYMAX, 31

        .equ    BDRPORT, 0xfe
        .equ    VMEMBEG, 0x4000
        .equ    ATTRSZE, 0x02ff
        .equ    VMEMSZE, 0x1800
        .equ    SCRROW6, 0x4600         ; screen row 6
        .equ    BYTSROW, 32             ; bytes per screen row

        .equ    FASCII, 32              ; incl.
        .equ    LASCII, 128             ; not incl.
        .equ    LF, 0x0A
        .equ    CR, 0x0D
        .equ    UNDERLINE, 0x01         ; first bit is underline
        .equ    INVERSE, 0x02           ; second bit is inverse

        .equ    CURSOR_ENABLED, 0x01    ; is cursor enabled? 
        .equ    CURSOR_VISIBLE, 0x02    ; is cursor visible?

        .equ    KEY_DOWN_BIT, 0b01000000
        .equ    KEY_CODE,     0b00111111
        .equ    KEY_CAPS,     0x1d
        .equ    KEY_SYMB,     0x17

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


        ;; -----------------------------------
        ;; extern void tty_attr(uint8_t attr);
        ;; -----------------------------------
        ;; set attribute
        ;; affects: af, bc, de, hl
_tty_attr::
        ;; grab parameters from stack
        pop     de                      ; ret address
        pop     bc                      ; c=attr
        ;; restore stack
        push    bc
        push    de
        ;; write to memory
        ld      a,c
        ld      (#_tty_at),a
        ret


        ;; ----------------------------
        ;; extern void tty_outc(int c);
        ;; ----------------------------
        ;; draw a character at current x,y
        ;; this function does not manage location i.e.
        ;; it leaves x,y untouched. it just draws the
        ;; character.
        ;; affects: af, bc, de, hl
_tty_outc::
        pop     hl                      ; get return address
        pop     de                      ; get char
        ;; restore stack after obtaining parameters
        push    de
        push    hl
outc_ascii:
        ;; make de point to correct character
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
        ;; draws char pointed by de at x,y
        ;; d' has attribute
outc_raw:
        ;; calculate correct row
        ld      a,(_tty_y)              ; get y, low res.
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
        ld      a,(_tty_x)              ; x into a
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
        ld      a,(_tty_at)             ; get attribute
        and     #UNDERLINE              ; is it underline?
        jr      nz, oc_ul
        ld      a,(hl)                  ; data
        inc     hl
        ld      b,(hl)                  ; data byte 2
        srl     a
        rr      b
        srl     a
        rr      b
        ld      a,b                     ; a=data
        jr      oc_ul_chk_end
        ;; underline!
oc_ul:
        ld      a,#0xfc 
oc_ul_chk_end:
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
        call    oc_chk_inverse
        or      b                       ; OR data
        ld      (hl),a                  ; back to screen
        ;; TODO: we only need second byte
        ;; if shifts > 2
        ex      af,af'
        or      a                       ; clip?
        jr      nz, pch_skip2           ; skip 2nd byte
        ex      af,af'
        inc     hl                      ; next byte
        ld      d,e                     ; mask to d (again)
        ld      a,d                     ; second mask
        and     (hl)                    ; mask AND screen
        ld      b,c                     ; c to b
        call    oc_chk_inverse
        or      b                       ; and data
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
        ;; value is in b, if inv is on 
        ;; then it will cpl it
oc_chk_inverse:
        push    bc                      ; store b=value
        push    de                      ; store mask
        exx
        pop     de                      ; restore mask
        pop     bc                      ; restore b as b'
        ld      l,a                     ; store a to l'
        ld      a,(_tty_at)             ; get attribute
        and     #INVERSE                ; inverse?
        jr      z, oc_inv_chk_end
        ;; inverse!
        ld      a,b                     ; value to a
        cpl
        xor     d                       ; complement d
        ld      b,a                     ; to b
oc_inv_chk_end:
        ld      a,l                     ; restore a
        push    bc
        push    de
        exx
        pop     de
        pop     bc
        ret


        ;; ------------------------------
        ;; extern void tty_tick_cursor();
        ;; ------------------------------
        ;; blink cursor at x,y
        ;; if shown then it is hiddena and
        ;; vice versa
__tty_tick_cursor::
        call    _ir_disable             ; no interrupts
        ld      a,(_tty_cur_sts)        ; get cursor status
        and     #CURSOR_ENABLED         ; is it enabled?
        jr      z, ttc_done             ; disabled...finish

        

ttc_done:
        call    _ir_enable              ; interrupts
        ret


        ;; ------------------------------
        ;; extern void tty_hide_cursor();
        ;; ------------------------------
        ;; hides cursor, unconditionally
_tty_hide_cursor::
        call    _ir_disable             ; no interrupts
        ld      a,(_tty_cur_sts)        ; current status
        and     #CURSOR_VISIBLE         ; is it visible?
        jr      z, thc_done             ; cursor not on screen
        ;; calling tick will hide (xor) it
        call    __tty_tick_cursor
thc_done:
        xor     a                       ; cursor invisible & disabled
        ld      (_tty_cur_sts),a        ; set cursor status
        call    _ir_enable              ; enable interrupts (again!)
        ret


        ;; ------------------------------
        ;; extern void tty_show_cursor();
        ;; ------------------------------
        ;; shows cursor, set blink state
_tty_show_cursor::
        call    _ir_disable             ; no interrupts
        ld      a,(_tty_cur_sts)        ; are we visible?
        and     #CURSOR_ENABLED         ; is it enabled?
        jr      z,tsc_just_flag         ; just the flag
        ;; if we are here it's enabled...
        ;; check if we are already visible
        ld      a,(_tty_cur_sts)        ; get status again
        and     #CURSOR_VISIBLE         ; are we visible
        jr      nz,tsc_just_flag        ; exit
        ;; if we are here, it's enabled and invisible
        ;; so do a tick...
        call    __tty_tick_cursor
tsc_just_flag:
        ld      a,(_tty_cur_sts)        ; get status
        or      #CURSOR_VISIBLE         ; or it
        ld      (_tty_cur_sts),a        ; and write it ack
        call    _ir_enable
        ret



        ;; ----------------------------
        ;; extern void tty_putc(int c);
        ;; ----------------------------
        ;; puts a character at current x,y
        ;; updates x and y, scrolls if needed
        ;; and interprets special
        ;; symbols: \n, \r
        ;; affects: af, bc, de, hl
_tty_putc::
        pop     hl                      ; get return address
        pop     de                      ; get char
        ;; restore stack after obtaining parameters
        push    de
        push    hl
putc_raw:
        ;; check for new line...
        ld      a,e                     ; to a
        cp      #LF                     ; line feed?
        jr      z, linefeed
        ;; check for any other control char
        cp      #FASCII                 ; first ascii
        ret     c                       ; < 32
        cp      #LASCII                 ; >127
        ret     nc
        ;; print char
        call    outc_ascii
        ;; are we at line end?
        ld      a,(_tty_x)              ; a=x
        cp      #CXMAX                  ; x==max x?
        jr      z,linefeed              ; linefeed
        inc     a                       ; increase x
        ld      (_tty_x),a              ; and store it
        ret        
linefeed:
        call    newline
        ret
newline:
        ld      a,(_tty_y)              ; get y coord.
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
        ld      (_tty_y),a              ; store new y
        xor     a                       ; a=0
        ld      (_tty_x),a              ; and new x
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
        ld      e,a                     ; ascii to e
        call    putc_raw                ; to screen
nextch:
        pop     hl                      ; pointer to next char
        inc     hl                      ; prepare for next char
        jr      puts_loop               ; and next



        ;; ----------------------------
        ;; extern int tty_getc();
        ;; ----------------------------
        ;; gets next character from keyb. buffer,
        ;; returns 0 if buffer empty.
        ;; affects: af, bc, de, hl
_tty_getc::
        call    _kbd_read               ; key waiting?
        ld      h,#0
        ld      a,l                     ; key to l
        or      a                       ; is zero?
        ret     z                       ; no key...
        dec     l                       ; make key code 0 based
        ld      a,l                     ; back o a
        and     #KEY_DOWN_BIT           ; is key down or up?
        jr      nz, tgc_key_down        ; key is down
        ;; key up means you need to select 
        ;; correct key map and map the key code.
tgc_key_up:
        ld      a,l                     ; key back into a...
        call    tgc_check_ctl           ; check symb. and caps
        or      a                       ; a=0=>normal char
        jr      z, _tty_getc            ; ignore
        ld      b,a                     ; store ctrl to b
        ld      a,(_tty_ctl_key)        ; current ctl keys
        ld      c,a                     ; store to c
        ld      a,b                     ; control key back
        cpl                             ; complement it
        and     c                       ; delete in a
        ld      (_tty_ctl_key),a        ; write into mem.
        jr      _tty_getc               ; next queued key...
tgc_key_down:
        ld      a,l                     ; key code to a
        call    tgc_check_ctl           ; check symb. and caps
        or      a                       ; is a 0?
        jr      z,tgc_char_down         ; if char down get it!
        ;; if we are here then it is symbol or caps
        ld      b,a                     ; store ctrl to b
        ld      a,(_tty_ctl_key)        ; current ctl keys
        or      b                       ; set correct bit
        ld      (_tty_ctl_key),a        ; write into mem.
        jr      _tty_getc               ; next queued key...
tgc_char_down:
        ld      a,(_tty_ctl_key)        ; get control key
        ld      b,l                     ; store l
        ld      hl,#key_map             ; basic key map (no control keys)
        ld      de,#40                  ; map size (...to next map)
tgc_next_map:        
        or      a                       ; test a
        jr      z, tgc_map_sel
        add     hl,de                   ; next map
        dec     a
        jr      tgc_next_map            ; and loop
tgc_map_sel:
        ;; map is in hl, key is in b
        ld      d,#0
        ld      a,b         
        and     #~KEY_DOWN_BIT          ; remove key down bit
        ld      e,a                     ; to e
        add     hl,de                   ; add key to hl
        ld      a,(hl)                  ; get mapped key
        ;; store to hl and return
        ld      h,#0
        ld      l,a
        ret        
        ;; checks if key in a is control
        ;; return a=0...normal key,
        ;; a=1/2 symb/caps
tgc_check_ctl:
        and     #~KEY_DOWN_BIT          ; remove key down bit
        cp      #KEY_CAPS 
        jr      nz,tgc_check_symb
        ld      a,#2                    ; caps on
        ret
tgc_check_symb:
        cp      #KEY_SYMB
        jr      nz,tgc_normal_key
        ld      a,#1
        ret
tgc_normal_key:
        xor     a
        ret


		;; key maps	
        ;; from keyboard scan codes to
        ;; ascii characters (respects
        ;; control keys)	
		;; 0x0d ... enter
		;; 0x20 ... space
		;; 0x01 ... symbol shift
		;; 0x02 ... caps shift
		;; 0x03 ... symbol + undefined
		;; 0x04 ... caps + undefined
		;; 0x08 ... backspace
		;; 0x09 ... left
		;; 0x0a ... down
		;; 0x0b ... up
		;; 0x0c ... right
		;; 0x61 ... pound symbol
		;; 0x21 ... single quote
key_map:	
		.byte '5',   '4',   '3',   '2',   '1'
		.byte '6',   '7',   '8',   '9',   '0'
		.byte 'y',   'u',   'i',   'o',   'p'
		.byte 'h',   'j',   'k',   'l',   0x0d 	
		.byte 'b',   'n',   'm',    0x01, 0x20
		.byte 'v',   'c',   'x',   'z',   0x02
		.byte 'g',   'f',   'd',   's',   'a'
		.byte 't',   'r',   'e',   'w',   'q'

key_map_sym:	
		.byte '%',   '$',   '#',   '@',   '!'
		.byte '&',   0x27,  '(',   ')',   '_'
		.byte '[',   ']',   0x80,  ';',   '"'
		.byte 0x5e,  '-',   '+',   '=',   0x0d 
		.byte '*',   ',',   '.',   0x01,  0x20	
		.byte '/',   '?',   0x61,  ':',   0x02
		.byte '}',   '{',   0x5d,  '|',   '~'
		.byte '>',   '<',   0x60,  0x03,  0x03

key_map_shf:	
		.byte 0x09,  '4',   '3',   '2',   '1'
		.byte 0x0a,  0x0b,  0x0c,  '9',   0x08
		.byte 'Y',   'U',   'I',   'O',   'P'
		.byte 'H',   'J',   'K',   'L',   0x04 
		.byte 'B',   'N',   'M',   0x01,  0x20
		.byte 'V',   'C',   'X',   'Z',   0x02
		.byte 'G',   'F',   'D',   'S',   'A'
		.byte 'T',   'R',   'E',   'W',   'Q'


        .area _INITIALIZED
_tty_x: 
        .ds     1
_tty_y:
        .ds     1
_tty_at:
        .ds     1
_tty_cur_sts:
        .ds     1
_tty_ctl_key:
        .ds     1

        .area _INITIALIZER
init_tty_x:
        .byte   0
init_tty_y:
        .byte   0
init_tty_at:
        .byte   0
init_tty_cur_sts:
        .byte   0x01
init_tty_ctl_key:
        .byte   0