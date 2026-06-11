        ;; asinhf.s
        ;;
        ;; libc asinhf for the xcc Z80 libc.
        ;;
        ;; Use the stable odd form
        ;;   asinh(x) = sign(x) * log(|x| + sqrt(|x|^2 + 1))
        ;; so large negative magnitudes do not collapse into the
        ;; cancellation-prone x + sqrt(x*x + 1) form.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module asinhf
        .optsdcc -mz80 sdcccall(1)

        .globl  _asinhf
        .globl  ___libc_fpclassifyf
        .globl  _sqrtf
        .globl  _logf
        .globl  ___fsadd
        .globl  ___fsmul

        .area   _CODE

_asinhf::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      c,l
        ld      b,h
        ld      hl,#-13
        add     hl,sp
        ld      sp,hl
        ld      -4(ix),e
        ld      -3(ix),d
        ld      -2(ix),c
        ld      -1(ix),b
        ld      e,-4(ix)
        ld      d,-3(ix)
        ld      l,-2(ix)
        ld      h,-1(ix)
        call    ___libc_fpclassifyf
        ld      a,e
        cp      #0                      ; NaN -> preserve payload/sign
        jp      z,asinhf_ret_x
        cp      #1                      ; +/-Inf -> preserve infinity sign
        jp      z,asinhf_ret_x
        cp      #2                      ; +/-0 -> preserve signed zero
        jp      z,asinhf_ret_x

        ld      a,-1(ix)
        and     #0x80
        ld      -13(ix),a
        ld      a,-1(ix)
        and     #0x7f
        ld      -5(ix),a
        ld      a,-4(ix)
        ld      -8(ix),a
        ld      a,-3(ix)
        ld      -7(ix),a
        ld      a,-2(ix)
        ld      -6(ix),a

        ;; tmp = |x| * |x|
        ld      l,-6(ix)
        ld      h,-5(ix)
        push    hl
        ld      l,-8(ix)
        ld      h,-7(ix)
        push    hl
        ld      e,-8(ix)
        ld      d,-7(ix)
        ld      l,-6(ix)
        ld      h,-5(ix)
        call    ___fsmul
        pop     bc
        pop     bc
        ld      -12(ix),e
        ld      -11(ix),d
        ld      -10(ix),l
        ld      -9(ix),h

        ;; tmp = |x|^2 + 1
        ld      hl,#0x3f80              ; 1.0f
        push    hl
        ld      hl,#0x0000
        push    hl
        ld      e,-12(ix)
        ld      d,-11(ix)
        ld      l,-10(ix)
        ld      h,-9(ix)
        call    ___fsadd
        pop     bc
        pop     bc

        ;; tmp = sqrt(|x|^2 + 1)
        call    _sqrtf
        ld      -12(ix),e
        ld      -11(ix),d
        ld      -10(ix),l
        ld      -9(ix),h

        ;; tmp = |x| + sqrt(|x|^2 + 1)
        ld      l,-10(ix)
        ld      h,-9(ix)
        push    hl
        ld      l,-12(ix)
        ld      h,-11(ix)
        push    hl
        ld      e,-8(ix)
        ld      d,-7(ix)
        ld      l,-6(ix)
        ld      h,-5(ix)
        call    ___fsadd
        pop     bc
        pop     bc

        ;; log(tmp), then restore the original sign.
        call    _logf
        ld      a,-13(ix)
        or      a
        jr      z,asinhf_done
        set     7,h
asinhf_done:
        ld      sp,ix
        pop     ix
        ret

asinhf_ret_x:
        ld      e,-4(ix)
        ld      d,-3(ix)
        ld      l,-2(ix)
        ld      h,-1(ix)
        ld      sp,ix
        pop     ix
        ret
