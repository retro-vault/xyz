        ; tmstrf.s
        ;
        ; strftime for the xcc Z80 libc, in assembly.  Walks the format string
        ; and emits each field, honouring the destination size (one byte is
        ; reserved for the terminating NUL).  Returns the number of characters
        ; written, or 0 if the result did not fit.
        ;
        ; The formatter keeps its output cursor, format cursor, remaining
        ; capacity, and per-call flags in an IX-framed local block so the
        ; routine stays reentrant.  IY carries the caller's struct tm pointer.
        ;
        ; struct tm offsets: 0 sec 2 min 4 hour 6 mday 8 mon 10 year 12 wday 14 yday
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module strftime
        .optsdcc -mz80 sdcccall(1)


        .globl  _strftime

SF_OUT      .equ -10       ; current output pointer
SF_START    .equ -8        ; original output base
SF_REM      .equ -6        ; bytes remaining (excludes the NUL slot)
SF_FMT      .equ -4        ; current format pointer
SF_LEAD     .equ -2        ; leading-zero suppression flag for %Y
SF_TRUNC    .equ -1        ; set when output was truncated

        .area   _CODE

__sf_wabbr: .ascii  "SunMonTueWedThuFriSat"
__sf_mabbr: .ascii  "JanFebMarAprMayJunJulAugSepOctNovDec"

__sf_wd0:   .asciz  "Sunday"
__sf_wd1:   .asciz  "Monday"
__sf_wd2:   .asciz  "Tuesday"
__sf_wd3:   .asciz  "Wednesday"
__sf_wd4:   .asciz  "Thursday"
__sf_wd5:   .asciz  "Friday"
__sf_wd6:   .asciz  "Saturday"
__sf_wptr:  .dw __sf_wd0,__sf_wd1,__sf_wd2,__sf_wd3,__sf_wd4,__sf_wd5,__sf_wd6

__sf_mo0:   .asciz  "January"
__sf_mo1:   .asciz  "February"
__sf_mo2:   .asciz  "March"
__sf_mo3:   .asciz  "April"
__sf_mo4:   .asciz  "May"
__sf_mo5:   .asciz  "June"
__sf_mo6:   .asciz  "July"
__sf_mo7:   .asciz  "August"
__sf_mo8:   .asciz  "September"
__sf_mo9:   .asciz  "October"
__sf_mo10:  .asciz  "November"
__sf_mo11:  .asciz  "December"
__sf_mptr:  .dw __sf_mo0,__sf_mo1,__sf_mo2,__sf_mo3,__sf_mo4,__sf_mo5
            .dw __sf_mo6,__sf_mo7,__sf_mo8,__sf_mo9,__sf_mo10,__sf_mo11

        ; _strftime
        ; inputs:  HL = char *s, DE = size_t maxsize, 4(ix)=fmt, 6(ix)=tm
        ; outputs: DE = number of bytes written (excl. NUL), or 0 if truncated
        ; clobbers: AF, BC, DE, HL, IX
_strftime::
        push    ix
        push    iy
        ld      ix,#0
        add     ix,sp
        ld      b,h
        ld      c,l                     ; BC = destination buffer
        ld      hl,#-10
        add     hl,sp
        ld      sp,hl
        ld      SF_OUT(ix),c
        ld      SF_OUT + 1(ix),b
        ld      SF_START(ix),c
        ld      SF_START + 1(ix),b
        xor     a
        ld      SF_TRUNC(ix),a
        ld      a,d
        or      e
        jp      z,sf_ret0_early         ; maxsize == 0: return 0, write nothing
        dec     de                      ; reserve the NUL byte
        ld      SF_REM(ix),e
        ld      SF_REM + 1(ix),d
        ld      l,6(ix)
        ld      h,7(ix)
        ld      SF_FMT(ix),l
        ld      SF_FMT + 1(ix),h
        ld      e,8(ix)
        ld      d,9(ix)
        push    de
        pop     iy                      ; IY = tm
sf_loop:
        ld      l,SF_FMT(ix)
        ld      h,SF_FMT + 1(ix)
        ld      a,(hl)
        or      a
        jp      z,sf_done
        inc     hl
        ld      SF_FMT(ix),l
        ld      SF_FMT + 1(ix),h
        cp      #0x25                   ; '%'
        jp      z,sf_spec
        call    __sf_emit
        jr      sf_loop
sf_spec:
        ld      l,SF_FMT(ix)
        ld      h,SF_FMT + 1(ix)
        ld      a,(hl)
        or      a
        jp      z,sf_done
        inc     hl
        ld      SF_FMT(ix),l
        ld      SF_FMT + 1(ix),h
        call    sf_dispatch
        jr      sf_loop
sf_done:
        ; NUL-terminate (the slot was reserved)
        ld      l,SF_OUT(ix)
        ld      h,SF_OUT + 1(ix)
        ld      (hl),#0
        ; truncated -> return 0 (C semantics)
        ld      a,SF_TRUNC(ix)
        or      a
        jr      nz,sf_ret0
        ; length = out - start
        ld      l,SF_OUT(ix)
        ld      h,SF_OUT + 1(ix)
        ld      e,SF_START(ix)
        ld      d,SF_START + 1(ix)
        or      a
        sbc     hl,de
        ex      de,hl                   ; DE = length
        ld      sp,ix
        pop     iy
        pop     ix
        ret
sf_ret0_early:
sf_ret0:
        ld      de,#0
        ld      sp,ix
        pop     iy
        pop     ix
        ret

        ; __sf_emit: emit the char in A if room remains
        ; clobbers AF; preserves BC, DE, HL, IX
__sf_emit::
        push    hl
        push    de
        ld      l,SF_REM(ix)
        ld      h,SF_REM + 1(ix)
        ld      d,a
        ld      a,h
        or      l
        jp      z,sf_emit_full          ; no room -> mark truncation
        dec     hl
        ld      SF_REM(ix),l
        ld      SF_REM + 1(ix),h
        ld      l,SF_OUT(ix)
        ld      h,SF_OUT + 1(ix)
        ld      (hl),d
        inc     hl
        ld      SF_OUT(ix),l
        ld      SF_OUT + 1(ix),h
        pop     de
        pop     hl
        ret
sf_emit_full:
        ld      a,#1
        ld      SF_TRUNC(ix),a
        pop     de
        pop     hl
        ret

        ; sf_emit2z: emit A (0..99) as two zero-padded digits
sf_emit2z:
        ld      c,#0x30
        jr      sf_emit2
        ; sf_emit2s: two digits, space padded
sf_emit2s:
        ld      c,#0x20
sf_emit2:
        ld      b,#0
sf_e2_t:
        cp      #10
        jr      c,sf_e2_done
        sub     #10
        inc     b
        jr      sf_e2_t
sf_e2_done:
        push    af                      ; ones
        ld      a,b
        or      a
        jr      nz,sf_e2_tens
        ld      a,c                     ; tens==0 -> pad
        call    __sf_emit
        jr      sf_e2_ones
sf_e2_tens:
        add     a,#0x30
        call    __sf_emit
sf_e2_ones:
        pop     af
        add     a,#0x30
        jp      __sf_emit

        ; sf_num: emit HL (unsigned, 0..9999) as decimal, no leading zeros
sf_num:
        ld      bc,#1000
        call    sf_digit_lead
        ld      bc,#100
        call    sf_digit_lead
        ld      bc,#10
        call    sf_digit_lead
        ld      a,l
        add     a,#0x30
        jp      __sf_emit
        ; sf_digit_lead: emit HL/BC digit unless it is a leading zero;
        ;   HL = HL % BC.  Uses __sf_lead flag to suppress leading zeros.
sf_digit_lead:
        call    sf_div
        ; A = digit; if digit==0 and nothing emitted yet, skip
        or      a
        jr      nz,sf_dl_emit
        ld      a,SF_LEAD(ix)
        or      a
        ret     z                       ; still leading -> skip the zero
        xor     a                       ; emit '0'
sf_dl_emit:
        push    af
        ld      a,#1
        ld      SF_LEAD(ix),a           ; mark that we have emitted a digit
        pop     af
        add     a,#0x30
        jp      __sf_emit

        ; sf_3z: emit HL (0..999) as three zero-padded digits
sf_3z:
        ld      bc,#100
        call    sf_div
        add     a,#0x30
        call    __sf_emit
        ld      bc,#10
        call    sf_div
        add     a,#0x30
        call    __sf_emit
        ld      a,l
        add     a,#0x30
        jp      __sf_emit

        ; sf_div: A = HL / BC (0..9), HL = HL % BC
sf_div:
        ld      a,#0xff
sf_div_loop:
        inc     a
        or      a
        sbc     hl,bc
        jr      nc,sf_div_loop
        add     hl,bc
        ret

        ; sf_str: emit the NUL-terminated string at HL
sf_str:
        ld      a,(hl)
        or      a
        ret     z
        inc     hl
        push    hl
        call    __sf_emit
        pop     hl
        jr      sf_str

        ; sf_dispatch: A = conversion specifier; IX = tm
sf_dispatch:
        cp      #0x59                   ; 'Y'
        jp      z,sf_Y
        cp      #0x79                   ; 'y'
        jp      z,sf_y
        cp      #0x43                   ; 'C'
        jp      z,sf_C
        cp      #0x6d                   ; 'm'
        jp      z,sf_m
        cp      #0x64                   ; 'd'
        jp      z,sf_d
        cp      #0x65                   ; 'e'
        jp      z,sf_e
        cp      #0x48                   ; 'H'
        jp      z,sf_H
        cp      #0x49                   ; 'I'
        jp      z,sf_I
        cp      #0x4d                   ; 'M'
        jp      z,sf_M
        cp      #0x53                   ; 'S'
        jp      z,sf_S
        cp      #0x6a                   ; 'j'
        jp      z,sf_j
        cp      #0x77                   ; 'w'
        jp      z,sf_w
        cp      #0x75                   ; 'u'
        jp      z,sf_u
        cp      #0x70                   ; 'p'
        jp      z,sf_p
        cp      #0x41                   ; 'A'
        jp      z,sf_A
        cp      #0x61                   ; 'a'
        jp      z,sf_a
        cp      #0x42                   ; 'B'
        jp      z,sf_B
        cp      #0x62                   ; 'b'
        jp      z,sf_b
        cp      #0x68                   ; 'h'
        jp      z,sf_b
        cp      #0x6e                   ; 'n'
        jp      z,sf_n
        cp      #0x74                   ; 't'
        jp      z,sf_t
        cp      #0x25                   ; '%'
        jp      z,sf_pct
        cp      #0x52                   ; 'R'
        jp      z,sf_R
        cp      #0x54                   ; 'T'
        jp      z,sf_T
        cp      #0x44                   ; 'D'
        jp      z,sf_D
        cp      #0x46                   ; 'F'
        jp      z,sf_F
        ; unknown: emit '%' then the char verbatim
        push    af
        ld      a,#0x25
        call    __sf_emit
        pop     af
        jp      __sf_emit

sf_year_hl:                             ; HL = tm_year + 1900
        ld      l,10(iy)
        ld      h,11(iy)
        ld      bc,#1900
        add     hl,bc
        ret

sf_Y:
        call    sf_year_hl
        xor     a
        ld      SF_LEAD(ix),a
        jp      sf_num
sf_y:
        call    sf_year_hl
        ld      bc,#100
        call    sf_div
        ld      a,l
        jp      sf_emit2z
sf_C:
        call    sf_year_hl
        ld      bc,#100
        call    sf_div                  ; A = year/100
        jp      sf_emit2z
sf_m:
        ld      a,8(iy)
        inc     a
        jp      sf_emit2z
sf_d:
        ld      a,6(iy)
        jp      sf_emit2z
sf_e:
        ld      a,6(iy)
        jp      sf_emit2s
sf_H:
        ld      a,4(iy)
        jp      sf_emit2z
sf_I:
        ld      a,4(iy)
        cp      #13
        jr      c,sf_I_chk
        sub     #12
        jr      sf_I_emit
sf_I_chk:
        or      a
        jr      nz,sf_I_emit
        ld      a,#12
sf_I_emit:
        cp      #0
        jr      nz,sf_I_ok
        ld      a,#12
sf_I_ok:
        jp      sf_emit2z
sf_M:
        ld      a,2(iy)
        jp      sf_emit2z
sf_S:
        ld      a,0(iy)
        jp      sf_emit2z
sf_j:
        ld      l,14(iy)
        ld      h,15(iy)
        inc     hl
        jp      sf_3z
sf_w:
        ld      a,12(iy)
        add     a,#0x30
        jp      __sf_emit
sf_u:
        ld      a,12(iy)
        or      a
        jr      nz,sf_u_e
        ld      a,#7
sf_u_e:
        add     a,#0x30
        jp      __sf_emit
sf_p:
        ld      a,4(iy)
        cp      #12
        jr      c,sf_p_am
        ld      a,#0x50                 ; 'P'
        call    __sf_emit
        ld      a,#0x4d                 ; 'M'
        jp      __sf_emit
sf_p_am:
        ld      a,#0x41                 ; 'A'
        call    __sf_emit
        ld      a,#0x4d
        jp      __sf_emit
sf_A:
        ld      a,12(iy)
        ld      hl,#__sf_wptr
        jr      sf_ptr_str
sf_B:
        ld      a,8(iy)
        ld      hl,#__sf_mptr
sf_ptr_str:
        add     a,a                     ; index*2
        ld      e,a
        ld      d,#0
        add     hl,de
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        ex      de,hl                   ; HL = string ptr
        jp      sf_str
sf_a:
        ld      a,12(iy)
        ld      hl,#__sf_wabbr
        jr      sf_abbr3
sf_b:
        ld      a,8(iy)
        ld      hl,#__sf_mabbr
sf_abbr3:
        ld      c,a
        ld      b,#0
        add     hl,bc
        add     hl,bc
        add     hl,bc                   ; HL = base + 3*index
        ld      a,(hl)
        call    __sf_emit
        inc     hl
        ld      a,(hl)
        call    __sf_emit
        inc     hl
        ld      a,(hl)
        jp      __sf_emit
sf_n:
        ld      a,#0x0a
        jp      __sf_emit
sf_t:
        ld      a,#0x09
        jp      __sf_emit
sf_pct:
        ld      a,#0x25
        jp      __sf_emit
sf_R:
        ld      a,4(iy)
        call    sf_emit2z
        ld      a,#0x3a
        call    __sf_emit
        ld      a,2(iy)
        jp      sf_emit2z
sf_T:
        ld      a,4(iy)
        call    sf_emit2z
        ld      a,#0x3a
        call    __sf_emit
        ld      a,2(iy)
        call    sf_emit2z
        ld      a,#0x3a
        call    __sf_emit
        ld      a,0(iy)
        jp      sf_emit2z
sf_D:
        ld      a,8(iy)
        inc     a
        call    sf_emit2z
        ld      a,#0x2f
        call    __sf_emit
        ld      a,6(iy)
        call    sf_emit2z
        ld      a,#0x2f
        call    __sf_emit
        call    sf_year_hl
        ld      bc,#100
        call    sf_div
        ld      a,l
        jp      sf_emit2z
sf_F:
        call    sf_year_hl
        push    hl
        ld      bc,#100
        call    sf_div                  ; A = century
        call    sf_emit2z
        pop     hl
        ld      bc,#100
        call    sf_div
        ld      a,l                     ; year%100
        call    sf_emit2z
        ld      a,#0x2d
        call    __sf_emit
        ld      a,8(iy)
        inc     a
        call    sf_emit2z
        ld      a,#0x2d
        call    __sf_emit
        ld      a,6(iy)
        jp      sf_emit2z
