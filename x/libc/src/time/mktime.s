        ; mktime.s
        ;
        ; mktime() for the xcc Z80 libc, in assembly.  Normalizes a struct tm
        ; (local == UTC) and returns the corresponding time_t.  Shares the leap
        ; test and month-length table with gmtime_r, and reuses gmtime_r to
        ; write the normalized fields back.  32-bit arithmetic uses the runtime
        ; long-multiply helper.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module mktime
        .optsdcc -mz80 sdcccall(1)


        .globl  _mktime
        .globl  _gmtime_r
        .globl  __time_leap
        .globl  __time_mdays
        .globl  __mullong

MK_SECS_LO  .equ -11       ; low 16 bits of the accumulating time_t
MK_SECS_HI  .equ -9        ; high 16 bits of the accumulating time_t
MK_DAYS     .equ -7        ; signed day accumulator
MK_YEAR     .equ -5        ; normalized full year
MK_ITER     .equ -3        ; year iterator for the 1970 walk
MK_MON      .equ -1        ; normalized month in [0,11]

        .area   _CODE

        ; _mktime
        ; inputs:  HL = struct tm *t
        ; outputs: DE:HL = time_t (DE=low16, HL=high16); *t normalized
        ; clobbers: AF, BC, DE, HL, IX, runtime-helper registers
_mktime::
        push    ix
        push    iy
        ld      b,h
        ld      c,l                     ; BC = t
        ld      ix,#0
        add     ix,sp
        ld      hl,#-11
        add     hl,sp
        ld      sp,hl
        push    bc
        pop     iy                      ; IY = t
        ; full year = tm_year + 1900
        ld      l,10(iy)
        ld      h,11(iy)
        ld      bc,#1900
        add     hl,bc
        ld      MK_YEAR(ix),l
        ld      MK_YEAR + 1(ix),h
        ; normalize month into [0,11]
        ld      l,8(iy)
        ld      h,9(iy)
mk_mneg:
        bit     7,h
        jr      z,mk_mpos
        ld      bc,#12
        add     hl,bc
        push    hl
        ld      l,MK_YEAR(ix)
        ld      h,MK_YEAR + 1(ix)
        dec     hl
        ld      MK_YEAR(ix),l
        ld      MK_YEAR + 1(ix),h
        pop     hl
        jr      mk_mneg
mk_mpos:
        ld      a,h
        or      a
        jr      nz,mk_msub
        ld      a,l
        cp      #12
        jr      c,mk_mdone
mk_msub:
        ld      bc,#-12
        add     hl,bc
        push    hl
        ld      l,MK_YEAR(ix)
        ld      h,MK_YEAR + 1(ix)
        inc     hl
        ld      MK_YEAR(ix),l
        ld      MK_YEAR + 1(ix),h
        pop     hl
        jr      mk_mpos
mk_mdone:
        ld      a,l
        ld      MK_MON(ix),a
        ; --- day count from 1970 to (year, month, mday) ---
        ld      hl,#1970
        ld      MK_ITER(ix),l
        ld      MK_ITER + 1(ix),h
        ld      hl,#0
        ld      MK_DAYS(ix),l
        ld      MK_DAYS + 1(ix),h
mk_yloop:
        ld      l,MK_ITER(ix)
        ld      h,MK_ITER + 1(ix)
        ld      e,MK_YEAR(ix)
        ld      d,MK_YEAR + 1(ix)
        or      a
        sbc     hl,de
        jr      z,mk_ydone
        jr      c,mk_yup
        ; iter > target: iter--; days -= diy(iter)
        ld      l,MK_ITER(ix)
        ld      h,MK_ITER + 1(ix)
        dec     hl
        ld      MK_ITER(ix),l
        ld      MK_ITER + 1(ix),h
        call    mk_diy_iter
        ld      l,MK_DAYS(ix)
        ld      h,MK_DAYS + 1(ix)
        or      a
        sbc     hl,bc
        ld      MK_DAYS(ix),l
        ld      MK_DAYS + 1(ix),h
        jr      mk_yloop
mk_yup:
        call    mk_diy_iter             ; BC = diy(iter) (clobbers HL)
        ld      l,MK_DAYS(ix)
        ld      h,MK_DAYS + 1(ix)
        add     hl,bc
        ld      MK_DAYS(ix),l
        ld      MK_DAYS + 1(ix),h
        ld      l,MK_ITER(ix)
        ld      h,MK_ITER + 1(ix)
        inc     hl
        ld      MK_ITER(ix),l
        ld      MK_ITER + 1(ix),h
        jr      mk_yloop
mk_ydone:
        ; add month offsets within the target year
        ld      l,MK_YEAR(ix)
        ld      h,MK_YEAR + 1(ix)
        call    __time_leap
        ld      c,a                     ; C = leap
        ld      a,MK_MON(ix)
        ld      b,a                     ; B = months to add
        ld      l,MK_DAYS(ix)
        ld      h,MK_DAYS + 1(ix)
mk_moff:
        ld      a,b
        or      a
        jr      z,mk_moffdone
        dec     b
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
        jr      nz,mk_moffadd
        ld      a,c
        or      a
        jr      z,mk_moffadd
        ld      e,#29
mk_moffadd:
        ld      d,#0
        add     hl,de
        jr      mk_moff
mk_moffdone:
        ; + (mday - 1)
        ld      e,6(iy)
        ld      d,7(iy)
        add     hl,de
        dec     hl
        ld      MK_DAYS(ix),l
        ld      MK_DAYS + 1(ix),h
        ; --- seconds = ((days*24 + hour)*60 + min)*60 + sec ---
        ; secs = sign-extend(days)
        ld      l,MK_DAYS(ix)
        ld      h,MK_DAYS + 1(ix)
        ld      a,h
        rla
        sbc     a,a
        ld      d,a
        ld      e,a
        ld      MK_SECS_LO(ix),l
        ld      MK_SECS_LO + 1(ix),h
        ld      MK_SECS_HI(ix),e
        ld      MK_SECS_HI + 1(ix),d
        ld      l,4(iy)
        ld      h,5(iy)
        ld      a,#24
        call    mk_muladd
        ld      l,2(iy)
        ld      h,3(iy)
        ld      a,#60
        call    mk_muladd
        ld      l,0(iy)
        ld      h,1(iy)
        ld      a,#60
        call    mk_muladd
        ; normalize *t from the result seconds, then return them
        push    ix
        pop     hl
        ld      de,#MK_SECS_LO
        add     hl,de                   ; HL = &secs
        push    iy
        pop     de                      ; DE = t (struct tm *)
        call    _gmtime_r
        ld      e,MK_SECS_LO(ix)
        ld      d,MK_SECS_LO + 1(ix)
        ld      l,MK_SECS_HI(ix)
        ld      h,MK_SECS_HI + 1(ix)
        ld      sp,ix
        pop     iy
        pop     ix
        ret

        ; mk_diy_iter: BC = days in __mk_iter (365/366)
mk_diy_iter:
        ld      l,MK_ITER(ix)
        ld      h,MK_ITER + 1(ix)
        call    __time_leap
        ld      bc,#365
        or      a
        ret     z
        inc     bc
        ret

        ; mk_muladd: __mk_secs = __mk_secs * A + sign_extend(HL)
        ; A = small multiplier, HL = signed 16-bit term
        ; clobbers AF, BC, DE, HL
mk_muladd:
        push    hl                      ; save term
        ld      e,MK_SECS_LO(ix)
        ld      d,MK_SECS_LO + 1(ix)
        ld      l,MK_SECS_HI(ix)
        ld      h,MK_SECS_HI + 1(ix)
        ld      bc,#0
        push    bc                      ; multiplier high word
        ld      c,a
        push    bc                      ; multiplier low word = A
        call    __mullong
        pop     bc
        pop     bc
        ld      MK_SECS_LO(ix),e
        ld      MK_SECS_LO + 1(ix),d
        ld      MK_SECS_HI(ix),l
        ld      MK_SECS_HI + 1(ix),h
        pop     hl                      ; HL = term
        ld      a,h
        rla
        sbc     a,a                     ; A = sign byte of term
        ld      c,l
        ld      b,h                     ; BC = term low 16
        ld      d,a                     ; D = sign byte
        ld      l,MK_SECS_LO(ix)
        ld      h,MK_SECS_LO + 1(ix)
        add     hl,bc
        ld      MK_SECS_LO(ix),l
        ld      MK_SECS_LO + 1(ix),h
        ld      l,MK_SECS_HI(ix)
        ld      h,MK_SECS_HI + 1(ix)
        ld      a,l
        adc     a,d
        ld      l,a
        ld      a,h
        adc     a,d
        ld      h,a
        ld      MK_SECS_HI(ix),l
        ld      MK_SECS_HI + 1(ix),h
        ret
