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

        .area   _DATA
__asinhf_x:      .ds 4
__asinhf_abs:    .ds 4
__asinhf_tmp:    .ds 4
__asinhf_sign:   .ds 1

        .area   _CODE

_asinhf::
        ld      (__asinhf_x),de
        ld      (__asinhf_x + 2),hl
        call    ___libc_fpclassifyf
        ld      a,e
        cp      #0                      ; NaN -> preserve payload/sign
        jr      z,asinhf_ret_x
        cp      #1                      ; +/-Inf -> preserve infinity sign
        jr      z,asinhf_ret_x
        cp      #2                      ; +/-0 -> preserve signed zero
        jr      z,asinhf_ret_x

        ld      a,h
        and     #0x80
        ld      (__asinhf_sign),a
        res     7,h
        ld      (__asinhf_abs),de
        ld      (__asinhf_abs + 2),hl

        ;; tmp = |x| * |x|
        ld      hl,(__asinhf_abs + 2)
        push    hl
        ld      hl,(__asinhf_abs)
        push    hl
        ld      de,(__asinhf_abs)
        ld      hl,(__asinhf_abs + 2)
        call    ___fsmul
        pop     bc
        pop     bc
        ld      (__asinhf_tmp),de
        ld      (__asinhf_tmp + 2),hl

        ;; tmp = |x|^2 + 1
        ld      hl,#0x3f80              ; 1.0f
        push    hl
        ld      hl,#0x0000
        push    hl
        ld      de,(__asinhf_tmp)
        ld      hl,(__asinhf_tmp + 2)
        call    ___fsadd
        pop     bc
        pop     bc

        ;; tmp = sqrt(|x|^2 + 1)
        call    _sqrtf
        ld      (__asinhf_tmp),de
        ld      (__asinhf_tmp + 2),hl

        ;; tmp = |x| + sqrt(|x|^2 + 1)
        ld      hl,(__asinhf_tmp + 2)
        push    hl
        ld      hl,(__asinhf_tmp)
        push    hl
        ld      de,(__asinhf_abs)
        ld      hl,(__asinhf_abs + 2)
        call    ___fsadd
        pop     bc
        pop     bc

        ;; log(tmp), then restore the original sign.
        call    _logf
        ld      a,(__asinhf_sign)
        or      a
        ret     z
        set     7,h
        ret

asinhf_ret_x:
        ld      de,(__asinhf_x)
        ld      hl,(__asinhf_x + 2)
        ret
