        ;; bsearch_sdcc0.s
        ;;
        ;; bsearch entry point for translation units whose comparator uses the
        ;; stack-only sdcccall(0) ABI. The bsearch entry itself retains libc's
        ;; sdcccall(1) ABI.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module bsearch_sdcc0
        .optsdcc -mz80 sdcccall(1)

        .globl  ___bsearch_sdcc0
        .globl  __mul16
        .globl  __sdcc_call_bc

        .area   _CODE

___bsearch_sdcc0::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      b,h
        ld      c,l
        ld      hl,#-16
        add     hl,sp
        ld      sp,hl

        ld      a,b
        or      c
        jp      z,bsearch0_fail
        ld      a,d
        or      e
        jp      z,bsearch0_fail
        ld      a,8(ix)
        or      9(ix)
        jp      z,bsearch0_fail
        ld      a,6(ix)
        or      7(ix)
        jp      z,bsearch0_fail

        ld      -16(ix),c
        ld      -15(ix),b
        ld      -14(ix),e
        ld      -13(ix),d
        ld      l,6(ix)
        ld      h,7(ix)
        ld      -12(ix),l
        ld      -11(ix),h
        ld      l,8(ix)
        ld      h,9(ix)
        ld      -10(ix),l
        ld      -9(ix),h
        ld      hl,#0
        ld      -8(ix),l
        ld      -7(ix),h
        ld      l,4(ix)
        ld      h,5(ix)
        ld      -6(ix),l
        ld      -5(ix),h

bsearch0_loop:
        ld      e,-8(ix)
        ld      d,-7(ix)
        ld      l,-6(ix)
        ld      h,-5(ix)
        or      a
        sbc     hl,de
        jp      z,bsearch0_fail
        jp      c,bsearch0_fail

        ld      l,-6(ix)
        ld      h,-5(ix)
        ld      e,-8(ix)
        ld      d,-7(ix)
        or      a
        sbc     hl,de
        srl     h
        rr      l
        ld      e,-8(ix)
        ld      d,-7(ix)
        add     hl,de
        ld      -4(ix),l
        ld      -3(ix),h

        ld      e,-12(ix)
        ld      d,-11(ix)
        call    __mul16
        ex      de,hl
        ld      e,-14(ix)
        ld      d,-13(ix)
        add     hl,de
        ld      -2(ix),l
        ld      -1(ix),h

        ld      c,-10(ix)
        ld      b,-9(ix)
        ld      e,-2(ix)
        ld      d,-1(ix)
        push    de
        ld      l,-16(ix)
        ld      h,-15(ix)
        push    hl
        call    __sdcc_call_bc
        pop     bc
        pop     bc

        ld      a,h
        or      l
        jp      z,bsearch0_found
        bit     7,h
        jp      z,bsearch0_go_high
        ld      l,-4(ix)
        ld      h,-3(ix)
        ld      -6(ix),l
        ld      -5(ix),h
        jp      bsearch0_loop

bsearch0_go_high:
        ld      l,-4(ix)
        ld      h,-3(ix)
        inc     hl
        ld      -8(ix),l
        ld      -7(ix),h
        jp      bsearch0_loop

bsearch0_found:
        ld      e,-2(ix)
        ld      d,-1(ix)
        ld      sp,ix
        pop     ix
        ret

bsearch0_fail:
        ld      de,#0
        ld      sp,ix
        pop     ix
        ret
