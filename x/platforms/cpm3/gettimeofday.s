        ;; gettimeofday.s  (sys backend: CP/M 3)
        ;;
        ;; Convert CP/M 3's BDOS clock record into the libc's Unix-style
        ;; `struct timespec` representation.

        .module gettimeofday
        .optsdcc -mz80 sdcccall(1)

        .globl  _gettimeofday
        .globl  __mullong

        .equ    T_GET,105
        .equ    BDOS,5
        .equ    CPM_EPOCH_ADJ,2921

        .area   _DATA
__cpm3_gtod_ct:
        .ds     4
__cpm3_gtod_acc:
        .ds     4
__cpm3_gtod_sec:
        .db     0

        .area   _CODE

__cpm3_bcd2bin_a:
        ld      e,a
        and     #0x0f
        ld      d,a
        ld      a,e
        and     #0xf0
        rrca
        rrca
        rrca
        rrca
        ld      e,a
        add     a,a
        ld      c,a
        add     a,a
        add     a,a
        add     a,c
        add     a,d
        ret

        ;; __cpm3_muladd_small
        ;; Multiply unsigned 16-bit BC by 32-bit constant DE pushed on the stack
        ;; and accumulate the 32-bit result into __cpm3_gtod_acc.
        ;; Entry: BC = multiplicand, stack = return, low-word, high-word.
__cpm3_muladd_bc:
        push    bc
        ld      e,c
        ld      d,b
        ld      hl,#0x0000
        call    __mullong
        pop     bc
        ld      a,(__cpm3_gtod_acc)
        ld      b,a
        ld      a,e
        add     a,b
        ld      (__cpm3_gtod_acc),a
        ld      a,(__cpm3_gtod_acc + 1)
        ld      b,a
        ld      a,d
        adc     a,b
        ld      (__cpm3_gtod_acc + 1),a
        ld      a,(__cpm3_gtod_acc + 2)
        ld      b,a
        ld      a,l
        adc     a,b
        ld      (__cpm3_gtod_acc + 2),a
        ld      a,(__cpm3_gtod_acc + 3)
        ld      b,a
        ld      a,h
        adc     a,b
        ld      (__cpm3_gtod_acc + 3),a
        ret

_gettimeofday::
        ld      (__cpm3_gtod_acc),hl
        push    hl
        push    ix
        push    iy
        ld      de,#__cpm3_gtod_ct
        ld      c,#T_GET
        call    BDOS
        pop     iy
        pop     ix
        ld      (__cpm3_gtod_sec),a
        pop     hl
        xor     a
        ld      (__cpm3_gtod_acc),a
        ld      (__cpm3_gtod_acc + 1),a
        ld      (__cpm3_gtod_acc + 2),a
        ld      (__cpm3_gtod_acc + 3),a
        ld      a,(__cpm3_gtod_ct)
        add     a,#0x69
        ld      c,a
        ld      a,(__cpm3_gtod_ct + 1)
        adc     a,#0x0b
        ld      b,a
        ld      de,#0x0001
        push    de
        ld      de,#0x5180
        push    de
        call    __cpm3_muladd_bc
        pop     de
        pop     de
        ld      a,(__cpm3_gtod_ct + 2)
        call    __cpm3_bcd2bin_a
        ld      c,a
        ld      b,#0x00
        ld      de,#0x0000
        push    de
        ld      de,#0x0e10
        push    de
        call    __cpm3_muladd_bc
        pop     de
        pop     de
        ld      a,(__cpm3_gtod_ct + 3)
        call    __cpm3_bcd2bin_a
        ld      c,a
        ld      b,#0x00
        ld      de,#0x0000
        push    de
        ld      de,#0x003c
        push    de
        call    __cpm3_muladd_bc
        pop     de
        pop     de
        ld      a,(__cpm3_gtod_sec)
        call    __cpm3_bcd2bin_a
        ld      b,a
        ld      a,(__cpm3_gtod_acc)
        add     a,b
        ld      (__cpm3_gtod_acc),a
        ld      a,(__cpm3_gtod_acc + 1)
        adc     a,#0x00
        ld      (__cpm3_gtod_acc + 1),a
        ld      a,(__cpm3_gtod_acc + 2)
        adc     a,#0x00
        ld      (__cpm3_gtod_acc + 2),a
        ld      a,(__cpm3_gtod_acc + 3)
        adc     a,#0x00
        ld      (__cpm3_gtod_acc + 3),a
        ld      a,(__cpm3_gtod_acc)
        ld      (hl),a
        inc     hl
        ld      a,(__cpm3_gtod_acc + 1)
        ld      (hl),a
        inc     hl
        ld      a,(__cpm3_gtod_acc + 2)
        ld      (hl),a
        inc     hl
        ld      a,(__cpm3_gtod_acc + 3)
        ld      (hl),a
        inc     hl
        xor     a
        ld      (hl),a
        inc     hl
        ld      (hl),a
        inc     hl
        ld      (hl),a
        inc     hl
        ld      (hl),a
        ld      de,#0x0000
        ret
