        ;; qsort_sdcc0.s
        ;;
        ;; qsort entry point for translation units whose comparator uses the
        ;; stack-only sdcccall(0) ABI. The qsort entry itself retains libc's
        ;; sdcccall(1) ABI.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module qsort_sdcc0
        .optsdcc -mz80 sdcccall(1)

        .globl  ___qsort_sdcc0
        .globl  __mul16
        .globl  __sdcc_call_bc

        .area   _CODE

;; Swap size bytes between lhs and rhs.
;;   HL = lhs
;;   DE = rhs
;;   BC = size
__qsort0_swap_bytes:
        ld      a,b
        or      c
        ret     z
qsort0_swap_loop:
        ld      a,(de)
        push    af
        ld      a,(hl)
        ld      (de),a
        pop     af
        ld      (hl),a
        inc     hl
        inc     de
        dec     bc
        ld      a,b
        or      c
        jr      nz,qsort0_swap_loop
        ret

___qsort_sdcc0::
        push    ix
        ld      ix,#0
        add     ix,sp

        ld      a,h
        or      l
        jp      z,qsort0_done
        ld      a,6(ix)
        or      7(ix)
        jp      z,qsort0_done
        ld      a,4(ix)
        or      5(ix)
        jp      z,qsort0_done
        ld      a,d
        or      a
        jr      nz,qsort0_setup
        ld      a,e
        cp      #2
        jp      c,qsort0_done

qsort0_setup:
        ld      bc,#-16
        add     ix,bc
        ld      sp,ix
        ld      bc,#16
        add     ix,bc
        ld      -16(ix),l
        ld      -15(ix),h
        ld      -14(ix),e
        ld      -13(ix),d
        ld      l,4(ix)
        ld      h,5(ix)
        ld      -12(ix),l
        ld      -11(ix),h
        ld      l,6(ix)
        ld      h,7(ix)
        ld      -10(ix),l
        ld      -9(ix),h
        ld      hl,#1
        ld      -8(ix),l
        ld      -7(ix),h

qsort0_outer:
        ld      e,-8(ix)
        ld      d,-7(ix)
        ld      l,-14(ix)
        ld      h,-13(ix)
        or      a
        sbc     hl,de
        jp      z,qsort0_done
        jp      c,qsort0_done
        ld      l,-8(ix)
        ld      h,-7(ix)
        ld      -6(ix),l
        ld      -5(ix),h

qsort0_inner:
        ld      l,-6(ix)
        ld      h,-5(ix)
        ld      a,h
        or      l
        jr      z,qsort0_next_i

        ld      e,-12(ix)
        ld      d,-11(ix)
        call    __mul16
        ex      de,hl
        ld      e,-16(ix)
        ld      d,-15(ix)
        add     hl,de
        ld      -2(ix),l
        ld      -1(ix),h

        ld      l,-2(ix)
        ld      h,-1(ix)
        ld      e,-12(ix)
        ld      d,-11(ix)
        or      a
        sbc     hl,de
        ld      -4(ix),l
        ld      -3(ix),h

        ld      c,-10(ix)
        ld      b,-9(ix)
        ld      e,-2(ix)
        ld      d,-1(ix)
        push    de
        ld      l,-4(ix)
        ld      h,-3(ix)
        push    hl
        call    __sdcc_call_bc
        pop     bc
        pop     bc
        bit     7,h
        jr      nz,qsort0_next_i
        ld      a,h
        or      l
        jr      z,qsort0_next_i

        ld      l,-4(ix)
        ld      h,-3(ix)
        ld      e,-2(ix)
        ld      d,-1(ix)
        ld      c,-12(ix)
        ld      b,-11(ix)
        call    __qsort0_swap_bytes
        ld      l,-6(ix)
        ld      h,-5(ix)
        dec     hl
        ld      -6(ix),l
        ld      -5(ix),h
        jp      qsort0_inner

qsort0_next_i:
        ld      l,-8(ix)
        ld      h,-7(ix)
        inc     hl
        ld      -8(ix),l
        ld      -7(ix),h
        jp      qsort0_outer

qsort0_done:
        ld      sp,ix
        pop     ix
        ret
