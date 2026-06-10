        ;; bsearch.s
        ;;
        ;; libc bsearch() for the xcc Z80 libc.
        ;; Uses the standard callback ABI and works over arbitrary element
        ;; widths by computing entry addresses as base + (mid * size).
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module bsearch
        .optsdcc -mz80 sdcccall(1)

        .globl  _bsearch
        .globl  __mul16
        .globl  __sdcc_call_bc

        .area   _DATA
__bsearch_key:
        .dw     0
__bsearch_base:
        .dw     0
__bsearch_size:
        .dw     0
__bsearch_cmp:
        .dw     0
__bsearch_low:
        .dw     0
__bsearch_high:
        .dw     0
__bsearch_mid:
        .dw     0
__bsearch_entry:
        .dw     0

        .area   _CODE

_bsearch::
        push    ix
        ld      ix,#0
        add     ix,sp

        ld      a,h
        or      l
        jp      z,bsearch_fail
        ld      a,d
        or      e
        jp      z,bsearch_fail
        ld      a,8(ix)
        or      9(ix)
        jp      z,bsearch_fail
        ld      a,6(ix)
        or      7(ix)
        jp      z,bsearch_fail

        ld      (__bsearch_key),hl
        ld      (__bsearch_base),de
        ld      l,6(ix)
        ld      h,7(ix)
        ld      (__bsearch_size),hl
        ld      l,8(ix)
        ld      h,9(ix)
        ld      (__bsearch_cmp),hl
        ld      hl,#0
        ld      (__bsearch_low),hl
        ld      l,4(ix)
        ld      h,5(ix)
        ld      (__bsearch_high),hl

bsearch_loop:
        ld      de,(__bsearch_low)
        ld      hl,(__bsearch_high)
        or      a
        sbc     hl,de                   ; high - low
        jr      z,bsearch_fail
        jr      c,bsearch_fail

        ld      hl,(__bsearch_high)
        ld      de,(__bsearch_low)
        or      a
        sbc     hl,de                   ; HL = high - low
        srl     h
        rr      l                       ; HL = (high - low) / 2
        ld      de,(__bsearch_low)
        add     hl,de
        ld      (__bsearch_mid),hl

        ld      de,(__bsearch_size)
        call    __mul16                 ; DE = mid * size
        ex      de,hl
        ld      de,(__bsearch_base)
        add     hl,de
        ld      (__bsearch_entry),hl

        ld      bc,(__bsearch_cmp)
        ld      hl,(__bsearch_key)
        ld      de,(__bsearch_entry)
        call    __sdcc_call_bc

        ld      a,d
        or      e
        jr      z,bsearch_found
        bit     7,d
        jr      z,bsearch_go_high
        ld      hl,(__bsearch_mid)
        ld      (__bsearch_high),hl
        jr      bsearch_loop

bsearch_go_high:
        ld      hl,(__bsearch_mid)
        inc     hl
        ld      (__bsearch_low),hl
        jr      bsearch_loop

bsearch_found:
        ld      de,(__bsearch_entry)
        pop     ix
        ret

bsearch_fail:
        ld      de,#0
        pop     ix
        ret
