        ;; snprintf.s
        ;; Split from printf.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module snprintf
        .optsdcc -mz80 sdcccall(0)

        .globl  _snprintf
        .globl  __stdio_alloc_ctx
        .globl  __stdio_init_nstring
        .globl  __stdio_store_ap_hl
        .globl  __stdio_store_fmt_hl
        .globl  __stdio_vformat

        .area   _CODE
_snprintf::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __stdio_alloc_ctx
        ld      l,4(ix)
        ld      h,5(ix)
        ld      e,6(ix)
        ld      d,7(ix)
        call    __stdio_init_nstring
        ld      l,8(ix)
        ld      h,9(ix)
        call    __stdio_store_fmt_hl
        push    ix
        pop     hl
        ld      de,#0x000A
        add     hl,de
        call    __stdio_store_ap_hl
        push    ix
        call    __stdio_vformat
        pop     ix
        push    hl
        pop     de
        ld      sp,ix
        pop     ix
        ret
