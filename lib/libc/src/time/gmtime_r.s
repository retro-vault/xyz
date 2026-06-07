        ; gmtime_r.s
        ;
        ; gmtime_r / localtime_r (local == UTC) for the xcc Z80 libc, in
        ; assembly.  Splits a 32-bit time_t into a struct tm.  The 32-bit
        ; seconds<->days arithmetic uses the runtime long helpers; the
        ; year/month walk is plain 16-bit code (valid across the whole 32-bit
        ; time_t range ~1902..2038, where leap == y % 4 == 0).
        ;
        ; The leap test and month-length table are exported because mktime
        ; (its inverse) shares them.
        ;
        ; struct tm offsets (int == 2 bytes):
        ;   0 sec  2 min  4 hour  6 mday  8 mon  10 year  12 wday  14 yday  16 isdst
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module gmtime_r
        .optsdcc -mz80 sdcccall(1)


        .globl  _gmtime_r
        .globl  _localtime_r
        .globl  __time_leap
        .globl  __time_mdays

        .globl  __divslong
        .globl  __modslong
        .globl  __divulong
        .globl  __modulong
        .globl  __divuint

        .area   _DATA
__gm_secs:  .ds 4          ; working time_t / seconds-of-day
__gm_days:  .ds 2          ; day count since 1970-01-01 (fits 16-bit)
__gm_year:  .ds 2          ; full year

        .area   _CODE

__time_mdays::
        .db     31,28,31,30,31,30,31,31,30,31,30,31

        ; __time_leap: HL = full year -> A = 1 if leap, else 0 (Z if not leap)
__time_leap::
        ld      a,l
        and     #3
        jr      nz,leap_no
        ld      a,#1
        ret
leap_no:
        xor     a
        ret

        ; __gm_diy: BC = days in __gm_year (366 if leap else 365)
__gm_diy:
        ld      hl,(__gm_year)
        call    __time_leap
        ld      bc,#365
        or      a
        ret     z
        inc     bc
        ret

        ; _gmtime_r / _localtime_r  (local == UTC)
        ; inputs:  HL = const time_t *timer, DE = struct tm *result
        ; outputs: DE = result
_localtime_r::
_gmtime_r::
        push    de                      ; save result ptr
        ld      a,(hl)
        ld      (__gm_secs),a
        inc     hl
        ld      a,(hl)
        ld      (__gm_secs + 1),a
        inc     hl
        ld      a,(hl)
        ld      (__gm_secs + 2),a
        inc     hl
        ld      a,(hl)
        ld      (__gm_secs + 3),a
        ; days = secs / 86400 (toward zero)
        ld      de,(__gm_secs)
        ld      hl,(__gm_secs + 2)
        ld      bc,#0x0001
        push    bc
        ld      bc,#0x5180
        push    bc
        call    __divslong
        pop     bc
        pop     bc
        ld      (__gm_days),de
        ; rem = secs % 86400 (sign of secs)
        ld      de,(__gm_secs)
        ld      hl,(__gm_secs + 2)
        ld      bc,#0x0001
        push    bc
        ld      bc,#0x5180
        push    bc
        call    __modslong
        pop     bc
        pop     bc
        bit     7,h                     ; rem < 0 ?
        jr      z,gm_rem_pos
        ld      a,e
        add     a,#0x80
        ld      e,a
        ld      a,d
        adc     a,#0x51
        ld      d,a
        ld      a,l
        adc     a,#0x01
        ld      l,a
        ld      a,h
        adc     a,#0x00
        ld      h,a
        ld      bc,(__gm_days)
        dec     bc
        ld      (__gm_days),bc
gm_rem_pos:
        ld      (__gm_secs),de          ; seconds-of-day (0..86399)
        ld      (__gm_secs + 2),hl
        pop     ix                      ; IX = result
        push    ix
        ; hour = sod / 3600
        ld      de,(__gm_secs)
        ld      hl,(__gm_secs + 2)
        ld      bc,#0
        push    bc
        ld      bc,#3600
        push    bc
        call    __divulong
        pop     bc
        pop     bc
        ld      4(ix),e
        ld      5(ix),d
        ; sod %= 3600
        ld      de,(__gm_secs)
        ld      hl,(__gm_secs + 2)
        ld      bc,#0
        push    bc
        ld      bc,#3600
        push    bc
        call    __modulong
        pop     bc
        pop     bc                      ; DE = rem (0..3599)
        ex      de,hl                   ; HL = dividend
        ld      de,#60
        call    __divuint               ; DE = min, HL = sec
        ld      2(ix),e
        ld      3(ix),d
        ld      0(ix),l
        ld      1(ix),h
        ; wday = ((days + 4) mod 7)
        ld      hl,(__gm_days)
        ld      bc,#4
        add     hl,bc
        bit     7,h
        jr      z,gm_wday_pos
        ld      bc,#65534               ; + 7*9362 (mod 7 == 0) to make positive
        add     hl,bc
gm_wday_pos:
        ld      de,#7
        call    __divuint               ; HL = remainder
        ld      12(ix),l
        ld      13(ix),h
        ; year walk
        ld      bc,#1970
        ld      (__gm_year),bc
gm_year_neg:
        ld      hl,(__gm_days)
        bit     7,h
        jr      z,gm_year_pos
        ld      hl,(__gm_year)
        dec     hl
        ld      (__gm_year),hl
        call    __gm_diy
        ld      hl,(__gm_days)
        add     hl,bc
        ld      (__gm_days),hl
        jr      gm_year_neg
gm_year_pos:
        call    __gm_diy                ; BC = days in current year (clobbers HL)
        ld      hl,(__gm_days)
        or      a
        sbc     hl,bc
        jr      c,gm_year_done
        ld      (__gm_days),hl
        ld      hl,(__gm_year)
        inc     hl
        ld      (__gm_year),hl
        jr      gm_year_pos
gm_year_done:
        ld      hl,(__gm_days)          ; day-of-year
        ld      14(ix),l
        ld      15(ix),h
        ld      hl,(__gm_year)
        ld      bc,#1900
        or      a
        sbc     hl,bc
        ld      10(ix),l
        ld      11(ix),h
        ; month walk
        ld      hl,(__gm_year)
        call    __time_leap
        ld      c,a                     ; leap
        ld      b,#0                    ; month
        ld      hl,(__gm_days)          ; day-of-year
gm_mon_loop:
        push    hl
        ld      hl,#__time_mdays
        ld      e,b
        ld      d,#0
        add     hl,de
        ld      a,(hl)
        pop     hl
        ld      e,a                     ; dim
        ld      a,b
        cp      #1
        jr      nz,gm_mon_cmp
        ld      a,c
        or      a
        jr      z,gm_mon_cmp
        ld      e,#29
gm_mon_cmp:
        ld      d,#0
        or      a
        sbc     hl,de
        jr      c,gm_mon_done
        inc     b
        jr      gm_mon_loop
gm_mon_done:
        add     hl,de                   ; restore doy within month
        inc     hl                      ; mday = doy + 1
        ld      6(ix),l
        ld      7(ix),h
        ld      8(ix),b
        ld      9(ix),#0
        xor     a
        ld      16(ix),a
        ld      17(ix),a
        pop     de                      ; result ptr
        ret
