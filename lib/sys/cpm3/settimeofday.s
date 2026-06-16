        ;; settimeofday.s  (sys backend: CP/M 3)
        ;;
        ;; Convert Unix-style seconds to the CP/M 3 T_SET clock record.

        .module settimeofday
        .optsdcc -mz80 sdcccall(1)

        .globl  _settimeofday
        .globl  __divulong
        .globl  __modulong
        .globl  __divuint

T_SET           .equ 104
BDOS            .equ 5
CPM_EPOCH_ADJ   .equ 2921

        .area   _DATA
__cpm3_stod_ct:
        .ds     4

        .area   _CODE

__cpm3_set_bin2bcd:
        ld      e,a
        xor     a
__cpm3_set_bin2bcd_tens:
        ld      d,a
        ld      a,e
        cp      #10
        jr      c,__cpm3_set_bin2bcd_done
        sub     #10
        ld      e,a
        ld      a,d
        add     a,#0x10
        jr      __cpm3_set_bin2bcd_tens
__cpm3_set_bin2bcd_done:
        ld      a,d
        add     a,e
        ret

_settimeofday::
        ld      a,h
        or      l
        jp      z,__cpm3_settimeofday_fail
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        inc     hl
        ld      l,(hl)
        inc     hl
        ld      h,(hl)
        ld      bc,#0x0001
        push    bc
        ld      bc,#0x5180
        push    bc
        call    __divulong
        pop     bc
        pop     bc
        ld      a,h
        or      l
        jr      nz,__cpm3_settimeofday_fail
        ld      a,e
        sub     #0x69
        ld      (__cpm3_stod_ct),a
        ld      a,d
        sbc     a,#0x0b
        ld      (__cpm3_stod_ct + 1),a
        jr      c,__cpm3_settimeofday_fail
        ld      a,(__cpm3_stod_ct)
        inc     a
        ld      (__cpm3_stod_ct),a
        jr      nz,__cpm3_settimeofday_days_done
        ld      a,(__cpm3_stod_ct + 1)
        inc     a
        ld      (__cpm3_stod_ct + 1),a
__cpm3_settimeofday_days_done:
        push    bc
        ld      bc,#0x0001
        push    bc
        ld      bc,#0x5180
        push    bc
        call    __modulong
        pop     bc
        pop     bc
        ld      bc,#0x0000
        push    bc
        ld      bc,#3600
        push    bc
        call    __divulong
        pop     bc
        pop     bc
        ld      a,e
        call    __cpm3_set_bin2bcd
        ld      (__cpm3_stod_ct + 2),a
        ld      bc,#0x0000
        push    bc
        ld      bc,#3600
        push    bc
        call    __modulong
        pop     bc
        pop     bc
        ex      de,hl
        ld      de,#60
        call    __divuint
        ld      a,e
        call    __cpm3_set_bin2bcd
        ld      (__cpm3_stod_ct + 3),a
        push    ix
        push    iy
        ld      de,#__cpm3_stod_ct
        ld      c,#T_SET
        call    BDOS
        pop     iy
        pop     ix
        ld      de,#0x0000
        ret
__cpm3_settimeofday_fail:
        ld      de,#0xffff
        ret
