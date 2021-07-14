        ;; util.s
        ;;
        ;; utility functions
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2021 Tomaz Stih
        ;;
		;; 2021-07-11   tstih
		.module util
		
		.globl  _lob
        .globl  _hib


        ;; -------------------------------
        ;; extern uint8_t lob(uint16_t w);
        ;; ------------------------------
        ;; returns low byte of word
        ;; affects: hl
		.area	_CODE
_lob::
        pop     de                      ; get ret address
        pop     hl                      ; get word
        ld      h,#0                    ; hi byte to 0
        push    hl                      ; restore...
        push    de                      ; ...stack
        ret

    
        ;; -------------------------------
        ;; extern uint8_t hib(uint16_t w);
        ;; ------------------------------
        ;; returns high byte of word
        ;; affects: hl
		.area	_CODE
_hib::
        pop     de                      ; get ret address
        pop     hl                      ; get word
        ld      l,h                     ; high to low
        ld      h,#0                    ; hi byte to 0
        push    hl                      ; restore...
        push    de                      ; ...stack
        ret