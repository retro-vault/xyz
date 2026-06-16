        ;; vsnprintf.s
        ;; Split from printf.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module vsnprintf
        .optsdcc -mz80 sdcccall(1)

        .globl  _vsnprintf
        .globl  __stdio_alloc_ctx
        .globl  __stdio_init_nstring
        .globl  __stdio_store_ap_hl
        .globl  __stdio_store_fmt_hl
        .globl  __stdio_vformat

        .area   _CODE
_vsnprintf::
        push    de
        push    hl
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __stdio_alloc_ctx
        ld      l,2(ix)
        ld      h,3(ix)
        ld      e,4(ix)
        ld      d,5(ix)
        call    __stdio_init_nstring
        ld      l,8(ix)
        ld      h,9(ix)
        call    __stdio_store_fmt_hl
        ld      l,10(ix)
        ld      h,11(ix)
        call    __stdio_store_ap_hl
        push    ix
        call    __stdio_vformat
        pop     ix
        push    hl
        pop     de
        ld      sp,ix
        pop     ix
        pop     bc
        pop     bc
        ret

