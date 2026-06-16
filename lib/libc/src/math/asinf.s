        ;; asinf.s
        ;; Split from invtrigf.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module asinf
        .optsdcc -mz80 sdcccall(1)

        .globl  _asinf
        .globl  ___fsmul
        .globl  ___fssub
        .globl  _atan2f
        .globl  _sqrtf

IT_XHI  .equ -6
IT_XLO  .equ -8

        .area   _CODE
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
