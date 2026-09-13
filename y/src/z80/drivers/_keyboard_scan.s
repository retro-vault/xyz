        ; Scan the ZX Spectrum keyboard and queue key transitions.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module _keyboard_scan
        .optsdcc -mz80 sdcccall(1)

        .globl  __kbd_scan
        .globl  __kbd_prev_scan
        .globl  __kbd_caps
        .globl  __kbd_symbol
        .globl  __kbd_buffer

        .equ    BUFSIZE, 32
        .equ    KEY_CAPS, 0x1d
        .equ    KEY_SYMB, 0x17
        .equ    KEY_DWN_BIT, 0x40

        .area   _CODE

        ; Called by the 50 Hz kernel timer. Queue raw press/release events.
__kbd_scan::
        ;; first handle special cases - caps and symbol shift
        ;; they must be processed first to give meaning
        ;; to keys after them (especially in emulators and
        ;; custom keyboards that send two key codes at exactly
        ;; the same time).
.check_caps:
        ld      bc,#0xfefe              ; shift row
        in      a,(c)                   ; scan line
        cpl                             ; if 0 then 1
        and     #1                      ; isolate shift bit
        ld      b,a                     ; store key state
        ld      hl,#__kbd_caps            ; get prev. state
        xor     (hl)                    ; if the same no change
        jr      z, .kbs_check_symbol
        ;; we have caps up or down here
        ld      a,b                     ; get status back
        or      a                       ; compare to itself
        jr      nz,.kbs_caps_down
.kbs_caps_up:
        ld      a,#KEY_CAPS             ; caps
        call    .queue_key              ; queue
        xor     a                       ; a=0
        ld      (hl),a                  ; to previous state
        jr      .kbs_check_symbol
.kbs_caps_down:
        ld      a,#KEY_CAPS             ; caps
        or      #KEY_DWN_BIT            ; ...add down bit
        call    .queue_key              ; queue
        ld      a,#1                    ; 1...
        ld      (hl),a                  ; ...to previous state
.kbs_check_symbol:
        ld      bc,#0x7ffe              ; shift row
        in      a,(c)                   ; scan line
        srl     a                       ; bit 1(symol) to bit 0
        cpl                             ; if 0 then 1
        and     #1                      ; isolate shift bit
        ld      b,a                     ; store key state
        ld      hl,#__kbd_symbol          ; get prev. state
        xor     (hl)                    ; if the same no change
        jr      z, .kbd_lines
        ;; we have symbol up or down here
        ld      a,b                     ; get status back
        or      a                       ; compare to itself
        jr      nz,.kbs_sym_down
.kbs_sym_up:
        ld      a,#KEY_SYMB             ; symbol shift
        call    .queue_key              ; queue
        xor     a                       ; a=0
        ld      (hl),a                  ; to previous state
        jr      .kbd_lines
.kbs_sym_down:
        ld      a,#KEY_SYMB             ; caps
        or      #KEY_DWN_BIT            ; ...add down bit
        call    .queue_key              ; queue
        ld      a,#1                    ; 1...
        ld      (hl),a                  ; ...to previous state
.kbd_lines:
        ;; now do normal scan
        ld      hl,#__kbd_prev_scan
        ld      d,#0                    ; scan lines counter
        ld      bc,#0xf7fe              ; first scan line
.scan_line:
        in      a,(c)                   ; get it in
        push    bc                      ; store b and c
        and     #0b00011111             ; just interested in bits 0-4
        cp      (hl)                    ; compare to previous scan
        jr      z,.next_line            ; nothing has changed
        ;; ah-ha...we have a change!
        ;; d=byte counter
        ld      e,a                     ; store curr state
        xor     (hl)                    ; xor prev state
        push    af                      ; store a
        ld      b,#0b00000000           ; bit 6 is 0
        and     e                       ; a...released buttons
        call    nz, .key_change
        pop     af                      ; back a
        ld      b,#0b01000000           ; bit 6 is 1
        and     (hl)                    ; a...pressed buttons
        call    nz, .key_change
        ld      (hl),e                  ; update prev scan
.next_line:
        ;; next scan line addr to b
        pop     bc                      ; restore b and c
        rlc     b
        inc     d
        ld      a,d                     ; get counter to a
        cp      #8                      ; max scan lines reached?
        jr      z,.end_scan             ; no more lines to scan?
        inc     hl                      ; inc prev scan line pointer
        jr      .scan_line              ; scan another one
.end_scan:
        ret
        ;; d is line number 0-7
        ;; a holds bits 0..4 (5 bits), bit 6 is 1...pressed, 0...not pressed
        ;; formula for key number is d*5 + set_bit(a)
.key_change:
        push    de                      ; store D,E
        ld      e,a                     ; store a
        ld      a,d                     ; get d into a
        rlc     d                       ; d=d*2
        rlc     d                       ; d=d*4
        add     d                       ; a=a+d*4=5*d
        ld      d,a                     ; d=d*5
        ld      a,#5                    ; 5 bits
.rotate_keymsk:
        sra     e                       ; bit into carry
        jr      nc,.next_key
        push    af
        add     d                       ; a=correct key code
        dec     a
        ;; skip special cases: CAPS and SYMBOL!
        ;; they were already handled.
        cp      #KEY_SYMB               ; don't queue sym. shift
        jr      z,.skip_queue           ; because we already incl. it.
        cp      #KEY_CAPS               ; and same for
        jr      z,.skip_queue           ; caps shift
        or      b                       ; add key down bit
        call    .queue_key
.skip_queue:
        pop     af                      ; restore bit countdown, before row offset
.next_key:
        dec     a
        jr      nz,.rotate_keymsk
        pop     de
        ret

        ;; .queue_key
        ;; param:  A = key code to queue
        ;; return: (none)
        ;; affects: BC, DE, HL
.queue_key:
        push    bc
        push    de
        push    hl
        ld      c,a                     ; store a
        ld      a,(#__kbd_buffer+2)       ; a=count
        cp      #BUFSIZE                ; is full?
        jr      z,.qkey_end             ; unfortunately we lost the key...
        ld      hl,(#__kbd_buffer)        ; l=start, h=end
        ld      de,#__kbd_buffer+3        ; de is start of kbd buffer
        ld      l,h                     ; l=end
        ld      h,#0                    ; hl=end
        add     hl,de                   ; hl=buffer address
        ld      (hl),c                  ; key to buffer
        inc     a                       ; count++
        ld      (#__kbd_buffer+2),a       ; store count
        ld      a,(#__kbd_buffer+1)       ; a=end
        inc     a                       ; end++
        cp      #BUFSIZE                ; beyond the edge?
        jr      nz,.qk_proceed
        xor     a                       ; a=0
.qk_proceed:
        ld      (#__kbd_buffer+1),a       ; store end
.qkey_end:
        pop     hl
        pop     de
        pop     bc
        ret

