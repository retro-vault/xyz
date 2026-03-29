;;; time.s
;;;
;;; ZX Spectrum real time clock functions
;;; A simple 2 byte 50Hz tick counter
;;;
;;; MIT License (see: LICENSE)
;;; Copyright (C) 2021 Tomaz Stih
;;;
;;; 2021-07-11   tstih

.module time

.globl  __clock
.globl  __clock_tick

.area   _CODE

;;; extern unsigned int clock(void);
;;; return:  HL = current tick count
;;; affects: HL
__clock::
        ld      hl,(_clock_ticks)
        ret

;;; extern void _clock_tick(void);
;;; return:  (none)
;;; affects: AF, HL
;;; notes:   called from 50Hz interrupt, updates 32-bit counter
__clock_tick::
        ;; first update clock
        ld      hl,(_clock_ticks)
        inc     hl
        ld      (_clock_ticks),hl
        ld      a,h
        or      l
        jr      nz,.ctk_time
        ld      hl,(_clock_ticks+2)
        inc     hl
        ld      (_clock_ticks+2),hl
.ctk_time:
        ;; now update time_t
        ld      a,(_clock_sec_countdown)
        dec     a                       ; reduce sec. countdown
        jr      z,.ctk_sec_elapsed
        ld      (_clock_sec_countdown),a
        ret
.ctk_sec_elapsed:
        ld      a,#50                   ; reset counter
        ld      (_clock_sec_countdown),a
        ;; increase time_t counter
        ld      hl,(_clock_time)
        inc     hl
        ld      (_clock_time),hl
        ld      a,h
        or      l
        ret     nz                      ; done
        ;; if we are here, high word must be increased too
        ld      hl,(_clock_sec_countdown+2)
        inc     hl
        ld      (_clock_sec_countdown+2),hl
        ret

.area   _INITIALIZED
_clock_ticks:
        .ds     4
_clock_time:
        .ds     4
_clock_sec_countdown:
        .ds     1

.area   _INITIALIZER
init_clock_ticks:
        .byte   0, 0, 0, 0
init_clock_time:
        .byte   0, 0, 0, 0
init_clock_sec_countdown:
        .byte   50