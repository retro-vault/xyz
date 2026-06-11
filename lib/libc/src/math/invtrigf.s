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
        .globl  _asinf
        .globl  _acosf
        .globl  _atan2f
        .globl  _sqrtf
        .globl  ___fsmul
        .globl  ___fssub

        .area   _CODE

IT_XLO  .equ -8
IT_XHI  .equ -6
IT_TLO  .equ -4
IT_THI  .equ -2

        ;; atanf(x) is routed through atan2f(x, 1.0f) so the quadrant logic
        ;; stays in one proven kernel.
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
_asinf::
        ld      b,h
        ld      c,l
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      hl,#-8
        add     hl,sp
        ld      sp,hl
        ld      IT_XLO(ix),e
        ld      IT_XLO+1(ix),d
        ld      IT_XHI(ix),c
        ld      IT_XHI+1(ix),b
        ;; Form x*x first so the domain check naturally falls out of sqrtf.
        ld      l,IT_XHI(ix)
        ld      h,IT_XHI+1(ix)
        push    hl
        ld      e,IT_XLO(ix)
        ld      d,IT_XLO+1(ix)
        push    de
        ld      e,IT_XLO(ix)
        ld      d,IT_XLO+1(ix)
        ld      l,IT_XHI(ix)
        ld      h,IT_XHI+1(ix)
        call    ___fsmul
        pop     bc
        pop     bc
        ;; Transform to 1 - x*x for the sqrtf leg of the identity.
        push    hl
        push    de
        ld      de,#0x0000
        ld      hl,#0x3f80              ; 1.0f
        call    ___fssub
        pop     bc
        pop     bc
        call    _sqrtf
        ;; atan2f takes y in HL:DE and x on the stack.
        push    hl
        push    de
        ld      e,IT_XLO(ix)
        ld      d,IT_XLO+1(ix)
        ld      l,IT_XHI(ix)
        ld      h,IT_XHI+1(ix)
        call    _atan2f
        pop     bc
        pop     bc
        ld      sp,ix
        pop     ix
        ret

        ;; acosf(x) = atan2f(sqrtf(1 - x*x), x)
_acosf::
        ld      b,h
        ld      c,l
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      hl,#-8
        add     hl,sp
        ld      sp,hl
        ld      IT_XLO(ix),e
        ld      IT_XLO+1(ix),d
        ld      IT_XHI(ix),c
        ld      IT_XHI+1(ix),b
        ;; Reuse the same 1 - x*x reduction as asinf.
        ld      l,IT_XHI(ix)
        ld      h,IT_XHI+1(ix)
        push    hl
        ld      e,IT_XLO(ix)
        ld      d,IT_XLO+1(ix)
        push    de
        ld      e,IT_XLO(ix)
        ld      d,IT_XLO+1(ix)
        ld      l,IT_XHI(ix)
        ld      h,IT_XHI+1(ix)
        call    ___fsmul
        pop     bc
        pop     bc
        push    hl
        push    de
        ld      de,#0x0000
        ld      hl,#0x3f80              ; 1.0f
        call    ___fssub
        pop     bc
        pop     bc
        call    _sqrtf
        ld      IT_TLO(ix),e
        ld      IT_TLO+1(ix),d
        ld      IT_THI(ix),l
        ld      IT_THI+1(ix),h
        ;; For acosf the reduced root becomes y and the original x is stacked.
        ld      l,IT_XHI(ix)
        ld      h,IT_XHI+1(ix)
        push    hl
        ld      e,IT_XLO(ix)
        ld      d,IT_XLO+1(ix)
        push    de
        ld      e,IT_TLO(ix)
        ld      d,IT_TLO+1(ix)
        ld      l,IT_THI(ix)
        ld      h,IT_THI+1(ix)
        call    _atan2f
        pop     bc
        pop     bc
        ld      sp,ix
        pop     ix
        ret
