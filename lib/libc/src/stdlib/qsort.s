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

        .area   _DATA
__qsort_base:
        .dw     0
__qsort_count:
        .dw     0
__qsort_size:
        .dw     0
__qsort_cmp:
        .dw     0
__qsort_i:
        .dw     0
__qsort_j:
        .dw     0
__qsort_lhs:
        .dw     0
__qsort_rhs:
        .dw     0
__qsort_tmp:
        .db     0

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
        ld      (__qsort_tmp),a
        ld      a,(hl)
        ld      (de),a
        ld      a,(__qsort_tmp)
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
        ld      (__qsort_base),hl
        ld      (__qsort_count),de
        ld      l,4(ix)
        ld      h,5(ix)
        ld      (__qsort_size),hl
        ld      l,6(ix)
        ld      h,7(ix)
        ld      (__qsort_cmp),hl
        ld      hl,#1
        ld      (__qsort_i),hl

qsort_outer:
        ld      de,(__qsort_i)
        ld      hl,(__qsort_count)
        or      a
        sbc     hl,de                   ; count - i
        jr      z,qsort_done
        jr      c,qsort_done
        ex      de,hl                   ; DE = remaining span (unused), HL = i
        ld      hl,(__qsort_i)
        ld      (__qsort_j),hl

qsort_inner:
        ld      hl,(__qsort_j)
        ld      a,h
        or      l
        jr      z,qsort_next_i

        ld      de,(__qsort_size)
        call    __mul16                 ; DE = j * size
        ex      de,hl
        ld      de,(__qsort_base)
        add     hl,de
        ld      (__qsort_rhs),hl

        ld      hl,(__qsort_rhs)
        ld      de,(__qsort_size)
        or      a
        sbc     hl,de
        ld      (__qsort_lhs),hl

        ld      bc,(__qsort_cmp)
        ld      hl,(__qsort_lhs)
        ld      de,(__qsort_rhs)
        call    __sdcc_call_bc
        bit     7,d
        jr      nz,qsort_next_i         ; compar(lhs,rhs) < 0
        ld      a,d
        or      e
        jr      z,qsort_next_i          ; compar(lhs,rhs) <= 0, inner loop done

qsort_do_swap:
        ld      hl,(__qsort_lhs)
        ld      de,(__qsort_rhs)
        ld      bc,(__qsort_size)
        call    __qsort_swap_bytes
        ld      hl,(__qsort_j)
        dec     hl
        ld      (__qsort_j),hl
        jr      qsort_inner

qsort_next_i:
        ld      hl,(__qsort_i)
        inc     hl
        ld      (__qsort_i),hl
        jr      qsort_outer

qsort_done:
        pop     ix
        ret
