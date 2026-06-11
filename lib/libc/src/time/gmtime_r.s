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

        ; __gm_diy: HL = full year, BC = days in year (366 if leap else 365)
__gm_diy:
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
        push    ix
        push    iy
        push    de                      ; save result ptr
        ld      b,h
        ld      c,l                     ; BC = timer
        ld      ix,#0
        add     ix,sp
        ld      hl,#-8
        add     hl,sp
        ld      sp,hl
        ld      e,0(ix)
        ld      d,1(ix)
        push    de
        pop     iy                      ; IY = result
        ld      h,b
        ld      l,c
        ld      a,(hl)
        ld      -8(ix),a
        inc     hl
        ld      a,(hl)
        ld      -7(ix),a
        inc     hl
        ld      a,(hl)
        ld      -6(ix),a
        inc     hl
        ld      a,(hl)
        ld      -5(ix),a
        ; days = secs / 86400 (toward zero)
        ld      e,-8(ix)
        ld      d,-7(ix)
        ld      l,-6(ix)
        ld      h,-5(ix)
        ld      bc,#0x0001
        push    bc
        ld      bc,#0x5180
        push    bc
        call    __divslong
        pop     bc
        pop     bc
        ld      -4(ix),e
        ld      -3(ix),d
        ; rem = secs % 86400 (sign of secs)
        ld      e,-8(ix)
        ld      d,-7(ix)
        ld      l,-6(ix)
        ld      h,-5(ix)
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
        ld      c,-4(ix)
        ld      b,-3(ix)
        dec     bc
        ld      -4(ix),c
        ld      -3(ix),b
gm_rem_pos:
        ld      -8(ix),e                ; seconds-of-day (0..86399)
        ld      -7(ix),d
        ld      -6(ix),l
        ld      -5(ix),h
        ; hour = sod / 3600
        ld      e,-8(ix)
        ld      d,-7(ix)
        ld      l,-6(ix)
        ld      h,-5(ix)
        ld      bc,#0
        push    bc
        ld      bc,#3600
        push    bc
        call    __divulong
        pop     bc
        pop     bc
        ld      4(iy),e
        ld      5(iy),d
        ; sod %= 3600
        ld      e,-8(ix)
        ld      d,-7(ix)
        ld      l,-6(ix)
        ld      h,-5(ix)
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
        ld      2(iy),e
        ld      3(iy),d
        ld      0(iy),l
        ld      1(iy),h
        ; wday = ((days + 4) mod 7)
        ld      l,-4(ix)
        ld      h,-3(ix)
        ld      bc,#4
        add     hl,bc
        bit     7,h
        jr      z,gm_wday_pos
        ld      bc,#65534               ; + 7*9362 (mod 7 == 0) to make positive
        add     hl,bc
gm_wday_pos:
        ld      de,#7
        call    __divuint               ; HL = remainder
        ld      12(iy),l
        ld      13(iy),h
        ; year walk
        ld      bc,#1970
        ld      -2(ix),c
        ld      -1(ix),b
gm_year_neg:
        ld      l,-4(ix)
        ld      h,-3(ix)
        bit     7,h
        jr      z,gm_year_pos
        ld      l,-2(ix)
        ld      h,-1(ix)
        dec     hl
        ld      -2(ix),l
        ld      -1(ix),h
        call    __gm_diy
        ld      l,-4(ix)
        ld      h,-3(ix)
        add     hl,bc
        ld      -4(ix),l
        ld      -3(ix),h
        jr      gm_year_neg
gm_year_pos:
        ld      l,-2(ix)
        ld      h,-1(ix)
        call    __gm_diy                ; BC = days in current year (clobbers HL)
        ld      l,-4(ix)
        ld      h,-3(ix)
        or      a
        sbc     hl,bc
        jr      c,gm_year_done
        ld      -4(ix),l
        ld      -3(ix),h
        ld      l,-2(ix)
        ld      h,-1(ix)
        inc     hl
        ld      -2(ix),l
        ld      -1(ix),h
        jr      gm_year_pos
gm_year_done:
        ld      l,-4(ix)                ; day-of-year
        ld      h,-3(ix)
        ld      14(iy),l
        ld      15(iy),h
        ld      l,-2(ix)
        ld      h,-1(ix)
        ld      bc,#1900
        or      a
        sbc     hl,bc
        ld      10(iy),l
        ld      11(iy),h
        ; month walk
        ld      l,-2(ix)
        ld      h,-1(ix)
        call    __time_leap
        ld      c,a                     ; leap
        ld      b,#0                    ; month
        ld      l,-4(ix)
        ld      h,-3(ix)                ; day-of-year
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
        ld      6(iy),l
        ld      7(iy),h
        ld      8(iy),b
        ld      9(iy),#0
        xor     a
        ld      16(iy),a
        ld      17(iy),a
        ld      sp,ix
        pop     de                      ; result ptr
        pop     iy
        pop     ix
        ret
