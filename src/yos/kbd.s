        ;; kbd.s
        ;;
        ;; zx spectrum low level keyboard scanning
        ;;
        ;; NOTES:
        ;;  kbd_scan must be wired to 50Hz interrupt
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2021 Tomaz Stih
        ;;
		;; 2014-09-17   tstih
		.module kbd
		
		.globl  _kbd_scan
		.globl  _kbd_read
        .globl  _kbd_buff

	
		BUFSIZE	= 0x20					; 32 bytes of keyb. buffer


		.area	_CODE
_kbd_scan::	
		ld		hl,#kbd_prev_scan
		ld		d,#0					; scan lines counter 
		ld		bc,#0xf7fe				; first scan line
scan_line:
		in		a,(c)					; get it in
		push	bc						; store b and c
		and		#0b00011111				; just interested in bits 0-4
		cp		(hl)					; compare to previous scan 
		jr		z,next_line				; nothing has changed
		;; ah-ha...we have a change!
		;; d=byte counter
		ld		e,a						; store curr state
		xor		(hl)					; xor prev state
		push	af						; store a
		ld		b,#0b00000000			; bit 6 is 0
		and		e						; a...released buttons
		call	nz, key_change
		pop		af						; back a
		ld		b,#0b01000000			; bit 6 is 1
		and		(hl)					; a...pressed buttons	
		call	nz, key_change
		ld		(hl),e					; update prev scan
next_line:	
		;; next scan line addr to b
		pop		bc						; restore b and c
		rlc		b						
		inc		d
		ld		a,d						; get counter to a 
		cp		#8						; max scan lines reached?
		jr		z,end_scan				; no more lines to scan?
		inc		hl						; inc prev scan line pointer
		jr		scan_line				; scan another one
end_scan:
		ret
		;; d is line number 0-7
		;; a holds bits 0..4 (5 bits), bit 6 is 1...pressed, 0...not pressed
		;; formula for key number is d*5 + set_bit(a)
key_change:	
		push	de						; store D,E
		ld		e,a						; store a
		ld		a,d						; get d into a
		rlc		d						; d=d*2
		rlc		d						; d=d*4
		add		d						; a=a+d*4=5*d
		ld		d,a						; d=d*5
		ld		a,#5					; 5 bits
rotate_keymsk:	
		sra		e						; bit into carry
		jr		nc,next_key
		push	af
		add		d						; a=correct key code
		dec		a
		or		b	
		call	queue_key
		pop		af
		sub		d						; back to a
next_key:
		dec		a
		jr 		nz,rotate_keymsk
		pop		de		
		ret
		;; queue key in a 
queue_key::
		push	bc
		push	de
		push	hl
		ld		c,a						; store a to c
		ld		a,(#_kbd_buff+2)		; a=count
		cp		#BUFSIZE				; is full?
		jr		z,qkey_end				; unfortunately we lost the key...
		ld		hl,(#_kbd_buff)			; l=start, h=end
		ld		de,#_kbd_buff+3			; de is start of kbd buffer
		ld		l,h						; l=end
		ld		h,#0					; hl=end
		add		hl,de					; hl=buffer address
		ld		(hl),c					; key to buffer
		inc		a						; count++
		ld		(#_kbd_buff+2),a		; store count
		ld		a,(#_kbd_buff+1)		; a=end
		inc		a						; end++
		cp		#BUFSIZE				; beyond the edge?
		jr		nz,qk_proceed
		xor		a						; a=0
qk_proceed:
		ld		(#_kbd_buff+1),a		; store end
qkey_end:
		pop		hl
		pop		de
		pop		bc
		ret

		;; -----------------------
		;; extern byte kbd_read();
		;; -----------------------
_kbd_read::
		call	_ir_disable			    ; could be interrupted by _kbd_scan
		ld		a,(#_kbd_buff+2)		; a=count
		cp		#0						; is it zero?
		jr		z,kr_empty				; no data in buffer
		; get the char
		ld		hl,(#_kbd_buff)			; l=start, h=end
		ld		de,#_kbd_buff+3			; de is start of kbd buffer
		ld		h,#0x00					; l=start, h=0
		add		hl,de					; hl points to correct place
		ld		b,(hl)					; get char to b
		; decrease counter, increase start
		dec		a
		ld		(#_kbd_buff+2),a
		ld		a,(#_kbd_buff)			; a=start
		inc		a
		cp 		#BUFSIZE				; end of buffer?
		jr		nz,kr_proceed
		xor		a						; reset start
kr_proceed:
		ld		(#_kbd_buff),a			; ...and store
		ld		l,b						; return char
		jr		kr_end					; game over
kr_empty:
		ld		l,#0x00					; key not found
kr_end:	
		call	_ir_enable			    ; enable (again)
		ret

		.area	_INITIALIZED
kbd_prev_scan:
		.ds		8
_kbd_buff::
		.ds		3 + BUFSIZE


		.area 	_INITIALIZER
init_kbd_prev_scan:
		.byte	0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f
init_kbd_buff:
		.byte	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0