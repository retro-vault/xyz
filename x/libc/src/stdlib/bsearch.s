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

        .area   _CODE

_bsearch::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      b,h
        ld      c,l                       ; preserve key across frame setup
        ld      hl,#-16
        add     hl,sp
        ld      sp,hl

        ld      a,b
        or      c
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

        ld      -16(ix),c
        ld      -15(ix),b               ; key
        ld      -14(ix),e
        ld      -13(ix),d               ; base
        ld      l,6(ix)
        ld      h,7(ix)
        ld      -12(ix),l
        ld      -11(ix),h               ; size
        ld      l,8(ix)
        ld      h,9(ix)
        ld      -10(ix),l
        ld      -9(ix),h                ; cmp
        ld      hl,#0
        ld      -8(ix),l
        ld      -7(ix),h                ; low
        ld      l,4(ix)
        ld      h,5(ix)
        ld      -6(ix),l
        ld      -5(ix),h                ; high

bsearch_loop:
        ld      e,-8(ix)
        ld      d,-7(ix)
        ld      l,-6(ix)
        ld      h,-5(ix)
        or      a
        sbc     hl,de                   ; high - low
        jp      z,bsearch_fail
        jp      c,bsearch_fail

        ld      l,-6(ix)
        ld      h,-5(ix)
        ld      e,-8(ix)
        ld      d,-7(ix)
        or      a
        sbc     hl,de                   ; HL = high - low
        srl     h
        rr      l                       ; HL = (high - low) / 2
        ld      e,-8(ix)
        ld      d,-7(ix)
        add     hl,de
        ld      -4(ix),l
        ld      -3(ix),h                ; mid

        ld      e,-12(ix)
        ld      d,-11(ix)
        call    __mul16                 ; DE = mid * size
        ex      de,hl
        ld      e,-14(ix)
        ld      d,-13(ix)
        add     hl,de
        ld      -2(ix),l
        ld      -1(ix),h                ; entry

        ld      c,-10(ix)
        ld      b,-9(ix)
        ld      l,-16(ix)
        ld      h,-15(ix)
        ld      e,-2(ix)
        ld      d,-1(ix)
        call    __sdcc_call_bc

        ld      a,d
        or      e
        jp      z,bsearch_found
        bit     7,d
        jp      z,bsearch_go_high
        ld      l,-4(ix)
        ld      h,-3(ix)
        ld      -6(ix),l
        ld      -5(ix),h                ; high = mid
        jp      bsearch_loop

bsearch_go_high:
        ld      l,-4(ix)
        ld      h,-3(ix)
        inc     hl
        ld      -8(ix),l
        ld      -7(ix),h                ; low = mid + 1
        jp      bsearch_loop

bsearch_found:
        ld      e,-2(ix)
        ld      d,-1(ix)
        ld      sp,ix
        pop     ix
        ret

bsearch_fail:
        ld      de,#0
        ld      sp,ix
        pop     ix
        ret
