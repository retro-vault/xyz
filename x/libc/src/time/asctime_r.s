        ; asctime_r.s
        ;
        ; asctime_r() for the xcc Z80 libc, hand-written in assembly.  Produces
        ; the fixed 26-byte "Www Mmm dd hh:mm:ss yyyy\n\0" form.
        ;
        ; struct tm field offsets (int == 2 bytes):
        ;   0 sec   2 min   4 hour   6 mday   8 mon   10 year   12 wday
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module asctime_r
        .optsdcc -mz80 sdcccall(1)


        .globl  _asctime_r

        .area   _CODE

__wday_abbr:
        .ascii  "SunMonTueWedThuFriSat"
__mon_abbr:
        .ascii  "JanFebMarAprMayJunJulAugSepOctNovDec"

        ; _asctime_r
        ; inputs:  HL = const struct tm *t, DE = char *buf
        ; outputs: DE = buf
        ; clobbers: AF, BC, HL, IX
_asctime_r::
        push    ix
        push    de                      ; save buf for the return value
        push    hl
        pop     ix                      ; IX = t
        ; --- weekday abbreviation ---
        ld      a,12(ix)
        cp      #7
        jr      c,asctime_wok
        xor     a
asctime_wok:
        ld      hl,#__wday_abbr
        call    asctime_index3          ; HL = &table[a*3]
        ld      bc,#3
        ldir
        ld      a,#0x20
        ld      (de),a
        inc     de
        ; --- month abbreviation ---
        ld      a,8(ix)
        cp      #12
        jr      c,asctime_mok
        xor     a
asctime_mok:
        ld      hl,#__mon_abbr
        call    asctime_index3
        ld      bc,#3
        ldir
        ld      a,#0x20
        ld      (de),a
        inc     de
        ; --- day (space padded), hour:min:sec (zero padded) ---
        ld      a,6(ix)
        ld      c,#0x20
        call    asctime_put2
        ld      a,#0x20
        ld      (de),a
        inc     de
        ld      a,4(ix)
        ld      c,#0x30
        call    asctime_put2
        ld      a,#0x3a
        ld      (de),a
        inc     de
        ld      a,2(ix)
        ld      c,#0x30
        call    asctime_put2
        ld      a,#0x3a
        ld      (de),a
        inc     de
        ld      a,0(ix)
        ld      c,#0x30
        call    asctime_put2
        ld      a,#0x20
        ld      (de),a
        inc     de
        ; --- year = tm_year + 1900 (four digits) ---
        ld      l,10(ix)
        ld      h,11(ix)
        ld      bc,#1900
        add     hl,bc
        call    asctime_put4
        ld      a,#0x0a                 ; '\n'
        ld      (de),a
        inc     de
        xor     a                       ; '\0'
        ld      (de),a
        pop     de                      ; DE = buf (return value)
        pop     ix
        ret

        ; asctime_index3: HL = HL + 3*A  (preserves DE, the output pointer)
        ; clobbers: BC
asctime_index3:
        ld      c,a
        ld      b,#0                    ; BC = A
        add     hl,bc                   ; base + A
        add     hl,bc                   ; base + 2A
        add     hl,bc                   ; base + 3A
        ret

        ; asctime_put2: write A (0..99) as two digits with pad char C.
        ; advances DE.  clobbers AF, B, L.
asctime_put2:
        ld      b,#0
asctime_put2_t:
        cp      #10
        jr      c,asctime_put2_done
        sub     #10
        inc     b
        jr      asctime_put2_t
asctime_put2_done:
        ld      l,a                     ; L = ones
        ld      a,b
        or      a
        jr      nz,asctime_put2_tens
        ld      a,c                     ; tens == 0: emit pad
        ld      (de),a
        inc     de
        jr      asctime_put2_ones
asctime_put2_tens:
        add     a,#0x30
        ld      (de),a
        inc     de
asctime_put2_ones:
        ld      a,l
        add     a,#0x30
        ld      (de),a
        inc     de
        ret

        ; asctime_put4: write HL (0..9999) as four digits to (DE).
        ; clobbers AF, BC, HL.
asctime_put4:
        ld      bc,#1000
        call    asctime_div
        add     a,#0x30
        ld      (de),a
        inc     de
        ld      bc,#100
        call    asctime_div
        add     a,#0x30
        ld      (de),a
        inc     de
        ld      bc,#10
        call    asctime_div
        add     a,#0x30
        ld      (de),a
        inc     de
        ld      a,l                     ; remainder < 10
        add     a,#0x30
        ld      (de),a
        inc     de
        ret

        ; asctime_div: A = HL / BC (0..9), HL = HL % BC.  clobbers AF.
asctime_div:
        ld      a,#0xff
asctime_div_loop:
        inc     a
        or      a                       ; clear carry
        sbc     hl,bc
        jr      nc,asctime_div_loop
        add     hl,bc                   ; restore the last subtraction
        ret
