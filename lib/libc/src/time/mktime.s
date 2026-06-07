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

        .area   _DATA
__mk_secs:  .ds 4          ; mktime result seconds
__mk_days:  .ds 2          ; mktime day accumulator
__mk_year:  .ds 2          ; mktime target full year
__mk_iter:  .ds 2          ; mktime year iterator
__mk_mon:   .ds 1          ; mktime normalized month

        .area   _CODE

        ; _mktime
        ; inputs:  HL = struct tm *t
        ; outputs: DE:HL = time_t (DE=low16, HL=high16); *t normalized
        ; clobbers: AF, BC, DE, HL, IX, runtime-helper registers
_mktime::
        push    hl
        pop     ix                      ; IX = t
        ; full year = tm_year + 1900
        ld      l,10(ix)
        ld      h,11(ix)
        ld      bc,#1900
        add     hl,bc
        ld      (__mk_year),hl
        ; normalize month into [0,11]
        ld      l,8(ix)
        ld      h,9(ix)
mk_mneg:
        bit     7,h
        jr      z,mk_mpos
        ld      bc,#12
        add     hl,bc
        push    hl
        ld      hl,(__mk_year)
        dec     hl
        ld      (__mk_year),hl
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
        ld      hl,(__mk_year)
        inc     hl
        ld      (__mk_year),hl
        pop     hl
        jr      mk_mpos
mk_mdone:
        ld      a,l
        ld      (__mk_mon),a
        ; --- day count from 1970 to (year, month, mday) ---
        ld      hl,#1970
        ld      (__mk_iter),hl
        ld      hl,#0
        ld      (__mk_days),hl
mk_yloop:
        ld      hl,(__mk_iter)
        ld      de,(__mk_year)
        or      a
        sbc     hl,de
        jr      z,mk_ydone
        jr      c,mk_yup
        ; iter > target: iter--; days -= diy(iter)
        ld      hl,(__mk_iter)
        dec     hl
        ld      (__mk_iter),hl
        call    mk_diy_iter
        ld      hl,(__mk_days)
        or      a
        sbc     hl,bc
        ld      (__mk_days),hl
        jr      mk_yloop
mk_yup:
        call    mk_diy_iter             ; BC = diy(iter) (clobbers HL)
        ld      hl,(__mk_days)
        add     hl,bc
        ld      (__mk_days),hl
        ld      hl,(__mk_iter)
        inc     hl
        ld      (__mk_iter),hl
        jr      mk_yloop
mk_ydone:
        ; add month offsets within the target year
        ld      hl,(__mk_year)
        call    __time_leap
        ld      c,a                     ; C = leap
        ld      a,(__mk_mon)
        ld      b,a                     ; B = months to add
        ld      hl,(__mk_days)
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
        ld      e,6(ix)
        ld      d,7(ix)
        add     hl,de
        dec     hl
        ld      (__mk_days),hl
        ; --- seconds = ((days*24 + hour)*60 + min)*60 + sec ---
        ; secs = sign-extend(days)
        ld      hl,(__mk_days)
        ld      a,h
        rla
        sbc     a,a
        ld      d,a
        ld      e,a
        ld      (__mk_secs),hl
        ld      (__mk_secs + 2),de
        ld      l,4(ix)
        ld      h,5(ix)
        ld      a,#24
        call    mk_muladd
        ld      l,2(ix)
        ld      h,3(ix)
        ld      a,#60
        call    mk_muladd
        ld      l,0(ix)
        ld      h,1(ix)
        ld      a,#60
        call    mk_muladd
        ; normalize *t from the result seconds, then return them
        ld      hl,#__mk_secs
        push    ix
        pop     de                      ; DE = t (struct tm *)
        call    _gmtime_r
        ld      de,(__mk_secs)
        ld      hl,(__mk_secs + 2)
        ret

        ; mk_diy_iter: BC = days in __mk_iter (365/366)
mk_diy_iter:
        ld      hl,(__mk_iter)
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
        ld      de,(__mk_secs)
        ld      hl,(__mk_secs + 2)
        ld      bc,#0
        push    bc                      ; multiplier high word
        ld      c,a
        push    bc                      ; multiplier low word = A
        call    __mullong
        pop     bc
        pop     bc
        ld      (__mk_secs),de
        ld      (__mk_secs + 2),hl
        pop     hl                      ; HL = term
        ld      a,h
        rla
        sbc     a,a                     ; A = sign byte of term
        ld      c,l
        ld      b,h                     ; BC = term low 16
        ld      d,a                     ; D = sign byte
        ld      hl,(__mk_secs)
        add     hl,bc
        ld      (__mk_secs),hl
        ld      hl,(__mk_secs + 2)
        ld      a,l
        adc     a,d
        ld      l,a
        ld      a,h
        adc     a,d
        ld      h,a
        ld      (__mk_secs + 2),hl
        ret
