        ;; acosf.s
        ;; Split from invtrigf.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module acosf
        .optsdcc -mz80 sdcccall(1)

        .globl  _acosf
        .globl  ___fsmul
        .globl  ___fssub
        .globl  _atan2f
        .globl  _sqrtf

IT_THI  .equ -2
IT_TLO  .equ -4
IT_XHI  .equ -6
IT_XLO  .equ -8

        .area   _CODE
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
