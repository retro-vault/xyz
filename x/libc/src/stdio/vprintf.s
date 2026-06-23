        ;; vprintf.s
        ;; Split from printf.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module vprintf
        .optsdcc -mz80 sdcccall(1)

        .globl  _vprintf
        .globl  __stdio_alloc_ctx
        .globl  __stdio_init_console
        .globl  __stdio_store_ap_hl
        .globl  __stdio_store_fmt_hl
        .globl  __stdio_vformat

        .area   _CODE
_vprintf::
        push    de
        push    hl
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __stdio_alloc_ctx
        push    de
        call    __stdio_init_console
        pop     bc
        ld      l,2(ix)
        ld      h,3(ix)
        call    __stdio_store_fmt_hl
        ld      l,4(ix)
        ld      h,5(ix)
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

