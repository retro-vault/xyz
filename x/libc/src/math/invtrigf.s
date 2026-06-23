        ;; invtrigf.s
        ;;
        ;; libc atanf / asinf / acosf for the xcc Z80 libc.
        ;; Built on the existing atan2f / sqrtf kernels plus the float runtime.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih



        .module invtrigf
        .optsdcc -mz80 sdcccall(1)

        .globl  _atanf
        .globl  _atan2f

        .area   _CODE
_atanf::
        ld      b,h
        ld      c,l
        ld      hl,#0x3f80              ; 1.0f high word
        push    hl
        ld      hl,#0x0000
        push    hl                      ; 1.0f low word
        ld      h,b
        ld      l,c
        call    _atan2f
        pop     bc
        pop     bc
        ret

        ;; asinf(x) = atan2f(x, sqrtf(1 - x*x))
