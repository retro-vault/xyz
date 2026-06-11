        ;; qsort.s
        ;;
        ;; libc qsort() for the xcc Z80 libc.
        ;; Keeps the tiny insertion-sort implementation from the original C
        ;; code because it is compact and works well on the small benchmark and
        ;; freestanding data sets this libc targets today.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module qsort
        .optsdcc -mz80 sdcccall(1)

        .globl  _qsort
        .globl  __mul16
        .globl  __sdcc_call_bc

        .area   _CODE

;; Swap size bytes between lhs and rhs.
;;   HL = lhs
;;   DE = rhs
;;   BC = size
__qsort_swap_bytes:
        ld      a,b
        or      c
        ret     z
qsort_swap_loop:
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
        jr      nz,qsort_swap_loop
        ret

_qsort::
        push    ix
        ld      ix,#0
        add     ix,sp

        ld      a,h
        or      l
        jp      z,qsort_done
        ld      a,6(ix)
        or      7(ix)
        jp      z,qsort_done
        ld      a,4(ix)
        or      5(ix)
        jp      z,qsort_done
        ld      a,d
        or      a
        jr      nz,qsort_setup
        ld      a,e
        cp      #2
        jp      c,qsort_done

qsort_setup:
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

qsort_outer:
        ld      e,-8(ix)
        ld      d,-7(ix)
        ld      l,-14(ix)
        ld      h,-13(ix)
        or      a
        sbc     hl,de                   ; count - i
        jp      z,qsort_done
        jp      c,qsort_done
        ex      de,hl                   ; DE = remaining span (unused), HL = i
        ld      l,-8(ix)
        ld      h,-7(ix)
        ld      -6(ix),l
        ld      -5(ix),h

qsort_inner:
        ld      l,-6(ix)
        ld      h,-5(ix)
        ld      a,h
        or      l
        jr      z,qsort_next_i

        ld      e,-12(ix)
        ld      d,-11(ix)
        call    __mul16                 ; DE = j * size
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
        ld      l,-4(ix)
        ld      h,-3(ix)
        ld      e,-2(ix)
        ld      d,-1(ix)
        call    __sdcc_call_bc
        bit     7,d
        jr      nz,qsort_next_i         ; compar(lhs,rhs) < 0
        ld      a,d
        or      e
        jr      z,qsort_next_i          ; compar(lhs,rhs) <= 0, inner loop done

qsort_do_swap:
        ld      l,-4(ix)
        ld      h,-3(ix)
        ld      e,-2(ix)
        ld      d,-1(ix)
        ld      c,-12(ix)
        ld      b,-11(ix)
        call    __qsort_swap_bytes
        ld      l,-6(ix)
        ld      h,-5(ix)
        dec     hl
        ld      -6(ix),l
        ld      -5(ix),h
        jp      qsort_inner

qsort_next_i:
        ld      l,-8(ix)
        ld      h,-7(ix)
        inc     hl
        ld      -8(ix),l
        ld      -7(ix),h
        jp      qsort_outer

qsort_done:
        ld      sp,ix
        pop     ix
        ret
