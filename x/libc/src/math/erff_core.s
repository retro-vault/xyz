        ;; erff_core.s
        ;;
        ;; Shared single-precision error-function kernel for the xcc Z80 libc.
        ;;
        ;; Uses Winitzki's compact approximation
        ;;   erf(x) ≈ sign(x) * sqrt(1 - exp(-x² * ((4/π) + a x²)/(1 + a x²)))
        ;; with a = 0.147. It stays small, uses only the existing soft-float
        ;; helpers, and is accurate enough for libc-grade scalar math tests.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module erff_core
        .optsdcc -mz80 sdcccall(1)

        .globl  __libc_erff_core
        .globl  ___libc_fpclassifyf
        .globl  ___fsmul
        .globl  ___fsadd
        .globl  ___fssub
        .globl  ___fsdiv
        .globl  _expf
        .globl  _sqrtf

        .area   _CODE

__erf_load_x:
        ld      e,-17(ix)
        ld      d,-16(ix)
        ld      l,-15(ix)
        ld      h,-14(ix)
        ret

__erf_load_x2:
        ld      e,-13(ix)
        ld      d,-12(ix)
        ld      l,-11(ix)
        ld      h,-10(ix)
        ret

__erf_load_t:
        ld      e,-9(ix)
        ld      d,-8(ix)
        ld      l,-7(ix)
        ld      h,-6(ix)
        ret

__erf_load_u:
        ld      e,-5(ix)
        ld      d,-4(ix)
        ld      l,-3(ix)
        ld      h,-2(ix)
        ret

__libc_erff_core::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      c,l
        ld      b,h
        ld      hl,#-17
        add     hl,sp
        ld      sp,hl
        ld      -17(ix),e
        ld      -16(ix),d
        ld      -15(ix),c
        ld      -14(ix),b
        call    __erf_load_x
        call    ___libc_fpclassifyf
        ld      a,e
        cp      #0                      ; NaN -> preserve payload/sign.
        jp      z,__erf_ret_x
        cp      #1                      ; +/-Inf -> saturate to +/-1.
        jp      z,__erf_inf
        cp      #2                      ; Signed zero is already exact.
        jp      z,__erf_ret_x

        ld      a,-14(ix)
        and     #0x80
        ld      -1(ix),a
        ld      a,-14(ix)
        and     #0x7f
        ld      -14(ix),a

        ;; x2 = x * x
        ld      l,-15(ix)
        ld      h,-14(ix)
        push    hl
        ld      l,-17(ix)
        ld      h,-16(ix)
        push    hl
        call    __erf_load_x
        call    ___fsmul
        pop     bc
        pop     bc
        ld      -13(ix),e
        ld      -12(ix),d
        ld      -11(ix),l
        ld      -10(ix),h

        ;; t = a * x^2
        ld      hl,#0x3e16              ; a = 0.147
        push    hl
        ld      hl,#0x872b
        push    hl
        call    __erf_load_x2
        call    ___fsmul
        pop     bc
        pop     bc
        ld      -9(ix),e
        ld      -8(ix),d
        ld      -7(ix),l
        ld      -6(ix),h

        ;; u = x^2 * ((4/pi) + t)
        ld      l,-7(ix)
        ld      h,-6(ix)
        push    hl
        ld      l,-9(ix)
        ld      h,-8(ix)
        push    hl
        ld      de,#0xf983
        ld      hl,#0x3fa2              ; 4/pi
        call    ___fsadd
        pop     bc
        pop     bc
        ld      -5(ix),e
        ld      -4(ix),d
        ld      -3(ix),l
        ld      -2(ix),h

        ld      l,-3(ix)
        ld      h,-2(ix)
        push    hl
        ld      l,-5(ix)
        ld      h,-4(ix)
        push    hl
        call    __erf_load_x2
        call    ___fsmul
        pop     bc
        pop     bc
        ld      -5(ix),e
        ld      -4(ix),d
        ld      -3(ix),l
        ld      -2(ix),h

        ;; t = 1 + a*x^2
        ld      l,-7(ix)
        ld      h,-6(ix)
        push    hl
        ld      l,-9(ix)
        ld      h,-8(ix)
        push    hl
        ld      de,#0x0000
        ld      hl,#0x3f80              ; 1.0f
        call    ___fsadd
        pop     bc
        pop     bc
        ld      -9(ix),e
        ld      -8(ix),d
        ld      -7(ix),l
        ld      -6(ix),h

        ;; u = -u / t
        ld      l,-7(ix)
        ld      h,-6(ix)
        push    hl
        ld      l,-9(ix)
        ld      h,-8(ix)
        push    hl
        call    __erf_load_u
        call    ___fsdiv
        pop     bc
        pop     bc
        ld      a,h
        xor     #0x80
        ld      h,a

        ;; u = exp(u)
        call    _expf
        ld      -5(ix),e
        ld      -4(ix),d
        ld      -3(ix),l
        ld      -2(ix),h

        ;; u = sqrt(1 - u)
        ld      l,-3(ix)
        ld      h,-2(ix)
        push    hl
        ld      l,-5(ix)
        ld      h,-4(ix)
        push    hl
        ld      de,#0x0000
        ld      hl,#0x3f80              ; 1.0f
        call    ___fssub
        pop     bc
        pop     bc
        call    _sqrtf

        ;; Restore the original sign.
        ld      a,-1(ix)
        or      a
        jr      z,__erf_finish
        set     7,h
__erf_finish:
        ld      sp,ix
        pop     ix
        ret

__erf_inf:
        ld      de,#0x0000
        ld      a,-14(ix)
        and     #0x80
        or      #0x3f
        ld      h,a
        ld      l,#0x80                 ; +/-1.0f
        jr      __erf_finish

__erf_ret_x:
        ld      e,-17(ix)
        ld      d,-16(ix)
        ld      l,-15(ix)
        ld      h,-14(ix)
        jr      __erf_finish
