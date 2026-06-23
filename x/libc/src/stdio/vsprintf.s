        ;; vsprintf.s
        ;; Split from printf.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module vsprintf
        .optsdcc -mz80 sdcccall(1)

        .globl  _vsprintf
        .globl  __stdio_alloc_ctx
        .globl  __stdio_init_string
        .globl  __stdio_store_ap_hl
        .globl  __stdio_store_fmt_hl
        .globl  __stdio_vformat

        .area   _CODE
_vsprintf::
        push    de
        push    hl
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __stdio_alloc_ctx
        ld      l,2(ix)
        ld      h,3(ix)
        call    __stdio_init_string
        ld      l,4(ix)
        ld      h,5(ix)
        call    __stdio_store_fmt_hl
        ld      l,8(ix)
        ld      h,9(ix)
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

